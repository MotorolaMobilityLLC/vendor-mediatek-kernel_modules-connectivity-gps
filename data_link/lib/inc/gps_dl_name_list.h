#ifndef _GPS_DL_NAME_LIST_H
#define _GPS_DL_NAME_LIST_H

#include "gps_dl_config.h"

#include "gps_each_link.h"
#include "gps_dsp_fsm.h"

const char *gps_dl_dsp_state_name(enum gps_dsp_state_t state);
const char *gps_dl_dsp_event_name(enum gps_dsp_event_t event);

#endif /* _GPS_DL_NAME_LIST_H */

