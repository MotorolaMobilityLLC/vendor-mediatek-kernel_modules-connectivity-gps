#include "gps_dl_config.h"

#include "gps_dl_name_list.h"


const char *const gps_dl_dsp_state_name_list[GPS_DSP_ST_MAX + 1] = {
	[GPS_DSP_ST_OFF]            = "OFF ",
	[GPS_DSP_ST_TURNED_ON]      = "ON  ",
	[GPS_DSP_ST_RESET_DONE]     = "RST ",
	[GPS_DSP_ST_WORKING]        = "WORK",
	[GPS_DSP_ST_HW_SLEEP_MODE]  = "SLP ",
	[GPS_DSP_ST_HW_STOP_MODE]   = "STOP",
	[GPS_DSP_ST_WAKEN_UP]       = "WAKE",
	[GPS_DSP_ST_MAX]            = "UNKN"
};

const char *gps_dl_dsp_state_name(enum gps_dsp_state_t state)
{
	if (state >= 0 && state < GPS_DSP_ST_MAX)
		return gps_dl_dsp_state_name_list[state];
	else
		return gps_dl_dsp_state_name_list[GPS_DSP_ST_MAX];
}


const char *const gps_dl_dsp_event_name_list[GPS_DSP_EVT_MAX + 1] = {
	[GPS_DSP_EVT_FUNC_OFF]          = "FUNC_OFF",
	[GPS_DSP_EVT_FUNC_ON]           = "FUNC_ON ",
	[GPS_DSP_EVT_RESET_DONE]        = "RST_DONE",
	[GPS_DSP_EVT_RAM_CODE_READY]    = "RAM_OKAY",
	[GPS_DSP_EVT_CTRL_TIMER_EXPIRE] = "TIMEOUT ",
	[GPS_DSP_EVT_HW_SLEEP_REQ]      = "SLP_REQ ",
	[GPS_DSP_EVT_HW_SLEEP_EXIT]     = "SLP_WAK ",
	[GPS_DSP_EVT_HW_STOP_REQ]       = "STOP_REQ",
	[GPS_DSP_EVT_HW_STOP_EXIT]      = "STOP_WAK",
	[GPS_DSP_EVT_MAX]               = "UNKNOWN "
};

const char *gps_dl_dsp_event_name(enum gps_dsp_event_t event)
{
	if (event >= 0 && event < GPS_DSP_EVT_MAX)
		return gps_dl_dsp_event_name_list[event];
	else
		return gps_dl_dsp_event_name_list[GPS_DSP_EVT_MAX];
}


