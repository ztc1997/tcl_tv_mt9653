// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 * Author: JB Tsai <jb.tsai@mediatek.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/of_irq.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/kthread.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/version.h>
#include <linux/ktime.h>
#include <comm_driver.h>
#include "edma_driver.h"
#include "edma_cmd_hnd.h"
#include "edma_reg.h"
#include "edma_queue.h"
#include "edma_api.h"
#include "apusys_power.h"
#include "edma_dbgfs.h"
#include "edma3_reg.h"

#define NO_INTERRUPT		0

#define	EDMA_EXT_MODE_SIZE		0x60
#define MTK_IOVA_START_ADDR 0x200000000
#define EDMA_CTRL_BY_TEE	1

static void *get_kva(u32 iova, u32 size, u8 iommu_en)
{
	struct comm_kmem mem;

	mem.iova = iova;
	mem.size = size;

	if (!IS_ALIGNED(iova, 4))
		return NULL;

	if (iommu_en == 1) {
		mem.iova |= MTK_IOVA_START_ADDR;
		mem.mem_type = APU_COMM_MEM_DRAM_DMA;
	} else {
		mem.mem_type = APU_COMM_MEM_VLM;
	}

	if (comm_util_get_cb()->query_mem(&mem, COMM_QUERY_BY_IOVA))
		return NULL;

	return (void *)(uintptr_t)mem.kva;
}

int check_args_ext_subcmd(void *kva, u32 count, u32 iommu_en)
{
	u32 *p;
	void *src, *dst;
	int i;

	// check subcmd on ioctl ext_mode case for syzkaller test
	for (i = 0; i < count; i++) {
		p = (u32 *)(uintptr_t)(kva + EDMA_EXT_MODE_SIZE * i);
		// [TODO] check size of src/dst buffer size
		src = get_kva(p[11],
			((p[0] & 0xffff) + 1) * ((p[0] >> 16 & 0xFFFF) + 1) * (p[1] + 1),
			iommu_en);

		dst = get_kva(p[13],
			((p[6] & 0xffff) + 1) * ((p[6] >> 16 & 0xFFFF) + 1) * (p[1] + 1),
			iommu_en);

		if ((src == NULL) || (dst == NULL) || (src == dst)) {
			LOG_DBG("%s(): subcmd[%x] fail, src:%lx dst:%lx\n",
				__func__, i, (uintptr_t)src, (uintptr_t)dst);
			return -EFAULT;
		}
	}

	return 0;
}

int check_args(struct edma_request *req, u32 ioctl_ext)
{
	void *kva;
	int ret = 0;

	switch (req->cmd) {
	case EDMA_PROC_EXT_MODE:
		if (req->ext_reg_addr) {
			kva = get_kva(req->ext_reg_addr,
				(req->ext_count) * EDMA_EXT_MODE_SIZE,
				req->desp_iommu_en);
			if ((kva == NULL) || (req->ext_count == 0) ||
			 (ioctl_ext && check_args_ext_subcmd(kva,
						req->ext_count,
						req->desp_iommu_en))) {
				ret = -EFAULT;
				break;
			}

		} else {
			ret = -EFAULT;
		}
		break;
	case EDMA_PROC_FILL:
		kva = get_kva(req->desp.dst_addr,
				req->desp.dst_tile_channel * req->desp.dst_tile_width,
				req->desp_iommu_en);
		if (kva == NULL)
			ret = -EFAULT;
		break;
	case EDMA_PROC_NORMAL:
		kva = get_kva(req->desp.src_addr,
			req->desp.src_tile_channel *
			req->desp.src_tile_width * req->desp.src_tile_height,
			req->desp_iommu_en);
		if (kva == NULL) {
			ret = -EFAULT;
			break;
		}

		kva = get_kva(req->desp.dst_addr,
				req->desp.dst_tile_channel * req->desp.dst_tile_width,
				req->desp_iommu_en);
		if (kva == NULL) {
			ret = -EFAULT;
			break;
		}
		break;
	case EDMA_PROC_FORMAT:
	case EDMA_PROC_COMPRESS:
	case EDMA_PROC_DECOMPRESS:
		kva = get_kva(req->desp.src_addr,
				req->desp.src_tile_channel *
				req->desp.src_tile_width * req->desp.src_tile_height,
				req->desp_iommu_en);
		if (kva == NULL) {
			ret = -EFAULT;
			break;
		}

		kva = get_kva(req->desp.dst_addr,
				req->desp.dst_tile_channel * req->desp.dst_tile_width,
				req->desp_iommu_en);
		if (kva == NULL) {
			ret = -EFAULT;
			break;
		}

		kva = get_kva(req->desp.src_uv_addr,
				req->desp.src_tile_channel *
				req->desp.src_tile_width * req->desp.src_tile_height,
				req->desp_iommu_en);
		if (kva == NULL) {
			ret = -EFAULT;
			break;
		}

		kva = get_kva(req->desp.dst_uv_addr,
				req->desp.dst_tile_channel * req->desp.dst_tile_width,
				req->desp_iommu_en);
		if (kva == NULL) {
			ret = -EFAULT;
			break;
		}
		break;
	case EDMA_PROC_RAW:
		kva = get_kva(req->desp.src_addr,
				req->desp.src_tile_channel *
				req->desp.src_tile_width * req->desp.src_tile_height,
				req->desp_iommu_en);
		if (kva == NULL) {
			ret = -EFAULT;
			break;
		}

		kva = get_kva(req->desp.dst_addr,
				req->desp.dst_tile_channel * req->desp.dst_tile_width,
				req->desp_iommu_en);
		if (kva == NULL) {
			ret = -EFAULT;
			break;
		}

		kva = get_kva(req->desp.src_uv_addr,
				req->desp.src_tile_channel *
				req->desp.src_tile_width * req->desp.src_tile_height,
				req->desp_iommu_en);
		if (kva == NULL) {
			ret = -EFAULT;
			break;
		}
		break;
	default:
		pr_err("%s: bad command\n", __func__);
		ret = -EINVAL;
	}

	if (ret) {
		LOG_DBG("%s():fail, dump args ---\n", __func__);
		LOG_DBG("%s():-------------------\n", __func__);
		LOG_DBG("%s(): cmd:%x\n", __func__, req->cmd);
		LOG_DBG("%s(): ext_reg_addr:%x\n", __func__, req->ext_reg_addr);
		LOG_DBG("%s(): ext_count:%x\n", __func__, req->ext_count);
		LOG_DBG("%s(): buf_iommu_en:%x\n", __func__, req->buf_iommu_en);
		LOG_DBG("%s(): desp_iommu_en:%x\n", __func__, req->desp_iommu_en);
		LOG_DBG("%s():-------------------\n", __func__);
	}

	return ret;
}


static inline void lock_command(struct edma_sub *edma_sub)
{
	mutex_lock(&edma_sub->cmd_mutex);
	edma_sub->is_cmd_done = false;
}

static inline int wait_command(struct edma_sub *edma_sub, u32 timeout)
{
	return (wait_event_interruptible_timeout(edma_sub->cmd_wait,
						 edma_sub->is_cmd_done,
						 msecs_to_jiffies
						 (timeout)) > 0)
	    ? 0 : -ETIMEDOUT;
}

static inline void unlock_command(struct edma_sub *edma_sub)
{
	mutex_unlock(&edma_sub->cmd_mutex);
}

static void print_error_status(struct edma_sub *edma_sub,
				struct edma_request *req)
{
	u32 status, i;
	unsigned int *ext_reg = NULL;

	status = edma_read_reg32(edma_sub->base_addr, APU_EDMA2_ERR_STATUS);
	pr_notice("%s error status %x\n", edma_sub->sub_name,
		status);

	for (i = 0; i < (EDMA_REG_SHOW_RANGE >> 2); i++) {
		status = edma_read_reg32(edma_sub->base_addr, i * 4);
		pr_notice("edma error dump register[0x%x] = 0x%x\n",
		i * 4, status);
	}
	if (req->ext_reg_addr != 0) {
		ext_reg = (unsigned int *)
			(uintptr_t)get_kva(req->ext_reg_addr, 0, req->desp_iommu_en);

		pr_notice("kva ext_reg =  0x%p, req->ext_count = %d\n",
			ext_reg, req->ext_count);

		/*
		if (ext_reg !=  0)
			for (i = 0; i < req->ext_count; i++) {
				for (j = 0; j < EDMA_EXT_MODE_SIZE/4; j++)
					pr_notice("descriptor%d [0x%x] = 0x%x\n",
						i, j*4, *(ext_reg + j));
			}
		else
			pr_notice("not support ext_reg dump!!\n");
		*/
	}
	for (i = (EDMA_REG_EX_R1 >> 2); i < (EDMA_REG_EX_R2 >> 2); i++) {
		status = edma_read_reg32(edma_sub->base_addr, i*4);
		pr_notice("edma dump extra register[0x%x] = 0x%x\n",
		i*4, status);
	}
	edma_sw_reset(edma_sub);
}

irqreturn_t edma_isr_handler(int irq, void *edma_sub_info)
{
	struct edma_sub *edma_sub = (struct edma_sub *)edma_sub_info;
	struct edma_device *edma_device =
		edma_sub->edma_device;
	u32 status;
	u32 desp0_done;
	u32 desp0_intr;
	u32 desp1_done;
	u32 desp1_intr;
	u32 desp2_done;
	u32 desp2_intr;
	u32 desp3_done;
	u32 desp3_intr;
	u32 desp4_done;
	u32 desp4_intr;

	if (edma_device->power_on_cnt == 0) {
		LOG_DBG("edma is_handler at power off status\n");
		return IRQ_HANDLED;
	}

	LOG_DBG("edma isr_handler +!!\n");

	status = edma_read_reg32(edma_sub->base_addr, APU_EDMA2_INT_STATUS);
	desp0_done = status & DESP0_DONE_STATUS;
	desp1_done = status & DESP1_DONE_STATUS;
	desp2_done = status & DESP2_DONE_STATUS;
	desp3_done = status & DESP3_DONE_STATUS;
	desp4_done = status & EXT_DESP_DONE_STATUS;
	desp0_intr = status & DESP0_DONE_INT_STATUS;
	desp1_intr = status & DESP1_DONE_INT_STATUS;
	desp2_intr = status & DESP2_DONE_INT_STATUS;
	desp3_intr = status & DESP3_DONE_INT_STATUS;
	desp4_intr = status & EXT_DESP_DONE_INT_STATUS;

	edma_sub->is_cmd_done = true;
	if (desp0_done | desp1_done | desp2_done | desp3_done | desp4_done)
		wake_up_interruptible(&edma_sub->cmd_wait);

	if (desp0_intr)
		edma_set_reg32(edma_sub->base_addr, APU_EDMA2_INT_STATUS,
						DESP0_DONE_INT_STATUS);
	else if (desp1_intr)
		edma_set_reg32(edma_sub->base_addr, APU_EDMA2_INT_STATUS,
						DESP1_DONE_INT_STATUS);
	else if (desp2_intr)
		edma_set_reg32(edma_sub->base_addr, APU_EDMA2_INT_STATUS,
						DESP2_DONE_INT_STATUS);
	else if (desp3_intr)
		edma_set_reg32(edma_sub->base_addr, APU_EDMA2_INT_STATUS,
						DESP3_DONE_INT_STATUS);
	else if (desp4_intr)
		edma_set_reg32(edma_sub->base_addr, APU_EDMA2_INT_STATUS,
						EXT_DESP_DONE_INT_STATUS);

	LOG_DBG("edma isr_handler -!!\n");

	return IRQ_HANDLED;
}

void edma_enable_sequence(struct edma_sub *edma_sub)
{
	edma_set_reg32(edma_sub->base_addr, APU_EDMA2_CTL_0, CLK_ENABLE);
	edma_set_reg32(edma_sub->base_addr, APU_EDMA2_CTL_0, DMA_SW_RST);
	edma_clear_reg32(edma_sub->base_addr, APU_EDMA2_CTL_0, DMA_SW_RST);
}

#define UDELAY_TIME 5
void edma_sw_reset(struct edma_sub *edma_sub)
{
	u32 value = 0;

	LOG_DBG("%s\n", __func__);
	edma_set_reg32(edma_sub->base_addr, APU_EDMA2_CTL_0, CLK_ENABLE);

#ifdef EDMA_V3
	edma_set_reg32(edma_sub->base_addr, APU_EDMA2_CTL_0, AXI_PROT_EN);

	while (!(value & RST_PROT_IDLE))
		value = edma_read_reg32(edma_sub->base_addr, APU_EDMA2_CTL_0);

	LOG_DBG("value = 0x%x\n", value);

	edma_set_reg32(edma_sub->base_addr, APU_EDMA2_CTL_0, DMA_SW_RST);
	udelay(UDELAY_TIME);
	edma_clear_reg32(edma_sub->base_addr, APU_EDMA2_CTL_0, DMA_SW_RST);
	udelay(UDELAY_TIME);
	edma_clear_reg32(edma_sub->base_addr, APU_EDMA2_CTL_0, AXI_PROT_EN);
#else
	edma_set_reg32(edma_sub->base_addr, APU_EDMA2_CTL_0, DMA_SW_RST);
	udelay(5);
	edma_clear_reg32(edma_sub->base_addr, APU_EDMA2_CTL_0, DMA_SW_RST);
#endif
	// set [1:0] edma0_ctxt_cfg_4gb_sel = 0x2
	value = edma_read_reg32_remap(0x1c502000, 0x31c);
	value = (value & ~0x3) | 0x2;
	edma_write_reg32_remap(0x1c502000, 0x31c, value);
}


int edma_trigger_internal_mode(void __iomem *base_addr,
				struct edma_request *req)
{
	u32 write_pointer;
	u32 desp_offset_reg;
	u32 value;
	u32 src_tile_channel = 0;
	u32 src_tile_width = 0;
	u32 src_tile_height = 0;
	u32 dst_tile_channel = 0;
	u32 dst_tile_width = 0;

	write_pointer = edma_read_reg32(base_addr, APU_EDMA2_CFG_0);
	write_pointer = (write_pointer & DESP_WRITE_POINTER_MASK) >> 4;

	desp_offset_reg = write_pointer*APU_EDMA2_DESP_OFFSET;
	if (req->desp.src_tile_channel)
		src_tile_channel = req->desp.src_tile_channel - 1;
	if (req->desp.src_tile_width)
		src_tile_width = req->desp.src_tile_width - 1;
	if (req->desp.src_tile_height)
		src_tile_height = req->desp.src_tile_height - 1;
	if (req->desp.dst_tile_channel)
		dst_tile_channel = req->desp.dst_tile_channel - 1;
	if (req->desp.dst_tile_width)
		dst_tile_width = req->desp.dst_tile_width - 1;

	edma_write_reg32(base_addr,
			APU_EDMA2_DESP0_0 + desp_offset_reg,
			src_tile_channel | (src_tile_width << 16));
	edma_write_reg32(base_addr,
			APU_EDMA2_DESP0_4 + desp_offset_reg,
			src_tile_height);
	edma_write_reg32(base_addr,
			APU_EDMA2_DESP0_8 + desp_offset_reg,
			req->desp.src_channel_stride);
	edma_write_reg32(base_addr,
			APU_EDMA2_DESP0_C + desp_offset_reg,
			req->desp.src_width_stride);

	edma_write_reg32(base_addr,
			APU_EDMA2_DESP0_18 + desp_offset_reg,
			dst_tile_channel | (dst_tile_width << 16));
	edma_write_reg32(base_addr,
			APU_EDMA2_DESP0_1C + desp_offset_reg,
			req->desp.dst_channel_stride);
	edma_write_reg32(base_addr,
			APU_EDMA2_DESP0_20 + desp_offset_reg,
			req->desp.dst_width_stride);

	edma_write_reg32(base_addr,
			APU_EDMA2_DESP0_2C + desp_offset_reg,
			req->desp.src_addr);
	edma_write_reg32(base_addr,
			APU_EDMA2_DESP0_34 + desp_offset_reg,
			req->desp.dst_addr);

	if (req->buf_iommu_en)
		edma_set_reg32(base_addr,
		APU_EDMA2_DESP0_40 + desp_offset_reg,
		DESP0_INT_ENABLE | DESP0_DMA_AWUSER_IOMMU |
		DESP0_DMA_ARUSER_IOMMU);
	else
		edma_set_reg32(base_addr,
			APU_EDMA2_DESP0_40 + desp_offset_reg,
			DESP0_INT_ENABLE);

	switch (req->cmd) {
	case EDMA_PROC_FILL:{
			edma_write_reg32(base_addr,
			APU_EDMA2_DESP0_44 + desp_offset_reg,
			DESP0_OUT_FILL_MODE);
			break;
		}
	case EDMA_PROC_NUMERICAL:{
			value = req->desp.in_format
				| (req->desp.out_format << 16);
			edma_write_reg32(base_addr,
					APU_EDMA2_DESP0_44 + desp_offset_reg,
					value);
			if (req->desp.in_format == EDMA_FORMAT_I8
				&& req->desp.out_format == EDMA_FORMAT_FP32) {
				edma_write_reg32(base_addr,
						APU_EDMA2_DESP0_48
						+ desp_offset_reg,
						req->desp.range_scale);
				edma_write_reg32(base_addr,
						APU_EDMA2_DESP0_4C
						+ desp_offset_reg,
						req->desp.min_fp32);
			} else if (req->desp.in_format == EDMA_FORMAT_FP32
				&& req->desp.out_format == EDMA_FORMAT_I8) {
				edma_write_reg32(base_addr,
						APU_EDMA2_DESP0_30
						+ desp_offset_reg,
						req->desp.range_scale);
				edma_write_reg32(base_addr,
						APU_EDMA2_DESP0_38
						+ desp_offset_reg,
						req->desp.min_fp32);
			}
			break;
		}
	case EDMA_PROC_FORMAT:{
			value = req->desp.in_format
					| (req->desp.out_format << 16);
			edma_write_reg32(base_addr,
					APU_EDMA2_DESP0_44 + desp_offset_reg,
					value);

			edma_write_reg32(base_addr,
				APU_EDMA2_DESP0_10 + desp_offset_reg,
				req->desp.src_uv_channel_stride);
			edma_write_reg32(base_addr,
				APU_EDMA2_DESP0_14 + desp_offset_reg,
				req->desp.src_uv_width_stride);
			edma_write_reg32(base_addr,
					APU_EDMA2_DESP0_30 + desp_offset_reg,
					req->desp.src_uv_addr);

			edma_write_reg32(base_addr,
					APU_EDMA2_DESP0_24 + desp_offset_reg,
					req->desp.dst_uv_channel_stride);
			edma_write_reg32(base_addr,
					APU_EDMA2_DESP0_28 + desp_offset_reg,
					req->desp.dst_uv_width_stride);
			edma_write_reg32(base_addr,
					APU_EDMA2_DESP0_38 + desp_offset_reg,
					req->desp.dst_uv_addr);

			edma_write_reg32(base_addr,
					APU_EDMA2_DESP0_48 + desp_offset_reg,
					req->desp.param_a);
			break;
		}
	case EDMA_PROC_COMPRESS:{
			value = req->desp.in_format
					| (req->desp.out_format << 16);
			edma_write_reg32(base_addr,
					APU_EDMA2_DESP0_44 + desp_offset_reg,
					value);
			edma_write_reg32(base_addr,
				APU_EDMA2_DESP0_10 + desp_offset_reg,
				req->desp.src_uv_channel_stride);
			edma_write_reg32(base_addr,
				APU_EDMA2_DESP0_14 + desp_offset_reg,
				req->desp.src_uv_width_stride);
			edma_write_reg32(base_addr,
					APU_EDMA2_DESP0_30 + desp_offset_reg,
					req->desp.src_uv_addr);

			edma_write_reg32(base_addr,
					APU_EDMA2_DESP0_24 + desp_offset_reg,
					req->desp.dst_uv_channel_stride);
			edma_write_reg32(base_addr,
					APU_EDMA2_DESP0_28 + desp_offset_reg,
					req->desp.dst_uv_width_stride);
			edma_write_reg32(base_addr,
					APU_EDMA2_DESP0_38 + desp_offset_reg,
					req->desp.dst_uv_addr);

			edma_write_reg32(base_addr,
					APU_EDMA2_DESP0_48 + desp_offset_reg,
					0);
			value = req->desp.dst_c_stride_pxl
				* req->desp.dst_w_stride_pxl / 16;
			edma_write_reg32(base_addr,
					APU_EDMA2_DESP0_4C + desp_offset_reg,
					value);
			edma_write_reg32(base_addr,
				APU_EDMA2_DESP0_58 + desp_offset_reg,
				req->desp.dst_c_stride_pxl
				| (req->desp.dst_w_stride_pxl << 16));
			edma_write_reg32(base_addr,
				APU_EDMA2_DESP0_5C + desp_offset_reg,
				req->desp.dst_c_offset_m1
				| (req->desp.dst_w_offset_m1 << 16));
			break;
		}
	case EDMA_PROC_DECOMPRESS:{
			value = req->desp.in_format
					| (req->desp.out_format << 16);
			edma_write_reg32(base_addr,
					APU_EDMA2_DESP0_44 + desp_offset_reg,
					value);
			edma_write_reg32(base_addr,
				APU_EDMA2_DESP0_10 + desp_offset_reg,
				req->desp.src_uv_channel_stride);
			edma_write_reg32(base_addr,
				APU_EDMA2_DESP0_14 + desp_offset_reg,
				req->desp.src_uv_width_stride);
			edma_write_reg32(base_addr,
					APU_EDMA2_DESP0_30 + desp_offset_reg,
					req->desp.src_uv_addr);

			edma_write_reg32(base_addr,
					APU_EDMA2_DESP0_24 + desp_offset_reg,
					req->desp.dst_uv_channel_stride);
			edma_write_reg32(base_addr,
					APU_EDMA2_DESP0_28 + desp_offset_reg,
					req->desp.dst_uv_width_stride);
			edma_write_reg32(base_addr,
					APU_EDMA2_DESP0_38 + desp_offset_reg,
					req->desp.dst_uv_addr);

			edma_write_reg32(base_addr,
					APU_EDMA2_DESP0_48 + desp_offset_reg,
					0);
			value = req->desp.dst_c_stride_pxl
				* req->desp.dst_w_stride_pxl / 16;
			edma_write_reg32(base_addr,
					APU_EDMA2_DESP0_4C + desp_offset_reg,
					value);
			edma_write_reg32(base_addr,
				APU_EDMA2_DESP0_50 + desp_offset_reg,
				req->desp.src_c_stride_pxl
				| (req->desp.src_w_stride_pxl << 16));
			edma_write_reg32(base_addr,
				APU_EDMA2_DESP0_54 + desp_offset_reg,
				req->desp.src_c_offset_m1
				| (req->desp.src_w_offset_m1 << 16));
			break;
		}
	case EDMA_PROC_RAW:{
			req->desp.plane_num--;
			value = (req->desp.plane_num << 23) | (1 << 22)
					| (req->desp.unpack_shift << 12)
					| req->desp.bit_num;
			edma_write_reg32(base_addr,
					APU_EDMA2_DESP0_44 + desp_offset_reg,
					value);
			break;
		}
	case EDMA_PROC_NORMAL:
	default:{
			edma_write_reg32(base_addr,
			APU_EDMA2_DESP0_44 + desp_offset_reg,
					0);
			break;
		}
	}

	edma_clear_reg32(base_addr, APU_EDMA2_CTL_0, EDMA_DESCRIPTOR_MODE);
	edma_set_reg32(base_addr, APU_EDMA2_CFG_0, DESP_NUM_INCR);

	return write_pointer;
}

int edma_set_external_with_normal(u8 *buf, u32 size,
					struct edma_normal *edma_normal)
{
	int ret = 0;
	u32 *start_addr;
	u32 src_tile_channel = edma_normal->tile_channel;
	u32 src_tile_width = edma_normal->tile_width;
	u32 src_tile_height = edma_normal->tile_height;
	u32 dst_tile_channel = edma_normal->tile_channel;
	u32 dst_tile_width = edma_normal->tile_width;

	if (size < APU_EDMA2_EX_DESP_OFFSET)
		return -ENOENT;

	memset(buf, 0, size);
	start_addr = (u32 *)buf;

	if (src_tile_channel)
		src_tile_channel--;
	if (src_tile_width)
		src_tile_width--;
	if (src_tile_height)
		src_tile_height--;
	if (dst_tile_channel)
		dst_tile_channel--;
	if (dst_tile_width)
		dst_tile_width--;

	*(start_addr + 0) = src_tile_channel | (src_tile_width << 16);
	*(start_addr + 1) = src_tile_height;
	*(start_addr + 2) = edma_normal->src_channel_stride;
	*(start_addr + 3) = edma_normal->src_width_stride;

	*(start_addr + 6) = dst_tile_channel | (dst_tile_width << 16);
	*(start_addr + 7) = edma_normal->dst_channel_stride;
	*(start_addr + 8) = edma_normal->dst_width_stride;

	*(start_addr + 11) = edma_normal->src_addr;
	*(start_addr + 13) = edma_normal->dst_addr;

	*(start_addr + 16) = DESP0_INT_ENABLE;

	return ret;
}
EXPORT_SYMBOL(edma_set_external_with_normal);

int edma_set_external_with_fill(u8 *buf, u32 size,
					struct edma_fill *edma_fill)
{
	int ret = 0;
	u32 *start_addr;
	u32 src_tile_channel = edma_fill->tile_channel;
	u32 src_tile_width = edma_fill->tile_width;
	u32 src_tile_height = edma_fill->tile_height;
	u32 dst_tile_channel = edma_fill->tile_channel;
	u32 dst_tile_width = edma_fill->tile_width;

	if (size < APU_EDMA2_EX_DESP_OFFSET)
		return -ENOENT;

	memset(buf, 0, size);
	start_addr = (u32 *)buf;

	if (src_tile_channel)
		src_tile_channel--;
	if (src_tile_width)
		src_tile_width--;
	if (src_tile_height)
		src_tile_height--;
	if (dst_tile_channel)
		dst_tile_channel--;
	if (dst_tile_width)
		dst_tile_width--;

	*(start_addr + 0) = src_tile_channel | (src_tile_width << 16);
	*(start_addr + 1) = src_tile_height;

	*(start_addr + 6) = dst_tile_channel | (dst_tile_width << 16);
	*(start_addr + 7) = edma_fill->dst_channel_stride;
	*(start_addr + 8) = edma_fill->dst_width_stride;

	*(start_addr + 13) = edma_fill->dst_addr;

	*(start_addr + 16) = DESP0_INT_ENABLE;
	*(start_addr + 17) = DESP0_OUT_FILL_MODE;

	return ret;
}
EXPORT_SYMBOL(edma_set_external_with_fill);

int edma_set_external_with_numerical(u8 *buf, u32 size,
					struct edma_numerical *edma_numerical)
{
	int ret = 0;
	u32 *start_addr;
	u32 src_tile_channel = edma_numerical->tile_channel;
	u32 src_tile_width = edma_numerical->tile_width;
	u32 src_tile_height = edma_numerical->tile_height;
	u32 dst_tile_channel = edma_numerical->tile_channel;
	u32 dst_tile_width = edma_numerical->tile_width;
	u8 in_format = edma_numerical->in_format;
	u8 out_format = edma_numerical->out_format;
	u32 value;

	if (size < APU_EDMA2_EX_DESP_OFFSET)
		return -ENOENT;

	memset(buf, 0, size);
	start_addr = (u32 *)buf;

	if (src_tile_channel)
		src_tile_channel--;
	if (src_tile_width)
		src_tile_width--;
	if (src_tile_height)
		src_tile_height--;
	if (dst_tile_channel)
		dst_tile_channel--;
	if (dst_tile_width)
		dst_tile_width--;

	*(start_addr + 0) = src_tile_channel | (src_tile_width << 16);
	*(start_addr + 1) = src_tile_height;
	*(start_addr + 2) = edma_numerical->src_channel_stride;
	*(start_addr + 3) = edma_numerical->src_width_stride;

	*(start_addr + 6) = dst_tile_channel | (dst_tile_width << 16);
	*(start_addr + 7) = edma_numerical->dst_channel_stride;
	*(start_addr + 8) = edma_numerical->dst_width_stride;

	*(start_addr + 11) = edma_numerical->src_addr;
	*(start_addr + 13) = edma_numerical->dst_addr;

	*(start_addr + 16) = DESP0_INT_ENABLE;
	value = in_format | (out_format << 16);
	*(start_addr + 17) = value;
	if (in_format == EDMA_FORMAT_I8 && out_format == EDMA_FORMAT_FP32) {
		*(start_addr + 18) = edma_numerical->range_scale;
		*(start_addr + 19) = edma_numerical->min_fp32;
	} else if (in_format == EDMA_FORMAT_FP32
					&& out_format == EDMA_FORMAT_I8) {
		*(start_addr + 12) = edma_numerical->range_scale;
		*(start_addr + 24) = edma_numerical->min_fp32;
	}

	return ret;
}
EXPORT_SYMBOL(edma_set_external_with_numerical);

int edma_set_external_with_format(u8 *buf, u32 size,
					struct edma_format *edma_format)
{
	int ret = 0;
	u32 *start_addr;
	u32 src_tile_channel = edma_format->src_tile_channel;
	u32 src_tile_width = edma_format->src_tile_width;
	u32 src_tile_height = edma_format->src_tile_height;
	u32 dst_tile_channel = edma_format->dst_tile_channel;
	u32 dst_tile_width = edma_format->dst_tile_width;
	u8 in_format = edma_format->in_format;
	u8 out_format = edma_format->out_format;
	u32 value;

	if (size < APU_EDMA2_EX_DESP_OFFSET)
		return -ENOENT;

	memset(buf, 0, size);
	start_addr = (u32 *)buf;

	if (src_tile_channel)
		src_tile_channel--;
	if (src_tile_width)
		src_tile_width--;
	if (src_tile_height)
		src_tile_height--;
	if (dst_tile_channel)
		dst_tile_channel--;
	if (dst_tile_width)
		dst_tile_width--;

	*(start_addr + 0) = src_tile_channel | (src_tile_width << 16);
	*(start_addr + 1) = src_tile_height;
	*(start_addr + 2) = edma_format->src_channel_stride;
	*(start_addr + 3) = edma_format->src_width_stride;

	*(start_addr + 6) = dst_tile_channel | (dst_tile_width << 16);
	*(start_addr + 7) = edma_format->dst_channel_stride;
	*(start_addr + 8) = edma_format->dst_width_stride;

	*(start_addr + 11) = edma_format->src_addr;
	*(start_addr + 13) = edma_format->dst_addr;

	*(start_addr + 16) = DESP0_INT_ENABLE;
	value = in_format | (out_format << 16);
	*(start_addr + 17) = value;
	*(start_addr + 4) = edma_format->src_uv_channel_stride;
	*(start_addr + 5) = edma_format->src_uv_width_stride;
	*(start_addr + 12) = edma_format->src_uv_addr;
	*(start_addr + 9) = edma_format->dst_uv_channel_stride;
	*(start_addr + 10) = edma_format->dst_uv_width_stride;
	*(start_addr + 14) = edma_format->dst_uv_addr;
	*(start_addr + 18) = edma_format->param_a;

	return ret;
}
EXPORT_SYMBOL(edma_set_external_with_format);

int edma_set_external_with_compress(u8 *buf, u32 size,
					struct edma_compress *edma_compress)
{
	int ret = 0;
	u32 *start_addr;
	u32 src_tile_channel = edma_compress->src_tile_channel;
	u32 src_tile_width = edma_compress->src_tile_width;
	u32 src_tile_height = edma_compress->src_tile_height;
	u32 dst_tile_channel = edma_compress->dst_tile_channel;
	u32 dst_tile_width = edma_compress->dst_tile_width;
	u8 in_format = edma_compress->in_format;
	u8 out_format = edma_compress->out_format;
	u32 value;

	if (size < APU_EDMA2_EX_DESP_OFFSET)
		return -ENOENT;

	memset(buf, 0, size);
	start_addr = (u32 *)buf;

	if (src_tile_channel)
		src_tile_channel--;
	if (src_tile_width)
		src_tile_width--;
	if (src_tile_height)
		src_tile_height--;
	if (dst_tile_channel)
		dst_tile_channel--;
	if (dst_tile_width)
		dst_tile_width--;

	*(start_addr + 0) = src_tile_channel | (src_tile_width << 16);
	*(start_addr + 1) = src_tile_height;
	*(start_addr + 2) = edma_compress->src_channel_stride;
	*(start_addr + 3) = edma_compress->src_width_stride;

	*(start_addr + 6) = dst_tile_channel | (dst_tile_width << 16);
	*(start_addr + 7) = edma_compress->dst_channel_stride;
	*(start_addr + 8) = edma_compress->dst_width_stride;

	*(start_addr + 11) = edma_compress->src_addr;
	*(start_addr + 13) = edma_compress->dst_addr;

	*(start_addr + 16) = DESP0_INT_ENABLE;
	value = in_format | (out_format << 16);
	*(start_addr + 17) = value;
	*(start_addr + 4) = edma_compress->src_uv_channel_stride;
	*(start_addr + 5) = edma_compress->src_uv_width_stride;
	*(start_addr + 12) = edma_compress->src_uv_addr;
	*(start_addr + 9) = edma_compress->dst_uv_channel_stride;
	*(start_addr + 10) = edma_compress->dst_uv_width_stride;
	*(start_addr + 14) = edma_compress->dst_uv_addr;

	value = edma_compress->dst_c_stride_pxl
			* edma_compress->dst_w_stride_pxl / 16;
	*(start_addr + 19) = value;
	*(start_addr + 22) = edma_compress->dst_c_stride_pxl
				| (edma_compress->dst_w_stride_pxl << 16);
	*(start_addr + 23) = edma_compress->dst_c_offset_m1
				| (edma_compress->dst_w_offset_m1 << 16);

	return ret;
}
EXPORT_SYMBOL(edma_set_external_with_compress);

int edma_set_external_with_decompress(u8 *buf, u32 size,
					struct edma_decompress *edma_decompress)
{
	int ret = 0;
	u32 *start_addr;
	u32 src_tile_channel = edma_decompress->src_tile_channel;
	u32 src_tile_width = edma_decompress->src_tile_width;
	u32 src_tile_height = edma_decompress->src_tile_height;
	u32 dst_tile_channel = edma_decompress->dst_tile_channel;
	u32 dst_tile_width = edma_decompress->dst_tile_width;
	u8 in_format = edma_decompress->in_format;
	u8 out_format = edma_decompress->out_format;
	u32 value;

	if (size < APU_EDMA2_EX_DESP_OFFSET)
		return -ENOENT;

	memset(buf, 0, size);
	start_addr = (u32 *)buf;

	if (src_tile_channel)
		src_tile_channel--;
	if (src_tile_width)
		src_tile_width--;
	if (src_tile_height)
		src_tile_height--;
	if (dst_tile_channel)
		dst_tile_channel--;
	if (dst_tile_width)
		dst_tile_width--;

	*(start_addr + 0) = src_tile_channel | (src_tile_width << 16);
	*(start_addr + 1) = src_tile_height;
	*(start_addr + 2) = edma_decompress->src_channel_stride;
	*(start_addr + 3) = edma_decompress->src_width_stride;

	*(start_addr + 6) = dst_tile_channel | (dst_tile_width << 16);
	*(start_addr + 7) = edma_decompress->dst_channel_stride;
	*(start_addr + 8) = edma_decompress->dst_width_stride;

	*(start_addr + 11) = edma_decompress->src_addr;
	*(start_addr + 13) = edma_decompress->dst_addr;

	*(start_addr + 16) = DESP0_INT_ENABLE;
	value = in_format | (out_format << 16);
	*(start_addr + 17) = value;
	*(start_addr + 4) = edma_decompress->src_uv_channel_stride;
	*(start_addr + 5) = edma_decompress->src_uv_width_stride;
	*(start_addr + 12) = edma_decompress->src_uv_addr;
	*(start_addr + 9) = edma_decompress->dst_uv_channel_stride;
	*(start_addr + 10) = edma_decompress->dst_uv_width_stride;
	*(start_addr + 14) = edma_decompress->dst_uv_addr;

	*(start_addr + 20) = edma_decompress->src_c_stride_pxl
				| (edma_decompress->src_w_stride_pxl << 16);
	*(start_addr + 21) = edma_decompress->src_c_offset_m1
				| (edma_decompress->src_w_offset_m1 << 16);

	return ret;
}
EXPORT_SYMBOL(edma_set_external_with_decompress);

int edma_set_external_with_raw(u8 *buf, u32 size,
					struct edma_raw *edma_raw)
{
	int ret = 0;
	u32 *start_addr;
	u32 src_tile_channel = edma_raw->src_tile_channel;
	u32 src_tile_width = edma_raw->src_tile_width;
	u32 src_tile_height = edma_raw->src_tile_height;
	u32 dst_tile_channel = edma_raw->dst_tile_channel;
	u32 dst_tile_width = edma_raw->dst_tile_width;
	u8 plane_num = edma_raw->plane_num;
	u32 value;

	if (size < APU_EDMA2_EX_DESP_OFFSET)
		return -ENOENT;

	memset(buf, 0, size);
	start_addr = (u32 *)buf;

	if (src_tile_channel)
		src_tile_channel--;
	if (src_tile_width)
		src_tile_width--;
	if (src_tile_height)
		src_tile_height--;
	if (dst_tile_channel)
		dst_tile_channel--;
	if (dst_tile_width)
		dst_tile_width--;
	if (plane_num)
		plane_num--;

	*(start_addr + 0) = src_tile_channel | (src_tile_width << 16);
	*(start_addr + 1) = src_tile_height;
	*(start_addr + 2) = edma_raw->src_channel_stride;
	*(start_addr + 3) = edma_raw->src_width_stride;

	*(start_addr + 6) = dst_tile_channel | (dst_tile_width << 16);
	*(start_addr + 7) = edma_raw->dst_channel_stride;
	*(start_addr + 8) = edma_raw->dst_width_stride;

	*(start_addr + 11) = edma_raw->src_addr;
	*(start_addr + 13) = edma_raw->dst_addr;

	*(start_addr + 16) = DESP0_INT_ENABLE;
	value = (plane_num << 23) | (1 << 22) | (edma_raw->unpack_shift << 12)
		| edma_raw->bit_num;
	*(start_addr + 17) = value;

	return ret;
}
EXPORT_SYMBOL(edma_set_external_with_raw);

void edma_teec_trigger_external(void __iomem *base_addr, u32 ext_addr, u32 num_desp,
					u8 desp_iommu_en)
{
	u32 error = 0;
	struct TEEC_Operation op = {0};
	ST_TEEC_AI_CONTROL ctrl;
	ST_MDLA_HAL_CONFIG cfg;

	ctrl.u8Version	= ST_TEEC_AI_CONTROL_VERSION;
	ctrl.u32PipelineID	= INT_MAX;
	ctrl.enEng		= E_AI_ENG_MDLA;
	ctrl.u8ModelIndex	= 0;
	ctrl.bIsSecurity	= 0;
	ctrl.u8BufCnt	= 1;
	ctrl.stBufRes[0].enBufType = EN_AI_BUF_TYPE_CODE;
	ctrl.stBufRes[0].enBufSource = EN_BUF_SOURCE_DRAM;
	ctrl.stBufRes[0].u64BufAddr	= (u64)ext_addr | MTK_IOVA_START_ADDR;
	ctrl.stBufRes[0].u32BufSize	= 0x8;

	cfg.cmd_type = 1;
	cfg.cmd.edma_hnd.sw_rst = 1;
	cfg.cmd.edma_hnd.ext_addr = ext_addr;
	cfg.cmd.edma_hnd.num_desp = num_desp;
	cfg.cmd.edma_hnd.desp_iommu_en = desp_iommu_en;

	op.params[0].tmpref.buffer = &ctrl;
	op.params[0].tmpref.size = sizeof(ctrl);
	op.params[1].tmpref.buffer = &cfg;
	op.params[1].tmpref.size = sizeof(cfg);
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT,
			TEEC_MEMREF_TEMP_INPUT, TEEC_NONE, TEEC_NONE);

	comm_util_get_cb()->send_teec_cmd(AI_TA_UUID_IDX, AI_CONTROL, &op, &error);
}

void edma_trigger_external(void __iomem *base_addr, u32 ext_addr, u32 num_desp,
					u8 desp_iommu_en)
{
	edma_set_reg32(base_addr, APU_EDMA2_CTL_0, EDMA_DESCRIPTOR_MODE);

	num_desp--;
	if (desp_iommu_en)
		edma_write_reg32(base_addr, APU_EDMA2_EXT_DESP_CFG_0,
			EXT_DESP_INT_ENABLE | EXT_DESP_USER_IOMMU | num_desp);
	else
		edma_write_reg32(base_addr, APU_EDMA2_EXT_DESP_CFG_0,
				(1<<16)/*EXT_DESP_INT_ENABLE*/ | num_desp);

	edma_write_reg32(base_addr, APU_EDMA2_EXT_DESP_CFG_1, ext_addr);
	edma_set_reg32(base_addr, APU_EDMA2_CFG_0, EXT_DESP_START);
}

void edma_test_init(void __iomem *base_addr)
{
	pr_notice("edma test init --\n");

	edma_set_reg32(base_addr, APU_EDMA3_CTRL, EXT_IOMMU_WRITE | EXT_IOMMU_READ);
}


int edma_get_available_edma(struct edma_device *edma_device)
{
	int i, value;

	for (i = 0; i < edma_device->edma_sub_num; i++) {
		struct edma_sub *edma_sub = edma_device->edma_sub[i];

		if (!edma_sub)
			continue;

		value = edma_read_reg32(edma_sub->base_addr, APU_EDMA2_CTL_0)
				& DMA_IDLE_MASK;
		if (value)
			break;
	}
	return i;
}

int edma_sync_normal_mode(struct edma_device *edma_device,
						struct edma_request *req)
{
	int ret = 0, write_pointer, sub_id;
	struct edma_sub *edma_sub;
	#if NO_INTERRUPT
	u32 status, desp0_done, desp1_done, desp2_done, desp3_done;
	u32 desp0_intr, desp1_intr, desp2_intr, desp3_intr;
	#endif

	ret = check_args(req, 0);
	if (ret) {
		pr_err("%s: check request fail, ret = %d\n", __func__, ret);
		return ret;
	}

	sub_id = edma_get_available_edma(edma_device);
	if (sub_id == edma_device->edma_sub_num) {
		pr_notice("no available edma to do\n");
		return -1;
	}

	edma_sub = edma_device->edma_sub[sub_id];
	edma_power_on(edma_sub);
	edma_enable_sequence(edma_sub);

	lock_command(edma_sub);
	write_pointer = edma_trigger_internal_mode(
				edma_sub->base_addr, req);
	#if NO_INTERRUPT
	while (1) {
		status = edma_read_reg32(edma_sub->base_addr,
					APU_EDMA2_INT_STATUS);
		desp0_done = status & DESP0_DONE_STATUS;
		desp1_done = status & DESP1_DONE_STATUS;
		desp2_done = status & DESP2_DONE_STATUS;
		desp3_done = status & DESP3_DONE_STATUS;
		desp0_intr = status & DESP0_DONE_INT_STATUS;
		desp1_intr = status & DESP1_DONE_INT_STATUS;
		desp2_intr = status & DESP2_DONE_INT_STATUS;
		desp3_intr = status & DESP3_DONE_INT_STATUS;

		if ((1<<write_pointer) & status) {
			if (desp0_intr)
				edma_set_reg32(edma_sub->base_addr,
				APU_EDMA2_INT_STATUS, DESP0_DONE_INT_STATUS);
			else if (desp1_intr)
				edma_set_reg32(edma_sub->base_addr,
				APU_EDMA2_INT_STATUS, DESP1_DONE_INT_STATUS);
			else if (desp2_intr)
				edma_set_reg32(edma_sub->base_addr,
				APU_EDMA2_INT_STATUS, DESP2_DONE_INT_STATUS);
			else if (desp3_intr)
				edma_set_reg32(edma_sub->base_addr,
				APU_EDMA2_INT_STATUS, DESP3_DONE_INT_STATUS);

			break;
		}
	}
	#else
	ret = wait_command(edma_sub, CMD_WAIT_TIME_MS);
	#endif
	if (ret) {
		pr_notice
		    ("%s:timeout\n", __func__);
		print_error_status(edma_sub, req);
	}

	unlock_command(edma_sub);
	edma_power_off(edma_sub, 0);

	return ret;
}

int edma_normal_mode(struct edma_sub *edma_sub, struct edma_request *req)
{
	int ret = 0, write_pointer;
	#if NO_INTERRUPT
	u32 status, desp0_done, desp1_done, desp2_done, desp3_done;
	u32 desp0_intr, desp1_intr, desp2_intr, desp3_intr;
	#endif

	ret = check_args(req, 0);
	if (ret) {
		pr_err("%s: check request fail, ret = %d\n", __func__, ret);
		return ret;
	}

	lock_command(edma_sub);
	write_pointer = edma_trigger_internal_mode(
			edma_sub->base_addr, req);
	#if NO_INTERRUPT
	while (1) {
		status = edma_read_reg32(edma_sub->base_addr,
					APU_EDMA2_INT_STATUS);
		desp0_done = status & DESP0_DONE_STATUS;
		desp1_done = status & DESP1_DONE_STATUS;
		desp2_done = status & DESP2_DONE_STATUS;
		desp3_done = status & DESP3_DONE_STATUS;
		desp0_intr = status & DESP0_DONE_INT_STATUS;
		desp1_intr = status & DESP1_DONE_INT_STATUS;
		desp2_intr = status & DESP2_DONE_INT_STATUS;
		desp3_intr = status & DESP3_DONE_INT_STATUS;

		if ((1<<write_pointer) & status) {
			if (desp0_intr)
				edma_set_reg32(edma_sub->base_addr,
				APU_EDMA2_INT_STATUS, DESP0_DONE_INT_STATUS);
			else if (desp1_intr)
				edma_set_reg32(edma_sub->base_addr,
				APU_EDMA2_INT_STATUS, DESP1_DONE_INT_STATUS);
			else if (desp2_intr)
				edma_set_reg32(edma_sub->base_addr,
				APU_EDMA2_INT_STATUS, DESP2_DONE_INT_STATUS);
			else if (desp3_intr)
				edma_set_reg32(edma_sub->base_addr,
				APU_EDMA2_INT_STATUS, DESP3_DONE_INT_STATUS);

			break;
		}
	}
	#else
	ret = wait_command(edma_sub, CMD_WAIT_TIME_MS);
	#endif
	if (ret) {
		pr_notice
		    ("%s:timeout\n", __func__);
		print_error_status(edma_sub, req);
	}

	unlock_command(edma_sub);

	return ret;
}

int edma_fill_mode(struct edma_sub *edma_sub, struct edma_request *req)
{
	int ret = 0, write_pointer;
	void __iomem *base_addr = edma_sub->base_addr;
	#if NO_INTERRUPT
	u32 status, desp0_done, desp1_done, desp2_done, desp3_done;
	u32 desp0_intr, desp1_intr, desp2_intr, desp3_intr;
	#endif

	ret = check_args(req, 0);
	if (ret) {
		pr_err("%s: check request fail, ret = %d\n", __func__, ret);
		return ret;
	}

	lock_command(edma_sub);

	edma_write_reg32(base_addr, APU_EDMA2_FILL_VALUE, req->fill_value);
	write_pointer = edma_trigger_internal_mode(
			edma_sub->base_addr, req);
	#if NO_INTERRUPT
	while (1) {
		status = edma_read_reg32(edma_sub->base_addr,
					APU_EDMA2_INT_STATUS);
		desp0_done = status & DESP0_DONE_STATUS;
		desp1_done = status & DESP1_DONE_STATUS;
		desp2_done = status & DESP2_DONE_STATUS;
		desp3_done = status & DESP3_DONE_STATUS;
		desp0_intr = status & DESP0_DONE_INT_STATUS;
		desp1_intr = status & DESP1_DONE_INT_STATUS;
		desp2_intr = status & DESP2_DONE_INT_STATUS;
		desp3_intr = status & DESP3_DONE_INT_STATUS;

		if ((1<<write_pointer) & status) {
			if (desp0_intr)
				edma_set_reg32(edma_sub->base_addr,
				APU_EDMA2_INT_STATUS, DESP0_DONE_INT_STATUS);
			else if (desp1_intr)
				edma_set_reg32(edma_sub->base_addr,
				APU_EDMA2_INT_STATUS, DESP1_DONE_INT_STATUS);
			else if (desp2_intr)
				edma_set_reg32(edma_sub->base_addr,
				APU_EDMA2_INT_STATUS, DESP2_DONE_INT_STATUS);
			else if (desp3_intr)
				edma_set_reg32(edma_sub->base_addr,
				APU_EDMA2_INT_STATUS, DESP3_DONE_INT_STATUS);

			break;
		}
	}
	#else
	ret = wait_command(edma_sub, CMD_WAIT_TIME_MS);
	#endif
	if (ret) {
		pr_notice
		    ("%s:timeout\n", __func__);
		print_error_status(edma_sub, req);
	}

	unlock_command(edma_sub);

	return ret;
}

int edma_numerical_mode(struct edma_sub *edma_sub, struct edma_request *req)
{
	int ret = 0, write_pointer;
	#if NO_INTERRUPT
	u32 status, desp0_done, desp1_done, desp2_done, desp3_done;
	u32 desp0_intr, desp1_intr, desp2_intr, desp3_intr;
	#endif

	ret = check_args(req, 0);
	if (ret) {
		pr_err("%s: check request fail, ret = %d\n", __func__, ret);
		return ret;
	}

	lock_command(edma_sub);
	write_pointer = edma_trigger_internal_mode(
			edma_sub->base_addr, req);
	#if NO_INTERRUPT
	while (1) {
		status = edma_read_reg32(edma_sub->base_addr,
					APU_EDMA2_INT_STATUS);
		desp0_done = status & DESP0_DONE_STATUS;
		desp1_done = status & DESP1_DONE_STATUS;
		desp2_done = status & DESP2_DONE_STATUS;
		desp3_done = status & DESP3_DONE_STATUS;
		desp0_intr = status & DESP0_DONE_INT_STATUS;
		desp1_intr = status & DESP1_DONE_INT_STATUS;
		desp2_intr = status & DESP2_DONE_INT_STATUS;
		desp3_intr = status & DESP3_DONE_INT_STATUS;

		if ((1<<write_pointer) & status) {
			if (desp0_intr)
				edma_set_reg32(edma_sub->base_addr,
				APU_EDMA2_INT_STATUS, DESP0_DONE_INT_STATUS);
			else if (desp1_intr)
				edma_set_reg32(edma_sub->base_addr,
				APU_EDMA2_INT_STATUS, DESP1_DONE_INT_STATUS);
			else if (desp2_intr)
				edma_set_reg32(edma_sub->base_addr,
				APU_EDMA2_INT_STATUS, DESP2_DONE_INT_STATUS);
			else if (desp3_intr)
				edma_set_reg32(edma_sub->base_addr,
				APU_EDMA2_INT_STATUS, DESP3_DONE_INT_STATUS);

			break;
		}
	}
	#else
	ret = wait_command(edma_sub, CMD_WAIT_TIME_MS);
	#endif
	if (ret) {
		pr_notice
		    ("%s:timeout\n", __func__);
		print_error_status(edma_sub, req);
	}

	unlock_command(edma_sub);

	return ret;
}

int edma_format_mode(struct edma_sub *edma_sub, struct edma_request *req)
{
	int ret = 0, write_pointer;
#if NO_INTERRUPT
	u32 status, desp0_done, desp1_done, desp2_done, desp3_done;
	u32 desp0_intr, desp1_intr, desp2_intr, desp3_intr;
#endif

	ret = check_args(req, 0);
	if (ret) {
		pr_err("%s: check request fail, ret = %d\n", __func__, ret);
		return ret;
	}

	lock_command(edma_sub);
	write_pointer = edma_trigger_internal_mode(
			edma_sub->base_addr, req);
#if NO_INTERRUPT
	while (1) {
		status = edma_read_reg32(edma_sub->base_addr,
					APU_EDMA2_INT_STATUS);
		desp0_done = status & DESP0_DONE_STATUS;
		desp1_done = status & DESP1_DONE_STATUS;
		desp2_done = status & DESP2_DONE_STATUS;
		desp3_done = status & DESP3_DONE_STATUS;
		desp0_intr = status & DESP0_DONE_INT_STATUS;
		desp1_intr = status & DESP1_DONE_INT_STATUS;
		desp2_intr = status & DESP2_DONE_INT_STATUS;
		desp3_intr = status & DESP3_DONE_INT_STATUS;

		if ((1<<write_pointer) & status) {
			if (desp0_intr)
				edma_set_reg32(edma_sub->base_addr,
				APU_EDMA2_INT_STATUS, DESP0_DONE_INT_STATUS);
			else if (desp1_intr)
				edma_set_reg32(edma_sub->base_addr,
				APU_EDMA2_INT_STATUS, DESP1_DONE_INT_STATUS);
			else if (desp2_intr)
				edma_set_reg32(edma_sub->base_addr,
				APU_EDMA2_INT_STATUS, DESP2_DONE_INT_STATUS);
			else if (desp3_intr)
				edma_set_reg32(edma_sub->base_addr,
				APU_EDMA2_INT_STATUS, DESP3_DONE_INT_STATUS);

			break;
		}
	}
#else
	ret = wait_command(edma_sub, CMD_WAIT_TIME_MS);
#endif
	if (ret) {
		pr_notice
		    ("%s:timeout\n", __func__);
		print_error_status(edma_sub, req);
	}

	unlock_command(edma_sub);

	return ret;
}

int edma_compress_mode(struct edma_sub *edma_sub, struct edma_request *req)
{
	int ret = 0, write_pointer;
	void __iomem *base_addr = edma_sub->base_addr;
	u32 value;
#if NO_INTERRUPT
	u32 status, desp0_done, desp1_done, desp2_done, desp3_done;
	u32 desp0_intr, desp1_intr, desp2_intr, desp3_intr;
#endif

	ret = check_args(req, 0);
	if (ret) {
		pr_err("%s: check request fail, ret = %d\n", __func__, ret);
		return ret;
	}

	lock_command(edma_sub);

	value = edma_read_reg32(base_addr, APU_EDMA2_CFG_0);
	value &= ~YUVRGB_MAT_MASK;
	value |= (req->desp.rgb2yuv_mat_bypass << 29) |
					(req->desp.rgb2yuv_mat_select << 25);
	edma_write_reg32(base_addr, APU_EDMA2_CFG_0, value);
	write_pointer = edma_trigger_internal_mode(
			edma_sub->base_addr, req);
#if NO_INTERRUPT
	while (1) {
		status = edma_read_reg32(edma_sub->base_addr,
					APU_EDMA2_INT_STATUS);
		desp0_done = status & DESP0_DONE_STATUS;
		desp1_done = status & DESP1_DONE_STATUS;
		desp2_done = status & DESP2_DONE_STATUS;
		desp3_done = status & DESP3_DONE_STATUS;
		desp0_intr = status & DESP0_DONE_INT_STATUS;
		desp1_intr = status & DESP1_DONE_INT_STATUS;
		desp2_intr = status & DESP2_DONE_INT_STATUS;
		desp3_intr = status & DESP3_DONE_INT_STATUS;

		if ((1<<write_pointer) & status) {
			if (desp0_intr)
				edma_set_reg32(edma_sub->base_addr,
				APU_EDMA2_INT_STATUS, DESP0_DONE_INT_STATUS);
			else if (desp1_intr)
				edma_set_reg32(edma_sub->base_addr,
				APU_EDMA2_INT_STATUS, DESP1_DONE_INT_STATUS);
			else if (desp2_intr)
				edma_set_reg32(edma_sub->base_addr,
				APU_EDMA2_INT_STATUS, DESP2_DONE_INT_STATUS);
			else if (desp3_intr)
				edma_set_reg32(edma_sub->base_addr,
				APU_EDMA2_INT_STATUS, DESP3_DONE_INT_STATUS);

			break;
		}
	}
#else
	ret = wait_command(edma_sub, CMD_WAIT_TIME_MS);
#endif
	if (ret) {
		pr_notice
		    ("%s:timeout\n", __func__);
		print_error_status(edma_sub, req);
	}

	unlock_command(edma_sub);

	return ret;
}

int edma_decompress_mode(struct edma_sub *edma_sub, struct edma_request *req)
{
	int ret = 0, write_pointer;
	void __iomem *base_addr = edma_sub->base_addr;
	u32 value;
#if NO_INTERRUPT
	u32 status, desp0_done, desp1_done, desp2_done, desp3_done;
	u32 desp0_intr, desp1_intr, desp2_intr, desp3_intr;
#endif

	ret = check_args(req, 0);
	if (ret) {
		pr_err("%s: check request fail, ret = %d\n", __func__, ret);
		return ret;
	}

	lock_command(edma_sub);

	value = edma_read_reg32(base_addr, APU_EDMA2_CFG_0);
	value &= ~YUVRGB_MAT_MASK;
	value |= (req->desp.yuv2rgb_mat_bypass << 30) |
					(req->desp.yuv2rgb_mat_select << 27);
	edma_write_reg32(base_addr, APU_EDMA2_CFG_0, value);
	write_pointer = edma_trigger_internal_mode(
			edma_sub->base_addr, req);
#if NO_INTERRUPT
	while (1) {
		status = edma_read_reg32(edma_sub->base_addr,
					APU_EDMA2_INT_STATUS);
		desp0_done = status & DESP0_DONE_STATUS;
		desp1_done = status & DESP1_DONE_STATUS;
		desp2_done = status & DESP2_DONE_STATUS;
		desp3_done = status & DESP3_DONE_STATUS;
		desp0_intr = status & DESP0_DONE_INT_STATUS;
		desp1_intr = status & DESP1_DONE_INT_STATUS;
		desp2_intr = status & DESP2_DONE_INT_STATUS;
		desp3_intr = status & DESP3_DONE_INT_STATUS;

		if ((1<<write_pointer) & status) {
			if (desp0_intr)
				edma_set_reg32(edma_sub->base_addr,
				APU_EDMA2_INT_STATUS, DESP0_DONE_INT_STATUS);
			else if (desp1_intr)
				edma_set_reg32(edma_sub->base_addr,
				APU_EDMA2_INT_STATUS, DESP1_DONE_INT_STATUS);
			else if (desp2_intr)
				edma_set_reg32(edma_sub->base_addr,
				APU_EDMA2_INT_STATUS, DESP2_DONE_INT_STATUS);
			else if (desp3_intr)
				edma_set_reg32(edma_sub->base_addr,
				APU_EDMA2_INT_STATUS, DESP3_DONE_INT_STATUS);

			break;
		}
	}
#else
	ret = wait_command(edma_sub, CMD_WAIT_TIME_MS);
#endif
	if (ret) {
		pr_notice
		    ("%s:timeout\n", __func__);
		print_error_status(edma_sub, req);
	}

	unlock_command(edma_sub);

	return ret;

}

int edma_raw_mode(struct edma_sub *edma_sub, struct edma_request *req)
{
	int ret = 0, write_pointer;
#if NO_INTERRUPT
	u32 status, desp0_done, desp1_done, desp2_done, desp3_done;
	u32 desp0_intr, desp1_intr, desp2_intr, desp3_intr;
#endif

	ret = check_args(req, 0);
	if (ret) {
		pr_err("%s: check request fail, ret = %d\n", __func__, ret);
		return ret;
	}

	lock_command(edma_sub);

	write_pointer = edma_trigger_internal_mode(
			edma_sub->base_addr, req);
#if NO_INTERRUPT
	while (1) {
		status = edma_read_reg32(edma_sub->base_addr,
					APU_EDMA2_INT_STATUS);
		desp0_done = status & DESP0_DONE_STATUS;
		desp1_done = status & DESP1_DONE_STATUS;
		desp2_done = status & DESP2_DONE_STATUS;
		desp3_done = status & DESP3_DONE_STATUS;
		desp0_intr = status & DESP0_DONE_INT_STATUS;
		desp1_intr = status & DESP1_DONE_INT_STATUS;
		desp2_intr = status & DESP2_DONE_INT_STATUS;
		desp3_intr = status & DESP3_DONE_INT_STATUS;

		if ((1<<write_pointer) & status) {
			if (desp0_intr)
				edma_set_reg32(edma_sub->base_addr,
				APU_EDMA2_INT_STATUS, DESP0_DONE_INT_STATUS);
			else if (desp1_intr)
				edma_set_reg32(edma_sub->base_addr,
				APU_EDMA2_INT_STATUS, DESP1_DONE_INT_STATUS);
			else if (desp2_intr)
				edma_set_reg32(edma_sub->base_addr,
				APU_EDMA2_INT_STATUS, DESP2_DONE_INT_STATUS);
			else if (desp3_intr)
				edma_set_reg32(edma_sub->base_addr,
				APU_EDMA2_INT_STATUS, DESP3_DONE_INT_STATUS);

			break;
		}
	}
#else
	ret = wait_command(edma_sub, CMD_WAIT_TIME_MS);
#endif
	if (ret) {
		pr_notice
		    ("%s:timeout\n", __func__);
		print_error_status(edma_sub, req);
	}

	unlock_command(edma_sub);

	return ret;
}

int edma_ext_mode(struct edma_sub *edma_sub, struct edma_request *req)
{
	int ret = 0;
	void __iomem *base_addr = edma_sub->base_addr;
	#if NO_INTERRUPT
	u32 status, ext_done;
	u32 ext_intr;
	#endif

	ret = check_args(req, 0);
	if (ret) {
		pr_err("%s: check request fail, ret = %d\n", __func__, ret);
		return ret;
	}

	lock_command(edma_sub);
	edma_write_reg32(base_addr, APU_EDMA2_FILL_VALUE, req->fill_value);
	edma_trigger_external(edma_sub->base_addr,
				req->ext_reg_addr,
				req->ext_count,
				req->desp_iommu_en);

	#if NO_INTERRUPT
	while (1) {
		status = edma_read_reg32(edma_sub->base_addr,
					APU_EDMA2_INT_STATUS);
		ext_done = status & EXT_DESP_DONE_STATUS;
		ext_intr = status & EXT_DESP_DONE_INT_STATUS;
		if (ext_done && ext_intr) {
			edma_set_reg32(edma_sub->base_addr,
			APU_EDMA2_INT_STATUS, EXT_DESP_DONE_INT_STATUS);
			break;
		}
	}
	#else
	ret = wait_command(edma_sub, CMD_WAIT_TIME_MS);
	#endif
	if (ret) {
		pr_notice
		    ("%s:timeout\n", __func__);
		print_error_status(edma_sub, req);
	}

	unlock_command(edma_sub);

	return ret;
}

int edma_sync_ext_mode(struct edma_device *edma_device,
						struct edma_request *req)
{
	int ret = 0, sub_id;
	struct edma_sub *edma_sub;
	void __iomem *base_addr;
	#if NO_INTERRUPT
	u32 status, ext_done;
	u32 ext_intr;
	#endif

	sub_id = edma_get_available_edma(edma_device);
	if (sub_id == edma_device->edma_sub_num) {
		pr_notice("no available edma to do\n");
		return -1;
	}

	edma_sub = edma_device->edma_sub[sub_id];
	base_addr = edma_sub->base_addr;
	edma_power_on(edma_sub);
	//edma_enable_sequence(edma_sub);

	ret = check_args(req, 1);
	if (ret) {
		pr_err("%s: check request fail, ret = %d\n", __func__, ret);
		edma_power_off(edma_sub, 0);
		return ret;
	}

	lock_command(edma_sub);
	//edma_write_reg32(base_addr, APU_EDMA2_FILL_VALUE, req->fill_value);
#if EDMA_CTRL_BY_TEE
	edma_teec_trigger_external(edma_sub->base_addr,
				req->ext_reg_addr,
				req->ext_count,
				req->desp_iommu_en);
#else
	edma_trigger_external(edma_sub->base_addr,
				req->ext_reg_addr,
				req->ext_count,
				req->desp_iommu_en);
#endif

	#if NO_INTERRUPT
	while (1) {
		status = edma_read_reg32(edma_sub->base_addr,
					APU_EDMA2_INT_STATUS);
		ext_done = status & EXT_DESP_DONE_STATUS;
		ext_intr = status & EXT_DESP_DONE_INT_STATUS;
		if (ext_done && ext_intr) {
			edma_set_reg32(edma_sub->base_addr,
					APU_EDMA2_INT_STATUS,
					EXT_DESP_DONE_INT_STATUS);
			break;
		}
	}
	#else
	ret = wait_command(edma_sub, CMD_WAIT_TIME_MS);
	#endif
	if (ret) {
		pr_notice
		    ("%s:timeout\n", __func__);
		print_error_status(edma_sub, req);
	}

	unlock_command(edma_sub);
	edma_power_off(edma_sub, 0);

	return ret;
}

int edma_ext_by_sub(struct edma_sub *edma_sub, struct edma_request *req)
{
	int ret = 0;
	void __iomem *base_addr;
	ktime_t t1, t2;

	base_addr = edma_sub->base_addr;
	ret = edma_power_on(edma_sub);
	if (ret != 0)
		return ret;

	// if buffer come from sram, need to check power on status
	ret = check_args(req, 0);
	if (ret) {
		pr_err("%s(): check request fail, ret = %d\n", __func__, ret);
		edma_power_off(edma_sub, 0);
		return ret;
	}

#if (EDMA_CTRL_BY_TEE == 0)
	edma_enable_sequence(edma_sub);
	edma_sw_reset(edma_sub); // no need in edma 3.0
#endif

	LOG_DBG("%s():ext_reg_addr = 0x%x, desp_iommu_en = %d\n",
			__func__, req->ext_reg_addr, req->desp_iommu_en);

	lock_command(edma_sub);
	t1 = ktime_get();
	LOG_DBG("edma_trigger_external +\n");

	//edma_write_reg32(base_addr, APU_EDMA2_FILL_VALUE, req->fill_value);
#if EDMA_CTRL_BY_TEE
	edma_teec_trigger_external(edma_sub->base_addr,
				req->ext_reg_addr,
				req->ext_count,
				req->desp_iommu_en);
#else
	edma_trigger_external(edma_sub->base_addr,
				req->ext_reg_addr,
				req->ext_count,
				req->desp_iommu_en);
#endif

	LOG_DBG("edma_trigger_external -\n");

	ret = wait_command(edma_sub, CMD_WAIT_TIME_MS);
	if (ret) {
		pr_notice
		    ("%s:timeout\n", __func__);
		print_error_status(edma_sub, req);
	}

	t2 = ktime_get();
	edma_sub->ip_time = ktime_to_us(ktime_sub(t2, t1));
	unlock_command(edma_sub);
	edma_power_off(edma_sub, 0);

	return ret;
}

bool edma_is_all_power_off(struct edma_device *edma_device)
{
	int i;

	for (i = 0; i < edma_device->edma_sub_num; i++)
		if (edma_device->edma_sub[i]->power_state == EDMA_POWER_ON)
			return false;

	return true;
}

int edma_power_on(struct edma_sub *edma_sub)
{
	struct edma_device *edma_device;
	int ret = 0;

	edma_device = edma_sub->edma_device;
	mutex_lock(&edma_device->power_mutex);
	if (timer_pending(&edma_device->power_timer)) {
		del_timer(&edma_device->power_timer);
		LOG_DBG("%s(): deltimer no pwr on, state = %d\n",
				__func__, edma_device->power_state);
	} else if ((edma_device->power_on_cnt == 0) &&
			(edma_device->power_state == EDMA_POWER_OFF)) {
		// no edma user and check power_state is already off
		ret = apu_device_power_on(EDMA);
		if (!ret) {
			LOG_DBG("%s(): power on success\n", __func__);
			edma_device->power_state = EDMA_POWER_ON;
		} else {
			LOG_ERR("%s(): power on fail\n",
							__func__);
			mutex_unlock(&edma_device->power_mutex);
			return ret;
		}
	} else {
		LOG_ERR("%s(): no pwr on, cnt = %u\n",
				__func__, edma_device->power_on_cnt);
	}

	edma_device->power_on_cnt++;
	edma_sub->power_state = EDMA_POWER_ON;
	mutex_unlock(&edma_device->power_mutex);
	return ret;
}

void edma_start_power_off(struct work_struct *work)
{
	int ret = 0;
	struct edma_device *edmaDev = NULL;

	edmaDev =
	    container_of(work, struct edma_device, power_off_work);

	LOG_DBG("%s(): contain power_state = %d!!\n", __func__,
		edmaDev->power_state);

	if (edmaDev->power_state == EDMA_POWER_OFF)
		LOG_ERR("%s(): pwr off twice\n",
						__func__);

	mutex_lock(&edmaDev->power_mutex);

	if (edmaDev->power_on_cnt == 0) {
		ret = apu_device_power_off(EDMA);
		if (ret != 0) {
			LOG_ERR("%s(): power off fail\n", __func__);
		} else {
			pr_notice("%s(): power off done!!\n", __func__);
			edmaDev->power_state = EDMA_POWER_OFF;
		}
	}
	mutex_unlock(&edmaDev->power_mutex);

}

void edma_power_time_up(struct timer_list *tlist)
{
	//struct edma_device *edma_device = (struct edma_device *)data;

	struct edma_device *edma_device =
	    container_of(tlist, struct edma_device, power_timer);

	//use kwork job to prevent power off at irq
	schedule_work(&edma_device->power_off_work);
}

int edma_power_off(struct edma_sub *edma_sub, u8 force)
{
	struct edma_device *edma_device =
		edma_sub->edma_device;
	int ret = 0;

	if (edma_device->dbg_cfg
		& EDMA_DBG_DISABLE_PWR_OFF) {

		LOG_INF("%s():no power off!!\n", __func__);

		return 0;
	}

	mutex_lock(&edma_device->power_mutex);
	edma_sub->power_state = EDMA_POWER_OFF;

	if (timer_pending(&edma_device->power_timer))
		del_timer(&edma_device->power_timer);

	if ((force == 1) || (g_edma_power_off_time == 0)) {

		if (edma_device->power_state != EDMA_POWER_OFF) {
			ret = apu_device_power_suspend(EDMA, 1);
			if (!ret) {
				LOG_INF("%s(): force power off success\n",
						__func__);
				edma_device->power_state =
					EDMA_POWER_OFF;
			} else {
				LOG_ERR("%s(): force power off fail\n",
					__func__);
				mutex_unlock(&edma_device->power_mutex);
				return ret;
			}

		} else
			LOG_ERR("%s(): force power off skip\n",
					__func__);
		edma_device->power_on_cnt = 0;

	} else {
		edma_device->power_timer.expires = jiffies +
		msecs_to_jiffies(g_edma_power_off_time);
		add_timer(&edma_device->power_timer);
	}

	if (edma_device->power_on_cnt)
		edma_device->power_on_cnt--;

	mutex_unlock(&edma_device->power_mutex);
	LOG_INF("%s(): power off, cnt:%x\n", __func__,  edma_device->power_on_cnt);

	return ret;

}

int edma_execute(struct edma_sub *edma_sub, struct edma_ext *edma_ext)
{
	int ret = 0;
	struct edma_request req = {0};
	ktime_t t1, t2;
	uint32_t exe_time;

	t1 = ktime_get();

	edma_setup_ext_mode_request(&req, edma_ext, EDMA_PROC_EXT_MODE);
	ret = edma_ext_by_sub(edma_sub, &req);

	t2 = ktime_get();
	exe_time = ktime_to_us(ktime_sub(t2, t1));

	LOG_DBG("%s():function done, exe_time = %d, ip time = %d\n",
		__func__, exe_time, edma_sub->ip_time);

	return ret;
}

