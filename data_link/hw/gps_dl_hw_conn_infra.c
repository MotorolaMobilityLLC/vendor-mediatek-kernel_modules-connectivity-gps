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
#include "gps_dl_hw_api.h"
#include "gps_dl_hw_priv_util.h"

#include "conn_infra/conn_host_csr_top.h"
#include "conn_infra/conn_infra_cfg.h"
#include "conn_infra/conn_uart_pta.h"
#include "conn_infra/conn_pta6.h"
#include "conn_infra/conn_semaphore.h"

void gps_dl_hw_set_gps_emi_remapping(unsigned int _20msb_of_36bit_phy_addr)
{
	GDL_HW_SET_CONN_INFRA_ENTRY(
		CONN_HOST_CSR_TOP_CONN2AP_REMAP_GPS_EMI_BASE_ADDR_CONN2AP_REMAP_GPS_EMI_BASE_ADDR,
		_20msb_of_36bit_phy_addr);
}

unsigned int gps_dl_hw_get_gps_emi_remapping(void)
{
	return GDL_HW_GET_CONN_INFRA_ENTRY(
		CONN_HOST_CSR_TOP_CONN2AP_REMAP_GPS_EMI_BASE_ADDR_CONN2AP_REMAP_GPS_EMI_BASE_ADDR);
}

void gps_dl_hw_print_hw_status(enum gps_dl_link_id_enum link_id)
{
	struct gps_dl_hw_dma_status_struct a2d_dma_status, d2a_dma_status;
	struct gps_dl_hw_usrt_status_struct usrt_status;
	enum gps_dl_hal_dma_ch_index a2d_dma_ch, d2a_dma_ch;
	unsigned int value;

	if (gps_dl_show_reg_wait_log())
		GDL_LOGXD(link_id, "");

	if (link_id == GPS_DATA_LINK_ID0) {
		a2d_dma_ch = GPS_DL_DMA_LINK0_A2D;
		d2a_dma_ch = GPS_DL_DMA_LINK0_D2A;
	} else if (link_id == GPS_DATA_LINK_ID1) {
		a2d_dma_ch = GPS_DL_DMA_LINK1_A2D;
		d2a_dma_ch = GPS_DL_DMA_LINK1_D2A;
	} else
		return;

	gps_dl_hw_save_usrt_status_struct(link_id, &usrt_status);
	gps_dl_hw_print_usrt_status_struct(link_id, &usrt_status);

	gps_dl_hw_save_dma_status_struct(a2d_dma_ch, &a2d_dma_status);
	gps_dl_hw_print_dma_status_struct(a2d_dma_ch, &a2d_dma_status);

	gps_dl_hw_save_dma_status_struct(d2a_dma_ch, &d2a_dma_status);
	gps_dl_hw_print_dma_status_struct(d2a_dma_ch, &d2a_dma_status);

	value = GDL_HW_RD_GPS_REG(0x80073160); /* DL0 */
	value = GDL_HW_RD_GPS_REG(0x80073134); /* DL1 */
	value = GDL_HW_GET_CONN_INFRA_ENTRY(CONN_HOST_CSR_TOP_HOST2GPS_DEGUG_SEL_HOST2GPS_DEGUG_SEL);
	value = GDL_HW_GET_CONN_INFRA_ENTRY(CONN_HOST_CSR_TOP_GPS_CFG2HOST_DEBUG_GPS_CFG2HOST_DEBUG);
}


/* CONN_INFRA_CFG_CKGEN_BUS_ADDR[5:2] */
#define CONN_INFRA_CFG_PTA_CLK_ADDR CONN_INFRA_CFG_CKGEN_BUS_ADDR
#define CONN_INFRA_CFG_PTA_CLK_MASK (\
	CONN_INFRA_CFG_CKGEN_BUS_CONN_CO_EXT_UART_PTA_OSC_CKEN_MASK   | \
	CONN_INFRA_CFG_CKGEN_BUS_CONN_CO_EXT_UART_PTA_HCLK_CKEN_MASK  | \
	CONN_INFRA_CFG_CKGEN_BUS_CONN_CO_EXT_UART_PTA5_OSC_CKEN_MASK  | \
	CONN_INFRA_CFG_CKGEN_BUS_CONN_CO_EXT_UART_PTA5_HCLK_CKEN_MASK)
#define CONN_INFRA_CFG_PTA_CLK_SHFT \
	CONN_INFRA_CFG_CKGEN_BUS_CONN_CO_EXT_UART_PTA5_HCLK_CKEN_SHFT

bool gps_dl_hw_is_pta_clk_cfg_ready(void)
{
	unsigned int pta_clk_cfg;
	bool okay = true;

	pta_clk_cfg = GDL_HW_GET_CONN_INFRA_ENTRY(CONN_INFRA_CFG_PTA_CLK);
	if (pta_clk_cfg != 0xF) {
		/* clock cfg is default ready, no need to set it
		 * if not as excepted, skip pta and pta_uart init
		 */
		okay = false;
	}

	GDL_LOGI("pta_clk_cfg = 0x%x", pta_clk_cfg);
	return okay;
}


bool gps_dl_hw_is_pta_uart_init_done(void)
{
	bool done;
	unsigned int pta_uart_en;

	pta_uart_en = GDL_HW_GET_CONN_INFRA_ENTRY(CONN_PTA6_RO_PTA_CTRL_ro_uart_apb_hw_en);
	done = (pta_uart_en == 1);
	GDL_LOGI("done = %d, pta_uart_en = %d", done, pta_uart_en);

	return done;
}

bool gps_dl_hw_init_pta_uart(void)
{
	unsigned int pta_uart_en;

	/* Set pta uart to MCU mode before init it.
	 * Note: both wfset and btset = 0, then pta_uart_en should become 0,
	 * set one of them = 1, then pta_uart_en should become 1.
	 */
	GDL_HW_SET_CONN_INFRA_ENTRY(CONN_PTA6_WFSET_PTA_CTRL_r_wfset_uart_apb_hw_en, 0);
	GDL_HW_SET_CONN_INFRA_ENTRY(CONN_PTA6_BTSET_PTA_CTRL_r_btset_uart_apb_hw_en, 0);
	pta_uart_en = GDL_HW_GET_CONN_INFRA_ENTRY(CONN_PTA6_RO_PTA_CTRL_ro_uart_apb_hw_en);

	if (pta_uart_en != 0) {
		GDL_LOGE("ro_uart_apb_hw_en not become 0, fail");
		return false;
	}

	GDL_HW_SET_CONN_INFRA_ENTRY(CONN_UART_PTA_HIGHSPEED_SPEED, 3);
	GDL_HW_SET_CONN_INFRA_ENTRY(CONN_UART_PTA_SAMPLE_COUNT_SAMPLE_COUNT, 5);
	GDL_HW_SET_CONN_INFRA_ENTRY(CONN_UART_PTA_SAMPLE_POINT_SAMPLE_POINT, 2);

	/* UART_PTA_BASE + 0x3C = 0x12, this step is no-need now
	 * GDL_HW_SET_CONN_INFRA_ENTRY(CONN_UART_PTA_GUARD_GUARD_CNT, 2);
	 * GDL_HW_SET_CONN_INFRA_ENTRY(CONN_UART_PTA_GUARD_GUARD_EN, 1);
	 */

	GDL_HW_SET_CONN_INFRA_ENTRY(CONN_UART_PTA_VFIFO_EN_RX_TIME_EN, 1); /* bit7 */
	GDL_HW_SET_CONN_INFRA_ENTRY(CONN_UART_PTA_VFIFO_EN_PTA_RX_FE_EN, 1); /* bit3 */
	GDL_HW_SET_CONN_INFRA_ENTRY(CONN_UART_PTA_VFIFO_EN_PTA_RX_MODE, 1); /* bit2 */

	GDL_HW_SET_CONN_INFRA_ENTRY(CONN_UART_PTA_FRACDIV_L_FRACDIV_L, 0x55);
	GDL_HW_SET_CONN_INFRA_ENTRY(CONN_UART_PTA_FRACDIV_M_FRACDIV_M, 2);

	/* UART_PTA_BASE + 0x3C[4] = 0, this step is no-need now
	 * GDL_HW_SET_CONN_INFRA_ENTRY(CONN_UART_PTA_GUARD_GUARD_EN, 0);
	 */

	GDL_HW_WR_CONN_INFRA_REG(CONN_UART_PTA_LCR_ADDR, 0xBF);
	GDL_HW_WR_CONN_INFRA_REG(CONN_UART_PTA_DLL_ADDR, 1);
	GDL_HW_WR_CONN_INFRA_REG(CONN_UART_PTA_DLM_ADDR, 0);
	GDL_HW_WR_CONN_INFRA_REG(CONN_UART_PTA_LCR_ADDR, 3);
	GDL_HW_WR_CONN_INFRA_REG(CONN_UART_PTA_FCR_ADDR, 0x37);

	GDL_HW_SET_CONN_INFRA_ENTRY(CONN_PTA6_WFSET_PTA_CTRL_r_wfset_uart_apb_hw_en, 1);
	GDL_HW_SET_CONN_INFRA_ENTRY(CONN_PTA6_WFSET_PTA_CTRL_r_wfset_lte_pta_en, 1);

	GDL_HW_SET_CONN_INFRA_ENTRY(CONN_PTA6_TMR_CTRL_1_r_idc_2nd_byte_tmout, 4); /* us */

	return true;
}

void gps_dl_hw_deinit_pta_uart(void)
{
	/* Currently no need to do pta uart deinit */
}


#define PTA_1M_CNT_VALUE 26 /* mobile platform uses 26M */

bool gps_dl_hw_is_pta_init_done(void)
{
	bool done;
	unsigned int pta_en;
	unsigned int pta_arb_en;
	unsigned int pta_1m_cnt;

	pta_en = GDL_HW_GET_CONN_INFRA_ENTRY(CONN_PTA6_RO_PTA_CTRL_ro_pta_en);
	pta_arb_en = GDL_HW_GET_CONN_INFRA_ENTRY(CONN_PTA6_RO_PTA_CTRL_ro_en_pta_arb);
	pta_1m_cnt = GDL_HW_GET_CONN_INFRA_ENTRY(CONN_PTA6_PTA_CLK_CFG_r_pta_1m_cnt);

	done = ((pta_en == 1) && (pta_arb_en == 1) && (pta_1m_cnt == PTA_1M_CNT_VALUE));
	GDL_LOGI("done = %d, pta_en = %d, pta_arb_en = %d, pta_1m_cnt = 0x%x",
		done, pta_en, pta_arb_en, pta_1m_cnt);

	return done;
}

void gps_dl_hw_init_pta(void)
{
	unsigned int pta_en;
	unsigned int pta_arb_en;

	GDL_HW_SET_CONN_INFRA_ENTRY(CONN_PTA6_PTA_CLK_CFG_r_pta_1m_cnt, PTA_1M_CNT_VALUE);

	/* Note: GPS use WFSET */
	GDL_HW_SET_CONN_INFRA_ENTRY(CONN_PTA6_WFSET_PTA_CTRL_r_wfset_en_pta_arb, 1);
	GDL_HW_SET_CONN_INFRA_ENTRY(CONN_PTA6_WFSET_PTA_CTRL_r_wfset_pta_en, 1);

	/* just confirm status change properly */
	pta_en = GDL_HW_GET_CONN_INFRA_ENTRY(CONN_PTA6_RO_PTA_CTRL_ro_pta_en);
	pta_arb_en = GDL_HW_GET_CONN_INFRA_ENTRY(CONN_PTA6_RO_PTA_CTRL_ro_en_pta_arb);

	if (!((pta_en == 1) && (pta_arb_en == 1))) {
		/* should not happen, do nothing but just show log */
		GDL_LOGE("pta_en = %d, pta_arb_en = %d, fail", pta_en, pta_arb_en);
	} else
		GDL_LOGI("pta_en = %d, pta_arb_en = %d, okay", pta_en, pta_arb_en);
}

void gps_dl_hw_deinit_pta(void)
{
	/* Currently no need to do pta deinit */
}


void gps_dl_hw_claim_pta_used_by_gps(void)
{
	/* Currently it's empty */
}

void gps_dl_hw_disclaim_pta_used_by_gps(void)
{
	/* Currently it's empty */
}

void gps_dl_hw_set_pta_blanking_parameter(void)
{
	/* Set timeout threashold: ms */
	GDL_HW_SET_CONN_INFRA_ENTRY(CONN_PTA6_TMR_CTRL_3_r_gps_l5_blank_tmr_thld, 3);
	GDL_HW_SET_CONN_INFRA_ENTRY(CONN_PTA6_TMR_CTRL_3_r_gps_l1_blank_tmr_thld, 3);

	/* Set blanking source: both LTE and NR */
	GDL_HW_SET_CONN_INFRA_ENTRY(CONN_PTA6_GPS_BLANK_CFG_r_idc_gps_l1_blank_src, 2);
	GDL_HW_SET_CONN_INFRA_ENTRY(CONN_PTA6_GPS_BLANK_CFG_r_idc_gps_l5_blank_src, 2);

	/* Use IDC mode */
	GDL_HW_SET_CONN_INFRA_ENTRY(CONN_PTA6_GPS_BLANK_CFG_r_gps_blank_src, 0);
}

/* TODO: confirm SEMA05 and M3
 * Use COS_SEMA_COEX_INDEX, the value is 5
 */
#define COS_SEMA_COEX_STA_ENTRY_FOR_GPS \
	CONN_SEMAPHORE_CONN_SEMA05_M3_OWN_STA_CONN_SEMA05_M3_OWN_STA

#define COS_SEMA_COEX_REL_ENTRY_FOR_GPS \
	CONN_SEMAPHORE_CONN_SEMA05_M3_OWN_REL_CONN_SEMA05_M3_OWN_REL

bool gps_dl_hw_take_conn_hw_sema(unsigned int try_timeout_ms)
{
	bool okay;
	bool show_log;

	show_log = gps_dl_set_show_reg_rw_log(true);
	/* poll until value is expected or timeout */
	GDL_HW_POLL_CONN_INFRA_ENTRY(COS_SEMA_COEX_STA_ENTRY_FOR_GPS, 1, POLL_US * 1000 * try_timeout_ms, &okay);
	gps_dl_set_show_reg_rw_log(show_log);

	GDL_LOGI("okay = %d", okay);

	return okay;
}

void gps_dl_hw_give_conn_hw_sema(void)
{
	bool show_log;

	show_log = gps_dl_set_show_reg_rw_log(true);
	GDL_HW_SET_CONN_INFRA_ENTRY(COS_SEMA_COEX_REL_ENTRY_FOR_GPS, 1);
	gps_dl_set_show_reg_rw_log(show_log);

	GDL_LOGI("");
}

