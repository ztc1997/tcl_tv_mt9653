// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include <linux/types.h>
#include <linux/errno.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/kref.h>
#include <linux/kthread.h>
#include <linux/uaccess.h>
#include <linux/atomic.h>

#include "mdw_cmn.h"
#include "mdw_queue.h"
#include "mdw_rsc.h"
#include "mdw_cmd.h"
#include "mdw_sched.h"
#include "mdw_dispr.h"
#include "mdw_trace.h"
#include "mdw_import.h"
#include "mdw_tag.h"
#ifdef APUSYS_MDW_TAG_SUPPORT
#define CREATE_TRACE_POINTS
#include "mdw_events.h"
#endif

struct mdw_sched_mgr {
	struct task_struct *task;
	struct completion cmplt;

	struct list_head ds_list; //done sc list
	struct mutex mtx;

	bool pause;
	bool stop;
};

static struct mdw_cmd_parser *cmd_parser;
static struct mdw_sched_mgr ms_mgr;

#define MDW_EXEC_PRINT " pid(%d/%d) cmd(0x%llx/0x%llx-#%d/%u)"\
	" dev(%d/%s-#%d) mp(0x%x/%u/%u/0x%llx) sched(%d/%u/%u/%u/%u/%d)"\
	" mem(%lu/%d/0x%x/0x%x) boost(%u) bw(%u) time(%u/%u)"

static void mdw_sched_met_start(struct mdw_apu_sc *sc, struct mdw_dev_info *d)
{
	mdw_trace_begin("apusys_scheduler|dev: %d_%d, cmd_id: 0x%08llx",
		d->type,
		d->idx,
		sc->parent->kid);
}

static void mdw_sched_met_end(struct mdw_apu_sc *sc, struct mdw_dev_info *d,
	int ret)
{
	mdw_trace_end("apusys_scheduler|dev: %d_%d, cmd_id: 0x%08llx, ret:%d",
		d->type,
		d->idx,
		sc->parent->kid, ret);
}

static inline uint64_t mdw_sched_shift(uint64_t val, int start, int end)
{
	return val << (63 - (end - start)) >> (63 - end);
}

static void mdw_sched_trace(struct mdw_apu_sc *sc,
	struct mdw_dev_info *d, struct apusys_cmd_hnd *h, int ret, int done)
{
	char state[16];
	uint64_t sc_info = 0, multi_info = 0, exec_info = 0, tcm_info = 0;

	/* prefix */
	memset(state, 0, sizeof(state));
	if (!done) {
		mdw_sched_met_start(sc, d);
		if (snprintf(state, sizeof(state)-1, "start :") < 0)
			return;
	} else {
		mdw_sched_met_end(sc, d, ret);
		if (ret) {
			if (snprintf(state, sizeof(state)-1, "fail :") < 0)
				return;
		} else {
			if (snprintf(state, sizeof(state)-1, "done :") < 0)
				return;
		}
	}

	/* if err, use mdw_drv_err */
	if (ret) {
		mdw_drv_err("%s"MDW_EXEC_PRINT" ret(%d)\n",
			state,
			sc->parent->pid,
			sc->parent->tgid,
			sc->parent->hdr->uid,
			sc->parent->kid,
			sc->idx,
			sc->parent->hdr->num_sc,
			d->type,
			d->name,
			d->idx,
			sc->hdr->pack_id,
			h->multicore_idx,
			sc->multi_total,
			sc->multi_bmp,
			sc->parent->hdr->priority,
			sc->parent->hdr->soft_limit,
			sc->parent->hdr->hard_limit,
			sc->hdr->ip_time,
			sc->hdr->suggest_time,
			0,//sc->par_cmd->power_save,
			sc->ctx,
			sc->hdr->tcm_force,
			sc->hdr->tcm_usage,
			sc->real_tcm_usage,
			h->boost_val,
			sc->bw,
			h->ip_time,
			sc->driver_time,
			ret);
	} else {
		mdw_drv_debug("%s"MDW_EXEC_PRINT" ret(%d)\n",
			state,
			sc->parent->pid,
			sc->parent->tgid,
			sc->parent->hdr->uid,
			sc->parent->kid,
			sc->idx,
			sc->parent->hdr->num_sc,
			d->type,
			d->name,
			d->idx,
			sc->hdr->pack_id,
			h->multicore_idx,
			sc->multi_total,
			sc->multi_bmp,
			sc->parent->hdr->priority,
			sc->parent->hdr->soft_limit,
			sc->parent->hdr->hard_limit,
			sc->hdr->ip_time,
			sc->hdr->suggest_time,
			0,//sc->par_cmd->power_save,
			sc->ctx,
			sc->hdr->tcm_force,
			sc->hdr->tcm_usage,
			sc->real_tcm_usage,
			h->boost_val,
			sc->bw,
			h->ip_time,
			sc->driver_time,
			ret);
	}

	/* encode info for 12 args limitation */
	sc_info = mdw_sched_shift((uint64_t)sc->idx, 32, 63) |
	mdw_sched_shift((uint64_t)sc->parent->hdr->num_sc, 0, 31);
	multi_info = mdw_sched_shift((uint64_t)sc->hdr->pack_id, 48, 63) |
		mdw_sched_shift((uint64_t)h->multicore_idx, 32, 47) |
		mdw_sched_shift((uint64_t)sc->multi_total, 16, 31) |
		mdw_sched_shift((uint64_t)sc->multi_bmp, 0, 15);
	exec_info = mdw_sched_shift((uint64_t)sc->parent->hdr->priority,
		48, 63) |
		mdw_sched_shift((uint64_t)sc->parent->hdr->soft_limit, 32, 47) |
		mdw_sched_shift((uint64_t)sc->parent->hdr->hard_limit, 16, 31) |
		mdw_sched_shift((uint64_t)sc->hdr->suggest_time, 0, 15);
	tcm_info = mdw_sched_shift((uint64_t)sc->ctx, 48, 63) |
		mdw_sched_shift((uint64_t)sc->hdr->tcm_force, 32, 47) |
		mdw_sched_shift((uint64_t)sc->hdr->tcm_usage, 16, 31) |
		mdw_sched_shift((uint64_t)sc->real_tcm_usage, 0, 15);

#ifdef APUSYS_MDW_TAG_SUPPORT
	/* trace cmd end */
	trace_mdw_cmd(done,
		sc->parent->pid,
		sc->parent->tgid,
		sc->parent->kid,
		sc_info,
		d->name,
		multi_info,
		exec_info,
		tcm_info,
		h->boost_val,
		h->ip_time,
		ret);
#endif
}
#undef MDW_EXEC_PRINT

static int mdw_sched_sc_done(void)
{
	struct mdw_apu_cmd *c = NULL;
	struct mdw_apu_sc *sc = NULL, *s = NULL;
	int ret = 0;

	mdw_flw_debug("\n");
	mdw_trace_begin("check done list|%s", __func__);

	/* get done sc from done sc list */
	mutex_lock(&ms_mgr.mtx);
	s = list_first_entry_or_null(&ms_mgr.ds_list,
		struct mdw_apu_sc, ds_item);
	if (s)
		list_del(&s->ds_item);
	mutex_unlock(&ms_mgr.mtx);
	if (!s) {
		ret = -ENODATA;
		goto out;
	}
	/* recv finished subcmd */
	while (1) {
		c = s->parent;
		ret = cmd_parser->end_sc(s, &sc);
		mdw_flw_debug("\n");
		/* check return value */
		if (ret) {
			mdw_drv_err("parse done sc fail(%d)\n", ret);
			complete(&c->cmplt);
			break;
		}
		/* check parsed sc */
		if (sc) {
		/*
		 * finished sc parse ok, should be call
		 * again because residual sc
		 */
			mdw_flw_debug("sc(0x%llx-#%d)\n",
				sc->parent->kid, sc->idx);
			mdw_sched(sc);
		} else {
		/* finished sc parse done, break loop */
			mdw_sched(NULL);
			break;
		}
	};

out:
	mdw_trace_end("check done list|%s", __func__);
	return ret;
}

static void mdw_sched_enque_done_sc(struct kref *ref)
{
	struct mdw_apu_sc *sc =
		container_of(ref, struct mdw_apu_sc, multi_ref);

	mdw_flw_debug("sc(0x%llx-#%d)\n", sc->parent->kid, sc->idx);
	cmd_parser->put_ctx(sc);

	mutex_lock(&ms_mgr.mtx);
	list_add_tail(&sc->ds_item, &ms_mgr.ds_list);
	mutex_unlock(&ms_mgr.mtx);

	mdw_sched(NULL);
}

int mdw_sched_dev_routine(void *arg)
{
	int ret = 0;
	struct mdw_dev_info *d = (struct mdw_dev_info *)arg;
	struct apusys_cmd_hnd h;
	struct mdw_apu_sc *sc = NULL;

	/* execute */
	while (!kthread_should_stop() && d->stop == false) {
		mdw_flw_debug("\n");
		ret = wait_for_completion_interruptible(&d->cmplt);
		if (ret)
			goto direc_next;

		sc = (struct mdw_apu_sc *)d->sc;
		if (!sc) {
			mdw_drv_warn("no sc to exec\n");
			goto direc_next;
		}

		mdw_trace_begin("dev(%s-%d) exec|sc(0x%llx-%d) boost(%d/%u)",
			d->name, d->idx, sc->parent->kid, sc->idx,
			h.boost_val, sc->boost);

		/* get mem ctx */
		if (cmd_parser->get_ctx(sc)) {
			mdw_drv_err("cmd(0x%llx-#%d) get ctx fail\n",
				sc->parent->kid, sc->idx);
			goto next;
		}

		/* construct cmd hnd */
		mdw_queue_boost(sc);
		cmd_parser->set_hnd(sc, d->idx, &h);

		/*
		 * Execute reviser to switch VLM:
		 * Skip set context on preemptive command,
		 * context should be set by engine driver itself.
		 * Give engine a callback to set context id.
		 */
		if (d->type != APUSYS_DEVICE_MDLA &&
			d->type != APUSYS_DEVICE_MDLA_RT) {
			mdw_rvs_set_ctx(d->type,
					d->idx, sc->ctx);
		}

		/* count qos start */
		mdw_qos_cmd_start(sc->parent->kid, sc->idx,
			sc->type, d->idx, h.boost_val);

		/* execute */
		mdw_sched_trace(sc, d, &h, ret, 0);
		getnstimeofday(&sc->ts_start);
		ret = d->dev->send_cmd(APUSYS_CMD_EXECUTE, &h, d->dev);
		getnstimeofday(&sc->ts_end);
		sc->driver_time = mdw_cmn_get_time_diff(&sc->ts_start,
			&sc->ts_end);

		/* count qos end */
		mutex_lock(&sc->mtx);
		sc->bw += mdw_qos_cmd_end(sc->parent->kid, sc->idx,
			sc->type, d->idx);
		sc->ip_time = sc->ip_time > h.ip_time ? sc->ip_time : h.ip_time;
		sc->boost = h.boost_val;
		sc->status = ret;
		mdw_flw_debug("multi bmp(0x%llx)\n", sc->multi_bmp);
		mutex_unlock(&sc->mtx);

		mdw_sched_trace(sc, d, &h, ret, 1);

		/* put device */
		if (mdw_rsc_put_dev(d))
			mdw_drv_err("put dev(%d-#%d) fail\n",
				d->type, d->dev->idx);

		mdw_flw_debug("sc(0x%llx-#%d) ref(%d)\n", sc->parent->kid,
			sc->idx, kref_read(&sc->multi_ref));
		kref_put(&sc->multi_ref, mdw_sched_enque_done_sc);
next:
		mdw_trace_end("dev(%s-%d) exec|sc(0x%llx-%d) boost(%d/%u)",
			d->name, d->idx, sc->parent->kid, sc->idx,
			h.boost_val, sc->boost);
direc_next:
		mdw_flw_debug("done\n");
		continue;
	}

	complete(&d->thd_done);

	return 0;
}

static int mdw_sched_get_type(uint64_t bmp)
{
	unsigned long tmp[BITS_TO_LONGS(APUSYS_DEVICE_MAX)];

	memset(&tmp, 0, sizeof(tmp));
	bitmap_from_arr32(tmp, (const uint32_t *)&bmp, APUSYS_DEVICE_MAX);

	return find_last_bit((unsigned long *)&tmp, APUSYS_DEVICE_MAX);
}

static int mdw_sched_dispatch(struct mdw_apu_sc *sc)
{
	int ret = 0, dev_num = 0, exec_num = 0;

	mdw_trace_begin("dispatch|sc(0x%llx-%d) pack(%d) multi(%d)",
		sc->parent->kid, sc->idx, sc->hdr->pack_id, sc->parent->multi);

	/* get dev num */
	dev_num =  mdw_rsc_get_dev_num(sc->type);

	/* check exec #dev */
	exec_num = cmd_parser->exec_core_num(sc);
	exec_num = exec_num < dev_num ? exec_num : dev_num;
	sc->multi_total = exec_num;

	/* select dispatch policy */
	if (sc->hdr->pack_id)
		ret = mdw_dispr_pack(sc);
	else if (sc->parent->multi == HDR_FLAG_MULTI_MULTI && exec_num >= 2)
		ret = mdw_dispr_multi(sc);
	else
		ret = mdw_dispr_norm(sc);

	mdw_trace_end("dispatch|sc(0x%llx-%d)",
		sc->parent->kid, sc->idx);

	return ret;
}

static int mdw_sched_routine(void *arg)
{
	int ret = 0;
	struct mdw_apu_sc *sc = NULL;
	uint64_t bmp = 0;
	int t = 0;

	mdw_flw_debug("\n");

	while (!kthread_should_stop() && !ms_mgr.stop) {
		ret = wait_for_completion_interruptible(&ms_mgr.cmplt);
		if (ret)
			mdw_drv_warn("sched ret(%d)\n", ret);

		if (ms_mgr.pause == true)
			continue;

		if (!mdw_sched_sc_done()) {
			mdw_sched(NULL);
			goto next;
		}

		mdw_dispr_check();

		bmp = mdw_rsc_get_avl_bmp();
		t = mdw_sched_get_type(bmp);
		if (t < 0 || t >= APUSYS_DEVICE_MAX) {
			mdw_flw_debug("nothing to sched(%d)\n", t);
			goto next;
		}

		/* get queue */
		sc = mdw_queue_pop(t);
		if (!sc) {
			mdw_drv_err("pop sc(%d) fail\n", t);
			goto fail_pop_sc;
		}
		mdw_flw_debug("pop sc(0x%llx-#%d/%d/%llu)\n",
			sc->parent->kid, sc->idx, sc->type, sc->period);

		/* dispatch cmd */
		ret = mdw_sched_dispatch(sc);
		if (ret) {
			mdw_flw_debug("sc(0x%llx-#%d) dispatch fail",
				sc->parent->kid, sc->idx);
			goto fail_exec_sc;
		}

		goto next;

fail_exec_sc:
	if (mdw_queue_insert(sc, true)) {
		mdw_drv_err("sc(0x%llx-#%d) insert fail\n",
			sc->parent->kid, sc->idx);
	}
fail_pop_sc:
next:
		mdw_flw_debug("\n");
		if (mdw_rsc_get_avl_bmp())
			mdw_sched(NULL);
	}

	mdw_drv_warn("schedule thread end\n");

	return 0;
}

int mdw_sched(struct mdw_apu_sc *sc)
{
	int ret = 0;

	/* no input sc, trigger sched thread only */
	if (!sc) {
		complete(&ms_mgr.cmplt);
		return 0;
	}

	/* insert sc to queue */
	ret = mdw_queue_insert(sc, false);
	if (ret) {
		mdw_drv_err("sc(0x%llx-#%d) enque fail\n",
			sc->parent->kid, sc->idx);
		return ret;
	}

	complete(&ms_mgr.cmplt);

	mdw_flw_debug("\n");

	return 0;
}

void mdw_sched_pause(void)
{
	struct mdw_dev_info *d = NULL;
	int type = 0, idx = 0, ret = 0, i = 0;

	if (ms_mgr.pause == true) {
		mdw_drv_warn("pause ready\n");
		return;
	}

	ms_mgr.pause = true;

	for (type = 0; type < APUSYS_DEVICE_MAX; type++) {
		for (idx = 0; idx < mdw_rsc_get_dev_num(type); idx++) {
			d = mdw_rsc_get_dinfo(type, idx);
			if (!d)
				continue;
			ret = d->suspend(d);
			if (ret) {
				mdw_drv_err("dev(%s%d) suspend fail(%d)\n",
					d->name, d->idx, ret);
				goto fail_sched_pause;
			}
		}
	}

	mdw_drv_info("pause\n");
	return;

fail_sched_pause:
	for (i = 0; i <= type; i++) {
		for (idx = 0; idx < mdw_rsc_get_dev_num(type); idx++) {
			d = mdw_rsc_get_dinfo(type, idx);
			if (!d)
				continue;
			ret = d->resume(d);
			if (ret) {
				mdw_drv_err("dev(%s%d) resume fail(%d)\n",
					d->name, d->idx, ret);
				goto fail_sched_pause;
			}
		}
	}
	ms_mgr.pause = false;
	mdw_drv_warn("resume\n");
}

void mdw_sched_restart(void)
{
	struct mdw_dev_info *d = NULL;
	int type = 0, idx = 0, ret = 0;

	if (ms_mgr.pause == false)
		mdw_drv_warn("resume ready\n");
	else
		mdw_drv_info("resume\n");

	for (type = 0; type < APUSYS_DEVICE_MAX; type++) {
		for (idx = 0; idx < mdw_rsc_get_dev_num(type); idx++) {
			d = mdw_rsc_get_dinfo(type, idx);
			if (!d)
				continue;
			ret = d->resume(d);
			if (ret)
				mdw_drv_err("dev(%s%d) resume fail(%d)\n",
				d->name, d->idx, ret);
		}
	}

	ms_mgr.pause = false;
	mdw_sched(NULL);
}

int mdw_sched_init(void)
{
	memset(&ms_mgr, 0, sizeof(ms_mgr));
	ms_mgr.pause = false;
	ms_mgr.stop = false;
	init_completion(&ms_mgr.cmplt);
	INIT_LIST_HEAD(&ms_mgr.ds_list);
	mutex_init(&ms_mgr.mtx);

	cmd_parser = mdw_cmd_get_parser();
	if (!cmd_parser)
		return -ENODEV;

	ms_mgr.task = kthread_run(mdw_sched_routine,
		NULL, "apusys_sched");
	if (!ms_mgr.task) {
		mdw_drv_err("create kthread(sched) fail\n");
		return -ENOMEM;
	}

	mdw_dispr_init();

	return 0;
}

void mdw_sched_exit(void)
{
	ms_mgr.stop = true;
	mdw_sched(NULL);
	mdw_dispr_exit();
}
