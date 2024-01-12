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

#include "linux/errno.h"

char *link_event_name[GPS_DL_LINK_EVT_NUM + 1] = {
	[GPS_DL_EVT_LINK_OPEN] = "LINK_OPEN",
	[GPS_DL_EVT_LINK_CLOSE] = "LINK_CLOSE",
	[GPS_DL_EVT_LINK_WRITE] = "LINK_WRITE",
	[GPS_DL_EVT_LINK_READ] = "LINK_READ",
	[GPS_DL_EVT_LINK_DSP_ROM_READY_TIMEOUT] = "ROM_READY_TIMEOUT",
	[GPS_DL_EVT_LINK_DSP_FSM_TIMEOUT] = "DSP_FSM_TIMEOUT",
	[GPS_DL_EVT_LINK_RESET_DSP] = "RESET_DSP",
	[GPS_DL_EVT_LINK_RESET_GPS] = "RESET_GPS",
	[GPS_DL_EVT_LINK_RESET_CONNSYS] = "RESET_CONNSYS",
	[GPS_DL_EVT_LINK_PRINT_HW_STATUS] = "PRINT_HW_STATUS",
	[GPS_DL_LINK_EVT_NUM] = "LINK_INVALID_EVT",
};

char *gps_dl_link_event_name(enum gps_dl_link_event_id evt)
{
	if (evt >= 0 && evt < GPS_DL_LINK_EVT_NUM)
		return link_event_name[evt];
	else
		return link_event_name[GPS_DL_LINK_EVT_NUM];
}

char *waitable_type_name[GPS_DL_WAIT_NUM + 1] = {
	[GPS_DL_WAIT_OPEN_CLOSE] = "OPEN_OR_CLOSE",
	[GPS_DL_WAIT_WRITE] = "WRITE",
	[GPS_DL_WAIT_READ] = "READ",
	[GPS_DL_WAIT_NUM] = "INVALID",
};

char *gps_dl_waitable_type_name(enum gps_each_link_waitable_type type)
{
	if (type >= 0 && type < GPS_DL_LINK_EVT_NUM)
		return waitable_type_name[type];
	else
		return waitable_type_name[GPS_DL_WAIT_NUM];
}


void gps_each_link_set_bool_flag(enum gps_dl_link_id_enum link_id,
	enum gps_each_link_bool_state name, bool value)
{
	struct gps_each_link *p = gps_dl_link_get(link_id);

	if (!p)
		return;

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
}

bool gps_each_link_get_bool_flag(enum gps_dl_link_id_enum link_id,
	enum gps_each_link_bool_state name)
{
	struct gps_each_link *p = gps_dl_link_get(link_id);
	bool value = false;

	if (!p)
		return false;

	switch (name) {
	case LINK_TO_BE_CLOSED:
		return p->sub_states.to_be_closed;
	case LINK_USER_OPEN:
		return p->sub_states.user_open;
	case LINK_OPEN_RESULT_OKAY:
		return p->sub_states.open_result_okay;
	default:
		break; /* TODO: warning it */
	}

	return value;
}

void gps_dl_link_set_ready_to_write(enum gps_dl_link_id_enum link_id, bool is_ready)
{
	struct gps_each_link *p = gps_dl_link_get(link_id);

	if (p)
		p->sub_states.is_ready_to_write = is_ready;
}

bool gps_dl_link_is_ready_to_write(enum gps_dl_link_id_enum link_id)
{
	struct gps_each_link *p = gps_dl_link_get(link_id);

	if (p)
		return p->sub_states.is_ready_to_write;
	else
		return false;
}

void gps_each_link_set_active(enum gps_dl_link_id_enum link_id, bool is_active)
{
	struct gps_each_link *p = gps_dl_link_get(link_id);

	if (p)
		p->sub_states.is_active = is_active;
}

bool gps_each_link_is_active(enum gps_dl_link_id_enum link_id)
{
	struct gps_each_link *p = gps_dl_link_get(link_id);

	if (p)
		return p->sub_states.is_active;
	else
		return false;
}

void gps_each_link_inc_session_id(enum gps_dl_link_id_enum link_id)
{
	struct gps_each_link *p = gps_dl_link_get(link_id);
	int sid;

	/* TODO: lock */
	if (p->session_id >= GPS_EACH_LINK_SID_MAX)
		p->session_id = 1;
	else
		p->session_id++;
	sid = p->session_id;
	/* TODO: unlock */

	GDL_LOGXD(link_id, "new sid = %d", sid);
}

int gps_each_link_get_session_id(enum gps_dl_link_id_enum link_id)
{
	struct gps_each_link *p = gps_dl_link_get(link_id);
	int sid;

	/* TODO: lock */
	sid = p->session_id;
	/* TODO: unlock */

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
	gps_each_link_set_bool_flag(link_id, LINK_OPEN_RESULT_OKAY, true);
	gps_dl_link_wake_up(&p->waitables[GPS_DL_WAIT_OPEN_CLOSE]);

	gps_each_link_take_big_lock(link_id, GDL_LOCK_FOR_OPEN_DONE);
	if (gps_each_link_get_bool_flag(link_id, LINK_USER_OPEN)) {
		GDL_LOGXW(link_id, "user still online, change to opened");
		gps_each_link_set_state(link_id, LINK_OPENED);

		/* TODO: ack on DSP reset done */
#if 0
		/* if has pending data, can send it now */
		gdl_ret = gdl_dma_buf_get_data_entry(&p->tx_dma_buf, &dma_buf_entry);
		if (gdl_ret == GDL_OKAY)
			gps_dl_hal_a2d_tx_dma_start(link_id, &dma_buf_entry);
#endif
	} else {
		GDL_LOGXW(link_id, "user already offline, change to closing");
		gps_each_link_set_state(link_id, LINK_CLOSING);
		gps_each_link_set_bool_flag(link_id, LINK_TO_BE_CLOSED, true);
		gps_dl_link_event_send(GPS_DL_EVT_LINK_CLOSE, link_id);
	}
	gps_each_link_give_big_lock(link_id);
}

void gps_each_link_init(enum gps_dl_link_id_enum link_id)
{
	struct gps_each_link *p = gps_dl_link_get(link_id);

	p->session_id = 0;
	gps_each_link_mutexes_init(p);
	gps_each_link_set_active(link_id, false);
	gps_dl_link_set_ready_to_write(link_id, false);
	gps_each_link_context_reset(link_id);
	gps_each_link_set_state(link_id, LINK_CLOSED);
}

void gps_each_link_deinit(enum gps_dl_link_id_enum link_id)
{
	struct gps_each_link *p = gps_dl_link_get(link_id);

	gps_each_link_mutexes_deinit(p);
}

void gps_each_link_context_reset(enum gps_dl_link_id_enum link_id)
{
	enum gps_each_link_waitable_type j;
	struct gps_each_link *p_link = gps_dl_link_get(link_id);

	for (j = 0; j < GPS_DL_WAIT_NUM; j++)
		gps_dl_link_waitable_reset(&p_link->waitables[j], j);
}

int gps_each_link_open(enum gps_dl_link_id_enum link_id)
{
	enum gps_each_link_state_enum state;
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
		retval = -EAGAIN;
		break;

	case LINK_RESET_DONE:
		/* RESET_DONE stands for user space not close me */
		retval = -EBUSY; /* twice open not allowed */
		break;

	case LINK_OPENED:
		retval = -EBUSY;; /* twice open not allowed */
		break;

	case LINK_CLOSED:
		gps_each_link_take_big_lock(link_id, GDL_LOCK_FOR_OPEN);
		gps_each_link_set_state(link_id, LINK_OPENING);

		/* TODO: simplify the flags */
		gps_each_link_set_bool_flag(link_id, LINK_TO_BE_CLOSED, false);
		gps_each_link_set_bool_flag(link_id, LINK_USER_OPEN, true);

		gps_dl_link_event_send(GPS_DL_EVT_LINK_OPEN, link_id);
		gps_dl_link_open_wait(link_id, &sigval);

		if (sigval != 0) {
			gps_each_link_set_bool_flag(link_id, LINK_USER_OPEN, false);
			gps_each_link_give_big_lock(link_id);
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
		GDL_LOGXD(link_id, "prev_state = %d, retval = %d", state, retval);
	else
		GDL_LOGXW(link_id, "prev_state = %d, retval = %d", state, retval);

	return retval;
}

int gps_each_link_reset(enum gps_dl_link_id_enum link_id)
{
	/*
	 * - set each link resetting flag
	 */
	enum gps_each_link_state_enum state, state2;
	int retval;

	state = gps_each_link_get_state(link_id);

	switch (state) {
	case LINK_CLOSING:
	case LINK_CLOSED:
		retval = -EBUSY;
		break;

	case LINK_RESETTING:
	case LINK_RESET_DONE:
		retval = 0;
		break;

	case LINK_OPENED:
		gps_each_link_take_big_lock(link_id, GDL_LOCK_FOR_RESET);

		state2 = gps_each_link_get_state(link_id);
		if (state2 != LINK_OPENED) {
			/* Already reset or close, not trigger reseeting again */
			gps_each_link_give_big_lock(link_id);
			GDL_LOGXW(link_id, "state changed: %d -> %d", state, state2);
			if (state2 == LINK_RESETTING || state2 == LINK_RESET_DONE)
				retval = 0;
			else
				retval = -EBUSY;
			break;
		}

		gps_each_link_set_bool_flag(link_id, LINK_IS_RESETTING, true);
		gps_dl_link_event_send(GPS_DL_EVT_LINK_RESET_DSP, link_id);
		gps_each_link_give_big_lock(link_id);
		retval = 0;
		break;

	default:
		retval = -EINVAL;
		break;
	}

	/* wait until cttld thread ack the status */
	if (retval == 0)
		GDL_LOGXD(link_id, "prev_state = %d, retval = %d", state, retval);
	else
		GDL_LOGXW(link_id, "prev_state = %d, retval = %d", state, retval);

	return retval;
}

int gps_dl_trigger_connsys_reset(void)
{
	/* - set gps global reset status
	 */
	return 0;
}

void gps_dl_ctrld_set_resest_status(void)
{
	gps_each_link_set_active(GPS_DATA_LINK_ID0, false);
	gps_each_link_set_active(GPS_DATA_LINK_ID1, false);
}

bool gps_connsys_reset_sent;
int gps_dl_link_connsys_reset_handler(void)
{
	if (gps_connsys_reset_sent) {
		gps_dl_ctrld_set_resest_status();
		gps_dl_link_event_send(GPS_DL_EVT_LINK_RESET_CONNSYS, GPS_DATA_LINK_ID0);
		gps_dl_link_event_send(GPS_DL_EVT_LINK_RESET_CONNSYS, GPS_DATA_LINK_ID1);
		gps_connsys_reset_sent = true;
	}

	return 0;
}

int gps_dl_connsys_reset_prepare(void)
{
	/*
	 * - set gps global reset status
	 * - wait ctrld wait it
	 * - set to wait reset done
	 * - return, then conninfra can goto do reset
	 */
	gps_dl_link_connsys_reset_handler();

	return 0;
}

int gps_dl_connsys_reset_done_handler(void)
{
	/*
	 * - change to idle status
	 */

	return 0;
}

void gps_dl_link_reset_ack(enum gps_dl_link_id_enum link_id)
{
	GDL_LOGXD(link_id, "");
	gps_each_link_take_big_lock(link_id, GDL_LOCK_FOR_RESET_DONE);
	gps_each_link_set_state(link_id, LINK_RESET_DONE);
	gps_each_link_give_big_lock(link_id);
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
	gps_each_link_set_state(link_id, LINK_CLOSED);
	gps_each_link_give_big_lock(link_id);
}

int gps_each_link_close(enum gps_dl_link_id_enum link_id)
{
	enum gps_each_link_state_enum state;
	long sigval = 0;
	int retval;

	state = gps_each_link_get_state(link_id);

	switch (state) {
	case LINK_CLOSING:
	case LINK_CLOSED:
		/* twice close */
		retval = -EINVAL;
		break;

	case LINK_RESETTING:
		/* TODO */
		retval = -EAGAIN;
		break;

	case LINK_RESET_DONE:
		gps_each_link_set_bool_flag(link_id, LINK_USER_OPEN, false);
		gps_each_link_set_state(link_id, LINK_CLOSED);
		retval = 0;
		break;

	case LINK_OPENED:
		gps_each_link_take_big_lock(link_id, GDL_LOCK_FOR_CLOSE);
		gps_each_link_set_state(link_id, LINK_CLOSING);

		/* set this status, hal proc will by pass the message from the link
		 * it can make LINK_CLOSE be processed faster
		 */
		gps_each_link_set_bool_flag(link_id, LINK_TO_BE_CLOSED, true);

		gps_dl_link_event_send(GPS_DL_EVT_LINK_CLOSE, link_id);
		gps_dl_link_close_wait(link_id, &sigval);

		if (sigval) {
			gps_each_link_set_bool_flag(link_id, LINK_USER_OPEN, false);
			gps_each_link_give_big_lock(link_id);
			retval = -EINVAL;
			break;
		}

		retval = 0;
		gps_each_link_set_bool_flag(link_id, LINK_USER_OPEN, false);
		gps_each_link_give_big_lock(link_id);
		break;
	default:
		retval = -EINVAL;
		break;
	}

	if (retval == 0)
		GDL_LOGXD(link_id, "prev_state = %d, retval = %d", state, retval);
	else
		GDL_LOGXW(link_id, "prev_state = %d, retval = %d", state, retval);

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

	GDL_LOGXW(link_id, "prev_state = %d, retval = %d", state, retval);

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

void gps_dl_link_waitable_reset(struct gps_each_link_waitable *p,
	enum gps_each_link_waitable_type type)
{
	p->fired = false;
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

	/* TODO: check race conditions? */
	GDL_TEST_TRUE_AND_SET_FALSE(p->fired, is_fired);
	if (is_fired) {
		GDL_LOGD("waitable = %s, no wait return", gps_dl_waitable_type_name(p->type));
		return GDL_OKAY;
	}

	GDL_LOGD("waitable = %s, wait start", gps_dl_waitable_type_name(p->type));
	val = wait_event_interruptible(p->wq, p->fired);
	if (val) {
		GDL_LOGI("signaled by %ld", val);
		if (p_sigval)
			*p_sigval = val;
		return GDL_FAIL_SIGNALED;
	}

	p->fired = false;
	GDL_LOGD("waitable = %s, wait done", gps_dl_waitable_type_name(p->type));
	return GDL_OKAY;
#else
	return GDL_FAIL_NOT_SUPPORT;
#endif
}

void gps_dl_link_wake_up(struct gps_each_link_waitable *p)
{
	bool is_fired;

	ASSERT_NOT_NULL(p, GDL_VOIDF());

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

			if (p->rx_dma_buf.has_pending_rx) {
				GDL_LOGD("has pending rx, trigger again");
				p->rx_dma_buf.has_pending_rx = false;
				gps_dl_hal_event_send(GPS_DL_HAL_EVT_D2A_RX_HAS_DATA, link_id);
			}

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

	GDL_LOGXD(link_id, "evt = %s", gps_dl_link_event_name(evt));

	switch (evt) {
	case GPS_DL_EVT_LINK_OPEN:
		gps_each_dsp_reg_gourp_read_init(link_id);
		gps_each_link_inc_session_id(link_id);
		gps_each_link_set_active(link_id, true);
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
		if (!gps_each_link_change_state(link_id, LINK_OPENED, LINK_RESETTING)) {
			/* no handle it again */
			break;
		}
		/* go and do close */
	case GPS_DL_EVT_LINK_CLOSE:
	case GPS_DL_EVT_LINK_RESET_DSP:
		/* TODO: avoid twice enter */

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
		gps_dl_link_set_ready_to_write(link_id, false);
		gps_dl_irq_each_link_mask(link_id, GPS_DL_IRQ_TYPE_MCUB, GPS_DL_IRQ_CTRL_FROM_THREAD);

		if (p_link->rx_dma_buf.has_pending_rx) {
			/* It means this irq has already masked, */
			/* DON'T mask again, otherwise twice unmask might be needed */
			GDL_LOGXD(link_id, "has pending rx, by pass mask the this IRQ");
			p_link->rx_dma_buf.has_pending_rx = false;
		} else
			gps_dl_irq_each_link_mask(link_id, GPS_DL_IRQ_TYPE_HAS_DATA, GPS_DL_IRQ_CTRL_FROM_THREAD);

		/* TODO: avoid twice mask need to be handled if HAS_CTRLD */
		gps_dl_irq_each_link_mask(link_id, GPS_DL_IRQ_TYPE_HAS_NODATA, GPS_DL_IRQ_CTRL_FROM_THREAD);

		gps_dl_hal_link_power_ctrl(link_id, 0);
		gps_dl_hal_conn_power_ctrl(link_id, 0);

		gps_dsp_fsm(GPS_DSP_EVT_FUNC_OFF, link_id);

		if (GPS_DL_EVT_LINK_RESET_DSP == evt)
			gps_dl_link_reset_ack(link_id);
		else
			gps_dl_link_close_ack(link_id); /* TODO: check fired race */

#if GPS_DL_ON_LINUX
		gps_dma_buf_reset(&p_link->tx_dma_buf);
		gps_dma_buf_reset(&p_link->rx_dma_buf);
#endif
		gps_each_link_context_reset(link_id);
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

	case GPS_DL_EVT_LINK_RESET_GPS:
		/* turn off GPS power directly */
		break;

	case GPS_DL_EVT_LINK_RESET_CONNSYS:
		/* turn off Connsys power directly
		 * 1. no need to do anything, just make sure the message queue is empty
		 * 2. how to handle ctrld block issue
		 */
		/* gps_dl_link_open_ack(link_id); */
		break;
	default:
		break;
	}
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

	return p->state_for_user;
}

void gps_each_link_set_state(enum gps_dl_link_id_enum link_id, enum gps_each_link_state_enum state)
{
	struct gps_each_link *p = gps_dl_link_get(link_id);
	enum gps_each_link_state_enum pre_state = p->state_for_user;

	GDL_LOGXD(link_id, "%d -> %d", pre_state, state);
	p->state_for_user = state;
}

bool gps_each_link_change_state(enum gps_dl_link_id_enum link_id, enum gps_each_link_state_enum from,
	enum gps_each_link_state_enum to)
{
	/* TODO */

	return false;
}

