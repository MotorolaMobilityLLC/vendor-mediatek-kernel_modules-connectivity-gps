/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#include "gps_dl_config.h"
#include "gps_mcudl_ylink.h"
#include "gps_dl_context.h"
#include "gps_dl_osal.h"
#include "gps_dl_name_list.h"
#include "gps_mcudl_log.h"
#include "gps_mcudl_plat_api.h"
#include "gps_mcudl_data_pkt_host_api.h"
#include "gps_mcusys_fsm.h"


void gps_mcudl_ylink_event_send(enum gps_mcudl_yid y_id, enum gps_mcudl_ylink_event_id evt)
{
#if GPS_DL_HAS_CTRLD
	struct gps_dl_osal_lxop *pOp = NULL;
	struct gps_dl_osal_signal *pSignal = NULL;
	int iRet;

	pOp = gps_dl_get_free_op();
	if (!pOp)
		return;

	pSignal = &pOp->signal;
	pSignal->timeoutValue = 0;
	if (y_id < GPS_MDLY_CH_NUM) {
		pOp->op.opId = GPS_DL_OPID_MCUDL_YLINK_EVENT_PROC;
		pOp->op.au4OpData[0] = y_id;
		pOp->op.au4OpData[1] = evt;
		iRet = gps_dl_put_act_op(pOp);
	} else {
		gps_dl_put_op_to_free_queue(pOp);
		/*printf error msg*/
		return;
	}
#else
	gps_mcudl_ylink_event_proc(y_id, evt);
#endif
}

void gps_mcudl_ylink_event_proc(enum gps_mcudl_yid y_id, enum gps_mcudl_ylink_event_id evt)
{
	MDL_LOGYW(y_id, "evt=%d", evt);

	switch (evt) {
	case GPS_MCUDL_YLINK_EVT_ID_RX_DATA_READY:
		if (y_id == GPS_MDLY_NORMAL)
			gps_mcudl_stpgps1_read_proc();
		break;

	case GPS_MCUDL_YLINK_EVT_ID_SLOT_FLUSH_ON_RECV_STA:
		gps_mcudl_ap2mcu_data_slot_flush_on_recv_sta(y_id);
		break;
	case GPS_MCUDL_YLINK_EVT_ID_MCU_RESET_START:
		gps_mcusys_mnlbin_fsm(GPS_MCUSYS_MNLBIN_SYS_RESET_START);
		break;
	case GPS_MCUDL_YLINK_EVT_ID_MCU_RESET_END:
		gps_mcusys_mnlbin_fsm(GPS_MCUSYS_MNLBIN_SYS_RESET_END);
		break;
	default:
		break;
	}
}

