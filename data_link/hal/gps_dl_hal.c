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
#include "gps_dl_hal.h"
#include "gps_dl_hw_api.h"
#include "gps_dsp_fsm.h"
#include "gps_each_link.h"
#include "gps_dl_isr.h"

char *hal_event_name[GPD_DL_HAL_EVT_NUM + 1] = {
	[GPS_DL_HAL_EVT_A2D_TX_DMA_DONE] = "HAL_TX_DMA_DONE",
	[GPS_DL_HAL_EVT_D2A_RX_HAS_DATA] = "HAL_RX_HAS_DATA",
	[GPS_DL_HAL_EVT_D2A_RX_HAS_NODATA] = "HAL_RX_HAS_NODATA",
	[GPS_DL_HAL_EVT_D2A_RX_DMA_DONE] = "HAL_RX_DMA_DONE",
	[GPD_DL_HAL_EVT_NUM] = "HAL_INVALID_EVT",
};

char *gps_dl_hal_event_name(enum gps_dl_hal_event_id evt)
{
	if (evt >= 0 && evt < GPD_DL_HAL_EVT_NUM)
		return hal_event_name[evt];
	else
		return hal_event_name[GPD_DL_HAL_EVT_NUM];
}

void gps_dl_hal_event_send(enum gps_dl_hal_event_id evt,
	enum gps_dl_link_id_enum link_id)
{
#if (GPS_DL_HAS_CTRLD == 0)
	gps_dl_hal_event_proc(evt, link_id, gps_each_link_get_session_id(link_id));
#else
	{
		struct gps_dl_osal_lxop *pOp;
		struct gps_dl_osal_signal *pSignal;
		int iRet;

		pOp = gps_dl_get_free_op();
		if (!pOp)
			return;

		pSignal = &pOp->signal;
		pSignal->timeoutValue = 0;/* send data need to wait ?ms */
		if (link_id < GPS_DATA_LINK_NUM) {
			pOp->op.opId = GPS_DL_OPID_HAL_EVENT_PROC;
			pOp->op.au4OpData[0] = link_id;
			pOp->op.au4OpData[1] = evt;
			pOp->op.au4OpData[2] = gps_each_link_get_session_id(link_id);
			iRet = gps_dl_put_act_op(pOp);
		} else {
			gps_dl_put_op_to_free_queue(pOp);
			/*printf error msg*/
			return;
		}
	}
#endif
}

void gps_dl_hal_event_proc(enum gps_dl_hal_event_id evt,
	enum gps_dl_link_id_enum link_id, int sid_on_evt)
{
	struct gps_each_link *p_link = gps_dl_link_get(link_id);
	struct gdl_dma_buf_entry dma_buf_entry;
	enum GDL_RET_STATUS gdl_ret;
	unsigned int write_index;
	int curr_sid;

	curr_sid = gps_each_link_get_session_id(link_id);


	if (gps_each_link_get_bool_flag(link_id, LINK_IS_RESETTING)) {
		/* ack the reset status */
		return;
	}

	if (sid_on_evt != curr_sid && sid_on_evt != GPS_EACH_LINK_SID_NO_CHECK) {
		GDL_LOGXW(link_id, "curr_sid = %d, evt = %s, on_sid = %d, not matching",
			curr_sid, gps_dl_hal_event_name(evt), sid_on_evt);
		return;
	} else if (!gps_each_link_is_active(link_id) ||
		gps_each_link_get_bool_flag(link_id, LINK_TO_BE_CLOSED)) {
		GDL_LOGXW(link_id, "curr_sid = %d, evt = %s, on_sid = %d, not active",
			curr_sid, gps_dl_hal_event_name(evt), sid_on_evt);
		return;
	}

	if (sid_on_evt == GPS_EACH_LINK_SID_NO_CHECK) {
		GDL_LOGXW(link_id, "curr_sid = %d, evt = %s, on_sid = %d, no check",
			curr_sid, gps_dl_hal_event_name(evt), sid_on_evt);
	} else if (sid_on_evt <= 0 || sid_on_evt > GPS_EACH_LINK_SID_MAX) {
		GDL_LOGXW(link_id, "curr_sid = %d, evt = %s, on_sid = %d, out of range",
			curr_sid, gps_dl_hal_event_name(evt), sid_on_evt);
	} else {
		GDL_LOGXD(link_id, "curr_sid = %d, evt = %s, on_sid = %d",
			curr_sid, gps_dl_hal_event_name(evt), sid_on_evt);
	}

	switch (evt) {
	case GPS_DL_HAL_EVT_D2A_RX_HAS_DATA:
		gdl_ret = gdl_dma_buf_get_free_entry(
			&p_link->rx_dma_buf, &dma_buf_entry);

		if (gdl_ret == GDL_OKAY)
			gps_dl_hal_d2a_rx_dma_start(link_id, &dma_buf_entry);
		else {
			GDL_LOGXW(link_id, "rx dma not start due to %s", gdl_ret_to_name(gdl_ret));

			/* TODO: has pending rx */
			p_link->rx_dma_buf.has_pending_rx = true;
		}
		break;

	/* TODO: handle the case data_len is just equal to buf_len, */
	/* the rx_dma_done and usrt_has_nodata both happen. */
	case GPS_DL_HAL_EVT_D2A_RX_DMA_DONE:
		/* TODO: to make mock work with it */

		/* stop and clear int flag in isr */
		/* gps_dl_hal_d2a_rx_dma_stop(link_id); */
		p_link->rx_dma_buf.dma_working_entry.write_index =
			p_link->rx_dma_buf.dma_working_entry.read_index;

		/* check whether no data also happen */
		if (gps_dl_hw_usrt_has_set_nodata_flag(link_id)) {
			p_link->rx_dma_buf.dma_working_entry.is_nodata = true;
			gps_dl_hw_usrt_clear_nodata_irq(link_id);
		} else
			p_link->rx_dma_buf.dma_working_entry.is_nodata = false;

		gdl_ret = gdl_dma_buf_set_free_entry(&p_link->rx_dma_buf,
			&p_link->rx_dma_buf.dma_working_entry);

		if (gdl_ret == GDL_OKAY) {
			p_link->rx_dma_buf.dma_working_entry.is_valid = false;
			gps_dl_link_wake_up(&p_link->waitables[GPS_DL_WAIT_READ]);
		}

		/* mask data irq */
		gps_dl_irq_each_link_unmask(link_id, GPS_DL_IRQ_TYPE_HAS_DATA, GPS_DL_IRQ_CTRL_FROM_HAL);
		break;

	case GPS_DL_HAL_EVT_D2A_RX_HAS_NODATA:
		/* get rx length */
		gdl_ret = gps_dl_hal_d2a_rx_dma_get_write_index(link_id, &write_index);

		/* 20181118 for mock, rx dma stop must after get write index */
		gps_dl_hal_d2a_rx_dma_stop(link_id);

		if (gdl_ret == GDL_OKAY) {
			/* no need to mask data irq */
			p_link->rx_dma_buf.dma_working_entry.write_index = write_index;
			p_link->rx_dma_buf.dma_working_entry.is_nodata = true;

			gdl_ret = gdl_dma_buf_set_free_entry(&p_link->rx_dma_buf,
				&p_link->rx_dma_buf.dma_working_entry);

			if (gdl_ret != GDL_OKAY)
				GDL_LOGD("gdl_dma_buf_set_free_entry ret = %s", gdl_ret_to_name(gdl_ret));

		} else
			GDL_LOGD("gps_dl_hal_d2a_rx_dma_get_write_index ret = %s", gdl_ret_to_name(gdl_ret));

		if (gdl_ret == GDL_OKAY) {
			p_link->rx_dma_buf.dma_working_entry.is_valid = false;
			gps_dl_link_wake_up(&p_link->waitables[GPS_DL_WAIT_READ]);
		}

		gps_dl_hw_usrt_clear_nodata_irq(link_id);
		gps_dl_irq_each_link_unmask(link_id, GPS_DL_IRQ_TYPE_HAS_NODATA, GPS_DL_IRQ_CTRL_FROM_HAL);
		gps_dl_irq_each_link_unmask(link_id, GPS_DL_IRQ_TYPE_HAS_DATA, GPS_DL_IRQ_CTRL_FROM_HAL);
		break;

	case GPS_DL_HAL_EVT_A2D_TX_DMA_DONE:
		/* data tx finished */
		gdl_ret = gdl_dma_buf_set_data_entry(&p_link->tx_dma_buf,
			&p_link->tx_dma_buf.dma_working_entry);

		p_link->tx_dma_buf.dma_working_entry.is_valid = false;

		GDL_LOGD("gdl_dma_buf_set_data_entry ret = %s", gdl_ret_to_name(gdl_ret));

		/* stop tx dma, should stop and clear int flag in isr */
		/* gps_dl_hal_a2d_tx_dma_stop(link_id); */

		/* wakeup writer if it's pending on it */
		gps_dl_link_wake_up(&p_link->waitables[GPS_DL_WAIT_WRITE]);

		gdl_ret = gdl_dma_buf_get_data_entry(&p_link->tx_dma_buf, &dma_buf_entry);
		if (gdl_ret == GDL_OKAY)
			gps_dl_hal_a2d_tx_dma_start(link_id, &dma_buf_entry);
		else
			GDL_LOGD("gdl_dma_buf_get_data_entry ret = %s", gdl_ret_to_name(gdl_ret));
		break;

	case GPS_DL_HAL_EVT_MCUB_HAS_IRQ:
		gps_dl_hal_mcub_flag_handler(link_id);
		gps_dl_irq_each_link_unmask(link_id, GPS_DL_IRQ_TYPE_MCUB, GPS_DL_IRQ_CTRL_FROM_HAL);
		break;

#if 0
	case GPS_DL_HAL_EVT_DSP_ROM_START:
		gps_dsp_fsm(GPS_DSP_EVT_RESET_DONE, link_id);

		/* if has pending data, can send it now */
		gdl_ret = gdl_dma_buf_get_data_entry(&p_link->tx_dma_buf, &dma_buf_entry);
		if (gdl_ret == GDL_OKAY)
			gps_dl_hal_a2d_tx_dma_start(link_id, &dma_buf_entry);
		break;

	case GPS_DL_HAL_EVT_DSP_RAM_START:
		gps_dsp_fsm(GPS_DSP_EVT_RAM_CODE_READY, link_id);

		/* start reg polling */
		break;
#endif

	default:
		break;
	}
}

void gps_dl_hal_mcub_flag_handler(enum gps_dl_link_id_enum link_id)
{
	struct gps_dl_hal_mcub_info d2a;

	/* Todo: while condition make sure DSP is on and session ID */
	while (1) {
		gps_dl_hw_get_mcub_info(link_id, &d2a);

		GDL_LOGXD(link_id, "d2a: flag = 0x%04x, d0 = 0x%04x, d1 = 0x%04x",
			d2a.flag, d2a.dat0, d2a.dat1);

		if (d2a.flag == 0)
			break;

		/* Todo: if (dsp is off) -> break */
		/* Note: clear flag before check and handle the flage event,
		 *   avoiding race condtion when dsp do "too fast twice ack".
		 *   EX: gps_each_dsp_reg_gourp_read_next
		 */
		gps_dl_hw_clear_mcub_d2a_flag(link_id, d2a.flag);

		if (d2a.flag & GPS_MCUB_D2AF_MASK_DSP_RESET_DONE) {
			/* gps_dl_hal_event_send(GPS_DL_HAL_EVT_DSP_ROM_START, link_id); */
			gps_dsp_fsm(GPS_DSP_EVT_RESET_DONE, link_id);
		}

		if (d2a.flag & GPS_MCUB_D2AF_MASK_DSP_RAMCODE_READY) {
			/* gps_dl_hal_event_send(GPS_DL_HAL_EVT_DSP_RAM_START, link_id); */
			gps_dsp_fsm(GPS_DSP_EVT_RAM_CODE_READY, link_id);

			/* start reg polling */
			gps_each_dsp_reg_gourp_read_start(link_id);
		}

		if (d2a.flag & GPS_MCUB_D2AF_MASK_DSP_REG_READ_READY)
			gps_each_dsp_reg_read_ack(link_id, &d2a);

	}
}

