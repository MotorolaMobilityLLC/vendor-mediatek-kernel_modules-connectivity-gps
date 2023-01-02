/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#include "gps_mcudl_log.h"
#include "gps_mcudl_data_pkt_host_api.h"
#include "gps_mcudl_plat_api.h"
#include "gps_dl_dma_buf.h"
#include "gps_each_link.h"
#include "gps_mcudl_context.h"
#include "gps_mcudl_each_link.h"
#include "gps_mcudl_link_state.h"
#include "gps_dl_osal.h"
#include "gps_mcudl_data_pkt_payload_struct.h"
#include "gps_mcudl_ylink.h"
#include "gps_mcusys_data_api.h"


struct geofence_pkt_host_sta_s {
	struct gps_mcudl_data_pkt_mcu_sta pkt_sta;
	gpsmdl_u64 last_ack_recv_len;
	bool reset_flag;
	bool is_enable;
};


#define RBUF_MAX (10*1024)
#define ENTR_MAX (64)
gpsmdl_u8              pars_rbuf[GPS_MDLY_CH_NUM][RBUF_MAX];
gpsmdl_u8              slot_rbuf[GPS_MDLY_CH_NUM][RBUF_MAX];
struct gps_mcudl_slot_entry_t entr_list[GPS_MDLY_CH_NUM][ENTR_MAX];


#define TX_BUF_MAX (GPSMDL_PKT_PAYLOAD_MAX)
struct gps_mcudl_data_trx_context {
	struct gps_mcudl_data_pkt_parser_t parser;
	struct gps_mcudl_data_rbuf_plus_t rx_rbuf;
	struct gps_mcudl_data_slot_t slot;
	gpsmdl_u8  tx_buf[TX_BUF_MAX];
	gpsmdl_u32 tx_len;
	struct gps_dl_osal_unsleepable_lock spin_lock;
	struct geofence_pkt_host_sta_s host_sta;
	gpsmdl_u64 local_flush_times;
	gpsmdl_u64 local_tx_len;
	gpsmdl_u64 remote_rx_len;
	bool wait_read_to_proc_flag;
	bool wait_write_to_flush_flag;
	bool wait_sta_to_flush_flag;
};


struct gps_mcudl_data_pkt_context {
	struct gps_mcudl_data_trx_context trx[GPS_MDLY_CH_NUM];
};


struct gps_mcudl_data_pkt_context g_data_pkt_ctx = { .trx = {
	[GPS_MDLY_NORMAL] = {
		.rx_rbuf = { .cfg = {
			.rbuf_ptr = &pars_rbuf[GPS_MDLY_NORMAL][0],
			.rbuf_len = RBUF_MAX,
		}, },
		.parser = { .cfg = {
			.p_pkt_proc_fn = &proc_func1,
			.rbuf_ptr = &pars_rbuf[GPS_MDLY_NORMAL][0],
			.rbuf_len = RBUF_MAX,
		}, },
		.slot = { .cfg = {
			.slot_id = GPS_MDLY_NORMAL,
			.p_intf_send_fn = &send_func1,

			.rbuf_ptr = &slot_rbuf[GPS_MDLY_NORMAL][0],
			.rbuf_len = RBUF_MAX,

			.entry_list_ptr = &entr_list[GPS_MDLY_NORMAL][0],
			.entry_list_len = ENTR_MAX,
		}, },
	},
}, };

struct gps_mcudl_data_trx_context *get_txrx_ctx(enum gps_mcudl_yid yid)
{
	return &g_data_pkt_ctx.trx[yid];
}

bool gps_mcudl_ap2mcu_xdata_send(enum gps_mcudl_xid xid, struct gdl_dma_buf_entry *p_entry)
{
	struct gps_mcudl_data_trx_context *p_trx_ctx;
	enum GDL_RET_STATUS status;
	gpsmdl_u32 data_len = 0;
	enum gps_mcudl_pkt_type type;
	enum gps_mcudl_yid yid;
	bool xid_is_valid;
	bool send_is_okay;

	yid = GPS_MDL_X2Y(xid);
	xid_is_valid = gps_mcudl_xid2ypl_type(xid, &type);
	if (xid_is_valid)
		return false;

	p_trx_ctx = get_txrx_ctx(yid);
	status = gdl_dma_buf_entry_to_buf(p_entry,
		&p_trx_ctx->tx_buf[0], TX_BUF_MAX, &data_len);

	send_is_okay = false;
	if (status == GDL_OKAY)
		send_is_okay = gps_mcudl_ap2mcu_ydata_send(yid, type, &p_trx_ctx->tx_buf[0], data_len);

	MDL_LOGXW(xid, "x_send get=%s, len=%d, ok=%d",
		gdl_ret_to_name(status), data_len, send_is_okay);
	return send_is_okay;
}

bool gps_mcudl_ap2mcu_xdata_send_v2(enum gps_mcudl_xid x_id,
	const gpsmdl_u8 *p_data, gpsmdl_u32 data_len, bool *p_to_notify)
{
	enum gps_mcudl_yid y_id;
	enum gps_mcudl_pkt_type type;
	bool xid_is_valid;
	bool send_is_okay;
	bool to_notify;

	y_id = GPS_MDL_X2Y(x_id);
	xid_is_valid = gps_mcudl_xid2ypl_type(x_id, &type);
	if (!xid_is_valid)
		return false;

	send_is_okay = gps_mcudl_ap2mcu_ydata_send(y_id, type, p_data, data_len);
	if (send_is_okay)
		to_notify = !gps_mcudl_ap2mcu_get_wait_write_flag(y_id);
	else
		to_notify = false;

	if (p_to_notify)
		*p_to_notify = to_notify;

	if (to_notify)
		gps_mcudl_ap2mcu_set_wait_write_flag(y_id, true);

	MDL_LOGXD(x_id, "x_send len=%d, ok=%d, ntf=%d", data_len, send_is_okay, to_notify);
	return send_is_okay;
}

void gps_mcudl_ap2mcu_context_init(enum gps_mcudl_yid yid)
{
	struct gps_mcudl_data_trx_context *p_trx_ctx;

	gps_mcudl_slot_init(&g_data_pkt_ctx.trx[yid].slot);
	gps_mcudl_data_pkt_parser_init(&g_data_pkt_ctx.trx[yid].parser);
	gps_mcudl_data_rbuf_init(&g_data_pkt_ctx.trx[yid].rx_rbuf);
	gps_dl_osal_unsleepable_lock_init(&g_data_pkt_ctx.trx[yid].spin_lock);

	p_trx_ctx = &g_data_pkt_ctx.trx[yid];
	memset(&p_trx_ctx->host_sta, 0, sizeof(p_trx_ctx->host_sta));
	p_trx_ctx->host_sta.is_enable = true;
	gps_mcudl_flowctrl_init();
	p_trx_ctx->wait_read_to_proc_flag = false;
	p_trx_ctx->wait_write_to_flush_flag = false;
	p_trx_ctx->wait_sta_to_flush_flag = false;
}

bool gps_mcudl_ap2mcu_ydata_send(enum gps_mcudl_yid yid,
	enum gps_mcudl_pkt_type type, const gpsmdl_u8 *p_data, gpsmdl_u32 data_len)
{
	struct gps_mcudl_data_trx_context *p_trx_ctx;

	p_trx_ctx = get_txrx_ctx(yid);
	return gps_mcudl_pkt_send(&p_trx_ctx->slot, type, p_data, data_len);
}

void gps_mcudl_mcu2ap_ydata_recv(enum gps_mcudl_yid yid,
	const gpsmdl_u8 *p_data, gpsmdl_u32 data_len)
{
	struct gps_mcudl_data_trx_context *p_trx_ctx;
	struct gps_mcudl_data_rbuf_plus_t *p_rbuf;

	p_trx_ctx = get_txrx_ctx(yid);
	p_rbuf = &p_trx_ctx->rx_rbuf;

	/* writer */
	gps_mcudl_data_rbuf_put(p_rbuf, p_data, data_len);
	p_trx_ctx->host_sta.pkt_sta.total_recv += (gpsmdl_u64)data_len;

	/* send msg to call gps_mcudl_ap2mcu_ydata_proc */
	gps_mcudl_mcu2ap_ydata_notify(yid);
}

void gps_mcudl_mcu2ap_ydata_notify(enum gps_mcudl_yid y_id)
{
	bool to_notify = true;

	to_notify = !gps_mcudl_ap2mcu_get_wait_read_flag(y_id);
	if (to_notify) {
		gps_mcudl_ap2mcu_set_wait_read_flag(y_id, true);
		gps_mcudl_ylink_event_send(y_id, GPS_MCUDL_YLINK_EVT_ID_RX_DATA_READY);
	}
	MDL_LOGYD(y_id, "ntf=%d", to_notify);
}

void gps_mcudl_mcu2ap_ydata_proc(enum gps_mcudl_yid yid)
{
	struct gps_mcudl_data_trx_context *p_trx_ctx;
	struct gps_mcudl_data_rbuf_plus_t *p_rbuf;
	struct gps_mcudl_data_pkt_parser_t *p_parser;
	gpsmdl_u32 reader_write_idx, reader_read_idx;

	p_trx_ctx = get_txrx_ctx(yid);
	p_rbuf = &p_trx_ctx->rx_rbuf;
	p_parser = &p_trx_ctx->parser;

	gps_mcudl_ap2mcu_set_wait_read_flag(yid, false);

	/* reader */
	gps_mcudl_data_rbuf_reader_sync_write_idx(p_rbuf);
	reader_write_idx = p_rbuf->cursor.rwi;
	gps_mcudl_data_pkt_parse(p_parser, reader_write_idx);

	reader_read_idx = p_parser->read_idx;
	p_rbuf->cursor.rri = reader_read_idx;
	gps_mcudl_data_rbuf_reader_update_read_idx(p_rbuf);

	MDL_LOGYD(yid, "reader: w=%d, r=%d", reader_write_idx, reader_read_idx);

	p_trx_ctx->host_sta.pkt_sta.total_parse_proc = (gpsmdl_u32)(p_parser->proc_byte_cnt);
	p_trx_ctx->host_sta.pkt_sta.total_parse_drop = (gpsmdl_u32)(p_parser->drop_byte_cnt);
	p_trx_ctx->host_sta.pkt_sta.total_route_drop = 0;
	p_trx_ctx->host_sta.pkt_sta.total_pkt_cnt    = (gpsmdl_u32)(p_parser->pkt_cnt);
	p_trx_ctx->host_sta.pkt_sta.LUINT_L32_VALID_BIT = 32;
	gps_mcudl_flowctrl_may_send_host_sta(yid);
}

void proc_func1(enum gps_mcudl_pkt_type type,
	const gpsmdl_u8 *payload_ptr, gpsmdl_u16 payload_len)
{
	enum gps_mcudl_yid y_id;
	struct gps_mcudl_data_trx_context *p_trx_ctx;

	y_id = GPS_MDLY_NORMAL;
	p_trx_ctx = &g_data_pkt_ctx.trx[y_id];

	MDL_LOGYD(y_id, "recv type=%d, len=%d", type, payload_len);
	switch (type) {
	case GFNS_RSP_MCU_RST_UP_PKT_STA:
		MDL_LOGYI(y_id, "recv type=%d: GFNS_RSP_MCU_RST_UP_PKT_STA", type);
		memset(&p_trx_ctx->host_sta, 0, sizeof(p_trx_ctx->host_sta));
		p_trx_ctx->host_sta.is_enable = true;
		p_trx_ctx->host_sta.reset_flag = true;
		return;

	case GFNS_RSP_MCU_ACK_DN_PKT_STA: {
		struct gps_mcudl_data_pkt_mcu_sta sta;
		int copy_size;
		bool to_notify = true;

		MDL_LOGYD(y_id, "recv type=%d, len=%d, expected_len=%d",
			type, payload_len, sizeof(sta));

		/* TODO: struct size check */
		copy_size = sizeof(sta);
		if (copy_size > payload_len)
			copy_size = payload_len;
		memcpy(&sta, payload_ptr, copy_size);
		gps_mcudl_flowctrl_remote_update_recv_byte(&sta);
		to_notify = !gps_mcudl_ap2mcu_get_wait_flush_flag(y_id);
		MDL_LOGYI(y_id, "recv_sta: %llu, %u, %u, %u, %u, 0x%x, ntf=%d",
			sta.total_recv,
			sta.total_parse_proc,
			sta.total_parse_drop,
			sta.total_route_drop,
			sta.total_pkt_cnt,
			sta.LUINT_L32_VALID_BIT,
			to_notify);
		if (to_notify) {
			gps_mcudl_ap2mcu_set_wait_flush_flag(y_id, true);
			gps_mcudl_ylink_event_send(y_id,
				GPS_MCUDL_YLINK_EVT_ID_SLOT_FLUSH_ON_RECV_STA);
		}
		return;
	}
	case GPS_MDLYPL_MCUSYS:
		gps_mcusys_data_frame_proc(payload_ptr, payload_len);
		break;

	default:
		gps_mcudl_mcu2ap_try_to_wakeup_xlink_reader(y_id, type, payload_ptr, payload_len);
		break;
	}
}

int send_func1(const gpsmdl_u8 *p_data, gpsmdl_u32 data_len)
{
	int send_len;

	send_len = gps_mcudl_stpgps1_write(p_data, data_len);
	MDL_LOGYW(GPS_MDLY_NORMAL, "send len=%d, ret=%d", data_len, send_len);
	return send_len;
}

void gps_mcudl_ap2mcu_data_slot_flush_on_xwrite(enum gps_mcudl_xid x_id)
{
	enum gps_mcudl_yid y_id;
	struct gps_mcudl_data_trx_context *p_trx_ctx;
	enum gps_mcudl_slot_flush_status flush_status;
	gpsmdl_u32 flush_done_len = 0;

	y_id = GPS_MDL_X2Y(x_id);
	p_trx_ctx = get_txrx_ctx(y_id);

	gps_mcudl_ap2mcu_set_wait_write_flag(y_id, false);
	flush_status = gps_mcudl_slot_flush(&p_trx_ctx->slot, &flush_done_len);
	if (flush_done_len > 0)
		gps_mcudl_ap2mcu_try_to_wakeup_xlink_writer(y_id);

	MDL_LOGYW(y_id, "flush: ret=%d, len=%d, x_id=%d", flush_status, flush_done_len, x_id);
}

void gps_mcudl_ap2mcu_data_slot_flush_on_recv_sta(enum gps_mcudl_yid y_id)
{
	struct gps_mcudl_data_trx_context *p_trx_ctx;
	enum gps_mcudl_slot_flush_status flush_status;
	gpsmdl_u32 flush_done_len = 0;

	gps_mcudl_ap2mcu_set_wait_flush_flag(y_id, false);
	p_trx_ctx = &g_data_pkt_ctx.trx[y_id];
	flush_status = gps_mcudl_slot_flush(&p_trx_ctx->slot, &flush_done_len);
	if (flush_done_len > 0)
		gps_mcudl_ap2mcu_try_to_wakeup_xlink_writer(y_id);

	MDL_LOGYD(y_id, "flush: ret=%d, len=%d", flush_status, flush_done_len);
}

void gps_mcudl_slot_protect(void)
{
	enum gps_mcudl_yid y_id;

	y_id = GPS_MDLY_NORMAL;
	gps_dl_osal_lock_unsleepable_lock(&g_data_pkt_ctx.trx[y_id].spin_lock);
}

void gps_mcudl_slot_unprotect(void)
{
	enum gps_mcudl_yid y_id;

	y_id = GPS_MDLY_NORMAL;
	gps_dl_osal_unlock_unsleepable_lock(&g_data_pkt_ctx.trx[y_id].spin_lock);
}

bool gps_mcudl_pkt_is_critical_type(gpsmdl_u8 type)
{
	enum gps_mcudl_pkt_type pkt_type;

	pkt_type = (enum gps_mcudl_pkt_type)type;
	switch (pkt_type) {
	case GPS_MDLYPL_MCUSYS:
	case GPS_MDLYPL_MCUFN:
	case GPS_MDLYPL_MNL:
	case GPS_MDLYPL_AGENT:
	case GPS_MDLYPL_NMEA:
	case GPS_MDLYPL_GDLOG:
	case GPS_MDLYPL_PMTK:
	case GPS_MDLYPL_MEAS:
	case GPS_MDLYPL_CORR:
	case GPS_MDLYPL_DSP0:
	case GPS_MDLYPL_DSP1:
		return false;

	default:
		break;
	}

	return true;
}

void gps_mcudl_flowctrl_init(void)
{
	enum gps_mcudl_yid y_id;
	struct gps_mcudl_data_trx_context *p_trx_ctx;

	y_id = GPS_MDLY_NORMAL;
	p_trx_ctx = get_txrx_ctx(y_id);
	p_trx_ctx->local_flush_times = 0;
	p_trx_ctx->local_tx_len = 0;
	p_trx_ctx->remote_rx_len = 0;
}

void gps_mcudl_flowctrl_local_add_send_byte(gpsmdl_u32 delta)
{
	enum gps_mcudl_yid y_id;
	struct gps_mcudl_data_trx_context *p_trx_ctx;

	y_id = GPS_MDLY_NORMAL;
	p_trx_ctx = get_txrx_ctx(y_id);
	p_trx_ctx->local_flush_times++;
	p_trx_ctx->local_tx_len += delta;
}

void gps_mcudl_flowctrl_remote_update_recv_byte(struct gps_mcudl_data_pkt_mcu_sta *p_sta)
{
	enum gps_mcudl_yid y_id;
	struct gps_mcudl_data_trx_context *p_trx_ctx;

	y_id = GPS_MDLY_NORMAL;
	p_trx_ctx = get_txrx_ctx(y_id);
	p_trx_ctx->remote_rx_len = p_sta->total_recv;
}

#define CONN_RECV_BUF_MAX (3000)
gpsmdl_u32 gps_mcudl_flowctrl_cal_window_size(void)
{
	gpsmdl_u32 win_size;
	enum gps_mcudl_yid y_id;
	struct gps_mcudl_data_trx_context *p_trx_ctx;

	y_id = GPS_MDLY_NORMAL;
	p_trx_ctx = get_txrx_ctx(y_id);

	if (p_trx_ctx->local_tx_len >= p_trx_ctx->remote_rx_len + CONN_RECV_BUF_MAX)
		win_size = 0;
	else
		win_size = CONN_RECV_BUF_MAX -
			(gpsmdl_u32)(p_trx_ctx->local_tx_len - p_trx_ctx->remote_rx_len);

	MDL_LOGYD(y_id, "cal_win=%u, local_tx=%llu, remote_rx=%llu", win_size,
		p_trx_ctx->local_tx_len, p_trx_ctx->remote_rx_len);
	return win_size;
}

#define GEOFENCE_PKT_HOST_ACK_LEN (2*1024)
void gps_mcudl_flowctrl_may_send_host_sta(enum gps_mcudl_yid yid)
{
	struct gps_mcudl_data_trx_context *p_trx_ctx;
	gpsmdl_u64 not_ack_len;
	bool to_notify = false;
	bool old_reset_flag = false;

	p_trx_ctx = get_txrx_ctx(yid);
	not_ack_len = p_trx_ctx->host_sta.pkt_sta.total_recv - p_trx_ctx->host_sta.last_ack_recv_len;

	/* ack the length of host already received for two case:*/
	/* 1. ack the total length host received every n (KB)*/
	/* 2. ack if connsys need to reset host statistic, all of the sta value should be set to zero.*/
	/*	  connsys will block sending until host ack of this reset cmd back.*/
	if ((not_ack_len >= GEOFENCE_PKT_HOST_ACK_LEN) ||
		(p_trx_ctx->host_sta.reset_flag)) {
		old_reset_flag = p_trx_ctx->host_sta.reset_flag;
		if (p_trx_ctx->host_sta.is_enable) {
			gps_mcudl_ap2mcu_ydata_send(yid, GFNS_REQ_MCU_ACK_UP_PKT_STA,
				(gpsmdl_u8 *)&(p_trx_ctx->host_sta.pkt_sta),
				sizeof(p_trx_ctx->host_sta.pkt_sta));
			p_trx_ctx->host_sta.reset_flag = 0;
		}

		to_notify = !gps_mcudl_ap2mcu_get_wait_flush_flag(yid);
		MDL_LOGYI(yid,
			"send_ack:recv=%u,last=%u,proc=%u,pkt=%u,pdrop=%u,rdrop=%u,en=%u,rst=%u,nack=%d,ntf=%d",
			(gpsmdl_u32)(p_trx_ctx->host_sta.pkt_sta.total_recv),
			(gpsmdl_u32)(p_trx_ctx->host_sta.last_ack_recv_len),
			p_trx_ctx->host_sta.pkt_sta.total_parse_proc,
			p_trx_ctx->host_sta.pkt_sta.total_pkt_cnt,
			p_trx_ctx->host_sta.pkt_sta.total_parse_drop,
			p_trx_ctx->host_sta.pkt_sta.total_route_drop,
			p_trx_ctx->host_sta.is_enable,
			old_reset_flag,
			not_ack_len,
			to_notify);

		if (p_trx_ctx->host_sta.is_enable && to_notify) {
			gps_mcudl_ap2mcu_set_wait_flush_flag(yid, true);
			gps_mcudl_ylink_event_send(yid,
				GPS_MCUDL_YLINK_EVT_ID_SLOT_FLUSH_ON_RECV_STA);
		}

		p_trx_ctx->host_sta.last_ack_recv_len = p_trx_ctx->host_sta.pkt_sta.total_recv;
	}
}

bool gps_mcudl_ap2mcu_get_wait_read_flag(enum gps_mcudl_yid y_id)
{
	struct gps_mcudl_data_trx_context *p_trx_ctx;
	bool flag;

	p_trx_ctx = &g_data_pkt_ctx.trx[y_id];
	gps_mcudl_slot_protect();
	flag = p_trx_ctx->wait_read_to_proc_flag;
	gps_mcudl_slot_unprotect();
	return flag;
}

void gps_mcudl_ap2mcu_set_wait_read_flag(enum gps_mcudl_yid y_id, bool flag)
{
	struct gps_mcudl_data_trx_context *p_trx_ctx;

	p_trx_ctx = &g_data_pkt_ctx.trx[y_id];
	gps_mcudl_slot_protect();
	p_trx_ctx->wait_read_to_proc_flag = flag;
	gps_mcudl_slot_unprotect();
}

bool gps_mcudl_ap2mcu_get_wait_write_flag(enum gps_mcudl_yid y_id)
{
	struct gps_mcudl_data_trx_context *p_trx_ctx;
	bool flag;

	p_trx_ctx = &g_data_pkt_ctx.trx[y_id];
	gps_mcudl_slot_protect();
	flag = p_trx_ctx->wait_write_to_flush_flag;
	gps_mcudl_slot_unprotect();
	return flag;
}

void gps_mcudl_ap2mcu_set_wait_write_flag(enum gps_mcudl_yid y_id, bool flag)
{
	struct gps_mcudl_data_trx_context *p_trx_ctx;

	p_trx_ctx = &g_data_pkt_ctx.trx[y_id];
	gps_mcudl_slot_protect();
	p_trx_ctx->wait_write_to_flush_flag = flag;
	gps_mcudl_slot_unprotect();
}

bool gps_mcudl_ap2mcu_get_wait_flush_flag(enum gps_mcudl_yid y_id)
{
	struct gps_mcudl_data_trx_context *p_trx_ctx;
	bool flag;

	p_trx_ctx = &g_data_pkt_ctx.trx[y_id];
	gps_mcudl_slot_protect();
	flag = p_trx_ctx->wait_sta_to_flush_flag;
	gps_mcudl_slot_unprotect();
	return flag;
}

void gps_mcudl_ap2mcu_set_wait_flush_flag(enum gps_mcudl_yid y_id, bool flag)
{
	struct gps_mcudl_data_trx_context *p_trx_ctx;

	p_trx_ctx = &g_data_pkt_ctx.trx[y_id];
	gps_mcudl_slot_protect();
	p_trx_ctx->wait_sta_to_flush_flag = flag;
	gps_mcudl_slot_unprotect();
}


void gps_mcudl_ap2mcu_try_to_wakeup_xlink_writer(enum gps_mcudl_yid y_id)
{
	enum gps_mcudl_xid x_id;
	struct gps_mcudl_each_link *p_xlink;

	MDL_LOGYD(y_id, "");
	for (x_id = GPS_MDLX_MCUSYS; x_id < GPS_MDLX_CH_NUM; x_id++) {
		p_xlink = gps_mcudl_link_get(x_id);
		gps_dl_link_wake_up(&p_xlink->waitables[GPS_DL_WAIT_WRITE]);
	}
}


bool gps_mcudl_xid2ypl_type(enum gps_mcudl_xid x_id, enum gps_mcudl_pkt_type *p_type)
{
	enum gps_mcudl_pkt_type type;
	bool is_okay = true;

	switch (x_id) {
	case GPS_MDLX_MCUSYS:
		type = GPS_MDLYPL_MCUSYS;
		break;
	case GPS_MDLX_MCUFN:
		type = GPS_MDLYPL_MCUFN;
		break;
	case GPS_MDLX_MNL:
		type = GPS_MDLYPL_MNL;
		break;
	case GPS_MDLX_AGENT:
		type = GPS_MDLYPL_AGENT;
		break;
	case GPS_MDLX_NMEA:
		type = GPS_MDLYPL_NMEA;
		break;
	case GPS_MDLX_GDLOG:
		type = GPS_MDLYPL_GDLOG;
		break;
	case GPS_MDLX_PMTK:
		type = GPS_MDLYPL_PMTK;
		break;
	case GPS_MDLX_MEAS:
		type = GPS_MDLYPL_MEAS;
		break;
	case GPS_MDLX_CORR:
		type = GPS_MDLYPL_CORR;
		break;
	case GPS_MDLX_DSP0:
		type = GPS_MDLYPL_DSP0;
		break;
	case GPS_MDLX_DSP1:
		type = GPS_MDLYPL_DSP1;
		break;

	case GPS_MDLX_AOL_TEST:
		type = GPS_MDLYPL_AOLTS;
		break;
	case GPS_MDLX_MPE_TEST:
		type = GPS_MDLYPL_MPETS;
		break;
	case GPS_MDLX_SCP_TEST:
		type = GPS_MDLYPL_SCPTS;
		break;

	case GPS_MDLX_LPPM:
		type = GPS_MDLYPL_LPPM;
		break;
	case GPS_MDLX_MPELOG:
		type = GPS_MDLYPL_MPELOG;
		break;

	default:
		is_okay = false;
		return is_okay;
	}

	is_okay = true;
	if (p_type)
		*p_type = type;
	return is_okay;
}

bool gps_mcudl_ypl_type2xid(enum gps_mcudl_pkt_type type, enum gps_mcudl_xid *p_xid)
{
	enum gps_mcudl_xid x_id;
	bool is_okay = true;

	switch (type) {
	case GPS_MDLYPL_MCUSYS:
		x_id = GPS_MDLX_MCUSYS;
		break;
	case GPS_MDLYPL_MCUFN:
		x_id = GPS_MDLX_MCUFN;
		break;
	case GPS_MDLYPL_MNL:
		x_id = GPS_MDLX_MNL;
		break;
	case GPS_MDLYPL_AGENT:
		x_id = GPS_MDLX_AGENT;
		break;
	case GPS_MDLYPL_NMEA:
		x_id = GPS_MDLX_NMEA;
		break;
	case GPS_MDLYPL_GDLOG:
		x_id = GPS_MDLX_GDLOG;
		break;
	case GPS_MDLYPL_PMTK:
		x_id = GPS_MDLX_PMTK;
		break;
	case GPS_MDLYPL_MEAS:
		x_id = GPS_MDLX_MEAS;
		break;
	case GPS_MDLYPL_CORR:
		x_id = GPS_MDLX_CORR;
		break;
	case GPS_MDLYPL_DSP0:
		x_id = GPS_MDLX_DSP0;
		break;
	case GPS_MDLYPL_DSP1:
		x_id = GPS_MDLX_DSP1;
		break;

	case GPS_MDLYPL_AOLTS:
		x_id = GPS_MDLX_AOL_TEST;
		break;
	case GPS_MDLYPL_MPETS:
		x_id = GPS_MDLX_MPE_TEST;
		break;
	case GPS_MDLYPL_SCPTS:
		x_id = GPS_MDLX_SCP_TEST;
		break;

	case GPS_MDLYPL_LPPM:
		x_id = GPS_MDLX_LPPM;
		break;
	case GPS_MDLYPL_MPELOG:
		x_id = GPS_MDLX_MPELOG;
		break;

	default:
		is_okay = false;
		return is_okay;
	}

	is_okay = true;
	if (p_xid)
		*p_xid = x_id;
	return is_okay;
}


void gps_mcudl_mcu2ap_try_to_wakeup_xlink_reader(enum gps_mcudl_yid y_id, enum gps_mcudl_pkt_type type,
	const gpsmdl_u8 *payload_ptr, gpsmdl_u16 payload_len)
{
	enum gps_mcudl_xid x_id;
	enum gps_mcudl_xid x_id_next;
	struct gps_mcudl_each_link *p_xlink;
	enum GDL_RET_STATUS gdl_ret;
	bool do_wake;

	if (!gps_mcudl_ypl_type2xid(type, &x_id)) {
		MDL_LOGYW(y_id, "recv type=%d, len=%d, no x_id", type, payload_len);
		return;
	}

_loop_start:
	if (x_id == GPS_MDLX_GDLOG) {
		/* duplicate the payload to GDLOG2 device node */
		x_id_next = GPS_MDLX_GDLOG2;
	} else if (x_id == GPS_MDLX_MPELOG) {
		/* duplicate the payload to MPELOG2 device node */
		x_id_next = GPS_MDLX_MPELOG2;
	} else {
		/* other payload type only be dispatched to signle device node */
		x_id_next = GPS_MDLX_CH_NUM;
	}

	MDL_LOGYD(y_id, "recv type=%d, len=%d, to x_id=%d", type, payload_len, x_id);
	p_xlink = gps_mcudl_link_get(x_id);

	if (gps_mcudl_each_link_get_state(x_id) != LINK_OPENED)
		goto _loop_end;

	gdl_ret = gdl_dma_buf_put(&p_xlink->rx_dma_buf, payload_ptr, payload_len);
	if (gdl_ret != GDL_OKAY) {
		MDL_LOGXW(x_id, "gdl_dma_buf_put: ret=%s", gdl_ret_to_name(gdl_ret));
		/* not return, still wakeup to avoid race condition */
		/* goto _loop_end; */
	}

	do_wake = gps_dl_link_wake_up2(&p_xlink->waitables[GPS_DL_WAIT_READ]);
	if (do_wake)
		MDL_LOGXI(x_id, "do_wake=%d, type=%d, len=%d, y_id=%d",
			do_wake, type, payload_len, y_id);
	else
		MDL_LOGXD(x_id, "do_wake=%d, type=%d, len=%d, y_id=%d",
			do_wake, type, payload_len, y_id);

_loop_end:
	if ((unsigned int)x_id_next < (unsigned int)GPS_MDLX_CH_NUM) {
		x_id = x_id_next;
		goto _loop_start;
	}
}

