/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#include "gps_dl_config.h"
#include "gps_dl_log.h"
#include "gps_mcudl_log.h"
#include "gps_mcudl_hal_conn.h"
#include "gps_mcudl_ylink.h"
#include "gps_dl_hal.h"
#include "gps_dl_hw_dep_api.h"
#include "gps_dl_time_tick.h"
#include "gps_mcudl_hw_mcu.h"
#include "gps_mcudl_hal_conn.h"
#include "gps_mcusys_fsm.h"


void gps_mcudl_hal_get_ecid_info(void)
{
	gps_dl_hw_dep_gps_control_adie_on();
	gps_dl_hw_dep_gps_get_ecid_info();
	gps_dl_hw_dep_gps_control_adie_off();
}

bool gps_mcudl_hal_dump_power_state(void)
{
	bool is_gps_awake = true;
	bool is_sw_clk_ext = false;
	struct gps_dl_power_raw_state raw;
	unsigned int L1_on_mode = 0, L1_off_mode = 0;
	unsigned int L5_on_mode = 0, L5_off_mode = 0;
	unsigned int flag = 0;
	unsigned int on_off_cnt = 0;
	gpsmdl_u32 xbitmask;

	memset(&raw, 0, sizeof(raw));
	gps_dl_hw_dep_gps_dump_power_state(&raw);
	xbitmask = gps_mcudl_ylink_get_xbitmask(GPS_MDLY_NORMAL);

	/* calculate and print readable log */

	if (raw.sw_gps_ctrl == 0x0000) {
		/* sw_gps_ctrl not support or gps dsp has not ever turned on */
		is_sw_clk_ext = false;
	} else if (raw.sw_gps_ctrl == 0xFFFF) {
		/* gps dsp is working */
		is_sw_clk_ext = false;
	} else {
		/* gps dsp is turned off */
		L1_on_mode  = ((raw.sw_gps_ctrl & 0xC000) >> 14);
		L5_on_mode  = ((raw.sw_gps_ctrl & 0x3000) >> 12);
		L1_off_mode = ((raw.sw_gps_ctrl & 0x0C00) >> 10);
		L5_off_mode = ((raw.sw_gps_ctrl & 0x0300) >>  8);
		flag        = ((raw.sw_gps_ctrl & 0x0080) >>  7);
		on_off_cnt  = ((raw.sw_gps_ctrl & 0x007F) >>  0);
		if (L1_off_mode != 0 || L5_off_mode != 0)
			is_sw_clk_ext = flag; /* dsp is in deep stop mode or clk_ext */
		else
			is_sw_clk_ext = false; /* dsp is off */
	}

	is_gps_awake = is_sw_clk_ext || raw.is_hw_clk_ext || (raw.mcu_pc != 0);

	MDL_LOGI(
		"awake=%d,mcu_pc=0x%08x,clk_ext=%d,%d,sw_ctrl=0x%04X[on=%u,%u,off=%u,%u,flag=%u,cnt=%u],xbitmask=0x%08x",
		is_gps_awake, raw.mcu_pc, raw.is_hw_clk_ext, is_sw_clk_ext, raw.sw_gps_ctrl,
		L1_on_mode, L5_on_mode, L1_off_mode, L5_off_mode, flag, on_off_cnt, xbitmask);
	return is_gps_awake;
}

#if GPS_DL_ON_LINUX
bool gps_mcudl_is_voting_for_coinninfra_on;
unsigned long gps_mcudl_vote_for_coinninfra_on_us;
unsigned long gps_mcudl_vote_phase_bitmask;

static void gps_mcudl_vote_to_deny_opp0(bool deny_opp0)
{
    /* Only MT6985 need it (from Android-T)
     * Enable it when we need to support MT6985 MP(e.g. vendor unfreeze, might be on Android-X).
     * Condition "!g_gps_common_on || gps_dl_hal_get_conn_infra_ver() == GDL_HW_CONN_INFRA_VER_MT6985"
     * can be used for runtime dispatching.
     */
#if 0
#include "dvfsrc-common.h"
	mtk_dvfsrc_dynamic_opp0(VCOREOPP_GPS, deny_opp0);
#endif
}

void gps_mcudl_set_opp_vote_phase(enum gps_mcudl_hal_opp_vote_phase phase, bool is_in)
{
	smp_mb__before_atomic();
	if (is_in)
		set_bit(phase, &gps_mcudl_vote_phase_bitmask);
	else
		clear_bit(phase, &gps_mcudl_vote_phase_bitmask);
	smp_mb__after_atomic();
}

void gps_mcudl_end_all_opp_vote_phase(void)
{
	unsigned long bitmask;

	bitmask = gps_mcudl_vote_phase_bitmask;
	if (bitmask != 0)
		GDL_LOGW("bitmask=0x%08lx is not zero", bitmask);
	gps_mcudl_vote_phase_bitmask = 0;
}

unsigned int gps_mcudl_get_opp_vote_phase_bitmask(void)
{
	unsigned long bitmask;

	bitmask = gps_mcudl_vote_phase_bitmask;
	return (unsigned int)bitmask;
}

void gps_mcudl_vote_to_deny_opp0_for_coinninfra_on(bool vote)
{
	unsigned long d_us;

	if (vote) {
		if (gps_mcudl_is_voting_for_coinninfra_on) {
			d_us = gps_dl_tick_get_us() - gps_mcudl_vote_for_coinninfra_on_us;
			GDL_LOGE("double vote true, d_us=%lu", d_us);
			return;
		}
		gps_mcudl_is_voting_for_coinninfra_on = true;
		gps_mcudl_vote_for_coinninfra_on_us = gps_dl_tick_get_us();
		gps_mcudl_vote_to_deny_opp0(true);
	} else {
		if (!gps_mcudl_is_voting_for_coinninfra_on) {
			GDL_LOGW("double vote false");
			return;
		}
		gps_mcudl_vote_to_deny_opp0(false);
		d_us = gps_dl_tick_get_us() - gps_mcudl_vote_for_coinninfra_on_us;
		gps_mcudl_is_voting_for_coinninfra_on = false;
		GDL_LOGE("abnormal vote end-1, d_us=%lu", d_us);
	}
}

bool gps_mcudl_hw_conn_force_wake(bool enable)
{
	bool wake_okay = false;
	bool conn_on_vote = gps_mcudl_is_voting_for_coinninfra_on;
	unsigned int vote_bitmask;
	unsigned long us0, d_us;
	bool conn_ver_valid = false;
	unsigned int conn_ver = gps_dl_hal_get_conn_infra_ver();
	enum gps_mcusys_gpsbin_state mcu_state = gps_mcusys_gpsbin_state_get();

	if (!enable) {
		wake_okay = gps_mcudl_hw_conn_force_wake_inner(false);
		return wake_okay;
	}

	vote_bitmask = gps_mcudl_get_opp_vote_phase_bitmask();
	if (vote_bitmask == 0) {
		wake_okay = gps_mcudl_hw_conn_force_wake_inner(true);
		if (conn_on_vote) {
			gps_mcudl_vote_to_deny_opp0(false);
			d_us = gps_dl_tick_get_us() - gps_mcudl_vote_for_coinninfra_on_us;
			gps_mcudl_is_voting_for_coinninfra_on = false;
			GDL_LOGE("abnormal vote end-2, d_us=%lu, w_ok=%d", d_us, wake_okay);
		}
		return wake_okay;
	}

	if (conn_on_vote) {
		us0 = gps_mcudl_vote_for_coinninfra_on_us;
		gps_mcudl_is_voting_for_coinninfra_on = false;
	} else {
		us0 = gps_dl_tick_get_us();
		gps_mcudl_vote_to_deny_opp0(true);
	}
	wake_okay = gps_mcudl_hw_conn_force_wake_inner(true);
	gps_mcudl_vote_to_deny_opp0(false);
	d_us = gps_dl_tick_get_us() - us0;
	conn_ver_valid = (mcu_state == GPS_MCUSYS_GPSBIN_POST_ON);
	if ((d_us >= 2000) || (!conn_ver_valid) ||
		(conn_ver == GDL_HW_CONN_INFRA_VER_MT6985)) {
		GDL_LOGW("vote end: d_us=%lu, bitmask=0x%08x, is_cv=%d, w_ok=%d, st=%d,0x%x",
			d_us, vote_bitmask, conn_on_vote, wake_okay, mcu_state, conn_ver);
	} else {
		GDL_LOGD("vote end: d_us=%lu, bitmask=0x%08x, is_cv=%d, w_ok=%d, st=%d,0x%x",
			d_us, vote_bitmask, conn_on_vote, wake_okay, mcu_state, conn_ver);
	}
	return wake_okay;
}

#else
void gps_mcudl_set_opp_vote_phase(enum gps_mcudl_hal_opp_vote_phase phase, bool is_in)
{
}

void gps_mcudl_end_all_opp_vote_phase(void)
{
}

unsigned int gps_mcudl_get_opp_vote_phase_bitmask(void)
{
	return 0;
}

void gps_mcudl_vote_to_deny_opp0_for_coinninfra_on(bool vote)
{
}

bool gps_mcudl_hw_conn_force_wake(bool enable)
{
	return gps_mcudl_hw_conn_force_wake_inner(enable);
}

#endif

