/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef _GPS_MCUDL_HAL_CCIF_H
#define _GPS_MCUDL_HAL_CCIF_H

#include "gps_mcudl_hw_type.h"

enum gps_mcudl_ccif_ch {
	GPS_MCUDL_CCIF_CH1 = 0,
	GPS_MCUDL_CCIF_CH2 = 1,
	GPS_MCUDL_CCIF_CH3 = 2,
	GPS_MCUDL_CCIF_CH4 = 3,
	GPS_MCUDL_CCIF_CH5 = 4,
	GPS_MCUDL_CCIF_CH6 = 5,
	GPS_MCUDL_CCIF_CH7 = 6,
	GPS_MCUDL_CCIF_CH8 = 7,
	GPS_MCUDL_CCIF_CH_NUM
};

bool gps_mcudl_hal_ccif_tx_is_busy(enum gps_mcudl_ccif_ch ch);
void gps_mcudl_hal_ccif_tx_prepare(enum gps_mcudl_ccif_ch ch);
void gps_mcudl_hal_ccif_tx_trigger(enum gps_mcudl_ccif_ch ch);
bool gps_mcudl_hal_ccif_tx_is_ack_done(enum gps_mcudl_ccif_ch ch);

void gps_mcudl_hal_ccif_show_status(void);


void gps_mcudl_hal_ccif_rx_isr(void);
void gps_mcudl_hal_ccif_rx_ch1_cb(void);
void gps_mcudl_hal_ccif_rx_ch2_to_ch8_cb(void);

#endif /* _GPS_MCUDL_HAL_CCIF_H */

