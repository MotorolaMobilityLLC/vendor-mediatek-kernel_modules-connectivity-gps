#ifndef _GPS_DATA_LINK_DEVICES_H
#define _GPS_DATA_LINK_DEVICES_H

#include "gps_dl_config.h"

int mtk_gps_data_link_devices_init(void);
void mtk_gps_data_link_devices_exit(void);
#if GPS_DL_CTRLD_MOCK_LINK_LAYER
void gps_dl_rx_event_cb(int data_link_num);
#endif

#endif /* _GPS_DATA_LINK_DEVICES_H */

