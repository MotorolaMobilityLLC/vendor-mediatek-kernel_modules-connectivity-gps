/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#ifndef _GPS_MCUDL_RESET_H
#define _GPS_MCUDL_RESET_H

#include "gps_dl_subsys_reset.h"

enum GDL_RET_STATUS gps_mcudl_reset_level_set_and_trigger(
	enum gps_each_link_reset_level level, bool wait_reset_done);
void gps_mcudl_handle_connsys_reset_done(void);

void gps_mcudl_connsys_coredump_init(void);
void gps_mcudl_connsys_coredump_deinit(void);
void gps_mcudl_connsys_coredump_start(void);

#endif /* _GPS_MCUDL_RESET_H */

