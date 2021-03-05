/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */
#ifndef __BGF_GPS_CFG_ON_REGS_H__
#define __BGF_GPS_CFG_ON_REGS_H__

#define BGF_GPS_CFG_ON_BASE                                    0x80001000

#define BGF_GPS_CFG_ON_GPS_CLKGEN_CTL_ADDR                     (BGF_GPS_CFG_ON_BASE + 0x004)
#define BGF_GPS_CFG_ON_GPS_L1_SLP_PWR_CTL_ADDR                 (BGF_GPS_CFG_ON_BASE + 0x020)
#define BGF_GPS_CFG_ON_GPS_L5_SLP_PWR_CTL_ADDR                 (BGF_GPS_CFG_ON_BASE + 0x024)
#define BGF_GPS_CFG_ON_GPS_OFF_PWR_CTL_ADDR                    (BGF_GPS_CFG_ON_BASE + 0x02c)
#define BGF_GPS_CFG_ON_GPS_CLKGEN1_CTL_ADDR                    (BGF_GPS_CFG_ON_BASE + 0x03C)

#define GPS_CFG_ON_GPS_L1_SLP_PWR_CTL_GPS_L1_SLP_PWR_CTL_CS_ADDR BGF_GPS_CFG_ON_GPS_L1_SLP_PWR_CTL_ADDR
#define GPS_CFG_ON_GPS_L1_SLP_PWR_CTL_GPS_L1_SLP_PWR_CTL_CS_MASK 0x0000000F
#define GPS_CFG_ON_GPS_L1_SLP_PWR_CTL_GPS_L1_SLP_PWR_CTL_CS_SHFT 0

#define GPS_CFG_ON_GPS_L5_SLP_PWR_CTL_GPS_L5_SLP_PWR_CTL_CS_ADDR BGF_GPS_CFG_ON_GPS_L5_SLP_PWR_CTL_ADDR
#define GPS_CFG_ON_GPS_L5_SLP_PWR_CTL_GPS_L5_SLP_PWR_CTL_CS_MASK 0x0000000F
#define GPS_CFG_ON_GPS_L5_SLP_PWR_CTL_GPS_L5_SLP_PWR_CTL_CS_SHFT 0

#define BGF_GPS_CFG_ON_GPS_TOP_OFF_PWR_CTL_GPS_TOP_OFF_PWR_CTL_CS_ADDR BGF_GPS_CFG_ON_GPS_OFF_PWR_CTL_ADDR
#define BGF_GPS_CFG_ON_GPS_TOP_OFF_PWR_CTL_GPS_TOP_OFF_PWR_CTL_CS_MASK 0x0000000F
#define BGF_GPS_CFG_ON_GPS_TOP_OFF_PWR_CTL_GPS_TOP_OFF_PWR_CTL_CS_SHFT 0

#define GPS_CFG_ON_GPS_CLKGEN1_CTL_CR_GPS_DIGCK_DIV_EN_ADDR    BGF_GPS_CFG_ON_GPS_CLKGEN1_CTL_ADDR
#define GPS_CFG_ON_GPS_CLKGEN1_CTL_CR_GPS_DIGCK_DIV_EN_MASK    0x00000010
#define GPS_CFG_ON_GPS_CLKGEN1_CTL_CR_GPS_DIGCK_DIV_EN_SHFT    4

#endif /* __BGF_GPS_CFG_ON_REGS_H__ */

