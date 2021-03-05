/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */
#ifndef _GPS_DL_HW_DEP_MACRO_H
#define _GPS_DL_HW_DEP_MACRO_H

#include "gps_dl_hw_ver.h"
#include "conn_infra/conn_infra_cfg.h"
#include "conn_infra/conn_host_csr_top.h"
#include "conn_infra/conn_infra_rgu.h"
#include "gps/bgf_gps_cfg.h"
#include "gps/gps_rgu_on.h"
#include "gps/gps_cfg_on.h"
#include "gps/gps_aon_top.h"

#if GPS_DL_HAS_PTA
#include "conn_infra/conn_uart_pta.h"
#include "conn_infra/conn_pta6.h"
#endif

#define GDL_HW_SUPPORT_LIST "SUPPORT:MT6885,MT6893"

#define GDL_HW_CHECK_CONN_INFRA_VER(p_poll_okay, p_poll_ver)             \
	GDL_HW_POLL_ENTRY_VERBOSE(GPS_DL_CONN_INFRA_BUS,                 \
		CONN_INFRA_CFG_CONN_HW_VER_RO_CONN_HW_VERSION,           \
		p_poll_okay, p_poll_ver, POLL_DEFAULT, (                 \
			(*p_poll_ver == GDL_HW_CONN_INFRA_VER_MT6885) || \
			(*p_poll_ver == GDL_HW_CONN_INFRA_VER_MT6893))   \
	)

#define GDL_HW_SET_EMI_REMAP_FIELD \
	CONN_HOST_CSR_TOP_CONN2AP_REMAP_GPS_EMI_BASE_ADDR_CONN2AP_REMAP_GPS_EMI_BASE_ADDR


#define GDL_HW_SET_CONN2GPS_SLP_PROT_RX_VAL(val) \
	GDL_HW_SET_CONN_INFRA_ENTRY( \
		CONN_INFRA_CFG_GALS_CONN2GPS_SLP_CTRL_R_CONN2GPS_SLP_PROT_RX_EN, val)

#define GDL_HW_POLL_CONN2GPS_SLP_PROT_RX_UNTIL_VAL(val, timeout, p_is_okay) \
	GDL_HW_POLL_CONN_INFRA_ENTRY( \
		CONN_INFRA_CFG_GALS_CONN2GPS_SLP_CTRL_R_CONN2GPS_SLP_PROT_RX_RDY, \
		val, timeout, p_is_okay)


#define GDL_HW_SET_CONN2GPS_SLP_PROT_TX_VAL(val) \
	GDL_HW_SET_CONN_INFRA_ENTRY( \
		CONN_INFRA_CFG_GALS_CONN2GPS_SLP_CTRL_R_CONN2GPS_SLP_PROT_TX_EN, val)

#define GDL_HW_POLL_CONN2GPS_SLP_PROT_TX_UNTIL_VAL(val, timeout, p_is_okay) \
	GDL_HW_POLL_CONN_INFRA_ENTRY( \
		CONN_INFRA_CFG_GALS_CONN2GPS_SLP_CTRL_R_CONN2GPS_SLP_PROT_TX_RDY, \
		val, timeout, p_is_okay)


#define GDL_HW_SET_GPS2CONN_SLP_PROT_RX_VAL(val) \
	GDL_HW_SET_CONN_INFRA_ENTRY( \
		CONN_INFRA_CFG_GALS_GPS2CONN_SLP_CTRL_R_GPS2CONN_SLP_PROT_RX_EN, val)

#define GDL_HW_POLL_GPS2CONN_SLP_PROT_RX_UNTIL_VAL(val, timeout, p_is_okay) \
	GDL_HW_POLL_CONN_INFRA_ENTRY( \
		CONN_INFRA_CFG_GALS_GPS2CONN_SLP_CTRL_R_GPS2CONN_SLP_PROT_RX_RDY, \
		val, timeout, p_is_okay)


#define GDL_HW_SET_GPS2CONN_SLP_PROT_TX_VAL(val) \
	GDL_HW_SET_CONN_INFRA_ENTRY( \
		CONN_INFRA_CFG_GALS_GPS2CONN_SLP_CTRL_R_GPS2CONN_SLP_PROT_TX_EN, val)

#define GDL_HW_POLL_GPS2CONN_SLP_PROT_TX_UNTIL_VAL(val, timeout, p_is_okay) \
	GDL_HW_POLL_CONN_INFRA_ENTRY( \
		CONN_INFRA_CFG_GALS_GPS2CONN_SLP_CTRL_R_GPS2CONN_SLP_PROT_TX_RDY, \
		val, timeout, p_is_okay)


/* For Connac2.0, wait until sleep prot disable done, or after polling 10 x 1ms */
#define GDL_HW_MAY_WAIT_CONN_INFRA_SLP_PROT_DISABLE_ACK(p_poll_okay) \
	GDL_HW_POLL_CONN_INFRA_ENTRY( \
		CONN_HOST_CSR_TOP_CONN_SLP_PROT_CTRL_CONN_INFRA_ON2OFF_SLP_PROT_ACK, \
		0, 10 * 1000 * POLL_US, p_poll_okay)


/* The dump address list for Connac2.0 */
#define GDL_HW_DUMP_SLP_RPOT_STATUS() do {\
	gps_dl_bus_rd_opt(GPS_DL_CONN_INFRA_BUS, \
		CONN_INFRA_CFG_GALS_CONN2GPS_SLP_CTRL_ADDR, \
		BMASK_RW_FORCE_PRINT); \
	gps_dl_bus_rd_opt(GPS_DL_CONN_INFRA_BUS, \
		CONN_INFRA_CFG_GALS_GPS2CONN_SLP_CTRL_ADDR, \
		BMASK_RW_FORCE_PRINT); \
} while (0)


#if (GPS_DL_HAS_CONNINFRA_DRV)
/*
 * For MT6885 and MT6893, conninfra driver do it due to GPS/BT share same bit,
 * so do nothing here if conninfra driver ready.
 */
#define GDL_HW_SET_CONN_INFRA_BGF_EN_MT6885_MT6893(val)
#else
#define GDL_HW_SET_CONN_INFRA_BGF_EN_MT6885_MT6893(val) \
	GDL_HW_SET_CONN_INFRA_ENTRY(CONN_INFRA_RGU_BGFYS_ON_TOP_PWR_CTL_BGFSYS_ON_TOP_PWR_ON, val)
#endif
#define GDL_HW_SET_CONN_INFRA_BGF_EN(val) GDL_HW_SET_CONN_INFRA_BGF_EN_MT6885_MT6893(val)

#define GDL_HW_SET_GPS_FUNC_EN(val) \
	GDL_HW_SET_CONN_INFRA_ENTRY(CONN_INFRA_CFG_GPS_PWRCTRL0_GP_FUNCTION_EN, val)


/* For MT6885 and MT6893, top clk is controlled by conninfra driver */
#if (GPS_DL_HAS_CONNINFRA_DRV)
#define GDL_HW_ADIE_TOP_CLK_EN_MT6885_MT6893(val) do { \
	if (val) { \
		conninfra_adie_top_ck_en_on(CONNSYS_ADIE_CTL_HOST_GPS); \
	} else { \
		conninfra_adie_top_ck_en_off(CONNSYS_ADIE_CTL_HOST_GPS); \
	} \
} while (0)
#else
#define GDL_HW_ADIE_TOP_CLK_EN_MT6885_MT6893(val)
#endif
#define GDL_HW_ADIE_TOP_CLK_EN(val, p_poll_okay) do { \
	GDL_HW_ADIE_TOP_CLK_EN_MT6885_MT6893(val); \
	*p_poll_okay = true; \
} while (0)

#define GDL_HW_RD_SPI_GPS_STATUS() do { \
	GDL_HW_RD_CONN_INFRA_REG(CONN_RF_SPI_MST_ADDR_SPI_STA_ADDR); \
	GDL_HW_RD_CONN_INFRA_REG(CONN_RF_SPI_MST_ADDR_SPI_GPS_GPS_ADDR_ADDR); \
	GDL_HW_RD_CONN_INFRA_REG(CONN_RF_SPI_MST_ADDR_SPI_GPS_GPS_WDAT_ADDR); \
	GDL_HW_RD_CONN_INFRA_REG(CONN_RF_SPI_MST_ADDR_SPI_GPS_GPS_RDAT_ADDR); \
	GDL_HW_RD_CONN_INFRA_REG(CONN_RF_SPI_MST_ADDR_SPI_STA_ADDR); \
} while (0)

/* For CONNAC2_0:
 * 8: HW TICK H/L, BG tick H/L, TX_END/TX_RD, RX_END/RX_WR
 * 3: PC, GALMAN CNT, WRHOST CNT
 */
#define GPS_DSP_REG_POLL_MAX (11)
#define GPS_L1_REG_POLL_LIST { \
	0x5028, 0x5029, 0x0100, 0x0101, 0x4882, 0x4883, 0x4886, 0x4887, \
	0xEF00, 0xEF01, 0xEF02, }

#define GPS_L5_REG_POLL_LIST { \
	0x5014, 0x5015, 0x0100, 0x0101, 0x4882, 0x4883, 0x4886, 0x4887, \
	0xEF00, 0xEF01, 0xEF02, }


/* For CONNAC2_0:
 * 9: PC, GALMAN CNT, WRHOST CNT, DBTT CNT, NEXT CNT, BG TICK H/L, HW TICK H/L
 * 11: USRT CTL, STA, TX_END/RD/MAX, RX_MAX/END/WR, TX_CNT, RX_CNT, MISC
 */
#define GPS_DSP_REG_DBG_POLL_MAX (20)
#define GPS_L1_REG_DBG_POLL_LIST  { \
	0xEF00, 0xEF01, 0xEF02, 0xEF03, 0xEF04, 0x0100, 0x0101, 0x5028, 0x5029, \
	0x4880, 0x4881, 0x4882, 0x4883, 0x4884, 0x4885, 0x4886, 0x4887, 0x4888, 0x4889, 0x488a, }

#define GPS_L5_REG_DBG_POLL_LIST { \
	0xEF00, 0xEF01, 0xEF02, 0xEF03, 0xEF04, 0x0100, 0x0101, 0x5014, 0x5015, \
	0x4880, 0x4881, 0x4882, 0x4883, 0x4884, 0x4885, 0x4886, 0x4887, 0x4888, 0x4889, 0x488a, }


#endif /* _GPS_DL_HW_DEP_MACRO_H */

