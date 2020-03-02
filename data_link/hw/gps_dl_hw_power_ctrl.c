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
#include "gps_dl_config.h"
#include "gps_dl_context.h"

#include "gps_dl_hal.h"
#if GPS_DL_MOCK_HAL
#include "gps_mock_hal.h"
#endif
#if GPS_DL_HAS_CONNINFRA_DRV
#if GPS_DL_ON_LINUX
#include "conninfra.h"
#elif GPS_DL_ON_CTP
#include "conninfra_ext.h"
#endif
#endif

#include "gps_dl_hw_api.h"
#include "gps_dl_hw_priv_util.h"
#include "gps_dl_hal_util.h"
#include "gps_dsp_fsm.h"
#include "gps_dl_subsys_reset.h"

#include "conn_infra/conn_infra_rgu.h"
#include "conn_infra/conn_infra_cfg.h"
#include "conn_infra/conn_host_csr_top.h"

#include "gps/gps_rgu_on.h"
#include "gps/gps_cfg_on.h"
#include "gps/gps_aon_top.h"
#include "gps/bgf_gps_cfg.h"

static int gps_dl_hw_gps_sleep_prot_ctrl(int op)
{
	bool poll_okay;

	if (1 == op) {
		/* disable when on */
		GDL_HW_SET_CONN_INFRA_ENTRY(CONN_INFRA_CFG_GALS_CONN2GPS_SLP_CTRL_R_CONN2GPS_SLP_PROT_RX_EN,  0);
		GDL_HW_POLL_CONN_INFRA_ENTRY(CONN_INFRA_CFG_GALS_CONN2GPS_SLP_CTRL_R_CONN2GPS_SLP_PROT_RX_RDY, 0,
			POLL_DEFAULT, &poll_okay);
		if (!poll_okay) {
			GDL_LOGE("_fail_disable_gps_slp_prot");
			goto _fail_disable_gps_slp_prot;
		}

		GDL_HW_SET_CONN_INFRA_ENTRY(CONN_INFRA_CFG_GALS_CONN2GPS_SLP_CTRL_R_CONN2GPS_SLP_PROT_TX_EN,  0);
		GDL_HW_POLL_CONN_INFRA_ENTRY(CONN_INFRA_CFG_GALS_CONN2GPS_SLP_CTRL_R_CONN2GPS_SLP_PROT_TX_RDY, 0,
			POLL_DEFAULT, &poll_okay);
		if (!poll_okay) {
			GDL_LOGE("_fail_disable_gps_slp_prot");
			goto _fail_disable_gps_slp_prot;
		}

		GDL_HW_SET_CONN_INFRA_ENTRY(CONN_INFRA_CFG_GALS_GPS2CONN_SLP_CTRL_R_GPS2CONN_SLP_PROT_RX_EN,  0);
		GDL_HW_POLL_CONN_INFRA_ENTRY(CONN_INFRA_CFG_GALS_GPS2CONN_SLP_CTRL_R_GPS2CONN_SLP_PROT_RX_RDY, 0,
			POLL_DEFAULT, &poll_okay);
		if (!poll_okay) {
			GDL_LOGE("_fail_disable_gps_slp_prot");
			goto _fail_disable_gps_slp_prot;
		}

		GDL_HW_SET_CONN_INFRA_ENTRY(CONN_INFRA_CFG_GALS_GPS2CONN_SLP_CTRL_R_GPS2CONN_SLP_PROT_TX_EN,  0);
		GDL_HW_POLL_CONN_INFRA_ENTRY(CONN_INFRA_CFG_GALS_GPS2CONN_SLP_CTRL_R_GPS2CONN_SLP_PROT_TX_RDY, 0,
			POLL_DEFAULT, &poll_okay);
		if (!poll_okay) {
			GDL_LOGE("_fail_disable_gps_slp_prot");
			goto _fail_disable_gps_slp_prot;
		}
		return 0;

_fail_disable_gps_slp_prot:
#if 0
		GDL_HW_WR_CONN_INFRA_REG(CONN_INFRA_CFG_GALS_GPS2CONN_SLP_CTRL_ADDR,
			CONN_INFRA_CFG_GALS_CONN2GPS_SLP_CTRL_R_CONN2GPS_SLP_PROT_RX_EN_MASK |
			CONN_INFRA_CFG_GALS_CONN2GPS_SLP_CTRL_R_CONN2GPS_SLP_PROT_TX_EN_MASK |
			CONN_INFRA_CFG_GALS_GPS2CONN_SLP_CTRL_R_GPS2CONN_SLP_PROT_RX_EN_MASK |
			CONN_INFRA_CFG_GALS_GPS2CONN_SLP_CTRL_R_GPS2CONN_SLP_PROT_TX_EN_MASK);
#endif
		return -1;
	} else if (0 == op) {
		/* enable when off */
		GDL_HW_SET_CONN_INFRA_ENTRY(CONN_INFRA_CFG_GALS_CONN2GPS_SLP_CTRL_R_CONN2GPS_SLP_PROT_TX_EN,  1);
		GDL_HW_POLL_CONN_INFRA_ENTRY(CONN_INFRA_CFG_GALS_CONN2GPS_SLP_CTRL_R_CONN2GPS_SLP_PROT_TX_RDY, 1,
			POLL_DEFAULT, &poll_okay);

		GDL_HW_SET_CONN_INFRA_ENTRY(CONN_INFRA_CFG_GALS_CONN2GPS_SLP_CTRL_R_CONN2GPS_SLP_PROT_RX_EN,  1);
		GDL_HW_POLL_CONN_INFRA_ENTRY(CONN_INFRA_CFG_GALS_CONN2GPS_SLP_CTRL_R_CONN2GPS_SLP_PROT_RX_RDY, 1,
			POLL_DEFAULT, &poll_okay);

		GDL_HW_SET_CONN_INFRA_ENTRY(CONN_INFRA_CFG_GALS_GPS2CONN_SLP_CTRL_R_GPS2CONN_SLP_PROT_TX_EN,  1);
		GDL_HW_POLL_CONN_INFRA_ENTRY(CONN_INFRA_CFG_GALS_GPS2CONN_SLP_CTRL_R_GPS2CONN_SLP_PROT_TX_RDY, 1,
			POLL_DEFAULT, &poll_okay);

		GDL_HW_SET_CONN_INFRA_ENTRY(CONN_INFRA_CFG_GALS_GPS2CONN_SLP_CTRL_R_GPS2CONN_SLP_PROT_RX_EN,  1);
		GDL_HW_POLL_CONN_INFRA_ENTRY(CONN_INFRA_CFG_GALS_GPS2CONN_SLP_CTRL_R_GPS2CONN_SLP_PROT_RX_RDY, 1,
			POLL_DEFAULT, &poll_okay);
	}

	return 0;
}

void gps_dl_hw_gps_force_wakeup_conninfra_top_off(bool enable)
{
	if (enable)
		GDL_HW_SET_CONN_INFRA_ENTRY(CONN_HOST_CSR_TOP_CONN_INFRA_WAKEPU_GPS_CONN_INFRA_WAKEPU_GPS, 1);
	else
		GDL_HW_SET_CONN_INFRA_ENTRY(CONN_HOST_CSR_TOP_CONN_INFRA_WAKEPU_GPS_CONN_INFRA_WAKEPU_GPS, 0);
}

void gps_dl_hw_gps_sw_request_emi_usage(bool request)
{
	bool show_log;
	bool reg_rw_log = gps_dl_log_reg_rw_is_on(GPS_DL_REG_RW_EMI_SW_REQ_CTRL);

	if (reg_rw_log) {
		show_log = gps_dl_set_show_reg_rw_log(true);
		GDL_HW_RD_CONN_INFRA_REG(CONN_INFRA_CFG_EMI_CTL_TOP_ADDR);
		GDL_HW_RD_CONN_INFRA_REG(CONN_INFRA_CFG_EMI_CTL_WF_ADDR);
		GDL_HW_RD_CONN_INFRA_REG(CONN_INFRA_CFG_EMI_CTL_BT_ADDR);
		GDL_HW_RD_CONN_INFRA_REG(CONN_INFRA_CFG_EMI_CTL_GPS_ADDR);
	}

	if (request) {
		/* GDL_HW_SET_CONN_INFRA_ENTRY(CONN_INFRA_CFG_EMI_CTL_GPS_EMI_REQ_GPS, 1);
		 * CONN_INFRA_CFG_EMI_CTL_GPS used by DSP, driver use TOP's.
		 */
		GDL_HW_SET_CONN_INFRA_ENTRY(CONN_INFRA_CFG_EMI_CTL_TOP_EMI_REQ_TOP, 1);
	} else {
		/* GDL_HW_SET_CONN_INFRA_ENTRY(CONN_INFRA_CFG_EMI_CTL_GPS_EMI_REQ_GPS, 0); */
		GDL_HW_SET_CONN_INFRA_ENTRY(CONN_INFRA_CFG_EMI_CTL_TOP_EMI_REQ_TOP, 0);
	}

	if (reg_rw_log)
		gps_dl_set_show_reg_rw_log(show_log);
}

int gps_dl_hw_gps_common_on(void)
{
	bool poll_okay;
	int i;

	/* Conninfra driver will do it
	 * GDL_HW_SET_CONN_INFRA_ENTRY(CONN_INFRA_RGU_BGFYS_ON_TOP_PWR_CTL_BGFSYS_ON_TOP_PWR_ON, 1);
	 */

	/* Wait until sleep prot disabled, 10 times per 1ms */
	GDL_HW_POLL_CONN_INFRA_ENTRY(
		CONN_HOST_CSR_TOP_CONN_SLP_PROT_CTRL_CONN_INFRA_ON2OFF_SLP_PROT_ACK, 0,
		10 * 1000 * POLL_US, &poll_okay);
	if (!poll_okay) {
		GDL_LOGE("_fail_conn_slp_prot_not_okay");
		goto _fail_conn_slp_prot_not_okay;
	}

	/* Poll conninfra hw version */
	GDL_HW_POLL_CONN_INFRA_ENTRY(CONN_INFRA_CFG_CONN_HW_VER_RO_CONN_HW_VERSION, 0x20010000,
		POLL_DEFAULT, &poll_okay);
	if (!poll_okay) {
		GDL_LOGE("_fail_conn_hw_ver_not_okay");
		goto _fail_conn_hw_ver_not_okay;
	}
	/* GPS SW EMI request
	 * gps_dl_hw_gps_sw_request_emi_usage(true);
	 */
	gps_dl_hal_emi_usage_init();

	/* Enable GPS function */
	GDL_HW_SET_CONN_INFRA_ENTRY(CONN_INFRA_CFG_GPS_PWRCTRL0_GP_FUNCTION_EN, 1);

	/* bit24: BGFSYS_ON_TOP primary power ack */
	GDL_HW_POLL_CONN_INFRA_ENTRY(CONN_INFRA_RGU_BGFSYS_ON_TOP_PWR_ACK_ST_BGFSYS_ON_TOP_PWR_ACK, 1,
		POLL_DEFAULT, &poll_okay);
	if (!poll_okay) {
		GDL_LOGE("_fail_bgf_top_1st_pwr_ack_not_okay");
		goto _fail_bgf_top_1st_pwr_ack_not_okay;
	}

	/* bit25: BGFSYS_ON_TOP secondary power ack */
	GDL_HW_POLL_CONN_INFRA_ENTRY(CONN_INFRA_RGU_BGFSYS_ON_TOP_PWR_ACK_ST_AN_BGFSYS_ON_TOP_PWR_ACK_S, 1,
		POLL_DEFAULT, &poll_okay);
	if (!poll_okay) {
		GDL_LOGE("_fail_bgf_top_2nd_pwr_ack_not_okay");
		goto _fail_bgf_top_2nd_pwr_ack_not_okay;
	}

	GDL_WAIT_US(200);

	/* sleep prot */
	if (gps_dl_hw_gps_sleep_prot_ctrl(1) != 0) {
		GDL_LOGE("_fail_disable_gps_slp_prot_not_okay");
		goto _fail_disable_gps_slp_prot_not_okay;
	}

	/* polling status and version */
	/* Todo: set GPS host csr flag selection */
	/* 0x18060240[3:0] == 4h'2 gps_top_off is GPS_ACTIVE state */
	GDL_HW_SET_CONN_INFRA_ENTRY(CONN_HOST_CSR_TOP_HOST2GPS_DEGUG_SEL_HOST2GPS_DEGUG_SEL, 0x80);
	for (i = 0; i < 3; i++) {
		GDL_HW_POLL_CONN_INFRA_ENTRY(CONN_HOST_CSR_TOP_GPS_CFG2HOST_DEBUG_GPS_CFG2HOST_DEBUG, 2,
			POLL_DEFAULT, &poll_okay);
		if (poll_okay)
			break;
		/*
		 * TODO:
		 * if (!gps_dl_reset_level_is_none()) break;
		 */
		GDL_LOGW("_poll_gps_top_off_active, cnt = %d", i + 1);
	}
	if (!poll_okay) {
		GDL_LOGE("_fail_gps_top_off_active_not_okay");
		goto _fail_gps_top_off_active_not_okay;
	}

	/* 0x18c21010[31:0] bgf ip version */
	GDL_HW_POLL_GPS_ENTRY(BGF_GPS_CFG_BGF_IP_VERSION_BGFSYS_VERSION, 0x20010000,
		POLL_DEFAULT, &poll_okay);
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

	/* Enable conninfra bus hung detection */
	GDL_HW_WR_CONN_INFRA_REG(0x1800F000, 0xF000001C);

	/* host csr gps bus debug mode enable 0x18c60000 = 0x10 */
	GDL_HW_WR_GPS_REG(0x80060000, 0x10);

	/* Power on A-die top clock */
#if (GPS_DL_HAS_CONNINFRA_DRV)
	conninfra_adie_top_ck_en_on(CONNSYS_ADIE_CTL_HOST_GPS);
#endif

	/* Enable PLL driver */
	GDL_HW_SET_GPS_ENTRY(GPS_CFG_ON_GPS_CLKGEN1_CTL_CR_GPS_DIGCK_DIV_EN, 1);

	return 0;

_fail_bgf_ip_cfg_not_okay:
_fail_bgf_ip_ver_not_okay:
_fail_gps_top_off_active_not_okay:
_fail_disable_gps_slp_prot_not_okay:
_fail_bgf_top_2nd_pwr_ack_not_okay:
_fail_bgf_top_1st_pwr_ack_not_okay:
	GDL_HW_SET_CONN_INFRA_ENTRY(CONN_INFRA_CFG_GPS_PWRCTRL0_GP_FUNCTION_EN, 0);
	GDL_HW_SET_CONN_INFRA_ENTRY(CONN_INFRA_CFG_EMI_CTL_GPS_EMI_REQ_GPS, 0);

_fail_conn_hw_ver_not_okay:
_fail_conn_slp_prot_not_okay:
	return -1;
}

void gps_dl_hw_gps_common_off(void)
{
	/* Power off A-die top clock */
#if (GPS_DL_HAS_CONNINFRA_DRV)
	conninfra_adie_top_ck_en_off(CONNSYS_ADIE_CTL_HOST_GPS);
#endif

	gps_dl_hw_gps_sleep_prot_ctrl(0);

	/* GPS SW EMI request
	 * gps_dl_hw_gps_sw_request_emi_usage(false);
	 */
	gps_dl_hal_emi_usage_deinit();

	if (gps_dl_log_reg_rw_is_on(GPS_DL_REG_RW_HOST_CSR_GPS_OFF))
		gps_dl_hw_dump_host_csr_conninfra_info(true);

	/* Disable GPS function */
	GDL_HW_SET_CONN_INFRA_ENTRY(CONN_INFRA_CFG_GPS_PWRCTRL0_GP_FUNCTION_EN, 0);
}

int gps_dl_hw_gps_dsp_ctrl(enum dsp_ctrl_enum ctrl)
{
	bool poll_okay;
	bool dsp_off_done;

	switch (ctrl) {
	case GPS_L1_DSP_ON:
		GDL_HW_SET_GPS_ENTRY(GPS_RGU_ON_GPS_L1_CR_RGU_GPS_L1_ON, 1);
		GDL_HW_SET_GPS_ENTRY(GPS_RGU_ON_GPS_L1_CR_RGU_GPS_L1_SOFT_RST_B, 1);
		GDL_HW_POLL_GPS_ENTRY(GPS_CFG_ON_GPS_L1_SLP_PWR_CTL_GPS_L1_SLP_PWR_CTL_CS, 3,
			POLL_DEFAULT, &poll_okay);
		if (!poll_okay) {
			GDL_LOGE("ctrl = %d fail", ctrl);
			return -1;
		}

		GDL_HW_SET_GPS_ENTRY(GPS_RGU_ON_GPS_L1_MEM_DLY_CTL_RGU_GPSSYS_L1_MEM_ADJ_DLY_EN, 1);
		GDL_HW_SET_GPS_ENTRY(GPS_RGU_ON_GPS_L1_DLY_CHAIN_CTL_RGU_GPS_L1_MEM_PDN_DELAY_DUMMY_NUM, 5);
		gps_dl_wait_us(1000); /* 3 x 32k clk ~= 1ms */
		break;

	case GPS_L1_DSP_OFF:
	case GPS_L1_DSP_ENTER_DSTOP:
	case GPS_L1_DSP_ENTER_DSLEEP:
		if (ctrl == GPS_L1_DSP_ENTER_DSLEEP) {
			GDL_HW_SET_GPS_ENTRY(GPS_AON_TOP_DSLEEP_CTL_GPS_PWR_STAT, 3);
			GDL_HW_SET_GPS_ENTRY(GPS_AON_TOP_DSLEEP_CTL_FORCE_OSC_EN_ON, 1);

			GDL_HW_SET_GPS_ENTRY(GPS_RGU_ON_GPS_L1_DSPPRAM_PDN_EN_RGU_GPS_L1_PRAM_HWCTL_PDN, 0);
			GDL_HW_SET_GPS_ENTRY(GPS_RGU_ON_GPS_L1_DSPPRAM_SLP_EN_RGU_GPS_L1_PRAM_HWCTL_SLP, 0x1FF);
			GDL_HW_SET_GPS_ENTRY(GPS_RGU_ON_GPS_L1_DSPXRAM_PDN_EN_RGU_GPS_L1_XRAM_HWCTL_PDN, 0);
			GDL_HW_SET_GPS_ENTRY(GPS_RGU_ON_GPS_L1_DSPXRAM_SLP_EN_RGU_GPS_L1_XRAM_HWCTL_SLP, 0xF);
			GDL_HW_SET_GPS_ENTRY(GPS_RGU_ON_GPS_L1_DSPYRAM_PDN_EN_RGU_GPS_L1_YRAM_HWCTL_PDN, 0);
			GDL_HW_SET_GPS_ENTRY(GPS_RGU_ON_GPS_L1_DSPYRAM_SLP_EN_RGU_GPS_L1_YRAM_HWCTL_SLP, 0x1FF);
		} else if (ctrl == GPS_L1_DSP_ENTER_DSTOP) {
			GDL_HW_SET_GPS_ENTRY(GPS_AON_TOP_DSLEEP_CTL_GPS_PWR_STAT, 1);
			GDL_HW_SET_GPS_ENTRY(GPS_AON_TOP_DSLEEP_CTL_FORCE_OSC_EN_ON, 0);

			GDL_HW_SET_GPS_ENTRY(GPS_RGU_ON_GPS_L1_DSPPRAM_PDN_EN_RGU_GPS_L1_PRAM_HWCTL_PDN, 0);
			GDL_HW_SET_GPS_ENTRY(GPS_RGU_ON_GPS_L1_DSPPRAM_SLP_EN_RGU_GPS_L1_PRAM_HWCTL_SLP, 0x1FF);
			GDL_HW_SET_GPS_ENTRY(GPS_RGU_ON_GPS_L1_DSPXRAM_PDN_EN_RGU_GPS_L1_XRAM_HWCTL_PDN, 0);
			GDL_HW_SET_GPS_ENTRY(GPS_RGU_ON_GPS_L1_DSPXRAM_SLP_EN_RGU_GPS_L1_XRAM_HWCTL_SLP, 0xF);
			GDL_HW_SET_GPS_ENTRY(GPS_RGU_ON_GPS_L1_DSPYRAM_PDN_EN_RGU_GPS_L1_YRAM_HWCTL_PDN, 0x1FF);
			GDL_HW_SET_GPS_ENTRY(GPS_RGU_ON_GPS_L1_DSPYRAM_SLP_EN_RGU_GPS_L1_YRAM_HWCTL_SLP, 0);
		} else {
			GDL_HW_SET_GPS_ENTRY(GPS_AON_TOP_DSLEEP_CTL_GPS_PWR_STAT, 0);
			GDL_HW_SET_GPS_ENTRY(GPS_AON_TOP_DSLEEP_CTL_FORCE_OSC_EN_ON, 0);

			GDL_HW_SET_GPS_ENTRY(GPS_RGU_ON_GPS_L1_DSPPRAM_PDN_EN_RGU_GPS_L1_PRAM_HWCTL_PDN, 0x1FF);
			GDL_HW_SET_GPS_ENTRY(GPS_RGU_ON_GPS_L1_DSPPRAM_SLP_EN_RGU_GPS_L1_PRAM_HWCTL_SLP, 0);
			GDL_HW_SET_GPS_ENTRY(GPS_RGU_ON_GPS_L1_DSPXRAM_PDN_EN_RGU_GPS_L1_XRAM_HWCTL_PDN, 0xF);
			GDL_HW_SET_GPS_ENTRY(GPS_RGU_ON_GPS_L1_DSPXRAM_SLP_EN_RGU_GPS_L1_XRAM_HWCTL_SLP, 0);
			GDL_HW_SET_GPS_ENTRY(GPS_RGU_ON_GPS_L1_DSPYRAM_PDN_EN_RGU_GPS_L1_YRAM_HWCTL_PDN, 0x1FF);
			GDL_HW_SET_GPS_ENTRY(GPS_RGU_ON_GPS_L1_DSPYRAM_SLP_EN_RGU_GPS_L1_YRAM_HWCTL_SLP, 0);
		}

		/* poll */
		dsp_off_done = gps_dl_hw_gps_dsp_is_off_done(GPS_DATA_LINK_ID0);

		GDL_HW_SET_GPS_ENTRY(GPS_RGU_ON_GPS_L1_CR_RGU_GPS_L1_SOFT_RST_B, 0);
		GDL_HW_SET_GPS_ENTRY(GPS_RGU_ON_GPS_L1_CR_RGU_GPS_L1_ON, 0);

		if (dsp_off_done)
			return 0;
		else
			return -1;

	case GPS_L5_DSP_ON:
	case GPS_L5_DSP_EXIT_DSTOP:
	case GPS_L5_DSP_EXIT_DSLEEP:
		GDL_HW_SET_GPS_ENTRY(GPS_RGU_ON_GPS_L5_CR_RGU_GPS_L5_ON, 1);
		GDL_HW_SET_GPS_ENTRY(GPS_RGU_ON_GPS_L5_CR_RGU_GPS_L5_SOFT_RST_B, 1);
		GDL_HW_POLL_GPS_ENTRY(GPS_CFG_ON_GPS_L5_SLP_PWR_CTL_GPS_L5_SLP_PWR_CTL_CS, 3,
			POLL_DEFAULT, &poll_okay);
		if (!poll_okay) {
			GDL_LOGE("ctrl = %d fail", ctrl);
			return -1;
		}

		GDL_HW_SET_GPS_ENTRY(GPS_RGU_ON_GPS_L5_MEM_DLY_CTL_RGU_GPSSYS_L5_MEM_ADJ_DLY_EN, 1);
		GDL_HW_SET_GPS_ENTRY(GPS_RGU_ON_GPS_L5_DLY_CHAIN_CTL_RGU_GPS_L5_MEM_PDN_DELAY_DUMMY_NUM, 9);
		gps_dl_wait_us(1000); /* 3 x 32k clk ~= 1ms */
		break;

	case GPS_L5_DSP_OFF:
	case GPS_L5_DSP_ENTER_DSTOP:
	case GPS_L5_DSP_ENTER_DSLEEP:
		if (ctrl == GPS_L5_DSP_ENTER_DSLEEP) {
			GDL_HW_SET_GPS_ENTRY(GPS_AON_TOP_DSLEEP_CTL_GPS_PWR_STAT, 3);
			GDL_HW_SET_GPS_ENTRY(GPS_AON_TOP_DSLEEP_CTL_FORCE_OSC_EN_ON, 1);

			GDL_HW_SET_GPS_ENTRY(GPS_RGU_ON_GPS_L5_DSPPRAM_PDN_EN_RGU_GPS_L5_PRAM_HWCTL_PDN, 0);
			GDL_HW_SET_GPS_ENTRY(GPS_RGU_ON_GPS_L5_DSPPRAM_SLP_EN_RGU_GPS_L5_PRAM_HWCTL_SLP, 0x3FF);
			GDL_HW_SET_GPS_ENTRY(GPS_RGU_ON_GPS_L5_DSPXRAM_PDN_EN_RGU_GPS_L5_XRAM_HWCTL_PDN, 0);
			GDL_HW_SET_GPS_ENTRY(GPS_RGU_ON_GPS_L5_DSPXRAM_SLP_EN_RGU_GPS_L5_XRAM_HWCTL_SLP, 0xF);
			GDL_HW_SET_GPS_ENTRY(GPS_RGU_ON_GPS_L5_DSPYRAM_PDN_EN_RGU_GPS_L5_YRAM_HWCTL_PDN, 0);
			GDL_HW_SET_GPS_ENTRY(GPS_RGU_ON_GPS_L5_DSPYRAM_SLP_EN_RGU_GPS_L5_YRAM_HWCTL_SLP, 0x3FF);
		} else if (ctrl == GPS_L5_DSP_ENTER_DSTOP) {
			GDL_HW_SET_GPS_ENTRY(GPS_AON_TOP_DSLEEP_CTL_GPS_PWR_STAT, 1);
			GDL_HW_SET_GPS_ENTRY(GPS_AON_TOP_DSLEEP_CTL_FORCE_OSC_EN_ON, 0);

			GDL_HW_SET_GPS_ENTRY(GPS_RGU_ON_GPS_L5_DSPPRAM_PDN_EN_RGU_GPS_L5_PRAM_HWCTL_PDN, 0);
			GDL_HW_SET_GPS_ENTRY(GPS_RGU_ON_GPS_L5_DSPPRAM_SLP_EN_RGU_GPS_L5_PRAM_HWCTL_SLP, 0x3FF);
			GDL_HW_SET_GPS_ENTRY(GPS_RGU_ON_GPS_L5_DSPXRAM_PDN_EN_RGU_GPS_L5_XRAM_HWCTL_PDN, 0);
			GDL_HW_SET_GPS_ENTRY(GPS_RGU_ON_GPS_L5_DSPXRAM_SLP_EN_RGU_GPS_L5_XRAM_HWCTL_SLP, 0xF);
			GDL_HW_SET_GPS_ENTRY(GPS_RGU_ON_GPS_L5_DSPYRAM_PDN_EN_RGU_GPS_L5_YRAM_HWCTL_PDN, 0x3FF);
			GDL_HW_SET_GPS_ENTRY(GPS_RGU_ON_GPS_L5_DSPYRAM_SLP_EN_RGU_GPS_L5_YRAM_HWCTL_SLP, 0);
		} else {
			GDL_HW_SET_GPS_ENTRY(GPS_AON_TOP_DSLEEP_CTL_GPS_PWR_STAT, 0);
			GDL_HW_SET_GPS_ENTRY(GPS_AON_TOP_DSLEEP_CTL_FORCE_OSC_EN_ON, 0);

			GDL_HW_SET_GPS_ENTRY(GPS_RGU_ON_GPS_L5_DSPPRAM_PDN_EN_RGU_GPS_L5_PRAM_HWCTL_PDN, 0x3FF);
			GDL_HW_SET_GPS_ENTRY(GPS_RGU_ON_GPS_L5_DSPPRAM_SLP_EN_RGU_GPS_L5_PRAM_HWCTL_SLP, 0);
			GDL_HW_SET_GPS_ENTRY(GPS_RGU_ON_GPS_L5_DSPXRAM_PDN_EN_RGU_GPS_L5_XRAM_HWCTL_PDN, 0xF);
			GDL_HW_SET_GPS_ENTRY(GPS_RGU_ON_GPS_L5_DSPXRAM_SLP_EN_RGU_GPS_L5_XRAM_HWCTL_SLP, 0);
			GDL_HW_SET_GPS_ENTRY(GPS_RGU_ON_GPS_L5_DSPYRAM_PDN_EN_RGU_GPS_L5_YRAM_HWCTL_PDN, 0x3FF);
			GDL_HW_SET_GPS_ENTRY(GPS_RGU_ON_GPS_L5_DSPYRAM_SLP_EN_RGU_GPS_L5_YRAM_HWCTL_SLP, 0);
		}

		/* poll */
		dsp_off_done = gps_dl_hw_gps_dsp_is_off_done(GPS_DATA_LINK_ID1);

		GDL_HW_SET_GPS_ENTRY(GPS_RGU_ON_GPS_L5_CR_RGU_GPS_L5_SOFT_RST_B, 0);
		GDL_HW_SET_GPS_ENTRY(GPS_RGU_ON_GPS_L5_CR_RGU_GPS_L5_ON, 0);

		if (dsp_off_done)
			return 0;
		else
			return -1;

	default:
		return -1;
	}

	return 0;
}

bool gps_dl_hw_gps_dsp_is_off_done(enum gps_dl_link_id_enum link_id)
{
	int i;
	bool done;
	bool show_log;

	if (GPS_DSP_ST_RESET_DONE == gps_dsp_state_get(link_id)) {
		GDL_LOGXD(link_id, "1st return, done = 1");
		return true;
	}

	i = 0;
	done = true;

	show_log = gps_dl_set_show_reg_rw_log(true);

	/* MCUB IRQ already mask at this time */
	gps_dl_hal_mcub_flag_handler(link_id);
	while (GPS_DSP_ST_RESET_DONE != gps_dsp_state_get(link_id)) {
		/* poll 10ms */
		if (i > 10) {
			done = false;
			break;
		}
		gps_dl_wait_us(1000);

		/* read dummy cr confirm dsp state for debug */
		if (GPS_DATA_LINK_ID0 == link_id)
			GDL_HW_RD_GPS_REG(0x80073160);
		else if (GPS_DATA_LINK_ID1 == link_id)
			GDL_HW_RD_GPS_REG(0x80073134);

		gps_dl_hal_mcub_flag_handler(link_id);
		i++;
	}
	gps_dl_set_show_reg_rw_log(show_log);

	GDL_LOGXW(link_id, "2nd return, done = %d", done);
	return done;
}

void gps_dl_hw_gps_adie_force_off(void)
{
	/* TODO */
}

