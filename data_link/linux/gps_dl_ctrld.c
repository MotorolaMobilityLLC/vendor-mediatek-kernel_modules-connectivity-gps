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
#include "gps_dl_ctrld.h"
#include "gps_each_device.h"
#if GPS_DL_MOCK_HAL
#include "gps_mock_mvcd.h"
#endif
#include "gps_data_link_devices.h"
#include "gps_dl_hal_api.h"

/*******************************************************************************
*                            P U B L I C   D A T A
********************************************************************************
*/

struct gps_dl_ctrld_context gps_dl_ctrld;

/*******************************************************************************
*                  F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/

#define GPS_DL_ERR_FUNC(fmt, arg...) osal_err_print("[E]%s(%d):"  fmt, __func__, __LINE__, ##arg)
#define GPS_DL_INFO_FUNC(fmt, arg...) osal_warn_print("[I]%s:"  fmt, __func__, ##arg)
#define GPS_DL_WARN_FUNC(fmt, arg...) osal_warn_print("[W]%s:"  fmt, __func__, ##arg)

static int gps_dl_opfunc_hardware_control(struct gps_dl_osal_op_dat *pOpDat);
static int gps_dl_opfunc_power_on_link0(struct gps_dl_osal_op_dat *pOpDat);
static int gps_dl_opfunc_power_on_link1(struct gps_dl_osal_op_dat *pOpDat);
static int gps_dl_opfunc_power_off_link0(struct gps_dl_osal_op_dat *pOpDat);
static int gps_dl_opfunc_power_off_link1(struct gps_dl_osal_op_dat *pOpDat);
static int gps_dl_opfunc_send_data_link0(struct gps_dl_osal_op_dat *pOpDat);
static int gps_dl_opfunc_send_data_link1(struct gps_dl_osal_op_dat *pOpDat);
static int gps_dl_opfunc_link_event_proc(struct gps_dl_osal_op_dat *pOpDat);
static int gps_dl_opfunc_hal_event_proc(struct gps_dl_osal_op_dat *pOpDat);
static struct gps_dl_osal_lxop *gps_dl_get_op(struct gps_dl_osal_lxop_q *pOpQ);
static int gps_dl_put_op(struct gps_dl_osal_lxop_q *pOpQ, struct gps_dl_osal_lxop *pOp);

static const GPS_DL_OPID_FUNC gps_dl_core_opfunc[] = {
	[GPS_DL_OPID_HW_CONF] = gps_dl_opfunc_hardware_control,/*HW config func*/
	[GPS_DL_OPID_PWR_ON_DL0] = gps_dl_opfunc_power_on_link0,/*GPS power on  dsp0 func*/
	[GPS_DL_OPID_PWR_ON_DL1] = gps_dl_opfunc_power_on_link1,/*GPS power on  dsp1 func*/
	[GPS_DL_OPID_PWR_OFF_DL0] = gps_dl_opfunc_power_off_link0,/*GPS power off  dsp0 func*/
	[GPS_DL_OPID_PWR_OFF_DL1] = gps_dl_opfunc_power_off_link1,/*GPS power off  dsp1 func*/
	[GPS_DL_OPID_SEND_DATA_DL0] = gps_dl_opfunc_send_data_link0,/*GPS send data to dsp0*/
	[GPS_DL_OPID_SEND_DATA_DL1] = gps_dl_opfunc_send_data_link1,/*GPS send data to dsp1*/
	[GPS_DL_OPID_LINK_EVENT_PROC] = gps_dl_opfunc_link_event_proc,/*GPS link event produce*/
	[GPS_DL_OPID_HAL_EVENT_PROC] = gps_dl_opfunc_hal_event_proc,/*GPS hal event produce*/
};

static int gps_dl_opfunc_hardware_control(struct gps_dl_osal_op_dat *pOpDat)
{
	/*Call hardware control function*/
	return 0;
}

static int gps_dl_opfunc_power_on_link0(struct gps_dl_osal_op_dat *pOpDat)
{
	gps_dl_rx_ctx_reset(0);
	/*Call gps data link hal send data function*/
	return 0;
}

static int gps_dl_opfunc_power_on_link1(struct gps_dl_osal_op_dat *pOpDat)
{
	gps_dl_rx_ctx_reset(1);
	/*Call gps data link hal send data function*/
	return 0;
}

static int gps_dl_opfunc_power_off_link0(struct gps_dl_osal_op_dat *pOpDat)
{
	/*Call gps data link hal send data function*/
	return 0;
}

static int gps_dl_opfunc_power_off_link1(struct gps_dl_osal_op_dat *pOpDat)
{
	/*Call gps data link hal send data function*/
	return 0;
}

static int gps_dl_opfunc_send_data_link0(struct gps_dl_osal_op_dat *pOpDat)
{
	unsigned char *buffer;
	unsigned int len;

	buffer = (char *)pOpDat->au4OpData[0];
	len = (unsigned int)pOpDat->au4OpData[1];

	/*Call gps data link hal send data function*/
#if GPS_DL_MOCK_HAL
	mock_output(buffer, len, 0);
#endif
	return 0;
}

static int gps_dl_opfunc_send_data_link1(struct gps_dl_osal_op_dat *pOpDat)
{
	unsigned char *buffer;
	unsigned int len;

	buffer = (char *)pOpDat->au4OpData[0];
	len = (unsigned int)pOpDat->au4OpData[1];

	/*Call gps data link hal send data function*/
#if GPS_DL_MOCK_HAL
	mock_output(buffer, len, 1);
#endif
	return 0;
}

static int gps_dl_opfunc_link_event_proc(struct gps_dl_osal_op_dat *pOpDat)
{
	enum gps_dl_link_event_id evt;
	enum gps_dl_link_id_enum link_id;

	link_id = (enum gps_dl_link_id_enum)pOpDat->au4OpData[0];
	evt = (enum gps_dl_link_event_id)pOpDat->au4OpData[1];
	gps_dl_link_event_proc(evt, link_id);

	return 0;
}

static int gps_dl_opfunc_hal_event_proc(struct gps_dl_osal_op_dat *pOpDat)
{
	enum gps_dl_hal_event_id evt;
	enum gps_dl_link_id_enum link_id;
	int sid_on_evt;

	link_id = (enum gps_dl_link_id_enum)pOpDat->au4OpData[0];
	evt = (enum gps_dl_hal_event_id)pOpDat->au4OpData[1];
	sid_on_evt = (int)pOpDat->au4OpData[2];
	gps_dl_hal_event_proc(evt, link_id, sid_on_evt);

	return 0;
}

unsigned int gps_dl_wait_event_checker(struct gps_dl_osal_thread *pThread)
{
	struct gps_dl_ctrld_context *pgps_dl_ctrld;

	if (pThread) {
		pgps_dl_ctrld = (struct gps_dl_ctrld_context *) (pThread->pThreadData);
		return !RB_EMPTY(&pgps_dl_ctrld->rOpQ);
	}
	GPS_DL_ERR_FUNC("pThread null\n");
	return 0;
}

unsigned int gps_dl_rx_thread_wait_event_checker(struct gps_dl_osal_thread *pThread)
{
	struct gps_dl_rx_context *p_RX = &gps_dl_ctrld.rRx_Ctx;

	if (pThread)
		return p_RX->flag;
	GPS_DL_ERR_FUNC("pThread null\n");
	return 0;
}

static int gps_dl_core_opid(struct gps_dl_osal_op_dat *pOpDat)
{
	int ret;

	if (pOpDat == NULL) {
		GPS_DL_ERR_FUNC("null operation data\n");
		/*print some message with error info */
		return -1;
	}

	if (pOpDat->opId >= GPS_DL_OPID_MAX) {
		GPS_DL_ERR_FUNC("Invalid OPID(%d)\n", pOpDat->opId);
		return -2;
	}

	if (gps_dl_core_opfunc[pOpDat->opId]) {
		GPS_DL_INFO_FUNC("GPS data link: operation id(%d)\n", pOpDat->opId);
		ret = (*(gps_dl_core_opfunc[pOpDat->opId])) (pOpDat);
		return ret;
	}

	GPS_DL_ERR_FUNC("GPS data link: null handler (%d)\n", pOpDat->opId);

	return -2;
}

static int gps_dl_put_op(struct gps_dl_osal_lxop_q *pOpQ, struct gps_dl_osal_lxop *pOp)
{
	int iRet;

	if (!pOpQ || !pOp) {
		GPS_DL_WARN_FUNC("invalid input param: pOpQ(0x%p), pLxOp(0x%p)\n", pOpQ, pOp);
		osal_assert(pOpQ);
		osal_assert(pOp);
		return -1;
	}

	iRet = osal_lock_sleepable_lock(&pOpQ->sLock);
	if (iRet) {
		GPS_DL_WARN_FUNC("osal_lock_sleepable_lock iRet(%d)\n", iRet);
		return -1;
	}

	/* acquire lock success */
	if (!RB_FULL(pOpQ))
		RB_PUT(pOpQ, pOp);
	else {
		GPS_DL_WARN_FUNC("RB_FULL(%p -> %p)\n", pOp, pOpQ);
		iRet = -1;
	}

	osal_unlock_sleepable_lock(&pOpQ->sLock);

	if (iRet)
		return -1;
	return 0;
}

int gps_dl_put_act_op(struct gps_dl_osal_lxop *pOp)
{
	struct gps_dl_ctrld_context *pgps_dl_ctrld = &gps_dl_ctrld;
	struct gps_dl_osal_signal *pSignal = NULL;
	int waitRet = -1;
	int bRet = 0;

	osal_assert(pgps_dl_ctrld);
	osal_assert(pOp);

	do {
		if (!pgps_dl_ctrld || !pOp) {
			GPS_DL_ERR_FUNC("pgps_dl_ctx(0x%p), pOp(0x%p)\n", pgps_dl_ctrld, pOp);
			break;
		}

		/* Init ref_count to 1 indicating that current thread holds a ref to it */
		atomic_set(&pOp->ref_count, 1);

		pSignal = &pOp->signal;
		if (pSignal->timeoutValue) {
			pOp->result = -9;
			osal_signal_init(pSignal);
		}

		/* Increment ref_count by 1 as gps control thread will hold a reference also,
		 * this must be done here instead of on target thread, because
		 * target thread might not be scheduled until a much later time,
		 * allowing current thread to decrement ref_count at the end of function,
		 * putting op back to free queue before target thread has a chance to process.
		 */
		atomic_inc(&pOp->ref_count);

		/* put to active Q */
		bRet = gps_dl_put_op(&pgps_dl_ctrld->rOpQ, pOp);
		if (bRet == -1) {
			GPS_DL_WARN_FUNC("put to active queue fail\n");
			atomic_dec(&pOp->ref_count);
			break;
		}

		/* wake up gps control thread */
		osal_trigger_event(&pgps_dl_ctrld->rgpsdlWq);

		if (pSignal->timeoutValue == 0) {
			bRet = -1;
			break;
		}

		/* check result */
		waitRet = osal_wait_for_signal_timeout(pSignal, &pgps_dl_ctrld->thread);

		if (waitRet == 0)
			GPS_DL_ERR_FUNC("opId(%d) completion timeout\n", pOp->op.opId);
		else if (pOp->result)
			GPS_DL_WARN_FUNC("opId(%d) result:%d\n", pOp->op.opId, pOp->result);

		/* op completes, check result */
		bRet = (pOp->result) ? -1 : 0;
	} while (0);

	if (pOp && atomic_dec_and_test(&pOp->ref_count)) {
		/* put Op back to freeQ */
		gps_dl_put_op(&pgps_dl_ctrld->rFreeOpQ, pOp);
	}

	return bRet;
}

struct gps_dl_osal_lxop *gps_dl_get_free_op(void)
{
	struct gps_dl_osal_lxop *pOp = NULL;
	struct gps_dl_ctrld_context *pgps_dl_ctrld = &gps_dl_ctrld;

	osal_assert(pgps_dl_ctrld);
	pOp = gps_dl_get_op(&pgps_dl_ctrld->rFreeOpQ);
	if (pOp)
		osal_memset(pOp, 0, sizeof(struct gps_dl_osal_lxop));
	return pOp;
}

static struct gps_dl_osal_lxop *gps_dl_get_op(struct gps_dl_osal_lxop_q *pOpQ)
{
	struct gps_dl_osal_lxop *pOp;
	int iRet;

	if (pOpQ == NULL) {
		GPS_DL_ERR_FUNC("pOpQ = NULL\n");
		osal_assert(pOpQ);
		return NULL;
	}

	iRet = osal_lock_sleepable_lock(&pOpQ->sLock);
	if (iRet) {
		GPS_DL_ERR_FUNC("osal_lock_sleepable_lock iRet(%d)\n", iRet);
		return NULL;
	}

	/* acquire lock success */
	RB_GET(pOpQ, pOp);
	osal_unlock_sleepable_lock(&pOpQ->sLock);

	if (pOp == NULL) {
		GPS_DL_WARN_FUNC("RB_GET(%p) return NULL\n", pOpQ);
		osal_assert(pOp);
		return NULL;
	}

	return pOp;
}

int gps_dl_put_op_to_free_queue(struct gps_dl_osal_lxop *pOp)
{
	struct gps_dl_ctrld_context *pgps_dl_ctrld = &gps_dl_ctrld;

	if (gps_dl_put_op(&pgps_dl_ctrld->rFreeOpQ, pOp) < 0)
		return -1;

	return 0;
}

int gps_dl_register_rx_event_cb(GPS_DL_RX_EVENT_CB func)
{
	struct gps_dl_rx_context *p_RX = &gps_dl_ctrld.rRx_Ctx;

	if (func != NULL) {
		p_RX->event_callback = func;
		return 0;
	}

	return -1;
}

int gps_dl_add_to_rx_queue(char *buffer, unsigned int length, unsigned int data_link_num)
{
#if 1
	unsigned int roomLeft, last_len;
	struct gps_dl_rx_context *p_RX = &gps_dl_ctrld.rRx_Ctx;

	if (data_link_num >= GPS_DATA_LINK_NUM) {
		GPS_DL_ERR_FUNC("gps_dl_receive_data:%d\n", data_link_num);
		return -1;
	}

	GPS_DL_INFO_FUNC("gps_dl_add_to_rx_queue:%p %d\n", buffer, length);

	osal_lock_unsleepable_lock(&p_RX->ring[data_link_num].mtx);

	if (p_RX->ring[data_link_num].read_p <= p_RX->ring[data_link_num].write_p)
		roomLeft = GPS_DL_RX_BUFFER_SIZE -
			p_RX->ring[data_link_num].write_p + p_RX->ring[data_link_num].read_p - 1;
	else
		roomLeft = p_RX->ring[data_link_num].read_p - p_RX->ring[data_link_num].write_p - 1;

	if (roomLeft < length) {
		osal_unlock_unsleepable_lock(&p_RX->ring[data_link_num].mtx);
		GPS_DL_ERR_FUNC("Queue full, remain buffer len(%d), data len(%d), w_p(%d), r_p(%d)\n",
			roomLeft, length, p_RX->ring[data_link_num].write_p, p_RX->ring[data_link_num].read_p);
		osal_assert(0);
		return -1;
	}

	if (length + p_RX->ring[data_link_num].write_p < GPS_DL_RX_BUFFER_SIZE) {
		osal_memcpy(p_RX->ring[data_link_num].buffer + p_RX->ring[data_link_num].write_p, buffer, length);
		p_RX->ring[data_link_num].write_p += length;
	} else {
		last_len = GPS_DL_RX_BUFFER_SIZE - p_RX->ring[data_link_num].write_p;
		osal_memcpy(p_RX->ring[data_link_num].buffer + p_RX->ring[data_link_num].write_p, buffer, last_len);
		osal_memcpy(p_RX->ring[data_link_num].buffer, buffer + last_len, length - last_len);
		p_RX->ring[data_link_num].write_p = length - last_len;
	}

	osal_unlock_unsleepable_lock(&p_RX->ring[data_link_num].mtx);
#endif

	#if GPS_DL_CTRLD_MOCK_LINK_LAYER
	p_RX->flag = 1;
	osal_trigger_event(&p_RX->rRxwq);
	#endif
	return 0;
}

int gps_dl_receive_data(char *buffer, unsigned int length, unsigned int data_link_num)
{
	unsigned int copyLen = 0;
	unsigned int tailLen = 0;
	struct gps_dl_rx_context *p_RX = &gps_dl_ctrld.rRx_Ctx;

	if (data_link_num >= GPS_DATA_LINK_NUM) {
		GPS_DL_ERR_FUNC("gps_dl_receive_data err:%d\n", data_link_num);
		return -1;
	}

	GPS_DL_INFO_FUNC("gps_dl_receive_data:%p %d\n", buffer, length);

	osal_lock_unsleepable_lock(&p_RX->ring[data_link_num].mtx);

	if (p_RX->ring[data_link_num].write_p > p_RX->ring[data_link_num].read_p) {
		copyLen = p_RX->ring[data_link_num].write_p - p_RX->ring[data_link_num].read_p;
		if (copyLen > length)
			copyLen = length;
		osal_memcpy(buffer, p_RX->ring[data_link_num].buffer + p_RX->ring[data_link_num].read_p, copyLen);
		p_RX->ring[data_link_num].read_p += copyLen;
	} else if (p_RX->ring[data_link_num].write_p < p_RX->ring[data_link_num].read_p) {
		tailLen = GPS_DL_RX_BUFFER_SIZE - p_RX->ring[data_link_num].read_p;
		if (tailLen > length) {	/* exclude equal case to skip wrap check */
			copyLen = length;
			osal_memcpy(buffer,
				p_RX->ring[data_link_num].buffer + p_RX->ring[data_link_num].read_p, copyLen);
			p_RX->ring[data_link_num].read_p += copyLen;
		} else {
			/* part 1: copy tailLen */
			osal_memcpy(buffer,
				p_RX->ring[data_link_num].buffer + p_RX->ring[data_link_num].read_p, tailLen);
			buffer += tailLen;	/* update buffer offset */
			/* part 2: check if head length is enough */
			copyLen = length - tailLen;
			copyLen = (p_RX->ring[data_link_num].write_p < copyLen) ?
				p_RX->ring[data_link_num].write_p : copyLen;
			if (copyLen)
				osal_memcpy(buffer, p_RX->ring[data_link_num].buffer, copyLen);
			/* Update read_p final position */
			p_RX->ring[data_link_num].read_p = copyLen;
			/* update return length: head + tail */
			copyLen += tailLen;
		}
	} else {
		/*ring buffer NULL*/
	}

	osal_unlock_unsleepable_lock(&p_RX->ring[data_link_num].mtx);
	return copyLen;
}

static int gps_dl_rx_thread(void *pdata)
{
	struct gps_dl_rx_context *p_RX = (struct gps_dl_rx_context *)pdata;
	int data_link_num;

	GPS_DL_INFO_FUNC("gps data link rx thread starts\n");

	while (1) {
		/*hardware irq buttom schedule will notify this completion*/
		osal_thread_wait_for_event(&p_RX->rx_thread, &p_RX->rRxwq, gps_dl_rx_thread_wait_event_checker);

		if (osal_thread_should_stop(&p_RX->rx_thread)) {
			/* Error Message*/
			GPS_DL_ERR_FUNC("osal_thread_should_stop\n");
			break;
		}

		/*copy data from DMA domain*/

		/*wake up read function to read data*/
		data_link_num = 0;
		if (p_RX->event_callback == NULL) {
			GPS_DL_ERR_FUNC("event_callback NULL\n");
			#if GPS_DL_CTRLD_MOCK_LINK_LAYER
			gps_dl_rx_event_cb(0);
			p_RX->flag = 0;
			#endif
		} else {
			(*(p_RX->event_callback))(data_link_num);
			p_RX->flag = 0;
		}
	}
	return 0;
}

static int gps_dl_ctrl_thread(void *pData)
{
	struct gps_dl_ctrld_context *pgps_dl_ctrld = (struct gps_dl_ctrld_context *) pData;
	struct gps_dl_osal_event *pEvent = NULL;
	struct gps_dl_osal_lxop *pOp;
	int iResult;

	if (pgps_dl_ctrld == NULL) {
		GPS_DL_ERR_FUNC("pgps_dl_ctx is NULL\n");
		return -1;
	}

	GPS_DL_INFO_FUNC("gps control thread starts\n");

	pEvent = &(pgps_dl_ctrld->rgpsdlWq);

	for (;;) {
		pOp = NULL;
		pEvent->timeoutValue = 0;

		osal_thread_wait_for_event(&pgps_dl_ctrld->thread, pEvent, gps_dl_wait_event_checker);

		if (osal_thread_should_stop(&pgps_dl_ctrld->thread)) {
			GPS_DL_INFO_FUNC(" thread should stop now...\n");
			/* TODO: clean up active opQ */
			break;
		}

		/* get Op from Queue */
		pOp = gps_dl_get_op(&pgps_dl_ctrld->rOpQ);
		if (!pOp) {
			GPS_DL_WARN_FUNC("get_lxop activeQ fail\n");
			continue;
		}

		/*Execute operation*/
		iResult = gps_dl_core_opid(&pOp->op);

		if (atomic_dec_and_test(&pOp->ref_count))
			gps_dl_put_op(&pgps_dl_ctrld->rFreeOpQ, pOp);
		else if (osal_op_is_wait_for_signal(pOp))
			osal_op_raise_signal(pOp, iResult);

		if (iResult)
			GPS_DL_WARN_FUNC("opid (0x%x) failed, iRet(%d)\n", pOp->op.opId, iResult);

	}

	GPS_DL_INFO_FUNC("gps control thread exits succeed\n");

	return 0;
}

void gps_dl_rx_ctx_reset(int data_link_num)
{
	struct gps_dl_rx_context *p_RX = &gps_dl_ctrld.rRx_Ctx;

	p_RX->ring[data_link_num].read_p = 0;
	p_RX->ring[data_link_num].write_p = 0;
}

int gps_dl_ctrld_init(void)
{
	struct gps_dl_ctrld_context *pgps_dl_ctrld;
	struct gps_dl_osal_thread *pThread;
	struct gps_dl_osal_thread *pRXThread;
	int iRet;
	int i;

	pgps_dl_ctrld = &gps_dl_ctrld;
	osal_memset(&gps_dl_ctrld, 0, sizeof(gps_dl_ctrld));

	/* Create gps data link control thread */
	pThread = &gps_dl_ctrld.thread;
	osal_strncpy(pThread->threadName, "gps_kctrld", sizeof(pThread->threadName));
	pThread->pThreadData = (void *)pgps_dl_ctrld;
	pThread->pThreadFunc = (void *)gps_dl_ctrl_thread;

	iRet = osal_thread_create(pThread);
	if (iRet) {
		/*Error print*/
		GPS_DL_ERR_FUNC("Create gps data link control thread fail:%d\n", iRet);
		return -1;
	}

	/* Create gps data link rx thread */
	pRXThread = &gps_dl_ctrld.rRx_Ctx.rx_thread;
	osal_strncpy(pRXThread->threadName, "mtk_gps_dl_rx", sizeof(pRXThread->threadName));
	pRXThread->pThreadData = (void *)(&(pgps_dl_ctrld->rRx_Ctx));
	pRXThread->pThreadFunc = (void *)gps_dl_rx_thread;

	iRet = osal_thread_create(pRXThread);
	if (iRet) {
		/*Error print*/
		GPS_DL_ERR_FUNC("Create gps data link RX thread fail:%d\n", iRet);
		return -1;
	}

	/* Initialize gps control Thread Information: Thread */
	osal_event_init(&pgps_dl_ctrld->rgpsdlWq);
	osal_sleepable_lock_init(&pgps_dl_ctrld->rOpQ.sLock);
	osal_sleepable_lock_init(&pgps_dl_ctrld->rFreeOpQ.sLock);
	/* Initialize op queue */
	RB_INIT(&pgps_dl_ctrld->rOpQ, GPS_DL_OP_BUF_SIZE);
	RB_INIT(&pgps_dl_ctrld->rFreeOpQ, GPS_DL_OP_BUF_SIZE);

	/* Put all to free Q */
	for (i = 0; i < GPS_DL_OP_BUF_SIZE; i++) {
		osal_signal_init(&(pgps_dl_ctrld->arQue[i].signal));
		gps_dl_put_op(&pgps_dl_ctrld->rFreeOpQ, &(pgps_dl_ctrld->arQue[i]));
	}

	/*Init rx wait queue, mutex*/
	osal_event_init(&pgps_dl_ctrld->rRx_Ctx.rRxwq);
	for (i = 0; i < GPS_DATA_LINK_NUM; i++)
		osal_unsleepable_lock_init(&pgps_dl_ctrld->rRx_Ctx.ring[i].mtx);

	/*Init rx buffer*/
	gps_dl_rx_ctx_reset(0);
	gps_dl_rx_ctx_reset(1);

	iRet = osal_thread_run(pThread);
	if (iRet) {
		/*Error print*/
		GPS_DL_ERR_FUNC("gps data link ontrol thread run fail:%d\n", iRet);
		return -1;
	}

	iRet = osal_thread_run(pRXThread);
	if (iRet) {
		/*Error print*/
		GPS_DL_ERR_FUNC("gps data link RX thread run fail:%d\n", iRet);
		return -1;
	}

	return 0;
}

int gps_dl_ctrld_deinit(void)
{
	struct gps_dl_osal_thread *pThread;
	struct gps_dl_osal_thread *pRXThread;
	int iRet;

	pThread = &gps_dl_ctrld.thread;

	iRet = osal_thread_stop(pThread);
	if (iRet)
		GPS_DL_ERR_FUNC("gps data link ontrol thread stop fail:%d\n", iRet);
	else
		GPS_DL_INFO_FUNC("gps data link ontrol thread stop success:%d\n", iRet);

	osal_event_deinit(&gps_dl_ctrld.rgpsdlWq);

	pRXThread = &gps_dl_ctrld.rRx_Ctx.rx_thread;
	iRet = osal_thread_stop(pRXThread);
	if (iRet) {
		GPS_DL_ERR_FUNC("gps data link RX thread stop fail:%d\n", iRet);
		return -1;
	}
	GPS_DL_INFO_FUNC("gps data link RX thread stop success:%d\n", iRet);

	osal_event_deinit(&gps_dl_ctrld.rRx_Ctx.rRxwq);

	iRet = osal_thread_destroy(pThread);
	if (iRet) {
		GPS_DL_ERR_FUNC("gps data link ontrol thread destroy fail:%d\n", iRet);
		return -1;
	}
	GPS_DL_INFO_FUNC("gps data link ontrol thread destroy success:%d\n", iRet);

	iRet = osal_thread_destroy(pRXThread);
	if (iRet) {
		GPS_DL_ERR_FUNC("gps data link RX thread destroy fail:%d\n", iRet);
		return -1;
	}
	GPS_DL_INFO_FUNC("gps data link RX thread destroy success:%d\n", iRet);
	return 0;
}
