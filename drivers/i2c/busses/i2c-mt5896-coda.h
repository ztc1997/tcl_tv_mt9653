/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef _I2C_MT5896_CODA_H_
#define _I2C_MT5896_CODA_H_

//Page MIIC0
#define HWI2C_REG_0000_MIIC0 (0x0000)
    #define REG_0000_MIIC0_REG_MIIC_CFG Fld(8, 0, AC_FULLB0)//[7:0]
#define HWI2C_REG_0004_MIIC0 (0x0004)
    #define REG_0004_MIIC0_REG_CMD_START Fld(1, 0, AC_MSKB0)//[0:0]
    #define REG_0004_MIIC0_REG_CMD_STOP Fld(1, 8, AC_MSKB1)//[8:8]
#define HWI2C_REG_0008_MIIC0 (0x0008)
    #define REG_0008_MIIC0_REG_WDATA Fld(8, 0, AC_FULLB0)//[7:0]
    #define REG_0008_MIIC0_REG_WRITE_ACK Fld(1, 8, AC_MSKB1)//[8:8]
#define HWI2C_REG_000C_MIIC0 (0x000C)
    #define REG_000C_MIIC0_REG_RDATA Fld(8, 0, AC_FULLB0)//[7:0]
    #define REG_000C_MIIC0_REG_RDATA_EN Fld(1, 8, AC_MSKB1)//[8:8]
    #define REG_000C_MIIC0_REG_ACK_BIT Fld(1, 9, AC_MSKB1)//[9:9]
#define HWI2C_REG_0010_MIIC0 (0x0010)
    #define REG_0010_MIIC0_REG_FLAG Fld(1, 0, AC_MSKB0)//[0:0]
#define HWI2C_REG_0014_MIIC0 (0x0014)
    #define REG_0014_MIIC0_REG_MIIC_STATE Fld(5, 0, AC_MSKB0)//[4:0]
    #define REG_0014_MIIC0_REG_MIIC_INT_STATUS Fld(7, 8, AC_MSKB1)//[14:8]
#define HWI2C_REG_0018_MIIC0 (0x0018)
    #define REG_0018_MIIC0_REG_SCLI Fld(1, 0, AC_MSKB0)//[0:0]
    #define REG_0018_MIIC0_REG_SCLO Fld(1, 4, AC_MSKB0)//[4:4]
    #define REG_0018_MIIC0_REG_SDAI Fld(1, 1, AC_MSKB0)//[1:1]
#define HWI2C_REG_0020_MIIC0 (0x0020)
    #define REG_0020_MIIC0_REG_STOP_CNT Fld(16, 0, AC_FULLW10)//[15:0]
#define HWI2C_REG_0024_MIIC0 (0x0024)
    #define REG_0024_MIIC0_REG_HCNT Fld(16, 0, AC_FULLW10)//[15:0]
#define HWI2C_REG_0028_MIIC0 (0x0028)
    #define REG_0028_MIIC0_REG_LCNT Fld(16, 0, AC_FULLW10)//[15:0]
#define HWI2C_REG_002C_MIIC0 (0x002C)
    #define REG_002C_MIIC0_REG_SDA_CNT Fld(16, 0, AC_FULLW10)//[15:0]
#define HWI2C_REG_0030_MIIC0 (0x0030)
    #define REG_0030_MIIC0_REG_START_CNT Fld(16, 0, AC_FULLW10)//[15:0]
#define HWI2C_REG_0034_MIIC0 (0x0034)
    #define REG_0034_MIIC0_REG_DATA_LAT_CNT Fld(16, 0, AC_FULLW10)//[15:0]
#define HWI2C_REG_0038_MIIC0 (0x0038)
    #define REG_0038_MIIC0_REG_TIMEOUT_CNT Fld(16, 0, AC_FULLW10)//[15:0]
#define HWI2C_REG_003C_MIIC0 (0x003C)
    #define REG_003C_MIIC0_REG_SCLI_DELAY Fld(3, 0, AC_MSKB0)//[2:0]
#define HWI2C_REG_0040_MIIC0 (0x0040)
    #define REG_0040_MIIC0_REG_RESERVE_0 Fld(8, 0, AC_FULLB0)//[7:0]
    #define REG_0040_MIIC0_REG_RESERVE_1 Fld(8, 8, AC_FULLB1)//[15:8]
#define HWI2C_REG_0044_MIIC0 (0x0044)
    #define REG_0044_MIIC0_REG_START_SETUP_CNT Fld(16, 0, AC_FULLW10)//[15:0]
#define HWI2C_REG_0048_MIIC0 (0x0048)
    #define REG_0048_MIIC0_REG_STOP_HOLD_CNT Fld(16, 0, AC_FULLW10)//[15:0]
#define HWI2C_REG_004C_MIIC0 (0x004C)
    #define REG_004C_MIIC0_REG_BYTE2BYTE_CNT Fld(16, 0, AC_FULLW10)//[15:0]
#define HWI2C_REG_0050_MIIC0 (0x0050)
    #define REG_0050_MIIC0_REG_SCL_SDA_SWAP Fld(1, 0, AC_MSKB0)//[0:0]
    #define REG_0050_MIIC0_REG_MI2C_ACK_CNT_EN Fld(1, 1, AC_MSKB0)//[1:1]
#define HWI2C_REG_0054_MIIC0 (0x0054)
    #define REG_0054_MIIC0_REG_MI2C_ACK_CNT Fld(16, 0, AC_FULLW10)//[15:0]
#define HWI2C_REG_0080_MIIC0 (0x0080)
    #define REG_0080_MIIC0_REG_DMA_CFG Fld(3, 0, AC_MSKB0)//[2:0]
    #define REG_0080_MIIC0_REG_MIU_RST Fld(1, 3, AC_MSKB0)//[3:3]
    #define REG_0080_MIIC0_REG_MIU_PRIORITY Fld(1, 4, AC_MSKB0)//[4:4]
    #define REG_0080_MIIC0_REG_MIU_NS Fld(1, 5, AC_MSKB0)//[5:5]
    #define REG_0080_MIIC0_REG_DMA_USE_REG Fld(1, 6, AC_MSKB0)//[6:6]
#define HWI2C_REG_0084_MIIC0 (0x0084)
    #define REG_0084_MIIC0_REG_MIU_ADDR_0 Fld(16, 0, AC_FULLW10)//[15:0]
#define HWI2C_REG_0088_MIIC0 (0x0088)
    #define REG_0088_MIIC0_REG_MIU_ADDR_1 Fld(16, 0, AC_FULLW10)//[15:0]
#define HWI2C_REG_008C_MIIC0 (0x008C)
    #define REG_008C_MIIC0_REG_STOP_DISABLE Fld(1, 5, AC_MSKB0)//[5:5]
    #define REG_008C_MIIC0_REG_READ_CMD Fld(1, 6, AC_MSKB0)//[6:6]
    #define REG_008C_MIIC0_REG_MIU_SEL Fld(2, 7, AC_MSKW10)//[8:7]
#define HWI2C_REG_0090_MIIC0 (0x0090)
    #define REG_0090_MIIC0_REG_DMA_TRANSFER_DONE Fld(1, 0, AC_MSKB0)//[0:0]
#define HWI2C_REG_0094_MIIC0 (0x0094)
    #define REG_0094_MIIC0_REG_CMD_DATA_0 Fld(16, 0, AC_FULLW10)//[15:0]
#define HWI2C_REG_0098_MIIC0 (0x0098)
    #define REG_0098_MIIC0_REG_CMD_DATA_1 Fld(16, 0, AC_FULLW10)//[15:0]
#define HWI2C_REG_009C_MIIC0 (0x009C)
    #define REG_009C_MIIC0_REG_CMD_DATA_2 Fld(16, 0, AC_FULLW10)//[15:0]
#define HWI2C_REG_00A0_MIIC0 (0x00A0)
    #define REG_00A0_MIIC0_REG_CMD_DATA_3 Fld(16, 0, AC_FULLW10)//[15:0]
#define HWI2C_REG_00A4_MIIC0 (0x00A4)
    #define REG_00A4_MIIC0_REG_CMD_LEN Fld(4, 0, AC_MSKB0)//[3:0]
#define HWI2C_REG_00A8_MIIC0 (0x00A8)
    #define REG_00A8_MIIC0_REG_DATA_LEN_0 Fld(16, 0, AC_FULLW10)//[15:0]
#define HWI2C_REG_00AC_MIIC0 (0x00AC)
    #define REG_00AC_MIIC0_REG_DATA_LEN_1 Fld(16, 0, AC_FULLW10)//[15:0]
#define HWI2C_REG_00B0_MIIC0 (0x00B0)
    #define REG_00B0_MIIC0_REG_DMA_TC_0 Fld(16, 0, AC_FULLW10)//[15:0]
#define HWI2C_REG_00B4_MIIC0 (0x00B4)
    #define REG_00B4_MIIC0_REG_DMA_TC_1 Fld(16, 0, AC_FULLW10)//[15:0]
#define HWI2C_REG_00B8_MIIC0 (0x00B8)
    #define REG_00B8_MIIC0_REG_SAR Fld(10, 0, AC_MSKW10)//[9:0]
    #define REG_00B8_MIIC0_REG_10BIT_MODE Fld(1, 10, AC_MSKB1)//[10:10]
#define HWI2C_REG_00BC_MIIC0 (0x00BC)
    #define REG_00BC_MIIC0_REG_DMA_TRIGGER Fld(1, 0, AC_MSKB0)//[0:0]
    #define REG_00BC_MIIC0_REG_RE_TRIGGER Fld(1, 8, AC_MSKB1)//[8:8]
#define HWI2C_REG_00C0_MIIC0 (0x00C0)
    #define REG_00C0_MIIC0_REG_RESERVE_0 Fld(8, 0, AC_FULLB0)//[7:0]
    #define REG_00C0_MIIC0_REG_RESERVE_1 Fld(8, 8, AC_FULLB1)//[15:8]
#define HWI2C_REG_00C4_MIIC0 (0x00C4)
    #define REG_00C4_MIIC0_REG_STATE Fld(8, 0, AC_FULLB0)//[7:0]
    #define REG_00C4_MIIC0_REG_MIU_LAST_DONE_Z Fld(1, 8, AC_MSKB1)//[8:8]
#define HWI2C_REG_00C8_MIIC0 (0x00C8)
    #define REG_00C8_MIIC0_REG_MIU_ADDR_B39_32 Fld(8, 0, AC_FULLB0)//[7:0]
#define HWI2C_REG_0100_MIIC0 (0x0100)
    #define REG_0100_MIIC0_REG_WBUF_00 Fld(8, 0, AC_FULLB0)//[7:0]
    #define REG_0100_MIIC0_REG_WBUF_01 Fld(8, 8, AC_FULLB1)//[15:8]
#define HWI2C_REG_0104_MIIC0 (0x0104)
    #define REG_0104_MIIC0_REG_WBUF_02 Fld(8, 0, AC_FULLB0)//[7:0]
    #define REG_0104_MIIC0_REG_WBUF_03 Fld(8, 8, AC_FULLB1)//[15:8]
#define HWI2C_REG_0108_MIIC0 (0x0108)
    #define REG_0108_MIIC0_REG_WBUF_04 Fld(8, 0, AC_FULLB0)//[7:0]
    #define REG_0108_MIIC0_REG_WBUF_05 Fld(8, 8, AC_FULLB1)//[15:8]
#define HWI2C_REG_010C_MIIC0 (0x010C)
    #define REG_010C_MIIC0_REG_WBUF_06 Fld(8, 0, AC_FULLB0)//[7:0]
    #define REG_010C_MIIC0_REG_WBUF_07 Fld(8, 8, AC_FULLB1)//[15:8]
#define HWI2C_REG_0110_MIIC0 (0x0110)
    #define REG_0110_MIIC0_REG_WBUF_08 Fld(8, 0, AC_FULLB0)//[7:0]
    #define REG_0110_MIIC0_REG_WBUF_09 Fld(8, 8, AC_FULLB1)//[15:8]
#define HWI2C_REG_0114_MIIC0 (0x0114)
    #define REG_0114_MIIC0_REG_WBUF_10 Fld(8, 0, AC_FULLB0)//[7:0]
    #define REG_0114_MIIC0_REG_WBUF_11 Fld(8, 8, AC_FULLB1)//[15:8]
#define HWI2C_REG_0118_MIIC0 (0x0118)
    #define REG_0118_MIIC0_REG_WBUF_12 Fld(8, 0, AC_FULLB0)//[7:0]
    #define REG_0118_MIIC0_REG_WBUF_13 Fld(8, 8, AC_FULLB1)//[15:8]
#define HWI2C_REG_011C_MIIC0 (0x011C)
    #define REG_011C_MIIC0_REG_WBUF_14 Fld(8, 0, AC_FULLB0)//[7:0]
    #define REG_011C_MIIC0_REG_WBUF_15 Fld(8, 8, AC_FULLB1)//[15:8]
#define HWI2C_REG_0120_MIIC0 (0x0120)
    #define REG_0120_MIIC0_REG_WBUF_16 Fld(8, 0, AC_FULLB0)//[7:0]
    #define REG_0120_MIIC0_REG_WBUF_17 Fld(8, 8, AC_FULLB1)//[15:8]
#define HWI2C_REG_0124_MIIC0 (0x0124)
    #define REG_0124_MIIC0_REG_WBUF_18 Fld(8, 0, AC_FULLB0)//[7:0]
    #define REG_0124_MIIC0_REG_WBUF_19 Fld(8, 8, AC_FULLB1)//[15:8]
#define HWI2C_REG_0128_MIIC0 (0x0128)
    #define REG_0128_MIIC0_REG_WBUF_20 Fld(8, 0, AC_FULLB0)//[7:0]
    #define REG_0128_MIIC0_REG_WBUF_21 Fld(8, 8, AC_FULLB1)//[15:8]
#define HWI2C_REG_012C_MIIC0 (0x012C)
    #define REG_012C_MIIC0_REG_WBUF_22 Fld(8, 0, AC_FULLB0)//[7:0]
    #define REG_012C_MIIC0_REG_WBUF_23 Fld(8, 8, AC_FULLB1)//[15:8]
#define HWI2C_REG_0130_MIIC0 (0x0130)
    #define REG_0130_MIIC0_REG_WBUF_24 Fld(8, 0, AC_FULLB0)//[7:0]
    #define REG_0130_MIIC0_REG_WBUF_25 Fld(8, 8, AC_FULLB1)//[15:8]
#define HWI2C_REG_0134_MIIC0 (0x0134)
    #define REG_0134_MIIC0_REG_WBUF_26 Fld(8, 0, AC_FULLB0)//[7:0]
    #define REG_0134_MIIC0_REG_WBUF_27 Fld(8, 8, AC_FULLB1)//[15:8]
#define HWI2C_REG_0138_MIIC0 (0x0138)
    #define REG_0138_MIIC0_REG_WBUF_28 Fld(8, 0, AC_FULLB0)//[7:0]
    #define REG_0138_MIIC0_REG_WBUF_29 Fld(8, 8, AC_FULLB1)//[15:8]
#define HWI2C_REG_013C_MIIC0 (0x013C)
    #define REG_013C_MIIC0_REG_WBUF_30 Fld(8, 0, AC_FULLB0)//[7:0]
    #define REG_013C_MIIC0_REG_WBUF_31 Fld(8, 8, AC_FULLB1)//[15:8]
#define HWI2C_REG_0140_MIIC0 (0x0140)
    #define REG_0140_MIIC0_REG_WBUF_32 Fld(8, 0, AC_FULLB0)//[7:0]
    #define REG_0140_MIIC0_REG_WBUF_33 Fld(8, 8, AC_FULLB1)//[15:8]
#define HWI2C_REG_0144_MIIC0 (0x0144)
    #define REG_0144_MIIC0_REG_WBUF_34 Fld(8, 0, AC_FULLB0)//[7:0]
    #define REG_0144_MIIC0_REG_WBUF_35 Fld(8, 8, AC_FULLB1)//[15:8]
#define HWI2C_REG_0148_MIIC0 (0x0148)
    #define REG_0148_MIIC0_REG_WBUF_36 Fld(8, 0, AC_FULLB0)//[7:0]
    #define REG_0148_MIIC0_REG_WBUF_37 Fld(8, 8, AC_FULLB1)//[15:8]
#define HWI2C_REG_014C_MIIC0 (0x014C)
    #define REG_014C_MIIC0_REG_WBUF_38 Fld(8, 0, AC_FULLB0)//[7:0]
    #define REG_014C_MIIC0_REG_WBUF_39 Fld(8, 8, AC_FULLB1)//[15:8]
#define HWI2C_REG_0150_MIIC0 (0x0150)
    #define REG_0150_MIIC0_REG_WBUF_40 Fld(8, 0, AC_FULLB0)//[7:0]
    #define REG_0150_MIIC0_REG_WBUF_41 Fld(8, 8, AC_FULLB1)//[15:8]
#define HWI2C_REG_0154_MIIC0 (0x0154)
    #define REG_0154_MIIC0_REG_WBUF_42 Fld(8, 0, AC_FULLB0)//[7:0]
    #define REG_0154_MIIC0_REG_WBUF_43 Fld(8, 8, AC_FULLB1)//[15:8]
#define HWI2C_REG_0158_MIIC0 (0x0158)
    #define REG_0158_MIIC0_REG_WBUF_44 Fld(8, 0, AC_FULLB0)//[7:0]
    #define REG_0158_MIIC0_REG_WBUF_45 Fld(8, 8, AC_FULLB1)//[15:8]
#define HWI2C_REG_015C_MIIC0 (0x015C)
    #define REG_015C_MIIC0_REG_WBUF_46 Fld(8, 0, AC_FULLB0)//[7:0]
    #define REG_015C_MIIC0_REG_WBUF_47 Fld(8, 8, AC_FULLB1)//[15:8]
#define HWI2C_REG_0180_MIIC0 (0x0180)
    #define REG_0180_MIIC0_REG_RBUF_00 Fld(8, 0, AC_FULLB0)//[7:0]
    #define REG_0180_MIIC0_REG_RBUF_01 Fld(8, 8, AC_FULLB1)//[15:8]
#define HWI2C_REG_0184_MIIC0 (0x0184)
    #define REG_0184_MIIC0_REG_RBUF_02 Fld(8, 0, AC_FULLB0)//[7:0]
    #define REG_0184_MIIC0_REG_RBUF_03 Fld(8, 8, AC_FULLB1)//[15:8]
#define HWI2C_REG_0188_MIIC0 (0x0188)
    #define REG_0188_MIIC0_REG_RBUF_04 Fld(8, 0, AC_FULLB0)//[7:0]
    #define REG_0188_MIIC0_REG_RBUF_05 Fld(8, 8, AC_FULLB1)//[15:8]
#define HWI2C_REG_018C_MIIC0 (0x018C)
    #define REG_018C_MIIC0_REG_RBUF_06 Fld(8, 0, AC_FULLB0)//[7:0]
    #define REG_018C_MIIC0_REG_RBUF_07 Fld(8, 8, AC_FULLB1)//[15:8]
#define HWI2C_REG_0190_MIIC0 (0x0190)
    #define REG_0190_MIIC0_REG_RBUF_08 Fld(8, 0, AC_FULLB0)//[7:0]
    #define REG_0190_MIIC0_REG_RBUF_09 Fld(8, 8, AC_FULLB1)//[15:8]
#define HWI2C_REG_0194_MIIC0 (0x0194)
    #define REG_0194_MIIC0_REG_RBUF_10 Fld(8, 0, AC_FULLB0)//[7:0]
    #define REG_0194_MIIC0_REG_RBUF_11 Fld(8, 8, AC_FULLB1)//[15:8]
#define HWI2C_REG_0198_MIIC0 (0x0198)
    #define REG_0198_MIIC0_REG_RBUF_12 Fld(8, 0, AC_FULLB0)//[7:0]
    #define REG_0198_MIIC0_REG_RBUF_13 Fld(8, 8, AC_FULLB1)//[15:8]
#define HWI2C_REG_019C_MIIC0 (0x019C)
    #define REG_019C_MIIC0_REG_RBUF_14 Fld(8, 0, AC_FULLB0)//[7:0]
    #define REG_019C_MIIC0_REG_RBUF_15 Fld(8, 8, AC_FULLB1)//[15:8]
#define HWI2C_REG_01A0_MIIC0 (0x01A0)
    #define REG_01A0_MIIC0_REG_RBUF_16 Fld(8, 0, AC_FULLB0)//[7:0]
    #define REG_01A0_MIIC0_REG_RBUF_17 Fld(8, 8, AC_FULLB1)//[15:8]
#define HWI2C_REG_01A4_MIIC0 (0x01A4)
    #define REG_01A4_MIIC0_REG_RBUF_18 Fld(8, 0, AC_FULLB0)//[7:0]
    #define REG_01A4_MIIC0_REG_RBUF_19 Fld(8, 8, AC_FULLB1)//[15:8]
#define HWI2C_REG_01A8_MIIC0 (0x01A8)
    #define REG_01A8_MIIC0_REG_RBUF_20 Fld(8, 0, AC_FULLB0)//[7:0]
    #define REG_01A8_MIIC0_REG_RBUF_21 Fld(8, 8, AC_FULLB1)//[15:8]
#define HWI2C_REG_01AC_MIIC0 (0x01AC)
    #define REG_01AC_MIIC0_REG_RBUF_22 Fld(8, 0, AC_FULLB0)//[7:0]
    #define REG_01AC_MIIC0_REG_RBUF_23 Fld(8, 8, AC_FULLB1)//[15:8]
#define HWI2C_REG_01B0_MIIC0 (0x01B0)
    #define REG_01B0_MIIC0_REG_RBUF_24 Fld(8, 0, AC_FULLB0)//[7:0]
    #define REG_01B0_MIIC0_REG_RBUF_25 Fld(8, 8, AC_FULLB1)//[15:8]
#define HWI2C_REG_01B4_MIIC0 (0x01B4)
    #define REG_01B4_MIIC0_REG_RBUF_26 Fld(8, 0, AC_FULLB0)//[7:0]
    #define REG_01B4_MIIC0_REG_RBUF_27 Fld(8, 8, AC_FULLB1)//[15:8]
#define HWI2C_REG_01B8_MIIC0 (0x01B8)
    #define REG_01B8_MIIC0_REG_RBUF_28 Fld(8, 0, AC_FULLB0)//[7:0]
    #define REG_01B8_MIIC0_REG_RBUF_29 Fld(8, 8, AC_FULLB1)//[15:8]
#define HWI2C_REG_01BC_MIIC0 (0x01BC)
    #define REG_01BC_MIIC0_REG_RBUF_30 Fld(8, 0, AC_FULLB0)//[7:0]
    #define REG_01BC_MIIC0_REG_RBUF_31 Fld(8, 8, AC_FULLB1)//[15:8]
#define HWI2C_REG_01C0_MIIC0 (0x01C0)
    #define REG_01C0_MIIC0_REG_RBUF_32 Fld(8, 0, AC_FULLB0)//[7:0]
    #define REG_01C0_MIIC0_REG_RBUF_33 Fld(8, 8, AC_FULLB1)//[15:8]
#define HWI2C_REG_01C4_MIIC0 (0x01C4)
    #define REG_01C4_MIIC0_REG_RBUF_34 Fld(8, 0, AC_FULLB0)//[7:0]
    #define REG_01C4_MIIC0_REG_RBUF_35 Fld(8, 8, AC_FULLB1)//[15:8]
#define HWI2C_REG_01C8_MIIC0 (0x01C8)
    #define REG_01C8_MIIC0_REG_RBUF_36 Fld(8, 0, AC_FULLB0)//[7:0]
    #define REG_01C8_MIIC0_REG_RBUF_37 Fld(8, 8, AC_FULLB1)//[15:8]
#define HWI2C_REG_01CC_MIIC0 (0x01CC)
    #define REG_01CC_MIIC0_REG_RBUF_38 Fld(8, 0, AC_FULLB0)//[7:0]
    #define REG_01CC_MIIC0_REG_RBUF_39 Fld(8, 8, AC_FULLB1)//[15:8]
#define HWI2C_REG_01D0_MIIC0 (0x01D0)
    #define REG_01D0_MIIC0_REG_RBUF_40 Fld(8, 0, AC_FULLB0)//[7:0]
    #define REG_01D0_MIIC0_REG_RBUF_41 Fld(8, 8, AC_FULLB1)//[15:8]
#define HWI2C_REG_01D4_MIIC0 (0x01D4)
    #define REG_01D4_MIIC0_REG_RBUF_42 Fld(8, 0, AC_FULLB0)//[7:0]
    #define REG_01D4_MIIC0_REG_RBUF_43 Fld(8, 8, AC_FULLB1)//[15:8]
#define HWI2C_REG_01D8_MIIC0 (0x01D8)
    #define REG_01D8_MIIC0_REG_RBUF_44 Fld(8, 0, AC_FULLB0)//[7:0]
    #define REG_01D8_MIIC0_REG_RBUF_45 Fld(8, 8, AC_FULLB1)//[15:8]
#define HWI2C_REG_01DC_MIIC0 (0x01DC)
    #define REG_01DC_MIIC0_REG_RBUF_46 Fld(8, 0, AC_FULLB0)//[7:0]
    #define REG_01DC_MIIC0_REG_RBUF_47 Fld(8, 8, AC_FULLB1)//[15:8]

#endif // _I2C_MT5896_CODA_H_

