/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */
#include "gps_dl_config.h"

#include "gps_dl_hw_ver.h"
#include "gps_dl_hw_dep_api.h"
#include "gps_dl_hw_dep_macro.h"

#include "../gps_dl_hw_priv_util.h"

bool gps_dl_hw_dep_en_gps_func_and_poll_bgf_ack(void)
{
	bool poll_okay = false;

	/* bit24: BGFSYS_ON_TOP primary power ack */
	GDL_HW_POLL_CONN_INFRA_ENTRY(CONN_INFRA_RGU_ON_BGFSYS_ON_TOP_PWR_ACK_ST_BGFSYS_ON_TOP_PWR_ACK, 1,
		POLL_DEFAULT, &poll_okay);
	if (!poll_okay) {
		GDL_LOGE("_fail_bgf_top_on_1st_pwr_ack_not_okay");
		goto _fail_bgf_top_1st_pwr_ack_not_okay;
	}

	/* bit25: BGFSYS_ON_TOP secondary power ack */
	GDL_HW_POLL_CONN_INFRA_ENTRY(CONN_INFRA_RGU_ON_BGFSYS_ON_TOP_PWR_ACK_ST_AN_BGFSYS_ON_TOP_PWR_ACK_S, 1,
		POLL_DEFAULT, &poll_okay);
	if (!poll_okay) {
		GDL_LOGE("_fail_bgf_top_on_2nd_pwr_ack_not_okay");
		goto _fail_bgf_top_2nd_pwr_ack_not_okay;
	}

	/* Enable GPS function */
	GDL_HW_SET_GPS_FUNC_EN(1);

	/* bit24: BGFSYS_OFF_TOP primary power ack */
	GDL_HW_POLL_CONN_INFRA_ENTRY(CONN_INFRA_RGU_ON_BGFSYS_OFF_TOP_PWR_ACK_ST_BGFSYS_OFF_TOP_PWR_ACK, 1,
		POLL_DEFAULT, &poll_okay);
	if (!poll_okay) {
		GDL_LOGE("_fail_bgf_top_off_1st_pwr_ack_not_okay");
		goto _fail_bgf_top_1st_pwr_ack_not_okay;
	}

	/* bit25: BGFSYS_OFF_TOP secondary power ack */
	GDL_HW_POLL_CONN_INFRA_ENTRY(CONN_INFRA_RGU_ON_BGFSYS_OFF_TOP_PWR_ACK_ST_BGFSYS_OFF_TOP_PWR_ACK_S, 1,
		POLL_DEFAULT, &poll_okay);
	if (!poll_okay) {
		GDL_LOGE("_fail_bgf_top_off_2nd_pwr_ack_not_okay");
		goto _fail_bgf_top_2nd_pwr_ack_not_okay;
	}
	return true; /* okay */

_fail_bgf_top_1st_pwr_ack_not_okay:
_fail_bgf_top_2nd_pwr_ack_not_okay:
	return false; /* fail */
}


bool gps_dl_hw_dep_poll_bgf_bus_and_gps_top_ack(void)
{
	bool poll_okay = false;
	int i;

	/* 0x18c21010[31:0] bgf ip version */
	GDL_HW_POLL_GPS_ENTRY(BGF_GPS_CFG_BGF_IP_VERSION_BGFSYS_VERSION,
		GDL_HW_BGF_VER_MT6893, POLL_DEFAULT, &poll_okay);
	if (!poll_okay) {
		GDL_LOGE("_fail_bgf_ip_ver_not_okay");
		goto _fail_bgf_ip_ver_not_okay;
	}

	/* 0x18c21014[7:0] bgf ip cfg */
	GDL_HW_POLL_GPS_ENTRY(BGF_GPS_CFG_BGF_IP_CONFIG_BGFSYS_CONFIG, 0,
		POLL_DEFAULT, &poll_okay);
	if (!poll_okay) {
		GDL_LOGE("_fail_bgf_ip_cfg_not_okay");
		goto _fail_bgf_ip_cfg_not_okay;
	}

	/* 0x18c0102c[3:0] == 4h'2 gps_top_off is GPS_ACTIVE state */
	for (i = 0; i < 3; i++) {
		GDL_HW_POLL_GPS_ENTRY(BGF_GPS_CFG_ON_GPS_TOP_OFF_PWR_CTL_GPS_TOP_OFF_PWR_CTL_CS, 2,
			POLL_DEFAULT, &poll_okay);
		if (poll_okay)
			break;
		/*
		 * TODO:
		 * if (!gps_dl_reset_level_is_none()) break;
		 */
		if (i > 0)
			GDL_LOGW("_poll_gps_top_off_active, cnt = %d", i + 1);
	}
	if (!poll_okay) {
		GDL_LOGE("_fail_gps_top_off_active_not_okay");
		goto _fail_gps_top_off_active_not_okay;
	}
	return true;

_fail_gps_top_off_active_not_okay:
_fail_bgf_ip_ver_not_okay:
_fail_bgf_ip_cfg_not_okay:
	return false;
}

void gps_dl_hw_dep_may_set_bus_debug_flag(void)
{
	/* Do nothing for MT6893 */
}

