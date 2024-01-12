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

