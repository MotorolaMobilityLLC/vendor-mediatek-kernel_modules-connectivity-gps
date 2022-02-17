#ifndef _GPS_DL_SUBSYS_RESET_H
#define _GPS_DL_SUBSYS_RESET_H

#include "gps_dl_base.h"

enum gps_each_link_reset_level {
	GPS_DL_RESET_LEVEL_NONE,
	GPS_DL_RESET_LEVEL_GPS_SINGLE_LINK,
	GPS_DL_RESET_LEVEL_GPS_SUBSYS,
	GPS_DL_RESET_LEVEL_CONNSYS,
	GPS_DL_RESET_LEVEL_NUM
};

enum GDL_RET_STATUS gps_dl_reset_level_set_and_trigger(
	enum gps_each_link_reset_level level, bool wait_reset_done);

bool gps_dl_reset_level_is_none(void);
bool gps_dl_reset_level_is_single(void);
bool gps_dl_reset_level_gt_single(void);

int gps_dl_trigger_gps_subsys_reset(bool wait_reset_done);
int gps_dl_trigger_connsys_reset(void);
void gps_dl_handle_connsys_reset_done(void);

void gps_dl_register_conninfra_reset_cb(void);
void gps_dl_unregister_conninfra_reset_cb(void);

#endif /* _GPS_DL_SUBSYS_RESET_H */

