/* SPDX-License-Identifier: GPL-2.0*/
/*
 * Copyright (C) 2019 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#ifndef _APUSYS_POWER_H_
#define _APUSYS_POWER_H_

#include "apusys_power_user.h"

struct profiling_timestamp {
	u64 begin;
	u64 end;
};

#ifndef AI_SIM
#include "pmic_api_buck.h"
/******************************************************
 * for apusys power platform device API
 ******************************************************/
#ifndef BUILD_POLICY_TEST
extern int apu_power_device_register(enum DVFS_USER, struct platform_device*);
#endif
extern void apu_power_device_unregister(enum DVFS_USER);
extern int apu_device_power_on(enum DVFS_USER);
extern int apu_device_power_off(enum DVFS_USER);
extern int apu_device_power_suspend(enum DVFS_USER user, int suspend);
extern void apu_device_set_opp(enum DVFS_USER user, uint8_t opp);
extern uint64_t apu_get_power_info(int force);
extern bool apu_get_power_on_status(enum DVFS_USER user);
extern void apu_power_on_callback(void);
extern int apu_power_callback_device_register(enum POWER_CALLBACK_USER user,
					void (*power_on_callback)(void *para),
					void (*power_off_callback)(void *para));
extern void apu_power_callback_device_unregister(enum POWER_CALLBACK_USER user);
extern uint8_t apusys_boost_value_to_opp
				(enum DVFS_USER user, uint8_t boost_value);
extern enum DVFS_FREQ apusys_opp_to_freq(enum DVFS_USER user, uint8_t opp);
extern uint8_t apusys_freq_to_opp(enum DVFS_VOLTAGE_DOMAIN buck_domain,
							uint32_t freq);
extern int8_t apusys_get_ceiling_opp(enum DVFS_USER user);
extern int8_t apusys_get_opp(enum DVFS_USER user);
extern void apu_power_reg_dump(void);
extern int apu_power_power_stress(int type, int device, int opp);
extern bool apusys_power_check(void);
extern void apu_set_vcore_boost(bool enable);
extern void apu_qos_set_vcore(int target_volt);
void apu_profiling(struct profiling_timestamp *profile, const char *tag);
#else /* AI_SIM */
#ifndef BUILD_POLICY_TEST
static inline int apu_power_device_register(enum DVFS_USER user
, struct platform_device *plat_dev) {return 0; }
#endif
static inline void apu_power_device_unregister(enum DVFS_USER user) {}
static inline int apu_device_power_on(enum DVFS_USER user) {return 0; }
static inline int apu_device_power_off(enum DVFS_USER user) {return 0; }
static inline int apu_device_power_suspend(enum DVFS_USER user, int suspend) {return 0; }
static inline void apu_device_set_opp(enum DVFS_USER user, uint8_t opp) {}
static inline uint64_t apu_get_power_info(int force) {return 0; }
static inline bool apu_get_power_on_status(enum DVFS_USER user) {return true; }
static inline void apu_power_on_callback(void) {}
static inline int apu_power_callback_device_register(enum POWER_CALLBACK_USER user,
					void (*power_on_callback)(void *para),
					void (*power_off_callback)(void *para)) {return 0; }
static inline void apu_power_callback_device_unregister(enum POWER_CALLBACK_USER user) {}
static inline uint8_t apusys_boost_value_to_opp
				(enum DVFS_USER user, uint8_t boost_value) {return 0; }
static inline enum DVFS_FREQ apusys_opp_to_freq(enum DVFS_USER user, uint8_t opp)
{return DVFS_FREQ_00_026000_F; }
static inline uint8_t apusys_freq_to_opp(enum DVFS_VOLTAGE_DOMAIN buck_domain,
							uint32_t freq) {return 0; }
static inline int8_t apusys_get_ceiling_opp(enum DVFS_USER user) {return 0; }
static inline int8_t apusys_get_opp(enum DVFS_USER user) {return 0; }
static inline void apu_power_reg_dump(void) {}
static inline int apu_power_power_stress(int type, int device, int opp) {return 0; }
static inline bool apusys_power_check(void) {return true; }
static inline void apu_set_vcore_boost(bool enable) {}
static inline void apu_qos_set_vcore(int target_volt) {}
static inline void apu_profiling(struct profiling_timestamp *profile, const char *tag) {}
#endif
#endif
