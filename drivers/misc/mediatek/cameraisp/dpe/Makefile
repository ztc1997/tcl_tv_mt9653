#
# Copyright (C) 2015 MediaTek Inc.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
#



$(info $(srctree))
ccflags-y += -I$(srctree)/drivers/misc/mediatek/m4u/$(MTK_PLATFORM)/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/mmp/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/cmdq/v3/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/cmdq/v3/$(MTK_PLATFORM)/
ccflags-y += -I$(srctree)/drivers/staging/android/ion \
             -I$(srctree)/drivers/staging/android/ion/mtk
ccflags-y += -I$(srctree)/drivers/misc/mediatek/smi/
ccflags-$(CONFIG_MTK_IOMMU_V2) += -I$(srctree)/drivers/iommu
ccflags-$(CONFIG_MTK_IOMMU_V2) += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/$(MTK_PLATFORM)/include/

obj-y  += cameradpe_dummy.o
obj-$(CONFIG_MTK_CAMERA_ISP_DPE_SUPPORT) += camera_dpe.o


camera_dpe-objs += isp_60/engine_request.o
camera_dpe-objs  += isp_60/camera_dpe.o
camera_dpe-objs += isp_60/videobuf2-dma-contig.o


MTK_DPE_VER := 1

subdir-ccflags-y += -DMTK_DPE_VER=$(MTK_DPE_VER)