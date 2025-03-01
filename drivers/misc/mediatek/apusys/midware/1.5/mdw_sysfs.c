// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/sysfs.h>
#include <linux/device.h>

#include "mdw_rsc.h"
#include "mdw_queue.h"
#include "mdw_cmn.h"

static struct device *mdw_dev;

static ssize_t dsp_task_num_show(struct device *dev,
				     struct device_attribute *attr, char *buf)
{
	struct mdw_queue *mq = NULL;
	int ret = 0;

	mq = mdw_rsc_get_queue(APUSYS_DEVICE_VPU);
	if (!mq)
		return -EINVAL;

	ret = sprintf(buf, "%u\n", mq->normal_task_num);

	return ret;
}
static DEVICE_ATTR_RO(dsp_task_num);

static ssize_t dla_task_num_show(struct device *dev,
				     struct device_attribute *attr, char *buf)
{
	struct mdw_queue *mq = NULL;
	int ret = 0;

	mq = mdw_rsc_get_queue(APUSYS_DEVICE_MDLA);
	if (!mq)
		return -EINVAL;

	ret = sprintf(buf, "%u\n", mq->normal_task_num);

	return ret;
}
static DEVICE_ATTR_RO(dla_task_num);

static ssize_t dma_task_num_show(struct device *dev,
				     struct device_attribute *attr, char *buf)
{
	struct mdw_queue *mq = NULL;
	int ret = 0;

	mq = mdw_rsc_get_queue(APUSYS_DEVICE_EDMA);
	if (!mq)
		return -EINVAL;

	ret = sprintf(buf, "%u\n", mq->normal_task_num);

	return ret;
}
static DEVICE_ATTR_RO(dma_task_num);

static struct attribute *mdw_task_attrs[] = {
	&dev_attr_dsp_task_num.attr,
	&dev_attr_dla_task_num.attr,
	&dev_attr_dma_task_num.attr,
	NULL,
};

static struct attribute_group mdw_devinfo_attr_group = {
	.name	= "queue",
	.attrs	= mdw_task_attrs,
};

int mdw_sysfs_init(struct device *kdev)
{
	int ret = 0;

	/* create /sys/devices/platform/apusys/device/xxx */
	mdw_dev = kdev;
	ret = sysfs_create_group(&mdw_dev->kobj, &mdw_devinfo_attr_group);
	if (ret)
		mdw_drv_err("create mdw attribute fail, ret %d\n", ret);

	return ret;
}

void mdw_sysfs_exit(void)
{
	if (mdw_dev)
		sysfs_remove_group(&mdw_dev->kobj, &mdw_devinfo_attr_group);

	mdw_dev = NULL;
}
