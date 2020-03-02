#include "gps_dl_config.h"

#include "gps_dl_subsys_reset.h"
#include "gps_each_link.h"
#include "gps_dl_name_list.h"

#if GPS_DL_HAS_CONNINFRA_DRV
#include "conninfra.h"
#endif

enum GDL_RET_STATUS gps_dl_reset_level_set_and_trigger(
	enum gps_each_link_reset_level level, bool wait_reset_done)
{
	enum gps_dl_link_id_enum link_id;
	struct gps_each_link *p;
	enum gps_each_link_state_enum old_state, new_state;
	enum gps_each_link_reset_level old_level, new_level;
	bool need_wait[GPS_DATA_LINK_NUM] = {false};
	bool to_send_reset_event;
	long sigval;
	enum GDL_RET_STATUS wait_status;

	if (level != GPS_DL_RESET_LEVEL_GPS_SUBSYS && level !=  GPS_DL_RESET_LEVEL_CONNSYS) {
		GDL_LOGW("level = %d, do nothing and return");
		return GDL_FAIL_INVAL;
	}

	if (wait_reset_done)
		; /* TODO: take mutex to allow pending more waiter */

	for (link_id = 0; link_id < GPS_DATA_LINK_NUM; link_id++) {
		p = gps_dl_link_get(link_id);
		to_send_reset_event = false;

		gps_each_link_spin_lock_take(link_id, GPS_DL_SPINLOCK_FOR_LINK_STATE);
		old_state = p->state_for_user;
		old_level = p->reset_level;

		switch (old_state) {
		case LINK_RESET_DONE:
		case LINK_CLOSED:
			need_wait[link_id] = false;
			if (level == GPS_DL_RESET_LEVEL_CONNSYS) {
				p->state_for_user = LINK_DISABLED;
				p->reset_level = level;
			}
			break;

		case LINK_OPENING:
		case LINK_OPENED:
		case LINK_CLOSING:
			need_wait[link_id] = true;
			p->state_for_user = LINK_RESETTING;
			p->reset_level = level;
			to_send_reset_event = true;
			break;

		case LINK_RESETTING:
			need_wait[link_id] = true;
			if (old_level < level)
				p->reset_level = level;
			break;

		case LINK_DISABLED:
		case LINK_UNINIT:
			need_wait[link_id] = false;
			break;

		default:
			need_wait[link_id] = false;
			break;
		}

		new_state = p->state_for_user;
		new_level = p->reset_level;
		gps_each_link_spin_lock_give(link_id, GPS_DL_SPINLOCK_FOR_LINK_STATE);

		if (to_send_reset_event) {
			gps_dl_link_waitable_reset(link_id, GPS_DL_WAIT_RESET);
			if (level == GPS_DL_RESET_LEVEL_CONNSYS)
				gps_dl_link_event_send(GPS_DL_EVT_LINK_PRE_CONN_RESET, link_id);
			else
				gps_dl_link_event_send(GPS_DL_EVT_LINK_RESET_GPS, link_id);
		}

		GDL_LOGXE(link_id, "level = %d (%d -> %d), state: %s -> %s, is_sent = %d, to_wait = %d",
			level, old_level, new_level,
			gps_dl_link_state_name(old_state), gps_dl_link_state_name(new_state),
			to_send_reset_event, need_wait[link_id]);
	}

	if (!wait_reset_done) {
		GDL_LOGE("force no wait");
		return GDL_OKAY;
	}

	for (link_id = 0; link_id < GPS_DATA_LINK_NUM; link_id++) {
		if (!need_wait[link_id])
			continue;

		sigval = 0;
		p = gps_dl_link_get(link_id);
		wait_status = gps_dl_link_wait_on(&p->waitables[GPS_DL_WAIT_RESET], &sigval);
		if (wait_status == GDL_FAIL_SIGNALED) {
			GDL_LOGXE(link_id, "sigval = %d", sigval);
			return GDL_FAIL_SIGNALED;
		}

		GDL_LOGXE(link_id, "wait ret = %s", gdl_ret_to_name(wait_status));
	}

	if (wait_reset_done)
		; /* TODO: take mutex to allow pending more waiter */

	return GDL_OKAY;
}

int gps_dl_trigger_gps_subsys_reset(bool wait_reset_done)
{
	enum GDL_RET_STATUS ret_status;

	ret_status = gps_dl_reset_level_set_and_trigger(GPS_DL_RESET_LEVEL_GPS_SUBSYS, wait_reset_done);
	if (ret_status != GDL_OKAY) {
		GDL_LOGE("status %s is not okay, return -1", gdl_ret_to_name(ret_status));
		return -1;
	}
	return 0;
}

void gps_dl_handle_connsys_reset_done(void)
{
	enum gps_dl_link_id_enum link_id;
	struct gps_each_link *p;
	enum gps_each_link_state_enum state;
	bool to_send_reset_event;

	for (link_id = 0; link_id < GPS_DATA_LINK_NUM; link_id++) {
		p = gps_dl_link_get(link_id);
		to_send_reset_event = false;

		gps_each_link_spin_lock_take(link_id, GPS_DL_SPINLOCK_FOR_LINK_STATE);
		state = p->state_for_user;
		if (state == LINK_RESETTING || state == LINK_DISABLED)
			to_send_reset_event = true;
		gps_each_link_spin_lock_give(link_id, GPS_DL_SPINLOCK_FOR_LINK_STATE);

		if (to_send_reset_event)
			gps_dl_link_event_send(GPS_DL_EVT_LINK_POST_CONN_RESET, link_id);

		GDL_LOGXE(link_id, "state = %s, is_sent = %d",
			gps_dl_link_state_name(state), to_send_reset_event);
	}
}

int gps_dl_trigger_connsys_reset(void)
{
#if GPS_DL_HAS_CONNINFRA_DRV
	int ret;

	GDL_LOGE("");
	ret = conninfra_trigger_whole_chip_rst(CONNDRV_TYPE_GPS, "GPS test");
	GDL_LOGE("conninfra_trigger_whole_chip_rst return = %d", ret);
#else
	GDL_LOGE("has no conninfra_drv");
#endif
	return 0;
}

#if GPS_DL_HAS_CONNINFRA_DRV
int gps_dl_on_pre_connsys_reset(void)
{
	enum GDL_RET_STATUS ret_status;

	GDL_LOGE("");
	ret_status = gps_dl_reset_level_set_and_trigger(GPS_DL_RESET_LEVEL_CONNSYS, true);

	if (ret_status != GDL_OKAY) {
		GDL_LOGE("status %s is not okay, return -1", gdl_ret_to_name(ret_status));
		return -1;
	}

	return 0;
}

int gps_dl_on_post_connsys_reset(void)
{
	GDL_LOGE("");
	gps_dl_handle_connsys_reset_done();
	return 0;
}

struct sub_drv_ops_cb gps_dl_conninfra_ops_cb;
#endif

void gps_dl_register_conninfra_reset_cb(void)
{
#if GPS_DL_HAS_CONNINFRA_DRV
	memset(&gps_dl_conninfra_ops_cb, 0, sizeof(gps_dl_conninfra_ops_cb));
	gps_dl_conninfra_ops_cb.rst_cb.pre_whole_chip_rst = gps_dl_on_pre_connsys_reset;
	gps_dl_conninfra_ops_cb.rst_cb.post_whole_chip_rst = gps_dl_on_post_connsys_reset;

	conninfra_sub_drv_ops_register(CONNDRV_TYPE_GPS, &gps_dl_conninfra_ops_cb);
#endif
}

void gps_dl_unregister_conninfra_reset_cb(void)
{
#if GPS_DL_HAS_CONNINFRA_DRV
	conninfra_sub_drv_ops_unregister(CONNDRV_TYPE_GPS);
#endif
}

