/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2016 MediaTek Inc.
 * Author: PC Chen <pc.chen@mediatek.com>
 *         Tiffany Lin <tiffany.lin@mediatek.com>
 */

#ifndef _MTK_VCODEC_DEC_H_
#define _MTK_VCODEC_DEC_H_

#include <media/videobuf2-core.h>
#include <media/videobuf2-v4l2.h>
#include "mtk_vcodec_drv.h"

#ifdef CONFIG_MSTAR_CHIP
#ifndef TV_INTEGRATION
#define TV_INTEGRATION
#endif
#endif

#define VCODEC_CAPABILITY_4K_DISABLED   0x10
#define VCODEC_DEC_4K_CODED_WIDTH       4096U
#define VCODEC_DEC_4K_CODED_HEIGHT      2304U
#define MTK_VDEC_MAX_W  2048U
#define MTK_VDEC_MAX_H  1088U
#ifdef TV_INTEGRATION
#define MTK_MAX_CTRLS_HINT      100
#else
#define MTK_MAX_CTRLS_HINT      20
#endif

/**
 * struct vdec_fb  - decoder frame buffer
 * @fb_base     : frame buffer plane memory info
 * @status      : frame buffer status (vdec_fb_status)
 * @num_planes  : frame buffer plane number
 * @timestamp : frame buffer timestamp
 * @index       : frame buffer index in vb2 queue
 */
struct vdec_fb {
	struct mtk_vcodec_mem   fb_base[VIDEO_MAX_PLANES];
	unsigned int    status;
	unsigned int    num_planes;
	long long timestamp;
	unsigned int    index;
	enum v4l2_field field;
	enum mtk_frame_type frame_type;
	bool            drop;
	int             general_buf_fd;
	struct dma_buf *dma_general_buf;
	int slice_done_count;
};

/**
 * enum eos_types  - decoder different eos types
 * @NON_EOS     : no eos, normal frame
 * @EOS_WITH_DATA      : early eos , mean this frame need to decode
 * @EOS : byteused of the last frame is zero
 */
enum eos_types {
	NON_EOS = 0,
	EOS_WITH_DATA,
	EOS
};

/**
 * struct mtk_video_dec_buf - Private data related to each VB2 buffer.
 * @b:          VB2 buffer
 * @list:       link list
 * @used:       Capture buffer contain decoded frame data and keep in
 *                      codec data structure
 * @ready_to_display:   Capture buffer not display yet
 * @queued_in_vb2:      Capture buffer is queue in vb2
 * @queued_in_v4l2:     Capture buffer is in v4l2 driver, but not in vb2
 *                      queue yet
 * @lastframe:  Output buffer is last buffer - EOS
 * @bs_buffer:  Decode status, and buffer information of Output buffer
 * @frame_buffer:       Decode status, and buffer information of Capture buffer
 * @flags:  flags derived from v4l2_buffer for buffer operations
 *
 * Note : These status information help us track and debug buffer state
 */
struct mtk_video_dec_buf {
	struct vb2_v4l2_buffer  vb;
	struct list_head        list;

	bool    used;
	bool    ready_to_display;
	bool    queued_in_vb2;
	bool    queued_in_v4l2;
	enum eos_types lastframe;
	struct mtk_vcodec_mem bs_buffer;
	struct vdec_fb  frame_buffer;
	int     flags;
	int     general_buf_fd;
	void   *general_vb2_buf;
	struct hdr10plus_info hdr10plus_buf;
};

extern const struct v4l2_ioctl_ops mtk_vdec_ioctl_ops;
extern const struct v4l2_m2m_ops mtk_vdec_m2m_ops;


/*
 * mtk_vdec_lock/mtk_vdec_unlock are for ctx instance to
 * get/release lock before/after access decoder hw.
 * mtk_vdec_lock get decoder hw lock and set curr_ctx
 * to ctx instance that get lock
 */
void mtk_vdec_unlock(struct mtk_vcodec_ctx *ctx, u32 hw_id);
void mtk_vdec_lock(struct mtk_vcodec_ctx *ctx, u32 hw_id);
int mtk_vcodec_dec_queue_init(void *priv, struct vb2_queue *src_vq,
	struct vb2_queue *dst_vq);
void mtk_vcodec_dec_set_default_params(struct mtk_vcodec_ctx *ctx);
void mtk_vcodec_dec_empty_queues(struct file *file, struct mtk_vcodec_ctx *ctx);
void mtk_vcodec_dec_release(struct mtk_vcodec_ctx *ctx);
int mtk_vcodec_dec_ctrls_setup(struct mtk_vcodec_ctx *ctx);


#endif /* _MTK_VCODEC_DEC_H_ */
