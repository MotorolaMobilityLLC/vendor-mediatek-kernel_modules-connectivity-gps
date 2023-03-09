/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#include "gps_dl_config.h"
#include "gps_mcudl_hal_conn.h"
#include "gps_mcudl_ylink.h"
#include "gps_dl_hw_dep_api.h"


void gps_mcudl_hal_get_ecid_info(void)
{
	gps_dl_hw_dep_gps_control_adie_on();
	gps_dl_hw_dep_gps_get_ecid_info();
	gps_dl_hw_dep_gps_control_adie_off();
}

void gps_mcudl_hal_dump_power_state(void)
{
	gps_dl_hw_dep_gps_dump_power_state();
}

