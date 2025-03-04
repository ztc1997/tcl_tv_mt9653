/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2019 MediaTek Inc.
 */

#ifndef _UFS_MEDIATEK_H
#define _UFS_MEDIATEK_H

#include <linux/bitops.h>
#include <linux/soc/mediatek/mtk_sip_svc.h>

/*
 * Vendor specific UFSHCI Registers
 */
#define REG_UFS_REFCLK_CTRL         0x144
#define REG_UFS_EXTREG              0x2100
#define REG_UFS_MPHYCTRL            0x2200
#define REG_UFS_REJECT_MON          0x22AC
#define REG_UFS_DEBUG_SEL           0x22C0
#define REG_UFS_PROBE               0x22C8

/*
 * Ref-clk control
 *
 * Values for register REG_UFS_REFCLK_CTRL
 */
#define REFCLK_RELEASE              0x0
#define REFCLK_REQUEST              BIT(0)
#define REFCLK_ACK                  BIT(1)

#define REFCLK_REQ_TIMEOUT_US       3000

/*
 * Vendor specific pre-defined parameters
 */
#define UFS_MTK_LIMIT_NUM_LANES_RX  2
#define UFS_MTK_LIMIT_NUM_LANES_TX  2
#define UFS_MTK_LIMIT_HSGEAR_RX     UFS_HS_G4
#define UFS_MTK_LIMIT_HSGEAR_TX     UFS_HS_G4
#define UFS_MTK_LIMIT_PWMGEAR_RX    UFS_PWM_G4
#define UFS_MTK_LIMIT_PWMGEAR_TX    UFS_PWM_G4
#define UFS_MTK_LIMIT_RX_PWR_PWM    SLOW_MODE
#define UFS_MTK_LIMIT_TX_PWR_PWM    SLOW_MODE
#define UFS_MTK_LIMIT_RX_PWR_HS     FAST_MODE
#define UFS_MTK_LIMIT_TX_PWR_HS     FAST_MODE
#define UFS_MTK_LIMIT_HS_RATE       PA_HS_MODE_B
#define UFS_MTK_LIMIT_DESIRED_MODE  UFS_HS_MODE

/*
 * Other attributes
 */
#define VS_DEBUGCLOCKENABLE         0xD0A1
#define VS_SAVEPOWERCONTROL         0xD0A6
#define VS_UNIPROPOWERDOWNCONTROL   0xD0A8

/*
 * Vendor specific link state
 */
enum {
	VS_LINK_DISABLED            = 0,
	VS_LINK_DOWN                = 1,
	VS_LINK_UP                  = 2,
	VS_LINK_HIBERN8             = 3,
	VS_LINK_LOST                = 4,
	VS_LINK_CFG                 = 5,
};

/*
 * SiP commands
 */
#define MTK_SIP_UFS_CONTROL               MTK_SIP_SMC_CMD(0x276)
#define UFS_MTK_SIP_VA09_PWR_CTRL         BIT(0)
#define UFS_MTK_SIP_DEVICE_RESET          BIT(1)
#define UFS_MTK_SIP_CRYPTO_CTRL           BIT(2)
#define UFS_MTK_SIP_REF_CLK_NOTIFICATION  BIT(3)

/*
 * VS_DEBUGCLOCKENABLE
 */
enum {
	TX_SYMBOL_CLK_REQ_FORCE = 5,
};

/*
 * VS_SAVEPOWERCONTROL
 */
enum {
	RX_SYMBOL_CLK_GATE_EN   = 0,
	SYS_CLK_GATE_EN         = 2,
	TX_CLK_GATE_EN          = 3,
};

/*
 * Host capability
 */
enum ufs_mtk_host_caps {
	UFS_MTK_CAP_BOOST_CRYPT_ENGINE         = 1 << 0,
	UFS_MTK_CAP_VA09_PWR_CTRL              = 1 << 1,
	UFS_MTK_CAP_DISABLE_AH8                = 1 << 2,
	UFS_MTK_CAP_PROPRIETARY_CRYPTO         = 1 << 3,
	UFS_MTK_CAP_SUSPEND_SHUNDOWN           = 1 << 4,
};

struct ufs_mtk_crypt_cfg {
	struct regulator *reg_vcore;
	struct clk *clk_crypt_perf;
	struct clk *clk_crypt_mux;
	struct clk *clk_crypt_lp;
	int vcore_volt;
};

enum {
	UFS_MTK_HW_VER_2 = 2,
	UFS_MTK_HW_VER_3  = 3,
};

struct ufs_mtk_hw_ver {
	u8 step;
	u8 minor;
	u8 major;
};

struct ufs_mtk_host {
	struct phy *mphy;
	struct regulator *reg_va09;
	struct reset_control *hci_reset;
	struct reset_control *unipro_reset;
	struct reset_control *crypto_reset;
	struct ufs_hba *hba;
	struct ufs_mtk_crypt_cfg *crypt;
	struct list_head crypto_disable_list_head;
	struct ufs_mtk_hw_ver hw_ver;
	enum ufs_mtk_host_caps caps;
	bool mphy_powered_on;
	bool unipro_lpm;
	bool ref_clk_enabled;
	u16 ref_clk_ungating_wait_us;
	u16 ref_clk_gating_wait_us;
};

/**
 * struct ufs_crypto_disable_info - UFS crypto disable info
 * @list: list headed by host->crypto_disable_list_head
 * @lba: logical block address
 * @length: transfer length in block
 */
struct ufs_crypto_disable_info {
	struct list_head list;
	u64 lba;
	u64 length;
};

#endif /* !_UFS_MEDIATEK_H */
