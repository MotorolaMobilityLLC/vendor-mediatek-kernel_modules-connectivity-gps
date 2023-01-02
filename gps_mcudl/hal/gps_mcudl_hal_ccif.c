/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#include "gps_mcudl_hal_ccif.h"
#include "gps_mcudl_hw_ccif.h"
#include "gps_dl_isr.h"
#include "gps_dl_time_tick.h"
#include "gps_dl_subsys_reset.h"
#include "gps_mcu_hif_host.h"
#include "gps_mcudl_log.h"
#include "gps_mcudl_ylink.h"
#include "gps_mcudl_data_pkt_slot.h"
#include "gps_mcudl_hal_user_fw_own_ctrl.h"
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

unsigned long g_gps_ccif_irq_cnt;
unsigned int g_gps_fw_log_irq_cnt;
void gps_mcudl_hal_ccif_rx_isr(void)
{
	unsigned int rch_mask, last_rch_mask = 0;
	enum gps_mcudl_ccif_ch ch;
	unsigned long tick_us0, tick_us1, dt_us;
	unsigned int recheck_cnt = 0;
	bool already_wakeup = false;

	gps_dl_irq_mask(gps_dl_irq_index_to_id(GPS_DL_IRQ_CCIF), GPS_DL_IRQ_CTRL_FROM_ISR);
	g_gps_ccif_irq_cnt++;
	tick_us0 = gps_dl_tick_get_us();

	already_wakeup = gps_mcudl_hal_user_add_if_fw_own_is_clear(GMDL_FW_OWN_CTRL_BY_CCIF);
	if (!already_wakeup) {
		GDL_LOGD("ntf to clr_fw_own, ccif_irq_cnt=%d", g_gps_ccif_irq_cnt);
		gps_mcudl_hal_set_ccif_irq_en_flag(false);
		gps_mcudl_ylink_event_send(GPS_MDLY_NORMAL, GPS_MCUDL_YLINK_EVT_ID_CCIF_CLR_FW_OWN);
		return;
	}

recheck_rch:
	if (!gps_dl_conninfra_is_readable()) {
		GDL_LOGE("readable check fail, ccif_irq_cnt=%d", g_gps_ccif_irq_cnt);
		gps_mcudl_hal_set_ccif_irq_en_flag(false);
		gps_mcudl_ylink_event_send(GPS_MDLY_NORMAL, GPS_MCUDL_YLINK_EVT_ID_CCIF_ISR_ABNORMAL);
		return;
	}
	rch_mask = gps_mcudl_hw_ccif_get_rch_bitmask();

	/* handling read 0xdeadfeed case */
	if ((rch_mask & 0xFFFFFF00) != 0) {
		GDL_LOGW("cnt=%lu, rch_mask=0x%x, abnormal", g_gps_ccif_irq_cnt, rch_mask);
		gps_mcudl_hal_set_ccif_irq_en_flag(false);
		gps_mcudl_ylink_event_send(GPS_MDLY_NORMAL, GPS_MCUDL_YLINK_EVT_ID_CCIF_ISR_ABNORMAL);
		return;
	}

	if (rch_mask == 0) {
		gps_dl_irq_unmask(gps_dl_irq_index_to_id(GPS_DL_IRQ_CCIF), GPS_DL_IRQ_CTRL_FROM_ISR);
		gps_mcudl_hal_user_set_fw_own_may_notify(GMDL_FW_OWN_CTRL_BY_CCIF);
		return;
	}

	for (ch = 0; ch < GPS_MCUDL_CCIF_CH_NUM; ch++) {
		if (rch_mask & (1UL << ch)) {
			gps_mcudl_hw_ccif_set_rch_ack(ch);
			if (ch == GPS_MCUDL_CCIF_CH4)
				gps_mcu_hif_host_ccif_irq_handler_in_isr();
#if GPS_DL_HAS_CONNINFRA_DRV
			else if (ch == GPS_MCUDL_CCIF_CH1) {
				connsys_log_irq_handler(CONN_DEBUG_TYPE_GPS);
				g_gps_fw_log_irq_cnt++;
				MDL_LOGW("gps_fw_log_irq_cnt=%d", g_gps_fw_log_irq_cnt);
			} else if (ch == GPS_MCUDL_CCIF_CH2) {
#if 0
				/* SUBSYS reset
				 * Currently, treat it as whole chip reset
				 */
				conninfra_trigger_whole_chip_rst(
					CONNDRV_TYPE_GPS, "GNSS FW trigger subsys reset");
#else
				/* mark irq is not enabled and return for this case */
				gps_mcudl_hal_set_ccif_irq_en_flag(false);

				/* SUBSYS reset */
				gps_mcudl_trigger_gps_subsys_reset(false, "GNSS FW trigger subsys reset");
				return;
#endif
			} else if (ch == GPS_MCUDL_CCIF_CH3) {
				/* mark irq is not enabled and return for this case */
				gps_mcudl_hal_set_ccif_irq_en_flag(false);

				/* WHOLE_CHIP reset */
				conninfra_trigger_whole_chip_rst(
					CONNDRV_TYPE_GPS, "GNSS FW trigger whole chip reset");
				return;
			}
#endif
		}
	}

	tick_us1 = gps_dl_tick_get_us();
	dt_us = tick_us1 - tick_us0;
	if ((rch_mask != (1UL << GPS_MCUDL_CCIF_CH4)) || (dt_us >= 10000)) {
		GDL_LOGW("cnt=%lu, rch_mask=0x%x, dt_us=%lu, recheck=%u, last_mask=0x%x",
			g_gps_ccif_irq_cnt, rch_mask, dt_us, recheck_cnt, last_rch_mask);
	} else {
		GDL_LOGD("cnt=%lu, rch_mask=0x%x, dt_us=%lu, recheck=%u, last_mask=0x%x",
			g_gps_ccif_irq_cnt, rch_mask, dt_us, recheck_cnt, last_rch_mask);
	}
	recheck_cnt++;
	last_rch_mask = rch_mask;
	goto recheck_rch;
}

bool g_gps_ccif_irq_en;

bool gps_mcudl_hal_get_ccif_irq_en_flag(void)
{
	bool enable;

	gps_mcudl_slot_protect();
	enable = g_gps_ccif_irq_en;
	gps_mcudl_slot_unprotect();
	return enable;
}

void gps_mcudl_hal_set_ccif_irq_en_flag(bool enable)
{
	gps_mcudl_slot_protect();
	g_gps_ccif_irq_en = enable;
	gps_mcudl_slot_unprotect();
}

