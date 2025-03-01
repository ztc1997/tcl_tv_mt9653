/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */
#ifndef _MTK_TV_DRM_VIDEO_FRC_THERMAL_H_
#define _MTK_TV_DRM_VIDEO_FRC_THERMAL_H_

#include <linux/platform_device.h>

//-------------------------------------------------------------------------------------------------
//  Local Defines
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
//  Enum
//-------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------
//  Structures
//-------------------------------------------------------------------------------------------------
struct mtk_frc_cdev {
	unsigned long cur_state;
};

struct mtk_frc_thermal {
	bool thermal_disable_memc;
};

//-------------------------------------------------------------------------------------------------
//  Global Functions
//-------------------------------------------------------------------------------------------------
int mtk_frc_cdev_probe(struct platform_device *pdev);
int mtk_frc_cdev_remove(struct platform_device *pdev);

#endif
