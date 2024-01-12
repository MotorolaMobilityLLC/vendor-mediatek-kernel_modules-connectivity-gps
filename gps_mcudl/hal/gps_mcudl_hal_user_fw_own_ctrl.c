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
	unsigned int sess_id;
	unsigned int user_clr_bitmask;
	unsigned int user_clr_cnt;
	unsigned int user_clr_cnt_on_ntf_set;
	unsigned int real_clr_cnt;
	unsigned int real_set_cnt;
};

struct gps_mcudl_fw_own_user_context g_gps_mcudl_fw_own_ctx;


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
	g_gps_mcudl_fw_own_ctx.user_clr_bitmask = (1UL << user);
	g_gps_mcudl_fw_own_ctx.user_clr_cnt = 1;
	g_gps_mcudl_fw_own_ctx.user_clr_cnt_on_ntf_set = 1;
	g_gps_mcudl_fw_own_ctx.real_clr_cnt = 1;
	g_gps_mcudl_fw_own_ctx.real_set_cnt = 0;
	gps_mcul_hal_user_fw_own_unlock();
}

bool gps_mcudl_hal_user_clr_fw_own(enum gps_mcudl_fw_own_ctrl_user user)
{
	bool already_cleared = false;
	bool clear_okay = false;
	unsigned int real_clear_cnt;

	gps_mcul_hal_user_fw_own_lock();
	if (!g_gps_mcudl_fw_own_ctx.init_done) {
		gps_mcul_hal_user_fw_own_unlock();
		MDL_LOGW("[init_done=0] bypass user=%d", user);
		return false;
	}
	already_cleared = (g_gps_mcudl_fw_own_ctx.user_clr_bitmask != 0);
	real_clear_cnt = g_gps_mcudl_fw_own_ctx.real_clr_cnt + 1;
	gps_mcul_hal_user_fw_own_unlock();

	clear_okay = true;
	if (!already_cleared) {
		/* TODO: Add mutex due to possibe for multi-task */
		clear_okay = gps_mcudl_hal_mcu_clr_fw_own();
		if (!clear_okay) {
			MDL_LOGE("user=%d, real_clr_cnt=%d, clear_okay=%d",
				user, real_clear_cnt, clear_okay);
		} else {
			MDL_LOGD("user=%d, real_clr_cnt=%d, clear_okay=%d",
				user, real_clear_cnt, clear_okay);
		}
	}

	if (!clear_okay) {
#if GPS_DL_HAS_CONNINFRA_DRV
		conninfra_trigger_whole_chip_rst(
			CONNDRV_TYPE_GPS, "GNSS clear fw own fail");
#endif
	}

	gps_mcul_hal_user_fw_own_lock();
	g_gps_mcudl_fw_own_ctx.user_clr_cnt++;
	g_gps_mcudl_fw_own_ctx.user_clr_bitmask |= (1UL << user);
	if (!already_cleared)
		g_gps_mcudl_fw_own_ctx.real_clr_cnt++;
	gps_mcul_hal_user_fw_own_unlock();
	return clear_okay;
}

bool gps_mcudl_hal_user_add_if_fw_own_is_clear(enum gps_mcudl_fw_own_ctrl_user user)
{
	bool init_done;
	bool already_cleared;

	gps_mcul_hal_user_fw_own_lock();
	init_done = g_gps_mcudl_fw_own_ctx.init_done;
	already_cleared = (g_gps_mcudl_fw_own_ctx.user_clr_bitmask != 0);
	if (init_done && already_cleared) {
		g_gps_mcudl_fw_own_ctx.user_clr_bitmask |= (1UL << user);
		g_gps_mcudl_fw_own_ctx.user_clr_cnt++;
	}
	gps_mcul_hal_user_fw_own_unlock();

	if (!init_done)
		MDL_LOGW("[init_done=0] bypass user=%d", user);
	return init_done && already_cleared;
}

bool gps_mcudl_hal_user_set_fw_own_may_notify(enum gps_mcudl_fw_own_ctrl_user user)
{
	bool to_notify;

	gps_mcul_hal_user_fw_own_lock();
	if (!g_gps_mcudl_fw_own_ctx.init_done) {
		gps_mcul_hal_user_fw_own_unlock();
		MDL_LOGW("[init_done=0] bypass user=%d", user);
		return false;
	}
	g_gps_mcudl_fw_own_ctx.user_clr_bitmask &= ~(1UL << user);
	to_notify = !g_gps_mcudl_fw_own_ctx.notified_to_set;
	if (to_notify) {
		g_gps_mcudl_fw_own_ctx.user_clr_cnt_on_ntf_set = g_gps_mcudl_fw_own_ctx.user_clr_cnt;
		g_gps_mcudl_fw_own_ctx.notified_to_set = true;
	}
	gps_mcul_hal_user_fw_own_unlock();

	if (to_notify)
		gps_mcudl_ylink_event_send(GPS_MDLY_NORMAL, GPS_MCUDL_YLINK_EVT_ID_MCU_SET_FW_OWN);
	return to_notify;
}

void gps_mcudl_hal_user_set_fw_own_if_no_recent_clr(void)
{
	bool has_recent_clr = false;
	bool notified_to_set = false;
	bool user_clr_bitmask = false;
	bool set_okay = false;
	unsigned user_clr_cnt;
	unsigned user_clr_cnt_on_ntf_set;

	gps_mcul_hal_user_fw_own_lock();
	if (!g_gps_mcudl_fw_own_ctx.init_done) {
		gps_mcul_hal_user_fw_own_unlock();
		MDL_LOGW("[init_done=0] bypass");
		return;
	}
	user_clr_bitmask = g_gps_mcudl_fw_own_ctx.user_clr_bitmask;
	notified_to_set = g_gps_mcudl_fw_own_ctx.notified_to_set;
	user_clr_cnt = g_gps_mcudl_fw_own_ctx.user_clr_cnt;
	user_clr_cnt_on_ntf_set = g_gps_mcudl_fw_own_ctx.user_clr_cnt_on_ntf_set;
	has_recent_clr =
		((user_clr_cnt != user_clr_cnt_on_ntf_set) && notified_to_set && (user_clr_bitmask == 0));
	if (has_recent_clr) {
		g_gps_mcudl_fw_own_ctx.user_clr_cnt_on_ntf_set = g_gps_mcudl_fw_own_ctx.user_clr_cnt;
		g_gps_mcudl_fw_own_ctx.notified_to_set = true;
	} else {
		g_gps_mcudl_fw_own_ctx.real_set_cnt++;
		g_gps_mcudl_fw_own_ctx.notified_to_set = false;
	}
	gps_mcul_hal_user_fw_own_unlock();

	if (has_recent_clr) {
		MDL_LOGI("has_recent_clr=%d, user=0x%x, ntf=%d, cnt=%d,%d",
			has_recent_clr, user_clr_bitmask, notified_to_set,
			user_clr_cnt_on_ntf_set, user_clr_cnt);
		gps_mcudl_ylink_event_send(GPS_MDLY_NORMAL, GPS_MCUDL_YLINK_EVT_ID_MCU_SET_FW_OWN);
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

