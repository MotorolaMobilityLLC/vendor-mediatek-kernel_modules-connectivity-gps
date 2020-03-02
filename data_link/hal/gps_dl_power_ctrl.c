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

#include "gps_dl_hal.h"
#include "gps_dl_hw_api.h"
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

#if GPS_DL_HAS_PLAT_DRV
#include "gps_dl_linux_plat_drv.h"
#endif
#if GPS_DL_ON_CTP
#include "gps_dl_ctp.h"
#endif

/* TODO: move them into a single structure */
bool g_gps_common_on;
bool g_gps_dsp_on_array[GPS_DATA_LINK_NUM];
int g_gps_dsp_off_ret_array[GPS_DATA_LINK_NUM];
bool g_gps_conninfa_on;
bool g_gps_pta_init_done;
bool g_gps_tia_on;
bool g_gps_dma_irq_en;

unsigned int g_conn_user;

bool gps_dl_hal_get_dma_irq_en_flag(void)
{
	bool enable;

	/* spin lock protection between gps_dl_isr_dma_done and gps_kctrld
	 * both link 0 & 1 spin lock can be used, use 0's.
	 */
	gps_each_link_spin_lock_take(GPS_DATA_LINK_ID0, GPS_DL_SPINLOCK_FOR_LINK_STATE);
	enable = g_gps_dma_irq_en;
	gps_each_link_spin_lock_give(GPS_DATA_LINK_ID0, GPS_DL_SPINLOCK_FOR_LINK_STATE);
	return enable;
}

void gps_dl_hal_set_dma_irq_en_flag(bool enable)
{
	gps_each_link_spin_lock_take(GPS_DATA_LINK_ID0, GPS_DL_SPINLOCK_FOR_LINK_STATE);
	g_gps_dma_irq_en = enable;
	gps_each_link_spin_lock_give(GPS_DATA_LINK_ID0, GPS_DL_SPINLOCK_FOR_LINK_STATE);
}

int gps_dl_hal_link_power_ctrl(enum gps_dl_link_id_enum link_id, int op)
{
	int dsp_ctrl_ret;
	bool conninfra_okay;
	int hung_value;

	conninfra_okay = gps_dl_conninfra_is_okay_or_handle_it(&hung_value, (op == 0));
	GDL_LOGXI(link_id, "op = %d, conn_okay = %d, gps_common = %d, L1 = %d, L5 = %d",
		op, conninfra_okay, g_gps_common_on,
		g_gps_dsp_on_array[GPS_DATA_LINK_ID0],
		g_gps_dsp_on_array[GPS_DATA_LINK_ID1]);

	if (!conninfra_okay) {
		GDL_LOGXE(link_id, "conninfra_okay = %d, hung_value = %d",
			conninfra_okay, hung_value);
		if (hung_value == 0) {
			/* for this excepition, trigger connsys reset to make GPS power okay */
			/*
			 * TODO: no care it now
			 * gps_dl_trigger_connsys_reset();
			 */
		} else
			return -1;
	}

	/* fill it by datasheet */
	if (1 == op) {
		/* GPS SW force wakeup conninfra top off */
		gps_dl_hw_gps_force_wakeup_conninfra_top_off(true);

		/* GPS common */
		if (!g_gps_common_on) {
			if (gps_dl_hw_gps_common_on() != 0) {
				gps_dl_hw_gps_force_wakeup_conninfra_top_off(false);
				return -1;
			}
#if GPS_DL_ON_LINUX
			gps_dl_hal_md_blanking_init_pta();
#endif
			g_gps_common_on = true;
		}

		if (g_gps_dsp_on_array[link_id]) {
			gps_dl_hw_gps_force_wakeup_conninfra_top_off(false);
			return 0;
		}

		g_gps_dsp_on_array[link_id] = true;

#if GPS_DL_HAS_PLAT_DRV
		gps_dl_lna_pin_ctrl(link_id, true, false);
#endif
#if GPS_DL_MOCK_HAL
		gps_dl_mock_open(link_id);
#endif

		if (GPS_DATA_LINK_ID0 == link_id)
			dsp_ctrl_ret = gps_dl_hw_gps_dsp_ctrl(GPS_L1_DSP_ON);
		else if (GPS_DATA_LINK_ID1 == link_id)
			dsp_ctrl_ret = gps_dl_hw_gps_dsp_ctrl(GPS_L5_DSP_ON);
		else
			dsp_ctrl_ret = -1;
		gps_dl_hw_gps_force_wakeup_conninfra_top_off(false);

		if (dsp_ctrl_ret == 0) {
			/* USRT setting */
			gps_dl_hw_usrt_ctrl(link_id, true, gps_dl_is_dma_enabled(), gps_dl_is_1byte_mode());
			return 0;
		} else
			return -1;
	} else if (3 == op || 5 == op) {
		if (GPS_DATA_LINK_ID0 == link_id) {
			if (3 == op)
				gps_dl_hw_gps_dsp_ctrl(GPS_L1_DSP_EXIT_DSLEEP);
			else if (5 == op)
				gps_dl_hw_gps_dsp_ctrl(GPS_L1_DSP_EXIT_DSTOP);
		} else if (GPS_DATA_LINK_ID1 == link_id) {
			if (3 == op)
				gps_dl_hw_gps_dsp_ctrl(GPS_L5_DSP_EXIT_DSLEEP);
			else if (5 == op)
				gps_dl_hw_gps_dsp_ctrl(GPS_L5_DSP_EXIT_DSTOP);
		}
		gps_dl_hw_usrt_ctrl(link_id, true, gps_dl_is_dma_enabled(), gps_dl_is_1byte_mode());
	} else if (2 == op || 4 == op) {
		gps_dl_hw_usrt_ctrl(link_id, false, gps_dl_is_dma_enabled(), gps_dl_is_1byte_mode());
		if (GPS_DATA_LINK_ID0 == link_id) {
			if (2 == op)
				gps_dl_hw_gps_dsp_ctrl(GPS_L1_DSP_ENTER_DSLEEP);
			else if (4 == op)
				gps_dl_hw_gps_dsp_ctrl(GPS_L1_DSP_ENTER_DSTOP);
		} else if (GPS_DATA_LINK_ID1 == link_id) {
			if (2 == op)
				gps_dl_hw_gps_dsp_ctrl(GPS_L5_DSP_ENTER_DSLEEP);
			else if (4 == op)
				gps_dl_hw_gps_dsp_ctrl(GPS_L5_DSP_ENTER_DSTOP);
		}
	} else if (0 == op) {
		gps_dl_hw_gps_force_wakeup_conninfra_top_off(true);
		if (g_gps_dsp_on_array[link_id]) {
			g_gps_dsp_on_array[link_id] = false;

			/* USRT setting */
			gps_dl_hw_usrt_ctrl(link_id, false, gps_dl_is_dma_enabled(), gps_dl_is_1byte_mode());

			if (GPS_DATA_LINK_ID0 == link_id) {
#if 0
				if (g_gps_dsp_l5_on &&
					(0x1 == GDL_HW_GET_GPS_ENTRY(GPS_RGU_ON_GPS_L5_CR_RGU_GPS_L5_ON)))
					/* TODO: ASSERT SW status and HW status are same */
#endif
				g_gps_dsp_off_ret_array[link_id] = gps_dl_hw_gps_dsp_ctrl(GPS_L1_DSP_OFF);
			} else if (GPS_DATA_LINK_ID1 == link_id) {
#if 0
				if (g_gps_dsp_l1_on &&
					(0x1 == GDL_HW_GET_GPS_ENTRY(GPS_RGU_ON_GPS_L1_CR_RGU_GPS_L1_ON)))
					/* TODO: ASSERT SW status and HW status are same */
#endif
				g_gps_dsp_off_ret_array[link_id] = gps_dl_hw_gps_dsp_ctrl(GPS_L5_DSP_OFF);
			}

#if GPS_DL_HAS_PLAT_DRV
			gps_dl_lna_pin_ctrl(link_id, false, false);
#endif
#if GPS_DL_MOCK_HAL
			gps_dl_mock_close(link_id);
#endif
		}

		if (!g_gps_dsp_on_array[GPS_DATA_LINK_ID0] && !g_gps_dsp_on_array[GPS_DATA_LINK_ID1]) {
			if ((g_gps_dsp_off_ret_array[GPS_DATA_LINK_ID0] != 0) ||
				(g_gps_dsp_off_ret_array[GPS_DATA_LINK_ID1] != 0)) {
				GDL_LOGXE(link_id, "l1 ret = %d, l5 ret = %d, force adie off",
					g_gps_dsp_off_ret_array[GPS_DATA_LINK_ID0],
					g_gps_dsp_off_ret_array[GPS_DATA_LINK_ID1]);
				gps_dl_hw_gps_adie_force_off();
			}

			if (g_gps_common_on) {
#if GPS_DL_ON_LINUX
				gps_dl_hal_md_blanking_deinit_pta();
#endif
				gps_dl_hw_gps_common_off();
				g_gps_common_on = false;
			}
		}

		gps_dl_hw_gps_force_wakeup_conninfra_top_off(false);
		return 0;
	}

	return -1;
}

int gps_dl_hal_conn_power_ctrl(enum gps_dl_link_id_enum link_id, int op)
{
	int ret = 0;
	int hung_value;

	GDL_LOGXD(link_id,
		"op = %d, conn_user = 0x%x, infra_on = %d, tia_on = %d, dma_irq_en = %d",
		op, g_conn_user, g_gps_conninfa_on, g_gps_tia_on, gps_dl_hal_get_dma_irq_en_flag());

	if (1 == op) {
		if (g_conn_user == 0) {
			gps_dl_log_info_show();
#if GPS_DL_HAS_CONNINFRA_DRV
#if GPS_DL_ON_LINUX
			ret = conninfra_pwr_on(CONNDRV_TYPE_GPS);
#elif GPS_DL_ON_CTP
			ret = conninfra_power_on(CONNDRV_TYPE_GPS);
#endif
			if (ret != 0) {
				GDL_LOGXE(link_id, "conninfra_pwr_on fail, ret = %d", ret);
				return -1;
			}
			g_gps_conninfa_on = true;
#endif
			/* check conninfra okay is suggest after conninfra_pwr_on */
			if (!gps_dl_conninfra_is_okay_or_handle_it(&hung_value, false)) {
				if (hung_value != 0)
					return -1;
			}

			gps_dl_emi_remap_calc_and_set();

#if GPS_DL_HAS_PLAT_DRV
			gps_dl_wake_lock_hold(true);

			gps_dl_tia_gps_ctrl(true);
			g_gps_tia_on = true;
#endif
			gps_dl_irq_unmask_dma_intr(GPS_DL_IRQ_CTRL_FROM_THREAD);
			gps_dl_hal_set_dma_irq_en_flag(true);
		}
		g_conn_user |= (1UL << link_id);
	} else if (0 == op) {
		g_conn_user &= ~(1UL << link_id);
		if (g_conn_user == 0) {
			if (gps_dl_hal_get_dma_irq_en_flag()) {
				/* Note1: currently, twice mask should not happen here,
				 * due to ISR does mask/unmask pair operations,
				 * except one case, please see Note2
				 */
				gps_dl_irq_mask_dma_intr(GPS_DL_IRQ_CTRL_FROM_THREAD);

				/* Note2: double-get to check race condition, if en_flag changed
				 * it must gps_dl_isr_dma_done change it, need to unmask to make balance
				 */
				if (!gps_dl_hal_get_dma_irq_en_flag())
					gps_dl_irq_unmask_dma_intr(GPS_DL_IRQ_CTRL_FROM_THREAD);
				else
					gps_dl_hal_set_dma_irq_en_flag(false);
			}
#if GPS_DL_HAS_PLAT_DRV
			if (g_gps_tia_on) {
				gps_dl_tia_gps_ctrl(false);
				g_gps_tia_on = false;
			}

			gps_dl_wake_lock_hold(false);
#endif
			gps_dl_hal_conn_infra_driver_off();
		}
	}

	return 0;
}

void gps_dl_hal_conn_infra_driver_off(void)
{
#if GPS_DL_HAS_CONNINFRA_DRV
	int ret;

	if (g_gps_conninfa_on) {
		if (!gps_dl_conninfra_is_okay_or_handle_it(NULL, false)) {
			GDL_LOGE("conninfra is not okay");
			/* just show warnning, not return here */
		}
#if GPS_DL_ON_LINUX
		ret = conninfra_pwr_off(CONNDRV_TYPE_GPS);
#elif GPS_DL_ON_CTP
		ret = conninfra_power_off(CONNDRV_TYPE_GPS);
#endif
		if (ret != 0) {
			GDL_LOGE("conninfra_pwr_off fail, ret = %d", ret);

			/* TODO: trigger whole chip reset? */
			return;
		}

		GDL_LOGW("conninfra_pwr_off okay");
		g_gps_conninfa_on = false;
	} else
		GDL_LOGW("conninfra_pwr_off already done");
#endif
}

#if GPS_DL_ON_LINUX
bool gps_dl_hal_md_blanking_init_pta(void)
{
	bool okay, done;

	if (g_gps_pta_init_done) {
		GDL_LOGW("already init done, do nothing return");
		return false;
	}

	if (!gps_dl_hw_is_pta_clk_cfg_ready()) {
		GDL_LOGE("gps_dl_hw_is_pta_clk_cfg_ready fail");
		return false;
	}

	okay = gps_dl_hw_take_conn_hw_sema(100);
	if (!okay) {
		GDL_LOGE("gps_dl_hw_take_conn_hw_sema fail");
		return false;
	}

	/* do pta uart init firstly */
	done = gps_dl_hw_is_pta_uart_init_done();
	if (!done) {
		okay = gps_dl_hw_init_pta_uart();
		if (!okay) {
			gps_dl_hw_give_conn_hw_sema();
			GDL_LOGE("gps_dl_hw_init_pta_uart fail");
			return false;
		}
	} else
		GDL_LOGW("pta uart already init done");

	/* do pta init secondly */
	done = gps_dl_hw_is_pta_init_done();
	if (!done)
		gps_dl_hw_init_pta();
	else
		GDL_LOGW("pta already init done");

	gps_dl_hw_claim_pta_used_by_gps();

	gps_dl_hw_set_pta_blanking_parameter();

	gps_dl_hw_give_conn_hw_sema();

	/* okay, update flags */
#if GPS_DL_HAS_PLAT_DRV
	gps_dl_update_status_for_md_blanking(true);
#endif
	g_gps_pta_init_done = true;

	return true; /* okay */
}

void gps_dl_hal_md_blanking_deinit_pta(void)
{
	bool okay;

	/* clear the flags anyway */
#if GPS_DL_HAS_PLAT_DRV
	gps_dl_update_status_for_md_blanking(false);
#endif
	if (!g_gps_pta_init_done) {
		GDL_LOGW("already deinit done, do nothing return");
		return;
	}
	g_gps_pta_init_done = false;

	/* Currently, deinit is no need.
	 * Just keep the bellow code for future usage.
	 */
	okay = gps_dl_hw_take_conn_hw_sema(0); /* 0 stands for polling just one time */
	if (!okay) {
		GDL_LOGE("gps_dl_hw_take_conn_hw_sema fail");
		return;
	}

	gps_dl_hw_disclaim_pta_used_by_gps();

	if (gps_dl_hw_is_pta_init_done())
		gps_dl_hw_deinit_pta();
	else
		GDL_LOGW("pta already deinit done");

	if (gps_dl_hw_is_pta_uart_init_done())
		gps_dl_hw_deinit_pta_uart();
	else
		GDL_LOGW("pta uart already deinit done");

	gps_dl_hw_give_conn_hw_sema();
}
#endif /* GPS_DL_ON_LINUX */
