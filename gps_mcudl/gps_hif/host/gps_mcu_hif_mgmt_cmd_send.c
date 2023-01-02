/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#include "gps_dl_time_tick.h"
#include "gps_mcudl_log.h"
#include "gps_mcu_hif_mgmt_cmd_send.h"

bool gps_mcu_hif_mgmt_cmd_send_fw_log_ctrl(bool enable)
{
	bool is_okay = false;

	if (enable) {
		is_okay = gps_mcu_hif_send(GPS_MCU_HIF_CH_DMALESS_MGMT,
			"\x03\x01", 2);
	} else {
		is_okay = gps_mcu_hif_send(GPS_MCU_HIF_CH_DMALESS_MGMT,
			"\x03\x00", 2);
	}
	MDL_LOGW("write cmd3, is_ok=%d", is_okay);

	gps_dl_sleep_us(100*1000, 200*1000);
	MDL_LOGI("have some wait, done");
	return is_okay;
}

