// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#include <dt-bindings/gce/mt6873-gce.h>

#include "cmdq-util.h"

const char *cmdq_thread_module_dispatch(phys_addr_t gce_pa, s32 thread)
{
	switch (thread) {
	case 0 ... 6:
	case 8 ... 9:
		return "DISP";
	case 7:
		return "VDEC";
	case 10:
	case 19 ... 22:
		return "MDP";
	case 11:
	case 13 ... 14:
	case 16 ... 18:
		return "ISP";
	case 12:
		return "VENC";
	case 15:
		return "CMDQ";
	case 23:
		return "SMI";
	default:
		return "CMDQ";
	}
}
EXPORT_SYMBOL(cmdq_thread_module_dispatch);

const char *cmdq_event_module_dispatch(phys_addr_t gce_pa, const u16 event,
	s32 thread)
{
	switch (event) {
	case CMDQ_EVENT_VDEC_LAT_SOF_0 ... CMDQ_EVENT_VDEC_LAT_ENG_EVENT_7:
	case CMDQ_EVENT_VDEC_CORE0_SOF_0 ... CMDQ_EVENT_VDEC_CORE0_ENG_EVENT_7:
		return "VDEC";
	case CMDQ_EVENT_ISP_FRAME_DONE_A ... CMDQ_EVENT_CQ_VR_SNAP_C_INT:
		return "ISP";
	case CMDQ_EVENT_VENC_CMDQ_FRAME_DONE ... CMDQ_EVENT_VENC_CMDQ_VPS_DONE:
		return "VENC";
	case CMDQ_EVENT_FDVT_DONE:
		return "FDVT";
	case CMDQ_EVENT_RSC_DONE:
		return "RSC";
	case CMDQ_EVENT_IMG2_DIP_FRAME_DONE_P2_0
		... CMDQ_EVENT_IMG1_MSS_DONE_LINK_MISC:
		return "DIP";
	case CMDQ_EVENT_MDP_RDMA0_SOF
		... CMDQ_EVENT_MDP_RDMA0_SW_RST_DONE_ENG_EVENT:
		return "MDP";
	case CMDQ_EVENT_DISP_OVL0_SOF ... CMDQ_EVENT_BUF_UNDERRUN_ENG_EVENT_7:
	case CMDQ_SYNC_TOKEN_CONFIG_DIRTY ... CMDQ_SYNC_TOKEN_CABC_EOF:
		return "DISP";
	default:
		return cmdq_thread_module_dispatch(gce_pa, thread);
	}
}
EXPORT_SYMBOL(cmdq_event_module_dispatch);

u32 cmdq_util_hw_id(u32 pa)
{
	return 0;
}
EXPORT_SYMBOL(cmdq_util_hw_id);

u32 cmdq_test_get_subsys_list(u32 **regs_out)
{
	static u32 regs[] = {
		0x14000100,	/* mmsys MMSYS_CG_CON0 */
		0x11f600a0,	/* msdc0 SW_DBG_SEL */
		0x1121004c,	/* audio AFE_I2S_CON3_OFFSET */
		0x110020bc,	/* uart0 */
	};

	*regs_out = regs;
	return ARRAY_SIZE(regs);
}
EXPORT_SYMBOL(cmdq_test_get_subsys_list);

const char *cmdq_util_hw_name(void *chan)
{
	return "GCE";
}
EXPORT_SYMBOL(cmdq_util_hw_name);

bool cmdq_thread_ddr_module(const s32 thread)
{
	switch (thread) {
	case 0 ... 6:
	case 8 ... 9:
	case 15:
		return false;
	default:
		return true;
	}
}
EXPORT_SYMBOL(cmdq_thread_ddr_module);

static int __init cmdq_platform_init(void)
{
	return 0;
}
module_init(cmdq_platform_init);

MODULE_LICENSE("GPL v2");
