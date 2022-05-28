/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019-2021 MediaTek Inc.
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
#include "gps_dl_hw_dep_macro.h"
#include "gps_dl_hw_priv_util.h"
#include "gps_dl_hal_util.h"
#include "gps_dsp_fsm.h"
#include "gps_dl_name_list.h"
#if GPS_DL_ON_LINUX
#include "gps_dl_subsys_reset.h"
#endif

#include "conn_infra/conn_infra_cfg.h"
#include "conn_infra/conn_host_csr_top.h"

#include "gps/bgf_gps_cfg.h"
#include "gps/gps_aon_top.h"

/*******************************************************************************
*                                 M A C R O S
********************************************************************************
*/
#include "gps_dl_hw_atf.h"

#if 0
static int gps_dl_hw_gps_sleep_prot_ctrl(int op)
{
	bool poll_okay = false;

	if (1 == op) {
		/* disable when on */
		GDL_HW_SET_CONN2GPS_SLP_PROT_RX_VAL(0);
		GDL_HW_POLL_CONN2GPS_SLP_PROT_RX_UNTIL_VAL(0, POLL_DEFAULT, &poll_okay);
		if (!poll_okay) {
			GDL_LOGE("_fail_disable_gps_slp_prot - conn2gps rx");
			goto _fail_disable_gps_slp_prot;
		}

		GDL_HW_SET_CONN2GPS_SLP_PROT_TX_VAL(0);
		GDL_HW_POLL_CONN2GPS_SLP_PROT_TX_UNTIL_VAL(0, POLL_DEFAULT, &poll_okay);
		if (!poll_okay) {
			GDL_LOGE("_fail_disable_gps_slp_prot - conn2gps tx");
			goto _fail_disable_gps_slp_prot;
		}

		GDL_HW_SET_GPS2CONN_SLP_PROT_RX_VAL(0);
		GDL_HW_POLL_GPS2CONN_SLP_PROT_RX_UNTIL_VAL(0, POLL_DEFAULT, &poll_okay);
		if (!poll_okay) {
			GDL_LOGE("_fail_disable_gps_slp_prot - gps2conn rx");
			goto _fail_disable_gps_slp_prot;
		}

		GDL_HW_SET_GPS2CONN_SLP_PROT_TX_VAL(0);
		GDL_HW_POLL_GPS2CONN_SLP_PROT_TX_UNTIL_VAL(0, POLL_DEFAULT, &poll_okay);
		if (!poll_okay) {
			GDL_LOGE("_fail_disable_gps_slp_prot - gps2conn tx");
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
		GDL_HW_SET_CONN2GPS_SLP_PROT_TX_VAL(1);
		GDL_HW_POLL_CONN2GPS_SLP_PROT_TX_UNTIL_VAL(1, POLL_DEFAULT, &poll_okay);
		if (!poll_okay) {
			/* From DE: need to trigger connsys reset */
			GDL_LOGE("_fail_enable_gps_slp_prot - conn2gps tx");
			goto _fail_enable_gps_slp_prot;
		}

		GDL_HW_SET_CONN2GPS_SLP_PROT_RX_VAL(1);
		GDL_HW_POLL_CONN2GPS_SLP_PROT_RX_UNTIL_VAL(1, POLL_DEFAULT, &poll_okay);
		if (!poll_okay) {
			/* not handle it, just show warning */
			GDL_LOGE("_fail_enable_gps_slp_prot - conn2gps rx");
		}

		GDL_HW_SET_GPS2CONN_SLP_PROT_TX_VAL(1);
		GDL_HW_POLL_GPS2CONN_SLP_PROT_TX_UNTIL_VAL(1, POLL_DEFAULT, &poll_okay);
		if (!poll_okay) {
			/* not handle it, just show warning */
			GDL_LOGE("_fail_enable_gps_slp_prot - gps2conn tx");
		}

		GDL_HW_SET_GPS2CONN_SLP_PROT_RX_VAL(1);
		GDL_HW_POLL_GPS2CONN_SLP_PROT_RX_UNTIL_VAL(1, POLL_DEFAULT, &poll_okay);
		if (!poll_okay) {
			/* From DE: need to trigger connsys reset */
			GDL_LOGE("_fail_enable_gps_slp_prot - gps2conn rx");
			goto _fail_enable_gps_slp_prot;
		}

		return 0;

_fail_enable_gps_slp_prot:
		/* trigger reset on outer function */
#if 0
		gps_dl_trigger_connsys_reset();
#endif
		return -1;
	}

	return 0;
}
#endif

bool gps_dl_hw_gps_force_wakeup_conninfra_top_off(bool enable)
{
	struct arm_smccc_res res;
	int ret;

	arm_smccc_smc(MTK_SIP_KERNEL_GPS_CONTROL, SMC_GPS_WAKEUP_CONNINFRA_TOP_OFF_OPID,
			enable, 0, 0, 0, 0, 0, &res);
	ret = res.a0;
	return ret;
}

void gps_dl_hw_gps_sw_request_peri_usage(bool request)
{
	gps_dl_hw_dep_gps_sw_request_peri_usage(request);
}

void gps_dl_hw_gps_sw_request_emi_usage(bool request)
{
	bool reg_rw_log = false;
	struct arm_smccc_res res;
	int ret;

#if GPS_DL_ON_LINUX
	reg_rw_log = gps_dl_log_reg_rw_is_on(GPS_DL_REG_RW_EMI_SW_REQ_CTRL);
#endif

	arm_smccc_smc(MTK_SIP_KERNEL_GPS_CONTROL, SMC_GPS_SW_REQUEST_EMI_USAGE_OPID,
			request, reg_rw_log, 0, 0, 0, 0, &res);
	ret = res.a0;
}

int gps_dl_hw_gps_common_on_part1(unsigned int *poll_ver)
{
	struct arm_smccc_res res;
	int ret;

	if (poll_ver != NULL) {
		arm_smccc_smc(MTK_SIP_KERNEL_GPS_CONTROL, SMC_GPS_COMMON_ON_PART1_OPID,
				0, 0, 0, 0, 0, 0, &res);
		ret = res.a0;
		*poll_ver = res.a1;
		return ret;
	}
	return -1;
}

int gps_dl_hw_gps_common_on_part3(int i)
{
	struct arm_smccc_res res;
	int ret;

	arm_smccc_smc(MTK_SIP_KERNEL_GPS_CONTROL, SMC_GPS_COMMON_ON_PART3_OPID,
			i, 0, 0, 0, 0, 0, &res);
	ret = res.a0;
	return ret;
}

int gps_dl_hw_gps_common_on_part4(void)
{
	struct arm_smccc_res res;
	int ret;

	arm_smccc_smc(MTK_SIP_KERNEL_GPS_CONTROL, SMC_GPS_COMMON_ON_PART4_OPID,
			0, 0, 0, 0, 0, 0, &res);
	ret = res.a0;
	return ret;
}

int gps_dl_hw_gps_common_on_part5(void)
{
	struct arm_smccc_res res;
	int ret;

	arm_smccc_smc(MTK_SIP_KERNEL_GPS_CONTROL, SMC_GPS_COMMON_ON_PART5_OPID,
			0, 0, 0, 0, 0, 0, &res);
	ret = res.a0;
	return ret;
}

int gps_dl_hw_gps_common_on_fail_handle(void)
{
	struct arm_smccc_res res;
	int ret;

	arm_smccc_smc(MTK_SIP_KERNEL_GPS_CONTROL, SMC_GPS_COMMON_ON_FAIL_HANDLER_OPID,
			0, 0, 0, 0, 0, 0, &res);
	ret = res.a0;
	return ret;
}

int gps_dl_hw_gps_common_on(void)
{
	int poll_okay = -1;
	unsigned int poll_ver, adie_ver = 0;

	/*wake up 3T 32k clock to ready*/
	GDL_WAIT_US(200);

	poll_okay = gps_dl_hw_gps_common_on_part1(&poll_ver);
	if (poll_okay == -1) {
		GDL_LOGE("_fail_gps_hw_common_on_part1_fail");
		goto _fail_gps_hw_common_on_part1_not_okay;
	}

	/*set gps emi remap here*/
	gps_dl_emi_remap_calc_and_set();

	/* Enable Conninfra BGF */
	gps_dl_hw_gps_common_on_part3(1);

	/* GDL_HW_CHECK_CONN_INFRA_VER may check a list and return ok if poll_ver is in the list,
	 * record the poll_ver here and we can know which one it is,
	 * and it may help for debug purpose.
	 */
#if GPS_DL_ON_LINUX
	gps_dl_hal_set_conn_infra_ver(poll_ver);
#endif

#if GPS_DL_HAS_CONNINFRA_DRV
	adie_ver = conninfra_get_ic_info(CONNSYS_ADIE_CHIPID);
	if (!(adie_ver == 0x6637 || adie_ver == 0x6635)) {
		GDL_LOGE("_fail_adie_ver_not_okay, adie_ver = 0x%08x", adie_ver);
		goto _fail_adie_ver_not_okay;
	}
	gps_dl_hal_set_adie_ver(adie_ver);
#endif

	GDL_LOGW("%s: poll_ver = 0x%08x, adie_ver = 0x%08x is ok", GDL_HW_SUPPORT_LIST, poll_ver, adie_ver);

#if GPS_DL_ON_CTP
	/* Request EMI anyway */
	gps_dl_hw_gps_sw_request_emi_usage(true);
#elif GPS_DL_ON_LINUX
	/* Will not request EMI until data routing */
	gps_dl_hal_emi_usage_init();
#endif

	poll_okay = gps_dl_hw_gps_common_on_part4();
	if (poll_okay == -1) {
		GDL_LOGE("_fail_gps_hw_common_on_part4_fail");
		goto _fail_gps_hw_common_on_part4_not_okay;
	}

#if GPS_DL_HAS_CONNINFRA_DRV
	if (0x6637 == gps_dl_hal_get_adie_ver()) {
		/*open mt6637 top clock buffer : ADIE TOP 0xB18[1] = 1*/
		if (conninfra_spi_update_bits(SYS_SPI_TOP, 0xB18, 0x2, 0x2) != 0) {
			GDL_LOGE("conninfra_spi_update_bits_not_okay");
			goto _fail_open_mt6637_top_clock_buf;
		}
	}
#endif

	poll_okay = gps_dl_hw_gps_common_on_part5();
	if (poll_okay == -1) {
		GDL_LOGE("_fail_gps_hw_common_on_part5_fail");
		goto _fail_gps_hw_common_on_part5_not_okay;
	}

	return 0;
_fail_gps_hw_common_on_part5_not_okay:
#if GPS_DL_HAS_CONNINFRA_DRV
_fail_open_mt6637_top_clock_buf:
#endif
_fail_gps_hw_common_on_part4_not_okay:
	gps_dl_hw_gps_common_on_fail_handle();

#if GPS_DL_HAS_CONNINFRA_DRV
_fail_adie_ver_not_okay:
#endif
_fail_gps_hw_common_on_part1_not_okay:
	return -1;

}

int gps_dl_hw_gps_common_off_part1(void)
{
	struct arm_smccc_res res;
	int ret;

	arm_smccc_smc(MTK_SIP_KERNEL_GPS_CONTROL, SMC_GPS_COMMON_OFF_PART1_OPID,
			0, 0, 0, 0, 0, 0, &res);
	ret = res.a0;
	return ret;
}

int gps_dl_hw_gps_common_off_part2(void)
{
	struct arm_smccc_res res;
	int ret;

	arm_smccc_smc(MTK_SIP_KERNEL_GPS_CONTROL, SMC_GPS_COMMON_OFF_PART2_OPID,
			0, 0, 0, 0, 0, 0, &res);
	ret = res.a0;
	return ret;
}

int gps_dl_hw_gps_common_off_part3(void)
{
	struct arm_smccc_res res;
	int ret;

	arm_smccc_smc(MTK_SIP_KERNEL_GPS_CONTROL, SMC_GPS_COMMON_OFF_PART3_OPID,
			0, 0, 0, 0, 0, 0, &res);
	ret = res.a0;
	return ret;
}

int gps_dl_hw_gps_common_off(void)
{
	/*Disable BPLL driver*/
	gps_dl_hw_gps_common_off_part1();

#if GPS_DL_HAS_CONNINFRA_DRV
	if (0x6637 == gps_dl_hal_get_adie_ver()) {
		/*close mt6637 top clock buffer : ADIE TOP 0xB18[1] = 0*/
		if (conninfra_spi_update_bits(SYS_SPI_TOP, 0xB18, 0x0, 0x2) != 0)
			GDL_LOGE("conninfra_spi_update_bits_not_okay");
	}
#endif

	if (gps_dl_hw_gps_common_off_part2() != 0) {
		GDL_LOGE("enable sleep prot fail, trigger connsys reset");
#if GPS_DL_ON_LINUX
		gps_dl_trigger_connsys_reset();
#endif
		return -1;
	}

#if GPS_DL_ON_CTP
	/* Release EMI anyway */
	gps_dl_hw_gps_sw_request_emi_usage(false);

#elif GPS_DL_ON_LINUX
	/* Will force to release EMI */
	gps_dl_hal_emi_usage_deinit();
#endif

	/* L1 infra request, only for mt6983\6879\... */
	gps_dl_hw_gps_common_off_part3();

	if (gps_dl_log_reg_rw_is_on(GPS_DL_REG_RW_HOST_CSR_GPS_OFF))
		gps_dl_hw_dump_host_csr_conninfra_info(true);

	return 0;
}

/* L1 and L5 share same pwr stat and current we can support the bellow case:
 * 1. Both L1 and L5 on / off
 * 2. Both L1 and L5 enter deep stop mode and wakeup
 * 3. L5 stays off, L1 do on / off
 * 4. L5 stays off, L1 enter deep stop mode and wakeup
 */
unsigned int g_gps_pwr_stat;

int gps_dl_hw_gps_pwr_stat_ctrl(enum dsp_ctrl_enum ctrl)
{
	struct arm_smccc_res res;
	int ret;
	bool clk_ext = false;
	bool dpstop_revert_mvcd0 = false, dpstop_revert_mvcd1 = false;

#if GPS_DL_ON_LINUX
	clk_ext = gps_dl_hal_get_need_clk_ext_flag(GPS_DATA_LINK_ID0);
	dpstop_revert_mvcd0 = gps_dl_hal_get_deep_stop_mode_revert_for_mvcd(GPS_DATA_LINK_ID0);
	dpstop_revert_mvcd1 = gps_dl_hal_get_deep_stop_mode_revert_for_mvcd(GPS_DATA_LINK_ID1);
#endif

	arm_smccc_smc(MTK_SIP_KERNEL_GPS_CONTROL, SMC_GPS_DL_HW_GPS_PWR_STAT_CTRL_OPID,
			ctrl, clk_ext, dpstop_revert_mvcd0, dpstop_revert_mvcd1, 0, 0, &res);
	ret = res.a0;
	return ret;
}

int gps_dl_hw_set_dsp_on_and_poll_ack(enum gps_dl_link_id_enum link_id)
{
	struct arm_smccc_res res;
	int ret;

	arm_smccc_smc(MTK_SIP_KERNEL_GPS_CONTROL, SMC_GPS_DL_SET_DSP_ON_AND_POLL_ACK_OPID,
			link_id, 0, 0, 0, 0, 0, &res);
	ret = res.a0;
	return ret;
}

int gps_dl_hw_set_cfg_dsp_mem_and_dsp_off(enum dsp_ctrl_enum ctrl,
	enum gps_dl_link_id_enum link_id)
{
	struct arm_smccc_res res;
	int ret;

	arm_smccc_smc(MTK_SIP_KERNEL_GPS_CONTROL, SMC_GPS_DL_SET_CFG_DSP_MEM_AND_DSP_OFF_OPID,
			ctrl, link_id, 0, 0, 0, 0, &res);
	ret = res.a0;
	return ret;
}

int gps_dl_hw_gps_dsp_ctrl(enum dsp_ctrl_enum ctrl)
{
	bool poll_okay = false;
	bool dsp_off_done = false;

	switch (ctrl) {
	case GPS_L1_DSP_ON:
	case GPS_L1_DSP_EXIT_DSTOP:
	case GPS_L1_DSP_EXIT_DSLEEP:
		gps_dl_hw_gps_pwr_stat_ctrl(ctrl);
		poll_okay = (bool)gps_dl_hw_set_dsp_on_and_poll_ack(GPS_DATA_LINK_ID0);
		if (!poll_okay) {
			GDL_LOGE("ctrl = %d fail", ctrl);
			return -1;
		}

		gps_dl_wait_us(100); /* 3 x 32k clk ~= 100us */
		gps_dl_hw_usrt_ctrl(GPS_DATA_LINK_ID0,
			true, gps_dl_is_dma_enabled(), gps_dl_is_1byte_mode());
		break;

	case GPS_L1_DSP_CLEAR_PWR_STAT:
		gps_dl_hw_gps_pwr_stat_ctrl(ctrl);
		return 0;

	case GPS_L1_DSP_OFF:
	case GPS_L1_DSP_ENTER_DSTOP:
	case GPS_L1_DSP_ENTER_DSLEEP:
		/* poll */
		dsp_off_done = gps_dl_hw_gps_dsp_is_off_done(GPS_DATA_LINK_ID0);

		gps_dl_hw_usrt_ctrl(GPS_DATA_LINK_ID0,
			false, gps_dl_is_dma_enabled(), gps_dl_is_1byte_mode());
		gps_dl_hw_gps_pwr_stat_ctrl(ctrl);
		gps_dl_hw_set_cfg_dsp_mem_and_dsp_off(ctrl, GPS_DATA_LINK_ID0);
		if (dsp_off_done)
			return 0;
		else
			return -1;

	case GPS_L5_DSP_ON:
	case GPS_L5_DSP_EXIT_DSTOP:
	case GPS_L5_DSP_EXIT_DSLEEP:
		gps_dl_hw_gps_pwr_stat_ctrl(ctrl);
		poll_okay = gps_dl_hw_set_dsp_on_and_poll_ack(GPS_DATA_LINK_ID1);
		if (!poll_okay) {
			GDL_LOGE("ctrl = %d fail", ctrl);
			return -1;
		}

		gps_dl_wait_us(100); /* 3 x 32k clk ~= 1ms */
		gps_dl_hw_usrt_ctrl(GPS_DATA_LINK_ID1,
			true, gps_dl_is_dma_enabled(), gps_dl_is_1byte_mode());
		break;

	case GPS_L5_DSP_CLEAR_PWR_STAT:
		gps_dl_hw_gps_pwr_stat_ctrl(ctrl);
		return 0;

	case GPS_L5_DSP_OFF:
	case GPS_L5_DSP_ENTER_DSTOP:
	case GPS_L5_DSP_ENTER_DSLEEP:
		/* poll */
		dsp_off_done = gps_dl_hw_gps_dsp_is_off_done(GPS_DATA_LINK_ID1);

		gps_dl_hw_usrt_ctrl(GPS_DATA_LINK_ID1,
			false, gps_dl_is_dma_enabled(), gps_dl_is_1byte_mode());
		gps_dl_hw_gps_pwr_stat_ctrl(ctrl);
		gps_dl_hw_set_cfg_dsp_mem_and_dsp_off(ctrl, GPS_DATA_LINK_ID1);
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
	bool done = false;
	bool record_last_show_log = false;
	bool need_dump_for_reset_done = false;
	struct gps_dl_hw_usrt_status_struct usrt_status;
	struct gps_dl_hw_dma_status_struct a2d_dma_status, d2a_dma_status;
	enum gps_dl_hal_dma_ch_index a2d_dma_ch, d2a_dma_ch;

	if (link_id == GPS_DATA_LINK_ID0) {
		a2d_dma_ch = GPS_DL_DMA_LINK0_A2D;
		d2a_dma_ch = GPS_DL_DMA_LINK0_D2A;
	} else if (link_id == GPS_DATA_LINK_ID1) {
		a2d_dma_ch = GPS_DL_DMA_LINK1_A2D;
		d2a_dma_ch = GPS_DL_DMA_LINK1_D2A;
	} else
		return false;

	gps_each_dsp_reg_dump_if_any_rec(link_id);

	/* TODO: move it to proper place */
	if (GPS_DSP_ST_HW_STOP_MODE == gps_dsp_state_get(link_id)) {
		/* expect it change to RESET_DONE after this call */
		if (!gps_dl_hal_mcub_flag_handler(link_id)) {
			GDL_LOGXW(link_id, "pre-check fail");
			return false;
		}
	}

	gps_dl_hw_gps_dump_top_rf_temp_cr();
	gps_dl_hw_gps_dump_gps_rf_temp_cr();

	if (GPS_DSP_ST_RESET_DONE == gps_dsp_state_get(link_id)) {
		GDL_LOGXD(link_id, "1st return, done = 1");
		need_dump_for_reset_done = gps_dsp_state_is_dump_needed_for_reset_done(link_id);
		if (!need_dump_for_reset_done)
			return true;
	}

	i = 0;

	record_last_show_log = gps_dl_show_reg_rw_log();
	gps_dl_set_show_reg_rw_log(true);
	do {
		/* MCUB IRQ already mask at this time */
		if (!gps_dl_hal_mcub_flag_handler(link_id)) {
			done = false;
			break;
		}

		done = true;
		while (GPS_DSP_ST_RESET_DONE != gps_dsp_state_get(link_id)
				|| need_dump_for_reset_done) {
			/* poll 200ms */
			if (i >= 200) {
				done = false;
				gps_dl_hw_save_usrt_status_struct(GPS_DATA_LINK_ID0, &usrt_status);
				gps_dl_hw_print_usrt_status_struct(GPS_DATA_LINK_ID0, &usrt_status);
				gps_dl_hw_save_usrt_status_struct(GPS_DATA_LINK_ID1, &usrt_status);
				gps_dl_hw_print_usrt_status_struct(GPS_DATA_LINK_ID1, &usrt_status);
				gps_dl_hw_save_dma_status_struct(a2d_dma_ch, &a2d_dma_status);
				gps_dl_hw_print_dma_status_struct(a2d_dma_ch, &a2d_dma_status);
				gps_dl_hw_save_dma_status_struct(d2a_dma_ch, &d2a_dma_status);
				gps_dl_hw_print_dma_status_struct(d2a_dma_ch, &d2a_dma_status);
				gps_dl_hw_dep_dump_gps_pos_info(link_id);
#if GPS_DL_USE_PERI_REMAP
				gps_dl_hw_get_gps_peri_remapping();

#endif
				/* it means a2z dump is already done */
				if (gps_each_link_get_bool_flag(link_id, LINK_NEED_A2Z_DUMP))
					break;

				/* only dump for need_dump_for_reset_done = true*/
				if (need_dump_for_reset_done) {
					gps_dl_hw_get_gps_emi_remapping();
					GDL_HW_GET_CONN2GPS_SLP_PROT_RX_VAL();
					GDL_HW_GET_CONN2GPS_SLP_PROT_RX_UNTIL_VAL();
					GDL_HW_GET_CONN2GPS_SLP_PROT_TX_VAL();
					GDL_HW_GET_CONN2GPS_SLP_PROT_TX_UNTIL_VAL();
					GDL_HW_GET_GPS2CONN_SLP_PROT_RX_VAL();
					GDL_HW_GET_GPS2CONN_SLP_PROT_RX_UNTIL_VAL();
					GDL_HW_GET_GPS2CONN_SLP_PROT_TX_VAL();
					GDL_HW_GET_GPS2CONN_SLP_PROT_TX_UNTIL_VAL();
					gps_dl_conninfra_is_okay_or_handle_it(NULL, true);
				}

#if 0
				/* dump for No IOC_QUERY case */
				gps_dl_hw_do_gps_a2z_dump();
#endif
				break;
			}
			gps_dl_sleep_us(999, 1001);

			/* read dummy cr confirm dsp state for debug */
			GDL_HW_RD_GPS_REG(0x80073160);
			GDL_HW_RD_GPS_REG(0x80073134);

			if (!gps_dl_hal_mcub_flag_handler(link_id)) {
				done = false;
				break;
			}
			i++;

			if ((i % 20) == 0)
				gps_dl_set_show_reg_rw_log(true);
			else
				gps_dl_set_show_reg_rw_log(false);
		}
	} while (0);
	gps_dl_set_show_reg_rw_log(record_last_show_log);
	GDL_LOGXW(link_id, "2nd return, done = %d, i = %d, need_dump_for_reset_done = %d", done, i,
		need_dump_for_reset_done);
	return done;
}

void gps_dl_hw_gps_adie_force_off(void)
{
#if GPS_DL_HAS_CONNINFRA_DRV
	unsigned int spi_data;
	int rd_status;
	int wr_status;

	/* TOP: 0xFC[1:0] = 2'b11 */
	spi_data = 0;
	rd_status = conninfra_spi_read(SYS_SPI_TOP, 0xFC, &spi_data);
	ASSERT_ZERO(rd_status, GDL_VOIDF());
	GDL_LOGW("spi_data(0xFC) = 0x%x", spi_data);
	wr_status = conninfra_spi_write(SYS_SPI_TOP, 0xFC, spi_data | 3UL);
	ASSERT_ZERO(wr_status, GDL_VOIDF());
	spi_data = 0;
	rd_status = conninfra_spi_read(SYS_SPI_TOP, 0xFC, &spi_data);
	ASSERT_ZERO(rd_status, GDL_VOIDF());
	GDL_LOGW("spi_data(0xFC) = 0x%x", spi_data);

	/* TOP: 0xA0C[31:0] = 0xFFFFFFFF; 0xAFC[31:0] = 0xFFFFFFFF */
	spi_data = 0;
	rd_status = conninfra_spi_read(SYS_SPI_TOP, 0xA10, &spi_data);
	ASSERT_ZERO(rd_status, GDL_VOIDF());
	GDL_LOGW("spi_data(0xA10) = 0x%x (first dump)", spi_data);
	wr_status = conninfra_spi_write(SYS_SPI_TOP, 0xA0C, 0xFFFFFFFF);
	ASSERT_ZERO(wr_status, GDL_VOIDF());
	spi_data = 0;
	rd_status = conninfra_spi_read(SYS_SPI_TOP, 0xA10, &spi_data);
	ASSERT_ZERO(rd_status, GDL_VOIDF());
	GDL_LOGW("spi_data(0xA10) = 0x%x (see bit3 for 0xA0C)", spi_data);

	wr_status = conninfra_spi_write(SYS_SPI_TOP, 0xAFC, 0xFFFFFFFF);
	ASSERT_ZERO(wr_status, GDL_VOIDF());
	spi_data = 0;
	rd_status = conninfra_spi_read(SYS_SPI_TOP, 0xA10, &spi_data);
	ASSERT_ZERO(rd_status, GDL_VOIDF());
	GDL_LOGW("spi_data(0xA10) = 0x%x (see bit4 for 0xAFC)", spi_data);

	/* TOP: 0xF8[0] = 1'b0 */
	spi_data = 0;
	rd_status = conninfra_spi_read(SYS_SPI_TOP, 0xF8, &spi_data);
	ASSERT_ZERO(rd_status, GDL_VOIDF());
	GDL_LOGW("spi_data(0xF8) = 0x%x", spi_data);
	wr_status = conninfra_spi_write(SYS_SPI_TOP, 0xF8, spi_data & (~1UL));
	ASSERT_ZERO(wr_status, GDL_VOIDF());
	spi_data = 0;
	rd_status = conninfra_spi_read(SYS_SPI_TOP, 0xF8, &spi_data);
	ASSERT_ZERO(rd_status, GDL_VOIDF());
	GDL_LOGW("spi_data(0xF8) = 0x%x", spi_data);

	/* GPS: 0x0[15] = 1'b1 */
	spi_data = 0;
	rd_status = conninfra_spi_read(SYS_SPI_GPS, 0x518, &spi_data);
	ASSERT_ZERO(rd_status, GDL_VOIDF());
	GDL_LOGW("spi_data(GPS:0x518) = 0x%x", spi_data);
	spi_data = 0;
	rd_status = conninfra_spi_read(SYS_SPI_GPS, 0x500, &spi_data);
	ASSERT_ZERO(rd_status, GDL_VOIDF());
	GDL_LOGW("spi_data(GPS:0x500) = 0x%x", spi_data);
	wr_status = conninfra_spi_write(SYS_SPI_GPS, 0x500, spi_data | (1UL << 15));
	ASSERT_ZERO(wr_status, GDL_VOIDF());
	spi_data = 0;
	rd_status = conninfra_spi_read(SYS_SPI_GPS, 0x500, &spi_data);
	ASSERT_ZERO(rd_status, GDL_VOIDF());
	GDL_LOGW("spi_data(GPS:0x500) = 0x%x", spi_data);

	/* TOP: 0xF8[0] = 1'b1 */
	spi_data = 0;
	rd_status = conninfra_spi_read(SYS_SPI_TOP, 0xF8, &spi_data);
	ASSERT_ZERO(rd_status, GDL_VOIDF());
	GDL_LOGW("spi_data(0xF8) = 0x%x", spi_data);
	wr_status = conninfra_spi_write(SYS_SPI_TOP, 0xF8, spi_data | 1UL);
	ASSERT_ZERO(wr_status, GDL_VOIDF());
	spi_data = 0;
	rd_status = conninfra_spi_read(SYS_SPI_TOP, 0xF8, &spi_data);
	ASSERT_ZERO(rd_status, GDL_VOIDF());
	GDL_LOGW("spi_data(0xF8) = 0x%x", spi_data);

	/* GPS: 0x0[15] = 1'b1 */
	spi_data = 0;
	rd_status = conninfra_spi_read(SYS_SPI_GPS, 0x500, &spi_data);
	ASSERT_ZERO(rd_status, GDL_VOIDF());
	GDL_LOGW("spi_data(GPS:0x500) = 0x%x", spi_data);
	wr_status = conninfra_spi_write(SYS_SPI_GPS, 0x500, spi_data | (1UL << 15));
	ASSERT_ZERO(wr_status, GDL_VOIDF());
	spi_data = 0;
	rd_status = conninfra_spi_read(SYS_SPI_GPS, 0x500, &spi_data);
	ASSERT_ZERO(rd_status, GDL_VOIDF());
	GDL_LOGW("spi_data(GPS:0x500) = 0x%x", spi_data);

	/* TOP: 0xF8[0] = 1'b0 */
	spi_data = 0;
	rd_status = conninfra_spi_read(SYS_SPI_TOP, 0xF8, &spi_data);
	ASSERT_ZERO(rd_status, GDL_VOIDF());
	GDL_LOGW("spi_data(0xF8) = 0x%x", spi_data);
	wr_status = conninfra_spi_write(SYS_SPI_TOP, 0xF8, spi_data & (~1UL));
	ASSERT_ZERO(wr_status, GDL_VOIDF());
	spi_data = 0;
	rd_status = conninfra_spi_read(SYS_SPI_TOP, 0xF8, &spi_data);
	ASSERT_ZERO(rd_status, GDL_VOIDF());
	GDL_LOGW("spi_data(0xF8) = 0x%x", spi_data);

	/* TOP: 0xA0C[31:0] = 0; 0xAFC[31:0] = 0 */
	wr_status = conninfra_spi_write(SYS_SPI_TOP, 0xA0C, 0);
	ASSERT_ZERO(wr_status, GDL_VOIDF());
	spi_data = 0;
	rd_status = conninfra_spi_read(SYS_SPI_TOP, 0xA10, &spi_data);
	ASSERT_ZERO(rd_status, GDL_VOIDF());
	GDL_LOGW("spi_data(0xA10) = 0x%x (see bit3 for 0xA0C)", spi_data);

	wr_status = conninfra_spi_write(SYS_SPI_TOP, 0xAFC, 0);
	ASSERT_ZERO(wr_status, GDL_VOIDF());
	spi_data = 0;
	rd_status = conninfra_spi_read(SYS_SPI_TOP, 0xA10, &spi_data);
	ASSERT_ZERO(rd_status, GDL_VOIDF());
	GDL_LOGW("spi_data(0xA10) = 0x%x (see bit4 for 0xAFC)", spi_data);

	/* TOP: 0xFC[1:0] = 2'b00 */
	rd_status = conninfra_spi_read(SYS_SPI_TOP, 0xFC, &spi_data);
	ASSERT_ZERO(rd_status, GDL_VOIDF());
	GDL_LOGW("spi_data(0xFC) = 0x%x", spi_data);
	wr_status = conninfra_spi_write(SYS_SPI_TOP, 0xFC, spi_data & (~3UL));
	ASSERT_ZERO(wr_status, GDL_VOIDF());
	spi_data = 0;
	rd_status = conninfra_spi_read(SYS_SPI_TOP, 0xFC, &spi_data);
	ASSERT_ZERO(rd_status, GDL_VOIDF());
	GDL_LOGW("spi_data(0xFC) = 0x%x", spi_data);
#else
	GDL_LOGE("no conninfra driver");
#endif
}

void gps_dl_hw_gps_dump_top_rf_cr(void)
{
#if GPS_DL_HAS_CONNINFRA_DRV
	unsigned int spi_data;
	int rd_status;
	int i;
	const int rd_addr_list[] = {0x03C, 0xA18, 0xA1C, 0x0C8, 0xA00, 0x0B4, 0x34C};
	int rd_addr;

	for (i = 0; i < ARRAY_SIZE(rd_addr_list); i++) {
		rd_addr = rd_addr_list[i];
		spi_data = 0;
		rd_status = conninfra_spi_read(SYS_SPI_TOP, rd_addr, &spi_data);
		GDL_LOGW("rd: addr = 0x%x, val = 0x%x, rd_status = %d",
			rd_addr, spi_data, rd_status);
	}
#else
	GDL_LOGE("no conninfra driver");
#endif
}

void gps_dl_hw_gps_dump_top_rf_temp_cr(void)
{
#if GPS_DL_HAS_CONNINFRA_DRV
	unsigned int spi_data;
	int rd_status;

	spi_data = 0;
	rd_status = conninfra_spi_read(SYS_SPI_TOP, 0xA10, &spi_data);
	ASSERT_ZERO(rd_status, GDL_VOIDF());
	GDL_LOGW("spi_data(0xA10) = 0x%x (noramlly dump)", spi_data);
#else
	GDL_LOGE("no conninfra driver");
#endif
}

