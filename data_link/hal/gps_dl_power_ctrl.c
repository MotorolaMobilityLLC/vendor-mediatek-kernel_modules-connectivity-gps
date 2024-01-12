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
#if GPS_DL_MOCK_HAL
#include "gps_mock_hal.h"
#endif
#if GPS_DL_HAS_CONNINFRA_DRV
#include "conninfra.h"
#endif
#if GPS_DL_HAS_PLAT_DRV
#include "gps_dl_linux_plat_drv.h"
#endif

/* TODO: move them into a single structure */
bool g_gps_common_on;
bool g_gps_dsp_l1_on;
bool g_gps_dsp_l5_on;

unsigned int g_conn_user;

void gps_dl_hal_link_power_ctrl(enum gps_dl_link_id_enum link_id, int op)
{
	GDL_LOGXI(link_id, "op = %d, curr = g:%d,l1:%d,l5:%d",
		op, g_gps_common_on, g_gps_dsp_l1_on, g_gps_dsp_l5_on);

	/* TODO: init usrt/dma when on; confirm urst/dma is proper when off */

	/* fill it by datasheet */
	if (1 == op) {
		/* GPS common */
		if (!(g_gps_dsp_l1_on || g_gps_dsp_l5_on)) {
			/* assert g_gps_common_on == false */
			gps_dl_hw_gps_common_on();
			g_gps_common_on = true;
		}

		/* DSP */
		if (!g_gps_dsp_l1_on && (GPS_DATA_LINK_ID0 == link_id)) {
			gps_dl_hw_gps_dsp_ctrl(GPS_L1_DSP_ON);
			g_gps_dsp_l1_on = true;
		} else if (!g_gps_dsp_l5_on && (GPS_DATA_LINK_ID1 == link_id)) {
			gps_dl_hw_gps_dsp_ctrl(GPS_L5_DSP_ON);
			g_gps_dsp_l5_on = true;
		}

		/* USRT setting */
		gps_dl_hw_usrt_ctrl(link_id, true, gps_dl_is_dma_enabled(), gps_dl_is_1byte_mode());

#if GPS_DL_MOCK_HAL
		gps_dl_mock_open(link_id);
#endif
	} else if (0 == op) {
		/* USRT setting */
		gps_dl_hw_usrt_ctrl(link_id, false, gps_dl_is_dma_enabled(), gps_dl_is_1byte_mode());

		if (g_gps_dsp_l1_on && (GPS_DATA_LINK_ID0 == link_id)) {
#if 0
			if (g_gps_dsp_l5_on && (0x1 == GDL_HW_GET_GPS_ENTRY(GPS_RGU_ON_GPS_L5_CR_RGU_GPS_L5_ON)))
				/* TODO: ASSERT SW status and HW status are same */
#endif
			gps_dl_hw_gps_dsp_ctrl(GPS_L1_DSP_OFF);
			g_gps_dsp_l1_on = false;
		} else if (g_gps_dsp_l5_on && (GPS_DATA_LINK_ID1 == link_id)) {
#if 0
			if (g_gps_dsp_l1_on && (0x1 == GDL_HW_GET_GPS_ENTRY(GPS_RGU_ON_GPS_L1_CR_RGU_GPS_L1_ON)))
				/* TODO: ASSERT SW status and HW status are same */
#endif
			gps_dl_hw_gps_dsp_ctrl(GPS_L5_DSP_OFF);
			g_gps_dsp_l5_on = false;
		}

		if (!(g_gps_dsp_l1_on || g_gps_dsp_l5_on)) {
			/* assert g_gps_common_on == true */
			gps_dl_hw_gps_common_off();
			g_gps_common_on = false;
		}
#if GPS_DL_MOCK_HAL
		gps_dl_mock_close(link_id);
#endif
	}
}

int gps_dl_hal_conn_power_ctrl(enum gps_dl_link_id_enum link_id, int op)
{
	unsigned int old_user;

	if (1 == op) {
		if (g_conn_user == 0) {
#if GPS_DL_HAS_CONNINFRA_DRV
			/* TODO: handling fail case */
			conninfra_pwr_on(CONNDRV_TYPE_GPS);
#endif
			gps_dl_remap_ctx_cal_and_set();

#if GPS_DL_HAS_PLAT_DRV
			gps_dl_wake_lock_hold(true);
			gps_dl_tia_gps_ctrl(true);
			gps_dl_update_status_for_md_blanking(true);
#endif
			gps_dl_irq_unmask_dma_intr(GPS_DL_IRQ_CTRL_FROM_THREAD);
		}
		g_conn_user |= (1UL << link_id);
	} else if (0 == op) {
		old_user = g_conn_user;
		g_conn_user &= ~(1UL << link_id);
		if (old_user != 0 && g_conn_user == 0) {
			/* Note: currently, twice mask should not happen here, */
			/* due to ISR does mask/unmask pair operations */
			gps_dl_irq_mask_dma_intr(GPS_DL_IRQ_CTRL_FROM_THREAD);
#if GPS_DL_HAS_PLAT_DRV
			gps_dl_update_status_for_md_blanking(false);
			gps_dl_tia_gps_ctrl(false);
			gps_dl_wake_lock_hold(false);
#endif
#if GPS_DL_HAS_CONNINFRA_DRV
			conninfra_pwr_off(CONNDRV_TYPE_GPS);
#endif
		}
	}

	return 0;
}

