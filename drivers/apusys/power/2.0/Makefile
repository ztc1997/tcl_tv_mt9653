#
# Copyright (C) 2019 MediaTek Inc.
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

PDIR = $(MTK_PLATFORM)

ccflags-y += -I$(SUBDIRS)/debug/
ccflags-y += -I$(SUBDIRS)/include/
ccflags-y += -I$(SUBDIRS)/include/mt-plat
ccflags-y += -I$(SUBDIRS)/power/2.0/
ccflags-y += -I$(SUBDIRS)/power/2.0/test/
ccflags-y += -I$(SUBDIRS)/power/2.0/$(PDIR)
ccflags-y += -I$(SUBDIRS)/midware/1.0/

# FPGA environment
ifeq ($(CONFIG_MTK_FPGA),y)
subdir-ccflags-y += -DMTK_FPGA_PORTING
else ifeq ($(CONFIG_FPGA_EARLY_PORTING),y)
subdir-ccflags-y += -DMTK_FPGA_PORTING
endif

ifeq ($(AI_SIM),yes)
power-objs += apusys_power_driver_sim.o
else
power-objs += apusys_power_driver.o apusys_power_ctl.o apusys_power_debug.o \
                $(PDIR)/hal_config_power.o $(PDIR)/apu_power_regulator.o \
                $(PDIR)/apu_power_clock.o $(PDIR)/apusys_power_cust.o \
                $(PDIR)/apu_power_table.o $(PDIR)/apu_platform_resource.o \
                $(PDIR)/apu_platform_debug.o $(PDIR)/apusys_power_rule_check.o
ifeq (,$(filter $(strip $(MTK_PLATFORM)), mt6885))
power-objs += $(PDIR)/apu_power_tag.o
endif
endif

ifneq (,$(findstring aging,$(CONFIG_BUILD_ARM64_DTB_OVERLAY_IMAGE_NAMES)))
subdir-ccflags-y += -DAGING_MARGIN
endif

obj-m += power.o

#obj-y += vpu_dummy_driver.o
#vpu_dummy_driver-objs:= $(PDIR)/dummy_vpu_driver.o

#obj-y += mdla_dummy_driver.o
#mdla_dummy_driver-objs:= $(PDIR)/dummy_mdla_driver.o
