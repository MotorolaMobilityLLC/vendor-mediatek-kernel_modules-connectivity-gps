/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#include "gps_mcudl_log.h"
#include "gps_mcudl_xlink.h"
#include "gps_mcudl_hal_mcu.h"
#include "gps_mcudl_hal_ccif.h"
#include "gps_mcu_hif_api.h"


bool gps_mcudl_xlink_is_connected_to_mcu_lifecycle(enum gps_mcudl_xid x_id)
{
	bool not_connected_to;

	not_connected_to = (
		x_id == GPS_MDLX_GDLOG2 ||
		x_id == GPS_MDLX_MPELOG2 ||
		false
	);

	return !not_connected_to;
}

bool gps_mcudl_xlink_is_connected_mnlbin(enum gps_mcudl_xid x_id)
{
	bool connected_to;

	connected_to = (
		x_id == GPS_MDLX_MNL ||
		x_id == GPS_MDLX_AGENT ||
		x_id == GPS_MDLX_NMEA ||
		x_id == GPS_MDLX_PMTK ||
		x_id == GPS_MDLX_MEAS ||
		x_id == GPS_MDLX_CORR ||
		false
	);

	return !!connected_to;
}

bool gps_mcudl_xlink_is_requiring_wake_lock(enum gps_mcudl_xid x_id)
{
	return !!(1UL << x_id &
		gps_mcudl_xlink_get_bitmask_requiring_wake_lock());
}

unsigned int gps_mcudl_xlink_get_bitmask_requiring_wake_lock(void)
{
	unsigned int bitmask;

	bitmask = (
		(1UL << GPS_MDLX_GDLOG) &
		(1UL << GPS_MDLX_GDLOG2) &
		(1UL << GPS_MDLX_MPELOG) &
		(1UL << GPS_MDLX_MPELOG2) &
		(1UL << GPS_MDLX_LPPM) &
		0xFFFFFFFF
	);

	return bitmask;
}

void gps_mcudl_xlink_trigger_print_hw_status(void)
{
	gps_mcudl_xlink_event_send(GPS_MDLX_MCUSYS, GPS_MCUDL_EVT_LINK_PRINT_HW_STATUS);
}

void gps_mcudl_xlink_test_fw_own_ctrl(bool to_set)
{
	bool is_okay;

	if (to_set)
		is_okay = gps_mcudl_hal_mcu_set_fw_own();
	else
		is_okay = gps_mcudl_hal_mcu_clr_fw_own();
	MDL_LOGW("to_set=%d, ok=%d", to_set, is_okay);
}

void gps_mcudl_xlink_test_toggle_ccif(unsigned int ch)
{
	if (ch >= 8)
		return;
	gps_mcudl_hal_ccif_tx_prepare(ch);
	gps_mcudl_hal_ccif_tx_trigger(ch);
}

void gps_mcudl_xlink_test_toggle_reset_by_gps_hif(void)
{
	bool is_okay;

	is_okay = gps_mcu_hif_send(GPS_MCU_HIF_CH_DMALESS_MGMT, "\x03", 1);
	MDL_LOGW("write cmd3, is_ok=%d", is_okay);
}

