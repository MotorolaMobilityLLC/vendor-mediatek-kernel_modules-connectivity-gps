#ifndef _GPS_DL_CONFIG_H
#define _GPS_DL_CONFIG_H

enum gps_dl_link_id_enum {
	GPS_DATA_LINK_ID0	= 0,
	GPS_DATA_LINK_ID1	= 1,
	GPS_DATA_LINK_NUM	= 2,
};

#define LINK_ID_IS_VALID(link_id) \
	(((link_id) >= 0) & ((link_id) < GPS_DATA_LINK_NUM))

#define CHOOSE_BY_LINK_ID(link_id, val_for_id0, val_for_id1, val_for_otherwise) \
	(!LINK_ID_IS_VALID((link_id)) ? (val_for_otherwise) : ( \
		(link_id == GPS_DATA_LINK_ID0) ? val_for_id0 : val_for_id1))

#ifdef GPS_DL_HAS_MOCK
#define GPS_DL_HW_IS_MOCK     (1)
#define GPS_DL_MOCK_HAL       (1)
#else
#define GPS_DL_HW_IS_MOCK     (0)
#define GPS_DL_MOCK_HAL       (0)
#endif

#define GPS_DL_MOCK_RX_TIMEOUT (0)

#define GPS_DL_ON_LINUX       (1)
#define GPS_DL_ON_CTP         (0)

#define GPS_DL_HAS_LINK_LAYER (1)
#define GPS_DL_HAS_CTRLD      (1)
#define GPS_DL_CTRLD_MOCK_LINK_LAYER    (0)
#define GPS_DL_NO_USE_IRQ     (0)
#define GPS_DL_USE_THREADED_IRQ (1)

#ifndef GPS_DL_HAS_CONNINFRA_DRV
#define GPS_DL_HAS_CONNINFRA_DRV (0)
#endif

#define GPS_DL_HAS_PLAT_DRV   (1)

#define GPS_DL_IS_MODULE      (1)

#include "gps_dl_log.h"

#endif /* _GPS_DL_CONFIG_H */

