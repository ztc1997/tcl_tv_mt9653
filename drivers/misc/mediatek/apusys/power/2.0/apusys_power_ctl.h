/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef _APUSYS_POWER_CTL_H_
#define _APUSYS_POWER_CTL_H_

#include "apusys_power_cust.h"


#define MAX(a, b)	((a) > (b) ? (a) : (b))
#define MIN(a, b)	((a) < (b) ? (a) : (b))

extern struct apusys_dvfs_opps apusys_opps;
extern struct device *apu_dev;
extern spinlock_t ipuif_lock;

extern void apusys_dvfs_policy(uint64_t round_id);
extern void apusys_set_opp(enum DVFS_USER user, uint8_t opp);
extern bool apusys_check_opp_change(void);
extern int apusys_power_init(enum DVFS_USER user, void *init_power_data);
extern void apusys_power_uninit(enum DVFS_USER user);
extern int apusys_power_on(enum DVFS_USER user);
extern int apusys_power_off(enum DVFS_USER user);
extern enum DVFS_FREQ apusys_get_dvfs_freq(enum DVFS_VOLTAGE_DOMAIN domain);
extern void event_trigger_dvfs_policy(void);
extern bool apusys_get_power_on_status(enum DVFS_USER user);
#if SUPPORT_VCORE_TO_IPUIF
extern void apusys_set_apu_vcore(int target_volt);
extern void apusys_ipuif_opp_change(void);
#endif
#endif
