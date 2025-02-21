/* SPDX-License-Identifier: GPL-2.0 */
// Copyright (c) 2020 MediaTek Inc.

#ifndef __MTK_CAM_SENINF_SENINF1_H__
#define __MTK_CAM_SENINF_SENINF1_H__

#define SENINF_CTRL 0x0000
#define SENINF_EN_SHIFT 0
#define SENINF_EN_MASK (0x1 << 0)

#define SENINF_DBG 0x0004
#define RG_SENINF_DBG_SEL_SHIFT 0
#define RG_SENINF_DBG_SEL_MASK (0xf << 0)

#define SENINF_CSI2_CTRL 0x0010
#define RG_SENINF_CSI2_EN_SHIFT 0
#define RG_SENINF_CSI2_EN_MASK (0x1 << 0)
#define SENINF_CSI2_SW_RST_SHIFT 4
#define SENINF_CSI2_SW_RST_MASK (0x1 << 4)

#define SENINF_TESTMDL_CTRL 0x0020
#define RG_SENINF_TESTMDL_EN_SHIFT 0
#define RG_SENINF_TESTMDL_EN_MASK (0x1 << 0)
#define SENINF_TESTMDL_SW_RST_SHIFT 4
#define SENINF_TESTMDL_SW_RST_MASK (0x1 << 4)

#define SENINF_TG_CTRL 0x0030
#define SENINF_TG_SW_RST_SHIFT 4
#define SENINF_TG_SW_RST_MASK (0x1 << 4)

#define SENINF_SCAM_CTRL 0x0040
#define RG_SENINF_SCAM_EN_SHIFT 0
#define RG_SENINF_SCAM_EN_MASK (0x1 << 0)
#define SENINF_SCAM_SW_RST_SHIFT 4
#define SENINF_SCAM_SW_RST_MASK (0x1 << 4)

#define SENINF_PCAM_CTRL 0x0050
#define RG_SENINF_PCAM_DATA_SEL_SHIFT 0
#define RG_SENINF_PCAM_DATA_SEL_MASK (0x7 << 0)

#define SENINF_CCIR_CTRL 0x0060
#define SENINF_CCIR_SW_RST_SHIFT 4
#define SENINF_CCIR_SW_RST_MASK (0x1 << 4)

#define SENINF_SPARE 0x00f8
#define RG_SENINF_SPARE_0_SHIFT 0
#define RG_SENINF_SPARE_0_MASK (0xff << 0)
#define RG_SENINF_SPARE_1_SHIFT 16
#define RG_SENINF_SPARE_1_MASK (0xff << 16)

#endif
