/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#include "gps_mcudl_hal_ccif.h"
#include "gps_mcudl_hw_ccif.h"
#include "gps_dl_isr.h"
#include "gps_mcu_hif_host.h"
#include "gps_mcudl_log.h"
#if GPS_DL_HAS_CONNINFRA_DRV
#include "conninfra.h"
#include "connsyslog.h"
#endif

bool gps_mcudl_hal_ccif_tx_is_busy(enum gps_mcudl_ccif_ch ch)
{
	return gps_mcudl_hw_ccif_is_tch_busy(ch);
}

void gps_mcudl_hal_ccif_tx_prepare(enum gps_mcudl_ccif_ch ch)
{
	gps_mcudl_hw_ccif_set_tch_busy(ch);
}

void gps_mcudl_hal_ccif_tx_trigger(enum gps_mcudl_ccif_ch ch)
{
	gps_mcudl_hw_ccif_set_tch_start(ch);
}

bool gps_mcudl_hal_ccif_tx_is_ack_done(enum gps_mcudl_ccif_ch ch)
{
	return !!(gps_mcudl_hw_ccif_get_tch_start_bitmask() & (1UL << ch));
}

void gps_mcudl_hal_ccif_show_status(void)
{
	unsigned int busy_mask, start_mask, rch_mask;
	enum gpsmdl_ccif_misc_cr_group id;
	unsigned int mask, dummy, shadow;

	busy_mask = gps_mcudl_hw_ccif_get_tch_busy_bitmask();
	start_mask = gps_mcudl_hw_ccif_get_tch_start_bitmask();
	rch_mask = gps_mcudl_hw_ccif_get_rch_bitmask();
	MDL_LOGW("busy=0x%x, start=0x%x, rch=0x%x",
		busy_mask, start_mask, rch_mask);

	for (id = 0; id < GMDL_CCIF_MISC_NUM; id++) {
		mask = gps_mcudl_hw_ccif_get_irq_mask(id);
		dummy = gps_mcudl_hw_ccif_get_dummy(id);
		shadow = gps_mcudl_hw_ccif_get_shadow(id);
		MDL_LOGW("msic_id=%d, mask=0x%x, dummy=0x%x, shadow=0x%x",
			id, mask, dummy, shadow);
	}
}

unsigned int g_gps_fw_log_irq_cnt;
void gps_mcudl_hal_ccif_rx_isr(void)
{
	unsigned int rch_mask;
	enum gps_mcudl_ccif_ch ch;

	gps_dl_irq_mask(gps_dl_irq_index_to_id(GPS_DL_IRQ_CCIF), GPS_DL_IRQ_CTRL_FROM_ISR);
	rch_mask = gps_mcudl_hw_ccif_get_rch_bitmask();
	GDL_LOGW("rch_mask=0x%x", rch_mask);
	for (ch = 0; ch < GPS_MCUDL_CCIF_CH_NUM; ch++) {
		if (rch_mask & (1UL << ch)) {
			gps_mcudl_hw_ccif_set_rch_ack(ch);
			if (ch == GPS_MCUDL_CCIF_CH4)
				gps_mcu_hif_host_ccif_isr();
#if GPS_DL_HAS_CONNINFRA_DRV
			else if (ch == GPS_MCUDL_CCIF_CH1) {
				connsys_log_irq_handler(CONN_DEBUG_TYPE_GPS);
				g_gps_fw_log_irq_cnt++;
				MDL_LOGW("gps_fw_log_irq_cnt=%d", g_gps_fw_log_irq_cnt);
			} else if (ch == GPS_MCUDL_CCIF_CH2) {
				/* SUBSYS reset
				 * Currently, treat it as whole chip reset
				 */
				conninfra_trigger_whole_chip_rst(
					CONNDRV_TYPE_GPS, "GNSS FW trigger subsys reset");
			} else if (ch == GPS_MCUDL_CCIF_CH3) {
				/* WHOLE_CHIP reset */
				conninfra_trigger_whole_chip_rst(
					CONNDRV_TYPE_GPS, "GNSS FW trigger whole chip reset");
			}
#endif
		}
	}
	gps_dl_irq_unmask(gps_dl_irq_index_to_id(GPS_DL_IRQ_CCIF), GPS_DL_IRQ_CTRL_FROM_ISR);
}

void gps_mcudl_hal_ccif_rx_ch1_cb(void)
{
}

void gps_mcudl_hal_ccif_rx_ch2_to_ch8_cb(void)
{
}
