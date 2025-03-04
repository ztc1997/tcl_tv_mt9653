/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */
#ifndef _MTK_DRM_GOP_PLANE_H_
#define _MTK_DRM_GOP_PLANE_H_

#include "mtk_tv_drm_kms.h"

struct drm_prop_enum_list gop_hmirror_enum_list[] = {
	{.type = MTK_DRM_HMIRROR_DISABLE, .name = MTK_PLANE_PROP_DISABLE},
	{.type = MTK_DRM_HMIRROR_ENABLE, .name = MTK_PLANE_PROP_ENABLE},
};

struct drm_prop_enum_list gop_vmirror_enum_list[] = {
	{.type = MTK_DRM_VMIRROR_DISABLE, .name = MTK_PLANE_PROP_DISABLE},
	{.type = MTK_DRM_VMIRROR_ENABLE, .name = MTK_PLANE_PROP_ENABLE},
};

struct drm_prop_enum_list gop_hstretch_enum_list[] = {
	{.type = MTK_DRM_HSTRETCH_DUPLICATE, .name = MTK_PLANE_PROP_HSTRETCH_DUPLICATE},
	{.type = MTK_DRM_HSTRETCH_6TAP8PHASE, .name = MTK_PLANE_PROP_HSTRETCH_6TAP8PHASE},
	{.type = MTK_DRM_HSTRETCH_4TAP256PHASE, .name = MTK_PLANE_PROP_HSTRETCH_4TAP256PHASE},
};

struct drm_prop_enum_list gop_vstretch_enum_list[] = {
	{.type = MTK_DRM_VSTRETCH_2TAP16PHASE, .name = MTK_PLANE_PROP_VSTRETCH_2TAP16PHASE},
	{.type = MTK_DRM_VSTRETCH_DUPLICATE, .name = MTK_PLANE_PROP_VSTRETCH_DUPLICATE},
	{.type = MTK_DRM_VSTRETCH_BILINEAR, .name = MTK_PLANE_PROP_VSTRETCH_BILINEAR},
	{.type = MTK_DRM_VSTRETCH_4TAP, .name = MTK_PLANE_PROP_VSTRETCH_4TAP256PHASE},
};

struct drm_prop_enum_list gop_afbc_feature_list[] = {
	{.type = MTK_DRM_SUPPORT_FEATURE_YES, .name = MTK_PLANE_PROP_SUPPORT},
	{.type = MTK_DRM_SUPPORT_FEATURE_NO, .name = MTK_PLANE_PROP_UNSUPPORT},
};

struct drm_prop_enum_list gop_bypass_enum_list[] = {
	{.type = MTK_DRM_PLANE_BYPASS_DISABLE, .name = MTK_PLANE_PROP_DISABLE},
	{.type = MTK_DRM_PLANE_BYPASS_ENABLE, .name = MTK_PLANE_PROP_ENABLE},
};

struct drm_prop_enum_list gop_aid_type_enum_list[] = {
	{.type = MTK_DRM_AID_TYPE_NS, .name = MTK_PLANE_PROP_AID_NS},
	{.type = MTK_DRM_AID_TYPE_SDC, .name = MTK_PLANE_PROP_AID_SDC},
	{.type = MTK_DRM_AID_TYPE_S, .name = MTK_PLANE_PROP_AID_S},
	{.type = MTK_DRM_AID_TYPE_CSP, .name = MTK_PLANE_PROP_AID_CSP},
};

static const struct ext_prop_info gop_plane_props[] = {
	{
	 .prop_name = MTK_PLANE_PROP_HMIRROR,
	 .flag = DRM_MODE_PROP_ENUM | DRM_MODE_PROP_ATOMIC,
	 .enum_info.enum_list = &gop_hmirror_enum_list[0],
	 .enum_info.enum_length = ARRAY_SIZE(gop_hmirror_enum_list),
	 },
	{
	 .prop_name = MTK_PLANE_PROP_VMIRROR,
	 .flag = DRM_MODE_PROP_ENUM | DRM_MODE_PROP_ATOMIC,
	 .enum_info.enum_list = &gop_vmirror_enum_list[0],
	 .enum_info.enum_length = ARRAY_SIZE(gop_vmirror_enum_list),
	 },
	{
	 .prop_name = MTK_PLANE_PROP_HSTRETCH,
	 .flag = DRM_MODE_PROP_ENUM | DRM_MODE_PROP_ATOMIC,
	 .enum_info.enum_list = &gop_hstretch_enum_list[0],
	 .enum_info.enum_length = ARRAY_SIZE(gop_hstretch_enum_list),
	 },
	{
	 .prop_name = MTK_PLANE_PROP_VSTRETCH,
	 .flag = DRM_MODE_PROP_ENUM | DRM_MODE_PROP_ATOMIC,
	 .enum_info.enum_list = &gop_vstretch_enum_list[0],
	 .enum_info.enum_length = ARRAY_SIZE(gop_vstretch_enum_list),
	 },
	{
	 .prop_name = MTK_PLANE_PROP_AFBC_FEATURE,
	 .flag = DRM_MODE_PROP_IMMUTABLE | DRM_MODE_PROP_ATOMIC,
	 .enum_info.enum_list = &gop_afbc_feature_list[0],
	 .enum_info.enum_length = ARRAY_SIZE(gop_afbc_feature_list),
	 },
	{
	 .prop_name = MTK_GOP_PLANE_PROP_PNL_CURLUM,
	 .flag = DRM_MODE_PROP_RANGE | DRM_MODE_PROP_ATOMIC,
	 .range_info.min = 0,
	 .range_info.max = 65535,
	 },
	{
		.prop_name = MTK_GOP_PLANE_PROP_BYPASS,
		.flag = DRM_MODE_PROP_ENUM | DRM_MODE_PROP_ATOMIC,
		.enum_info.enum_list = &gop_bypass_enum_list[0],
		.enum_info.enum_length = ARRAY_SIZE(gop_bypass_enum_list),
	},
	{
		.prop_name = MTK_GOP_PLANE_PROP_PQ_DATA,
		.flag = DRM_MODE_PROP_BLOB | DRM_MODE_PROP_ATOMIC,
	},
	{
		.prop_name = MTK_GOP_PLANE_PROP_ALPHA_MODE,
		.flag = DRM_MODE_PROP_BLOB | DRM_MODE_PROP_ATOMIC,
	},
	{
		.prop_name = MTK_GOP_PLANE_PROP_AID_TYPE,
		.flag = DRM_MODE_PROP_ENUM | DRM_MODE_PROP_ATOMIC,
		.enum_info.enum_list = &gop_aid_type_enum_list[0],
		.enum_info.enum_length = ARRAY_SIZE(gop_aid_type_enum_list),
	},
};

#endif
