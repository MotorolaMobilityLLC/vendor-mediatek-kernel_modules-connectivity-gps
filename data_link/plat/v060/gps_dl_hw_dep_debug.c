/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */
#include "gps_dl_config.h"

#include "gps_dl_hw_ver.h"
#include "gps_dl_hw_dep_api.h"
#include "gps_dl_hw_dep_macro.h"

#include "../gps_dl_hw_priv_util.h"

void gps_dl_hw_dep_dump_gps_pos_info(enum gps_dl_link_id_enum link_id)
{
	/* TODO */
}

void gps_dl_hw_dep_dump_host_csr_range(unsigned int flag_start, unsigned int len)
{
	/* TODO */
}

void gps_dl_hw_dep_dump_host_csr_gps_info(void)
{
	unsigned int flag;
	const struct gps_dl_hw_host_csr_dump_range *p_range;
	int i, j;

	gps_dl_bus_rd_opt(GPS_DL_CONN_INFRA_BUS,
		CONN_DBG_CTL_CR_DBGCTL2BGF_OFF_DEBUG_SEL_ADDR,
		BMASK_RW_FORCE_PRINT);
	gps_dl_bus_rd_opt(GPS_DL_CONN_INFRA_BUS,
		CONN_DBG_CTL_BGF_MONFLAG_OFF_OUT_ADDR,
		BMASK_RW_FORCE_PRINT);

	for (i = 0; i < g_gps_v06x_host_csr_dump_range_num; i++) {
		p_range = &g_gps_v06x_host_csr_dump_range_ptr[i];
		for (j = 0; j < p_range->len; j++) {
			flag = j + p_range->flag_start;
			gps_dl_bus_wr_opt(GPS_DL_CONN_INFRA_BUS,
				CONN_DBG_CTL_CR_DBGCTL2BGF_OFF_DEBUG_SEL_ADDR, flag,
				BMASK_RW_FORCE_PRINT);
			gps_dl_bus_rd_opt(GPS_DL_CONN_INFRA_BUS,
				CONN_DBG_CTL_BGF_MONFLAG_OFF_OUT_ADDR,
				BMASK_RW_FORCE_PRINT);
		}
	}
}

void gps_dl_hw_dep_dump_host_csr_conninfra_info(void)
{
	/* TODO */
}

void gps_dl_hw_dep_may_do_bus_check_and_print(unsigned int host_addr)
{
	/* TODO */
}

void gps_dl_hw_gps_dump_gps_rf_temp_cr(void)
{
	/* TODO */
}

void gps_dl_hw_dep_gps_dump_power_state(void)
{
#define BGF_LP_DBG_DUMP_LEN (5)
	unsigned int is_fw_own = 0;
	unsigned int conn_wake_by_top = 0, conn_wake_by_gps = 0;
	unsigned int clock_det = 0;
	unsigned int conn_pwr_st = 0;

	unsigned int bgf_dummy = 0;
	unsigned int bgf_dbg_30004a = 0, bgf_dbg_30004b = 0;
	unsigned int bgf_dbg_300040[BGF_LP_DBG_DUMP_LEN] = {0};
	unsigned int i;

	is_fw_own = GDL_HW_RD_CONN_INFRA_REG(CONN_HOST_CSR_TOP_BGF_LPCTL_ADDR);
	conn_wake_by_top = GDL_HW_RD_CONN_INFRA_REG(CONN_HOST_CSR_TOP_CONN_INFRA_WAKEPU_TOP_ADDR);
	conn_wake_by_gps = GDL_HW_RD_CONN_INFRA_REG(CONN_HOST_CSR_TOP_CONN_INFRA_WAKEPU_GPS_ADDR);

	clock_det = GDL_HW_RD_CONN_INFRA_REG(CONN_DBG_CTL_CLOCK_DETECT_ADDR);
	conn_pwr_st = GDL_HW_RD_CONN_INFRA_REG(CONN_HOST_CSR_TOP_CONNSYS_PWR_STATES_ADDR);

	GDL_HW_WR_CONN_INFRA_REG(CONN_HOST_CSR_TOP_CR_HOSTCSR2BGF_ON_DBG_SEL_ADDR, 0x200c00);
	bgf_dummy = GDL_HW_RD_CONN_INFRA_REG(CONN_HOST_CSR_TOP_BGF_MONFLG_ON_OUT_ADDR);

	for (i = 0; i < BGF_LP_DBG_DUMP_LEN; i++) {
		GDL_HW_WR_CONN_INFRA_REG(CONN_HOST_CSR_TOP_CR_HOSTCSR2BGF_ON_DBG_SEL_ADDR, (0x300040 + i));
		bgf_dbg_300040[i] = GDL_HW_RD_CONN_INFRA_REG(CONN_HOST_CSR_TOP_BGF_MONFLG_ON_OUT_ADDR);
	}
	GDL_HW_WR_CONN_INFRA_REG(CONN_HOST_CSR_TOP_CR_HOSTCSR2BGF_ON_DBG_SEL_ADDR, 0x30004a);
	bgf_dbg_30004a = GDL_HW_RD_CONN_INFRA_REG(CONN_HOST_CSR_TOP_BGF_MONFLG_ON_OUT_ADDR);
	GDL_HW_WR_CONN_INFRA_REG(CONN_HOST_CSR_TOP_CR_HOSTCSR2BGF_ON_DBG_SEL_ADDR, 0x30004b);
	bgf_dbg_30004b = GDL_HW_RD_CONN_INFRA_REG(CONN_HOST_CSR_TOP_BGF_MONFLG_ON_OUT_ADDR);

	GDL_LOGI("fw_own=%u, conn_wk:t/g=%d/%d, clk_det=0x%x, conn_pwr=0x%x, bgf_dmy=0x%x",
		is_fw_own, conn_wake_by_top, conn_wake_by_gps, clock_det, conn_pwr_st, bgf_dummy);

	GDL_LOGI("bgf_dbg_300040: [0-4]=0x%x,0x%x,0x%x,0x%x,0x%x [a]=0x%x, [b]=0x%x",
		bgf_dbg_300040[0],
		bgf_dbg_300040[1],
		bgf_dbg_300040[2],
		bgf_dbg_300040[3],
		bgf_dbg_300040[4], /* next to BGF_LP_DBG_DUMP_LEN */
		bgf_dbg_30004a,
		bgf_dbg_30004b);
}

