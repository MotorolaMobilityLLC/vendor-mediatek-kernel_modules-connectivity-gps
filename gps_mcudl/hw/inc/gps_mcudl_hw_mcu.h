/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef _GPS_MCUDL_HW_MCU_H
#define _GPS_MCUDL_HW_MCU_H

#include "gps_mcudl_hw_type.h"

bool gps_mcudl_hw_conn_force_wake(bool enable);

bool gps_mcudl_hw_mcu_do_on_with_rst_held(void);
void gps_mcudl_hw_mcu_speed_up_clock(void);
void gps_mcudl_hw_mcu_release_rst(void);
bool gps_mcudl_hw_mcu_wait_idle_loop_or_timeout_us(unsigned int timeout_us);
void gps_mcudl_hw_mcu_show_status(void);

bool gps_mcudl_hw_mcu_set_or_clr_fw_own(bool to_set);
void gps_mcudl_hw_mcu_do_off(void);

/* tmp*/
#if 1
void gps_dl_hw_set_mcu_emi_remapping_tmp(unsigned int _20msb_of_36bit_phy_addr);
unsigned int gps_dl_hw_get_mcu_emi_remapping_tmp(void);
void gps_dl_hw_set_gps_dyn_remapping_tmp(unsigned int val);
#endif
/*tmp*/

#endif /* _GPS_MCUDL_HW_MCU_H */

