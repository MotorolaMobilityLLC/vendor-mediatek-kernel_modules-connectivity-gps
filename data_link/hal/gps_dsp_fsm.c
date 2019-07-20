/* #include "autoconf.h" */

#if 1

/* #ifdef CONFIG_GPS_SUPPORT */
#if 0
#include "gps_reroute.h"
#include "gps_reroute_emi.h"
#include "emi_symbol_hook.h"
#include "gps_util.h"
#include "wmt_task.h"
#include "cos_api.h"
#endif

#include "gps_dsp_fsm.h"
#include "gps_dl_log.h"
#include "gps_each_link.h"
#include "gps_dl_name_list.h"

/* extern kal_uint32 g_mcu_real_clock_rate; */
/* extern kal_uint32 g_max_mcu_clock_rate; */
/* #define GPS_REQ_CLOCK_FREQ_MHZ_MVCD     (g_max_mcu_clock_rate/ 1000000UL) */
/* #define GPS_REQ_CLOCK_FREQ_MHZ_NORMAL   (1) 1MHz */

enum gps_dsp_state_t g_gps_dsp_state[GPS_DATA_LINK_NUM];


enum gps_dsp_state_t gps_dsp_state_get(enum gps_dl_link_id_enum link_id)
{
	return g_gps_dsp_state[link_id];
}

bool gps_dsp_state_is(enum gps_dsp_state_t state, enum gps_dl_link_id_enum link_id)
{
	return !!(g_gps_dsp_state[link_id] == state);
}

void gps_dsp_state_change_to(enum gps_dsp_state_t next_state, enum gps_dl_link_id_enum link_id)
{
	if (next_state == GPS_DSP_ST_TURNED_ON) {
		/* gps_clock_switch (GPS_REQ_CLOCK_FREQ_MHZ_MVCD); */
		/* gps_ctrl_timer_start(GPS_DSP_RESET_TIMEOUT_MS); */
	}

	if (next_state == GPS_DSP_ST_RESET_DONE) {
		/* Note: here is just a candidate caller, if no problem, we call it */
		/* in GPS_Reroute_Ext_Power_Ctrl_Inner */
		/* GPS_Reroute_Set_DSP_Config_By_MCUB(); */

		/* gps_clock_switch (GPS_REQ_CLOCK_FREQ_MHZ_MVCD); */
		/* gps_ctrl_timer_start(GPS_DSP_MVCD_TIMEOUT_MS); */
#if 0
		if (!g_gps_bypass_dsp_turned_on_state) {
			/* ready to do reroute */
			gps_data_reroute_set_pending(false);
			GPS_Reroute_Buffer_to_GPS_port_real();
		}
#endif
	}

	if (next_state == GPS_DSP_ST_WAKEN_UP)
		; /* gps_ctrl_timer_start(GPS_DSP_WAKEUP_TIMEOUT_MS); */

	if (next_state == GPS_DSP_ST_WORKING)
		; /* gps_clock_switch (GPS_REQ_CLOCK_FREQ_MHZ_NORMAL); */

	if (next_state == GPS_DSP_ST_OFF) {
		/* gps_clock_switch (0); - already done before gps_dsp_fsm */
		/* gps_ctrl_timer_stop(); */
	}

	g_gps_dsp_state[link_id] = next_state;
}

void gps_dsp_fsm(enum gps_dsp_event_t evt, enum gps_dl_link_id_enum link_id)
{
#if 1
	bool abnormal_flag = true;
	enum gps_dsp_state_t last_state = gps_dsp_state_get(link_id);

	if (GPS_DSP_EVT_FUNC_ON != evt && GPS_DSP_ST_OFF == last_state) {
		/* bypass all event except gps func on when gps is off */
		abnormal_flag = true;
		goto _last_check;
	}

	if (GPS_DSP_EVT_FUNC_OFF == evt) {
#if 0
		if (GPS_DSP_ST_HW_SLEEP_MODE == last_state ||
			GPS_DSP_ST_HW_STOP_MODE == last_state) {
			/* Special handing rather than GPS_Reroute_Ext_Power_Ctrl_Inner */
			if (GPS_DSP_ST_HW_SLEEP_MODE == last_state)
				; /* wmt_task_subsystem_power_ctrl_handler(COS_GPS_CTRL, 3); */
			else if (GPS_DSP_ST_HW_STOP_MODE == last_state)
				; /* wmt_task_subsystem_power_ctrl_handler(COS_GPS_CTRL, 5); */

			/* wmt_task_subsystem_power_ctrl_handler(COS_GPS_CTRL, 0); */
			/* GPS_Reroute_Buffer_to_GPS_port_drop(); */
			/* GPS_Reroute_Cos_Sleep_Enable(); */
		}
#endif
		gps_dsp_state_change_to(GPS_DSP_ST_OFF, link_id);
		goto _last_check;
	}

	if (GPS_DSP_EVT_CTRL_TIMER_EXPIRE == evt) {
		/* if (!gps_ctrl_timer_check_valid()) */
		{
			abnormal_flag = true;
			goto _last_check;
		}

		switch (last_state) {
		case GPS_DSP_ST_TURNED_ON:
			/* GPS_DSP_EVT_RESET_DONE timeout (180ms) */
			/* GPS_Reroute_Buffer_to_GPS_port_drop(); */
			/* gps_dsp_state_change_to(GPS_DSP_ST_RESET_DONE, link_id); */
			/* ERROR timeout - each DSP restart */

			/* TODO: gps_each_link_reset(link_id); */
			break;
		case GPS_DSP_ST_RESET_DONE:
			/* GPS_DSP_EVT_RAM_CODE_READY timeout (1s) */
			gps_dsp_state_change_to(GPS_DSP_ST_WORKING, link_id);
			break;
#if 0
		case GPS_DSP_ST_HW_SLEEP_MODE:
			GPS_Reroute_Ext_Power_Ctrl_Inner(3);
			/* cos_resource_enable(COS_SLEEP_MODE, g_gps_sleep_handle); */
			gps_dsp_state_change_to(GPS_DSP_ST_WAKEN_UP, link_id);
			break;
		case GPS_DSP_ST_WAKEN_UP:
			gps_dsp_state_change_to(GPS_DSP_ST_WORKING, link_id);
			break;
#endif
		default:
			abnormal_flag = true;
		}
		goto _last_check;
	}

	switch (last_state) {
	case GPS_DSP_ST_OFF:
		if (GPS_DSP_EVT_FUNC_ON == evt) {
#if 0
			if (g_gps_bypass_dsp_turned_on_state)
				gps_dsp_state_change_to(GPS_DSP_ST_RESET_DONE, link_id);
			else
				gps_dsp_state_change_to(GPS_DSP_ST_TURNED_ON, link_id);
#endif
			gps_dsp_state_change_to(GPS_DSP_ST_TURNED_ON, link_id);
			abnormal_flag = false;
		} else
			abnormal_flag = true;
		break;

	case GPS_DSP_ST_TURNED_ON:
		if (GPS_DSP_EVT_RESET_DONE == evt) {
			gps_dsp_state_change_to(GPS_DSP_ST_RESET_DONE, link_id);
			abnormal_flag = false;
		} else
			abnormal_flag = true;
		break;

	case GPS_DSP_ST_RESET_DONE:
		if (GPS_DSP_EVT_RAM_CODE_READY == evt) {
			/* TODO */
			/* gps_ctrl_timer_stop(); */
			gps_dsp_state_change_to(GPS_DSP_ST_WORKING, link_id);
			abnormal_flag = false;
		} /* add GPS_DSP_EVT_REQUEST_HW_SLEEP_MODE == evt */
		else
			abnormal_flag = true;
		break;

	case GPS_DSP_ST_WORKING:
		if (GPS_DSP_EVT_RESET_DONE == evt) {
			/* PMTK101 like restart or to be powered off */
			gps_dsp_state_change_to(GPS_DSP_ST_RESET_DONE, link_id);
			abnormal_flag = false;
		}
#if 0
		else if (GPS_DSP_EVT_HW_SLEEP_REQ == evt) {
			GPS_Reroute_Ext_Power_Ctrl_Inner(2);
			/* to_do_timer check!!! - dynamic change timer */
			/* gps_ctrl_timer_start(800); */
			gps_dsp_state_change_to(GPS_DSP_ST_HW_SLEEP_MODE, link_id);
		} else if (GPS_DSP_EVT_HW_STOP_REQ == evt) {
			GPS_Reroute_Ext_Power_Ctrl_Inner(4);
			gps_dsp_state_change_to(GPS_DSP_ST_HW_STOP_MODE, link_id);
		} else
			abnormal_flag = true;
#endif
		abnormal_flag = true;
		break;

	case GPS_DSP_ST_HW_SLEEP_MODE:
#if 0
		if (GPS_DSP_EVT_HW_SLEEP_EXIT == evt) {
			/* from host - only for test */
			GPS_Reroute_Ext_Power_Ctrl_Inner(3);
			gps_dsp_state_change_to(GPS_DSP_ST_WAKEN_UP, link_id);
		} else
			abnormal_flag = true;
#endif
		abnormal_flag = true;
		break;

	case GPS_DSP_ST_HW_STOP_MODE:
#if 0
		if (GPS_DSP_EVT_HW_STOP_EXIT == evt) {
			/* from host */
			GPS_Reroute_Ext_Power_Ctrl_Inner(5);
			gps_dsp_state_change_to(GPS_DSP_ST_WAKEN_UP, link_id);
		} else
			abnormal_flag = true;
#endif
		abnormal_flag = true;
		break;

	case GPS_DSP_ST_WAKEN_UP:
#if 0
		if (GPS_DSP_EVT_RESET_DONE == evt) {
			/* May need to set 6631/35 flag to DSP or change to RESET_DONE */
			/* Note: here is just a candidate caller, if no problem, we call it */
			/* in GPS_Reroute_Ext_Power_Ctrl_Inner */
			/* GPS_Reroute_Set_DSP_Config_By_MCUB(); */
		} else if (GPS_DSP_EVT_RAM_CODE_READY == evt) {
			gps_ctrl_timer_stop();
			gps_dsp_state_change_to(GPS_DSP_ST_WORKING, link_id);
		} else
			abnormal_flag = true;
#endif
		abnormal_flag = true;
		break;

	default:
		abnormal_flag = true;
	}

_last_check:
	/* Note: due to in WMT task, g_mcu_real_clock_rate not update here */
	/* It's updated when next WMT task message is processed */
	/* GPS_TRC("gps_dsp_fsm: old_st=%d, evt=%d, new_st=%d, ab=%d, clk=%d, screen=%d", */
	/* last_state, evt, gps_dsp_state_get(), abnormal_flag, */
	/* g_mcu_real_clock_rate, cos_get_host_screen_on_state()); */
	GDL_LOGXD(link_id, "gps_dsp_fsm: old_st=%s, evt=%s, new_st=%s, is_err=%d",
		gps_dl_dsp_state_name(last_state), gps_dl_dsp_event_name(evt),
		gps_dl_dsp_state_name(gps_dsp_state_get(link_id)), abnormal_flag);
	return;
#endif
}

#endif /* CONFIG_GPS_SUPPORT */

