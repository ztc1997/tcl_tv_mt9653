/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * governor.h - internal header for devfreq governors.
 *
 * Copyright (C) 2011 Samsung Electronics
 *	MyungJoo Ham <myungjoo.ham@samsung.com>
 *
 * This header is for devfreq governors in drivers/devfreq/
 */

#ifndef _GOVERNOR_H
#define _GOVERNOR_H

#include <linux/devfreq.h>

#define to_devfreq(DEV)	container_of((DEV), struct devfreq, dev)

/* Devfreq events */
#define DEVFREQ_GOV_START			0x1
#define DEVFREQ_GOV_STOP			0x2
#define DEVFREQ_GOV_INTERVAL			0x3
#define DEVFREQ_GOV_SUSPEND			0x4
#define DEVFREQ_GOV_RESUME			0x5

#define DEVFREQ_MIN_FREQ			0
#define DEVFREQ_MAX_FREQ			ULONG_MAX

/**
 * struct devfreq_governor - Devfreq policy governor
 * @node:		list node - contains registered devfreq governors
 * @name:		Governor's name
 * @immutable:		Immutable flag for governor. If the value is 1,
 *			this govenror is never changeable to other governor.
 * @get_target_freq:	Returns desired operating frequency for the device.
 *			Basically, get_target_freq will run
 *			devfreq_dev_profile.get_dev_status() to get the
 *			status of the device (load = busy_time / total_time).
 *			If no_central_polling is set, this callback is called
 *			only with update_devfreq() notified by OPP.
 * @event_handler:      Callback for devfreq core framework to notify events
 *                      to governors. Events include per device governor
 *                      init and exit, opp changes out of devfreq, suspend
 *                      and resume of per device devfreq during device idle.
 *
 * Note that the callbacks are called with devfreq->lock locked by devfreq.
 */
struct devfreq_governor {
	struct list_head node;

	const char name[DEVFREQ_NAME_LEN];
	const unsigned int immutable;
	int (*get_target_freq)(struct devfreq *this, unsigned long *freq);
	int (*event_handler)(struct devfreq *devfreq,
				unsigned int event, void *data);
};

extern void devfreq_monitor_start(struct devfreq *devfreq);
extern void devfreq_monitor_stop(struct devfreq *devfreq);
extern void devfreq_monitor_suspend(struct devfreq *devfreq);
extern void devfreq_monitor_resume(struct devfreq *devfreq);
extern void devfreq_interval_update(struct devfreq *devfreq,
					unsigned int *delay);

extern int devfreq_add_governor(struct devfreq_governor *governor);
extern int devfreq_remove_governor(struct devfreq_governor *governor);

extern int devfreq_update_status(struct devfreq *devfreq, unsigned long freq);

extern int update_devfreq(struct devfreq *devfreq);

static inline int devfreq_update_stats(struct devfreq *df)
{
	return df->profile->get_dev_status(df->dev.parent, &df->last_status);
}
#endif /* _GOVERNOR_H */
