/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */
#ifndef _GPS_DL_HW_DEP_API_H
#define _GPS_DL_HW_DEP_API_H

#include "conn_infra/conn_infra_cfg.h"

#define GDL_HW_CONN_INFRA_VER (0x20010000)
#define GDL_HW_BGF_VER        (0x20010000)


#define GDL_HW_SET_CONN2GPS_SLP_PROT_RX_VAL(val) \
	GDL_HW_SET_CONN_INFRA_ENTRY(CONN_INFRA_CFG_GALS_CONN2GPS_SLP_CTRL_R_CONN2GPS_SLP_PROT_RX_EN, val)

#define GDL_HW_POLL_CONN2GPS_SLP_PROT_RX_UNTIL_VAL(val, timeout, p_is_okay) \
	GDL_HW_POLL_CONN_INFRA_ENTRY( \
		CONN_INFRA_CFG_GALS_CONN2GPS_SLP_CTRL_R_CONN2GPS_SLP_PROT_RX_RDY, \
		val, timeout, p_is_okay)


#define GDL_HW_SET_CONN2GPS_SLP_PROT_TX_VAL(val) \
	GDL_HW_SET_CONN_INFRA_ENTRY(CONN_INFRA_CFG_GALS_CONN2GPS_SLP_CTRL_R_CONN2GPS_SLP_PROT_TX_EN, val)

#define GDL_HW_POLL_CONN2GPS_SLP_PROT_TX_UNTIL_VAL(val, timeout, p_is_okay) \
	GDL_HW_POLL_CONN_INFRA_ENTRY( \
		CONN_INFRA_CFG_GALS_CONN2GPS_SLP_CTRL_R_CONN2GPS_SLP_PROT_TX_RDY, \
		val, timeout, p_is_okay)


#define GDL_HW_SET_GPS2CONN_SLP_PROT_RX_VAL(val) \
	GDL_HW_SET_CONN_INFRA_ENTRY(CONN_INFRA_CFG_GALS_GPS2CONN_SLP_CTRL_R_GPS2CONN_SLP_PROT_RX_EN, val)

#define GDL_HW_POLL_GPS2CONN_SLP_PROT_RX_UNTIL_VAL(val, timeout, p_is_okay) \
	GDL_HW_POLL_CONN_INFRA_ENTRY( \
		CONN_INFRA_CFG_GALS_GPS2CONN_SLP_CTRL_R_GPS2CONN_SLP_PROT_RX_RDY, \
		val, timeout, p_is_okay)


#define GDL_HW_SET_GPS2CONN_SLP_PROT_TX_VAL(val) \
	GDL_HW_SET_CONN_INFRA_ENTRY(CONN_INFRA_CFG_GALS_GPS2CONN_SLP_CTRL_R_GPS2CONN_SLP_PROT_TX_EN, val)

#define GDL_HW_POLL_GPS2CONN_SLP_PROT_TX_UNTIL_VAL(val, timeout, p_is_okay) \
	GDL_HW_POLL_CONN_INFRA_ENTRY( \
		CONN_INFRA_CFG_GALS_GPS2CONN_SLP_CTRL_R_GPS2CONN_SLP_PROT_TX_RDY, \
		val, timeout, p_is_okay)


#define GDL_HW_SET_CONN_INFRA_BGF_EN(val) \
	GDL_HW_SET_CONN_INFRA_ENTRY(CONN_INFRA_RGU_BGFYS_ON_TOP_PWR_CTL_BGFSYS_ON_TOP_PWR_ON, val)

#define GDL_HW_SET_GPS_FUNC_EN(val) \
	GDL_HW_SET_CONN_INFRA_ENTRY(CONN_INFRA_CFG_GPS_PWRCTRL0_GP_FUNCTION_EN, val)

#endif /* _GPS_DL_HW_DEP_API_H */

