/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef _VOC_DRV_MAILBOX_H
#define _VOC_DRV_MAILBOX_H
//------------------------------------------------------------------------------
//  Include Files
//------------------------------------------------------------------------------
#include "voc_common.h"
#include "voc_communication.h"
//------------------------------------------------------------------------------
//  Macros
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
//  Variables
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// Enumerate And Structure
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// Global Function
//------------------------------------------------------------------------------
int voc_drv_mailbox_bind(struct mtk_snd_voc *snd_voc);
int voc_drv_mailbox_vd_task_state(void);
int voc_drv_mailbox_dma_start_channel(void);
int voc_drv_mailbox_dma_stop_channel(void);
int voc_drv_mailbox_dma_init_channel(
		uint64_t dma_paddr,
		uint32_t buf_size,
		uint32_t channels,
		uint32_t sample_width,
		uint32_t sample_rate);
int voc_drv_mailbox_dma_enable_sinegen(bool en);
int voc_drv_mailbox_dma_get_reset_status(void);
int voc_drv_mailbox_enable_vq(bool en);
int voc_drv_mailbox_config_vq(uint8_t mode);
int voc_drv_mailbox_enable_hpf(int32_t stage);
int voc_drv_mailbox_config_hpf(int32_t coeff);
int voc_drv_mailbox_dmic_gain(int16_t gain);
int voc_drv_mailbox_dmic_mute(bool en);
int voc_drv_mailbox_dmic_switch(bool en);
int voc_drv_mailbox_reset_voice(void);
int voc_drv_mailbox_update_snd_model(dma_addr_t snd_model_paddr, uint32_t size);
int voc_drv_mailbox_get_hotword_ver(void);
int voc_drv_mailbox_get_hotword_identifier(void);
int voc_drv_mailbox_enable_slt_test(int32_t loop, uint32_t addr);
int voc_drv_mailbox_enable_seamless(bool en);
int voc_drv_mailbox_notify_status(int32_t status);
int voc_drv_mailbox_ts_sync_start(void);
int voc_drv_mailbox_ts_delay_req(void);
struct voc_communication_operater *voc_drv_mailbox_get_op(void);
void voc_drv_mailbox_init(void);
#endif /* _VOC_DRV_MAILBOX_H */
