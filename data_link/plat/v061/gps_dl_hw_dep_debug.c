/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */
#include "gps_dl_config.h"

#include "gps_dl_hw_ver.h"
#include "gps_dl_hw_dep_api.h"
#include "gps_dl_hw_dep_macro.h"
#include "gps_dl_hw_atf.h"

#include "../gps_dl_hw_priv_util.h"

void gps_dl_hw_dep_dump_gps_pos_info(enum gps_dl_link_id_enum link_id)
{
	/* TODO */
}

void gps_dl_hw_dep_dump_host_csr_range(unsigned int flag_start, unsigned int len)
{
	struct arm_smccc_res res;
	unsigned int flag, atf_ret, flag2, out;
	#define HOST_CSR_PRINT_LINE_MAX (8)
	unsigned int print_list[HOST_CSR_PRINT_LINE_MAX];
	unsigned int print_flag;
	unsigned int non_print_cnt;

	non_print_cnt = 0;
	memset(&print_list[0], 0, sizeof(print_list));

	for (flag = flag_start; flag < (flag_start + len); flag++) {
		if (non_print_cnt >= HOST_CSR_PRINT_LINE_MAX) {
			GDL_LOGW("flag=0x%x,cnt=%d,out=0x%08x,0x%08x,0x%08x,0x%08x,0x%08x,0x%08x,0x%08x,0x%08x",
				print_flag, non_print_cnt,
				print_list[0], print_list[1], print_list[2], print_list[3],
				print_list[4], print_list[5], print_list[6], print_list[7]);
			non_print_cnt = 0;
			memset(&print_list[0], 0, sizeof(print_list));
		}

		arm_smccc_smc(MTK_SIP_KERNEL_GPS_CONTROL, SMC_GPS_DL_HW_DEP_SET_HOST_CSR_GPS_DBG_SEL_OPID,
			flag, 0, 0, 0, 0, 0, &res);
		atf_ret = res.a0;
		flag2 = res.a1;
		out = res.a2;
		if (flag != flag2 || atf_ret != 0)
			GDL_LOGW("atf_ret=%d, flag=0x%08x,0x%08x, out=0x%08x", atf_ret, flag, flag2, out);

		if (non_print_cnt == 0)
			print_flag = flag;
		print_list[non_print_cnt++] = out;
	}

	if (non_print_cnt != 0) {
		GDL_LOGW("flag=0x%x,cnt=%d,out=0x%08x,0x%08x,0x%08x,0x%08x,0x%08x,0x%08x,0x%08x,0x%08x",
			print_flag, non_print_cnt,
			print_list[0], print_list[1], print_list[2], print_list[3],
			print_list[4], print_list[5], print_list[6], print_list[7]);
	}
}

void gps_dl_hw_dep_dump_host_csr_gps_info(void)
{
	int i;
	const struct gps_dl_hw_host_csr_dump_range *p_range;

	for (i = 0; i < g_gps_v06x_host_csr_dump_range_num; i++) {
		p_range = &g_gps_v06x_host_csr_dump_range_ptr[i];
		gps_dl_hw_dep_dump_host_csr_range(p_range->flag_start, p_range->len);
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

static void gps_dl_hw_dep_set_bgf_on_dbg_sel(unsigned int flag_value)
{
	struct arm_smccc_res res;
	unsigned int atf_ret, flag2;

	memset(&res, 0, sizeof(res));
	arm_smccc_smc(MTK_SIP_KERNEL_GPS_CONTROL, SMC_GPS_DL_HW_DEP_SET_HOST_CSR2BGF_DBG_SEL_OPID,
		flag_value, 0, 0, 0, 0, 0, &res);
	atf_ret = res.a0;
	flag2 = res.a1;
	if (flag_value != flag2 || atf_ret != 0)
		GDL_LOGW("atf_ret=%d, flag=0x%08x,0x%08x", atf_ret, flag_value, flag2);
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

	gps_dl_hw_dep_set_bgf_on_dbg_sel(0x200c00);
	bgf_dummy = GDL_HW_RD_CONN_INFRA_REG(CONN_HOST_CSR_TOP_BGF_MONFLG_ON_OUT_ADDR);

	for (i = 0; i < BGF_LP_DBG_DUMP_LEN; i++) {
		gps_dl_hw_dep_set_bgf_on_dbg_sel((0x300040 + i));
		bgf_dbg_300040[i] = GDL_HW_RD_CONN_INFRA_REG(CONN_HOST_CSR_TOP_BGF_MONFLG_ON_OUT_ADDR);
	}
	gps_dl_hw_dep_set_bgf_on_dbg_sel(0x30004a);
	bgf_dbg_30004a = GDL_HW_RD_CONN_INFRA_REG(CONN_HOST_CSR_TOP_BGF_MONFLG_ON_OUT_ADDR);
	gps_dl_hw_dep_set_bgf_on_dbg_sel(0x30004b);
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

