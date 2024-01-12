/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */
#include "gps_dl_config.h"
#include "gps_dl_hw_ver.h"
#include "gps_dl_hw_dep_api.h"
#include "../gps_dl_hw_priv_util.h"
#include "gps_dl_hw_dep_macro.h"
#include "conn_infra/conn_semaphore.h"
#include "conn_infra/conn_vdnr_gen_on_debug_ctrl_ao.h"
#include "gps_dl_context.h"

/*
 * COS_SEMA_BGF_SEL_INDEX = 0
 * GPS use M5
 */
#define COS_SEMA_BGF_SEL_STA_ENTRY_FOR_GPS \
	CONN_SEMAPHORE_CONN_SEMA00_M5_OWN_STA_CONN_SEMA00_M5_OWN_STA

#define COS_SEMA_BGF_SEL_REL_ENTRY_FOR_GPS \
	CONN_SEMAPHORE_CONN_SEMA00_M5_OWN_REL_CONN_SEMA00_M5_OWN_REL

bool gps_dl_hw_take_conn_bgf_sel_hw_sema(unsigned int try_timeout_ms)
{
	bool okay;
	bool show_log;
	unsigned int poll_us, poll_max_us;
	unsigned int val;

	show_log = gps_dl_set_show_reg_rw_log(true);
	/* poll until value is expected or timeout */
	poll_us = 0;
	poll_max_us = POLL_US * 1000 * try_timeout_ms;
	okay = false;
	while (!okay) {
		val = GDL_HW_GET_CONN_INFRA_ENTRY(COS_SEMA_BGF_SEL_STA_ENTRY_FOR_GPS);
		/* 2bit value:
		 * 0 -> need waiting
		 * 1,3 -> okay; 2 -> already taken
		 */
		if (val != 0) {
			okay = true;
			break;
		}
		if (poll_us >= poll_max_us) {
			okay = false;
			break;
		}
		gps_dl_wait_us(POLL_INTERVAL_US);
		poll_us += POLL_INTERVAL_US;
	}
	gps_dl_set_show_reg_rw_log(show_log);

	if (!okay)
		GDL_LOGW("okay = %d", okay);
	else
		GDL_LOGD("okay = %d", okay);
	return okay;
}

void gps_dl_hw_give_conn_bgf_sel_hw_sema(void)
{
	bool show_log;

	show_log = gps_dl_set_show_reg_rw_log(true);
	GDL_HW_SET_CONN_INFRA_ENTRY(COS_SEMA_BGF_SEL_REL_ENTRY_FOR_GPS, 1);
	gps_dl_set_show_reg_rw_log(show_log);

	GDL_LOGD("");
}

void gps_dl_hw_dep_dump_host_csr_gps_info(void)
{
	int i;
	bool okay = false;

	okay = gps_dl_hw_take_conn_bgf_sel_hw_sema(100);
	if (!okay) {
		GDL_LOGE("gps_dl_hw_take_conn_bgf_sel_hw_sema fail");
		return;
	}

	gps_dl_bus_rd_opt(GPS_DL_CONN_INFRA_BUS,
		CONN_HOST_CSR_TOP_HOST2GPS_DEGUG_SEL_ADDR,
		BMASK_RW_FORCE_PRINT);
	gps_dl_bus_rd_opt(GPS_DL_CONN_INFRA_BUS,
		CONN_HOST_CSR_TOP_GPS_CFG2HOST_DEBUG_ADDR,
		BMASK_RW_FORCE_PRINT);

	for (i = 0xA2; i <= 0xBA; i++) {
		gps_dl_bus_wr_opt(GPS_DL_CONN_INFRA_BUS,
			CONN_HOST_CSR_TOP_HOST2GPS_DEGUG_SEL_ADDR, i,
			BMASK_RW_FORCE_PRINT);
		gps_dl_bus_rd_opt(GPS_DL_CONN_INFRA_BUS,
			CONN_HOST_CSR_TOP_GPS_CFG2HOST_DEBUG_ADDR,
			BMASK_RW_FORCE_PRINT);
	}

	gps_dl_hw_give_conn_bgf_sel_hw_sema();

}

static void gps_dl_hw_dump_host_csr_conninfra_info_inner(void)
{

	/* Due to RW_DO_CHECK might be enabled, not use
	 * GDL_HW_SET_CONN_INFRA_ENTRY and GDL_HW_GET_CONN_INFRA_ENTRY to avoid redundant print.
	 */
	gps_dl_bus_rd_opt(GPS_DL_CONN_INFRA_BUS,
		CONN_VDNR_GEN_ON_DEBUG_CTRL_AO_CONN_INFRA_VDNR_GEN_ON_U_DEBUG_CTRL_AO_CONN_INFRA_ON_RESULT2_ADDR,
		BMASK_RW_FORCE_PRINT);
	gps_dl_bus_rd_opt(GPS_DL_CONN_INFRA_BUS,
		CONN_VDNR_GEN_ON_DEBUG_CTRL_AO_CONN_INFRA_VDNR_GEN_ON_U_DEBUG_CTRL_AO_CONN_INFRA_ON_RESULT3_ADDR,
		BMASK_RW_FORCE_PRINT);
	gps_dl_bus_rd_opt(GPS_DL_CONN_INFRA_BUS,
		CONN_VDNR_GEN_ON_DEBUG_CTRL_AO_CONN_INFRA_VDNR_GEN_ON_U_DEBUG_CTRL_AO_CONN_INFRA_ON_RESULT4_ADDR,
		BMASK_RW_FORCE_PRINT);
	gps_dl_bus_rd_opt(GPS_DL_CONN_INFRA_BUS,
		CONN_VDNR_GEN_ON_DEBUG_CTRL_AO_CONN_INFRA_VDNR_GEN_ON_U_DEBUG_CTRL_AO_CONN_INFRA_ON_RESULT5_ADDR,
		BMASK_RW_FORCE_PRINT);
	gps_dl_bus_rd_opt(GPS_DL_CONN_INFRA_BUS,
		CONN_VDNR_GEN_ON_DEBUG_CTRL_AO_CONN_INFRA_VDNR_GEN_ON_U_DEBUG_CTRL_AO_CONN_INFRA_ON_RESULT6_ADDR,
		BMASK_RW_FORCE_PRINT);
	gps_dl_bus_rd_opt(GPS_DL_CONN_INFRA_BUS,
		CONN_VDNR_GEN_ON_DEBUG_CTRL_AO_CONN_INFRA_VDNR_GEN_ON_U_DEBUG_CTRL_AO_CONN_INFRA_ON_RESULT7_ADDR,
		BMASK_RW_FORCE_PRINT);
	gps_dl_bus_rd_opt(GPS_DL_CONN_INFRA_BUS,
		CONN_VDNR_GEN_ON_DEBUG_CTRL_AO_CONN_INFRA_VDNR_GEN_ON_U_DEBUG_CTRL_AO_CONN_INFRA_ON_RESULT8_ADDR,
		BMASK_RW_FORCE_PRINT);
	gps_dl_bus_rd_opt(GPS_DL_CONN_INFRA_BUS,
		CONN_VDNR_GEN_ON_DEBUG_CTRL_AO_CONN_INFRA_VDNR_GEN_ON_U_DEBUG_CTRL_AO_CONN_INFRA_ON_RESULT9_ADDR,
		BMASK_RW_FORCE_PRINT);
	gps_dl_bus_rd_opt(GPS_DL_CONN_INFRA_BUS,
		CONN_VDNR_GEN_ON_DEBUG_CTRL_AO_CONN_INFRA_VDNR_GEN_ON_U_DEBUG_CTRL_AO_CONN_INFRA_ON_RESULT10_ADDR,
		BMASK_RW_FORCE_PRINT);
	gps_dl_bus_rd_opt(GPS_DL_CONN_INFRA_BUS,
		CONN_VDNR_GEN_ON_DEBUG_CTRL_AO_CONN_INFRA_VDNR_GEN_ON_U_DEBUG_CTRL_AO_CONN_INFRA_ON_RESULT11_ADDR,
		BMASK_RW_FORCE_PRINT);
	gps_dl_bus_rd_opt(GPS_DL_CONN_INFRA_BUS,
		CONN_VDNR_GEN_ON_DEBUG_CTRL_AO_CONN_INFRA_VDNR_GEN_ON_U_DEBUG_CTRL_AO_CONN_INFRA_ON_RESULT12_ADDR,
		BMASK_RW_FORCE_PRINT);

}

void gps_dl_hw_dep_dump_host_csr_conninfra_info(void)
{
	gps_dl_hw_dump_host_csr_conninfra_info_inner();
}

void gps_dl_hw_dep_may_do_bus_check_and_print(unsigned int host_addr)
{
	/* not do rw check because here is the checking */
	GDL_LOGI("for addr = 0x%08x", host_addr);

	gps_dl_bus_rd_opt(GPS_DL_CONN_INFRA_BUS,
		CONN_VDNR_GEN_ON_DEBUG_CTRL_AO_CONN_INFRA_VDNR_GEN_ON_U_DEBUG_CTRL_AO_CONN_INFRA_ON_RESULT10_ADDR,
		BMASK_RW_FORCE_PRINT);
	gps_dl_bus_rd_opt(GPS_DL_CONN_INFRA_BUS,
		CONN_VDNR_GEN_ON_DEBUG_CTRL_AO_CONN_INFRA_VDNR_GEN_ON_U_DEBUG_CTRL_AO_CONN_INFRA_ON_RESULT11_ADDR,
		BMASK_RW_FORCE_PRINT);
	gps_dl_bus_rd_opt(GPS_DL_CONN_INFRA_BUS,
		CONN_VDNR_GEN_ON_DEBUG_CTRL_AO_CONN_INFRA_VDNR_GEN_ON_U_DEBUG_CTRL_AO_CONN_INFRA_ON_RESULT12_ADDR,
		BMASK_RW_FORCE_PRINT);
}

