// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#include <linux/fs.h>
#include <linux/module.h>
#include <linux/of_device.h>

#include <mtk_lpm_module.h>

static int __init mt6853_early_initcall(void)
{
	return 0;
}
#ifndef MTK_LPM_MODE_MODULE
subsys_initcall(mt6853_early_initcall);
#endif

static int __init mt6853_device_initcall(void)
{
	mtk_dbg_common_fs_init();
	mt6853_dbg_fs_init();
	mtk_cpupm_dbg_init();
	return 0;
}

static int __init mt6853_late_initcall(void)
{
	mt6853_lpm_trace_init();
	return 0;
}
#ifndef MTK_LPM_MODE_MODULE
late_initcall_sync(mt6853_late_initcall);
#endif

int __init mt6853_init(void)
{
	int ret = 0;
#ifdef MTK_LPM_MODE_MODULE
	ret = mt6853_early_initcall();
#endif
	if (ret)
		goto mt6853_init_fail;

	ret = mt6853_device_initcall();

	if (ret)
		goto mt6853_init_fail;

#ifdef MTK_LPM_MODE_MODULE
	ret = mt6853_late_initcall();
#endif

	if (ret)
		goto mt6853_init_fail;

	return 0;
mt6853_init_fail:
	return -EAGAIN;
}

void __exit mt6853_exit(void)
{
}

module_init(mt6853_init);
module_exit(mt6853_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("MT6853 Low Power Debug Module");
MODULE_AUTHOR("MediaTek Inc.");
