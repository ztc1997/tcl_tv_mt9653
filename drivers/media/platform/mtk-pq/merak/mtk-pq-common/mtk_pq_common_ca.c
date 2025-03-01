// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */
#if IS_ENABLED(CONFIG_OPTEE)
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/timer.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/interrupt.h>
#include <linux/of.h>
#include <linux/types.h>
#include <linux/videodev2.h>
#include <linux/timekeeping.h>
#include <linux/genalloc.h>
#include <linux/platform_device.h>
#include <media/v4l2-mem2mem.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ioctl.h>
#include <media/videobuf2-v4l2.h>
#include <media/videobuf2-vmalloc.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-event.h>
#include "mtk_pq.h"
#include "mtk_pq_enhance.h"
#include "mtk_pq_common_ca.h"

static void uuid_to_octets(uint8_t d[TEE_IOCTL_UUID_LEN], const TEEC_UUID *s)
{
	d[0] = s->timeLow >> 24;
	d[1] = s->timeLow >> 16;
	d[2] = s->timeLow >> 8;
	d[3] = s->timeLow;
	d[4] = s->timeMid >> 8;
	d[5] = s->timeMid;
	d[6] = s->timeHiAndVersion >> 8;
	d[7] = s->timeHiAndVersion;
	memcpy(d + 8, s->clockSeqAndNode, sizeof(s->clockSeqAndNode));
}

static int _optee_match(struct tee_ioctl_version_data *data, const void *vers)
{
	return 1;
}

static int _mtk_pq_common_ca_get_optee_version(struct mtk_pq_device *pq_dev)
{
	if (pq_dev->display_info.secure_info.optee_version != -1)
		goto final;
#if (DYNAMIC_OPTEE_VERSION_SUPPORT)
	if (pq_dev->display_info.secure_info.optee_version == -1) {
		struct file *verfile = NULL;
		char *sVer = NULL;
		int u32ReadCount = 0;
		mm_segment_t cur_mm_seg;
		long lFileSize;
		loff_t pos;

		sVer = malloc(sizeof(char) * BOOTARG_SIZE);
		if (sVer == NULL)
			goto final;
		cur_mm_seg = get_fs();
		set_fs(KERNEL_DS);
		verfile = filp_open("/proc/tz2_mstar/version", O_RDONLY, 0);
		if (verfile == NULL || IS_ERR_OR_NULL(verfile)) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
				"[PQ SVP] can't open /proc/tz2_mstar/version\n");
			pq_dev->display_info.secure_info.optee_version = MTK_OPTEE_VER_1;
			set_fs(cur_mm_seg);
			kfree(sVer);
			goto final;
		}
		pos = vfs_llseek(verfile, 0, SEEK_SET);
		u32ReadCount = kernel_read(verfile, sVer, BOOTARG_SIZE, &pos);
		set_fs(cur_mm_seg);
		if (u32ReadCount > BOOTARG_SIZE) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
				"[PQ SVP] cmdline info more than buffer size\n");
			goto exit;
		}
		sVer[BOOTARG_SIZE - BOOTARG_OFFSET_2] = '\n';
		sVer[BOOTARG_SIZE - BOOTARG_OFFSET_1] = '\0';
		if (strncmp(sVer, "2.4", BOOTARG_CMP_SIZE) == 0)
			pq_dev->display_info.secure_info.optee_version = MTK_OPTEE_VER_2;
		else if (strncmp(sVer, "3.2", BOOTARG_CMP_SIZE) == 0)
			pq_dev->display_info.secure_info.optee_version = MTK_OPTEE_VER_3;
		else
			pq_dev->display_info.secure_info.optee_version = MTK_OPTEE_VER_1;
exit:
		if (verfile != NULL) {
			cur_mm_seg = get_fs();
			set_fs(KERNEL_DS);
			filp_close(verfile, NULL);
			set_fs(cur_mm_seg);
		}
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_SVP, "OPTEE Version %s OPTEE_VERSION %d\n",
				sVer, pq_dev->display_info.secure_info.optee_version);
		kfree(sVer);
	}
#endif
final:
	return pq_dev->display_info.secure_info.optee_version;
}

static void _mtk_pq_common_ca_teec_free_temp_refs(TEEC_Operation *operation,
			TEEC_SharedMemory *shms)
{
	size_t n = 0;
	uint32_t param_type = 0;
	TEEC_SharedMemory *free_shm = NULL;

	if (!operation)
		return;
	for (n = 0; n < TEEC_CONFIG_PAYLOAD_REF_COUNT; n++) {
		param_type = TEEC_PARAM_TYPE_GET(operation->paramTypes, n);
		switch (param_type) {
		case TEEC_MEMREF_TEMP_INPUT:
		case TEEC_MEMREF_TEMP_OUTPUT:
		case TEEC_MEMREF_TEMP_INOUT:
			free_shm = shms + n;
			tee_shm_free((struct tee_shm *)free_shm->shadow_buffer);
			free_shm->id = -1;
			free_shm->shadow_buffer = NULL;
			free_shm->buffer = NULL;
			free_shm->registered_fd = -1;
			free_shm->buffer_allocated = false;
			break;
		case TEEC_NONE:
		case TEEC_VALUE_INPUT:
		case TEEC_VALUE_OUTPUT:
		case TEEC_VALUE_INOUT:
		case TEEC_MEMREF_WHOLE:
		case TEEC_MEMREF_PARTIAL_INPUT:
		case TEEC_MEMREF_PARTIAL_OUTPUT:
		case TEEC_MEMREF_PARTIAL_INOUT:
			break;
		default:
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "invalid param_type : %x\n", param_type);
			break;
		}
	}
}

int _mtk_pq_common_ca_teec_alloc_shm(TEEC_Context *ctx, TEEC_SharedMemory *shm)
{
	struct tee_shm *_shm = NULL;
	void *shm_va = NULL;
	size_t len = shm->size;
	struct tee_context *context = (struct tee_context *) ctx->fd;

	if (!len)
		len = DEFAULT_SHM_SIZE;
	if (len < PAGE_SIZE)
		_shm = tee_shm_alloc(context, len, TEE_SHM_MAPPED);
	else
		_shm = tee_shm_alloc(context, len, TEE_SHM_MAPPED | TEE_SHM_DMA_BUF);
	if (!_shm) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "tee shm alloc got NULL\n");
		return -ENOMEM;
	}
	if (IS_ERR(_shm)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "tee shm alloc fail\n");
		return -ENOMEM;
	}
	shm_va = tee_shm_get_va(_shm, 0);
	if (!shm_va) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "tee shm get va got NULL\n");
		return -ENOMEM;
	}
	if (IS_ERR(shm_va)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "tee shm get va fail\n");
		return -ENOMEM;
	}
	shm->buffer = shm_va;
	shm->shadow_buffer = (void *)_shm;
	shm->alloced_size = len;
	shm->registered_fd = -1;
	shm->buffer_allocated = true;
	return 0;
}

static void _mtk_pq_common_ca_teec_post_proc_tmpref(uint32_t param_type,
		TEEC_TempMemoryReference *tmpref, struct tee_param *param, TEEC_SharedMemory *shm)
{
	if (param_type != TEEC_MEMREF_TEMP_INPUT) {
		if (param->u.memref.size <= tmpref->size && tmpref->buffer)
			memcpy(tmpref->buffer, shm->buffer, param->u.memref.size);
		tmpref->size = param->u.memref.size;
	}
}

#if (MULTI_INVOKE_TYPE_SUPPORT)
static void _mtk_pq_common_ca_teec_post_proc_whole(TEEC_RegisteredMemoryReference *memref,
			struct tee_param *param)
{
	TEEC_SharedMemory *shm = memref->parent;

	if (shm->flags & TEEC_MEM_OUTPUT) {
		/*
		 * We're using a shadow buffer in this reference, copy back
		 * the shadow buffer into the real buffer now that we've
		 * returned from secure world.
		 */
		if (shm->shadow_buffer && param->u.memref.size <= shm->size)
			memcpy(shm->buffer, shm->shadow_buffer, param->u.memref.size);
		memref->size = param->u.memref.size;
	}
}
static void _mtk_pq_common_ca_teec_post_proc_partial(uint32_t param_type,
			TEEC_RegisteredMemoryReference *memref,
			struct tee_param *param)
{
	if (param_type != TEEC_MEMREF_PARTIAL_INPUT) {
		TEEC_SharedMemory *shm = memref->parent;
		/*
		 * We're using a shadow buffer in this reference, copy back
		 * the shadow buffer into the real buffer now that we've
		 * returned from secure world.
		 */
		if (shm->shadow_buffer && param->u.memref.size <= memref->size)
			memcpy((char *)shm->buffer + memref->offset,
					(char *)shm->shadow_buffer + memref->offset,
					param->u.memref.size);
		memref->size = param->u.memref.size;
	}
}
#endif

static void _mtk_pq_common_ca_teec_post_proc_operation(TEEC_Operation *operation,
			struct tee_param *params, TEEC_SharedMemory *shms)
{
	size_t n = 0;
	uint32_t param_type = 0;

	if (!operation)
		return;
	for (n = 0; n < TEEC_CONFIG_PAYLOAD_REF_COUNT; n++) {
		param_type = TEEC_PARAM_TYPE_GET(operation->paramTypes, n);
		switch (param_type) {
		case TEEC_NONE:
			break;
#if (MULTI_INVOKE_TYPE_SUPPORT)
		case TEEC_VALUE_INPUT:
			break;
		case TEEC_VALUE_OUTPUT:
		case TEEC_VALUE_INOUT:
			operation->params[n].value.a = params[n].u.value.a;
			operation->params[n].value.b = params[n].u.value.b;
			break;
		case TEEC_MEMREF_WHOLE:
			_mtk_pq_common_ca_teec_post_proc_whole(&operation->params[n].memref,
						params + n);
			break;
		case TEEC_MEMREF_PARTIAL_INPUT:
		case TEEC_MEMREF_PARTIAL_OUTPUT:
		case TEEC_MEMREF_PARTIAL_INOUT:
			_mtk_pq_common_ca_teec_post_proc_partial(param_type,
				&operation->params[n].memref, params + n);
			break;
#endif
		case TEEC_MEMREF_TEMP_INPUT:
		case TEEC_MEMREF_TEMP_OUTPUT:
		case TEEC_MEMREF_TEMP_INOUT:
			_mtk_pq_common_ca_teec_post_proc_tmpref(param_type,
				&operation->params[n].tmpref, params + n, shms + n);
			break;
		default:
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "invalid param_type : %x\n", param_type);
			break;
		}
	}
}

static int _mtk_pq_common_ca_teec_pre_proc_tmpref(TEEC_Context *ctx,
			uint32_t param_type, TEEC_TempMemoryReference *tmpref,
			struct tee_param *param, TEEC_SharedMemory *shm)
{
	int ret = 0;

	switch (param_type) {
	case TEEC_MEMREF_TEMP_INPUT:
		param->attr = TEE_IOCTL_PARAM_ATTR_TYPE_MEMREF_INPUT;
		shm->flags = TEEC_MEM_INPUT;
		break;
	case TEEC_MEMREF_TEMP_OUTPUT:
		param->attr = TEE_IOCTL_PARAM_ATTR_TYPE_MEMREF_OUTPUT;
		shm->flags = TEEC_MEM_OUTPUT;
		break;
	case TEEC_MEMREF_TEMP_INOUT:
		param->attr = TEE_IOCTL_PARAM_ATTR_TYPE_MEMREF_INOUT;
		shm->flags = TEEC_MEM_INPUT | TEEC_MEM_OUTPUT;
		break;
	default:
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "invalid param_type : %x\n", param_type);
		return -EINVAL;
	}
	shm->size = tmpref->size;
	ret = _mtk_pq_common_ca_teec_alloc_shm(ctx, shm);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "share mem alloc fail\n");
		return -EPERM;
	}
	memcpy(shm->buffer, tmpref->buffer, tmpref->size);
	param->u.memref.size = tmpref->size;
	//Workaround, shm->shadow_buffer is shm object.
	param->u.memref.shm = (struct tee_shm *) shm->shadow_buffer;
	return ret;
}

#if (MULTI_INVOKE_TYPE_SUPPORT)
static int _mtk_pq_common_ca_teec_pre_proc_whole(
		TEEC_RegisteredMemoryReference *memref, struct tee_param *param)
{
	const uint32_t inout = TEEC_MEM_INPUT | TEEC_MEM_OUTPUT;
	uint32_t flags = memref->parent->flags & inout;
	TEEC_SharedMemory *shm = NULL;

	if (flags == inout)
		param->attr = TEE_IOCTL_PARAM_ATTR_TYPE_MEMREF_INOUT;
	else if (flags & TEEC_MEM_INPUT)
		param->attr = TEE_IOCTL_PARAM_ATTR_TYPE_MEMREF_INPUT;
	else if (flags & TEEC_MEM_OUTPUT)
		param->attr = TEE_IOCTL_PARAM_ATTR_TYPE_MEMREF_OUTPUT;
	else
		return -EINVAL;
	shm = memref->parent;
	/*
	 * We're using a shadow buffer in this reference, copy the real buffer
	 * into the shadow buffer if needed. We'll copy it back once we've
	 * returned from the call to secure world.
	 */
	if (shm->shadow_buffer && (flags & TEEC_MEM_INPUT))
		memcpy(shm->shadow_buffer, shm->buffer, shm->size);
	param->u.memref.shm = shm->shadow_buffer;
	param->u.memref.size = shm->size;
	return 0;
}

static int _mtk_pq_common_ca_teec_pre_proc_partial(uint32_t param_type,
		TEEC_RegisteredMemoryReference *memref, struct tee_param *param)
{
	uint32_t req_shm_flags = 0;
	TEEC_SharedMemory *shm = NULL;

	switch (param_type) {
	case TEEC_MEMREF_PARTIAL_INPUT:
		req_shm_flags = TEEC_MEM_INPUT;
		param->attr = TEE_IOCTL_PARAM_ATTR_TYPE_MEMREF_INPUT;
		break;
	case TEEC_MEMREF_PARTIAL_OUTPUT:
		req_shm_flags = TEEC_MEM_OUTPUT;
		param->attr = TEE_IOCTL_PARAM_ATTR_TYPE_MEMREF_OUTPUT;
		break;
	case TEEC_MEMREF_PARTIAL_INOUT:
		req_shm_flags = TEEC_MEM_OUTPUT | TEEC_MEM_INPUT;
		param->attr = TEE_IOCTL_PARAM_ATTR_TYPE_MEMREF_INOUT;
		break;
	default:
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "invalid param_type : %x\n", param_type);
		return -EINVAL;
	}
	shm = memref->parent;
	if ((shm->flags & req_shm_flags) != req_shm_flags)
		return -EPERM;
	/*
	 * We're using a shadow buffer in this reference, copy the real buffer
	 * into the shadow buffer if needed. We'll copy it back once we've
	 * returned from the call to secure world.
	 */
	if (shm->shadow_buffer && param_type != TEEC_MEMREF_PARTIAL_OUTPUT)
		memcpy((char *)shm->shadow_buffer + memref->offset,
					(char *)shm->buffer + memref->offset, memref->size);
	param->u.memref.shm = shm->shadow_buffer;
	param->u.memref.shm_offs = memref->offset;
	param->u.memref.size = memref->size;
	return 0;
}
#endif

static int _mtk_pq_common_ca_teec_pre_proc_operation(TEEC_Context *ctx, TEEC_Operation *operation,
		struct tee_param *params, TEEC_SharedMemory *shms)
{
	int ret = 0;
	size_t n = 0;
	uint32_t param_type = 0;

	if (!operation) {
		memset(params, 0, sizeof(struct tee_param) * TEEC_CONFIG_PAYLOAD_REF_COUNT);
		return ret;
	}
	for (n = 0; n < TEEC_CONFIG_PAYLOAD_REF_COUNT; n++) {
		param_type = TEEC_PARAM_TYPE_GET(operation->paramTypes, n);
		switch (param_type) {
		case TEEC_NONE:
			params[n].attr = param_type;
			break;
#if (MULTI_INVOKE_TYPE_SUPPORT)
		case TEEC_VALUE_INPUT:
		case TEEC_VALUE_OUTPUT:
		case TEEC_VALUE_INOUT:
			params[n].attr = param_type;
			params[n].u.value.a = operation->params[n].value.a;
			params[n].u.value.b = operation->params[n].value.b;
			break;
		case TEEC_MEMREF_WHOLE:
			ret = _mtk_pq_common_ca_teec_pre_proc_whole(
					&operation->params[n].memref, params + n);
			break;
		case TEEC_MEMREF_PARTIAL_INPUT:
		case TEEC_MEMREF_PARTIAL_OUTPUT:
		case TEEC_MEMREF_PARTIAL_INOUT:
			ret = _mtk_pq_common_ca_teec_pre_proc_partial(param_type,
				&operation->params[n].memref, params + n);
			break;
#endif
		case TEEC_MEMREF_TEMP_INPUT:
		case TEEC_MEMREF_TEMP_OUTPUT:
		case TEEC_MEMREF_TEMP_INOUT:
			ret = _mtk_pq_common_ca_teec_pre_proc_tmpref(ctx, param_type,
				&(operation->params[n].tmpref), params + n, shms + n);
			break;
		default:
			ret = -EINVAL;
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "invalid param_type : %x\n", param_type);
			break;
		}
	}
	return ret;
}

int mtk_pq_common_ca_teec_invoke_cmd(struct mtk_pq_device *pq_dev, TEEC_Session *session,
		u32 cmd_id, TEEC_Operation *operation, u32 *error_origin)
{
	int ret = -EPERM;
	int otpee_version = _mtk_pq_common_ca_get_optee_version(pq_dev);
	uint32_t err_ori = 0;
	uint64_t buf[(sizeof(struct tee_ioctl_invoke_arg) +
			TEEC_CONFIG_PAYLOAD_REF_COUNT *
				sizeof(struct tee_param)) / sizeof(uint64_t)] = { 0 };
	struct tee_ioctl_invoke_arg *arg = NULL;
	struct tee_param *params = NULL;
	TEEC_SharedMemory shm[TEEC_CONFIG_PAYLOAD_REF_COUNT] = { 0 };

	if (!session) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "session is NULL!!!\n");
		return -EPERM;
	}
	if (otpee_version == MTK_OPTEE_VER_3) {
		arg = (struct tee_ioctl_invoke_arg *)buf;
		arg->num_params = TEEC_CONFIG_PAYLOAD_REF_COUNT;
		params = (struct tee_param *)(arg + 1);
		arg->session = session->session_id;
		arg->func = cmd_id;
		if (operation)
			operation->session = session;
		ret = _mtk_pq_common_ca_teec_pre_proc_operation(
			&(session->ctx), operation, params, shm);
		if (ret) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "teec pre process failed!\n");
			err_ori = TEEC_ORIGIN_API;
			goto out_free_temp_refs;
		}
		ret = tee_client_invoke_func((struct tee_context *)session->ctx.fd, arg, params);
		if (ret) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "TEE_IOC_INVOKE failed!\n");
			err_ori = TEEC_ORIGIN_COMMS;
			goto out_free_temp_refs;
		}
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_SVP, "tee client invoke cmd success.\n");
		ret = arg->ret;
		err_ori = arg->ret_origin;
		_mtk_pq_common_ca_teec_post_proc_operation(operation, params, shm);
out_free_temp_refs:
		_mtk_pq_common_ca_teec_free_temp_refs(operation, shm);
		if (error_origin)
			*error_origin = err_ori;
	} else
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"Not support OPTEE version (%d)\n", otpee_version);
	return ret;
}

void mtk_pq_common_ca_teec_close_session(struct mtk_pq_device *pq_dev, TEEC_Session *session)
{
	int otpee_version = _mtk_pq_common_ca_get_optee_version(pq_dev);

	if (!session) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "tee close fail session is NULL!!!\n");
		return;
	}
	if (!session->ctx.fd) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "tee close fail session->ctx.fd is NULL!!!\n");
		return;
	}
	if (otpee_version == MTK_OPTEE_VER_3)
		tee_client_close_session((struct tee_context *)session->ctx.fd,
			session->session_id);
	else
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"Not support OPTEE version (%d)\n", otpee_version);
}

int mtk_pq_common_ca_teec_open_session(struct mtk_pq_device *pq_dev, TEEC_Context *context,
		TEEC_Session *session, const TEEC_UUID *destination, TEEC_Operation *operation,
		u32 *error_origin)
{
	int otpee_version = _mtk_pq_common_ca_get_optee_version(pq_dev);

	if ((!session) || (!context)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "tee open fail session or context is NULL!!!\n");
		return -EPERM;
	}
	if (otpee_version == 3) {
		struct tee_ioctl_open_session_arg arg;
		struct tee_param param;
		int rc;

		memset(&arg, 0, sizeof(struct tee_ioctl_open_session_arg));
		memset(&param, 0, sizeof(struct tee_param));
		uuid_to_octets(arg.uuid, destination);
		arg.clnt_login = TEE_IOCTL_LOGIN_PUBLIC;
		arg.num_params = 1;
		if (operation != NULL)
			param.attr = operation->paramTypes;
		rc = tee_client_open_session(context->ctx, &arg, (struct tee_param *) &param);
		if ((rc == 0) && (arg.ret == 0)) {
			session->session_id = arg.session;
			session->ctx.fd = (uintptr_t)context->ctx;
			if (operation != NULL) {
				operation->params[0].value.a = param.u.value.a;
				operation->params[0].value.b = param.u.value.b;
			}
			return 0;
		}
		if (error_origin != NULL)
			*error_origin = arg.ret_origin;
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"tee open failed! rc=0x%08x, err=0x%08x, ori=0x%08x\n",
			rc, arg.ret, arg.ret_origin);
	} else {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"Not support OPTEE version (%d)\n", otpee_version);
	}
	return -EPERM;
}

void mtk_pq_common_ca_teec_finalize_context(struct mtk_pq_device *pq_dev, TEEC_Context *context)
{
	int otpee_version = _mtk_pq_common_ca_get_optee_version(pq_dev);

	if (!context) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"tee finalize context fail context is NULL!!!\n");
		return;
	}
	if (!context->ctx) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"tee finalize context fail context->ctx is NULL!!!\n");
		return;
	}
	if (otpee_version == MTK_OPTEE_VER_3)
		tee_client_close_context(context->ctx);
	else
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"Not support OPTEE version (%d)\n", otpee_version);
}

int mtk_pq_common_ca_teec_init_context(struct mtk_pq_device *pq_dev, TEEC_Context *context)
{
	if (_mtk_pq_common_ca_get_optee_version(pq_dev) == MTK_OPTEE_VER_3) {
		struct tee_ioctl_version_data vers = {
			.impl_id = TEE_OPTEE_CAP_TZ,
			.impl_caps = TEE_IMPL_ID_OPTEE,
			.gen_caps = TEE_GEN_CAP_GP,
		};
		context->ctx = tee_client_open_context(NULL, _optee_match, NULL, &vers);
		if (IS_ERR(context->ctx)) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "context is NULL!\n");
			return -EPERM;
		}
		return 0;
	}
	STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Not support OPTEE version (%d)\n",
				_mtk_pq_common_ca_get_optee_version(pq_dev));
	return -EPERM;
}

#endif // IS_ENABLED(CONFIG_OPTEE)
