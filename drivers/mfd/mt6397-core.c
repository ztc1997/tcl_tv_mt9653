// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2014 MediaTek Inc.
 * Author: Flora Fu, MediaTek
 */

#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/of_irq.h>
#include <linux/regmap.h>
#include <linux/mfd/core.h>
#include <linux/mfd/mt6323/core.h>
#include <linux/mfd/mt6358/core.h>
#include <linux/mfd/mt6359p/core.h>
#include <linux/mfd/mt6397/core.h>
#include <linux/mfd/mt6323/registers.h>
#include <linux/mfd/mt6358/registers.h>
#include <linux/mfd/mt6359p/registers.h>
#include <linux/mfd/mt6397/registers.h>

#define MT6323_RTC_BASE		0x8000
#define MT6323_RTC_SIZE		0x40

#define MT6358_RTC_BASE		0x0588
#define MT6358_RTC_SIZE		0x3c

#define MT6397_RTC_BASE		0xe000
#define MT6397_RTC_SIZE		0x3e

#define MT6323_PWRC_BASE	0x8000
#define MT6323_PWRC_SIZE	0x40

static const struct resource mt6323_rtc_resources[] = {
	DEFINE_RES_MEM(MT6323_RTC_BASE, MT6323_RTC_SIZE),
	DEFINE_RES_IRQ(MT6323_IRQ_STATUS_RTC),
};

static const struct resource mt6358_rtc_resources[] = {
	DEFINE_RES_MEM(MT6358_RTC_BASE, MT6358_RTC_SIZE),
	DEFINE_RES_IRQ(MT6358_IRQ_RTC),
};

static const struct resource mt6359p_rtc_resources[] = {
	DEFINE_RES_MEM(MT6358_RTC_BASE, MT6358_RTC_SIZE),
	DEFINE_RES_IRQ(MT6359P_IRQ_RTC),
};

static const struct resource mt6397_rtc_resources[] = {
	DEFINE_RES_MEM(MT6397_RTC_BASE, MT6397_RTC_SIZE),
	DEFINE_RES_IRQ(MT6397_IRQ_RTC),
};

static const struct resource mt6323_keys_resources[] = {
	DEFINE_RES_IRQ(MT6323_IRQ_STATUS_PWRKEY),
	DEFINE_RES_IRQ(MT6323_IRQ_STATUS_FCHRKEY),
};

static const struct resource mt6359p_keys_resources[] = {
	DEFINE_RES_IRQ(MT6359P_IRQ_PWRKEY),
	DEFINE_RES_IRQ(MT6359P_IRQ_HOMEKEY),
	DEFINE_RES_IRQ(MT6359P_IRQ_PWRKEY_R),
	DEFINE_RES_IRQ(MT6359P_IRQ_HOMEKEY_R),
};
static const struct resource mt6397_keys_resources[] = {
	DEFINE_RES_IRQ(MT6397_IRQ_PWRKEY),
	DEFINE_RES_IRQ(MT6397_IRQ_HOMEKEY),
};

static const struct resource mt6359p_auxadc_resources[] = {
	DEFINE_RES_IRQ_NAMED(MT6359P_IRQ_AUXADC_IMP, "imp"),
};

static const struct resource mt6359p_battery_oc_resources[] = {
	DEFINE_RES_IRQ_NAMED(MT6359P_IRQ_FG_CUR_H, "fg_cur_h"),
	DEFINE_RES_IRQ_NAMED(MT6359P_IRQ_FG_CUR_L, "fg_cur_l"),
};

static const struct resource mt6359p_lbat_service_resources[] = {
	DEFINE_RES_IRQ_NAMED(MT6359P_IRQ_BAT_H, "bat_h"),
	DEFINE_RES_IRQ_NAMED(MT6359P_IRQ_BAT_L, "bat_l"),
};

static const struct resource mt6323_pwrc_resources[] = {
	DEFINE_RES_MEM(MT6323_PWRC_BASE, MT6323_PWRC_SIZE),
};

static const struct resource mt6359p_gauge_resources[] = {
	DEFINE_RES_IRQ_NAMED(MT6359P_IRQ_FG_BAT_H, "COULOMB_H"),
	DEFINE_RES_IRQ_NAMED(MT6359P_IRQ_FG_BAT_L, "COULOMB_L"),
	DEFINE_RES_IRQ_NAMED(MT6359P_IRQ_BAT2_H, "VBAT_H"),
	DEFINE_RES_IRQ_NAMED(MT6359P_IRQ_BAT2_L, "VBAT_L"),
	DEFINE_RES_IRQ_NAMED(MT6359P_IRQ_NAG_C_DLTV, "NAFG"),
	DEFINE_RES_IRQ_NAMED(MT6359P_IRQ_BATON_BAT_OU, "BAT_OUT"),
	DEFINE_RES_IRQ_NAMED(MT6359P_IRQ_FG_ZCV, "ZCV"),
	DEFINE_RES_IRQ_NAMED(MT6359P_IRQ_FG_N_CHARGE_L, "FG_N_CHARGE_L"),
	DEFINE_RES_IRQ_NAMED(MT6359P_IRQ_FG_IAVG_H, "FG_IAVG_H"),
	DEFINE_RES_IRQ_NAMED(MT6359P_IRQ_FG_IAVG_L, "FG_IAVG_L"),
	DEFINE_RES_IRQ_NAMED(MT6359P_IRQ_BAT_TEMP_H, "BAT_TMP_H"),
	DEFINE_RES_IRQ_NAMED(MT6359P_IRQ_BAT_TEMP_L, "BAT_TMP_L"),
};

static const struct resource mt6359p_accdet_resources[] = {
	DEFINE_RES_IRQ_NAMED(MT6359P_IRQ_ACCDET, "ACCDET_IRQ"),
	DEFINE_RES_IRQ_NAMED(MT6359P_IRQ_ACCDET_EINT0, "ACCDET_EINT0"),
	DEFINE_RES_IRQ_NAMED(MT6359P_IRQ_ACCDET_EINT1, "ACCDET_EINT1"),
};

static const struct mfd_cell mt6323_devs[] = {
	{
		.name = "mt6323-rtc",
		.num_resources = ARRAY_SIZE(mt6323_rtc_resources),
		.resources = mt6323_rtc_resources,
		.of_compatible = "mediatek,mt6323-rtc",
	}, {
		.name = "mt6323-regulator",
		.of_compatible = "mediatek,mt6323-regulator"
	}, {
		.name = "mt6323-led",
		.of_compatible = "mediatek,mt6323-led"
	}, {
		.name = "mtk-pmic-keys",
		.num_resources = ARRAY_SIZE(mt6323_keys_resources),
		.resources = mt6323_keys_resources,
		.of_compatible = "mediatek,mt6323-keys"
	}, {
		.name = "mt6323-pwrc",
		.num_resources = ARRAY_SIZE(mt6323_pwrc_resources),
		.resources = mt6323_pwrc_resources,
		.of_compatible = "mediatek,mt6323-pwrc"
	},
};

static const struct mfd_cell mt6358_devs[] = {
	{
		.name = "mt6358-regulator",
		.of_compatible = "mediatek,mt6358-regulator"
	}, {
		.name = "mt6358-rtc",
		.num_resources = ARRAY_SIZE(mt6358_rtc_resources),
		.resources = mt6358_rtc_resources,
		.of_compatible = "mediatek,mt6358-rtc",
	}, {
		.name = "mt6358-sound",
		.of_compatible = "mediatek,mt6358-sound"
	},
};

static const struct mfd_cell mt6359p_devs[] = {
	{
		.name = "mt-pmic",
		.of_compatible = "mediatek,mt63xx-debug",
	}, {
		.name = "mt6359p-accdet",
		.of_compatible = "mediatek,mt6359p-accdet",
		.num_resources = ARRAY_SIZE(mt6359p_accdet_resources),
		.resources = mt6359p_accdet_resources,
	}, {
		.name = "mt635x-auxadc",
		.of_compatible = "mediatek,mt6359p-auxadc",
		.num_resources = ARRAY_SIZE(mt6359p_auxadc_resources),
		.resources = mt6359p_auxadc_resources,
	}, {
		.name = "mt6359p-efuse",
		.of_compatible = "mediatek,mt6359p-efuse",
	}, {
		.name = "mt6359p-regulator",
		.of_compatible = "mediatek,mt6359p-regulator"
	}, {
		.name = "mt6359p-rtc",
		.num_resources = ARRAY_SIZE(mt6359p_rtc_resources),
		.resources = mt6359p_rtc_resources,
		.of_compatible = "mediatek,mt6359p-rtc",
	}, {
		.name = "mt6359p-gauge",
		.num_resources = ARRAY_SIZE(mt6359p_gauge_resources),
		.resources = mt6359p_gauge_resources,
		.of_compatible = "mediatek,mt6359p-gauge",
	}, {
		.name = "mtk-battery-oc-throttling",
		.of_compatible = "mediatek,mt6359p-battery_oc_throttling",
		.num_resources = ARRAY_SIZE(mt6359p_battery_oc_resources),
		.resources = mt6359p_battery_oc_resources,
	}, {
		.name = "mtk-dynamic-loading-throttling",
		.of_compatible = "mediatek,mt6359p-dynamic_loading_throttling",
	}, {
		.name = "mtk-lbat_service",
		.of_compatible = "mediatek,mt6359p-lbat_service",
		.num_resources = ARRAY_SIZE(mt6359p_lbat_service_resources),
		.resources = mt6359p_lbat_service_resources,
	}, {
		.name = "mtk-pmic-keys",
		.num_resources = ARRAY_SIZE(mt6359p_keys_resources),
		.resources = mt6359p_keys_resources,
		.of_compatible = "mediatek,mt6359p-keys"
	}, {
		.name = "mt6359p-sound",
		.of_compatible = "mediatek,mt6359p-sound"
	}, {
		.name = "mtk-clock-buffer",
		.of_compatible = "mediatek,mt6359p-clock-buffer",
	}
};

static const struct mfd_cell mt6397_devs[] = {
	{
		.name = "mt6397-rtc",
		.num_resources = ARRAY_SIZE(mt6397_rtc_resources),
		.resources = mt6397_rtc_resources,
		.of_compatible = "mediatek,mt6397-rtc",
	}, {
		.name = "mt6397-regulator",
		.of_compatible = "mediatek,mt6397-regulator",
	}, {
		.name = "mt6397-codec",
		.of_compatible = "mediatek,mt6397-codec",
	}, {
		.name = "mt6397-clk",
		.of_compatible = "mediatek,mt6397-clk",
	}, {
		.name = "mt6397-pinctrl",
		.of_compatible = "mediatek,mt6397-pinctrl",
	}, {
		.name = "mtk-pmic-keys",
		.num_resources = ARRAY_SIZE(mt6397_keys_resources),
		.resources = mt6397_keys_resources,
		.of_compatible = "mediatek,mt6397-keys"
	}
};

struct chip_data {
	u32 cid_addr;
	u32 cid_shift;
	const struct mfd_cell *cells;
	int cell_size;
	int (*irq_init)(struct mt6397_chip *chip);
};

static const struct chip_data mt6323_core = {
	.cid_addr = MT6323_CID,
	.cid_shift = 0,
	.cells = mt6323_devs,
	.cell_size = ARRAY_SIZE(mt6323_devs),
	.irq_init = mt6397_irq_init,
};

static const struct chip_data mt6358_core = {
	.cid_addr = MT6358_SWCID,
	.cid_shift = 8,
	.cells = mt6358_devs,
	.cell_size = ARRAY_SIZE(mt6358_devs),
	.irq_init = mt6358_irq_init,
};

static const struct chip_data mt6359p_core = {
	.cid_addr = MT6359P_SWCID,
	.cid_shift = 8,
	.cells = mt6359p_devs,
	.cell_size = ARRAY_SIZE(mt6359p_devs),
	.irq_init = mt6358_irq_init,
};

static const struct chip_data mt6397_core = {
	.cid_addr = MT6397_CID,
	.cid_shift = 0,
	.cells = mt6397_devs,
	.cell_size = ARRAY_SIZE(mt6397_devs),
	.irq_init = mt6397_irq_init,
};

static int mt6397_probe(struct platform_device *pdev)
{
	int ret;
	unsigned int id = 0;
	struct mt6397_chip *pmic;
	const struct chip_data *pmic_core;

	pmic = devm_kzalloc(&pdev->dev, sizeof(*pmic), GFP_KERNEL);
	if (!pmic)
		return -ENOMEM;

	pmic->dev = &pdev->dev;

	/*
	 * mt6397 MFD is child device of soc pmic wrapper.
	 * Regmap is set from its parent.
	 */
	pmic->regmap = dev_get_regmap(pdev->dev.parent, NULL);
	if (!pmic->regmap)
		return -ENODEV;

	pmic_core = of_device_get_match_data(&pdev->dev);
	if (!pmic_core)
		return -ENODEV;

	ret = regmap_read(pmic->regmap, pmic_core->cid_addr, &id);
	if (ret) {
		dev_err(&pdev->dev, "Failed to read chip id: %d\n", ret);
		return ret;
	}

	pmic->chip_id = (id >> pmic_core->cid_shift) & 0xff;

	platform_set_drvdata(pdev, pmic);

	pmic->irq = platform_get_irq(pdev, 0);
	if (pmic->irq <= 0)
		return pmic->irq;

	ret = pmic_core->irq_init(pmic);
	if (ret)
		return ret;

	ret = devm_mfd_add_devices(&pdev->dev, PLATFORM_DEVID_NONE,
				   pmic_core->cells, pmic_core->cell_size,
				   NULL, 0, pmic->irq_domain);
	if (ret) {
		irq_domain_remove(pmic->irq_domain);
		dev_err(&pdev->dev, "failed to add child devices: %d\n", ret);
	}

	return ret;
}

static const struct of_device_id mt6397_of_match[] = {
	{
		.compatible = "mediatek,mt6323",
		.data = &mt6323_core,
	}, {
		.compatible = "mediatek,mt6358",
		.data = &mt6358_core,
	}, {
		.compatible = "mediatek,mt6359p",
		.data = &mt6359p_core,
	}, {
		.compatible = "mediatek,mt6397",
		.data = &mt6397_core,
	}, {
		/* sentinel */
	}
};
MODULE_DEVICE_TABLE(of, mt6397_of_match);

static const struct platform_device_id mt6397_id[] = {
	{ "mt6397", 0 },
	{ },
};
MODULE_DEVICE_TABLE(platform, mt6397_id);

static struct platform_driver mt6397_driver = {
	.probe = mt6397_probe,
	.driver = {
		.name = "mt6397",
		.of_match_table = of_match_ptr(mt6397_of_match),
	},
	.id_table = mt6397_id,
};

module_platform_driver(mt6397_driver);

MODULE_AUTHOR("Flora Fu, MediaTek");
MODULE_DESCRIPTION("Driver for MediaTek MT6397 PMIC");
MODULE_LICENSE("GPL");
