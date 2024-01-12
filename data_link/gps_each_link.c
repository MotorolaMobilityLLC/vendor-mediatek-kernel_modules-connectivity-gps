/*
 * Copyright (C) 2019 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */
#include "gps_dl_config.h"
#include "gps_dl_time_tick.h"

#include "gps_each_link.h"
#include "gps_dl_hal.h"
#include "gps_dl_hal_api.h"
#include "gps_dl_hal_util.h"
#include "gps_dl_hw_api.h"
#include "gps_dl_isr.h"
#include "gps_dl_lib_misc.h"
#include "gps_dsp_fsm.h"
#include "gps_dl_osal.h"
#include "gps_dl_name_list.h"
#include "linux/jiffies.h"

#include "linux/errno.h"
#if GPS_DL_HAS_PLAT_DRV
#include "gps_dl_linux_plat_drv.h"
#endif

void gps_each_link_set_bool_flag(enum gps_dl_link_id_enum link_id,
	enum gps_each_link_bool_state name, bool value)
{
	struct gps_each_link *p = gps_dl_link_get(link_id);

	if (!p)
		return;

	gps_each_link_spin_lock_take(link_id, GPS_DL_SPINLOCK_FOR_LINK_STATE);
	switch (name) {
	case LINK_TO_BE_CLOSED:
		p->sub_states.to_be_closed = value;
		break;
	case LINK_USER_OPEN:
		p->sub_states.user_open = value;
		break;
	case LINK_OPEN_RESULT_OKAY:
		p->sub_states.open_result_okay = value;
		break;
	default:
		break; /* do nothing */
	}
	gps_each_link_spin_lock_give(link_id, GPS_DL_SPINLOCK_FOR_LINK_STATE);
}

bool gps_each_link_get_bool_flag(enum gps_dl_link_id_enum link_id,
	enum gps_each_link_bool_state name)
{
	struct gps_each_link *p = gps_dl_link_get(link_id);
	bool value = false;

	if (!p)
		return false;

	gps_each_link_spin_lock_take(link_id, GPS_DL_SPINLOCK_FOR_LINK_STATE);
	switch (name) {
	case LINK_TO_BE_CLOSED:
		value = p->sub_states.to_be_closed;
		break;
	case LINK_USER_OPEN:
		value = p->sub_states.user_open;
		break;
	case LINK_OPEN_RESULT_OKAY:
		value = p->sub_states.open_result_okay;
		break;
	default:
		break; /* TODO: warning it */
	}
	gps_each_link_spin_lock_give(link_id, GPS_DL_SPINLOCK_FOR_LINK_STATE);

	return value;
}

void gps_dl_link_set_ready_to_write(enum gps_dl_link_id_enum link_id, bool is_ready)
{
	struct gps_each_link *p = gps_dl_link_get(link_id);

	gps_each_link_spin_lock_take(link_id, GPS_DL_SPINLOCK_FOR_LINK_STATE);
	if (p)
		p->sub_states.is_ready_to_write = is_ready;
	gps_each_link_spin_lock_give(link_id, GPS_DL_SPINLOCK_FOR_LINK_STATE);
}

bool gps_dl_link_is_ready_to_write(enum gps_dl_link_id_enum link_id)
{
	struct gps_each_link *p = gps_dl_link_get(link_id);
	bool ready;

	gps_each_link_spin_lock_take(link_id, GPS_DL_SPINLOCK_FOR_LINK_STATE);
	if (p)
		ready = p->sub_states.is_ready_to_write;
	else
		ready = false;
	gps_each_link_spin_lock_give(link_id, GPS_DL_SPINLOCK_FOR_LINK_STATE);

	return ready;
}

void gps_each_link_set_active(enum gps_dl_link_id_enum link_id, bool is_active)
{
	struct gps_each_link *p = gps_dl_link_get(link_id);

	gps_each_link_spin_lock_take(link_id, GPS_DL_SPINLOCK_FOR_LINK_STATE);
	if (p)
		p->sub_states.is_active = is_active;
	gps_each_link_spin_lock_give(link_id, GPS_DL_SPINLOCK_FOR_LINK_STATE);
}

bool gps_each_link_is_active(enum gps_dl_link_id_enum link_id)
{
	struct gps_each_link *p = gps_dl_link_get(link_id);
	bool ready;

	gps_each_link_spin_lock_take(link_id, GPS_DL_SPINLOCK_FOR_LINK_STATE);
	if (p)
		ready = p->sub_states.is_active;
	else
		ready = false;
	gps_each_link_spin_lock_give(link_id, GPS_DL_SPINLOCK_FOR_LINK_STATE);

	return ready;
}

void gps_each_link_inc_session_id(enum gps_dl_link_id_enum link_id)
{
	struct gps_each_link *p = gps_dl_link_get(link_id);
	int sid;

	gps_each_link_spin_lock_take(link_id, GPS_DL_SPINLOCK_FOR_LINK_STATE);
	if (p->session_id >= GPS_EACH_LINK_SID_MAX)
		p->session_id = 1;
	else
		p->session_id++;
	sid = p->session_id;
	gps_each_link_spin_lock_give(link_id, GPS_DL_SPINLOCK_FOR_LINK_STATE);

	GDL_LOGXD(link_id, "new sid = %d", sid);
}

int gps_each_link_get_session_id(enum gps_dl_link_id_enum link_id)
{
	struct gps_each_link *p = gps_dl_link_get(link_id);
	int sid;

	gps_each_link_spin_lock_take(link_id, GPS_DL_SPINLOCK_FOR_LINK_STATE);
	sid = p->session_id;
	gps_each_link_spin_lock_give(link_id, GPS_DL_SPINLOCK_FOR_LINK_STATE);

	return sid;
}

void gps_dl_link_open_wait(enum gps_dl_link_id_enum link_id, long *p_sigval)
{
	struct gps_each_link *p = gps_dl_link_get(link_id);
	enum GDL_RET_STATUS gdl_ret;
	long sigval;

	gdl_ret = gps_dl_link_wait_on(&p->waitables[GPS_DL_WAIT_OPEN_CLOSE], &sigval);
	if (gdl_ret == GDL_FAIL_SIGNALED) {
		if (p_sigval != NULL) {
			*p_sigval = sigval;
			return;
		}
	} else if (gdl_ret == GDL_FAIL_NOT_SUPPORT)
		; /* show warnning */
}

void gps_dl_link_open_ack(enum gps_dl_link_id_enum link_id)
{
#if 0
	enum GDL_RET_STATUS gdl_ret;
	struct gdl_dma_buf_entry dma_buf_entry;
#endif
	struct gps_each_link *p = gps_dl_link_get(link_id);

	GDL_LOGXD(link_id, "");

	/* TODO: open fail case */
	gps_each_link_set_bool_flag(link_id, LINK_OPEN_RESULT_OKAY, true);
	gps_dl_link_wake_up(&p->waitables[GPS_DL_WAIT_OPEN_CLOSE]);

	gps_each_link_take_big_lock(link_id, GDL_LOCK_FOR_OPEN_DONE);
	if (gps_each_link_get_bool_flag(link_id, LINK_USER_OPEN)) {
		GDL_LOGXW(link_id, "user still online, try to change to opened");

		/* Note: if pre_status not OPENING, it might be RESETTING, not handle it here */
		gps_each_link_change_state_from(link_id, LINK_OPENING, LINK_OPENED);

		/* TODO: ack on DSP reset done */
#if 0
		/* if has pending data, can send it now */
		gdl_ret = gdl_dma_buf_get_data_entry(&p->tx_dma_buf, &dma_buf_entry);
		if (gdl_ret == GDL_OKAY)
			gps_dl_hal_a2d_tx_dma_start(link_id, &dma_buf_entry);
#endif
	} else {
		GDL_LOGXW(link_id, "user already offline, try to change to closing");

		/* Note: if pre_status not OPENING, it might be RESETTING, not handle it here */
		if (gps_each_link_change_state_from(link_id, LINK_OPENING, LINK_CLOSING)) {
			gps_each_link_set_bool_flag(link_id, LINK_TO_BE_CLOSED, true);
			gps_dl_link_event_send(GPS_DL_EVT_LINK_CLOSE, link_id);
		}
	}
	gps_each_link_give_big_lock(link_id);
}

void gps_each_link_init(enum gps_dl_link_id_enum link_id)
{
	struct gps_each_link *p = gps_dl_link_get(link_id);

	p->session_id = 0;
	gps_each_link_mutexes_init(p);
	gps_each_link_spin_locks_init(p);
	gps_each_link_set_active(link_id, false);
	gps_dl_link_set_ready_to_write(link_id, false);
	gps_each_link_context_init(link_id);
	gps_each_link_set_state(link_id, LINK_CLOSED);
}

void gps_each_link_deinit(enum gps_dl_link_id_enum link_id)
{
	struct gps_each_link *p = gps_dl_link_get(link_id);

	gps_each_link_set_state(link_id, LINK_UNINIT);
	gps_each_link_mutexes_deinit(p);
	gps_each_link_spin_locks_deinit(p);
}

void gps_each_link_context_init(enum gps_dl_link_id_enum link_id)
{
	enum gps_each_link_waitable_type j;

	for (j = 0; j < GPS_DL_WAIT_NUM; j++)
		gps_dl_link_waitable_reset(link_id, j);
}

void gps_each_link_context_clear(enum gps_dl_link_id_enum link_id)
{
	gps_dl_link_waitable_reset(link_id, GPS_DL_WAIT_WRITE);
	gps_dl_link_waitable_reset(link_id, GPS_DL_WAIT_READ);
}

int gps_each_link_open(enum gps_dl_link_id_enum link_id)
{
	enum gps_each_link_state_enum state, state2;
	enum GDL_RET_STATUS gdl_ret;
	long sigval = 0;
	bool okay;
	int retval;
#if GPS_DL_ON_CTP
	/* Todo: is it need on LINUX? */
	struct gps_each_link *p_link = gps_dl_link_get(link_id);

	gps_dma_buf_align_as_byte_mode(&p_link->tx_dma_buf);
	gps_dma_buf_align_as_byte_mode(&p_link->rx_dma_buf);
#endif

#if 0
#if (GPS_DL_ON_LINUX && !GPS_DL_NO_USE_IRQ && !GPS_DL_HW_IS_MOCK)
	if (!p_link->sub_states.irq_is_init_done) {
		gps_dl_irq_init();
		p_link->sub_states.irq_is_init_done = true;
	}
#endif
#endif

	state = gps_each_link_get_state(link_id);

	switch (state) {
	case LINK_CLOSING:
	case LINK_RESETTING:
	case LINK_DISABLED:
		retval = -EAGAIN;
		break;

	case LINK_RESET_DONE:
		/* RESET_DONE stands for user space not close me */
		retval = -EBUSY; /* twice open not allowed */
		break;

	case LINK_OPENED:
	case LINK_OPENING:
		retval = -EBUSY;; /* twice open not allowed */
		break;

	case LINK_CLOSED:
		okay = gps_each_link_change_state_from(link_id, LINK_CLOSED, LINK_OPENING);
		if (!okay) {
			retval = -EBUSY;
			break;
		}

		/* TODO: simplify the flags */
		gps_each_link_set_bool_flag(link_id, LINK_TO_BE_CLOSED, false);
		gps_each_link_set_bool_flag(link_id, LINK_USER_OPEN, true);

		gps_dl_link_waitable_reset(link_id, GPS_DL_WAIT_OPEN_CLOSE);
		gps_dl_link_event_send(GPS_DL_EVT_LINK_OPEN, link_id);
		gps_dl_link_open_wait(link_id, &sigval);

		/* TODO: Check this mutex can be removed?
		 * the possible purpose is make it's atomic from LINK_USER_OPEN and LINK_OPEN_RESULT_OKAY.
		 */
		gps_each_link_take_big_lock(link_id, GDL_LOCK_FOR_OPEN);
		if (sigval != 0) {
			gps_each_link_set_bool_flag(link_id, LINK_USER_OPEN, false);

			gdl_ret = gps_dl_link_try_wait_on(link_id, GPS_DL_WAIT_OPEN_CLOSE);
			if (gdl_ret == GDL_OKAY) {
				okay = gps_each_link_change_state_from(link_id, LINK_OPENED, LINK_CLOSING);

				/* Change okay, need to send event to trigger close */
				if (okay) {
					gps_each_link_give_big_lock(link_id);
					gps_dl_link_event_send(GPS_DL_EVT_LINK_CLOSE, link_id);
					GDL_LOGXW(link_id, "sigval = %d, corner case 1: close it", sigval);
					retval = -EBUSY;
					break;
				}

				/* Not change okay, state maybe RESETTING or RESET_DONE */
				state2 = gps_each_link_get_state(link_id);
				if (state2 == LINK_RESET_DONE)
					gps_each_link_set_state(link_id, LINK_CLOSED);

				gps_each_link_give_big_lock(link_id);
				GDL_LOGXW(link_id, "sigval = %d, corner case 2: %s",
					sigval, gps_dl_link_state_name(state2));
				retval = -EBUSY;
				break;
			}

			gps_each_link_give_big_lock(link_id);
			GDL_LOGXW(link_id, "sigval = %d, normal case", sigval);
			retval = -EINVAL;
			break;
		}

		okay = gps_each_link_get_bool_flag(link_id, LINK_OPEN_RESULT_OKAY);
		gps_each_link_give_big_lock(link_id);

		if (okay)
			retval = 0;
		else
			retval = -EBUSY;
		break;

	default:
		retval = -EINVAL;
		break;
	}

	if (retval == 0)
		GDL_LOGXD(link_id, "prev_state = %s, retval = %d", gps_dl_link_state_name(state), retval);
	else
		GDL_LOGXW(link_id, "prev_state = %s, retval = %d", gps_dl_link_state_name(state), retval);

	return retval;
}

int gps_each_link_reset(enum gps_dl_link_id_enum link_id)
{
	/*
	 * - set each link resetting flag
	 */
	enum gps_each_link_state_enum state, state2;
	bool okay;
	int retval;

	state = gps_each_link_get_state(link_id);

	switch (state) {
	case LINK_OPENING:
	case LINK_CLOSING:
	case LINK_CLOSED:
	case LINK_DISABLED:
		retval = -EBUSY;
		break;

	case LINK_RESETTING:
	case LINK_RESET_DONE:
		retval = 0;
		break;

	case LINK_OPENED:
		okay = gps_each_link_change_state_from(link_id, LINK_OPENED, LINK_RESETTING);
		if (!okay) {
			state2 = gps_each_link_get_state(link_id);

			/* Already reset or close, not trigger reseeting again */
			GDL_LOGXW(link_id, "state flip to %s - corner case", gps_dl_link_state_name(state2));
			if (state2 == LINK_RESETTING || state2 == LINK_RESET_DONE)
				retval = 0;
			else
				retval = -EBUSY;
			break;
		}

		gps_each_link_set_bool_flag(link_id, LINK_IS_RESETTING, true);

		/* no need to wait reset ack
		 * TODO: make sure message send okay
		 */
		gps_dl_link_waitable_reset(link_id, GPS_DL_WAIT_RESET);
		gps_dl_link_event_send(GPS_DL_EVT_LINK_RESET_DSP, link_id);
		retval = 0;
		break;

	default:
		retval = -EINVAL;
		break;
	}

	/* wait until cttld thread ack the status */
	if (retval == 0)
		GDL_LOGXD(link_id, "prev_state = %s, retval = %d", gps_dl_link_state_name(state), retval);
	else
		GDL_LOGXW(link_id, "prev_state = %s, retval = %d", gps_dl_link_state_name(state), retval);

	return retval;
}

void gps_dl_ctrld_set_resest_status(void)
{
	gps_each_link_set_active(GPS_DATA_LINK_ID0, false);
	gps_each_link_set_active(GPS_DATA_LINK_ID1, false);
}

void gps_dl_link_reset_ack_inner(enum gps_dl_link_id_enum link_id, bool post_conn_reset)
{
	struct gps_each_link *p = gps_dl_link_get(link_id);
	enum gps_each_link_state_enum old_state, new_state;
	enum gps_each_link_reset_level old_level, new_level;
	bool user_still_open;
	bool both_clear_done = false;

	gps_each_link_take_big_lock(link_id, GDL_LOCK_FOR_RESET_DONE);
	gps_each_link_spin_lock_take(link_id, GPS_DL_SPINLOCK_FOR_LINK_STATE);
	old_state = p->state_for_user;
	old_level = p->reset_level;
	user_still_open = p->sub_states.user_open;

	switch (old_level) {
	case GPS_DL_RESET_LEVEL_GPS_SINGLE_LINK:
		p->reset_level = GPS_DL_RESET_LEVEL_NONE;
		if (p->sub_states.user_open)
			p->state_for_user = LINK_RESET_DONE;
		else
			p->state_for_user = LINK_CLOSED;
		break;

	case GPS_DL_RESET_LEVEL_CONNSYS:
		if (!post_conn_reset)
			break;
		p->reset_level = GPS_DL_RESET_LEVEL_NONE;
		both_clear_done = gps_dl_link_try_to_clear_both_resetting_status();
		break;

	case GPS_DL_RESET_LEVEL_GPS_SUBSYS:
		p->reset_level = GPS_DL_RESET_LEVEL_NONE;
		both_clear_done = gps_dl_link_try_to_clear_both_resetting_status();
		break;

	default:
		break;
	}

	new_state = p->state_for_user;
	new_level = p->reset_level;
	gps_each_link_spin_lock_give(link_id, GPS_DL_SPINLOCK_FOR_LINK_STATE);

	/* TODO: if both clear, show another link's log */
	GDL_LOGXE(link_id,
		"state change: %s -> %s, level: %d -> %d, user = %d, post_reset = %d, both_clear = %d",
		gps_dl_link_state_name(old_state), gps_dl_link_state_name(new_state),
		old_level, new_level,
		user_still_open, post_conn_reset, both_clear_done);

	gps_each_link_give_big_lock(link_id);

	/* Note: for CONNSYS or GPS_SUBSYS RESET, here might be still RESETTING,
	 * if any other link not reset done (see both_clear_done print).
	 */
	gps_dl_link_wake_up(&p->waitables[GPS_DL_WAIT_RESET]);
}

bool gps_dl_link_try_to_clear_both_resetting_status(void)
{
	enum gps_dl_link_id_enum link_id;
	struct gps_each_link *p;

	for (link_id = 0; link_id < GPS_DATA_LINK_NUM; link_id++) {
		p = gps_dl_link_get(link_id);
		if (p->reset_level != GPS_DL_RESET_LEVEL_NONE)
			return false;
	}

	for (link_id = 0; link_id < GPS_DATA_LINK_NUM; link_id++) {
		p = gps_dl_link_get(link_id);

		if (p->sub_states.user_open)
			p->state_for_user = LINK_RESET_DONE;
		else
			p->state_for_user = LINK_CLOSED;
	}

	return true;
}

void gps_dl_link_reset_ack(enum gps_dl_link_id_enum link_id)
{
	gps_dl_link_reset_ack_inner(link_id, false);
}

void gps_dl_link_on_post_conn_reset(enum gps_dl_link_id_enum link_id)
{
	gps_dl_link_reset_ack_inner(link_id, true);
}

void gps_dl_link_close_wait(enum gps_dl_link_id_enum link_id, long *p_sigval)
{
	struct gps_each_link *p = gps_dl_link_get(link_id);
	enum GDL_RET_STATUS gdl_ret;
	long sigval;

	gdl_ret = gps_dl_link_wait_on(&p->waitables[GPS_DL_WAIT_OPEN_CLOSE], &sigval);
	if (gdl_ret == GDL_FAIL_SIGNALED) {
		if (p_sigval != NULL) {
			*p_sigval = sigval;
			return;
		}
	} else if (gdl_ret == GDL_FAIL_NOT_SUPPORT)
		; /* show warnning */
}

void gps_dl_link_close_ack(enum gps_dl_link_id_enum link_id)
{
	struct gps_each_link *p = gps_dl_link_get(link_id);

	GDL_LOGXD(link_id, "");
	gps_dl_link_wake_up(&p->waitables[GPS_DL_WAIT_OPEN_CLOSE]);

	gps_each_link_take_big_lock(link_id, GDL_LOCK_FOR_CLOSE_DONE);

	/* gps_each_link_set_state(link_id, LINK_CLOSED); */
	/* For case of reset_done */
	gps_each_link_change_state_from(link_id, LINK_CLOSING, LINK_CLOSED);

	gps_each_link_give_big_lock(link_id);
}

int gps_each_link_close(enum gps_dl_link_id_enum link_id)
{
	enum gps_each_link_state_enum state, state2;
	long sigval = 0;
	bool okay;
	int retval;

	state = gps_each_link_get_state(link_id);

	switch (state) {
	case LINK_OPENING:
	case LINK_CLOSING:
	case LINK_CLOSED:
	case LINK_DISABLED:
		/* twice close */
		/* TODO: show user open flag */
		retval = -EINVAL;
		break;

	case LINK_RESETTING:
		gps_each_link_set_bool_flag(link_id, LINK_USER_OPEN, false);
		GDL_LOGXE(link_id, "state check: %s, return close ok", gps_dl_link_state_name(state));
		/* return okay to avoid twice close */
		retval = 0;
		break;

	case LINK_RESET_DONE:
		gps_each_link_set_bool_flag(link_id, LINK_USER_OPEN, false);
		gps_each_link_set_state(link_id, LINK_CLOSED);
		retval = 0;
		break;

	case LINK_OPENED:
		gps_each_link_take_big_lock(link_id, GDL_LOCK_FOR_CLOSE);
		okay = gps_each_link_change_state_from(link_id, LINK_OPENED, LINK_CLOSING);
		gps_each_link_set_bool_flag(link_id, LINK_USER_OPEN, false);
		if (!okay) {
			state2 = gps_each_link_get_state(link_id);
			if (state2 == LINK_RESET_DONE)
				gps_each_link_set_state(link_id, LINK_CLOSED);
			else {
				gps_each_link_give_big_lock(link_id);
				GDL_LOGXW(link_id, "state check: %s, return close ok",
					gps_dl_link_state_name(state2));
			}
			gps_each_link_give_big_lock(link_id);
			retval = 0;
			break;
		}
		gps_each_link_give_big_lock(link_id);

		/* set this status, hal proc will by pass the message from the link
		 * it can make LINK_CLOSE be processed faster
		 */
		gps_each_link_set_bool_flag(link_id, LINK_TO_BE_CLOSED, true);

		/* clean the done(fired) flag before send and wait */
		gps_dl_link_waitable_reset(link_id, GPS_DL_WAIT_OPEN_CLOSE);
		gps_dl_link_event_send(GPS_DL_EVT_LINK_CLOSE, link_id);
		gps_dl_link_close_wait(link_id, &sigval);

		if (sigval) {
			retval = -EINVAL;
			break;
		}

		retval = 0;
		break;
	default:
		retval = -EINVAL;
		break;
	}

	if (retval == 0)
		GDL_LOGXD(link_id, "prev_state = %s, retval = %d", gps_dl_link_state_name(state), retval);
	else
		GDL_LOGXW(link_id, "prev_state = %s, retval = %d", gps_dl_link_state_name(state), retval);

	return retval;
}

int gps_each_link_check(enum gps_dl_link_id_enum link_id)
{
	enum gps_each_link_state_enum state;
	int retval = 0;

	state = gps_each_link_get_state(link_id);

	switch (state) {
	case LINK_OPENING:
	case LINK_CLOSING:
	case LINK_CLOSED:
	case LINK_DISABLED:
		break;

	case LINK_RESETTING:
#if 0
		if (rstflag == 1) {
			/* chip resetting */
			retval = -888;
		} else if (rstflag == 2) {
			/* chip reset end */
			retval = -889;
		} else {
			/* normal */
			retval = 0;
		}
#endif
		retval = -888;
		break;

	case LINK_RESET_DONE:
		retval = 889;
		break;

	case LINK_OPENED:
		gps_dl_link_event_send(GPS_DL_EVT_LINK_PRINT_HW_STATUS, link_id);
		break;

	default:
		break;
	}

	GDL_LOGXW(link_id, "prev_state = %s, retval = %d", gps_dl_link_state_name(state), retval);

	return retval;
}

int gps_each_link_enter_dsleep(enum gps_dl_link_id_enum link_id)
{
	return 0;
}

int gps_each_link_leave_dsleep(enum gps_dl_link_id_enum link_id)
{
	return 0;
}


int gps_each_link_enter_dstop(enum gps_dl_link_id_enum link_id)
{
	return 0;
}

int gps_each_link_leave_dstop(enum gps_dl_link_id_enum link_id)
{
	return 0;
}

void gps_dl_link_waitable_init(struct gps_each_link_waitable *p,
	enum gps_each_link_waitable_type type)
{
	p->type = type;
	p->fired = false;
#if GPS_DL_ON_LINUX
	init_waitqueue_head(&p->wq);
#endif
}

void gps_dl_link_waitable_reset(enum gps_dl_link_id_enum link_id, enum gps_each_link_waitable_type type)
{
	struct gps_each_link *p_link = gps_dl_link_get(link_id);

	/* TOOD: check NULL and boundary */
	p_link->waitables[type].fired = false;
}

#define GDL_TEST_TRUE_AND_SET_FALSE(x, x_old) \
	do {                \
		x_old = x;      \
		if (x_old) {    \
			x = false;  \
	} } while (0)

#define GDL_TEST_FALSE_AND_SET_TRUE(x, x_old) \
	do {                \
		x_old = x;      \
		if (!x_old) {   \
			x = true;   \
	} } while (0)

enum GDL_RET_STATUS gps_dl_link_wait_on(struct gps_each_link_waitable *p, long *p_sigval)
{
#if GPS_DL_ON_LINUX
	long val;
	bool is_fired;

	p->waiting = true;
	/* TODO: check race conditions? */
	GDL_TEST_TRUE_AND_SET_FALSE(p->fired, is_fired);
	if (is_fired) {
		GDL_LOGD("waitable = %s, no wait return", gps_dl_waitable_type_name(p->type));
		p->waiting = false;
		return GDL_OKAY;
	}

	GDL_LOGD("waitable = %s, wait start", gps_dl_waitable_type_name(p->type));
	val = wait_event_interruptible(p->wq, p->fired);
	p->waiting = false;

	if (val) {
		GDL_LOGI("signaled by %ld", val);
		if (p_sigval)
			*p_sigval = val;
		p->waiting = false;
		return GDL_FAIL_SIGNALED;
	}

	p->fired = false;
	p->waiting = false;
	GDL_LOGD("waitable = %s, wait done", gps_dl_waitable_type_name(p->type));
	return GDL_OKAY;
#else
	return GDL_FAIL_NOT_SUPPORT;
#endif
}

enum GDL_RET_STATUS gps_dl_link_try_wait_on(enum gps_dl_link_id_enum link_id,
	enum gps_each_link_waitable_type type)
{
	struct gps_each_link *p_link;
	struct gps_each_link_waitable *p;
	bool is_fired;

	p_link = gps_dl_link_get(link_id);
	p = &p_link->waitables[type];

	GDL_TEST_TRUE_AND_SET_FALSE(p->fired, is_fired);
	if (is_fired) {
		GDL_LOGD("waitable = %s, okay", gps_dl_waitable_type_name(p->type));
		p->waiting = false;
		return GDL_OKAY;
	}

	return GDL_FAIL;
}

void gps_dl_link_wake_up(struct gps_each_link_waitable *p)
{
	bool is_fired;

	ASSERT_NOT_NULL(p, GDL_VOIDF());

	if (!p->waiting) {
		GDL_LOGW("waitable = %s, nobody waiting", gps_dl_waitable_type_name(p->type));

		/* return;
		 * not return, just show warning
		 */
	}

	GDL_TEST_FALSE_AND_SET_TRUE(p->fired, is_fired);
	GDL_LOGD("waitable = %s, fired = %d", gps_dl_waitable_type_name(p->type), is_fired);

	if (!is_fired) {
#if GPS_DL_ON_LINUX
		wake_up(&p->wq);
#else
#endif
	}
}

/* TODO: determine return value type */
int gps_each_link_write(enum gps_dl_link_id_enum link_id,
	unsigned char *buf, unsigned int len)
{
	return gps_each_link_write_with_opt(link_id, buf, len, true);
}

int gps_each_link_write_with_opt(enum gps_dl_link_id_enum link_id,
	unsigned char *buf, unsigned int len, bool wait_tx_done)
{
	struct gps_each_link *p = gps_dl_link_get(link_id);
	enum GDL_RET_STATUS gdl_ret;
	long sigval = 0;

	if (NULL == p)
		return -1;

	if (len > p->tx_dma_buf.len)
		return -1;

	if (gps_each_link_get_state(link_id) != LINK_OPENED) {
		GDL_LOGXW(link_id, "not opened, drop the write data len = %d", link_id);
		return -EBUSY;
	}

	while (1) {
		gdl_ret = gdl_dma_buf_put(&p->tx_dma_buf, buf, len);

		if (gdl_ret == GDL_OKAY) {
			gps_dl_link_event_send(GPS_DL_EVT_LINK_WRITE, link_id);
#if (GPS_DL_NO_USE_IRQ == 1)
			if (wait_tx_done) {
				do {
					gps_dl_hal_a2d_tx_dma_wait_until_done_and_stop_it(
						link_id, GPS_DL_RW_NO_TIMEOUT);
					gps_dl_hal_event_send(GPS_DL_HAL_EVT_A2D_TX_DMA_DONE, link_id);
					/* for case tx transfer_max > 0, GPS_DL_HAL_EVT_A2D_TX_DMA_DONE may */
					/* start anthor dma session again, need to loop again until all data done */
				} while (!gps_dma_buf_is_empty(&p->tx_dma_buf));
			}
#endif
			return 0;
		} else if (gdl_ret == GDL_FAIL_NOSPACE || gdl_ret == GDL_FAIL_BUSY) {
			/* TODO: */
			/* 1. note: BUSY stands for others thread is do write, it should be impossible */
			/* - If wait on BUSY, should wake up the waitings or return eno_again? */
			/* 2. note: NOSPACE stands for need wait for tx dma working done */
			gps_dma_buf_show(&p->tx_dma_buf);
			GDL_LOGI("gdl_dma_buf_put wait due to %s", gdl_ret_to_name(gdl_ret));
			gdl_ret = gps_dl_link_wait_on(&p->waitables[GPS_DL_WAIT_WRITE], &sigval);
			if (gdl_ret == GDL_FAIL_SIGNALED)
				return -1;
		} else {
			GDL_LOGI("gdl_dma_buf_put fail %s", gdl_ret_to_name(gdl_ret));
			return -1;
		}
	}

	return -1;
}

#define GPS_DL_READ_SHOW_BUF_MAX_LEN (32)
int gps_each_link_read(enum gps_dl_link_id_enum link_id,
	unsigned char *buf, unsigned int len) {
	return gps_each_link_read_with_timeout(link_id, buf, len, GPS_DL_RW_NO_TIMEOUT, NULL);
}

int gps_each_link_read_with_timeout(enum gps_dl_link_id_enum link_id,
	unsigned char *buf, unsigned int len, int timeout_usec, bool *p_is_nodata)
{
	struct gps_each_link *p = gps_dl_link_get(link_id);
	enum GDL_RET_STATUS gdl_ret;
#if (GPS_DL_NO_USE_IRQ == 0)
	long sigval = 0;
#endif
	unsigned int data_len;

	if (NULL == p)
		return -1;

	while (1) {
		gdl_ret = gdl_dma_buf_get(&p->rx_dma_buf, buf, len, &data_len, p_is_nodata);

		if (gdl_ret == GDL_OKAY) {
			if (data_len <= GPS_DL_READ_SHOW_BUF_MAX_LEN)
				gps_dl_hal_show_buf("read buf", buf, data_len);
			else
				GDL_LOGD("read buf, len = %d", data_len);

			gps_each_link_spin_lock_take(link_id, GPS_DL_SPINLOCK_FOR_DMA_BUF);
			if (p->rx_dma_buf.has_pending_rx) {
				p->rx_dma_buf.has_pending_rx = false;
				gps_each_link_spin_lock_give(link_id, GPS_DL_SPINLOCK_FOR_DMA_BUF);

				GDL_LOGW("has pending rx, trigger again");
				gps_dl_hal_event_send(GPS_DL_HAL_EVT_D2A_RX_HAS_DATA, link_id);
			} else
				gps_each_link_spin_lock_give(link_id, GPS_DL_SPINLOCK_FOR_DMA_BUF);

			return data_len;
		} else if (gdl_ret == GDL_FAIL_NODATA) {
			GDL_LOGI("gdl_dma_buf_get no data and wait");
#if (GPS_DL_NO_USE_IRQ == 1)
			gdl_ret = gps_dl_hal_wait_and_handle_until_usrt_has_data(
				link_id, timeout_usec);
			if (gdl_ret == GDL_FAIL_TIMEOUT)
				return -1;

			gdl_ret = gps_dl_hal_wait_and_handle_until_usrt_has_nodata_or_rx_dma_done(
				link_id, timeout_usec);
			if (gdl_ret == GDL_FAIL_TIMEOUT)
				return -1;
			continue;
#else
			gdl_ret = gps_dl_link_wait_on(&p->waitables[GPS_DL_WAIT_READ], &sigval);
			if (gdl_ret == GDL_FAIL_SIGNALED || gdl_ret == GDL_FAIL_NOT_SUPPORT)
				return -1;
#endif
		} else {
			GDL_LOGI("gdl_dma_buf_get fail %s", gdl_ret_to_name(gdl_ret));
			return -1;
		}
	}

	return 0;
}

void gps_dl_link_event_send(enum gps_dl_link_event_id evt,
	enum gps_dl_link_id_enum link_id)
{
#if (GPS_DL_HAS_CTRLD == 0)
	gps_dl_link_event_proc(evt, link_id);
#else
	{
		struct gps_dl_osal_lxop *pOp;
		struct gps_dl_osal_signal *pSignal;
		int iRet;

		pOp = gps_dl_get_free_op();
		if (!pOp)
			return;

		pSignal = &pOp->signal;
		pSignal->timeoutValue = 0;/* send data need to wait ?ms */
		if (link_id < GPS_DATA_LINK_NUM) {
			pOp->op.opId = GPS_DL_OPID_LINK_EVENT_PROC;
			pOp->op.au4OpData[0] = link_id;
			pOp->op.au4OpData[1] = evt;
			iRet = gps_dl_put_act_op(pOp);
		} else {
			gps_dl_put_op_to_free_queue(pOp);
			/*printf error msg*/
			return;
		}
	}
#endif
}

void gps_dl_link_event_proc(enum gps_dl_link_event_id evt,
	enum gps_dl_link_id_enum link_id)
{
	struct gps_each_link *p_link = gps_dl_link_get(link_id);
	struct gdl_dma_buf_entry dma_buf_entry;
	enum GDL_RET_STATUS gdl_ret;
	bool dma_working, pending_rx;
	unsigned long j0, j1;

	j0 = jiffies;
	GDL_LOGXD(link_id, "evt = %s", gps_dl_link_event_name(evt));

	switch (evt) {
	case GPS_DL_EVT_LINK_OPEN:
		gps_each_dsp_reg_gourp_read_init(link_id);
		gps_each_link_inc_session_id(link_id);
		gps_each_link_set_active(link_id, true);
#if GPS_DL_HAS_PLAT_DRV
		gps_dl_lna_pin_ctrl(link_id, true, false);
#endif
		gps_dl_hal_conn_power_ctrl(link_id, 1);
		gps_dl_hal_link_power_ctrl(link_id, 1);
		gps_dl_irq_each_link_unmask(link_id, GPS_DL_IRQ_TYPE_HAS_DATA, GPS_DL_IRQ_CTRL_FROM_THREAD);
		gps_dl_irq_each_link_unmask(link_id, GPS_DL_IRQ_TYPE_HAS_NODATA, GPS_DL_IRQ_CTRL_FROM_THREAD);
		gps_dsp_fsm(GPS_DSP_EVT_FUNC_ON, link_id);

		/* check if MCUB ROM ready */
		gps_dl_hal_mcub_flag_handler(link_id);
		gps_dl_irq_each_link_unmask(link_id, GPS_DL_IRQ_TYPE_MCUB, GPS_DL_IRQ_CTRL_FROM_THREAD);
#if GPS_DL_NO_USE_IRQ
		gps_dl_wait_us(1000); /* wait 1ms */
#endif
		gps_dl_link_open_ack(link_id); /* TODO: ack on DSP reset done */
		gps_dl_link_set_ready_to_write(link_id, true); /* TODO: make it writable on DSP reset done */
		break;

	case GPS_DL_EVT_LINK_DSP_ROM_READY_TIMEOUT:
		/* check again mcub not ready triggered */
		if (false)
			break; /* wait hal handle it */

		/* true: */
		if (!gps_each_link_change_state_from(link_id, LINK_OPENED, LINK_RESETTING)) {
			/* no handle it again */
			break;
		}
		/* go and do close */
	case GPS_DL_EVT_LINK_CLOSE:
	case GPS_DL_EVT_LINK_RESET_DSP:
	case GPS_DL_EVT_LINK_RESET_GPS:
	case GPS_DL_EVT_LINK_PRE_CONN_RESET:
		/* TODO: avoid twice enter */

		if (GPS_DSP_ST_OFF == gps_dsp_state_get(link_id)) {
			GDL_LOGXD(link_id, "dsp state is off, do nothing for %s",
				gps_dl_link_event_name(evt));

			if (GPS_DL_EVT_LINK_CLOSE == evt)
				gps_dl_link_close_ack(link_id); /* TODO: check fired race */
			else
				gps_dl_link_reset_ack(link_id);

			break;
		}

		/* TODO: Corresponding DMA must be stopped before turn off L1 or L5 power */
		/* 1. New DMA transfer can not be started */
		/* 2. Mask DMA interrupt if possible, when another DSP not working */
		/* Note: gps_dl_hal_conn_pre_power_ctrl might need!!! */
		/* 3. If DMA is working, must stop it when it at proper status (Check with Jingwu) */
		/* A state machine might be introduced to make sure safe stopping */

		/* TODO: make sure all DSP is top and interrupt is disabled / marked */

		/* Note: the order to mask might be important */
		/* TODO: avoid twice mask need to be handled */
		gps_each_link_set_active(link_id, false);
#if GPS_DL_HAS_PLAT_DRV
		gps_dl_lna_pin_ctrl(link_id, false, false);
#endif
		gps_dl_link_set_ready_to_write(link_id, false);
		gps_dl_irq_each_link_mask(link_id, GPS_DL_IRQ_TYPE_MCUB, GPS_DL_IRQ_CTRL_FROM_THREAD);

		gps_each_link_spin_lock_take(link_id, GPS_DL_SPINLOCK_FOR_DMA_BUF);
		dma_working = p_link->rx_dma_buf.dma_working_entry.is_valid;
		pending_rx = p_link->rx_dma_buf.has_pending_rx;
		if (dma_working || pending_rx) {
			p_link->rx_dma_buf.has_pending_rx = false;
			gps_each_link_spin_lock_give(link_id, GPS_DL_SPINLOCK_FOR_DMA_BUF);

			/* It means this irq has already masked, */
			/* DON'T mask again, otherwise twice unmask might be needed */
			GDL_LOGXD(link_id,
				"has dma_working = %d, pending rx = %d, by pass mask the this IRQ",
				dma_working, pending_rx);
		} else {
			gps_each_link_spin_lock_give(link_id, GPS_DL_SPINLOCK_FOR_DMA_BUF);
			gps_dl_irq_each_link_mask(link_id, GPS_DL_IRQ_TYPE_HAS_DATA, GPS_DL_IRQ_CTRL_FROM_THREAD);
		}

		/* TODO: avoid twice mask need to be handled if HAS_CTRLD */
		gps_dl_irq_each_link_mask(link_id, GPS_DL_IRQ_TYPE_HAS_NODATA, GPS_DL_IRQ_CTRL_FROM_THREAD);

		gps_dl_hal_link_power_ctrl(link_id, 0);
		gps_dl_hal_conn_power_ctrl(link_id, 0);

		gps_dsp_fsm(GPS_DSP_EVT_FUNC_OFF, link_id);

		gps_each_link_context_clear(link_id);
#if GPS_DL_ON_LINUX
		gps_dma_buf_reset(&p_link->tx_dma_buf);
		gps_dma_buf_reset(&p_link->rx_dma_buf);
#endif

		if (GPS_DL_EVT_LINK_CLOSE == evt)
			gps_dl_link_close_ack(link_id); /* TODO: check fired race */
		else
			gps_dl_link_reset_ack(link_id);
		break;

	case GPS_DL_EVT_LINK_POST_CONN_RESET:
		gps_dl_link_on_post_conn_reset(link_id);
		break;

	case GPS_DL_EVT_LINK_WRITE:
		if (!gps_dl_link_is_ready_to_write(link_id)) {
			GDL_LOGXW(link_id, "too early writing");
			break;
		}

		gdl_ret = gdl_dma_buf_get_data_entry(&p_link->tx_dma_buf, &dma_buf_entry);
		if (gdl_ret == GDL_OKAY)
			gps_dl_hal_a2d_tx_dma_start(link_id, &dma_buf_entry);
		break;

	case GPS_DL_EVT_LINK_PRINT_HW_STATUS:
		gps_dl_hw_print_hw_status(link_id);
		gps_each_dsp_reg_gourp_read_start(link_id);
		break;

	case GPS_DL_EVT_LINK_DSP_FSM_TIMEOUT:
		gps_dsp_fsm(GPS_DSP_EVT_CTRL_TIMER_EXPIRE, link_id);
		break;
#if 0
	case GPS_DL_EVT_LINK_RESET_GPS:
		/* turn off GPS power directly */
		break;

	case GPS_DL_EVT_LINK_PRE_CONN_RESET:
		/* turn off Connsys power directly
		 * 1. no need to do anything, just make sure the message queue is empty
		 * 2. how to handle ctrld block issue
		 */
		/* gps_dl_link_open_ack(link_id); */
		break;
#endif
	default:
		break;
	}

	j1 = jiffies;
	GDL_LOGXD(link_id, "evt2 = %s, dj = %u", gps_dl_link_event_name(evt), j1 - j0);
}

int gps_each_link_send_data(char *buffer, unsigned int len, unsigned int data_link_num)
{
#if GPS_DL_HAS_CTRLD
	struct gps_dl_osal_lxop *pOp;
	struct gps_dl_osal_signal *pSignal;
	int iRet;

	pOp = gps_dl_get_free_op();
	if (!pOp)
		return -1;

	pSignal = &pOp->signal;
	pSignal->timeoutValue = 0;/* send data need to wait ?ms */
	if (data_link_num == 0)
		pOp->op.opId = GPS_DL_OPID_SEND_DATA_DL0;
	else if (data_link_num == 1)
		pOp->op.opId = GPS_DL_OPID_SEND_DATA_DL1;
	else {
		gps_dl_put_op_to_free_queue(pOp);
		/*printf error msg*/
		return -1;
	}
	pOp->op.au4OpData[0] = (unsigned long)buffer;
	pOp->op.au4OpData[1] = len;
	iRet = gps_dl_put_act_op(pOp);
#endif
	return 0;
}

int gps_each_link_power_control(bool on_off, unsigned int data_link_num)
{
#if GPS_DL_HAS_CTRLD
	struct gps_dl_osal_lxop *pOp;
	struct gps_dl_osal_signal *pSignal;
	int iRet;

	pOp = gps_dl_get_free_op();
	if (!pOp)
		return -1;

	pSignal = &pOp->signal;
	pSignal->timeoutValue = 0;/* power on/off need to wait ?ms */
	if ((data_link_num == 0) && on_off)
		pOp->op.opId = GPS_DL_OPID_PWR_ON_DL0;
	else if ((data_link_num == 0) && (!on_off))
		pOp->op.opId = GPS_DL_OPID_PWR_OFF_DL0;
	else if ((data_link_num == 1) && on_off)
		pOp->op.opId = GPS_DL_OPID_PWR_ON_DL1;
	else if ((data_link_num == 1) && (!on_off))
		pOp->op.opId = GPS_DL_OPID_PWR_OFF_DL1;
	else {
		gps_dl_put_op_to_free_queue(pOp);
		/*printf error msg*/
		return -1;
	}

	iRet = gps_dl_put_act_op(pOp);
#endif
	return 0;
}

int gps_each_link_receive_data(char *buffer, unsigned int length, unsigned int data_link_num)
{
#if GPS_DL_HAS_CTRLD
	return gps_dl_receive_data(buffer, length, data_link_num);
#else
	return -1;
#endif
}

#if GPS_DL_HAS_CTRLD
int gps_each_link_register_event_cb(GPS_DL_RX_EVENT_CB func)
{
	return gps_dl_register_rx_event_cb(func);
}
#endif

void gps_each_link_mutexes_init(struct gps_each_link *p)
{
	enum gps_each_link_mutex i;

	for (i = 0; i < GPS_DL_MTX_NUM; i++)
		osal_sleepable_lock_init(&p->mutexes[i]);
}

void gps_each_link_mutexes_deinit(struct gps_each_link *p)
{
	enum gps_each_link_mutex i;

	for (i = 0; i < GPS_DL_MTX_NUM; i++)
		osal_sleepable_lock_deinit(&p->mutexes[i]);
}

void gps_each_link_spin_locks_init(struct gps_each_link *p)
{
	enum gps_each_link_mutex i;

	for (i = 0; i < GPS_DL_SPINLOCK_NUM; i++)
		osal_unsleepable_lock_init(&p->spin_locks[i]);
}

void gps_each_link_spin_locks_deinit(struct gps_each_link *p)
{
#if 0
	enum gps_each_link_mutex i;

	for (i = 0; i < GPS_DL_SPINLOCK_NUM; i++)
		osal_unsleepable_lock_deinit(&p->spin_locks[i]);
#endif
}

void gps_each_link_mutex_take(enum gps_dl_link_id_enum link_id, enum gps_each_link_mutex mtx_id)
{
	/* TODO: check range */
	struct gps_each_link *p = gps_dl_link_get(link_id);

	/* TODO: handle killed */
	osal_lock_sleepable_lock(&p->mutexes[mtx_id]);
}

void gps_each_link_mutex_give(enum gps_dl_link_id_enum link_id, enum gps_each_link_mutex mtx_id)
{
	/* TODO: check range */
	struct gps_each_link *p = gps_dl_link_get(link_id);

	osal_unlock_sleepable_lock(&p->mutexes[mtx_id]);
}

void gps_each_link_spin_lock_take(enum gps_dl_link_id_enum link_id, enum gps_each_link_spinlock spin_lock_id)
{
	/* TODO: check range */
	struct gps_each_link *p = gps_dl_link_get(link_id);

	osal_lock_unsleepable_lock(&p->spin_locks[spin_lock_id]);
}

void gps_each_link_spin_lock_give(enum gps_dl_link_id_enum link_id, enum gps_each_link_spinlock spin_lock_id)
{
	/* TODO: check range */
	struct gps_each_link *p = gps_dl_link_get(link_id);

	osal_unlock_unsleepable_lock(&p->spin_locks[spin_lock_id]);
}

int gps_each_link_take_big_lock(enum gps_dl_link_id_enum link_id,
	enum gps_each_link_lock_reason reason)
{
	gps_each_link_mutex_take(link_id, GPS_DL_MTX_BIG_LOCK);
	return 0;
}

int gps_each_link_give_big_lock(enum gps_dl_link_id_enum link_id)
{
	gps_each_link_mutex_give(link_id, GPS_DL_MTX_BIG_LOCK);
	return 0;
}

enum gps_each_link_state_enum gps_each_link_get_state(enum gps_dl_link_id_enum link_id)
{
	struct gps_each_link *p = gps_dl_link_get(link_id);
	enum gps_each_link_state_enum state;

	gps_each_link_spin_lock_take(link_id, GPS_DL_SPINLOCK_FOR_LINK_STATE);
	state = p->state_for_user;
	gps_each_link_spin_lock_give(link_id, GPS_DL_SPINLOCK_FOR_LINK_STATE);

	return state;
}

void gps_each_link_set_state(enum gps_dl_link_id_enum link_id, enum gps_each_link_state_enum state)
{
	struct gps_each_link *p = gps_dl_link_get(link_id);
	enum gps_each_link_state_enum pre_state;


	gps_each_link_spin_lock_take(link_id, GPS_DL_SPINLOCK_FOR_LINK_STATE);
	pre_state = p->state_for_user;
	p->state_for_user = state;
	gps_each_link_spin_lock_give(link_id, GPS_DL_SPINLOCK_FOR_LINK_STATE);

	GDL_LOGXD(link_id, "state change: %s -> %s",
		gps_dl_link_state_name(pre_state), gps_dl_link_state_name(state));
}

bool gps_each_link_change_state_from(enum gps_dl_link_id_enum link_id,
	enum gps_each_link_state_enum from, enum gps_each_link_state_enum to)
{
	bool is_okay = false;
	struct gps_each_link *p = gps_dl_link_get(link_id);
	enum gps_each_link_state_enum pre_state;

	gps_each_link_spin_lock_take(link_id, GPS_DL_SPINLOCK_FOR_LINK_STATE);
	pre_state = p->state_for_user;
	if (from == pre_state) {
		p->state_for_user = to;
		is_okay = true;
	}
	gps_each_link_spin_lock_give(link_id, GPS_DL_SPINLOCK_FOR_LINK_STATE);

	if (is_okay) {
		GDL_LOGXD(link_id, "state change: %s -> %s, okay",
			gps_dl_link_state_name(from), gps_dl_link_state_name(to));
	} else {
		GDL_LOGXW(link_id, "state change: %s -> %s, fail on pre_state = %s",
			gps_dl_link_state_name(from), gps_dl_link_state_name(to),
			gps_dl_link_state_name(pre_state));
	}

	return is_okay;
}

