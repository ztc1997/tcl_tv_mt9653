// SPDX-License-Identifier: GPL-2.0
// Copyright (c) 2019 MediaTek Inc.

/*****************************************************************************
 *
 * Filename:
 * ---------
 *	 IMX519mipi_Sensor.c
 *
 * Project:
 * --------
 *	 ALPS
 *
 * Description:
 * ------------
 *	 Source code of Sensor driver
 *
 *
 *------------------------------------------------------------------------------
 * Upper this line, this part is controlled by CC/CQ. DO NOT MODIFY!!
 *============================================================================
 ****************************************************************************/
#include <linux/videodev2.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/atomic.h>
#include <linux/types.h>

#include "kd_camera_typedef.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"

#include "imx519mipiraw_Sensor.h"
#include "imx519_eeprom.h"

#include "adaptor-subdrv.h"

#include "adaptor-subdrv.h"
#include "adaptor-i2c.h"

#define read_cmos_sensor_8(...) subdrv_i2c_rd_u8(__VA_ARGS__)
#define read_cmos_sensor(...) subdrv_i2c_rd_u16(__VA_ARGS__)
#define write_cmos_sensor_8(...) subdrv_i2c_wr_u8(__VA_ARGS__)
#define imx519_table_write_cmos_sensor(...) subdrv_i2c_wr_regs_u8(__VA_ARGS__)

#undef VENDOR_EDIT

/***************Modify Following Strings for Debug**********************/
#define PFX "IMX519_camera_sensor"
/****************************   Modify end	**************************/
#define LOG_INF(format, args...) pr_debug(PFX "[%s] " format, __func__, ##args)

#define FULL_SIZE_60_FPS 0
#define READOUT_TIME_DECREASE 0

#define MULTI_WRITE 1
#if MULTI_WRITE
#define I2C_BUFFER_LEN 765 /* trans# max is 255, each 3 bytes */
#else
#define I2C_BUFFER_LEN 3
#endif

#ifdef VENDOR_EDIT
#define MODULE_ID_OFFSET 0x0000
#endif

/* 2-trio setting on capture mode */
#define IMX519_CAP_2TRIO 0

static struct imgsensor_info_struct imgsensor_info = {
	.sensor_id = IMX519_SENSOR_ID,

	.checksum_value = 0x8ac2d94a,
	.pre = {
		.pclk = 500000000,
		.linelength = 5920,
		.framelength = 2815,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 2328,
		.grabwindow_height = 1728, /*1746*/
		.mipi_data_lp2hs_settle_dc = 85,
		/* following for GetDefaultFramerateByScenario() */
		.mipi_pixel_rate = 226290000,
		.max_framerate = 300, /* 30fps */
	},
#if IMX519_CAP_2TRIO
	.cap = {
		.pclk = 600000000,
		.linelength = 6400,
		.framelength = 3564,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 4656, /* 0x1230 */
		.grabwindow_height = 3496, /* 0xda8 */
		.mipi_data_lp2hs_settle_dc = 85,
		.mipi_pixel_rate = 480055000,
		.max_framerate = 260,
	},
#else
	#if FULL_SIZE_60_FPS
	.cap = {
		.pclk = 1384000000,
		.linelength = 6400,
		.framelength = 3604,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 4656, /* 0x1230 */
		.grabwindow_height = 3496, /* 0xda8 */
		.mipi_data_lp2hs_settle_dc = 85,
		.mipi_pixel_rate = 1018110000,
		.max_framerate = 600,
	},
	#else
		#if READOUT_TIME_DECREASE
	.cap = {
		.pclk = 1264000000,
		.linelength = 8320,
		.framelength = 5064,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 4656,
		.grabwindow_height = 3496,
		.mipi_data_lp2hs_settle_dc = 85,
		.mipi_pixel_rate = 787200000,
		.max_framerate = 300,
	},
		#else
	.cap = {
		.pclk = 700000000,
		.linelength = 6400,
		.framelength = 3645,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 4656,
		.grabwindow_height = 3496,
		.mipi_data_lp2hs_settle_dc = 85,
		.mipi_pixel_rate = 559540000,
		.max_framerate = 300,
	},
		#endif
	#endif
#endif /* IMX519_CAP_2TRIO */
#if READOUT_TIME_DECREASE
	.normal_video = {
		.pclk = 1264000000,
		.linelength = 8320,
		.framelength = 5064,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 4656,
		.grabwindow_height = 2616,
		.mipi_data_lp2hs_settle_dc = 85,
		.mipi_pixel_rate = 787200000,
		.max_framerate = 300,
	},
#else
	.normal_video = {
		.pclk = 700000000,
		.linelength = 6400,
		.framelength = 3645,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 4656,
		.grabwindow_height = 2616,
		.mipi_data_lp2hs_settle_dc = 85,
		.mipi_pixel_rate = 559540000,
		.max_framerate = 300,
	},
#endif
	.hs_video = {
		.pclk = 1000000000,
		.linelength = 5920,
		.framelength = 1407,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 1920,
		.grabwindow_height = 1080,
		.mipi_data_lp2hs_settle_dc = 85,
		.mipi_pixel_rate = 394910000,
		.max_framerate = 1200,
	},
	.slim_video = {
		.pclk = 1000000000,
		.linelength = 5920,
		.framelength = 1407,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 1920,
		.grabwindow_height = 1080,
		.mipi_data_lp2hs_settle_dc = 85,
		.mipi_pixel_rate = 392910000,
		.max_framerate = 1200,
	},
	.custom1 = {
		.pclk = 1176000000,
		.linelength = 5920,
		.framelength = 827,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 1280,
		.grabwindow_height = 720,
		.mipi_data_lp2hs_settle_dc = 85,
		.mipi_pixel_rate = 333260000,
		.max_framerate = 2400,
	},
	.custom2 = {
		.pclk = 1376000000,
		.linelength = 2960,
		.framelength = 968,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 1280, /*0x500*/
		.grabwindow_height = 720, /*0x2D0*/
		.mipi_pixel_rate = 834000000,
		.mipi_data_lp2hs_settle_dc = 85,
		.max_framerate = 4800,
	},
	.custom3 = { /* 4M*60fps */
		.pclk = 1400000000,
		.linelength = 12800,
		.framelength = 1822,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 2328, /*0x500*/
		.grabwindow_height = 1748, /*0x2D0*/
		.mipi_pixel_rate = 296230000,
		.mipi_data_lp2hs_settle_dc = 85,
		.max_framerate = 600,
	},
	.min_gain = 64, /*1x gain*/
	.max_gain = 1024, /*16x gain*/
	.min_gain_iso = 100,
	.margin = 32,		/* sensor framelength & shutter margin */
	.min_shutter = 2,	/* min shutter */
	.gain_step = 1,
	.gain_type = 0,
	.max_frame_length = 0xffff,
	.ae_shut_delay_frame = 0,
	.ae_sensor_gain_delay_frame = 0,
	.ae_ispGain_delay_frame = 2,	/* isp gain delay frame for AE cycle */
	.ihdr_support = 0,	/* 1, support; 0,not support */
	.ihdr_le_firstline = 0,	/* 1,le first ; 0, se first */
	.temperature_support = 1, /* 1, support; 0,not support */
	.sensor_mode_num = 8,	/* support sensor mode num */

	.cap_delay_frame = 2,	/* enter capture delay frame num */
	.pre_delay_frame = 2,	/* enter preview delay frame num */
	.video_delay_frame = 2,	/* enter video delay frame num */
	.hs_video_delay_frame = 2,
	.slim_video_delay_frame = 2,	/* enter slim video delay frame num */
	.custom1_delay_frame = 2,	/* enter custom1 delay frame num */
	.custom2_delay_frame = 2,	/* enter custom2 delay frame num */
	.custom3_delay_frame = 2,

	.isp_driving_current = ISP_DRIVING_4MA,
	.sensor_interface_type = SENSOR_INTERFACE_TYPE_MIPI,
	/* .mipi_sensor_type = MIPI_OPHY_NCSI2, */
	/* 0,MIPI_OPHY_NCSI2;  1,MIPI_OPHY_CSI2 */
	.mipi_sensor_type = MIPI_CPHY, /* 0,MIPI_OPHY_NCSI2; 1,MIPI_OPHY_CSI2 */
	.mipi_settle_delay_mode = 0,
	.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_B,
#if IMX519_CAP_2TRIO
	.mclk = 6, /* 6MHz mclk for 26fps */
	.mipi_lane_num = SENSOR_MIPI_2_LANE,
#else
	.mclk = 24, /* mclk value, suggest 24 or 26 for 24Mhz or 26Mhz */
	/*.mipi_lane_num = SENSOR_MIPI_4_LANE,*/
	.mipi_lane_num = SENSOR_MIPI_3_LANE,
#endif
	.i2c_addr_table = {0x34, 0xff},
	/* record sensor support all write id addr,
	 * only supprt 4 must end with 0xff
	 */
	.i2c_speed = 1000, /* i2c read/write speed */
};



/* Sensor output window information */
static struct SENSOR_WINSIZE_INFO_STRUCT imgsensor_winsize_info[8] = {
	{4656, 3496, 000, 000, 4656, 3496, 2328, 1728,
	0000, 0000, 2328, 1728, 0, 0, 2328, 1728}, /* Preview */
	{4656, 3496, 000, 000, 4656, 3496, 4656, 3496,
	0000, 0000, 4656, 3496, 0, 0, 4656, 3496}, /* capture */
	{4656, 3496, 000, 440, 4656, 2616, 4656, 2616,
	0000, 0000, 4656, 2616, 0, 0, 4656, 2616}, /* video */
	{4656, 3496, 408, 668, 3840, 2160, 1920, 1080,
	0000, 0000, 1920, 1080, 0, 0, 1920, 1080}, /* hight speed video */
	{4656, 3496, 408, 668, 3840, 2160, 1920, 1080,
	0000, 0000, 1920, 1080, 0, 0, 1920, 1080}, /* slim video */
	{4656, 3496, 1048, 1028, 2560, 1440, 1280,  720,
	 0000, 0000, 1280,  720,    0,	  0, 1280,  720}, /*custom1*/
	{4656, 3496, 1048, 1028, 2560, 1440, 1280,  720,
	 0000, 0000, 1280,  720,    0,	  0, 1280,  720}, /*custom2*/
	{4656, 3496,    0,    0, 4656, 3496, 2328, 1748,
	    0,    0, 2328, 1748,    0,    0, 2328, 1748}, /* custom3 */
};
 /*VC1 for HDR(DT=0X35), VC2 for PDAF(DT=0X36), unit : 10bit */
static struct SENSOR_VC_INFO_STRUCT SENSOR_VC_INFO[3] = {
	/* Preview mode setting */
	{0x03, 0x0a, 0x00, 0x08, 0x40, 0x00,
	 0x00, 0x2b, 0x0A70, 0x07D8, 0x00, 0x00, 0x0000, 0x0000,
	 0x00, 0x36, 0x0B5E, 0x0001, 0x00, 0x00, 0x0000, 0x0000},
	/* Capture mode setting */
	{0x03, 0x0a, 0x00, 0x08, 0x40, 0x00,
	 0x00, 0x2b, 0x14E0, 0x0FB0, 0x00, 0x00, 0x0000, 0x0000,
	 0x00, 0x36, 0x16BC, 0x0001, 0x00, 0x00, 0x0000, 0x0000},
	/* Video mode setting */
	{0x02, 0x0a, 0x00, 0x08, 0x40, 0x00,
	 0x00, 0x2b, 0x14E0, 0x0FB0, 0x00, 0x00, 0x0000, 0x0000,
	 0x00, 0x36, 0x16BC, 0x0001, 0x00, 0x00, 0x0000, 0x0000}
};

#if MULTI_WRITE
static void imx519_get_pdaf_reg_setting(struct subdrv_ctx *ctx, MUINT32 regNum, kal_uint16 *regDa)
{
	int i, idx;

	for (i = 0; i < regNum; i++) {
		idx = 2 * i;
		regDa[idx + 1] = read_cmos_sensor_8(ctx, regDa[idx]);
		/*LOG_INF("%x %x", regDa[idx], regDa[idx+1]);*/
	}
}
static void imx519_set_pdaf_reg_setting(struct subdrv_ctx *ctx, MUINT32 regNum, kal_uint16 *regDa)
{
	imx519_table_write_cmos_sensor(ctx, regDa, regNum*2);
}

static void imx519_apply_LRC(struct subdrv_ctx *ctx)
{
	u8 imx519_LRC_data[352] = {0};

	LOG_INF("E");

	read_imx519_LRC(ctx, imx519_LRC_data);

	/* L */
	subdrv_i2c_wr_p8(ctx, 0x7520, imx519_LRC_data, 70);

	/* R */
	subdrv_i2c_wr_p8(ctx, 0x7568, imx519_LRC_data + 70, 70);

	LOG_INF("readback LRC, L1(%d) L70(%d) R1(%d) R70(%d)\n",
		read_cmos_sensor_8(ctx, 0x7520), read_cmos_sensor_8(ctx, 0x7565),
		read_cmos_sensor_8(ctx, 0x7568), read_cmos_sensor_8(ctx, 0x75AD));

}

static void set_dummy(struct subdrv_ctx *ctx)
{
	LOG_INF("dummyline = %d, dummypixels = %d\n",
		ctx->dummy_line, ctx->dummy_pixel);
	/* return;*/ /* for test */
	write_cmos_sensor_8(ctx, 0x0104, 0x01);

	write_cmos_sensor_8(ctx, 0x0340, ctx->frame_length >> 8);
	write_cmos_sensor_8(ctx, 0x0341, ctx->frame_length & 0xFF);
	write_cmos_sensor_8(ctx, 0x0342, ctx->line_length >> 8);
	write_cmos_sensor_8(ctx, 0x0343, ctx->line_length & 0xFF);

	write_cmos_sensor_8(ctx, 0x0104, 0x00);

}	/*	set_dummy  */

static void set_mirror_flip(struct subdrv_ctx *ctx, kal_uint8 image_mirror)
{
	kal_uint8 itemp;

	LOG_INF("image_mirror = %d\n", image_mirror);
	itemp = read_cmos_sensor_8(ctx, 0x0101);
	itemp &= ~0x03;

	switch (image_mirror) {

	case IMAGE_NORMAL:
	write_cmos_sensor_8(ctx, 0x0101, itemp);
	break;

	case IMAGE_V_MIRROR:
	write_cmos_sensor_8(ctx, 0x0101, itemp | 0x02);
	break;

	case IMAGE_H_MIRROR:
	write_cmos_sensor_8(ctx, 0x0101, itemp | 0x01);
	break;

	case IMAGE_HV_MIRROR:
	write_cmos_sensor_8(ctx, 0x0101, itemp | 0x03);
	break;
	}
}

static void set_max_framerate(struct subdrv_ctx *ctx, UINT16 framerate, kal_bool min_framelength_en)
{
	/*kal_int16 dummy_line;*/
	kal_uint32 frame_length = ctx->frame_length;

	LOG_INF("framerate = %d, min framelength should enable %d\n", framerate,
		min_framelength_en);

	frame_length = ctx->pclk / framerate * 10 / ctx->line_length;
	if (frame_length >= ctx->min_frame_length)
		ctx->frame_length = frame_length;
	else
		ctx->frame_length = ctx->min_frame_length;

	ctx->dummy_line =
			ctx->frame_length - ctx->min_frame_length;

	if (ctx->frame_length > imgsensor_info.max_frame_length) {
		ctx->frame_length = imgsensor_info.max_frame_length;
		ctx->dummy_line =
			ctx->frame_length - ctx->min_frame_length;
	}
	if (min_framelength_en)
		ctx->min_frame_length = ctx->frame_length;
	set_dummy(ctx);
}	/*	set_max_framerate  */

#define MAX_CIT_LSHIFT 7
static void write_shutter(struct subdrv_ctx *ctx, kal_uint32 shutter)
{
	kal_uint16 realtime_fps = 0;
	kal_uint16 l_shift = 1;

	if (shutter > ctx->min_frame_length - imgsensor_info.margin)
		ctx->frame_length = shutter + imgsensor_info.margin;
	else
		ctx->frame_length = ctx->min_frame_length;
	if (ctx->frame_length > imgsensor_info.max_frame_length)
		ctx->frame_length = imgsensor_info.max_frame_length;
	if (shutter < imgsensor_info.min_shutter)
		shutter = imgsensor_info.min_shutter;

	if (ctx->autoflicker_en) {
		realtime_fps = ctx->pclk / ctx->line_length * 10
				/ ctx->frame_length;
		LOG_INF("autoflicker enable, realtime_fps = %d\n",
			realtime_fps);
		if (realtime_fps >= 297 && realtime_fps <= 305)
			set_max_framerate(ctx, 296, 0);
		else if (realtime_fps >= 147 && realtime_fps <= 150)
			set_max_framerate(ctx, 146, 0);
	}

	/* long expsoure */
	if (shutter >
		(imgsensor_info.max_frame_length - imgsensor_info.margin)) {

		for (l_shift = 1; l_shift < MAX_CIT_LSHIFT; l_shift++) {
			if ((shutter >> l_shift)
		    < (imgsensor_info.max_frame_length - imgsensor_info.margin))

				break;
		}
		if (l_shift > MAX_CIT_LSHIFT) {
			LOG_INF(
			    "Unable to set such a long exposure %d, set to max\n",
			    shutter);

			l_shift = MAX_CIT_LSHIFT;
		}
		shutter = shutter >> l_shift;
		ctx->frame_length = shutter + imgsensor_info.margin;
		LOG_INF("enter long exposure mode, time is %d", l_shift);
		write_cmos_sensor_8(ctx, 0x3100,
			read_cmos_sensor(ctx, 0x3100) | (l_shift & 0x7));
		/* Frame exposure mode customization for LE*/
		ctx->ae_frm_mode.frame_mode_1 = IMGSENSOR_AE_MODE_SE;
		ctx->ae_frm_mode.frame_mode_2 = IMGSENSOR_AE_MODE_SE;
		ctx->current_ae_effective_frame = 2;
	} else {
		write_cmos_sensor_8(ctx, 0x0104, 0x01);
		write_cmos_sensor_8(ctx, 0x3100,
			read_cmos_sensor(ctx, 0x3100) & 0xf8);
		write_cmos_sensor_8(ctx, 0x0340, ctx->frame_length >> 8);
		write_cmos_sensor_8(ctx, 0x0341, ctx->frame_length & 0xFF);
		write_cmos_sensor_8(ctx, 0x0104, 0x00);
		ctx->current_ae_effective_frame = 2;
		LOG_INF("set frame_length\n");
	}

	/* Update Shutter */
	write_cmos_sensor_8(ctx, 0x0104, 0x01);
	write_cmos_sensor_8(ctx, 0x0350, 0x01); /* Enable auto extend */
	write_cmos_sensor_8(ctx, 0x0202, (shutter >> 8) & 0xFF);
	write_cmos_sensor_8(ctx, 0x0203, shutter  & 0xFF);
	write_cmos_sensor_8(ctx, 0x0104, 0x00);

	LOG_INF("shutter =%d, framelength =%d\n",
		shutter, ctx->frame_length);
}	/*	write_shutter  */

/*************************************************************************
 * FUNCTION
 *	set_shutter
 *
 * DESCRIPTION
 *	This function set e-shutter of sensor to change exposure time.
 *
 * PARAMETERS
 *	iShutter : exposured lines
 *
 * RETURNS
 *	None
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
static void set_shutter(struct subdrv_ctx *ctx, kal_uint32 shutter)
{
	ctx->shutter = shutter;

	write_shutter(ctx, shutter);
} /* set_shutter */


/*************************************************************************
 * FUNCTION
 *	set_shutter_frame_length
 *
 * DESCRIPTION
 *	for frame & 3A sync
 *
 *************************************************************************/
static void set_shutter_frame_length(struct subdrv_ctx *ctx, kal_uint16 shutter,
				     kal_uint16 frame_length,
				     kal_bool auto_extend_en)
{	kal_uint16 realtime_fps = 0;
	kal_int32 dummy_line = 0;

	ctx->shutter = shutter;

	/*0x3500, 0x3501, 0x3502 will increase VBLANK to
	 *get exposure larger than frame exposure
	 *AE doesn't update sensor gain at capture mode,
	 *thus extra exposure lines must be updated here.
	 */

	/* OV Recommend Solution */
	/*if shutter bigger than frame_length,
	 *should extend frame length first
	 */
	/* Change frame time */
	dummy_line = frame_length - ctx->frame_length;
	ctx->frame_length = ctx->frame_length + dummy_line;
	ctx->min_frame_length = ctx->frame_length;

	if (shutter > ctx->min_frame_length - imgsensor_info.margin)
		ctx->frame_length = shutter + imgsensor_info.margin;
	else
		ctx->frame_length = ctx->min_frame_length;
	if (ctx->frame_length > imgsensor_info.max_frame_length)
		ctx->frame_length = imgsensor_info.max_frame_length;
	shutter = (shutter < imgsensor_info.min_shutter)
			? imgsensor_info.min_shutter : shutter;
	shutter =
	(shutter > (imgsensor_info.max_frame_length - imgsensor_info.margin))
		? (imgsensor_info.max_frame_length - imgsensor_info.margin)
		: shutter;

	if (ctx->autoflicker_en) {
		realtime_fps = ctx->pclk / ctx->line_length * 10 /
				ctx->frame_length;
		if (realtime_fps >= 297 && realtime_fps <= 305)
			set_max_framerate(ctx, 296, 0);
		else if (realtime_fps >= 147 && realtime_fps <= 150)
			set_max_framerate(ctx, 146, 0);
		else {
			/* Extend frame length */
			write_cmos_sensor_8(ctx, 0x0104, 0x01);
			write_cmos_sensor_8(ctx, 0x0340,
					ctx->frame_length >> 8);
			write_cmos_sensor_8(ctx, 0x0341,
					ctx->frame_length & 0xFF);
			write_cmos_sensor_8(ctx, 0x0104, 0x00);
		}
	} else {
		/* Extend frame length */
		write_cmos_sensor_8(ctx, 0x0104, 0x01);
		write_cmos_sensor_8(ctx, 0x0340, ctx->frame_length >> 8);
		write_cmos_sensor_8(ctx, 0x0341, ctx->frame_length & 0xFF);
		write_cmos_sensor_8(ctx, 0x0104, 0x00);
	}

	/* Update Shutter */
	write_cmos_sensor_8(ctx, 0x0104, 0x01);
	if (auto_extend_en)
		write_cmos_sensor_8(ctx, 0x0350, 0x01); /* Enable auto extend */
	else
		write_cmos_sensor_8(ctx, 0x0350, 0x00); /* Disable auto extend */
	write_cmos_sensor_8(ctx, 0x0202, (shutter >> 8) & 0xFF);
	write_cmos_sensor_8(ctx, 0x0203, shutter  & 0xFF);
	write_cmos_sensor_8(ctx, 0x0104, 0x00);
	LOG_INF(
		"Exit! shutter =%d, framelength =%d/%d, dummy_line=%d, auto_extend=%d\n",
		shutter, ctx->frame_length, frame_length,
		dummy_line, read_cmos_sensor_8(ctx, 0x0350));

}	/* set_shutter_frame_length */

static kal_uint16 gain2reg(struct subdrv_ctx *ctx, const kal_uint16 gain)
{
	kal_uint16 reg_gain = 0x0;

	reg_gain = 1024 - (1024*64)/gain;
	return (kal_uint16) reg_gain;
}

/*************************************************************************
 * FUNCTION
 *	set_gain
 *
 * DESCRIPTION
 *	This function is to set global gain to sensor.
 *
 * PARAMETERS
 *	iGain : sensor global gain(base: 0x40)
 *
 * RETURNS
 *	the actually gain set to sensor.
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
static kal_uint16 set_gain(struct subdrv_ctx *ctx, kal_uint16 gain)
{
	kal_uint16 reg_gain;

	if (gain < imgsensor_info.min_gain || gain > imgsensor_info.max_gain) {
		LOG_INF("Error gain setting");

		if (gain < imgsensor_info.min_gain)
			gain = imgsensor_info.min_gain;
		else
			gain = imgsensor_info.max_gain;
	}

	reg_gain = gain2reg(ctx, gain);
	ctx->gain = reg_gain;
	LOG_INF("gain = %d, reg_gain = 0x%x\n ", gain, reg_gain);

	write_cmos_sensor_8(ctx, 0x0104, 0x01);
	write_cmos_sensor_8(ctx, 0x0204, (reg_gain>>8) & 0xFF);
	write_cmos_sensor_8(ctx, 0x0205, reg_gain & 0xFF);
	write_cmos_sensor_8(ctx, 0x0104, 0x00);

	return gain;
} /* set_gain */

/*************************************************************************
 * FUNCTION
 *	night_mode
 *
 * DESCRIPTION
 *	This function night mode of sensor.
 *
 * PARAMETERS
 *	bEnable: KAL_TRUE -> enable night mode, otherwise, disable night mode
 *
 * RETURNS
 *	None
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
static kal_uint32 streaming_control(struct subdrv_ctx *ctx, kal_bool enable)
{
	LOG_INF("streaming_enable(0=Sw Standby,1=streaming): %d\n",
		enable);
	if (enable)
		write_cmos_sensor_8(ctx, 0x0100, 0X01);
	else
		write_cmos_sensor_8(ctx, 0x0100, 0x00);
	return ERROR_NONE;
}

#if IMX519_CAP_2TRIO
static kal_uint16 imx519_2trio_init_setting[] = {
	0x0136, 0x0C,
	0x0137, 0x00,
	0x3C7E, 0x04,
	0x3C7F, 0x0C,
	0x3020, 0x00,
	0x3E35, 0x01,
	0x3F7F, 0x01,
	0x5637, 0x11,
	0x5657, 0x11,
	0x5659, 0x12,
	0x5933, 0x11,
	0x5953, 0x11,
	0x5955, 0x12,
	0x5AB3, 0x11,
	0x5AD3, 0x11,
	0x5AD5, 0x12,
	0x5C15, 0x2A,
	0x5C17, 0x80,
	0x5C19, 0x31,
	0x5C1B, 0x87,
	0x5C25, 0x25,
	0x5C27, 0x7B,
	0x5C29, 0x2A,
	0x5C2B, 0x80,
	0x5C2D, 0x31,
	0x5C2F, 0x87,
	0x5C35, 0x2B,
	0x5C37, 0x81,
	0x5C39, 0x31,
	0x5C3B, 0x87,
	0x5C45, 0x25,
	0x5C47, 0x7B,
	0x5C49, 0x2A,
	0x5C4B, 0x80,
	0x5C4D, 0x31,
	0x5C4F, 0x87,
	0x5C55, 0x2D,
	0x5C57, 0x83,
	0x5C59, 0x32,
	0x5C5B, 0x88,
	0x5C65, 0x29,
	0x5C67, 0x7F,
	0x5C69, 0x2E,
	0x5C6B, 0x84,
	0x5C6D, 0x32,
	0x5C6F, 0x88,
	0x5E69, 0x04,
	0x5F18, 0x10,
	0x5F1A, 0x0E,
	0x5F20, 0x12,
	0x5F22, 0x10,
	0x5F24, 0x0E,
	0x5F28, 0x10,
	0x5F2A, 0x0E,
	0x5F30, 0x12,
	0x5F32, 0x10,
	0x5F34, 0x0E,
	0x5F38, 0x0F,
	0x5F39, 0x0D,
	0x5F3C, 0x11,
	0x5F3D, 0x0F,
	0x5F3E, 0x0D,
	0x5FD1, 0x00,
	0x7600, 0x01,
	0x79A0, 0x01,
	0x79A1, 0x01,
	0x79A2, 0x01,
	0x79A3, 0x01,
	0x79A4, 0x01,
	0x79A5, 0x20,
	0x79A9, 0x00,
	0x79AA, 0x01,
	0x79AD, 0x00,
	0x79AF, 0x00,
	0x8173, 0x01,
	0x835C, 0x01,
	0x8A74, 0x01,
	0x8C1F, 0x00,
	0x8C27, 0x00,
	0x8C3B, 0x03,
	0x9004, 0x1A,
	0x920C, 0x6A,
	0x920D, 0x22,
	0x920E, 0x6A,
	0x920F, 0x23,
	0x9214, 0x6A,
	0x9215, 0x20,
	0x9216, 0x6A,
	0x9217, 0x21,
	0x9218, 0x63,
	0x9219, 0x38,
	0x921A, 0x63,
	0x921B, 0x39,
	0x921C, 0x63,
	0x921D, 0x3A,
	0x921E, 0x63,
	0x921F, 0x3B,
	0x9220, 0x63,
	0x9221, 0x3E,
	0x9222, 0x63,
	0x9223, 0x68,
	0x9224, 0x63,
	0x9225, 0x69,
	0x9226, 0x63,
	0x9227, 0x6A,
	0x9228, 0x63,
	0x9229, 0x6B,
	0x922A, 0x93,
	0x922B, 0x95,
	0x922C, 0x93,
	0x922D, 0x97,
	0x922E, 0x93,
	0x922F, 0x99,
	0x9230, 0x93,
	0x9231, 0xC5,
	0x9232, 0x93,
	0x9233, 0xC7,
	0x9234, 0x93,
	0x9235, 0xC9,
	0x995C, 0x8C,
	0x995D, 0x00,
	0x995E, 0x00,
	0x9963, 0x64,
	0x9964, 0x50,
	0xAA0A, 0x26,
	0xAE03, 0x04,
	0xAE04, 0x03,
	0xAE05, 0x03,
	0xAA06, 0x3F,
	0xAA07, 0x05,
	0xAA08, 0x04,
	0xAA12, 0x3F,
	0xAA13, 0x04,
	0xAA14, 0x03,
	0xAA1E, 0x12,
	0xAA1F, 0x05,
	0xAA20, 0x04,
	0xAA2A, 0x0D,
	0xAA2B, 0x04,
	0xAA2C, 0x03,
	0xAC19, 0x02,
	0xAC1B, 0x01,
	0xAC1D, 0x01,
	0xAC3C, 0x00,
	0xAC3D, 0x01,
	0xAC3E, 0x00,
	0xAC3F, 0x01,
	0xAC40, 0x00,
	0xAC41, 0x01,
	0xAC61, 0x02,
	0xAC63, 0x01,
	0xAC65, 0x01,
	0xAC84, 0x00,
	0xAC85, 0x01,
	0xAC86, 0x00,
	0xAC87, 0x01,
	0xAC88, 0x00,
	0xAC89, 0x01,
};
static kal_uint16 imx519_2trio_capture_setting[] = {
	0x0111, 0x03,
	0x0112, 0x0A,
	0x0113, 0x0A,
	0x0114, 0x01,
	0x0342, 0x19,
	0x0343, 0x00,
	0x0340, 0x0D,
	0x0341, 0xEC,
	0x0344, 0x00,
	0x0345, 0x00,
	0x0346, 0x00,
	0x0347, 0x00,
	0x0348, 0x12,
	0x0349, 0x2F,
	0x034A, 0x0D,
	0x034B, 0xA7,
	0x0220, 0x00,
	0x0221, 0x11,
	0x0222, 0x01,
	0x0900, 0x00,
	0x0901, 0x11,
	0x0902, 0x0A,
	0x3F4C, 0x01,
	0x3F4D, 0x01,
	0x4254, 0x7F,
	0x0401, 0x00,
	0x0404, 0x00,
	0x0405, 0x10,
	0x0408, 0x00,
	0x0409, 0x00,
	0x040A, 0x00,
	0x040B, 0x00,
	0x040C, 0x12,
	0x040D, 0x30,
	0x040E, 0x0D,
	0x040F, 0xA8,
	0x034C, 0x12,
	0x034D, 0x30,
	0x034E, 0x0D,
	0x034F, 0xA8,
	0x0301, 0x06,
	0x0303, 0x02,
	0x0305, 0x02,
	0x0306, 0x01,
	0x0307, 0x2C,
	0x0309, 0x0A,
	0x030B, 0x01,
	0x030D, 0x02,
	0x030E, 0x01,
	0x030F, 0x5E,
	0x0310, 0x01,
	0x0820, 0x10,
	0x0821, 0x68,
	0x0822, 0x00,
	0x0823, 0x00,
	0x0106, 0x00,
	0x0B00, 0x00,
	0x3230, 0x00,
	0x3F14, 0x01,
	0x3F3C, 0x01,
	0x3F0D, 0x0A,
	0x3C06, 0x00,
	0x3C07, 0xA2,
	0x3C0A, 0x00,
	0x3C0B, 0x00,
	0x3F78, 0x00,
	0x3F79, 0x00,
	0x3F7C, 0x00,
	0x3F7D, 0x00,
	0x0202, 0x03,
	0x0203, 0xE8,
	0x0224, 0x01,
	0x0225, 0xF4,
	0x0204, 0x00,
	0x0205, 0x00,
	0x0216, 0x00,
	0x0217, 0x00,
	0x020E, 0x01,
	0x020F, 0x00,
	0x0218, 0x01,
	0x0219, 0x00,
	0x3E20, 0x01,
	0x3E37, 0x01,
};
#else /* not IMX519_CAP_2TRIO */
static kal_uint16 imx519_init_setting[] = {
	0x0136, 0x18,
	0x0137, 0x00,
	0x3C7E, 0x01,
	0x3C7F, 0x07,
	0x3020, 0x00,
	0x3E35, 0x01,
	0x3F7F, 0x01,
	0x5609, 0x57,
	0x5613, 0x51,
	0x561F, 0x5E,
	0x5623, 0xD2,
	0x5637, 0x11,
	0x5657, 0x11,
	0x5659, 0x12,
	0x5733, 0x60,
	0x5905, 0x57,
	0x590F, 0x51,
	0x591B, 0x5E,
	0x591F, 0xD2,
	0x5933, 0x11,
	0x5953, 0x11,
	0x5955, 0x12,
	0x5A2F, 0x60,
	0x5A85, 0x57,
	0x5A8F, 0x51,
	0x5A9B, 0x5E,
	0x5A9F, 0xD2,
	0x5AB3, 0x11,
	0x5AD3, 0x11,
	0x5AD5, 0x12,
	0x5BAF, 0x60,
	0x5C15, 0x2A,
	0x5C17, 0x80,
	0x5C19, 0x31,
	0x5C1B, 0x87,
	0x5C25, 0x25,
	0x5C27, 0x7B,
	0x5C29, 0x2A,
	0x5C2B, 0x80,
	0x5C2D, 0x31,
	0x5C2F, 0x87,
	0x5C35, 0x2B,
	0x5C37, 0x81,
	0x5C39, 0x31,
	0x5C3B, 0x87,
	0x5C45, 0x25,
	0x5C47, 0x7B,
	0x5C49, 0x2A,
	0x5C4B, 0x80,
	0x5C4D, 0x31,
	0x5C4F, 0x87,
	0x5C55, 0x2D,
	0x5C57, 0x83,
	0x5C59, 0x32,
	0x5C5B, 0x88,
	0x5C65, 0x29,
	0x5C67, 0x7F,
	0x5C69, 0x2E,
	0x5C6B, 0x84,
	0x5C6D, 0x32,
	0x5C6F, 0x88,
	0x5E69, 0x04,
	0x5E9D, 0x00,
	0x5F18, 0x10,
	0x5F1A, 0x0E,
	0x5F20, 0x12,
	0x5F22, 0x10,
	0x5F24, 0x0E,
	0x5F28, 0x10,
	0x5F2A, 0x0E,
	0x5F30, 0x12,
	0x5F32, 0x10,
	0x5F34, 0x0E,
	0x5F38, 0x0F,
	0x5F39, 0x0D,
	0x5F3C, 0x11,
	0x5F3D, 0x0F,
	0x5F3E, 0x0D,
	0x5F61, 0x07,
	0x5F64, 0x05,
	0x5F67, 0x03,
	0x5F6A, 0x03,
	0x5F6D, 0x07,
	0x5F70, 0x07,
	0x5F73, 0x05,
	0x5F76, 0x02,
	0x5F79, 0x07,
	0x5F7C, 0x07,
	0x5F7F, 0x07,
	0x5F82, 0x07,
	0x5F85, 0x03,
	0x5F88, 0x02,
	0x5F8B, 0x01,
	0x5F8E, 0x01,
	0x5F91, 0x04,
	0x5F94, 0x05,
	0x5F97, 0x02,
	0x5F9D, 0x07,
	0x5FA0, 0x07,
	0x5FA3, 0x07,
	0x5FA6, 0x07,
	0x5FA9, 0x03,
	0x5FAC, 0x01,
	0x5FAF, 0x01,
	0x5FB5, 0x03,
	0x5FB8, 0x02,
	0x5FBB, 0x01,
	0x5FC1, 0x07,
	0x5FC4, 0x07,
	0x5FC7, 0x07,
	0x5FD1, 0x00,
	0x6302, 0x79,
	0x6305, 0x78,
	0x6306, 0xA5,
	0x6308, 0x03,
	0x6309, 0x20,
	0x630B, 0x0A,
	0x630D, 0x48,
	0x630F, 0x06,
	0x6311, 0xA4,
	0x6313, 0x03,
	0x6314, 0x20,
	0x6316, 0x0A,
	0x6317, 0x31,
	0x6318, 0x4A,
	0x631A, 0x06,
	0x631B, 0x40,
	0x631C, 0xA4,
	0x631E, 0x03,
	0x631F, 0x20,
	0x6321, 0x0A,
	0x6323, 0x4A,
	0x6328, 0x80,
	0x6329, 0x01,
	0x632A, 0x30,
	0x632B, 0x02,
	0x632C, 0x20,
	0x632D, 0x02,
	0x632E, 0x30,
	0x6330, 0x60,
	0x6332, 0x90,
	0x6333, 0x01,
	0x6334, 0x30,
	0x6335, 0x02,
	0x6336, 0x20,
	0x6338, 0x80,
	0x633A, 0xA0,
	0x633B, 0x01,
	0x633C, 0x60,
	0x633D, 0x02,
	0x633E, 0x60,
	0x633F, 0x01,
	0x6340, 0x30,
	0x6341, 0x02,
	0x6342, 0x20,
	0x6343, 0x03,
	0x6344, 0x80,
	0x6345, 0x03,
	0x6346, 0x90,
	0x6348, 0xF0,
	0x6349, 0x01,
	0x634A, 0x20,
	0x634B, 0x02,
	0x634C, 0x10,
	0x634D, 0x03,
	0x634E, 0x60,
	0x6350, 0xA0,
	0x6351, 0x01,
	0x6352, 0x60,
	0x6353, 0x02,
	0x6354, 0x50,
	0x6355, 0x02,
	0x6356, 0x60,
	0x6357, 0x01,
	0x6358, 0x30,
	0x6359, 0x02,
	0x635A, 0x30,
	0x635B, 0x03,
	0x635C, 0x90,
	0x635F, 0x01,
	0x6360, 0x10,
	0x6361, 0x01,
	0x6362, 0x40,
	0x6363, 0x02,
	0x6364, 0x50,
	0x6368, 0x70,
	0x636A, 0xA0,
	0x636B, 0x01,
	0x636C, 0x50,
	0x637D, 0xE4,
	0x637E, 0xB4,
	0x638C, 0x8E,
	0x638D, 0x38,
	0x638E, 0xE3,
	0x638F, 0x4C,
	0x6390, 0x30,
	0x6391, 0xC3,
	0x6392, 0xAE,
	0x6393, 0xBA,
	0x6394, 0xEB,
	0x6395, 0x6E,
	0x6396, 0x34,
	0x6397, 0xE3,
	0x6398, 0xCF,
	0x6399, 0x3C,
	0x639A, 0xF3,
	0x639B, 0x0C,
	0x639C, 0x30,
	0x639D, 0xC1,
	0x63B9, 0xA3,
	0x63BA, 0xFE,
	0x7600, 0x01,
	0x79A0, 0x01,
	0x79A1, 0x01,
	0x79A2, 0x01,
	0x79A3, 0x01,
	0x79A4, 0x01,
	0x79A5, 0x20,
	0x79A9, 0x00,
	0x79AA, 0x01,
	0x79AD, 0x00,
	0x79AF, 0x00,
	0x8173, 0x01,
	0x835C, 0x01,
	0x8A74, 0x01,
	0x8C1F, 0x00,
	0x8C27, 0x00,
	0x8C3B, 0x03,
	0x9004, 0x0B,
	0x920C, 0x6A,
	0x920D, 0x22,
	0x920E, 0x6A,
	0x920F, 0x23,
	0x9214, 0x6A,
	0x9215, 0x20,
	0x9216, 0x6A,
	0x9217, 0x21,
	0x9385, 0x3E,
	0x9387, 0x1B,
	0x938D, 0x4D,
	0x938F, 0x43,
	0x9391, 0x1B,
	0x9395, 0x4D,
	0x9397, 0x43,
	0x9399, 0x1B,
	0x939D, 0x3E,
	0x939F, 0x2F,
	0x93A5, 0x43,
	0x93A7, 0x2F,
	0x93A9, 0x2F,
	0x93AD, 0x34,
	0x93AF, 0x2F,
	0x93B5, 0x3E,
	0x93B7, 0x2F,
	0x93BD, 0x4D,
	0x93BF, 0x43,
	0x93C1, 0x2F,
	0x93C5, 0x4D,
	0x93C7, 0x43,
	0x93C9, 0x2F,
	0x974B, 0x02,
	0x995C, 0x8C,
	0x995D, 0x00,
	0x995E, 0x00,
	0x9963, 0x64,
	0x9964, 0x50,
	0xAA0A, 0x26,
	0xAE03, 0x04,
	0xAE04, 0x03,
	0xAE05, 0x03,
	0xBC1C, 0x08,
	0xAA06, 0x3F,
	0xAA07, 0x05,
	0xAA08, 0x04,
	0xAA12, 0x3F,
	0xAA13, 0x04,
	0xAA14, 0x03,
	0xAA1E, 0x12,
	0xAA1F, 0x05,
	0xAA20, 0x04,
	0xAA2A, 0x0D,
	0xAA2B, 0x04,
	0xAA2C, 0x03,
	0xAC19, 0x02,
	0xAC1B, 0x01,
	0xAC1D, 0x01,
	0xAC3C, 0x00,
	0xAC3D, 0x01,
	0xAC3E, 0x00,
	0xAC3F, 0x01,
	0xAC40, 0x00,
	0xAC41, 0x01,
	0xAC61, 0x02,
	0xAC63, 0x01,
	0xAC65, 0x01,
	0xAC84, 0x00,
	0xAC85, 0x01,
	0xAC86, 0x00,
	0xAC87, 0x01,
	0xAC88, 0x00,
	0xAC89, 0x01,
	0x38AC, 0x01,
	0x38AD, 0x01,
	0x38AE, 0x01,
	0x38AF, 0x01,
	0x38B0, 0x01,
	0x38B1, 0x01,
	0x38B2, 0x01,
	0x38B3, 0x01,
/* 2x1OCL adjustment setting */
	0x9810, 0x00,
	0x9813, 0x00,
	0x9816, 0x00,
	0x9819, 0x00,
	0x981C, 0x00,
	0x981F, 0x00,
	0x9822, 0x00,
	0x9825, 0x00,
	0xA92B, 0x00,
	0xA92D, 0x00,
	0xA92F, 0x00,
	0x983A, 0x00,
	0x983D, 0x00,
	0x9843, 0x00,
	0x984C, 0x00,
	0xA95B, 0x00,
	0xA95D, 0x00,
	0xA95F, 0x00,
	0x9860, 0x00,
	0x9863, 0x00,
	0x9869, 0x00,
	0x9872, 0x00,
	0xA98B, 0x00,
	0xA98D, 0x00,
	0xA98F, 0x00,
	0x9883, 0x00,
	0x9886, 0x00,
	0x9889, 0x00,
	0x988C, 0x00,
	0x988F, 0x00,
	0x9892, 0x00,
	0x9895, 0x00,
	0x9898, 0x00,
	0xA9BB, 0x00,
	0xA9BD, 0x00,
	0xA9BF, 0x00,
	0x98AC, 0x00,
	0x98AF, 0x00,
	0x98B5, 0x00,
	0x98BE, 0x00,
	0xA9EB, 0x00,
	0xA9ED, 0x00,
	0xA9EF, 0x00,
};
#if FULL_SIZE_60_FPS
static kal_uint16 imx519_capture_60_setting[] = {
	0x0111, 0x03,
	0x0112, 0x0A,
	0x0113, 0x0A,
	0x0114, 0x02,
	0x0342, 0x19,
	0x0343, 0x00,
	0x0340, 0x0E,
	0x0341, 0x14,
	0x0344, 0x00,
	0x0345, 0x00,
	0x0346, 0x00,
	0x0347, 0x00,
	0x0348, 0x12,
	0x0349, 0x2F,
	0x034A, 0x0D,
	0x034B, 0xA7,
	0x0220, 0x01,
	0x0221, 0x11,
	0x0222, 0x01,
	0x0900, 0x00,
	0x0901, 0x11,
	0x0902, 0x0A,
	0x3F4C, 0x01,
	0x3F4D, 0x01,
	0x4254, 0x7F,
	0x0401, 0x00,
	0x0404, 0x00,
	0x0405, 0x10,
	0x0408, 0x00,
	0x0409, 0x00,
	0x040A, 0x00,
	0x040B, 0x00,
	0x040C, 0x12,
	0x040D, 0x30,
	0x040E, 0x0D,
	0x040F, 0xA8,
	0x034C, 0x12,
	0x034D, 0x30,
	0x034E, 0x0D,
	0x034F, 0xA8,
	/*PDAF Area Config Begin*/
	0x38A3, 0x02,
	0x38B4, 0x05,
	0x38B5, 0xBB,
	0x38B6, 0x04,
	0x38B7, 0x2B,
	0x38B8, 0x0C,
	0x38B9, 0x74,
	0x38BA, 0x09,
	0x38BB, 0x7C,
	0x38AC, 0x01,
	0x38AD, 0x00,
	0x38AE, 0x00,
	0x38AF, 0x00,
	0x38B0, 0x00,
	0x38B1, 0x00,
	0x38B2, 0x00,
	0x38B3, 0x00,
	/*PDAF Area Config End*/
	0x0301, 0x06,
	0x0303, 0x02,
	0x0305, 0x04,
	0x0306, 0x01,
	0x0307, 0x5A,
	0x0309, 0x0A,
	0x030B, 0x01,
	0x030D, 0x04,
	0x030E, 0x01,
	0x030F, 0x19,
	0x0310, 0x01,
	0x0820, 0x13,
	0x0821, 0xC2,
	0x0822, 0x00,
	0x0823, 0x00,
	0x3E20, 0x01,
	0x3E37, 0x01,
	0x3E3B, 0x00,
	0x0106, 0x00,
	0x0B00, 0x00,
	0x3230, 0x00,
	0x3F14, 0x00,
	0x3F3C, 0x03,
	0x3F0D, 0x0A,
	0x3FBC, 0x00,
	0x3C06, 0x00,
	0x3C07, 0x80,
	0x3C0A, 0x00,
	0x3C0B, 0x00,
	0x3F78, 0x01,
	0x3F79, 0x54,
	0x3F7C, 0x00,
	0x3F7D, 0x00,
	0x0202, 0x03,
	0x0203, 0xE8,
	0x0224, 0x03,
	0x0225, 0xE8,
	0x0204, 0x00,
	0x0205, 0x00,
	0x0216, 0x00,
	0x0217, 0x00,
	0x020E, 0x01,
	0x020F, 0x00,
	0x0218, 0x01,
	0x0219, 0x00,
	0x3614, 0x00,
	0x3616, 0x0D,
	0x3617, 0x56,
	0xB612, 0x20,
	0xB613, 0x20,
	0xB614, 0x20,
	0xB615, 0x20,
	0xB616, 0x0A,
	0xB617, 0x0A,
	0xB618, 0x20,
	0xB619, 0x20,
	0xB61A, 0x20,
	0xB61B, 0x20,
	0xB61C, 0x0A,
	0xB61D, 0x0A,
	0xB666, 0x30,
	0xB667, 0x30,
	0xB668, 0x30,
	0xB669, 0x30,
	0xB66A, 0x14,
	0xB66B, 0x14,
	0xB66C, 0x20,
	0xB66D, 0x20,
	0xB66E, 0x20,
	0xB66F, 0x20,
	0xB670, 0x10,
	0xB671, 0x10,
	0x3900, 0x00,
	0x3901, 0x00,
	0x3237, 0x00,
	0x30AC, 0x00,
};
#else
	#if READOUT_TIME_DECREASE
static kal_uint16 imx519_capture_30_setting[] = {
	0x0111, 0x03,
	0x0112, 0x0A,
	0x0113, 0x0A,
	0x0114, 0x02,
	0x0342, 0x20,
	0x0343, 0x80,
	0x0340, 0x13,
	0x0341, 0xC8,
	0x0344, 0x00,
	0x0345, 0x00,
	0x0346, 0x00,
	0x0347, 0x00,
	0x0348, 0x12,
	0x0349, 0x2F,
	0x034A, 0x0D,
	0x034B, 0xA7,
	0x0220, 0x01,
	0x0221, 0x11,
	0x0222, 0x01,
	0x0900, 0x00,
	0x0901, 0x11,
	0x0902, 0x0A,
	0x3F4C, 0x01,
	0x3F4D, 0x01,
	0x4254, 0x7F,
	0x0401, 0x00,
	0x0404, 0x00,
	0x0405, 0x10,
	0x0408, 0x00,
	0x0409, 0x00,
	0x040A, 0x00,
	0x040B, 0x00,
	0x040C, 0x12,
	0x040D, 0x30,
	0x040E, 0x0D,
	0x040F, 0xA8,
	0x034C, 0x12,
	0x034D, 0x30,
	0x034E, 0x0D,
	0x034F, 0xA8,
	0x0301, 0x06,
	0x0303, 0x02,
	0x0305, 0x04,
	0x0306, 0x01,
	0x0307, 0x3C,
	0x0309, 0x0A,
	0x030B, 0x02,
	0x030D, 0x03,
	0x030E, 0x01,
	0x030F, 0x1F,
	0x0310, 0x01,
	0x0820, 0x0D,
	0x0821, 0x74,
	0x0822, 0x00,
	0x0823, 0x00,
	0x0106, 0x00,
	0x0B00, 0x00,
	0x3230, 0x00,
	0x3F14, 0x00,
	0x3F3C, 0x03,
	0x3F0D, 0x0A,
	0x3C06, 0x01,
	0x3C07, 0xA1,
	0x3C0A, 0x00,
	0x3C0B, 0x00,
	0x3F78, 0x00,
	0x3F79, 0x00,
	0x3F7C, 0x00,
	0x3F7D, 0x00,
	0x0202, 0x03,
	0x0203, 0xE8,
	0x0224, 0x03,
	0x0225, 0xE8,
	0x0204, 0x00,
	0x0205, 0x00,
	0x0216, 0x00,
	0x0217, 0x00,
	0x020E, 0x01,
	0x020F, 0x00,
	0x0218, 0x01,
	0x0219, 0x00,
	0x3E20, 0x01,
	0x3E37, 0x01,
};
	#else
static kal_uint16 imx519_capture_30_setting[] = {
	0x0111, 0x03,
	0x0112, 0x0A,
	0x0113, 0x0A,
	0x0114, 0x02,
	0x0342, 0x19,
	0x0343, 0x00,
	0x0340, 0x0E,
	0x0341, 0x3D,
	0x0344, 0x00,
	0x0345, 0x00,
	0x0346, 0x00,
	0x0347, 0x00,
	0x0348, 0x12,
	0x0349, 0x2F,
	0x034A, 0x0D,
	0x034B, 0xA7,
	0x0220, 0x01,
	0x0221, 0x11,
	0x0222, 0x01,
	0x0900, 0x00,
	0x0901, 0x11,
	0x0902, 0x0A,
	0x3F4C, 0x01,
	0x3F4D, 0x01,
	0x4254, 0x7F,
	0x0401, 0x00,
	0x0404, 0x00,
	0x0405, 0x10,
	0x0408, 0x00,
	0x0409, 0x00,
	0x040A, 0x00,
	0x040B, 0x00,
	0x040C, 0x12,
	0x040D, 0x30,
	0x040E, 0x0D,
	0x040F, 0xA8,
	0x034C, 0x12,
	0x034D, 0x30,
	0x034E, 0x0D,
	0x034F, 0xA8,
	0x0301, 0x06,
	0x0303, 0x04,
	0x0305, 0x04,
	0x0306, 0x01,
	0x0307, 0x5E,
	0x0309, 0x0A,
	0x030B, 0x02,
	0x030D, 0x04,
	0x030E, 0x01,
	0x030F, 0x10,
	0x0310, 0x01,
	0x0820, 0x09,
	0x0821, 0x90,
	0x0822, 0x00,
	0x0823, 0x00,
	0x0106, 0x00,
	0x0B00, 0x00,
	0x3230, 0x00,
	0x3F14, 0x00,
	0x3F3C, 0x03,
	0x3F0D, 0x0A,
	0x3C06, 0x00,
	0x3C07, 0x80,
	0x3C0A, 0x00,
	0x3C0B, 0x00,
	0x3F78, 0x01,
	0x3F79, 0x54,
	0x3F7C, 0x00,
	0x3F7D, 0x00,
	0x0202, 0x03,
	0x0203, 0xE8,
	0x0224, 0x03,
	0x0225, 0xE8,
	0x0204, 0x00,
	0x0205, 0x00,
	0x0216, 0x00,
	0x0217, 0x00,
	0x020E, 0x01,
	0x020F, 0x00,
	0x0218, 0x01,
	0x0219, 0x00,
	0x3E20, 0x01,
	0x3E37, 0x01,
};
	#endif
#endif
#endif /*IMX519_CAP_2TRIO*/

static kal_uint16 imx519_preview_setting[] = {
	0x0111, 0x03,
	0x0112, 0x0A,
	0x0113, 0x0A,
	0x0114, 0x02,
	0x0342, 0x17,
	0x0343, 0x20,
	0x0340, 0x0A,
	0x0341, 0xFF,
	0x0344, 0x00,
	0x0345, 0x00,
	0x0346, 0x00,
	0x0347, 0x12,
	0x0348, 0x12,
	0x0349, 0x2F,
	0x034A, 0x0D,
	0x034B, 0x95,
	0x0220, 0x00,
	0x0221, 0x11,
	0x0222, 0x01,
	0x0900, 0x01,
	0x0901, 0x22,
	0x0902, 0x08, //0x84,
	0x3F4C, 0x05,
	0x3F4D, 0x03,
	/*PDAF Area Config Begin*/
	0x38A3, 0x02,/*0:Fixed (16x12), 1:Fixed (8x6), 2:Flexible*/
	0x38B4, 0x02,
	0x38B5, 0xE2,/*738*/
	0x38B6, 0x02,
	0x38B7, 0x10,/*528*/
	0x38B8, 0x06,
	0x38B9, 0x36,/*1590*/
	0x38BA, 0x04,
	0x38BB, 0xB0,/*1200*/
	0x38AC, 0x01,/* enable and disable Flexible window,total 8*/
	0x38AD, 0x00,
	0x38AE, 0x00,
	0x38AF, 0x00,
	0x38B0, 0x00,
	0x38B1, 0x00,
	0x38B2, 0x00,
	0x38B3, 0x00,
	/*PDAF Area Config End*/
	0x4254, 0x7F,
	0x0401, 0x00,
	0x0404, 0x00,
	0x0405, 0x10,
	0x0408, 0x00,
	0x0409, 0x00,
	0x040A, 0x00,
	0x040B, 0x00,
	0x040C, 0x09,
	0x040D, 0x18,
	0x040E, 0x06,
	0x040F, 0xC0,
	0x034C, 0x09,
	0x034D, 0x18,
	0x034E, 0x06,
	0x034F, 0xC0,
	0x0301, 0x06,
	0x0303, 0x04,
	0x0305, 0x04,
	0x0306, 0x00,
	0x0307, 0xFA,
	0x0309, 0x0A,
	0x030B, 0x04,
	0x030D, 0x04,
	0x030E, 0x00,
	0x030F, 0xE6,
	0x0310, 0x01,
	0x0820, 0x04,
	0x0821, 0x0B,
	0x0822, 0x00,
	0x0823, 0x00,
	0x3E20, 0x01,
	0x3E37, 0x01,
	0x3E3B, 0x00,
	0x0106, 0x00,
	0x0B00, 0x00,
	0x3230, 0x00,
	0x3F14, 0x00,
	0x3F3C, 0x03,
	0x3F0D, 0x0A,
	0x3FBC, 0x00,
	0x3C06, 0x00,
	0x3C07, 0x00,
	0x3C0A, 0x00,
	0x3C0B, 0xF0,
	0x3F78, 0x01,
	0x3F79, 0x38,
	0x3F7C, 0x00,
	0x3F7D, 0x00,
	0x0202, 0x03,
	0x0203, 0xE8,
	0x0224, 0x01,
	0x0225, 0xF4,
	0x0204, 0x00,
	0x0205, 0x00,
	0x0216, 0x00,
	0x0217, 0x00,
	0x020E, 0x01,
	0x020F, 0x00,
	0x0218, 0x01,
	0x0219, 0x00,
};

#if READOUT_TIME_DECREASE
static kal_uint16 imx519_normal_video_setting[] = {
	0x0111, 0x03,
	0x0112, 0x0A,
	0x0113, 0x0A,
	0x0114, 0x02,
	0x0342, 0x20,
	0x0343, 0x80,
	0x0340, 0x13,
	0x0341, 0xC8,
	0x0344, 0x00,
	0x0345, 0x00,
	0x0346, 0x01,
	0x0347, 0xB0,
	0x0348, 0x12,
	0x0349, 0x2F,
	0x034A, 0x0B,
	0x034B, 0xE7,
	0x0220, 0x01,
	0x0221, 0x11,
	0x0222, 0x01,
	0x0900, 0x00,
	0x0901, 0x11,
	0x0902, 0x0A,
	0x3F4C, 0x01,
	0x3F4D, 0x01,
	0x4254, 0x7F,
	0x0401, 0x00,
	0x0404, 0x00,
	0x0405, 0x10,
	0x0408, 0x00,
	0x0409, 0x00,
	0x040A, 0x00,
	0x040B, 0x00,
	0x040C, 0x12,
	0x040D, 0x30,
	0x040E, 0x0A,
	0x040F, 0x38,
	0x034C, 0x12,
	0x034D, 0x30,
	0x034E, 0x0A,
	0x034F, 0x38,
	0x0301, 0x06,
	0x0303, 0x02,
	0x0305, 0x04,
	0x0306, 0x01,
	0x0307, 0x3C,
	0x0309, 0x0A,
	0x030B, 0x02,
	0x030D, 0x03,
	0x030E, 0x01,
	0x030F, 0x1F,
	0x0310, 0x01,
	0x0820, 0x0D,
	0x0821, 0x74,
	0x0822, 0x00,
	0x0823, 0x00,
	0x0106, 0x00,
	0x0B00, 0x00,
	0x3230, 0x00,
	0x3F14, 0x00,
	0x3F3C, 0x03,
	0x3F0D, 0x0A,
	0x3C06, 0x01,
	0x3C07, 0xA1,
	0x3C0A, 0x00,
	0x3C0B, 0x00,
	0x3F78, 0x00,
	0x3F79, 0x00,
	0x3F7C, 0x00,
	0x3F7D, 0x00,
	0x0202, 0x03,
	0x0203, 0xE8,
	0x0224, 0x03,
	0x0225, 0xE8,
	0x0204, 0x00,
	0x0205, 0x00,
	0x0216, 0x00,
	0x0217, 0x00,
	0x020E, 0x01,
	0x020F, 0x00,
	0x0218, 0x01,
	0x0219, 0x00,
	0x3E20, 0x01,
	0x3E37, 0x01,
};
#else
static kal_uint16 imx519_normal_video_setting[] = {
	0x0111, 0x03,
	0x0112, 0x0A,
	0x0113, 0x0A,
	0x0114, 0x02,
	0x0342, 0x19,
	0x0343, 0x00,
	0x0340, 0x0E,
	0x0341, 0x3D,
	0x0344, 0x00,
	0x0345, 0x00,
	0x0346, 0x01,
	0x0347, 0xB0,
	0x0348, 0x12,
	0x0349, 0x2F,
	0x034A, 0x0B,
	0x034B, 0xE7,
	0x0220, 0x01,
	0x0221, 0x11,
	0x0222, 0x01,
	0x0900, 0x00,
	0x0901, 0x11,
	0x0902, 0x0A,
	0x3F4C, 0x01,
	0x3F4D, 0x01,
	0x4254, 0x7F,
	0x0401, 0x00,
	0x0404, 0x00,
	0x0405, 0x10,
	0x0408, 0x00,
	0x0409, 0x00,
	0x040A, 0x00,
	0x040B, 0x00,
	0x040C, 0x12,
	0x040D, 0x30,
	0x040E, 0x0A,
	0x040F, 0x38,
	0x034C, 0x12,
	0x034D, 0x30,
	0x034E, 0x0A,
	0x034F, 0x38,
	0x0301, 0x06,
	0x0303, 0x04,
	0x0305, 0x04,
	0x0306, 0x01,
	0x0307, 0x5E,
	0x0309, 0x0A,
	0x030B, 0x02,
	0x030D, 0x04,
	0x030E, 0x01,
	0x030F, 0x10,
	0x0310, 0x01,
	0x0820, 0x09,
	0x0821, 0x90,
	0x0822, 0x00,
	0x0823, 0x00,
	0x0106, 0x00,
	0x0B00, 0x00,
	0x3230, 0x00,
	0x3F14, 0x00,
	0x3F3C, 0x03,
	0x3F0D, 0x0A,
	0x3C06, 0x00,
	0x3C07, 0x80,
	0x3C0A, 0x00,
	0x3C0B, 0x00,
	0x3F78, 0x01,
	0x3F79, 0x54,
	0x3F7C, 0x00,
	0x3F7D, 0x00,
	0x0202, 0x03,
	0x0203, 0xE8,
	0x0224, 0x03,
	0x0225, 0xE8,
	0x0204, 0x00,
	0x0205, 0x00,
	0x0216, 0x00,
	0x0217, 0x00,
	0x020E, 0x01,
	0x020F, 0x00,
	0x0218, 0x01,
	0x0219, 0x00,
	0x3E20, 0x01,
	0x3E37, 0x01,
};
#endif

static kal_uint16 imx519_custom1_setting[] = {
	0x0111, 0x03,
	0x0112, 0x0A,
	0x0113, 0x0A,
	0x0114, 0x02,
	0x0342, 0x17,
	0x0343, 0x20,
	0x0340, 0x03,
	0x0341, 0x3B,
	0x0344, 0x00,
	0x0345, 0x00,
	0x0346, 0x04,
	0x0347, 0x02,
	0x0348, 0x12,
	0x0349, 0x2F,
	0x034A, 0x09,
	0x034B, 0xA5,
	0x0220, 0x00,
	0x0221, 0x11,
	0x0222, 0x01,
	0x0900, 0x01,
	0x0901, 0x22,
	0x0902, 0x08,
	0x3F4C, 0x05,
	0x3F4D, 0x03,
	0x4254, 0x7F,
	0x0401, 0x00,
	0x0404, 0x00,
	0x0405, 0x10,
	0x0408, 0x02,
	0x0409, 0x0C,
	0x040A, 0x00,
	0x040B, 0x00,
	0x040C, 0x05,
	0x040D, 0x00,
	0x040E, 0x02,
	0x040F, 0xD0,
	0x034C, 0x05,
	0x034D, 0x00,
	0x034E, 0x02,
	0x034F, 0xD0,
	0x0301, 0x06,
	0x0303, 0x02,
	0x0305, 0x04,
	0x0306, 0x01,
	0x0307, 0x26,
	0x0309, 0x0A,
	0x030B, 0x04,
	0x030D, 0x04,
	0x030E, 0x01,
	0x030F, 0x44,
	0x0310, 0x01,
	0x0820, 0x05,
	0x0821, 0xB2,
	0x0822, 0x00,
	0x0823, 0x00,
	0x0106, 0x00,
	0x0B00, 0x00,
	0x3230, 0x00,
	0x3F14, 0x00,
	0x3F3C, 0x03,
	0x3F0D, 0x0A,
	0x3C06, 0x00,
	0x3C07, 0x00,
	0x3C0A, 0x00,
	0x3C0B, 0xF0,
	0x3F78, 0x01,
	0x3F79, 0x38,
	0x3F7C, 0x00,
	0x3F7D, 0x00,
	0x0202, 0x03,
	0x0203, 0x1B,
	0x0224, 0x01,
	0x0225, 0xF4,
	0x0204, 0x00,
	0x0205, 0x00,
	0x0216, 0x00,
	0x0217, 0x00,
	0x020E, 0x01,
	0x020F, 0x00,
	0x0218, 0x01,
	0x0219, 0x00,
	0x3E20, 0x01,
	0x3E37, 0x01,
};

static kal_uint16 imx519_custom2_setting[] = {
	0x0111, 0x03,
	0x0112, 0x0A,
	0x0113, 0x0A,
	0x0114, 0x02,
	0x0342, 0x0B,
	0x0343, 0x90,
	0x0340, 0x03,
	0x0341, 0xC8,
	0x0344, 0x00,
	0x0345, 0x00,
	0x0346, 0x04,
	0x0347, 0x00,
	0x0348, 0x12,
	0x0349, 0x2F,
	0x034A, 0x09,
	0x034B, 0xA7,
	0x0220, 0x00,
	0x0221, 0x11,
	0x0222, 0x01,
	0x0900, 0x01,
	0x0901, 0x22,
	0x0902, 0x08,
	0x3F4C, 0x01,
	0x3F4D, 0x01,
	0x4254, 0x7F,
	0x0401, 0x00,
	0x0404, 0x00,
	0x0405, 0x10,
	0x0408, 0x02,
	0x0409, 0x0C,
	0x040A, 0x00,
	0x040B, 0x02,
	0x040C, 0x05,
	0x040D, 0x00,
	0x040E, 0x02,
	0x040F, 0xD0,
	0x034C, 0x05,
	0x034D, 0x00,
	0x034E, 0x02,
	0x034F, 0xD0,
	0x0301, 0x06,
	0x0303, 0x02,
	0x0305, 0x04,
	0x0306, 0x01,
	0x0307, 0x58,
	0x0309, 0x0A,
	0x030B, 0x02,
	0x030D, 0x04,
	0x030E, 0x01,
	0x030F, 0x95,
	0x0310, 0x01,
	0x0820, 0x0E,
	0x0821, 0x3D,
	0x0822, 0x00,
	0x0823, 0x00,
	0x0106, 0x00,
	0x0B00, 0x00,
	0x3230, 0x00,
	0x3F14, 0x01,
	0x3F3C, 0x01,
	0x3F0D, 0x0A,
	0x3C06, 0x00,
	0x3C07, 0x00,
	0x3C0A, 0x00,
	0x3C0B, 0x66,
	0x3C0C, 0x80,
	0x3C0D, 0x00,
	0x3C0E, 0xA0,
	0x3C0F, 0x01,
	0x3C10, 0x60,
	0x3C11, 0x70,
	0x3C12, 0x00,
	0x3C13, 0xA0,
	0x3C14, 0x01,
	0x3C15, 0x4D,
	0x3C16, 0x43,
	0x3C17, 0x1B,
	0x3C18, 0x4D,
	0x3C19, 0x43,
	0x3C1A, 0x2F,
	0x3C1B, 0x00,
	0x3F78, 0x01,
	0x3F79, 0xC6,
	0x3F7C, 0x00,
	0x3F7D, 0x00,
	0x0202, 0x03,
	0x0203, 0xA8,
	0x0224, 0x01,
	0x0225, 0xF4,
	0x0204, 0x01,
	0x0205, 0x2B,
	0x0216, 0x00,
	0x0217, 0x00,
	0x020E, 0x01,
	0x020F, 0x00,
	0x0218, 0x01,
	0x0219, 0x00,
	0x3E20, 0x01,
	0x3E37, 0x00,
};

static kal_uint16 imx519_custom3_setting[] = {
	0x0111, 0x03,
	0x0112, 0x0A,
	0x0113, 0x0A,
	0x0114, 0x02,
	0x0342, 0x32,
	0x0343, 0x00,
	0x0340, 0x07,
	0x0341, 0x1E,
	0x0344, 0x00,
	0x0345, 0x00,
	0x0346, 0x00,
	0x0347, 0x00,
	0x0348, 0x12,
	0x0349, 0x2F,
	0x034A, 0x0D,
	0x034B, 0xA7,
	0x0220, 0x00,
	0x0221, 0x11,
	0x0222, 0x01,
	0x0900, 0x01,
	0x0901, 0x22,
	0x0902, 0x0A,
	0x3F4C, 0x01,
	0x3F4D, 0x03,
	0x4254, 0x7F,
	0x0401, 0x00,
	0x0404, 0x00,
	0x0405, 0x10,
	0x0408, 0x00,
	0x0409, 0x00,
	0x040A, 0x00,
	0x040B, 0x00,
	0x040C, 0x09,
	0x040D, 0x18,
	0x040E, 0x06,
	0x040F, 0xD4,
	0x034C, 0x09,
	0x034D, 0x18,
	0x034E, 0x06,
	0x034F, 0xD4,
	0x0301, 0x06,
	0x0303, 0x02,
	0x0305, 0x04,
	0x0306, 0x01,
	0x0307, 0x5E,
	0x0309, 0x0A,
	0x030B, 0x04,
	0x030D, 0x04,
	0x030E, 0x01,
	0x030F, 0x20,
	0x0310, 0x01,
	0x0820, 0x05,
	0x0821, 0x10,
	0x0822, 0x00,
	0x0823, 0x00,
	0x0106, 0x00,
	0x0B00, 0x00,
	0x3230, 0x00,
	0x3F14, 0x01,
	0x3F3C, 0x01,
	0x3F0D, 0x0A,
	0x3C06, 0x00,
	0x3C07, 0x00,
	0x3C0A, 0x01,
	0x3C0B, 0xC8,
	0x3F78, 0x02,
	0x3F79, 0x18,
	0x3F7C, 0x00,
	0x3F7D, 0x00,
	0x0202, 0x03,
	0x0203, 0xE8,
	0x0224, 0x01,
	0x0225, 0xF4,
	0x0204, 0x01,
	0x0205, 0x2B,
	0x0216, 0x00,
	0x0217, 0x00,
	0x020E, 0x01,
	0x020F, 0x00,
	0x0218, 0x01,
	0x0219, 0x00,
	0x3E20, 0x01,
	0x3E37, 0x00,
};

#endif

#if IMX519_CAP_2TRIO
static void sensor_init(struct subdrv_ctx *ctx)
{
	LOG_INF("2-TRIO init\n");
	imx519_table_write_cmos_sensor(ctx, imx519_2trio_init_setting,
		sizeof(imx519_2trio_init_setting)/sizeof(kal_uint16));
	set_mirror_flip(ctx, ctx->mirror);
}
#else
static void sensor_init(struct subdrv_ctx *ctx)
{
	LOG_INF("E\n");
	imx519_table_write_cmos_sensor(ctx, imx519_init_setting,
		sizeof(imx519_init_setting)/sizeof(kal_uint16));

	/*enable temperature sensor, TEMP_SEN_CTL:*/
	write_cmos_sensor_8(ctx, 0x0138, 0x01);

	set_mirror_flip(ctx, ctx->mirror);

	LOG_INF("X");
}	/*	  sensor_init  */
#endif /* IMX519_CAP_2TRIO */

static void preview_setting(struct subdrv_ctx *ctx)
{
	LOG_INF("E\n");

	imx519_table_write_cmos_sensor(ctx, imx519_preview_setting,
		sizeof(imx519_preview_setting)/sizeof(kal_uint16));

	LOG_INF("X");
} /* preview_setting */

#if IMX519_CAP_2TRIO
static void capture_setting(struct subdrv_ctx *ctx, kal_uint16 currefps)
{
	LOG_INF("%s 2-trio capture E! currefps:%d\n", __func__, currefps);
	imx519_table_write_cmos_sensor(ctx, imx519_2trio_capture_setting,
		sizeof(imx519_2trio_capture_setting)/sizeof(kal_uint16));
}
#else /* not IMX519_CAP_2TRIO */
#if FULL_SIZE_60_FPS
/* full size 60fps */
static void capture_setting(struct subdrv_ctx *ctx, kal_uint16 currefps)
{
	LOG_INF("%s 60 fps E! currefps:%d\n", __func__, currefps);

	imx519_table_write_cmos_sensor(ctx, imx519_capture_60_setting,
		sizeof(imx519_capture_60_setting)/sizeof(kal_uint16));

	LOG_INF("%s(PD 012515) 60 fpsX\n", __func__);
}
#else

/*full size 30fps*/
static void capture_setting(struct subdrv_ctx *ctx, kal_uint16 currefps)
{
	LOG_INF("%s(PD 012515) 30 fps E! currefps:%d\n", __func__, currefps);
	/*************MIPI output setting************/

	imx519_table_write_cmos_sensor(ctx, imx519_capture_30_setting,
		sizeof(imx519_capture_30_setting)/sizeof(kal_uint16));
	LOG_INF("%s(PD 012515) 30 fpsX\n", __func__);
}
#endif
#endif /* IMX519_CAP_2TRIO */

static void normal_video_setting(struct subdrv_ctx *ctx, kal_uint16 currefps)
{
	LOG_INF("%s E! currefps:%d\n", __func__, currefps);
	imx519_table_write_cmos_sensor(ctx, imx519_normal_video_setting,
	sizeof(imx519_normal_video_setting)/sizeof(kal_uint16));
	LOG_INF("X\n");
}

static void hs_video_setting(struct subdrv_ctx *ctx)
{
	LOG_INF("%s E! currefps 120\n", __func__);
	/*************MIPI output setting************/
	write_cmos_sensor_8(ctx, 0x0111, 0x03);
	write_cmos_sensor_8(ctx, 0x0112, 0x0A);
	write_cmos_sensor_8(ctx, 0x0113, 0x0A);
	write_cmos_sensor_8(ctx, 0x0114, 0x02);
	write_cmos_sensor_8(ctx, 0x0342, 0x17);
	write_cmos_sensor_8(ctx, 0x0343, 0x20);
	write_cmos_sensor_8(ctx, 0x0340, 0x05);
	write_cmos_sensor_8(ctx, 0x0341, 0x7F);
	write_cmos_sensor_8(ctx, 0x0344, 0x00);
	write_cmos_sensor_8(ctx, 0x0345, 0x00);
	write_cmos_sensor_8(ctx, 0x0346, 0x02);
	write_cmos_sensor_8(ctx, 0x0347, 0x92);
	write_cmos_sensor_8(ctx, 0x0348, 0x12);
	write_cmos_sensor_8(ctx, 0x0349, 0x2F);
	write_cmos_sensor_8(ctx, 0x034A, 0x0B);
	write_cmos_sensor_8(ctx, 0x034B, 0x05);
	write_cmos_sensor_8(ctx, 0x0220, 0x00);
	write_cmos_sensor_8(ctx, 0x0221, 0x11);
	write_cmos_sensor_8(ctx, 0x0222, 0x01);
	write_cmos_sensor_8(ctx, 0x0900, 0x01);
	write_cmos_sensor_8(ctx, 0x0901, 0x22);
	write_cmos_sensor_8(ctx, 0x0902, 0x08);
	write_cmos_sensor_8(ctx, 0x3F4C, 0x05);
	write_cmos_sensor_8(ctx, 0x3F4D, 0x03);
	write_cmos_sensor_8(ctx, 0x4254, 0x7F);
	write_cmos_sensor_8(ctx, 0x0401, 0x00);
	write_cmos_sensor_8(ctx, 0x0404, 0x00);
	write_cmos_sensor_8(ctx, 0x0405, 0x10);
	write_cmos_sensor_8(ctx, 0x0408, 0x00);
	write_cmos_sensor_8(ctx, 0x0409, 0xCC);
	write_cmos_sensor_8(ctx, 0x040A, 0x00);
	write_cmos_sensor_8(ctx, 0x040B, 0x00);
	write_cmos_sensor_8(ctx, 0x040C, 0x07);
	write_cmos_sensor_8(ctx, 0x040D, 0x80);
	write_cmos_sensor_8(ctx, 0x040E, 0x04);
	write_cmos_sensor_8(ctx, 0x040F, 0x38);
	write_cmos_sensor_8(ctx, 0x034C, 0x07);
	write_cmos_sensor_8(ctx, 0x034D, 0x80);
	write_cmos_sensor_8(ctx, 0x034E, 0x04);
	write_cmos_sensor_8(ctx, 0x034F, 0x38);
	write_cmos_sensor_8(ctx, 0x0301, 0x06);
	write_cmos_sensor_8(ctx, 0x0303, 0x02);
	write_cmos_sensor_8(ctx, 0x0305, 0x04);
	write_cmos_sensor_8(ctx, 0x0306, 0x00);
	write_cmos_sensor_8(ctx, 0x0307, 0xFA);
	write_cmos_sensor_8(ctx, 0x0309, 0x0A);
	write_cmos_sensor_8(ctx, 0x030B, 0x04);
	write_cmos_sensor_8(ctx, 0x030D, 0x04);
	write_cmos_sensor_8(ctx, 0x030E, 0x01);
	write_cmos_sensor_8(ctx, 0x030F, 0x7E);
	write_cmos_sensor_8(ctx, 0x0310, 0x01);
	write_cmos_sensor_8(ctx, 0x0820, 0x06);
	write_cmos_sensor_8(ctx, 0x0821, 0xB7);
	write_cmos_sensor_8(ctx, 0x0822, 0x00);
	write_cmos_sensor_8(ctx, 0x0823, 0x00);
	write_cmos_sensor_8(ctx, 0x3E20, 0x01);
	write_cmos_sensor_8(ctx, 0x3E37, 0x01);
	write_cmos_sensor_8(ctx, 0x3E3B, 0x00);
	write_cmos_sensor_8(ctx, 0x0106, 0x00);
	write_cmos_sensor_8(ctx, 0x0B00, 0x00);
	write_cmos_sensor_8(ctx, 0x3230, 0x00);
	write_cmos_sensor_8(ctx, 0x3F14, 0x00);
	write_cmos_sensor_8(ctx, 0x3F3C, 0x03);
	write_cmos_sensor_8(ctx, 0x3F0D, 0x0A);
	write_cmos_sensor_8(ctx, 0x3FBC, 0x00);
	write_cmos_sensor_8(ctx, 0x3C06, 0x00);
	write_cmos_sensor_8(ctx, 0x3C07, 0x00);
	write_cmos_sensor_8(ctx, 0x3C0A, 0x00);
	write_cmos_sensor_8(ctx, 0x3C0B, 0xF0);
	write_cmos_sensor_8(ctx, 0x3F78, 0x01);
	write_cmos_sensor_8(ctx, 0x3F79, 0x38);
	write_cmos_sensor_8(ctx, 0x3F7C, 0x00);
	write_cmos_sensor_8(ctx, 0x3F7D, 0x00);
	write_cmos_sensor_8(ctx, 0x0202, 0x03);
	write_cmos_sensor_8(ctx, 0x0203, 0xE8);
	write_cmos_sensor_8(ctx, 0x0224, 0x01);
	write_cmos_sensor_8(ctx, 0x0225, 0xF4);
	write_cmos_sensor_8(ctx, 0x0204, 0x00);
	write_cmos_sensor_8(ctx, 0x0205, 0x00);
	write_cmos_sensor_8(ctx, 0x0216, 0x00);
	write_cmos_sensor_8(ctx, 0x0217, 0x00);
	write_cmos_sensor_8(ctx, 0x020E, 0x01);
	write_cmos_sensor_8(ctx, 0x020F, 0x00);
	write_cmos_sensor_8(ctx, 0x0218, 0x01);
	write_cmos_sensor_8(ctx, 0x0219, 0x00);
	LOG_INF("X\n");
}

static void slim_video_setting(struct subdrv_ctx *ctx)
{
	LOG_INF("%s E\n", __func__);
	hs_video_setting(ctx);
}

/*full size 16M@24fps*/
static void custom1_setting(struct subdrv_ctx *ctx)
{
	LOG_INF("%s 240 fps E! currefps\n", __func__);
	/*************MIPI output setting************/

	imx519_table_write_cmos_sensor(ctx, imx519_custom1_setting,
		sizeof(imx519_custom1_setting)/sizeof(kal_uint16));

	LOG_INF("X");
}

/*full size 8M@24fps*/
static void custom2_setting(struct subdrv_ctx *ctx)
{
	LOG_INF("%s 480 fps E! currefps\n", __func__);
	/*************MIPI output setting************/

	imx519_table_write_cmos_sensor(ctx, imx519_custom2_setting,
		sizeof(imx519_custom2_setting)/sizeof(kal_uint16));

	LOG_INF("X");
}

/*full size 16M@24fps*/
static void custom3_setting(struct subdrv_ctx *ctx)
{
	LOG_INF("%s 4M*60 fps E! currefps\n", __func__);
	/*************MIPI output setting************/

	imx519_table_write_cmos_sensor(ctx, imx519_custom3_setting,
		sizeof(imx519_custom3_setting)/sizeof(kal_uint16));

	LOG_INF("X");
}

/*************************************************************************
 * FUNCTION
 *	get_imgsensor_id
 *
 * DESCRIPTION
 *	This function get the sensor ID
 *
 * PARAMETERS
 *	*sensorID : return the sensor ID
 *
 * RETURNS
 *	None
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
static int get_imgsensor_id(struct subdrv_ctx *ctx, UINT32 *sensor_id)
{
	kal_uint8 i = 0;
	kal_uint8 retry = 2;
	/*sensor have two i2c address 0x6c 0x6d & 0x21 0x20,
	 *we should detect the module used i2c address
	 */
	while (imgsensor_info.i2c_addr_table[i] != 0xff) {
		ctx->i2c_write_id = imgsensor_info.i2c_addr_table[i];
		do {
			*sensor_id = ((read_cmos_sensor_8(ctx, 0x0016) << 8)
					| read_cmos_sensor_8(ctx, 0x0017));
			if (*sensor_id == imgsensor_info.sensor_id) {
				LOG_INF("i2c write id: 0x%x, sensor id: 0x%x\n",
					ctx->i2c_write_id, *sensor_id);
				return ERROR_NONE;
			}

			LOG_INF("Read sensor id fail, id: 0x%x\n",
				ctx->i2c_write_id);
			retry--;
		} while (retry > 0);
		i++;
		retry = 2;
	}
	if (*sensor_id != imgsensor_info.sensor_id) {
		/*if Sensor ID is not correct,
		 *Must set *sensor_id to 0xFFFFFFFF
		 */
		*sensor_id = 0xFFFFFFFF;
		return ERROR_SENSOR_CONNECT_FAIL;
	}
	return ERROR_NONE;
}


/*************************************************************************
 * FUNCTION
 *	open
 *
 * DESCRIPTION
 *	This function initialize the registers of CMOS sensor
 *
 * PARAMETERS
 *	None
 *
 * RETURNS
 *	None
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
static int open(struct subdrv_ctx *ctx)
{
	kal_uint8 i = 0;
	kal_uint8 retry = 2;
	kal_uint16 sensor_id = 0;

	/*sensor have two i2c address 0x6c 0x6d & 0x21 0x20,
	 *we should detect the module used i2c address
	 */
	while (imgsensor_info.i2c_addr_table[i] != 0xff) {
		ctx->i2c_write_id = imgsensor_info.i2c_addr_table[i];
		do {
			sensor_id = ((read_cmos_sensor_8(ctx, 0x0016) << 8)
					| read_cmos_sensor_8(ctx, 0x0017));
			if (sensor_id == imgsensor_info.sensor_id) {
				LOG_INF("i2c write id: 0x%x, sensor id: 0x%x\n",
					ctx->i2c_write_id, sensor_id);
				break;
			}
			LOG_INF("Read sensor id fail, id: 0x%x\n",
				ctx->i2c_write_id);
			retry--;
		} while (retry > 0);
		i++;
		if (sensor_id == imgsensor_info.sensor_id)
			break;
		retry = 2;
	}
	if (imgsensor_info.sensor_id != sensor_id)
		return ERROR_SENSOR_CONNECT_FAIL;

	/* initail sequence write in  */
	sensor_init(ctx);


	ctx->autoflicker_en = KAL_FALSE;
	ctx->sensor_mode = IMGSENSOR_MODE_INIT;
	ctx->shutter = 0x3D0;
	ctx->gain = 0x100;
	ctx->pclk = imgsensor_info.pre.pclk;
	ctx->frame_length = imgsensor_info.pre.framelength;
	ctx->line_length = imgsensor_info.pre.linelength;
	ctx->min_frame_length = imgsensor_info.pre.framelength;
	ctx->dummy_pixel = 0;
	ctx->dummy_line = 0;
	ctx->ihdr_mode = 0;
	ctx->test_pattern = KAL_FALSE;
	ctx->current_fps = imgsensor_info.pre.max_framerate;

	return ERROR_NONE;
} /* open */

/*************************************************************************
 * FUNCTION
 *	close
 *
 * DESCRIPTION
 *
 *
 * PARAMETERS
 *	None
 *
 * RETURNS
 *	None
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
static int close(struct subdrv_ctx *ctx)
{
	LOG_INF("E\n");

	/*No Need to implement this function*/

	write_cmos_sensor_8(ctx, 0x0100, 0x00);

	return ERROR_NONE;
} /* close */


/*************************************************************************
 * FUNCTION
 * preview
 *
 * DESCRIPTION
 *	This function start the sensor preview.
 *
 * PARAMETERS
 *	*image_window : address pointer of pixel numbers in one period of HSYNC
 *  *sensor_config_data : address pointer of line numbers in one period of VSYNC
 *
 * RETURNS
 *	None
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
static kal_uint32 preview(struct subdrv_ctx *ctx, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
			  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("%s E\n", __func__);

	ctx->sensor_mode = IMGSENSOR_MODE_PREVIEW;
	ctx->pclk = imgsensor_info.pre.pclk;
	ctx->line_length = imgsensor_info.pre.linelength;
	ctx->frame_length = imgsensor_info.pre.framelength;
	ctx->min_frame_length = imgsensor_info.pre.framelength;
	ctx->autoflicker_en = KAL_FALSE;

	preview_setting(ctx);

	if (ctx->pdaf_mode)
		imx519_apply_LRC(ctx);

	return ERROR_NONE;
} /* preview */

/*************************************************************************
 * FUNCTION
 *	capture
 *
 * DESCRIPTION
 *	This function setup the CMOS sensor in capture MY_OUTPUT mode
 *
 * PARAMETERS
 *
 * RETURNS
 *	None
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
static kal_uint32 capture(struct subdrv_ctx *ctx, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
			  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("E\n");
	ctx->sensor_mode = IMGSENSOR_MODE_CAPTURE;

	if (ctx->current_fps != imgsensor_info.cap.max_framerate)
		LOG_INF(
			"Warning: current_fps %d fps is not support, so use cap's setting: %d fps!\n",
			ctx->current_fps,
			imgsensor_info.cap.max_framerate / 10);
	ctx->pclk = imgsensor_info.cap.pclk;
	ctx->line_length = imgsensor_info.cap.linelength;
	ctx->frame_length = imgsensor_info.cap.framelength;
	ctx->min_frame_length = imgsensor_info.cap.framelength;
	ctx->autoflicker_en = KAL_FALSE;


	capture_setting(ctx, ctx->current_fps);

	if (ctx->pdaf_mode)
		imx519_apply_LRC(ctx);

	/* set_mirror_flip(ctx, ctx->mirror); */

	return ERROR_NONE;
}	/* capture(ctx) */
static kal_uint32 normal_video(struct subdrv_ctx *ctx,
		MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
		MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("E\n");

	ctx->sensor_mode = IMGSENSOR_MODE_VIDEO;
	ctx->pclk = imgsensor_info.normal_video.pclk;
	ctx->line_length = imgsensor_info.normal_video.linelength;
	ctx->frame_length = imgsensor_info.normal_video.framelength;
	ctx->min_frame_length = imgsensor_info.normal_video.framelength;
	ctx->autoflicker_en = KAL_FALSE;
	normal_video_setting(ctx, ctx->current_fps);
	if (ctx->pdaf_mode)
		imx519_apply_LRC(ctx);

	/*set_mirror_flip(ctx, ctx->mirror);*/

	return ERROR_NONE;
}	/*	normal_video   */

static kal_uint32 hs_video(struct subdrv_ctx *ctx,
		MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
		MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("E\n");

	ctx->sensor_mode = IMGSENSOR_MODE_HIGH_SPEED_VIDEO;
	ctx->pclk = imgsensor_info.hs_video.pclk;
	/*ctx->video_mode = KAL_TRUE;*/
	ctx->line_length = imgsensor_info.hs_video.linelength;
	ctx->frame_length = imgsensor_info.hs_video.framelength;
	ctx->min_frame_length = imgsensor_info.hs_video.framelength;
	ctx->dummy_line = 0;
	ctx->dummy_pixel = 0;
	/*ctx->current_fps = 300;*/
	ctx->autoflicker_en = KAL_FALSE;
	hs_video_setting(ctx);
	/*set_mirror_flip(ctx, ctx->mirror);*/

	return ERROR_NONE;
}	/*	hs_video   */

static kal_uint32 slim_video(struct subdrv_ctx *ctx,
		MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
		MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("E\n");

	ctx->sensor_mode = IMGSENSOR_MODE_SLIM_VIDEO;
	ctx->pclk = imgsensor_info.slim_video.pclk;
	/*ctx->video_mode = KAL_TRUE;*/
	ctx->line_length = imgsensor_info.slim_video.linelength;
	ctx->frame_length = imgsensor_info.slim_video.framelength;
	ctx->min_frame_length = imgsensor_info.slim_video.framelength;
	ctx->dummy_line = 0;
	ctx->dummy_pixel = 0;
	/*ctx->current_fps = 300;*/
	ctx->autoflicker_en = KAL_FALSE;
	slim_video_setting(ctx);
	/*set_mirror_flip(ctx, ctx->mirror);*/

	return ERROR_NONE;
}	/* slim_video */


static kal_uint32 custom1(struct subdrv_ctx *ctx,
		MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
		MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("%s.\n", __func__);

	ctx->sensor_mode = IMGSENSOR_MODE_CUSTOM1;
	ctx->pclk = imgsensor_info.custom1.pclk;
	ctx->line_length = imgsensor_info.custom1.linelength;
	ctx->frame_length = imgsensor_info.custom1.framelength;
	ctx->min_frame_length = imgsensor_info.custom1.framelength;
	ctx->autoflicker_en = KAL_FALSE;
	custom1_setting(ctx);

	if (ctx->pdaf_mode)
		imx519_apply_LRC(ctx);

	return ERROR_NONE;
}	/* custom1 */

static kal_uint32 custom2(struct subdrv_ctx *ctx,
		MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
		MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("%s.\n", __func__);

	ctx->sensor_mode = IMGSENSOR_MODE_CUSTOM2;
	ctx->pclk = imgsensor_info.custom2.pclk;
	ctx->line_length = imgsensor_info.custom2.linelength;
	ctx->frame_length = imgsensor_info.custom2.framelength;
	ctx->min_frame_length = imgsensor_info.custom2.framelength;
	ctx->autoflicker_en = KAL_FALSE;
	custom2_setting(ctx);

	if (ctx->pdaf_mode)
		imx519_apply_LRC(ctx);

	return ERROR_NONE;
}	/* custom2 */

static kal_uint32 custom3(struct subdrv_ctx *ctx,
		MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
		MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("%s.\n", __func__);

	ctx->sensor_mode = IMGSENSOR_MODE_CUSTOM3;
	ctx->pclk = imgsensor_info.custom3.pclk;
	ctx->line_length = imgsensor_info.custom3.linelength;
	ctx->frame_length = imgsensor_info.custom3.framelength;
	ctx->min_frame_length = imgsensor_info.custom3.framelength;
	ctx->autoflicker_en = KAL_FALSE;
	custom3_setting(ctx);

	if (ctx->pdaf_mode)
		imx519_apply_LRC(ctx);

	return ERROR_NONE;
}	/* custom3 */

static int get_resolution(struct subdrv_ctx *ctx,
		MSDK_SENSOR_RESOLUTION_INFO_STRUCT *sensor_resolution)
{
	LOG_INF("E\n");
	sensor_resolution->SensorFullWidth =
		imgsensor_info.cap.grabwindow_width;
	sensor_resolution->SensorFullHeight =
		imgsensor_info.cap.grabwindow_height;

	sensor_resolution->SensorPreviewWidth =
		imgsensor_info.pre.grabwindow_width;
	sensor_resolution->SensorPreviewHeight =
		imgsensor_info.pre.grabwindow_height;

	sensor_resolution->SensorVideoWidth =
		imgsensor_info.normal_video.grabwindow_width;
	sensor_resolution->SensorVideoHeight =
		imgsensor_info.normal_video.grabwindow_height;

	sensor_resolution->SensorHighSpeedVideoWidth =
		imgsensor_info.hs_video.grabwindow_width;
	sensor_resolution->SensorHighSpeedVideoHeight =
		imgsensor_info.hs_video.grabwindow_height;

	sensor_resolution->SensorSlimVideoWidth =
		imgsensor_info.slim_video.grabwindow_width;
	sensor_resolution->SensorSlimVideoHeight =
		imgsensor_info.slim_video.grabwindow_height;

	sensor_resolution->SensorCustom1Width =
		imgsensor_info.custom1.grabwindow_width;
	sensor_resolution->SensorCustom1Height =
		imgsensor_info.custom1.grabwindow_height;

	sensor_resolution->SensorCustom2Width =
		imgsensor_info.custom2.grabwindow_width;
	sensor_resolution->SensorCustom2Height =
		imgsensor_info.custom2.grabwindow_height;

	sensor_resolution->SensorCustom3Width =
		imgsensor_info.custom3.grabwindow_width;
	sensor_resolution->SensorCustom3Height =
		imgsensor_info.custom3.grabwindow_height;

	return ERROR_NONE;
} /* get_resolution */

static int get_info(struct subdrv_ctx *ctx,
		enum MSDK_SCENARIO_ID_ENUM scenario_id,
		MSDK_SENSOR_INFO_STRUCT *sensor_info,
		MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{

	sensor_info->SensorClockPolarity = SENSOR_CLOCK_POLARITY_LOW;
	sensor_info->SensorClockFallingPolarity = SENSOR_CLOCK_POLARITY_LOW;
	sensor_info->SensorHsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
	sensor_info->SensorVsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
	sensor_info->SensorInterruptDelayLines = 4; /* not use */
	sensor_info->SensorResetActiveHigh = FALSE; /* not use */
	sensor_info->SensorResetDelayCount = 5; /* not use */

	sensor_info->SensroInterfaceType = imgsensor_info.sensor_interface_type;
	sensor_info->MIPIsensorType = imgsensor_info.mipi_sensor_type;
	sensor_info->SettleDelayMode = imgsensor_info.mipi_settle_delay_mode;
	sensor_info->SensorOutputDataFormat =
		imgsensor_info.sensor_output_dataformat;

	sensor_info->CaptureDelayFrame = imgsensor_info.cap_delay_frame;
	sensor_info->PreviewDelayFrame = imgsensor_info.pre_delay_frame;
	sensor_info->VideoDelayFrame = imgsensor_info.video_delay_frame;
	sensor_info->HighSpeedVideoDelayFrame =
		imgsensor_info.hs_video_delay_frame;
	sensor_info->SlimVideoDelayFrame =
		imgsensor_info.slim_video_delay_frame;
	sensor_info->Custom1DelayFrame = imgsensor_info.custom1_delay_frame;
	sensor_info->Custom2DelayFrame = imgsensor_info.custom2_delay_frame;
	sensor_info->Custom3DelayFrame = imgsensor_info.custom3_delay_frame;

	sensor_info->SensorMasterClockSwitch = 0; /* not use */
	sensor_info->SensorDrivingCurrent = imgsensor_info.isp_driving_current;

	sensor_info->AEShutDelayFrame = imgsensor_info.ae_shut_delay_frame;
	sensor_info->AESensorGainDelayFrame =
		imgsensor_info.ae_sensor_gain_delay_frame;
	sensor_info->AEISPGainDelayFrame =
		imgsensor_info.ae_ispGain_delay_frame;
	sensor_info->IHDR_Support = imgsensor_info.ihdr_support;
	sensor_info->IHDR_LE_FirstLine = imgsensor_info.ihdr_le_firstline;
	sensor_info->TEMPERATURE_SUPPORT = imgsensor_info.temperature_support;
	sensor_info->SensorModeNum = imgsensor_info.sensor_mode_num;
	sensor_info->PDAF_Support = 2;
	sensor_info->SensorMIPILaneNumber = imgsensor_info.mipi_lane_num;
	sensor_info->SensorClockFreq = imgsensor_info.mclk;
	sensor_info->SensorClockDividCount = 3; /* not use */
	sensor_info->SensorClockRisingCount = 0;
	sensor_info->SensorClockFallingCount = 2; /* not use */
	sensor_info->SensorPixelClockCount = 3; /* not use */
	sensor_info->SensorDataLatchCount = 2; /* not use */

	sensor_info->MIPIDataLowPwr2HighSpeedTermDelayCount = 0;
	sensor_info->MIPICLKLowPwr2HighSpeedTermDelayCount = 0;
	sensor_info->SensorWidthSampling = 0; /* 0 is default 1x */
	sensor_info->SensorHightSampling = 0; /* 0 is default 1x */
	sensor_info->SensorPacketECCOrder = 1;

	sensor_info->SensorGrabStartX_PRV = imgsensor_info.pre.startx;
	sensor_info->SensorGrabStartY_PRV = imgsensor_info.pre.starty;
	sensor_info->SensorGrabStartX_CAP = imgsensor_info.cap.startx;
	sensor_info->SensorGrabStartY_CAP = imgsensor_info.cap.starty;
	sensor_info->SensorGrabStartX_VD = imgsensor_info.normal_video.startx;
	sensor_info->SensorGrabStartY_VD = imgsensor_info.normal_video.starty;
	sensor_info->SensorGrabStartX_VD1 = imgsensor_info.hs_video.startx;
	sensor_info->SensorGrabStartY_VD1 = imgsensor_info.hs_video.starty;
	sensor_info->SensorGrabStartX_VD2 = imgsensor_info.slim_video.startx;
	sensor_info->SensorGrabStartY_VD2 = imgsensor_info.slim_video.starty;
	sensor_info->SensorGrabStartX_CST1 = imgsensor_info.custom1.startx;
	sensor_info->SensorGrabStartY_CST1 = imgsensor_info.custom1.starty;
	sensor_info->SensorGrabStartX_CST2 = imgsensor_info.custom2.startx;
	sensor_info->SensorGrabStartY_CST2 = imgsensor_info.custom2.starty;
	sensor_info->SensorGrabStartX_CST3 = imgsensor_info.custom3.startx;
	sensor_info->SensorGrabStartY_CST3 = imgsensor_info.custom3.starty;

	sensor_info->MIPIDataLowPwr2HSSettleDelayM0 =
		imgsensor_info.pre.mipi_data_lp2hs_settle_dc;
	sensor_info->MIPIDataLowPwr2HSSettleDelayM1 =
		imgsensor_info.cap.mipi_data_lp2hs_settle_dc;
	sensor_info->MIPIDataLowPwr2HSSettleDelayM2 =
		imgsensor_info.normal_video.mipi_data_lp2hs_settle_dc;
	sensor_info->MIPIDataLowPwr2HSSettleDelayM3 =
		imgsensor_info.hs_video.mipi_data_lp2hs_settle_dc;
	sensor_info->MIPIDataLowPwr2HSSettleDelayM4 =
		imgsensor_info.slim_video.mipi_data_lp2hs_settle_dc;

	switch (scenario_id) {
	case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		sensor_info->SensorGrabStartX = imgsensor_info.pre.startx;
		sensor_info->SensorGrabStartY = imgsensor_info.pre.starty;

		sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount =
			imgsensor_info.pre.mipi_data_lp2hs_settle_dc;

		break;
	case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		sensor_info->SensorGrabStartX = imgsensor_info.cap.startx;
		sensor_info->SensorGrabStartY = imgsensor_info.cap.starty;

		sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount =
			imgsensor_info.cap.mipi_data_lp2hs_settle_dc;

		break;
	case MSDK_SCENARIO_ID_VIDEO_PREVIEW:

		sensor_info->SensorGrabStartX =
			imgsensor_info.normal_video.startx;
		sensor_info->SensorGrabStartY =
			imgsensor_info.normal_video.starty;

		sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount =
			imgsensor_info.normal_video.mipi_data_lp2hs_settle_dc;

		break;
	case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
		sensor_info->SensorGrabStartX = imgsensor_info.hs_video.startx;
		sensor_info->SensorGrabStartY = imgsensor_info.hs_video.starty;

		sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount =
			imgsensor_info.hs_video.mipi_data_lp2hs_settle_dc;

		break;
	case MSDK_SCENARIO_ID_SLIM_VIDEO:
		sensor_info->SensorGrabStartX =
			imgsensor_info.slim_video.startx;
		sensor_info->SensorGrabStartY =
			imgsensor_info.slim_video.starty;

		sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount =
			imgsensor_info.slim_video.mipi_data_lp2hs_settle_dc;

		break;
	default:
		sensor_info->SensorGrabStartX = imgsensor_info.pre.startx;
		sensor_info->SensorGrabStartY = imgsensor_info.pre.starty;

		sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount =
			imgsensor_info.pre.mipi_data_lp2hs_settle_dc;
		break;
	case MSDK_SCENARIO_ID_CUSTOM1:
		sensor_info->SensorGrabStartX = imgsensor_info.custom1.startx;
		sensor_info->SensorGrabStartY = imgsensor_info.custom1.starty;

		sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount =
			imgsensor_info.custom1.mipi_data_lp2hs_settle_dc;
		break;
	case MSDK_SCENARIO_ID_CUSTOM2:
		sensor_info->SensorGrabStartX = imgsensor_info.custom2.startx;
		sensor_info->SensorGrabStartY = imgsensor_info.custom2.starty;

		sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount =
			imgsensor_info.custom2.mipi_data_lp2hs_settle_dc;
		break;
	case MSDK_SCENARIO_ID_CUSTOM3:
		sensor_info->SensorGrabStartX = imgsensor_info.custom3.startx;
		sensor_info->SensorGrabStartY = imgsensor_info.custom3.starty;

		sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount =
			imgsensor_info.custom3.mipi_data_lp2hs_settle_dc;
		break;
	}

	return ERROR_NONE;
}	/*	get_info  */


static int control(struct subdrv_ctx *ctx, enum MSDK_SCENARIO_ID_ENUM scenario_id,
			MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
			MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("scenario_id = %d\n", scenario_id);
	ctx->current_scenario_id = scenario_id;
	switch (scenario_id) {
	case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		preview(ctx, image_window, sensor_config_data);
		break;
	case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		capture(ctx, image_window, sensor_config_data);
		break;
	case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
		normal_video(ctx, image_window, sensor_config_data);
		break;
	case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
		hs_video(ctx, image_window, sensor_config_data);
		break;
	case MSDK_SCENARIO_ID_SLIM_VIDEO:
		slim_video(ctx, image_window, sensor_config_data);
		break;
	case MSDK_SCENARIO_ID_CUSTOM1:
		custom1(ctx, image_window, sensor_config_data);
		break;
	case MSDK_SCENARIO_ID_CUSTOM2:
		custom2(ctx, image_window, sensor_config_data);
		break;
	case MSDK_SCENARIO_ID_CUSTOM3:
		custom3(ctx, image_window, sensor_config_data);
		break;
	default:
		LOG_INF("Error ScenarioId setting");
		preview(ctx, image_window, sensor_config_data);
		return ERROR_INVALID_SCENARIO_ID;
	}
	return ERROR_NONE;
}	/* control(ctx) */



static kal_uint32 set_video_mode(struct subdrv_ctx *ctx, UINT16 framerate)
{
	LOG_INF("framerate = %d\n ", framerate);
	/* SetVideoMode Function should fix framerate */
	if (framerate == 0)
		/* Dynamic frame rate */
		return ERROR_NONE;
	if ((framerate == 300) && (ctx->autoflicker_en == KAL_TRUE))
		ctx->current_fps = 296;
	else if ((framerate == 150) && (ctx->autoflicker_en == KAL_TRUE))
		ctx->current_fps = 146;
	else
		ctx->current_fps = framerate;
	set_max_framerate(ctx, ctx->current_fps, 1);

	return ERROR_NONE;
}

static kal_uint32 set_auto_flicker_mode(struct subdrv_ctx *ctx,
		kal_bool enable, UINT16 framerate)
{
	LOG_INF("enable = %d, framerate = %d\n", enable, framerate);
	if (enable) /*enable auto flicker*/
		ctx->autoflicker_en = KAL_TRUE;
	else /*Cancel Auto flick*/
		ctx->autoflicker_en = KAL_FALSE;
	return ERROR_NONE;
}


static kal_uint32 set_max_framerate_by_scenario(struct subdrv_ctx *ctx,
		enum MSDK_SCENARIO_ID_ENUM scenario_id, MUINT32 framerate)
{
	kal_uint32 frame_length;

	LOG_INF("scenario_id = %d, framerate = %d\n", scenario_id, framerate);

	switch (scenario_id) {
	case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		frame_length = imgsensor_info.pre.pclk / framerate * 10
				/ imgsensor_info.pre.linelength;
		ctx->dummy_line =
			(frame_length > imgsensor_info.pre.framelength)
		? (frame_length - imgsensor_info.pre.framelength) : 0;
		ctx->frame_length =
			imgsensor_info.pre.framelength
			+ ctx->dummy_line;
		ctx->min_frame_length = ctx->frame_length;
		if (ctx->frame_length > ctx->shutter)
			set_dummy(ctx);
		break;
	case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
		if (framerate == 0)
			return ERROR_NONE;
		frame_length = imgsensor_info.normal_video.pclk /
				framerate * 10 /
				imgsensor_info.normal_video.linelength;
		ctx->dummy_line =
			(frame_length > imgsensor_info.normal_video.framelength)
		? (frame_length - imgsensor_info.normal_video.framelength)
		: 0;
		ctx->frame_length =
			imgsensor_info.normal_video.framelength
			+ ctx->dummy_line;
		ctx->min_frame_length = ctx->frame_length;
		if (ctx->frame_length > ctx->shutter)
			set_dummy(ctx);
		break;
	case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		if (ctx->current_fps != imgsensor_info.cap.max_framerate)
			LOG_INF(
				"Warning: current_fps %d fps is not support, so use cap's setting: %d fps!\n"
				, framerate
				, imgsensor_info.cap.max_framerate/10);
			frame_length = imgsensor_info.cap.pclk / framerate * 10
					/ imgsensor_info.cap.linelength;

		if (frame_length > imgsensor_info.max_frame_length) {
			LOG_INF(
				"Warning: frame_length %d > max_frame_length %d!\n"
				, frame_length
				, imgsensor_info.max_frame_length);
			break;
		}

		ctx->dummy_line =
			(frame_length > imgsensor_info.cap.framelength)
			? (frame_length - imgsensor_info.cap.framelength) : 0;
		ctx->frame_length =
			imgsensor_info.cap.framelength
			+ ctx->dummy_line;
		ctx->min_frame_length = ctx->frame_length;
		if (ctx->frame_length > ctx->shutter)
			set_dummy(ctx);
		break;
	case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
		frame_length = imgsensor_info.hs_video.pclk / framerate * 10
				/ imgsensor_info.hs_video.linelength;
		ctx->dummy_line =
			(frame_length > imgsensor_info.hs_video.framelength)
			  ? (frame_length - imgsensor_info.hs_video.framelength)
			  : 0;
		ctx->frame_length =
			imgsensor_info.hs_video.framelength
				+ ctx->dummy_line;
		ctx->min_frame_length = ctx->frame_length;
		if (ctx->frame_length > ctx->shutter)
			set_dummy(ctx);
		break;
	case MSDK_SCENARIO_ID_SLIM_VIDEO:
		frame_length = imgsensor_info.slim_video.pclk / framerate * 10
			/ imgsensor_info.slim_video.linelength;
		ctx->dummy_line =
			(frame_length > imgsensor_info.slim_video.framelength)
			? (frame_length - imgsensor_info.slim_video.framelength)
			: 0;
		ctx->frame_length =
			imgsensor_info.slim_video.framelength
			+ ctx->dummy_line;
		ctx->min_frame_length = ctx->frame_length;
		if (ctx->frame_length > ctx->shutter)
			set_dummy(ctx);
		break;
	case MSDK_SCENARIO_ID_CUSTOM1:
		frame_length = imgsensor_info.custom1.pclk / framerate * 10
				/ imgsensor_info.custom1.linelength;
		ctx->dummy_line =
			(frame_length > imgsensor_info.custom1.framelength)
			? (frame_length - imgsensor_info.custom1.framelength)
			: 0;
		ctx->frame_length =
			imgsensor_info.custom1.framelength
			+ ctx->dummy_line;
		ctx->min_frame_length = ctx->frame_length;
		if (ctx->frame_length > ctx->shutter)
			set_dummy(ctx);
		break;
	case MSDK_SCENARIO_ID_CUSTOM2:
		frame_length = imgsensor_info.custom2.pclk / framerate * 10
				/ imgsensor_info.custom2.linelength;
		ctx->dummy_line =
			(frame_length > imgsensor_info.custom2.framelength)
			? (frame_length - imgsensor_info.custom2.framelength)
			: 0;
		ctx->frame_length =
			imgsensor_info.custom2.framelength
			+ ctx->dummy_line;
		ctx->min_frame_length = ctx->frame_length;
		if (ctx->frame_length > ctx->shutter)
			set_dummy(ctx);
		break;
	case MSDK_SCENARIO_ID_CUSTOM3:
		frame_length = imgsensor_info.custom3.pclk / framerate * 10
				/ imgsensor_info.custom3.linelength;
		ctx->dummy_line =
			(frame_length > imgsensor_info.custom3.framelength)
			? (frame_length - imgsensor_info.custom3.framelength)
			: 0;
		ctx->frame_length =
			imgsensor_info.custom3.framelength
			+ ctx->dummy_line;
		ctx->min_frame_length = ctx->frame_length;
		if (ctx->frame_length > ctx->shutter)
			set_dummy(ctx);
		break;

	default:  /*coding with  preview scenario by default*/
		frame_length = imgsensor_info.pre.pclk / framerate * 10
			/ imgsensor_info.pre.linelength;
		ctx->dummy_line =
			(frame_length > imgsensor_info.pre.framelength)
			? (frame_length - imgsensor_info.pre.framelength) : 0;
		ctx->frame_length =
			imgsensor_info.pre.framelength + ctx->dummy_line;
		ctx->min_frame_length = ctx->frame_length;
		if (ctx->frame_length > ctx->shutter)
			set_dummy(ctx);
		LOG_INF("error scenario_id = %d, we use preview scenario\n",
			scenario_id);
		break;
	}
	return ERROR_NONE;
}


static kal_uint32 get_default_framerate_by_scenario(struct subdrv_ctx *ctx,
		enum MSDK_SCENARIO_ID_ENUM scenario_id, MUINT32 *framerate)
{

	switch (scenario_id) {
	case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		*framerate = imgsensor_info.pre.max_framerate;
		break;
	case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
		*framerate = imgsensor_info.normal_video.max_framerate;
		break;
	case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		*framerate = imgsensor_info.cap.max_framerate;
		break;
	case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
		*framerate = imgsensor_info.hs_video.max_framerate;
		break;
	case MSDK_SCENARIO_ID_SLIM_VIDEO:
		*framerate = imgsensor_info.slim_video.max_framerate;
		break;
	case MSDK_SCENARIO_ID_CUSTOM1:
		*framerate = imgsensor_info.custom1.max_framerate;
		break;
	case MSDK_SCENARIO_ID_CUSTOM2:
		*framerate = imgsensor_info.custom2.max_framerate;
		break;
	case MSDK_SCENARIO_ID_CUSTOM3:
		*framerate = imgsensor_info.custom3.max_framerate;
		break;
	default:
		break;
	}

	return ERROR_NONE;
}

static kal_uint32 set_test_pattern_mode(struct subdrv_ctx *ctx, kal_bool enable)
{
	LOG_INF("enable: %d\n", enable);

	if (enable)
		write_cmos_sensor_8(ctx, 0x0601, 0x0002); /*100% Color bar*/
	else
		write_cmos_sensor_8(ctx, 0x0601, 0x0000); /*No pattern*/

	ctx->test_pattern = enable;
	return ERROR_NONE;
}
static kal_uint32 imx519_ana_gain_table[] = {
	100000,
	100098,
	100196,
	100294,
	100392,
	100491,
	100589,
	100688,
	100787,
	100887,
	100986,
	101086,
	101186,
	101286,
	101386,
	101487,
	101587,
	101688,
	101789,
	101891,
	101992,
	102094,
	102196,
	102298,
	102400,
	102503,
	102605,
	102708,
	102811,
	102915,
	103018,
	103122,
	103226,
	103330,
	103434,
	103539,
	103644,
	103749,
	103854,
	103959,
	104065,
	104171,
	104277,
	104383,
	104490,
	104597,
	104703,
	104811,
	104918,
	105026,
	105133,
	105242,
	105350,
	105458,
	105567,
	105676,
	105785,
	105895,
	106004,
	106114,
	106224,
	106334,
	106445,
	106556,
	106667,
	106778,
	106889,
	107001,
	107113,
	107225,
	107338,
	107450,
	107563,
	107676,
	107789,
	107903,
	108017,
	108131,
	108245,
	108360,
	108475,
	108590,
	108705,
	108820,
	108936,
	109052,
	109168,
	109285,
	109402,
	109519,
	109636,
	109753,
	109871,
	109989,
	110108,
	110226,
	110345,
	110464,
	110583,
	110703,
	110823,
	110943,
	111063,
	111183,
	111304,
	111425,
	111547,
	111668,
	111790,
	111913,
	112035,
	112158,
	112281,
	112404,
	112527,
	112651,
	112775,
	112900,
	113024,
	113149,
	113274,
	113400,
	113525,
	113651,
	113778,
	113904,
	114031,
	114158,
	114286,
	114413,
	114541,
	114670,
	114798,
	114927,
	115056,
	115186,
	115315,
	115445,
	115576,
	115706,
	115837,
	115968,
	116100,
	116232,
	116364,
	116496,
	116629,
	116762,
	116895,
	117029,
	117162,
	117297,
	117431,
	117566,
	117701,
	117837,
	117972,
	118108,
	118245,
	118382,
	118519,
	118656,
	118794,
	118931,
	119070,
	119208,
	119347,
	119487,
	119626,
	119766,
	119906,
	120047,
	120188,
	120329,
	120471,
	120612,
	120755,
	120897,
	121040,
	121183,
	121327,
	121471,
	121615,
	121760,
	121905,
	122050,
	122196,
	122342,
	122488,
	122635,
	122782,
	122929,
	123077,
	123225,
	123373,
	123522,
	123671,
	123821,
	123971,
	124121,
	124272,
	124423,
	124574,
	124726,
	124878,
	125031,
	125183,
	125337,
	125490,
	125644,
	125799,
	125953,
	126108,
	126264,
	126420,
	126576,
	126733,
	126890,
	127047,
	127205,
	127363,
	127522,
	127681,
	127840,
	128000,
	128160,
	128321,
	128482,
	128643,
	128805,
	128967,
	129130,
	129293,
	129456,
	129620,
	129785,
	129949,
	130114,
	130280,
	130446,
	130612,
	130779,
	130946,
	131114,
	131282,
	131451,
	131620,
	131789,
	131959,
	132129,
	132300,
	132471,
	132642,
	132815,
	132987,
	133160,
	133333,
	133507,
	133681,
	133856,
	134031,
	134207,
	134383,
	134560,
	134737,
	134914,
	135092,
	135271,
	135450,
	135629,
	135809,
	135989,
	136170,
	136352,
	136533,
	136716,
	136898,
	137082,
	137265,
	137450,
	137634,
	137820,
	138005,
	138192,
	138378,
	138566,
	138753,
	138942,
	139130,
	139320,
	139510,
	139700,
	139891,
	140082,
	140274,
	140466,
	140659,
	140853,
	141047,
	141241,
	141436,
	141632,
	141828,
	142025,
	142222,
	142420,
	142618,
	142817,
	143017,
	143217,
	143417,
	143619,
	143820,
	144023,
	144225,
	144429,
	144633,
	144837,
	145042,
	145248,
	145455,
	145661,
	145869,
	146077,
	146286,
	146495,
	146705,
	146915,
	147126,
	147338,
	147550,
	147763,
	147977,
	148191,
	148406,
	148621,
	148837,
	149054,
	149271,
	149489,
	149708,
	149927,
	150147,
	150367,
	150588,
	150810,
	151032,
	151256,
	151479,
	151704,
	151929,
	152155,
	152381,
	152608,
	152836,
	153064,
	153293,
	153523,
	153754,
	153985,
	154217,
	154449,
	154683,
	154917,
	155152,
	155387,
	155623,
	155860,
	156098,
	156336,
	156575,
	156815,
	157055,
	157296,
	157538,
	157781,
	158025,
	158269,
	158514,
	158760,
	159006,
	159253,
	159502,
	159750,
	160000,
	160250,
	160502,
	160754,
	161006,
	161260,
	161514,
	161769,
	162025,
	162282,
	162540,
	162798,
	163057,
	163317,
	163578,
	163840,
	164103,
	164366,
	164630,
	164895,
	165161,
	165428,
	165696,
	165964,
	166234,
	166504,
	166775,
	167047,
	167320,
	167594,
	167869,
	168144,
	168421,
	168699,
	168977,
	169256,
	169536,
	169818,
	170100,
	170383,
	170667,
	170952,
	171237,
	171524,
	171812,
	172101,
	172391,
	172681,
	172973,
	173266,
	173559,
	173854,
	174150,
	174446,
	174744,
	175043,
	175342,
	175643,
	175945,
	176248,
	176552,
	176857,
	177163,
	177470,
	177778,
	178087,
	178397,
	178709,
	179021,
	179335,
	179649,
	179965,
	180282,
	180600,
	180919,
	181239,
	181560,
	181883,
	182206,
	182531,
	182857,
	183184,
	183513,
	183842,
	184173,
	184505,
	184838,
	185172,
	185507,
	185844,
	186182,
	186521,
	186861,
	187203,
	187546,
	187890,
	188235,
	188582,
	188930,
	189279,
	189630,
	189981,
	190335,
	190689,
	191045,
	191402,
	191760,
	192120,
	192481,
	192844,
	193208,
	193573,
	193939,
	194307,
	194677,
	195048,
	195420,
	195793,
	196169,
	196545,
	196923,
	197303,
	197683,
	198066,
	198450,
	198835,
	199222,
	199610,
	200000,
	200391,
	200784,
	201179,
	201575,
	201972,
	202372,
	202772,
	203175,
	203579,
	203984,
	204391,
	204800,
	205210,
	205622,
	206036,
	206452,
	206869,
	207287,
	207708,
	208130,
	208554,
	208980,
	209407,
	209836,
	210267,
	210700,
	211134,
	211570,
	212008,
	212448,
	212890,
	213333,
	213779,
	214226,
	214675,
	215126,
	215579,
	216034,
	216490,
	216949,
	217410,
	217872,
	218337,
	218803,
	219272,
	219742,
	220215,
	220690,
	221166,
	221645,
	222126,
	222609,
	223094,
	223581,
	224070,
	224561,
	225055,
	225551,
	226049,
	226549,
	227051,
	227556,
	228062,
	228571,
	229083,
	229596,
	230112,
	230631,
	231151,
	231674,
	232200,
	232727,
	233257,
	233790,
	234325,
	234862,
	235402,
	235945,
	236490,
	237037,
	237587,
	238140,
	238695,
	239252,
	239813,
	240376,
	240941,
	241509,
	242080,
	242654,
	243230,
	243810,
	244391,
	244976,
	245564,
	246154,
	246747,
	247343,
	247942,
	248544,
	249148,
	249756,
	250367,
	250980,
	251597,
	252217,
	252840,
	253465,
	254094,
	254726,
	255362,
	256000,
	256642,
	257286,
	257935,
	258586,
	259241,
	259898,
	260560,
	261224,
	261893,
	262564,
	263239,
	263918,
	264599,
	265285,
	265974,
	266667,
	267363,
	268063,
	268766,
	269474,
	270185,
	270899,
	271618,
	272340,
	273067,
	273797,
	274531,
	275269,
	276011,
	276757,
	277507,
	278261,
	279019,
	279781,
	280548,
	281319,
	282094,
	282873,
	283657,
	284444,
	285237,
	286034,
	286835,
	287640,
	288451,
	289266,
	290085,
	290909,
	291738,
	292571,
	293410,
	294253,
	295101,
	295954,
	296812,
	297674,
	298542,
	299415,
	300293,
	301176,
	302065,
	302959,
	303858,
	304762,
	305672,
	306587,
	307508,
	308434,
	309366,
	310303,
	311246,
	312195,
	313150,
	314110,
	315077,
	316049,
	317028,
	318012,
	319003,
	320000,
	321003,
	322013,
	323028,
	324051,
	325079,
	326115,
	327157,
	328205,
	329260,
	330323,
	331392,
	332468,
	333550,
	334641,
	335738,
	336842,
	337954,
	339073,
	340199,
	341333,
	342475,
	343624,
	344781,
	345946,
	347119,
	348299,
	349488,
	350685,
	351890,
	353103,
	354325,
	355556,
	356794,
	358042,
	359298,
	360563,
	361837,
	363121,
	364413,
	365714,
	367025,
	368345,
	369675,
	371014,
	372364,
	373723,
	375092,
	376471,
	377860,
	379259,
	380669,
	382090,
	383521,
	384962,
	386415,
	387879,
	389354,
	390840,
	392337,
	393846,
	395367,
	396899,
	398444,
	400000,
	401569,
	403150,
	404743,
	406349,
	407968,
	409600,
	411245,
	412903,
	414575,
	416260,
	417959,
	419672,
	421399,
	423140,
	424896,
	426667,
	428452,
	430252,
	432068,
	433898,
	435745,
	437607,
	439485,
	441379,
	443290,
	445217,
	447162,
	449123,
	451101,
	453097,
	455111,
	457143,
	459193,
	461261,
	463348,
	465455,
	467580,
	469725,
	471889,
	474074,
	476279,
	478505,
	480751,
	483019,
	485308,
	487619,
	489952,
	492308,
	494686,
	497087,
	499512,
	501961,
	504433,
	506931,
	509453,
	512000,
	514573,
	517172,
	519797,
	522449,
	525128,
	527835,
	530570,
	533333,
	536126,
	538947,
	541799,
	544681,
	547594,
	550538,
	553514,
	556522,
	559563,
	562637,
	565746,
	568889,
	572067,
	575281,
	578531,
	581818,
	585143,
	588506,
	591908,
	595349,
	598830,
	602353,
	605917,
	609524,
	613174,
	616867,
	620606,
	624390,
	628221,
	632099,
	636025,
	640000,
	644025,
	648101,
	652229,
	656410,
	660645,
	664935,
	669281,
	673684,
	678146,
	682667,
	687248,
	691892,
	696599,
	701370,
	706207,
	711111,
	716084,
	721127,
	726241,
	731429,
	736691,
	742029,
	747445,
	752941,
	758519,
	764179,
	769925,
	775758,
	781679,
	787692,
	793798,
	800000,
	806299,
	812698,
	819200,
	825806,
	832520,
	839344,
	846281,
	853333,
	860504,
	867797,
	875214,
	882759,
	890435,
	898246,
	906195,
	914286,
	922523,
	930909,
	939450,
	948148,
	957009,
	966038,
	975238,
	984615,
	994175,
	1003922,
	1013861,
	1024000,
	1034343,
	1044898,
	1055670,
	1066667,
	1077895,
	1089362,
	1101075,
	1113043,
	1125275,
	1137778,
	1150562,
	1163636,
	1177011,
	1190698,
	1204706,
	1219048,
	1233735,
	1248780,
	1264198,
	1280000,
	1296203,
	1312821,
	1329870,
	1347368,
	1365333,
	1383784,
	1402740,
	1422222,
	1442254,
	1462857,
	1484058,
	1505882,
	1528358,
	1551515,
	1575385,
	1600000,
};

static kal_int32 get_sensor_temperature(struct subdrv_ctx *ctx)
{
	UINT8 temperature = 0;
	INT32 temperature_convert = 0;

	temperature = read_cmos_sensor_8(ctx, 0x013a);

	if (temperature >= 0x0 && temperature <= 0x60)
		temperature_convert = temperature;
	else if (temperature >= 0x61 && temperature <= 0x7F)
		temperature_convert = 97;
	else if (temperature >= 0x80 && temperature <= 0xE2)
		temperature_convert = -30;
	else
		temperature_convert = (INT8)temperature | 0xFFFFFF0;

/* LOG_INF("temp_c(%d), read_reg(%d)\n", temperature_convert, temperature); */

	return temperature_convert;
}
static int feature_control(struct subdrv_ctx *ctx, MSDK_SENSOR_FEATURE_ENUM feature_id,
				 UINT8 *feature_para, UINT32 *feature_para_len)
{
	UINT16 *feature_return_para_16 = (UINT16 *) feature_para;
	UINT16 *feature_data_16 = (UINT16 *) feature_para;
	UINT32 *feature_return_para_32 = (UINT32 *) feature_para;
	UINT32 *feature_data_32 = (UINT32 *) feature_para;
	unsigned long long *feature_data = (unsigned long long *) feature_para;
	/* unsigned long long *feature_return_para
	 *  = (unsigned long long *) feature_para;
	 */
	struct SET_PD_BLOCK_INFO_T *PDAFinfo;
	struct SENSOR_WINSIZE_INFO_STRUCT *wininfo;
	struct SENSOR_VC_INFO_STRUCT *pvcinfo;
	/* SET_SENSOR_AWB_GAIN *pSetSensorAWB
	 *  = (SET_SENSOR_AWB_GAIN *)feature_para;
	 */
	MSDK_SENSOR_REG_INFO_STRUCT *sensor_reg_data
		= (MSDK_SENSOR_REG_INFO_STRUCT *) feature_para;

	/*LOG_INF("feature_id = %d\n", feature_id);*/
	switch (feature_id) {
	case SENSOR_FEATURE_GET_ANA_GAIN_TABLE:
		if ((void *)(uintptr_t) (*(feature_data + 1)) == NULL) {
			*(feature_data + 0) =
				sizeof(imx519_ana_gain_table);
		} else {
			memcpy((void *)(uintptr_t) (*(feature_data + 1)),
			(void *)imx519_ana_gain_table,
			sizeof(imx519_ana_gain_table));
		}
		break;
	case SENSOR_FEATURE_GET_GAIN_RANGE_BY_SCENARIO:
		*(feature_data + 1) = imgsensor_info.min_gain;
		*(feature_data + 2) = imgsensor_info.max_gain;
		break;
	case SENSOR_FEATURE_GET_BASE_GAIN_ISO_AND_STEP:
		*(feature_data + 0) = imgsensor_info.min_gain_iso;
		*(feature_data + 1) = imgsensor_info.gain_step;
		*(feature_data + 2) = imgsensor_info.gain_type;
		break;
	case SENSOR_FEATURE_GET_MIN_SHUTTER_BY_SCENARIO:
		*(feature_data + 1) = imgsensor_info.min_shutter;
		break;
	case SENSOR_FEATURE_GET_OFFSET_TO_START_OF_EXPOSURE:
		*(MUINT32 *)(uintptr_t)(*(feature_data + 1)) = 3000000;
		break;
	case SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ_BY_SCENARIO:
		switch (*feature_data) {
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
				= imgsensor_info.cap.pclk;
			break;
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
				= imgsensor_info.normal_video.pclk;
			break;
		case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
				= imgsensor_info.hs_video.pclk;
			break;
		case MSDK_SCENARIO_ID_CUSTOM1:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
				= imgsensor_info.custom1.pclk;
			break;
		case MSDK_SCENARIO_ID_CUSTOM2:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
				= imgsensor_info.custom2.pclk;
			break;
		case MSDK_SCENARIO_ID_CUSTOM3:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
				= imgsensor_info.custom3.pclk;
			break;
		case MSDK_SCENARIO_ID_SLIM_VIDEO:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
				= imgsensor_info.slim_video.pclk;
			break;
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		default:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
				= imgsensor_info.pre.pclk;
			break;
		}
		break;
	case SENSOR_FEATURE_GET_PERIOD_BY_SCENARIO:
		switch (*feature_data) {
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
			= (imgsensor_info.cap.framelength << 16)
				+ imgsensor_info.cap.linelength;
			break;
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
			= (imgsensor_info.normal_video.framelength << 16)
				+ imgsensor_info.normal_video.linelength;
			break;
		case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
			= (imgsensor_info.hs_video.framelength << 16)
				+ imgsensor_info.hs_video.linelength;
			break;
		case MSDK_SCENARIO_ID_SLIM_VIDEO:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
			= (imgsensor_info.slim_video.framelength << 16)
				+ imgsensor_info.slim_video.linelength;
			break;
		case MSDK_SCENARIO_ID_CUSTOM1:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
			= (imgsensor_info.custom1.framelength << 16)
				+ imgsensor_info.custom1.linelength;
			break;
		case MSDK_SCENARIO_ID_CUSTOM2:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
			= (imgsensor_info.custom2.framelength << 16)
				+ imgsensor_info.custom2.linelength;
			break;
		case MSDK_SCENARIO_ID_CUSTOM3:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
			= (imgsensor_info.custom3.framelength << 16)
				+ imgsensor_info.custom3.linelength;
			break;
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		default:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
			= (imgsensor_info.pre.framelength << 16)
				+ imgsensor_info.pre.linelength;
			break;
		}
		break;
	case SENSOR_FEATURE_GET_PERIOD:
		*feature_return_para_16++ = ctx->line_length;
		*feature_return_para_16 = ctx->frame_length;
		*feature_para_len = 4;
		break;
	case SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ:
		*feature_return_para_32 = ctx->pclk;
		*feature_para_len = 4;
		break;
	case SENSOR_FEATURE_SET_ESHUTTER:
		 set_shutter(ctx, *feature_data);
		break;
	case SENSOR_FEATURE_SET_NIGHTMODE:
		 /* night_mode((BOOL) *feature_data); */
		break;
	#ifdef VENDOR_EDIT
	case SENSOR_FEATURE_CHECK_MODULE_ID:
		*feature_return_para_32 = imgsensor_info.module_id;
		break;
	#endif
	case SENSOR_FEATURE_SET_GAIN:
		set_gain(ctx, (UINT16) *feature_data);
		break;
	case SENSOR_FEATURE_SET_FLASHLIGHT:
		break;
	case SENSOR_FEATURE_SET_ISP_MASTER_CLOCK_FREQ:
		break;
	case SENSOR_FEATURE_SET_REGISTER:
		write_cmos_sensor_8(ctx, sensor_reg_data->RegAddr,
				    sensor_reg_data->RegData);
		break;
	case SENSOR_FEATURE_GET_REGISTER:
		sensor_reg_data->RegData =
			read_cmos_sensor_8(ctx, sensor_reg_data->RegAddr);
		break;
	case SENSOR_FEATURE_GET_LENS_DRIVER_ID:
		/*get the lens driver ID from EEPROM
		 * or just return LENS_DRIVER_ID_DO_NOT_CARE
		 * if EEPROM does not exist in camera module.
		 */
		*feature_return_para_32 = LENS_DRIVER_ID_DO_NOT_CARE;
		*feature_para_len = 4;
		break;
	case SENSOR_FEATURE_SET_VIDEO_MODE:
		set_video_mode(ctx, *feature_data);
		break;
	case SENSOR_FEATURE_CHECK_SENSOR_ID:
		get_imgsensor_id(ctx, feature_return_para_32);
		break;
	case SENSOR_FEATURE_SET_AUTO_FLICKER_MODE:
		set_auto_flicker_mode(ctx, (BOOL)*feature_data_16,
				      *(feature_data_16+1));
		break;
	case SENSOR_FEATURE_SET_MAX_FRAME_RATE_BY_SCENARIO:
		 set_max_framerate_by_scenario(ctx,
				(enum MSDK_SCENARIO_ID_ENUM)*feature_data,
				*(feature_data+1));
		break;
	case SENSOR_FEATURE_GET_DEFAULT_FRAME_RATE_BY_SCENARIO:
		 get_default_framerate_by_scenario(ctx,
				(enum MSDK_SCENARIO_ID_ENUM)*(feature_data),
				(MUINT32 *)(uintptr_t)(*(feature_data+1)));
		break;
	case SENSOR_FEATURE_GET_PDAF_DATA:
		LOG_INF("SENSOR_FEATURE_GET_PDAF_DATA\n");
		break;
	case SENSOR_FEATURE_SET_TEST_PATTERN:
		set_test_pattern_mode(ctx, (BOOL)*feature_data);
		break;
	case SENSOR_FEATURE_GET_TEST_PATTERN_CHECKSUM_VALUE:
		/* for factory mode auto testing */
		*feature_return_para_32 = imgsensor_info.checksum_value;
		*feature_para_len = 4;
		break;
	case SENSOR_FEATURE_SET_FRAMERATE:
		LOG_INF("current fps :%d\n", (UINT32)*feature_data_32);
		ctx->current_fps = *feature_data_32;
		break;
	case SENSOR_FEATURE_SET_HDR:
		LOG_INF("ihdr enable :%d\n", (BOOL)*feature_data_32);
		ctx->ihdr_mode = *feature_data_32;
		break;
	case SENSOR_FEATURE_GET_CROP_INFO:
		LOG_INF("SENSOR_FEATURE_GET_CROP_INFO scenarioId:%d\n",
			(UINT32)*feature_data);
		wininfo =
	(struct SENSOR_WINSIZE_INFO_STRUCT *)(uintptr_t)(*(feature_data+1));

		switch (*feature_data_32) {
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
			memcpy((void *)wininfo,
				(void *)&imgsensor_winsize_info[1],
				sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
			break;
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			memcpy((void *)wininfo,
				(void *)&imgsensor_winsize_info[2],
				sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
			break;
		case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
			memcpy((void *)wininfo,
			(void *)&imgsensor_winsize_info[3],
			sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
			break;
		case MSDK_SCENARIO_ID_SLIM_VIDEO:
			memcpy((void *)wininfo,
			(void *)&imgsensor_winsize_info[4],
			sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
			break;
		case MSDK_SCENARIO_ID_CUSTOM1:
			memcpy((void *)wininfo,
			(void *)&imgsensor_winsize_info[5],
			sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
			break;
		case MSDK_SCENARIO_ID_CUSTOM2:
			memcpy((void *)wininfo,
			(void *)&imgsensor_winsize_info[6],
			sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
			break;
		case MSDK_SCENARIO_ID_CUSTOM3:
			memcpy((void *)wininfo,
			(void *)&imgsensor_winsize_info[7],
			sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
			break;
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		default:
			memcpy((void *)wininfo,
			(void *)&imgsensor_winsize_info[0],
			sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
			break;
		}
		break;
	case SENSOR_FEATURE_GET_PDAF_INFO:
		LOG_INF("SENSOR_FEATURE_GET_PDAF_INFO scenarioId:%d\n",
			(UINT16) *feature_data);
		PDAFinfo =
		  (struct SET_PD_BLOCK_INFO_T *)(uintptr_t)(*(feature_data+1));
		switch (*feature_data) {
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
			break;
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			break;
		case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
		case MSDK_SCENARIO_ID_SLIM_VIDEO:

		default:
			break;
		}
		break;
	case SENSOR_FEATURE_GET_SENSOR_PDAF_CAPACITY:
		LOG_INF(
		"SENSOR_FEATURE_GET_SENSOR_PDAF_CAPACITY scenarioId:%d\n",
			(UINT16) *feature_data);
		/*PDAF capacity enable or not, 2p8 only full size support PDAF*/
		switch (*feature_data) {
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
			*(MUINT32 *)(uintptr_t)(*(feature_data+1)) = 1;
			break;
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			/* video & capture use same setting */
			*(MUINT32 *)(uintptr_t)(*(feature_data+1)) = 1;
			break;
		case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
			*(MUINT32 *)(uintptr_t)(*(feature_data+1)) = 0;
			break;
		case MSDK_SCENARIO_ID_SLIM_VIDEO:
			*(MUINT32 *)(uintptr_t)(*(feature_data+1)) = 0;
			break;
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
			*(MUINT32 *)(uintptr_t)(*(feature_data+1)) = 1;
			break;
		case MSDK_SCENARIO_ID_CUSTOM1:
			*(MUINT32 *)(uintptr_t)(*(feature_data+1)) = 0;
			break;
		default:
			*(MUINT32 *)(uintptr_t)(*(feature_data+1)) = 0;
			break;
		}
		break;
	case SENSOR_FEATURE_GET_TEMPERATURE_VALUE:
		*feature_return_para_32 = get_sensor_temperature(ctx);
		*feature_para_len = 4;
		break;
	case SENSOR_FEATURE_GET_PDAF_REG_SETTING:
		LOG_INF("SENSOR_FEATURE_GET_PDAF_REG_SETTING %d",
			(*feature_para_len));
		imx519_get_pdaf_reg_setting(ctx, (*feature_para_len) / sizeof(UINT32)
					   , feature_data_16);
		break;
	case SENSOR_FEATURE_SET_PDAF_REG_SETTING:
		/*LOG_INF("SENSOR_FEATURE_SET_PDAF_REG_SETTING %d",
		 *	(*feature_para_len));
		 */
		imx519_set_pdaf_reg_setting(ctx, (*feature_para_len) / sizeof(UINT32)
					   , feature_data_16);
		break;
	case SENSOR_FEATURE_SET_PDAF:
		LOG_INF("PDAF mode :%d\n", *feature_data_16);
		ctx->pdaf_mode = *feature_data_16;
		break;
	case SENSOR_FEATURE_SET_IHDR_SHUTTER_GAIN:
		LOG_INF("SENSOR_SET_SENSOR_IHDR LE=%d, SE=%d, Gain=%d\n",
			(UINT16)*feature_data,
			(UINT16)*(feature_data+1),
			(UINT16)*(feature_data+2));
		break;
	case SENSOR_FEATURE_SET_SHUTTER_FRAME_TIME:
		set_shutter_frame_length(ctx, (UINT16) (*feature_data),
					(UINT16) (*(feature_data + 1)),
					(BOOL) (*(feature_data + 2)));
		break;
	case SENSOR_FEATURE_GET_FRAME_CTRL_INFO_BY_SCENARIO:
		/*
		 * 1, if driver support new sw frame sync
		 * set_shutter_frame_length(ctx) support third para auto_extend_en
		 */
		*(feature_data + 1) = 1;
		/* margin info by scenario */
		*(feature_data + 2) = imgsensor_info.margin;
		break;
	case SENSOR_FEATURE_SET_AWB_GAIN:
		break;
	case SENSOR_FEATURE_SET_HDR_SHUTTER:
		LOG_INF("SENSOR_FEATURE_SET_HDR_SHUTTER LE=%d, SE=%d\n",
			(UINT16)*feature_data, (UINT16)*(feature_data+1));
		break;
	case SENSOR_FEATURE_SET_STREAMING_SUSPEND:
		LOG_INF("SENSOR_FEATURE_SET_STREAMING_SUSPEND\n");
		streaming_control(ctx, KAL_FALSE);
		break;
	case SENSOR_FEATURE_SET_STREAMING_RESUME:
		LOG_INF("SENSOR_FEATURE_SET_STREAMING_RESUME, shutter:%llu\n",
			*feature_data);
		if (*feature_data != 0)
			set_shutter(ctx, *feature_data);
		streaming_control(ctx, KAL_TRUE);
		break;
	case SENSOR_FEATURE_GET_AE_FRAME_MODE_FOR_LE:
		memcpy(feature_return_para_32,
		&ctx->ae_frm_mode, sizeof(struct IMGSENSOR_AE_FRM_MODE));
		break;
	case SENSOR_FEATURE_GET_AE_EFFECTIVE_FRAME_FOR_LE:
		*feature_return_para_32 = ctx->current_ae_effective_frame;
		break;
	case SENSOR_FEATURE_GET_MIPI_PIXEL_RATE:
	{
		switch (*feature_data) {
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
				= imgsensor_info.cap.mipi_pixel_rate;
			break;
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
				= imgsensor_info.normal_video.mipi_pixel_rate;
			break;
		case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
				= imgsensor_info.hs_video.mipi_pixel_rate;
			break;
		case MSDK_SCENARIO_ID_CUSTOM1:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
				= imgsensor_info.custom1.mipi_pixel_rate;
			break;
		case MSDK_SCENARIO_ID_CUSTOM2:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
				= imgsensor_info.custom2.mipi_pixel_rate;
			break;
		case MSDK_SCENARIO_ID_CUSTOM3:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
				= imgsensor_info.custom3.mipi_pixel_rate;
			break;
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		default:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
				= imgsensor_info.pre.mipi_pixel_rate;
			break;
		}
	}
	break;

	case SENSOR_FEATURE_GET_VC_INFO:
		LOG_INF("SENSOR_FEATURE_GET_VC_INFO %d\n",
			(UINT16)*feature_data);
		pvcinfo =
		 (struct SENSOR_VC_INFO_STRUCT *)(uintptr_t)(*(feature_data+1));
		switch (*feature_data_32) {
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
			memcpy((void *)pvcinfo, (void *)&SENSOR_VC_INFO[1],
				sizeof(struct SENSOR_VC_INFO_STRUCT));
			break;
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			memcpy((void *)pvcinfo, (void *)&SENSOR_VC_INFO[2],
				sizeof(struct SENSOR_VC_INFO_STRUCT));
			break;
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
			memcpy((void *)pvcinfo, (void *)&SENSOR_VC_INFO[0],
				sizeof(struct SENSOR_VC_INFO_STRUCT));
			break;
		case MSDK_SCENARIO_ID_CUSTOM1:
			memcpy((void *)pvcinfo, (void *)&SENSOR_VC_INFO[1],
				sizeof(struct SENSOR_VC_INFO_STRUCT));
			break;
		case MSDK_SCENARIO_ID_CUSTOM2:
			memcpy((void *)pvcinfo, (void *)&SENSOR_VC_INFO[1],
				sizeof(struct SENSOR_VC_INFO_STRUCT));
			break;
		case MSDK_SCENARIO_ID_CUSTOM3:
			memcpy((void *)pvcinfo, (void *)&SENSOR_VC_INFO[0],
				sizeof(struct SENSOR_VC_INFO_STRUCT));
			break;
		default:
			break;
		}
	default:
		break;
	}

	return ERROR_NONE;
} /* feature_control(ctx) */


static struct subdrv_pw_seq_entry pw_seq[] = {
	{HW_ID_MCLK, 24, 0},
	{HW_ID_PDN, 0, 0},
	{HW_ID_RST, 0, 0},
	{HW_ID_AVDD, 2800000, 0},
	{HW_ID_DVDD, 1200000, 0},
	{HW_ID_DOVDD, 1800000, 1},
	{HW_ID_MCLK_DRIVING_CURRENT, 4, 5},
	{HW_ID_PDN, 1, 0},
	{HW_ID_RST, 1, 1},
};


static const struct subdrv_ctx defctx = {

	.ana_gain_def = 0x100,
	.ana_gain_max = 1024,
	.ana_gain_min = 64,
	.ana_gain_step = 1,
	.exposure_def = 0x3D0,
	.exposure_max = 0xffff - 32,
	.exposure_min = 2,
	.exposure_step = 1,
	.is_hflip = 1,
	.is_vflip = 1,
	.max_frame_length = 0xffff,

	.mirror = IMAGE_HV_MIRROR,	/* mirrorflip information */
	.sensor_mode = IMGSENSOR_MODE_INIT,
	/* IMGSENSOR_MODE enum value,record current sensor mode,such as:
	 * INIT, Preview, Capture, Video,High Speed Video, Slim Video
	 */
	.shutter = 0x3D0,	/* current shutter */
	.gain = 0x100,		/* current gain */
	.dummy_pixel = 0,	/* current dummypixel */
	.dummy_line = 0,	/* current dummyline */
	.current_fps = 300,
	.autoflicker_en = KAL_FALSE,
	.test_pattern = KAL_FALSE,
	.current_scenario_id = MSDK_SCENARIO_ID_CAMERA_PREVIEW,
	.ihdr_mode = 0, /* sensor need support LE, SE with HDR feature */
	.i2c_write_id = 0x34, /* record current sensor's i2c write id */
	.current_ae_effective_frame = 2,
};

static int init_ctx(struct subdrv_ctx *ctx,
		struct i2c_client *i2c_client, u8 i2c_write_id)
{
	memcpy(ctx, &defctx, sizeof(*ctx));
	ctx->i2c_client = i2c_client;
	ctx->i2c_write_id = i2c_write_id;
	return 0;
}

static int get_temp(struct subdrv_ctx *ctx, int *temp)
{
	*temp = get_sensor_temperature(ctx) * 1000;
	return 0;
}

static struct subdrv_ops ops = {
	.get_id = get_imgsensor_id,
	.init_ctx = init_ctx,
	.open = open,
	.get_info = get_info,
	.get_resolution = get_resolution,
	.control = control,
	.feature_control = feature_control,
	.close = close,
	.get_temp = get_temp,
};

const struct subdrv_entry imx519_mipi_raw_entry = {
	.name = "imx519_mipi_raw",
	.id = IMX519_SENSOR_ID,
	.pw_seq = pw_seq,
	.pw_seq_cnt = ARRAY_SIZE(pw_seq),
	.ops = &ops,
};

