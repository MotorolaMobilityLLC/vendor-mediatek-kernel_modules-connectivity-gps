/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#include "gps_mcudl_reset.h"
#include "gps_mcudl_context.h"
#include "gps_mcudl_xlink.h"
#include "gps_mcudl_each_link.h"
#include "gps_mcudl_link_state.h"
#include "gps_mcudl_link_sync.h"
#include "gps_mcudl_link_util.h"
#include "gps_mcudl_log.h"
#include "gps_dl_name_list.h"
#include "conninfra.h"
#include "connsys_coredump.h"

#if 1
enum GDL_RET_STATUS gps_mcudl_reset_level_set_and_trigger(
	enum gps_each_link_reset_level level, bool wait_reset_done)
{
	enum gps_mcudl_xid x_id;
	struct gps_mcudl_each_link *p = NULL;
	enum gps_each_link_state_enum old_state, new_state;
	enum gps_each_link_reset_level old_level, new_level;
	bool need_wait[GPS_MDLX_CH_NUM] = {false};
	bool to_send_reset_event = false;
	long sigval;
	enum GDL_RET_STATUS wait_status;

	if (level != GPS_DL_RESET_LEVEL_GPS_SUBSYS && level !=  GPS_DL_RESET_LEVEL_CONNSYS) {
		GDL_LOGW("level = %d, do nothing and return", level);
		return GDL_FAIL_INVAL;
	}

	if (wait_reset_done)
		; /* TODO: take mutex to allow pending more waiter */

	for (x_id = 0; x_id < GPS_MDLX_CH_NUM; x_id++) {
		p = gps_mcudl_link_get(x_id);
		to_send_reset_event = false;

		gps_mcudl_each_link_spin_lock_take(x_id, GPS_DL_SPINLOCK_FOR_LINK_STATE);
		old_state = p->state_for_user;
		old_level = p->reset_level;

		switch (old_state) {
		case LINK_CLOSED:
			need_wait[x_id] = false;
			p->state_for_user = LINK_DISABLED;
			p->reset_level = level;

			/* Send reset event to ctld:
			 *
			 * for GPS_DL_RESET_LEVEL_GPS_SUBSYS ctrld do nothing but
			 *   just change state from DISABLED back to CLOSED
			 *
			 * for GPS_DL_RESET_LEVEL_CONNSYS ctrld do nothing but
			 *   just change state from DISABLED state back to CLOSED
			 */
			to_send_reset_event = true;
			break;

		case LINK_OPENING:
		case LINK_OPENED:
		case LINK_CLOSING:
		case LINK_RESET_DONE:
		case LINK_RESUMING:
		case LINK_SUSPENDING:
		case LINK_SUSPENDED:
			need_wait[x_id] = true;
			p->state_for_user = LINK_RESETTING;
			p->reset_level = level;
			to_send_reset_event = true;
			break;

		case LINK_RESETTING:
			need_wait[x_id] = true;
			if (old_level < level)
				p->reset_level = level;
			break;

		case LINK_DISABLED:
		case LINK_UNINIT:
			need_wait[x_id] = false;
			break;

		default:
			need_wait[x_id] = false;
			break;
		}

		new_state = p->state_for_user;
		new_level = p->reset_level;
		gps_mcudl_each_link_spin_lock_give(x_id, GPS_DL_SPINLOCK_FOR_LINK_STATE);

		if (to_send_reset_event) {
			gps_mcudl_each_link_waitable_reset(x_id, GPS_DL_WAIT_RESET);
			if (level == GPS_DL_RESET_LEVEL_CONNSYS)
				gps_mcudl_xlink_event_send(x_id, GPS_MCUDL_EVT_LINK_PRE_CONN_RESET);
			/*lse*/
			/*	gps_mcudl_xlink_event_send(x_id, GPS_MCUDL_EVT_LINK_RESET_GPS);*/
		}

		MDL_LOGXE_STA(x_id,
			"state change: %s -> %s, level = %d (%d -> %d), is_sent = %d, to_wait = %d",
			gps_dl_link_state_name(old_state), gps_dl_link_state_name(new_state),
			level, old_level, new_level,
			to_send_reset_event, need_wait[x_id]);
	}

	if (!wait_reset_done) {
		GDL_LOGE("force no wait");
		return GDL_OKAY;
	}

	for (x_id = 0; x_id < GPS_MDLX_CH_NUM; x_id++) {
		if (!need_wait[x_id])
			continue;

		sigval = 0;
		p = gps_mcudl_link_get(x_id);
		wait_status = gps_dl_link_wait_on(&p->waitables[GPS_DL_WAIT_RESET], &sigval);
		if (wait_status == GDL_FAIL_SIGNALED) {
			GDL_LOGXE(x_id, "sigval = %ld", sigval);
			return GDL_FAIL_SIGNALED;
		}

		MDL_LOGXE(x_id, "wait ret = %s", gdl_ret_to_name(wait_status));
	}

	if (wait_reset_done)
		; /* TODO: take mutex to allow pending more waiter */

	return GDL_OKAY;
}

void gps_mcudl_handle_connsys_reset_done(void)
{
	enum gps_mcudl_xid x_id;
	struct gps_mcudl_each_link *p = NULL;
	enum gps_each_link_state_enum state;
	enum gps_each_link_reset_level level;
	bool to_send_reset_event = false;

	for (x_id = 0; x_id < GPS_MDLX_CH_NUM; x_id++) {
		p = gps_mcudl_link_get(x_id);
		to_send_reset_event = false;

		gps_mcudl_each_link_spin_lock_take(x_id, GPS_DL_SPINLOCK_FOR_LINK_STATE);
		state = p->state_for_user;
		level = p->reset_level;

		if (level == GPS_DL_RESET_LEVEL_CONNSYS) {
			if (state == LINK_DISABLED || state == LINK_RESETTING)
				to_send_reset_event = true;
		}
		gps_mcudl_each_link_spin_lock_give(x_id, GPS_DL_SPINLOCK_FOR_LINK_STATE);

		if (to_send_reset_event)
			gps_mcudl_xlink_event_send(x_id, GPS_MCUDL_EVT_LINK_POST_CONN_RESET);

		MDL_LOGXE_STA(x_id, "state check: %s, level = %d, is_sent = %d",
			gps_dl_link_state_name(state), level, to_send_reset_event);
	}
}
#endif



#if 1 /* coredump*/
int gps_mcudl_coredump_is_readable(void)
{
	int ret = 1;

#if GPS_DL_HAS_CONNINFRA_DRV
	if (conninfra_reg_readable() == 0) {
		GDL_LOGI("conninfra_reg_readable: 0\n");
		ret = 0;
	}
#endif
	GDL_LOGI("gps_mcudl_coredump_is_readable: %d\n", ret);

	return ret;
}

struct coredump_event_cb g_gps_coredump_cb = {
	.reg_readable = gps_mcudl_coredump_is_readable,
	.poll_cpupcr = NULL,
};
#if GPS_DL_HAS_CONNINFRA_DRV
void *g_gps_coredump_handler;
#endif

void gps_mcudl_connsys_coredump_init(void)
{
#if GPS_DL_HAS_CONNINFRA_DRV
	g_gps_coredump_handler = connsys_coredump_init(CONNDRV_TYPE_GPS, &g_gps_coredump_cb);
	if (g_gps_coredump_handler == NULL)
		GDL_LOGW("gps_mcudl_connsys_coredump_init fail");
#endif
}

void gps_mcudl_connsys_coredump_deinit(void)
{
#if GPS_DL_HAS_CONNINFRA_DRV
	connsys_coredump_deinit(g_gps_coredump_handler);
#endif
}

void gps_mcudl_connsys_coredump_start(void)
{
#if GPS_DL_HAS_CONNINFRA_DRV
	GDL_LOGI("gps_mcudl_connsys_coredump_start");
	connsys_coredump_start(g_gps_coredump_handler, 0, CONNDRV_TYPE_GPS, "GPS");
	connsys_coredump_clean(g_gps_coredump_handler);
#endif
}
#endif

