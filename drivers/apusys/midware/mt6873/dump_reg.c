// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
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

#include <linux/debugfs.h>
#include <linux/mutex.h>
#include <linux/io.h>
#include <linux/sched/clock.h>
#include <apusys_secure.h>
#include <apusys_plat.h>

#define APUSYS_REG_SIZE (0x100000)
#define APUSYS_BASE (0x19000000)
#define APUSYS_TO_INFRA_BASE (0x10000000)
#define NA	(-1)

char *reg_all_mem;
bool apusys_dump_force;
bool apusys_dump_skip_gals;
static void *apu_top;
static void *apu_to_infra_top;
static struct dentry *debug_node;
static struct mutex dbg_lock;

#define DBG_MUX_SEL_COUNT (9)
#define TOTAL_DBG_MUX_COUNT (22)
#define SEGMENT_COUNT (29)


struct dbg_mux_sel_value {
	char name[128];
	int status_reg_offset;
	int dbg_sel[DBG_MUX_SEL_COUNT];
};

struct dbg_mux_sel_info {
	int offset;
	int start_bit;
	int end_bit;
};

struct reg_dump_info {
	char name[128];
	int base;
	int size;
};

struct dbg_mux_sel_info info_table[DBG_MUX_SEL_COUNT] = {

	{0x29010, .start_bit = 1,  .end_bit = 1 }, //vcore_dbg_sel
	{0x29010, .start_bit = 4,  .end_bit = 2 }, //vcore_dbg_sel0
	{0x29010, .start_bit = 7,  .end_bit = 5 }, //vcore_dbg_sel1
	{0x20138, .start_bit = 2,  .end_bit = 0 }, //conn_dbg0_sel
	{0x20138, .start_bit = 11, .end_bit = 9 }, //conn_dbg3_sel
	{0x20138, .start_bit = 20, .end_bit = 18}, //conn_dbg6_sel
	{0x01098, .start_bit = 15, .end_bit = 8 }, //edma_up_dbg_bus_sel
	{0x30a10, .start_bit = 10, .end_bit = 10}, //vpu0_apu_gals_m_ctl_sel
	{0x31a10, .start_bit = 10, .end_bit = 10}, //vpu1_apu_gals_m_ctl_sel
};


struct dbg_mux_sel_value value_table[TOTAL_DBG_MUX_COUNT] = {

	{"VCORE2EMI_S0_GALS_TX",      0x2901C,
		{ 0,  3, NA, NA, NA, NA, NA, NA, NA} },
	{"VCORE2EMI_S1_GALS_TX",      0x2901C,
		{ 0,  4, NA, NA, NA, NA, NA, NA, NA} },
	{"APUSYS2ACP_VCORE_GALS_TX",  0x2901C,
		{ 0,  5, NA, NA, NA, NA, NA, NA, NA} },
	{"XPU2APUSYS_VCORE_GALS_RX",  0x2901C,
		{ 0,  6, NA, NA, NA, NA, NA, NA, NA} },
	{"VPU02CONN_GALS_RX",         0x2901C,
		{ 0,  7, NA,  1,  1, NA, NA, NA, NA} },
	{"VPU12CONN_GALS_RX",         0x2901C,
		{ 0,  7, NA,  1,  2, NA, NA, NA, NA} },
	{"CONN2VPU0_GALS_TX",         0x2901C,
		{ 0,  7, NA,  1,  4, NA, NA, NA, NA} },
	{"CONN2VPU1_GALS_TX",         0x2901C,
		{ 0,  7, NA,  1,  5, NA, NA, NA, NA} },
	{"MNOC_MISC_1/0_DBG_BUS",     0x2901C,
		{ 0,  7, NA,  4, NA,  2, NA, NA, NA} },
	{"MNOC_MISC_3/2_DBG_BUS",     0x2901C,
		{ 0,  7, NA,  4, NA,  3, NA, NA, NA} },
	{"MNOC_MISC_4_DBG_BUS",       0x2901C,
		{ 0,  7, NA,  4, NA,  4, NA, NA, NA} },
	{"APU_UP_SYS_DBG_BUS",        0x2901C,
		{ 0,  7, NA,  4, NA,  5,  1, NA, NA} },
	{"APU_EDMA_0_DBG_BUS_0",      0x2901C,
		{ 0,  7, NA,  4, NA,  5,  2, NA, NA} },
	{"APU_EDMA_0_DBG_BUS_1",      0x2901C,
		{ 0,  7, NA,  4, NA,  5,  3, NA, NA} },
	{"CONN2VCORE_EMI_S0_GALS_RX", 0x2901C,
		{ 1, NA,  3, NA, NA, NA, NA, NA, NA} },
	{"CONN2VCORE_EMI_S1_GALS_RX", 0x2901C,
		{ 1, NA,  4, NA, NA, NA, NA, NA, NA} },
	{"APUSYS2ACP_VCORE_GALS_RX",  0x2901C,
		{ 1, NA,  5, NA, NA, NA, NA, NA, NA} },
	{"XPU2APUSYS_VCORE_GALS_TX",  0x2901C,
		{ 1, NA,  6, NA, NA, NA, NA, NA, NA} },
	{"VPU02CONN_GALS_TX",         0x30A28,
		{NA, NA, NA, NA, NA, NA, NA,  0, NA} },
	{"CONN2VPU0_GALS_RX",         0x30A28,
		{NA, NA, NA, NA, NA, NA, NA,  1, NA} },
	{"VPU12CONN_GALS_TX",         0x31A28,
		{NA, NA, NA, NA, NA, NA, NA, NA,  0} },
	{"CONN2VPU1_GALS_RX",         0x31A28,
		{NA, NA, NA, NA, NA, NA, NA, NA,  1} },
};


struct reg_dump_info range_table[SEGMENT_COUNT] = {

	{"md32_sysCtrl",        0x19001000, 0x800 },
	{"md32_sysCtrl_PMU",    0x19001800, 0x800 },
	{"md32_wdt",            0x19002000, 0x1000},
	{"apu_iommu0_r0",       0x19010000, 0x1000},
	{"apu_iommu0_r1",       0x19011000, 0x1000},
	{"apu_iommu0_r2",       0x19012000, 0x1000},
	{"apu_iommu0_r3",       0x19013000, 0x1000},
	{"apu_iommu0_r4",       0x19014000, 0x1000},
	{"apu_xpu_rsi_config",  0x1901C000, 0x1000},
	{"apu_acp_ssc_config",  0x1901F000, 0x1000},
	{"apu_conn_config",     0x19020000, 0x1000},
	{"apu_sctrl_reviser",   0x19021000, 0x1000},
	{"apu_sema_stimer",     0x19022000, 0x1000},
	{"emi_config",          0x19023000, 0x1000},
	{"apb_dapc_wrapper",    0x19064000, 0x1000},
	{"infra_bcrm",          0x19065000, 0x1000},
	{"apb_debug_ctl",       0x19066000, 0x1000},
	{"noc_dapc_wrapper",    0x1906C000, 0x1000},
	{"apu_noc_bcrm",        0x1906D000, 0x1000},
	{"apu_noc_config_0",    0x1906E000, 0x2000},
	{"apu_noc_config_1",    0x19070000, 0x2000},
	{"apu_noc_config_2",    0x19072000, 0x2000},
	{"apu_noc_config_3",    0x19074000, 0x2000},
	{"apu_noc_config_4",    0x19076000, 0x2000},
	{"apu_rpc(CPC)",        0x190F0000, 0x1000},
	{"apu_rpc(DVFS)",       0x190F1000, 0x1000},
	{"apu_ao_ctrl",         0x190F2000, 0x1000},
	{"apb_dapc_ap_wrapper", 0x190F8000, 0x4000},
	{"noc_dapc_ap_wrapper", 0x190FC000, 0x4000},
};


static void set_dbg_sel(int val, int offset, int shift, int mask)
{
	void *target = apu_top + offset;
	u32 tmp = ioread32(target);

	tmp = (tmp & ~(mask << shift)) | (val << shift);
	iowrite32(tmp, target);
	tmp = ioread32(target);
}

u32 dbg_read(struct dbg_mux_sel_value sel)
{
	int i;
	int offset;
	int shift;
	int length;
	int mask;
	void *addr = apu_top + sel.status_reg_offset;
	struct dbg_mux_sel_info info;

	for (i = 0; i < DBG_MUX_SEL_COUNT; ++i) {
		if (sel.dbg_sel[i] >= 0) {
			info = info_table[i];
			offset = info.offset;
			shift = info.end_bit;
			length = info.start_bit - info.end_bit + 1;
			mask = (1 << length) - 1;

			set_dbg_sel(sel.dbg_sel[i], offset, shift, mask);
		}
	}

	return ioread32(addr);
}

static u32 gals_reg[TOTAL_DBG_MUX_COUNT];

void dump_gals_reg(void)
{
	int i;

	for (i = 0; i < TOTAL_DBG_MUX_COUNT; ++i)
		gals_reg[i] = dbg_read(value_table[i]);
}

void dump_gals(struct seq_file *sfile)
{
	int i;

	for (i = 0; i < TOTAL_DBG_MUX_COUNT; ++i)
		seq_printf(sfile, "%s:0x%08x\n",
			value_table[i].name, gals_reg[i]);
}


void apusys_dump_reg_skip_gals(int onoff)
{
	if (onoff)
		apusys_dump_skip_gals = true;
	else
		apusys_dump_skip_gals = false;
}

void apusys_reg_dump(void)
{
	int i, offset, size;

	mutex_lock(&dbg_lock);

	if (reg_all_mem == NULL)
		reg_all_mem = vzalloc(APUSYS_REG_SIZE);

	if (!apusys_dump_skip_gals)
		dump_gals_reg();

	for (i = 0; i < SEGMENT_COUNT; ++i) {
		offset = range_table[i].base - APUSYS_BASE;
		size = range_table[i].size;

		memcpy_fromio(reg_all_mem + offset, apu_top + offset, size);
	}

	mutex_unlock(&dbg_lock);
}

static void *dump_start(struct seq_file *sfile, loff_t *pos)
{
	mutex_lock(&dbg_lock);
	return pos + (*pos == 0);
}

static void *dump_next(struct seq_file *sfile, void *v, loff_t *pos)
{
	++(*pos);
	return NULL;
}

static void dump_stop(struct seq_file *sfile, void *v)
{
	mutex_unlock(&dbg_lock);
}

int dump_show(struct seq_file *sfile, void *v)
{
	u64 t;
	u64 nanosec_rem;
	int i;

	if (apusys_dump_force)
		apusys_reg_dump();


	if (reg_all_mem == NULL)
		goto out;

	mutex_lock(&dbg_lock);

	t = sched_clock();
		nanosec_rem = do_div(t, 1000000000);

	seq_printf(sfile, "[%5lu.%06lu] ------- dump GALS -------\n",
		(unsigned long) t, (unsigned long) (nanosec_rem / 1000));

	dump_gals(sfile);

	seq_puts(sfile, "---- dump from 0x1900_0000 to 0x190F_FFFF ----\n");
	for (i = 0; i < SEGMENT_COUNT; ++i)
		seq_printf(sfile, "%s:0x%08x to 0x%08x\n", range_table[i].name,
			range_table[i].base,
			range_table[i].base + range_table[i].size);

	seq_hex_dump(sfile, "", DUMP_PREFIX_OFFSET, 16, 4,
		reg_all_mem, APUSYS_REG_SIZE, false);

	vfree(reg_all_mem);
	reg_all_mem = NULL;

	mutex_unlock(&dbg_lock);

out:
	return 0;
}


static const struct seq_operations dump_ops = {
	.start = dump_start,
	.next = dump_next,
	.stop = dump_stop,
	.show = dump_show
};

static int dump_open(struct inode *inode, struct file *file)
{
	return single_open(file, dump_show, NULL);
}

static const struct file_operations apu_dump_debug_fops = {
	.owner = THIS_MODULE,
	.open = dump_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release
};

void apusys_dump_init(void)
{
	mutex_init(&dbg_lock);
	debug_node = debugfs_create_dir("debug", mdw_dbg_root);
	debugfs_create_file("apusys_reg_all", 0444,
			debug_node, NULL, &apu_dump_debug_fops);

	debugfs_create_bool("force_dump", 0644,
			debug_node, &apusys_dump_force);
	apu_top = ioremap_nocache(APUSYS_BASE, APUSYS_REG_SIZE);
	apu_to_infra_top = ioremap_nocache(APUSYS_TO_INFRA_BASE, 0x10000);

	reg_all_mem = NULL;
	apusys_dump_force = false;
	apusys_dump_skip_gals = false;
}

void apusys_dump_exit(void)
{
	debugfs_remove_recursive(debug_node);
	iounmap(apu_top);
	iounmap(apu_to_infra_top);
}
