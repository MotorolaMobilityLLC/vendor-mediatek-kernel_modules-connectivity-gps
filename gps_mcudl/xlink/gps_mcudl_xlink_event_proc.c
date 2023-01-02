/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */
#include "gps_dl_config.h"

#include "gps_dl_name_list.h"
#include "gps_dl_time_tick.h"
#include "gps_dl_hal.h"
#include "gps_dl_hal_api.h"
#include "gps_dl_hal_util.h"
#include "gps_dl_hw_api.h"
#include "gps_dl_isr.h"
#include "gps_dl_lib_misc.h"
#include "gps_dl_osal.h"
#include "gps_dl_context.h"
#include "gps_dl_subsys_reset.h"
#if GPS_DL_HAS_PLAT_DRV
#include "gps_dl_linux_plat_drv.h"
#endif
#include "gps_mcudl_data_pkt_host_api.h"
#include "gps_mcusys_fsm.h"
#include "gps_mcudl_log.h"
#include "gps_mcudl_context.h"
#include "gps_mcudl_link_util.h"
#include "gps_mcudl_link_sync.h"
#include "gps_mcudl_link_state.h"
#include "gps_mcudl_hal_mcu.h"
#include "gps_mcudl_hal_ccif.h"


void gps_mcudl_xlink_event_send(enum gps_mcudl_xid link_id,
	enum gps_mcudl_xlink_event_id evt)
{
#if GPS_DL_HAS_CTRLD
	struct gps_dl_osal_lxop *pOp = NULL;
	struct gps_dl_osal_signal *pSignal = NULL;
	int iRet;

	pOp = gps_dl_get_free_op();
	if (!pOp) {
		MDL_LOGXE(link_id, "gps_dl_get_free_op failed!");
		return;
	}

	pSignal = &pOp->signal;
	pSignal->timeoutValue = 0;/* send data need to wait ?ms */
	if (link_id < GPS_MDLX_CH_NUM) {
		pOp->op.opId = GPS_DL_OPID_MCUDL_XLINK_EVENT_PROC;
		pOp->op.au4OpData[0] = link_id;
		pOp->op.au4OpData[1] = evt;
		iRet = gps_dl_put_act_op(pOp);
		MDL_LOGXD(link_id, "iRet=%d", iRet);
	} else {
		gps_dl_put_op_to_free_queue(pOp);
		MDL_LOGXE(link_id, "invalid x_id=%d!", link_id);
	}
#else
	gps_mcudl_xlink_event_proc(link_id, evt);
#endif
}

void gps_mcudl_xlink_event_proc(enum gps_mcudl_xid link_id,
	enum gps_mcudl_xlink_event_id evt)
{
	struct gps_mcudl_each_link *p_link = gps_mcudl_link_get(link_id);
	bool show_log = false;
	/*bool show_log2 = false;*/
	unsigned long j0, j1;
	int ret;

	j0 = gps_dl_tick_get();
	MDL_LOGXW_EVT(link_id, "evt = %s", gps_mcudl_xlink_event_name(evt));

	switch (evt) {
	case GPS_MCUDL_EVT_LINK_OPEN:
		/* show_log = gps_dl_set_show_reg_rw_log(true); */
		gps_mcudl_each_link_inc_session_id(link_id);
		gps_mcudl_each_link_set_active(link_id, true);

		if (!gps_mcudl_xlink_is_connected_to_mcu_lifecycle(link_id)) {
			/* connot write for this type of links */
			gps_mcudl_each_link_set_ready_to_write(link_id, false);
			gps_mcudl_link_open_ack(link_id, true);
			break;
		}

		ret = gps_mcudl_hal_conn_power_ctrl(link_id, 1);
		if (ret != 0) {
			gps_mcudl_link_open_ack(link_id, false);
			break;
		}

		ret = gps_mcudl_hal_link_power_ctrl(link_id, 1);
		if (ret != 0) {
			gps_mcudl_hal_link_power_ctrl(link_id, 0);
			gps_mcudl_link_open_ack(link_id, false);
			break;
		}
#if 0
		gps_dl_link_irq_set(link_id, true);
#if GPS_DL_NO_USE_IRQ
		gps_dl_wait_us(1000); /* wait 1ms */
#endif
#endif
		/* TODO: move to mcu state change to later like gps_dsp_state_change_to */
		gps_mcudl_each_link_set_ready_to_write(link_id, true);

		if (!gps_mcudl_xlink_is_connected_mnlbin(link_id)) {
			gps_mcudl_link_open_ack(link_id, true);
			break;
		}

		if (gps_mcusys_mnlbin_state_is(GPS_MCUSYS_MNLBIN_ST_CTLR_CREATED))
			gps_mcudl_link_open_ack(link_id, true);
		else {
			/* wait gps_mcusys_mnlbin_fsm to ack it */
			MDL_LOGXI(link_id, "bypass gps_mcudl_link_open_ack now");
		}

		/* gps_dl_set_show_reg_rw_log(show_log); */
		break;

		/* TODO: go and do close */
	case GPS_MCUDL_EVT_LINK_CLOSE:
	case GPS_MCUDL_EVT_LINK_RESET:
	case GPS_MCUDL_EVT_LINK_PRE_CONN_RESET:
		if (evt != GPS_MCUDL_EVT_LINK_CLOSE)
			show_log = gps_dl_set_show_reg_rw_log(true);

		if (!gps_mcudl_xlink_is_connected_to_mcu_lifecycle(link_id)) {
			/* bypass all mcu related operations for this type of links */
			goto _close_non_mcu_link;
		}

		/* handle open fail case */
		if (!gps_mcudl_each_link_get_bool_flag(link_id, LINK_OPEN_RESULT_OKAY)) {
			MDL_LOGXW(link_id, "not open okay, just power off for %s",
				gps_mcudl_xlink_event_name(evt));

			gps_mcudl_each_link_set_active(link_id, false);
			gps_mcudl_hal_link_power_ctrl(link_id, 0);
			gps_mcudl_hal_conn_power_ctrl(link_id, 0);
			goto _close_or_reset_ack;
		}
#if 0
		/* to avoid twice enter */
		if (GPS_DSP_ST_OFF == gps_dsp_state_get(link_id)) {
			MDL_LOGXD(link_id, "dsp state is off, do nothing for %s",
				gps_mcudl_xlink_event_name(evt));

			if (evt != GPS_DL_EVT_LINK_CLOSE)
				gps_dl_set_show_reg_rw_log(show_log);

			goto _close_or_reset_ack;
		} else if (GPS_DSP_ST_HW_STOP_MODE == gps_dsp_state_get(link_id)) {
			/* exit deep stop mode and turn off it
			 * before exit deep stop, need clear pwr stat to make sure dsp is in hold-on state
			 * after exit deep stop mode.
			 */
			gps_dl_link_pre_leave_dpstop_setting(link_id);
			gps_dl_hal_link_clear_hw_pwr_stat(link_id);
			gps_dl_hal_link_power_ctrl(link_id, GPS_DL_HAL_LEAVE_DPSTOP);
		} else {
			/* make sure current link's DMAs are stopped and mask the IRQs */
			gps_dl_link_pre_off_setting(link_id);
		}
#else
		gps_mcudl_each_link_set_ready_to_write(link_id, false);
		/*gps_dl_link_irq_set(link_id, false);*/
		gps_mcudl_each_link_set_active(link_id, false);
#endif
		if (evt != GPS_MCUDL_EVT_LINK_CLOSE && !g_gps_mcudl_ever_do_coredump) {
			/* try to dump host csr info if not normal close operation */
			/*if (gps_dl_conninfra_is_okay_or_handle_it(NULL, true))*/
			/*	gps_dl_hw_dump_host_csr_gps_info(true);*/

			/*TODO: replace with PAD EINT*/
			gps_mcudl_hal_ccif_tx_prepare(GPS_MCUDL_CCIF_CH3);
			gps_mcudl_hal_ccif_tx_trigger(GPS_MCUDL_CCIF_CH3);
			gps_mcudl_connsys_coredump_start();
			g_gps_mcudl_ever_do_coredump = true;
		}

		gps_mcudl_hal_link_power_ctrl(link_id, 0);
		gps_mcudl_hal_conn_power_ctrl(link_id, 0);
_close_non_mcu_link:
		gps_mcudl_each_link_context_clear(link_id);
#if GPS_DL_ON_LINUX
		gps_dma_buf_reset(&p_link->tx_dma_buf);
		gps_dma_buf_reset(&p_link->rx_dma_buf);
		p_link->epoll_flag = false;
#endif

_close_or_reset_ack:
		if (evt != GPS_MCUDL_EVT_LINK_CLOSE)
			gps_dl_set_show_reg_rw_log(show_log);

		if (GPS_MCUDL_EVT_LINK_CLOSE == evt)
			gps_mcudl_link_close_ack(link_id); /* TODO: check fired race */
		else
			gps_mcudl_link_reset_ack(link_id);
		break;

	case GPS_MCUDL_EVT_LINK_POST_CONN_RESET:
		gps_mcudl_link_on_post_conn_reset(link_id);
		break;

	case GPS_MCUDL_EVT_LINK_WRITE:
		/* gps_dl_hw_print_usrt_status(link_id); */
		if (gps_mcudl_each_link_is_ready_to_write(link_id)) {
#if 0
			gps_dl_link_start_tx_dma_if_has_data(link_id);
#else
			/*
			 * to_flush: w/ gps_mcudl_ap2mcu_xdata_send_v2
			 */
			gps_mcudl_ap2mcu_data_slot_flush_on_xwrite(link_id);

			/* TODO: wake up the threads waiting on xlink */
#endif
		} else
			MDL_LOGXW(link_id, "too early writing");
		break;

	case GPS_MCUDL_EVT_LINK_PRINT_HW_STATUS:
	case GPS_MCUDL_EVT_LINK_PRINT_DATA_STATUS:
		if (!gps_mcudl_each_link_is_active(link_id)) {
			MDL_LOGXW(link_id, "inactive, do not dump hw status");
			break;
		}

		/*gps_dma_buf_show(&p_link->rx_dma_buf, true);*/
		/*gps_dma_buf_show(&p_link->tx_dma_buf, true);*/
		if (!gps_dl_conninfra_is_okay_or_handle_it(NULL, true))
			break;

		show_log = gps_dl_set_show_reg_rw_log(true);
		if (evt == GPS_MCUDL_EVT_LINK_PRINT_HW_STATUS)
			/*gps_dl_hw_dump_host_csr_gps_info(true);*/
		gps_mcudl_hal_mcu_show_status();
		gps_mcudl_hal_ccif_show_status();
		gps_dl_set_show_reg_rw_log(show_log);
		break;

	default:
		break;
	}

	j1 =  gps_dl_tick_get();
	MDL_LOGXW_EVT(link_id, "evt = %s, dj = %lu", gps_mcudl_xlink_event_name(evt), j1 - j0);
}

#if 0
void gps_dl_link_pre_off_setting(enum gps_mcudl_xid link_id)
{
	/*
	 * The order is important:
	 * 1. disallow write, avoiding to start dma
	 * 2. stop tx/rx dma and mask dma irq if it is last link
	 * 3. mask link's irqs
	 * 4. set inactive after all irq mask done
	 * (at this time isr can check inactive and unmask irq safely due to step 3 already mask irqs)
	 */
	gps_dl_link_set_ready_to_write(link_id, false);
	gps_dl_hal_link_confirm_dma_stop(link_id);
	gps_dl_link_irq_set(link_id, false);
	gps_mcudl_each_link_set_active(link_id, false);
}

bool g_gps_dl_dpstop_release_wakelock_fg;
void gps_dl_link_post_enter_dpstop_setting(enum gps_mcudl_xid link_id)
{
	bool cond = true;

#if 0
	cond = (GPS_DSP_ST_HW_STOP_MODE == gps_dsp_state_get(GPS_DATA_LINK_ID0)
			&& (GPS_DSP_ST_HW_STOP_MODE == gps_dsp_state_get(GPS_DATA_LINK_ID1)
			|| GPS_DSP_ST_OFF == gps_dsp_state_get(GPS_DATA_LINK_ID1)))
#endif
	if (cond) {
#if GPS_DL_HAS_PLAT_DRV
		gps_dl_wake_lock_hold(false);
#endif
		g_gps_dl_dpstop_release_wakelock_fg = true;
		MDL_LOGXW(link_id, "enter dpstop with wake_lock relased");
	}
}

void gps_dl_link_pre_leave_dpstop_setting(enum gps_mcudl_xid link_id)
{
	if (g_gps_dl_dpstop_release_wakelock_fg) {
#if GPS_DL_HAS_PLAT_DRV
		gps_dl_wake_lock_hold(true);
#endif
		g_gps_dl_dpstop_release_wakelock_fg = false;
		MDL_LOGXW(link_id, "exit dpstop with wake_lock holded");
	}
}
#endif

