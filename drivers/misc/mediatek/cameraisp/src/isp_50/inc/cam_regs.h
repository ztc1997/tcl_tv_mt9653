/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2015 MediaTek Inc.
 */

/******************************************************************************
 * cam_regs.h - MT6758 cam registers
 *
 * DESCRIPTION:
 *     This file provide register addresses and chip dependency infos in CAMSYS.
 *
 ******************************************************************************/

/**
 *    CAM interrupt status
 */
/* normal siganl */
#define VS_INT_ST                 (1L<<0)
#define TG_INT1_ST               (1L<<1)
#define TG_INT2_ST               (1L<<2)
#define EXPDON_ST                (1L<<3)
#define HW_PASS1_DON_ST (1L<<11)
#define SOF_INT_ST              (1L<<12)
#define SW_PASS1_DON_ST (1L<<30)
/* err status */
#define TG_ERR_ST              (1L<<4)
#define TG_GBERR_ST         (1L<<5)
#define CQ_CODE_ERR_ST   (1L<<6)
#define CQ_APB_ERR_ST     (1L<<7)
#define CQ_VS_ERR_ST       (1L<<8)
#define AMX_ERR_ST          (1L<<15)
#define RMX_ERR_ST          (1L<<16)
#define BMX_ERR_ST          (1L<<17)
#define RRZO_ERR_ST         (1L<<18)
#define AFO_ERR_ST          (1L<<19)
#define IMGO_ERR_ST         (1L<<20)
#define AAO_ERR_ST          (1L<<21)
#define PSO_ERR_ST          (1L<<22)
#define LCSO_ERR_ST         (1L<<23)
#define BNR_ERR_ST          (1L<<24)
#define LSC_ERR_ST           (1L<<25)
#define UFGO_ERR_ST        (1L<<26)
#define UFEO_ERR_ST        (1L<<27)
#define PDO_ERR_ST          (1L<<28)
#define DMA_ERR_ST          (1L<<29)
/* err status_2 */
#define FLKO_ERR_ST          (1L<<16)
#define LMVO_ERR_ST          (1L<<17)
#define RSSO_ERR_ST          (1L<<18)

/**
 *    CAM DMA done status
 */
#define IMGO_DONE_ST        (1L<<0)
#define UFEO_DONE_ST        (1L<<1)
#define RRZO_DONE_ST        (1L<<2)
#define EISO_DONE_ST        (1L<<3)
#define FLKO_DONE_ST        (1L<<4)
#define AFO_DONE_ST         (1L<<5)
#define LCSO_DONE_ST        (1L<<6)
#define AAO_DONE_ST         (1L<<7)
#define BPCI_DONE_ST        (1L<<9)
#define LSCI_DONE_ST        (1L<<10)
#define UFGO_DONE_ST        (1L<<11)
#define AF_TAR_DONE_ST    (1L<<12)
#define PDO_DONE_ST         (1L<<13)
#define PSO_DONE_ST         (1L<<14)
#define RSSO_DONE_ST        (1L<<15)
/**
 *    CAMSV interrupt status
 */
/* normal signal */
#define SV_VS1_ST           (1L<<0)
#define SV_TG_ST1           (1L<<1)
#define SV_TG_ST2           (1L<<2)
#define SV_EXPDON_ST        (1L<<3)
#define SV_SOF_INT_ST       (1L<<7)
#define SV_HW_PASS1_DON_ST  (1L<<10)
#define SV_SW_PASS1_DON_ST  (1L<<20)
/* err status */
#define SV_TG_ERR           (1L<<4)
#define SV_TG_GBERR         (1L<<5)
#define SV_IMGO_ERR         (1L<<16)
#define SV_IMGO_OVERRUN     (1L<<17)

/**
 *    IRQ signal mask
 */
#define INT_ST_MASK_CAM     ( \
			      VS_INT_ST |\
			      TG_INT1_ST |\
			      TG_INT2_ST |\
			      EXPDON_ST |\
			      HW_PASS1_DON_ST |\
			      SOF_INT_ST |\
			      SW_PASS1_DON_ST)
/**
 *    dma done mask
 */
#define DMA_ST_MASK_CAM     (\
			     IMGO_DONE_ST |\
			     UFEO_DONE_ST |\
			     RRZO_DONE_ST |\
			     EISO_DONE_ST |\
			     FLKO_DONE_ST |\
			     AFO_DONE_ST |\
			     LCSO_DONE_ST |\
			     AAO_DONE_ST |\
			     BPCI_DONE_ST |\
			     LSCI_DONE_ST |\
			     PDO_DONE_ST | \
			     PSO_DONE_ST)

/**
 *    IRQ Warning Mask
 */
#define INT_ST_MASK_CAM_WARN    (\
				 PDO_ERR_ST |\
				 UFGO_ERR_ST |\
				 UFEO_ERR_ST |\
				 RRZO_ERR_ST |\
				 AFO_ERR_ST |\
				 IMGO_ERR_ST |\
				 AAO_ERR_ST |\
				 PSO_ERR_ST | \
				 LCSO_ERR_ST |\
				 BNR_ERR_ST |\
				 LSC_ERR_ST)

#define INT_ST_MASK_CAM_WARN_2    (\
				 FLKO_ERR_ST |\
				 RSSO_ERR_ST |\
				 LMVO_ERR_ST)

/**
 *    IRQ Error Mask
 */
#define INT_ST_MASK_CAM_ERR     (\
				 TG_ERR_ST |\
				 TG_GBERR_ST |\
				 CQ_CODE_ERR_ST |\
				 CQ_APB_ERR_ST |\
				 CQ_VS_ERR_ST |\
				 AMX_ERR_ST |\
				 RMX_ERR_ST |\
				 BMX_ERR_ST |\
				 BNR_ERR_ST |\
				 LSC_ERR_ST |\
				 DMA_ERR_ST)


/**
 *    IRQ signal mask
 */
#define INT_ST_MASK_CAMSV       (\
				 SV_VS1_ST |\
				 SV_TG_ST1 |\
				 SV_TG_ST2 |\
				 SV_EXPDON_ST |\
				 SV_SOF_INT_ST |\
				 SV_HW_PASS1_DON_ST |\
				 SV_SW_PASS1_DON_ST)
/**
 *    IRQ Error Mask
 */
#define INT_ST_MASK_CAMSV_ERR   (\
				 SV_TG_ERR |\
				 SV_TG_GBERR |\
				 SV_IMGO_ERR |\
				 SV_IMGO_OVERRUN)


#define CAMSYS_REG_CG_CON               (ISP_CAMSYS_CONFIG_BASE + 0x0)
#define CAMSYS_REG_CG_SET               (ISP_CAMSYS_CONFIG_BASE + 0x4)
#define CAMSYS_REG_CG_CLR               (ISP_CAMSYS_CONFIG_BASE + 0x8)

#define CAM_REG_CTL_EN(module)  (isp_devs[module].regs + 0x0004)
#define CAM_REG_CTL_DMA_EN(module)  (isp_devs[module].regs + 0x0008)
#define CAM_REG_CTL_FMT_SEL(module)  (isp_devs[module].regs + 0x000C)
#define CAM_REG_CTL_SEL(module)  (isp_devs[module].regs + 0x0010)
#define CAM_REG_CTL_EN2(module)  (isp_devs[module].regs + 0x0018)
#define CAM_REG_CTL_RAW_INT_EN(module)  (isp_devs[module].regs + 0x0020)
#define CAM_REG_CTL_RAW_INT_STATUS(module)  (isp_devs[module].regs + 0x0024)
#define CAM_REG_CTL_RAW_INT_STATUSX(module)  (isp_devs[module].regs + 0x0028)
#define CAM_REG_CTL_RAW_INT2_EN(module)  (isp_devs[module].regs + 0x0030)
#define CAM_REG_CTL_RAW_INT2_STATUS(module)  (isp_devs[module].regs + 0x0034)
#define CAM_REG_CTL_RAW_INT2_STATUSX(module)  (isp_devs[module].regs + 0x0038)
#define CAM_REG_CTL_RAW_INT3_EN(module)  (isp_devs[module].regs + 0x00C0)
#define CAM_REG_CTL_RAW_INT3_STATUS(module)  (isp_devs[module].regs + 0x00C4)
#define CAM_REG_CTL_RAW_INT3_STATUSX(module)  (isp_devs[module].regs + 0x00C8)
#define CAM_REG_CTL_SW_CTL(module)  (isp_devs[module].regs + 0x0040)
#define CAM_REG_CTL_CD_DONE_SEL(module)  (isp_devs[module].regs + 0x0048)
#define CAM_REG_CTL_UNI_DONE_SEL(module)  (isp_devs[module].regs + 0x004C)
#define CAM_REG_CTL_TWIN_STATUS(module)  (isp_devs[module].regs + 0x0050)

#define CAM_REG_FBC_IMGO_CTL1(module)  (isp_devs[module].regs + 0x0110)
#define CAM_REG_FBC_IMGO_CTL2(module)  (isp_devs[module].regs + 0x0114)
#define CAM_REG_FBC_RRZO_CTL1(module)  (isp_devs[module].regs + 0x0118)
#define CAM_REG_FBC_RRZO_CTL2(module)  (isp_devs[module].regs + 0x011C)
#define CAM_REG_FBC_UFEO_CTL1(module)  (isp_devs[module].regs + 0x0120)
#define CAM_REG_FBC_UFEO_CTL2(module)  (isp_devs[module].regs + 0x0124)
#define CAM_REG_FBC_LCSO_CTL1(module)  (isp_devs[module].regs + 0x0128)
#define CAM_REG_FBC_LCSO_CTL2(module)  (isp_devs[module].regs + 0x012C)
#define CAM_REG_FBC_LMVO_CTL1(module)  (isp_devs[module].regs + 0x0158)
#define CAM_REG_FBC_LMVO_CTL2(module)  (isp_devs[module].regs + 0x015C)
#define CAM_REG_FBC_RSSO_CTL1(module)  (isp_devs[module].regs + 0x0160)
#define CAM_REG_FBC_RSSO_CTL2(module)  (isp_devs[module].regs + 0x0164)
#define CAM_REG_FBC_UFGO_CTL1(module)  (isp_devs[module].regs + 0x0168)
#define CAM_REG_FBC_UFGO_CTL2(module)  (isp_devs[module].regs + 0x016C)

#define CAM_REG_CQ_THR0_BASEADDR(module)  (isp_devs[module].regs + 0x0198)

#define CAM_REG_TG_SEN_MODE(module)  (isp_devs[module].regs + 0x0230)
#define CAM_REG_TG_VF_CON(module)  (isp_devs[module].regs + 0x0234)
#define CAM_REG_TG_INTER_ST(module)  (isp_devs[module].regs + 0x026C)
#define CAM_REG_TG_SUB_PERIOD(module)  (isp_devs[module].regs + 0x02A4)

#define CAM_REG_RRZ_IN_IMG(module)  (isp_devs[module].regs + 0x04E4)
#define CAM_REG_RRZ_OUT_IMG(module)  (isp_devs[module].regs + 0x04E8)

#define CAM_REG_IMGO_BASE_ADDR(module)  (isp_devs[module].regs + 0x1020)
#define CAM_REG_IMGO_XSIZE(module)  (isp_devs[module].regs + 0x1030)
#define CAM_REG_IMGO_YSIZE(module)  (isp_devs[module].regs + 0x1034)

#define CAM_REG_IMGO_CON(module)  (isp_devs[module].regs + 0x103C)
#define CAM_REG_IMGO_CON2(module)  (isp_devs[module].regs + 0x1040)
#define CAM_REG_IMGO_CON3(module)  (isp_devs[module].regs + 0x1044)

#define CAM_REG_RRZO_BASE_ADDR(module)  (isp_devs[module].regs + 0x1050)
#define CAM_REG_RRZO_XSIZE(module)  (isp_devs[module].regs + 0x1060)
#define CAM_REG_RRZO_YSIZE(module)  (isp_devs[module].regs + 0x1064)

#define CAM_REG_RRZO_CON(module)  (isp_devs[module].regs + 0x106C)
#define CAM_REG_RRZO_CON2(module)  (isp_devs[module].regs + 0x1070)
#define CAM_REG_RRZO_CON3(module)  (isp_devs[module].regs + 0x1074)

#define CAM_REG_AAO_CON(module)  (isp_devs[module].regs + 0x109C)
#define CAM_REG_AAO_CON2(module)  (isp_devs[module].regs + 0x10A0)
#define CAM_REG_AAO_CON3(module)  (isp_devs[module].regs + 0x10A4)

#define CAM_REG_AFO_CON(module)  (isp_devs[module].regs + 0x10CC)
#define CAM_REG_AFO_CON2(module)  (isp_devs[module].regs + 0x10D0)
#define CAM_REG_AFO_CON3(module)  (isp_devs[module].regs + 0x10D4)

#define CAM_REG_LCSO_CON(module)  (isp_devs[module].regs + 0x10FC)
#define CAM_REG_LCSO_CON2(module)  (isp_devs[module].regs + 0x1100)
#define CAM_REG_LCSO_CON3(module)  (isp_devs[module].regs + 0x1104)

#define CAM_REG_UFEO_CON(module)  (isp_devs[module].regs + 0x112C)
#define CAM_REG_UFEO_CON2(module)  (isp_devs[module].regs + 0x1130)
#define CAM_REG_UFEO_CON3(module)  (isp_devs[module].regs + 0x1134)

#define CAM_REG_PDO_CON(module)  (isp_devs[module].regs + 0x115C)
#define CAM_REG_PDO_CON2(module)  (isp_devs[module].regs + 0x1160)
#define CAM_REG_PDO_CON3(module)  (isp_devs[module].regs + 0x1164)

#define CAM_REG_BPCI_CON(module)  (isp_devs[module].regs + 0x118C)
#define CAM_REG_BPCI_CON2(module)  (isp_devs[module].regs + 0x1190)
#define CAM_REG_BPCI_CON3(module)  (isp_devs[module].regs + 0x1194)

#define CAM_REG_LSCI_CON(module)  (isp_devs[module].regs + 0x11EC)
#define CAM_REG_LSCI_CON2(module)  (isp_devs[module].regs + 0x11F0)
#define CAM_REG_LSCI_CON3(module)  (isp_devs[module].regs + 0x11F4)

#define CAM_REG_PDI_CON(module)  (isp_devs[module].regs + 0x124C)
#define CAM_REG_PDI_CON2(module)  (isp_devs[module].regs + 0x1250)
#define CAM_REG_PDI_CON3(module)  (isp_devs[module].regs + 0x1254)

#define CAM_REG_PSO_CON(module)  (isp_devs[module].regs + 0x127C)
#define CAM_REG_PSO_CON2(module)  (isp_devs[module].regs + 0x1280)
#define CAM_REG_PSO_CON3(module)  (isp_devs[module].regs + 0x1284)

#define CAM_REG_LMVO_CON(module)  (isp_devs[module].regs + 0x12AC)
#define CAM_REG_LMVO_CON2(module)  (isp_devs[module].regs + 0x12B0)
#define CAM_REG_LMVO_CON3(module)  (isp_devs[module].regs + 0x12B4)

#define CAM_REG_FLKO_CON(module)  (isp_devs[module].regs + 0x12DC)
#define CAM_REG_FLKO_CON2(module)  (isp_devs[module].regs + 0x12E0)
#define CAM_REG_FLKO_CON3(module)  (isp_devs[module].regs + 0x12E4)

#define CAM_REG_RSSO_CON(module)  (isp_devs[module].regs + 0x130C)
#define CAM_REG_RSSO_CON2(module)  (isp_devs[module].regs + 0x1310)
#define CAM_REG_RSSO_CON3(module)  (isp_devs[module].regs + 0x1314)

#define CAM_REG_UFGO_CON(module)  (isp_devs[module].regs + 0x133C)
#define CAM_REG_UFGO_CON2(module)  (isp_devs[module].regs + 0x1340)
#define CAM_REG_UFGO_CON3(module)  (isp_devs[module].regs + 0x1344)

#define CAM_REG_IMGO_ERR_STAT(module)  (isp_devs[module].regs + 0x1360)
#define CAM_REG_RRZO_ERR_STAT(module)  (isp_devs[module].regs + 0x1364)
#define CAM_REG_AAO_ERR_STAT(module)  (isp_devs[module].regs + 0x1368)
#define CAM_REG_AFO_ERR_STAT(module)  (isp_devs[module].regs + 0x136C)
#define CAM_REG_LCSO_ERR_STAT(module)  (isp_devs[module].regs + 0x1370)
#define CAM_REG_UFEO_ERR_STAT(module)  (isp_devs[module].regs + 0x1374)
#define CAM_REG_PDO_ERR_STAT(module)  (isp_devs[module].regs + 0x1378)
#define CAM_REG_BPCI_ERR_STAT(module)  (isp_devs[module].regs + 0x137C)
#define CAM_REG_LSCI_ERR_STAT(module)  (isp_devs[module].regs + 0x1384)
#define CAM_REG_PDI_ERR_STAT(module)  (isp_devs[module].regs + 0x138C)
#define CAM_REG_LMVO_ERR_STAT(module)  (isp_devs[module].regs + 0x1390)
#define CAM_REG_FLKO_ERR_STAT(module)  (isp_devs[module].regs + 0x1394)
#define CAM_REG_RSSO_A_ERR_STAT(module)  (isp_devs[module].regs + 0x1398)
#define CAM_REG_UFGO_ERR_STAT(module)  (isp_devs[module].regs + 0x139C)
#define CAM_REG_PSO_ERR_STAT(module)  (isp_devs[module].regs + 0x13A0)
#define CAM_REG_DMA_DEBUG_SEL(module)  (isp_devs[module].regs + 0x13C8)

#define CAM_REG_IMGO_FH_SPARE_2(module)  (isp_devs[module].regs + 0x1434)
#define CAM_REG_RRZO_FH_SPARE_2(module)  (isp_devs[module].regs + 0x1474)

#define CAM_REG_RRZ_HORI_STEP(module)  (isp_devs[module].regs + 0x14EC)
#define CAM_REG_RRZ_VERT_STEP(module)  (isp_devs[module].regs + 0x14F0)

/* CAM_UNI */
#define CAM_UNI_REG_TOP_SW_CTL(module)  (isp_devs[module].regs + 0x0008)
#define CAM_UNI_REG_TOP_DMA_EN(module)  (isp_devs[module].regs + 0x0014)
#define CAM_UNI_REG_RAWI_ERR_STAT(module)  (isp_devs[module].regs + 0x0154)

#define CAM_UNI_REG_RAWI_CON(module)  (isp_devs[module].regs + 0x013C)
#define CAM_UNI_REG_RAWI_CON2(module)  (isp_devs[module].regs + 0x0140)
#define CAM_UNI_REG_RAWI_CON3(module)  (isp_devs[module].regs + 0x0144)

#define CAM_UNI_REG_TOP_DBG_SET(module)  (isp_devs[module].regs + 0x002C)
#define CAM_UNI_REG_TOP_DBG_PORT(module)  (isp_devs[module].regs + 0x0030)

/* CAMSV */
#define CAMSV_REG_MODULE_EN(module)  (isp_devs[module].regs + 0x0510)
#define CAMSV_REG_INT_EN(module)    (isp_devs[module].regs + 0x0518)
#define CAMSV_REG_INT_STATUS(module)  (isp_devs[module].regs + 0x051C)
#define CAMSV_REG_SW_CTL(module)  (isp_devs[module].regs + 0x0520)
#define CAMSV_REG_FBC_IMGO_CTL1(module)  (isp_devs[module].regs + 0x0110)
#define CAMSV_REG_FBC_IMGO_CTL2(module)  (isp_devs[module].regs + 0x0114)
#define CAMSV_REG_IMGO_BASE_ADDR(module)  (isp_devs[module].regs + 0x0020)
#define CAMSV_REG_TG_SEN_MODE(module)  (isp_devs[module].regs + 0x0230)
#define CAMSV_REG_TG_VF_CON(module)  (isp_devs[module].regs + 0x0234)
#define CAMSV_REG_TG_INTER_ST(module)  (isp_devs[module].regs + 0x026C)
#define CAMSV_REG_TG_TIME_STAMP(module)  (isp_devs[module].regs + 0x02A0)

union FBC_CTRL_1 {
	struct { /* 0x18004110 */
		unsigned int  FBC_NUM       :  6;      /*  0.. 5, 0x0000003F */
		unsigned int  rsv_6         :  9;      /*  6..14, 0x00007FC0 */
		unsigned int  FBC_EN        :  1;      /* 15..15, 0x00008000 */
		unsigned int  FBC_MODE      :  1;      /* 16..16, 0x00010000 */
		unsigned int  LOCK_EN       :  1;      /* 17..17, 0x00020000 */
		unsigned int  rsv_18        :  2;      /* 18..19, 0x000C0000 */
		unsigned int  DROP_TIMING   :  1;      /* 20..20, 0x00100000 */
		unsigned int  rsv_21        :  3;      /* 21..23, 0x00E00000 */
		unsigned int  SUB_RATIO     :  8;      /* 24..31, 0xFF000000 */
	} Bits;
	unsigned int Raw;
};  /* CAM_A_FBC_IMGO_CTL1 */

union FBC_CTRL_2 {
	struct { /* 0x18004114 */
		unsigned int  FBC_CNT       :  7;      /*  0.. 6, 0x0000007F */
		unsigned int  rsv_7         :  1;      /*  7.. 7, 0x00000080 */
		unsigned int  RCNT          :  6;      /*  8..13, 0x00003F00 */
		unsigned int  rsv_14        :  2;      /* 14..15, 0x0000C000 */
		unsigned int  WCNT          :  6;      /* 16..21, 0x003F0000 */
		unsigned int  rsv_22        :  2;      /* 22..23, 0x00C00000 */
		unsigned int  DROP_CNT      :  8;      /* 24..31, 0xFF000000 */
	} Bits;
	unsigned int Raw;
};  /* CAM_A_FBC_IMGO_CTL2 */

