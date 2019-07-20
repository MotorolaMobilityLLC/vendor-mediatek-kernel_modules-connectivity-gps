#ifndef _GPS_DL_LINUX_PLAT_DRV_H
#define _GPS_DL_LINUX_PLAT_DRV_H

#include "gps_dl_config.h"

#if GPS_DL_HAS_PLAT_DRV
int gps_dl_linux_plat_drv_register(void);
int gps_dl_linux_plat_drv_unregister(void);
void __iomem *gps_dl_host_addr_to_virt(unsigned int host_addr);
void gps_dl_update_status_for_md_blanking(bool gps_is_on);
#endif

#endif /* _GPS_DL_LINUX_PLAT_DRV_H */

