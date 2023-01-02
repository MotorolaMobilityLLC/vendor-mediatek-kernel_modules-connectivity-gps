/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#include "gps_mcudl_config.h"
#include "gps_mcudl_log.h"
#include "gps_mcudl_ylink.h"
#include "gps_mcudl_hal_mcu.h"
#include "gps_mcudl_hal_user_fw_own_ctrl.h"
#include "gps_mcudl_data_pkt_slot.h"
#if GPS_DL_HAS_CONNINFRA_DRV
#include "conninfra.h"
#endif

struct gps_mcudl_fw_own_user_context {
	bool init_done;
	bool notified_to_set;
	bool is_fw_own;
	unsigned int sess_id;
	unsigned int user_clr_bitmask;
	unsigned int user_clr_cnt;
	unsigned int user_clr_cnt_on_ntf_set;
	unsigned int real_clr_cnt;
	unsigned int real_set_cnt;
};

struct gps_mcudl_fw_own_user_context g_gps_mcudl_fw_own_ctx;

/* set true to enable sleep even when non lpp mode */
bool g_gps_mcudl_hal_non_lppm_sleep_flag_ctrl;
bool g_gps_mcudl_hal_non_lppm_sleep_flag_used;


void gps_mcul_hal_user_fw_own_lock(void)
{
	gps_mcudl_slot_protect();
}

void gps_mcul_hal_user_fw_own_unlock(void)
{
	gps_mcudl_slot_unprotect();
}

void gps_mcudl_hal_user_fw_own_init(enum gps_mcudl_fw_own_ctrl_user user)
{
	gps_mcul_hal_user_fw_own_lock();
	g_gps_mcudl_fw_own_ctx.init_done = true;
	g_gps_mcudl_fw_own_ctx.notified_to_set = false;
	g_gps_mcudl_fw_own_ctx.sess_id++;

	/* fw_own is cleared when MCU is just on */
	g_gps_mcudl_fw_own_ctx.is_fw_own = false;
	g_gps_mcudl_fw_own_ctx.user_clr_bitmask = (1UL << user);
	g_gps_mcudl_fw_own_ctx.user_clr_cnt = 1;
	g_gps_mcudl_fw_own_ctx.user_clr_cnt_on_ntf_set = 1;
	g_gps_mcudl_fw_own_ctx.real_clr_cnt = 1;
	g_gps_mcudl_fw_own_ctx.real_set_cnt = 0;
	gps_mcul_hal_user_fw_own_unlock();
}

bool gps_mcudl_hal_user_clr_fw_own(enum gps_mcudl_fw_own_ctrl_user user)
{
	bool do_clear = false;
	bool clear_okay = true;
	bool is_fw_own = false;
	unsigned int real_clear_cnt;
	unsigned int user_clr_bitmask, user_clr_bitmask_new;

	gps_mcul_hal_user_fw_own_lock();
	if (!g_gps_mcudl_fw_own_ctx.init_done) {
		gps_mcul_hal_user_fw_own_unlock();
		MDL_LOGW("[init_done=0] bypass user=%d", user);
		return false;
	}
	user_clr_bitmask = g_gps_mcudl_fw_own_ctx.user_clr_bitmask;
	is_fw_own = g_gps_mcudl_fw_own_ctx.is_fw_own;
	do_clear = ((user_clr_bitmask == 0) && is_fw_own);
	real_clear_cnt = g_gps_mcudl_fw_own_ctx.real_clr_cnt + 1;
	gps_mcul_hal_user_fw_own_unlock();

	if (do_clear) {
		/* TODO: Add mutex due to possibe for multi-task */
		clear_okay = gps_mcudl_hal_mcu_clr_fw_own();
	}

	if (do_clear && !clear_okay) {
#if GPS_DL_HAS_CONNINFRA_DRV
		conninfra_trigger_whole_chip_rst(
			CONNDRV_TYPE_GPS, "GNSS clear fw own fail");
#endif
	}

	gps_mcul_hal_user_fw_own_lock();
	g_gps_mcudl_fw_own_ctx.user_clr_cnt++;
	g_gps_mcudl_fw_own_ctx.user_clr_bitmask |= (1UL << user);
	if (do_clear) {
		g_gps_mcudl_fw_own_ctx.is_fw_own = false;
		g_gps_mcudl_fw_own_ctx.real_clr_cnt++;
	}
	user_clr_bitmask_new = g_gps_mcudl_fw_own_ctx.user_clr_bitmask;
	gps_mcul_hal_user_fw_own_unlock();

	if (do_clear && !clear_okay) {
		MDL_LOGE("clr_bitmask=0x%x,0x%x, fw_own=%d, user=%d, do_clear=%d, okay=%d, cnt=%d",
			user_clr_bitmask, user_clr_bitmask_new,
			is_fw_own, user, do_clear, clear_okay, real_clear_cnt);
	} else {
		MDL_LOGD("clr_bitmask=0x%x,0x%x, fw_own=%d, user=%d, do_clear=%d, okay=%d, cnt=%d",
			user_clr_bitmask, user_clr_bitmask_new,
			is_fw_own, user, do_clear, clear_okay, real_clear_cnt);
	}
	return clear_okay;
}

bool gps_mcudl_hal_user_add_if_fw_own_is_clear(enum gps_mcudl_fw_own_ctrl_user user)
{
	bool init_done = false;
	bool already_cleared = false;
	bool is_fw_own = false;
	unsigned int user_clr_bitmask, user_clr_bitmask_new;

	gps_mcul_hal_user_fw_own_lock();
	init_done = g_gps_mcudl_fw_own_ctx.init_done;
	is_fw_own = g_gps_mcudl_fw_own_ctx.is_fw_own;
	user_clr_bitmask = g_gps_mcudl_fw_own_ctx.user_clr_bitmask;
	already_cleared = !is_fw_own;
	if (init_done && already_cleared) {
		g_gps_mcudl_fw_own_ctx.user_clr_bitmask |= (1UL << user);
		g_gps_mcudl_fw_own_ctx.user_clr_cnt++;
	}
	user_clr_bitmask_new = g_gps_mcudl_fw_own_ctx.user_clr_bitmask;
	gps_mcul_hal_user_fw_own_unlock();

	if (!init_done) {
		MDL_LOGW("[init_done=0] bypass user=%d", user);
		return false;
	}

	if (user_clr_bitmask != 0 && is_fw_own) {
		MDL_LOGE("clr_bitmask=0x%x,0x%x, fw_own=%d, user=%d, okay=%d",
			user_clr_bitmask, user_clr_bitmask_new,
			is_fw_own, user, already_cleared);
	} else {
		MDL_LOGD("clr_bitmask=0x%x,0x%x, fw_own=%d, user=%d, okay=%d",
			user_clr_bitmask, user_clr_bitmask_new,
			is_fw_own, user, already_cleared);
	}
	return already_cleared;
}

bool gps_mcudl_hal_user_set_fw_own_may_notify(enum gps_mcudl_fw_own_ctrl_user user)
{
	bool to_notify = false;
	bool notified_to_set = false;
	unsigned int user_clr_bitmask, user_clr_bitmask_new;

	gps_mcul_hal_user_fw_own_lock();
	if (!g_gps_mcudl_fw_own_ctx.init_done) {
		gps_mcul_hal_user_fw_own_unlock();
		MDL_LOGW("[init_done=0] bypass user=%d", user);
		return false;
	}

	user_clr_bitmask = g_gps_mcudl_fw_own_ctx.user_clr_bitmask;
	notified_to_set = g_gps_mcudl_fw_own_ctx.notified_to_set;
	g_gps_mcudl_fw_own_ctx.user_clr_bitmask &= ~(1UL << user);
	to_notify = (!notified_to_set && (g_gps_mcudl_fw_own_ctx.user_clr_bitmask == 0));
	if (to_notify) {
		g_gps_mcudl_fw_own_ctx.user_clr_cnt_on_ntf_set = g_gps_mcudl_fw_own_ctx.user_clr_cnt;
		g_gps_mcudl_fw_own_ctx.notified_to_set = true;
	}
	user_clr_bitmask_new = g_gps_mcudl_fw_own_ctx.user_clr_bitmask;
	gps_mcul_hal_user_fw_own_unlock();

	MDL_LOGD("clr_bitmask=0x%x,0x%x, user=%d, ntf=%d,%d",
		user_clr_bitmask, user_clr_bitmask_new, user, notified_to_set, to_notify);
	if (to_notify)
		gps_mcudl_ylink_event_send(GPS_MDLY_NORMAL, GPS_MCUDL_YLINK_EVT_ID_MCU_SET_FW_OWN);
	return to_notify;
}

void gps_mcudl_hal_user_set_fw_own_if_no_recent_clr(void)
{
	bool has_recent_clr = false;
	bool notified_to_set = false;
	bool is_fw_own = false;
	bool do_set = false;
	bool set_okay = false;
	unsigned user_clr_cnt;
	unsigned user_clr_cnt_on_ntf_set;
	unsigned int user_clr_bitmask;

	gps_mcul_hal_user_fw_own_lock();
	if (!g_gps_mcudl_fw_own_ctx.init_done) {
		gps_mcul_hal_user_fw_own_unlock();
		MDL_LOGW("[init_done=0] bypass");
		return;
	}

	is_fw_own = g_gps_mcudl_fw_own_ctx.is_fw_own;
	user_clr_bitmask = g_gps_mcudl_fw_own_ctx.user_clr_bitmask;
	notified_to_set = g_gps_mcudl_fw_own_ctx.notified_to_set;
	user_clr_cnt = g_gps_mcudl_fw_own_ctx.user_clr_cnt;
	user_clr_cnt_on_ntf_set = g_gps_mcudl_fw_own_ctx.user_clr_cnt_on_ntf_set;
	has_recent_clr =
		((user_clr_cnt != user_clr_cnt_on_ntf_set) && notified_to_set && (user_clr_bitmask == 0));
	if (has_recent_clr) {
		g_gps_mcudl_fw_own_ctx.user_clr_cnt_on_ntf_set = g_gps_mcudl_fw_own_ctx.user_clr_cnt;
		g_gps_mcudl_fw_own_ctx.notified_to_set = true;
		do_set = false;
	} else {
		do_set = (!is_fw_own && user_clr_bitmask == 0);
		if (do_set) {
			g_gps_mcudl_fw_own_ctx.real_set_cnt++;
			g_gps_mcudl_fw_own_ctx.is_fw_own = true;
		}
		g_gps_mcudl_fw_own_ctx.notified_to_set = false;
	}
	gps_mcul_hal_user_fw_own_unlock();

	if (has_recent_clr) {
		MDL_LOGD("has_recent_clr=%d, user=0x%x, ntf=%d, cnt=%d,%d",
			has_recent_clr, user_clr_bitmask, notified_to_set,
			user_clr_cnt_on_ntf_set, user_clr_cnt);
		gps_mcudl_ylink_event_send(GPS_MDLY_NORMAL, GPS_MCUDL_YLINK_EVT_ID_MCU_SET_FW_OWN);
		return;
	}

	if (!do_set) {
		if (user_clr_bitmask == 0) {
			MDL_LOGW("clr_bitmask=0x%x, fw_own=%d, bypass set",
				user_clr_bitmask, is_fw_own);
		} else {
			MDL_LOGD("clr_bitmask=0x%x, fw_own=%d, bypass set",
				user_clr_bitmask, is_fw_own);
		}
		return;
	}

	/* TODO: Add mutex due to possibe for multi-task */
	set_okay = gps_mcudl_hal_mcu_set_fw_own();
	if (!set_okay)
		MDL_LOGW("set_okay = %d", set_okay);
	else
		MDL_LOGD("set_okay = %d", set_okay);
}

void gps_mcudl_hal_user_fw_own_deinit(enum gps_mcudl_fw_own_ctrl_user user)
{
	gps_mcul_hal_user_fw_own_lock();
	g_gps_mcudl_fw_own_ctx.init_done = false;
	gps_mcul_hal_user_fw_own_unlock();
}

void gps_mcudl_hal_user_fw_own_status_dump(void)
{
	MDL_LOGW("TBD");
}

void gps_mcudl_hal_set_non_lppm_sleep_flag(bool enable)
{
	g_gps_mcudl_hal_non_lppm_sleep_flag_ctrl = enable;
}

void gps_mcudl_hal_sync_non_flag_lppm_sleep_flag(void)
{
	g_gps_mcudl_hal_non_lppm_sleep_flag_used =
		g_gps_mcudl_hal_non_lppm_sleep_flag_ctrl;
}

bool gps_mcudl_hal_get_non_lppm_sleep_flag(void)
{
	return g_gps_mcudl_hal_non_lppm_sleep_flag_used;
}

