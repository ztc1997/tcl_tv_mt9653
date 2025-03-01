// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include <linux/interrupt.h>
#include <media/v4l2-mem2mem.h>
#include "mtk_vcu.h"
#include "venc_ipi_msg.h"
#include "venc_vcu_if.h"
#include "mtk_vcodec_intr.h"
#include "mtk_vcodec_enc_pm.h"
#include "mtk_vcodec_enc.h"
#include "vdec_vcu_if.h"
#ifndef TV_ENCODE_INTEGRATION
#include "mtk_vcodec_mem.h"
#endif
#include "venc_drv_if.h"

static void handle_enc_init_msg(struct venc_vcu_inst *vcu, void *data)
{
	struct venc_vcu_ipi_msg_init *msg = data;

	if (vcu == NULL)
		return;
	vcu->inst_addr = msg->vcu_inst_addr;
	vcu->vsi = vcu_mapping_dm_addr(vcu->dev, msg->vcu_inst_addr);
}

static void handle_query_cap_ack_msg(struct venc_vcu_ipi_query_cap_ack *msg)
{
	struct venc_vcu_inst *vcu = (struct venc_vcu_inst *)msg->ap_inst_addr;
	void *data;
	int size = 0;

	if (vcu == NULL)
		return;
	mtk_vcodec_debug(vcu, "+ ap_inst_addr = 0x%llx, vcu_data_addr = 0x%llx, id = %d",
		(unsigned long long)msg->ap_inst_addr,
		(unsigned long long)msg->vcu_data_addr, msg->id);
	/* mapping VCU address to kernel virtual address */
	data = vcu_mapping_dm_addr(vcu->dev, msg->vcu_data_addr);
	switch (msg->id) {
	case GET_PARAM_CAPABILITY_SUPPORTED_FORMATS:
		size = sizeof(struct mtk_video_fmt);
		memcpy((void *)msg->ap_data_addr, data,
			size * MTK_MAX_ENC_CODECS_SUPPORT);
		break;
	case GET_PARAM_CAPABILITY_FRAME_SIZES:
		size = sizeof(struct mtk_codec_framesizes);
		memcpy((void *)msg->ap_data_addr, data,
			size * MTK_MAX_ENC_CODECS_SUPPORT);
		break;
	default:
		break;
	}
	mtk_vcodec_debug(vcu, "- vcu_inst_addr = 0x%llx", (unsigned long long)vcu->inst_addr);
}

#ifndef TV_ENCODE_INTEGRATION
static void handle_enc_waitisr_msg(struct venc_vcu_inst *vcu,
	void *data, uint32_t timeout)
{
	struct venc_vcu_ipi_msg_waitisr *msg = data;
	struct mtk_vcodec_ctx *ctx = vcu->ctx;

	msg->irq_status = ctx->irq_status;
	msg->timeout = timeout;
}
#endif

inline int get_mapped_fd(struct dma_buf *dmabuf)
{
	int target_fd = 0;

#ifndef TV_ENCODE_INTEGRATION
	unsigned long rlim_cur;
	unsigned long irqs;
	struct task_struct *task = NULL;
	struct files_struct *f = NULL;
	struct sighand_struct *sighand;
	spinlock_t      siglock;
	struct fdtable fdt;

	if (dmabuf == NULL || dmabuf->file == NULL)
		return 0;

	vcu_get_file_lock();

	vcu_get_task(&task, &f, 0);
	if (task == NULL || f == NULL ||
		probe_kernel_address(&task->sighand, sighand) ||
		probe_kernel_address(&task->sighand->siglock, siglock)) {
		vcu_put_file_lock();
		return -EMFILE;
	}

	spin_lock(&f->file_lock);
	if (probe_kernel_address(files_fdtable(f), fdt)) {
		spin_unlock(&f->file_lock);
		vcu_put_file_lock();
		return -EMFILE;
	}
	spin_unlock(&f->file_lock);

	if (!lock_task_sighand(task, &irqs)) {
		vcu_put_file_lock();
		return -EMFILE;
	}

	rlim_cur = task_rlimit(task, RLIMIT_NOFILE);
	unlock_task_sighand(task, &irqs);

	target_fd = __alloc_fd(f, 0, rlim_cur, O_CLOEXEC);

	get_file(dmabuf->file);

	if (target_fd < 0) {
		vcu_put_file_lock();
		return -EMFILE;
	}

	__fd_install(f, target_fd, dmabuf->file);

	vcu_put_file_lock();

	/* pr_info("get_mapped_fd: %d", target_fd); */
#endif
	return target_fd;
}

inline void close_mapped_fd(unsigned int target_fd)
{
#ifndef TV_ENCODE_INTEGRATION
	struct task_struct *task = NULL;
	struct files_struct *f = NULL;

	vcu_get_file_lock();
	vcu_get_task(&task, &f, 0);
	vcu_put_file_lock();
	if (task == NULL || f == NULL)
		return;

	__close_fd(f, target_fd);
#endif
}

int vcu_enc_ipi_handler(void *data, unsigned int len, void *priv)
{
	struct venc_vcu_ipi_msg_common *msg = data;
	struct venc_vcu_inst *vcu;
	struct mtk_vcodec_ctx *ctx;
	int ret = 0;
	unsigned long flags;
#ifndef TV_ENCODE_INTEGRATION
	struct task_struct *task = NULL;
	struct files_struct *f = NULL;

	vcu_get_file_lock();
	vcu_get_task(&task, &f, 0);
	vcu_put_file_lock();
	if (msg == NULL || task == NULL ||
	   task->tgid != current->tgid ||
	   (struct venc_vcu_inst *)(unsigned long)msg->venc_inst == NULL) {
		ret = -EINVAL;
		return ret;
	}
#endif

	vcu = (struct venc_vcu_inst *)(unsigned long)msg->venc_inst;
	if ((vcu != priv) && (msg->msg_id < VCU_IPIMSG_VENC_SEND_BASE)) {
		pr_info("%s, vcu:%p != priv:%p\n", __func__, vcu, priv);
		return 1;
	}

	mtk_vcodec_debug(vcu, "msg_id %x inst %p status %d",
					 msg->msg_id, vcu, msg->status);

	if (vcu->abort)
		return -EINVAL;

	ctx = vcu->ctx;
#ifndef TV_ENCODE_INTEGRATION
	if (sizeof(msg) > SHARE_BUF_SIZE) {
		mtk_vcodec_err(vcu, "venc_ap_ipi_msg_deint cannot be large than %d",
					   SHARE_BUF_SIZE);
		return -EINVAL;
	}
#endif

	switch (msg->msg_id) {
	case VCU_IPIMSG_ENC_INIT_DONE:
		handle_enc_init_msg(vcu, data);
		break;
	case VCU_IPIMSG_ENC_SET_PARAM_DONE:
		/* Prevent slowmotion with GCE mode on,
		 * user thread enter freezing while holding mutex (enc lock)
		 */
		current->flags |= PF_NOFREEZE;
		break;
	case VCU_IPIMSG_ENC_DEINIT_DONE:
		break;
	case VCU_IPIMSG_ENC_POWER_ON:
		venc_encode_prepare(ctx, 0, &flags);
		ret = 1;
		break;
	case VCU_IPIMSG_ENC_POWER_OFF:
		venc_encode_unprepare(ctx, 0, &flags);
		ret = 1;
		break;
	case VCU_IPIMSG_ENC_QUERY_CAP_ACK:
		handle_query_cap_ack_msg(data);
		break;
#ifndef TV_ENCODE_INTEGRATION
	case VCU_IPIMSG_ENC_WAIT_ISR:
		if (-1 == mtk_vcodec_wait_for_done_ctx(ctx, 0,
			MTK_INST_IRQ_RECEIVED,
			WAIT_INTR_TIMEOUT_MS)) {
			handle_enc_waitisr_msg(vcu, data, 1);
			mtk_vcodec_debug(vcu,
				"irq_status %x <-", ctx->irq_status);
		} else
			handle_enc_waitisr_msg(vcu, data, 0);
		ret = 1;
		break;
	case VCU_IPIMSG_ENC_PUT_BUFFER:
		mtk_enc_put_buf(ctx);
		ret = 1;
		break;
#endif
	case VCU_IPIMSG_ENC_ENCODE_ACK:
		break;
	default:
		mtk_vcodec_err(vcu, "unknown msg id %x", msg->msg_id);
		break;
	}

	/* deinit done timeout case handling do not touch vdec_vcu_inst
	 * or memory used after freed
	 */
	if (msg->msg_id != VCU_IPIMSG_ENC_DEINIT_DONE) {
		vcu->signaled = 1;
		vcu->failure = (msg->status != VENC_IPI_MSG_STATUS_OK);
	}

	mtk_vcodec_debug_leave(vcu);
	return ret;
}

static int vcu_enc_send_msg(struct venc_vcu_inst *vcu, void *msg,
							int len)
{
	int status;
#ifndef TV_ENCODE_INTEGRATION
	struct task_struct *task = NULL;
	struct files_struct *f = NULL;
#endif

	mtk_vcodec_debug_enter(vcu);

	if (!vcu->dev) {
		mtk_vcodec_err(vcu, "inst dev is NULL");
		return -EINVAL;
	}

	if (vcu->abort)
		return -EIO;

#ifndef TV_ENCODE_INTEGRATION
	vcu_get_file_lock();
	vcu_get_task(&task, &f, 0);
	vcu_put_file_lock();
	if (task == NULL ||
		vcu->daemon_pid != task->tgid) {
		if (task)
			mtk_vcodec_err(vcu, "send fail pid: inst %d curr %d",
				vcu->daemon_pid, task->tgid);
		vcu->abort = 1;
		return -EIO;
	}
#endif

	status = vcu_ipi_send(vcu->dev, vcu->id, msg, len);
	if (status) {
		mtk_vcodec_err(vcu, "vcu_ipi_send msg_id %x len %d fail %d",
					   *(uint32_t *)msg, len, status);
		if (status == -EIO)
			vcu->abort = 1;
		return status;
	}
	if (vcu->failure)
		return -EINVAL;

	mtk_vcodec_debug_leave(vcu);

	return 0;
}


#ifndef TV_ENCODE_INTEGRATION
void vcu_enc_set_pid(struct venc_vcu_inst *vcu)
{
	struct task_struct *task = NULL;
	struct files_struct *f = NULL;

	vcu_get_file_lock();
	vcu_get_task(&task, &f, 0);
	vcu_put_file_lock();
	if (task != NULL)
		vcu->daemon_pid = task->tgid;
	else
		vcu->daemon_pid = -1;
}
#endif


int vcu_enc_init(struct venc_vcu_inst *vcu)
{
	int status;
	struct venc_ap_ipi_msg_init out;

	mtk_vcodec_debug_enter(vcu);

#ifndef TV_ENCODE_INTEGRATION
	if (sizeof(out) > SHARE_BUF_SIZE) {
		mtk_vcodec_err(vcu, "venc_ap_ipi_msg_init cannot be large than %d",
					   SHARE_BUF_SIZE);
		return -EINVAL;
	}
#endif

	init_waitqueue_head(&vcu->wq_hd);
	vcu->signaled = 0;
	vcu->failure = 0;

	status = vcu_ipi_register(vcu->dev, vcu->id, vcu->handler,
							  NULL, vcu);
	if (status) {
		mtk_vcodec_err(vcu, "vcu_ipi_register fail %d", status);
		return -EINVAL;
	}

	memset(&out, 0, sizeof(out));
	out.msg_id = AP_IPIMSG_ENC_INIT;
	out.venc_inst = (unsigned long)vcu;

#ifndef TV_ENCODE_INTEGRATION
	vcu_enc_set_pid(vcu);
#endif
	if (vcu_enc_send_msg(vcu, &out, sizeof(out))) {
		mtk_vcodec_err(vcu, "AP_IPIMSG_ENC_INIT fail");
		return -EINVAL;
	}
	mtk_vcodec_debug_leave(vcu);

	return 0;
}

int vcu_enc_query_cap(struct venc_vcu_inst *vcu, unsigned int id, void *out)
{
	struct venc_ap_ipi_query_cap msg;
	int err = 0;

#ifndef TV_ENCODE_INTEGRATION
	if (sizeof(msg) > SHARE_BUF_SIZE) {
		mtk_vcodec_err(vcu, "venc_ap_ipi_query_cap cannot be large than %d",
					   SHARE_BUF_SIZE);
		return -EINVAL;
	}
#endif

	mtk_vcodec_debug(vcu, "+ id=%X", AP_IPIMSG_ENC_QUERY_CAP);
	vcu->dev = vcu_get_plat_device(vcu->ctx->dev->plat_dev);
	if (vcu->dev == NULL) {
		mtk_vcodec_err(vcu, "vcu_get_plat_device fail!!");
		return -EINVAL;
	}

	vcu->id = (vcu->id == IPI_VCU_INIT) ? IPI_VENC_COMMON : vcu->id;
	vcu->handler = vcu_enc_ipi_handler;

	err = vcu_ipi_register(vcu->dev,
		vcu->id, vcu->handler, NULL, vcu);
	if (err != 0) {
		mtk_vcodec_err(vcu, "vcu_ipi_register fail status=%d", err);
		return err;
	}

	memset(&msg, 0, sizeof(msg));
	msg.msg_id = AP_IPIMSG_ENC_QUERY_CAP;
	msg.id = id;
	msg.ap_inst_addr = (uintptr_t)vcu;
	msg.ap_data_addr = (uintptr_t)out;
#ifndef TV_ENCODE_INTEGRATION
	vcu_enc_set_pid(vcu);
#endif
	err = vcu_enc_send_msg(vcu, &msg, sizeof(msg));
	mtk_vcodec_debug(vcu, "- id=%X ret=%d", msg.msg_id, err);

	return err;
}

int vcu_enc_set_param(struct venc_vcu_inst *vcu,
					  enum venc_set_param_type id,
					  struct venc_enc_param *enc_param)
{
	struct venc_ap_ipi_msg_set_param out;

	mtk_vcodec_debug(vcu, "id %d ->", id);
#ifndef TV_ENCODE_INTEGRATION
	if (sizeof(out) > SHARE_BUF_SIZE) {
		mtk_vcodec_err(vcu, "venc_ap_ipi_msg_set_param cannot be large than %d",
					   SHARE_BUF_SIZE);
		return -EINVAL;
	}
#endif

	memset(&out, 0, sizeof(out));
	out.msg_id = AP_IPIMSG_ENC_SET_PARAM;
	out.vcu_inst_addr = vcu->inst_addr;
	out.param_id = id;
	switch (id) {
	case VENC_SET_PARAM_ENC:
		out.data_item = 0;
		break;
	case VENC_SET_PARAM_FORCE_INTRA:
		out.data_item = 0;
		break;
	case VENC_SET_PARAM_ADJUST_BITRATE:
		out.data_item = 1;
		out.data[0] = enc_param->bitrate;
		break;
	case VENC_SET_PARAM_ADJUST_FRAMERATE:
		out.data_item = 1;
		out.data[0] = enc_param->frm_rate;
		break;
	case VENC_SET_PARAM_GOP_SIZE:
		out.data_item = 1;
		out.data[0] = enc_param->gop_size;
		break;
	case VENC_SET_PARAM_INTRA_PERIOD:
		out.data_item = 1;
		out.data[0] = enc_param->intra_period;
		break;
#ifndef TV_ENCODE_INTEGRATION
	case VENC_SET_PARAM_SKIP_FRAME:
		out.data_item = 0;
		break;
	case VENC_SET_PARAM_PREPEND_HEADER:
		out.data_item = 0;
		break;
	case VENC_SET_PARAM_SCENARIO:
		out.data_item = 1;
		out.data[0] = enc_param->scenario;
		break;
	case VENC_SET_PARAM_NONREFP:
		out.data_item = 1;
		out.data[0] = enc_param->nonrefp;
		break;
	case VENC_SET_PARAM_DETECTED_FRAMERATE:
		out.data_item = 1;
		out.data[0] = enc_param->detectframerate;
		break;
	case VENC_SET_PARAM_RFS_ON:
		out.data_item = 1;
		out.data[0] = enc_param->rfs;
		break;
	case VENC_SET_PARAM_PREPEND_SPSPPS_TO_IDR:
		out.data_item = 1;
		out.data[0] = enc_param->prependheader;
		break;
	case VENC_SET_PARAM_OPERATION_RATE:
		out.data_item = 1;
		out.data[0] = enc_param->operationrate;
		break;
	case VENC_SET_PARAM_BITRATE_MODE:
		out.data_item = 1;
		out.data[0] = enc_param->bitratemode;
		break;
	case VENC_SET_PARAM_ROI_ON:
		out.data_item = 1;
		out.data[0] = enc_param->roion;
		break;
	case VENC_SET_PARAM_HEIF_GRID_SIZE:
		out.data_item = 1;
		out.data[0] = enc_param->heif_grid_size;
		break;
	case VENC_SET_PARAM_SEC_MODE:
		out.data_item = 1;
		out.data[0] = enc_param->svp_mode;
		break;
#endif
	case VENC_SET_PARAM_COLOR_DESC:
		out.data_item = 0; // passed via vsi
		break;
	case VENC_SET_PARAM_VIDEO_QUALITY:
		out.data_item = 0;
		break;
	default:
		mtk_vcodec_err(vcu, "id %d not supported", id);
		return -EINVAL;
	}

	if (vcu_enc_send_msg(vcu, &out, sizeof(out))) {
		mtk_vcodec_err(vcu,
			"AP_IPIMSG_ENC_SET_PARAM %d fail", id);
		return -EINVAL;
	}

	mtk_vcodec_debug(vcu, "id %d <-", id);

	return 0;
}

int vcu_enc_encode(struct venc_vcu_inst *vcu, unsigned int bs_mode,
				   struct venc_frm_buf *frm_buf,
				   struct mtk_vcodec_mem *bs_buf,
				   unsigned int *bs_size)
{

	struct venc_ap_ipi_msg_enc out;
	struct venc_vsi *vsi = (struct venc_vsi *)vcu->vsi;
	unsigned int i, ret;

	mtk_vcodec_debug(vcu, "bs_mode %d ->", bs_mode);

#ifndef TV_ENCODE_INTEGRATION
	if (sizeof(out) > SHARE_BUF_SIZE) {
		mtk_vcodec_err(vcu, "venc_ap_ipi_msg_enc cannot be large than %d",
					   SHARE_BUF_SIZE);
		return -EINVAL;
	}
#endif

	memset(&out, 0, sizeof(out));
	out.msg_id = AP_IPIMSG_ENC_ENCODE;
	out.vcu_inst_addr = vcu->inst_addr;
	out.bs_mode = bs_mode;
	if (frm_buf) {
		out.fb_num_planes = frm_buf->num_planes;
		for (i = 0; i < frm_buf->num_planes; i++) {
			out.input_addr[i] =
				frm_buf->fb_addr[i].dma_addr;
			out.input_size[i] =
				frm_buf->fb_addr[i].size;
			out.input_fd[i] =
				get_mapped_fd(frm_buf->fb_addr[i].dmabuf);
			out.data_offset[i] =
				frm_buf->fb_addr[i].data_offset;
		}
#ifndef TV_ENCODE_INTEGRATION
		if (frm_buf->has_meta) {
			vsi->meta_fd =
				get_mapped_fd(frm_buf->meta_dma);
			vsi->meta_size = sizeof(struct mtk_hdr_dynamic_info);
			vsi->meta_addr = frm_buf->meta_addr;
		} else {
#endif
			vsi->meta_fd = 0;
			vsi->meta_size = 0;
			vsi->meta_addr = 0;
#ifndef TV_ENCODE_INTEGRATION
		}
#endif

		mtk_vcodec_debug(vcu, " num_planes = %d input (dmabuf:%lx fd:%d), meta fd %d size %d %llx",
			frm_buf->num_planes,
			(unsigned long)frm_buf->fb_addr[0].dmabuf,
			out.input_fd[0], vsi->meta_fd, vsi->meta_size,
			vsi->meta_addr);
	}

	if (bs_buf) {
		out.bs_addr = bs_buf->dma_addr;
		out.bs_size = bs_buf->size;
		out.bs_fd = get_mapped_fd(bs_buf->dmabuf);
		out.bs_va = (uintptr_t)bs_buf->va;
		mtk_vcodec_debug(vcu, " output (dma:%lx fd:%x)",
			(unsigned long)bs_buf->dmabuf,
			out.bs_fd);
		mtk_vcodec_debug(vcu, " output (bs_addr:%lx)",
			(unsigned long)out.bs_addr);
	}

	ret = vcu_enc_send_msg(vcu, &out, sizeof(out));
	if (ret) {
		mtk_vcodec_err(vcu, "AP_IPIMSG_ENC_ENCODE %d fail %d",
					   bs_mode, ret);
		return ret;
	}

	if (frm_buf) {
		for (i = 0; i < frm_buf->num_planes; i++) {
			if (frm_buf->fb_addr[i].dmabuf != NULL)
				close_mapped_fd(out.input_fd[i]);
		}
#ifndef TV_ENCODE_INTEGRATION
		if (frm_buf->has_meta) {
			close_mapped_fd(vsi->meta_fd);
			dma_buf_put(frm_buf->meta_dma);
		}
#endif
	}

	if (bs_buf && bs_buf->dmabuf != NULL)
		close_mapped_fd(out.bs_fd);

	mtk_vcodec_debug(vcu, "bs_mode %d size %d key_frm %d <-",
		bs_mode, vcu->bs_size, vcu->is_key_frm);

	return 0;
}

int vcu_enc_deinit(struct venc_vcu_inst *vcu)
{
	struct venc_ap_ipi_msg_deinit out;

	mtk_vcodec_debug_enter(vcu);

#ifndef TV_ENCODE_INTEGRATION
	if (sizeof(out) > SHARE_BUF_SIZE) {
		mtk_vcodec_err(vcu, "venc_ap_ipi_msg_deint cannot be large than %d",
					   SHARE_BUF_SIZE);
		return -EINVAL;
	}
#endif

	memset(&out, 0, sizeof(out));
	out.msg_id = AP_IPIMSG_ENC_DEINIT;
	out.vcu_inst_addr = vcu->inst_addr;
	if (vcu_enc_send_msg(vcu, &out, sizeof(out))) {
		mtk_vcodec_err(vcu, "AP_IPIMSG_ENC_DEINIT fail");
		return -EINVAL;
	}
	current->flags &= ~PF_NOFREEZE;

	mtk_vcodec_debug_leave(vcu);

	return 0;
}

int vcu_enc_set_ctx(struct venc_vcu_inst *vcu)
{
	int err = 0;

#ifndef TV_ENCODE_INTEGRATION
	vcu_set_codec_ctx(vcu->dev,
		(void *)vcu->ctx,
		v4l2_m2m_get_vq(vcu->ctx->m2m_ctx,
			V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE),
		v4l2_m2m_get_vq(vcu->ctx->m2m_ctx,
			V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE),
		VCU_VENC);
#endif

	return err;
}

int vcu_enc_clear_ctx(struct venc_vcu_inst *vcu)
{
	int err = 0;
#ifndef TV_ENCODE_INTEGRATION
	vcu_clear_codec_ctx(vcu->dev,
		(void *)vcu->ctx, VCU_VENC);
#endif
	return err;
}

