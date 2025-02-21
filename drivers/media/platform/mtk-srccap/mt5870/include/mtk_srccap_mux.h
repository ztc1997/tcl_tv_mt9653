/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef MTK_SRCCAP_MUX_H
#define MTK_SRCCAP_MUX_H


/* ============================================================================================== */
/* ------------------------------------------ Defines ------------------------------------------- */
/* ============================================================================================== */

/* ============================================================================================== */
/* ------------------------------------------- Macros ------------------------------------------- */
/* ============================================================================================== */

/* ============================================================================================== */
/* ------------------------------------------- Enums -------------------------------------------- */
/* ============================================================================================== */

/* ============================================================================================== */
/* ------------------------------------------ Structs ------------------------------------------- */
/* ============================================================================================== */
struct mtk_srccap_dev;

struct srccap_mux_info {
};

/* ============================================================================================== */
/* ----------------------------------------- Functions ------------------------------------------ */
/* ============================================================================================== */
int mtk_srccap_register_mux_subdev(
	struct v4l2_device *srccap_dev,
	struct v4l2_subdev *subdev_mux,
	struct v4l2_ctrl_handler *mux_ctrl_handler);
void mtk_srccap_unregister_mux_subdev(
	struct v4l2_subdev *subdev_mux);

#endif  // MTK_SRCCAP_MUX_H
