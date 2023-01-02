/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef _GPS_MCUDL_DATA_PKT_HOST_API_H
#define _GPS_MCUDL_DATA_PKT_HOST_API_H

#include "gps_mcudl_xlink.h"
#include "gps_mcudl_data_intf_type.h"
#include "gps_mcudl_data_pkt.h"
#include "gps_mcudl_data_pkt_slot.h"
#include "gps_mcudl_data_pkt_rbuf.h"
#include "gps_mcudl_data_pkt_parser.h"
#include "gps_dl_dma_buf.h"

bool gps_mcudl_xid2ypl_type(enum gps_mcudl_xid x_id, enum gps_mcudl_pkt_type *p_type);
bool gps_mcudl_ypl_type2xid(enum gps_mcudl_pkt_type type, enum gps_mcudl_xid *p_xid);

void gps_mcudl_ap2mcu_context_init(enum gps_mcudl_yid yid);
bool gps_mcudl_ap2mcu_xdata_send(enum gps_mcudl_xid xid, struct gdl_dma_buf_entry *p_entry);
bool gps_mcudl_ap2mcu_xdata_send_v2(enum gps_mcudl_xid x_id,
	gpsmdl_u8 *p_data, gpsmdl_u32 data_len, bool *p_to_notify);

bool gps_mcudl_ap2mcu_ydata_send(enum gps_mcudl_yid yid, enum gps_mcudl_pkt_type type,
	gpsmdl_u8 *p_data, gpsmdl_u32 data_len);
void gps_mcudl_ap2mcu_ydata_recv(enum gps_mcudl_yid yid,
	gpsmdl_u8 *p_data, gpsmdl_u32 data_len);

void gps_mcudl_ap2mcu_data_slot_flush_on_xwrite(enum gps_mcudl_xid x_id);
void gps_mcudl_ap2mcu_data_slot_flush_on_recv_sta(enum gps_mcudl_yid y_id);

void proc_func1(enum gps_mcudl_pkt_type type,
	const gpsmdl_u8 *payload_ptr, gpsmdl_u16 payload_len);

int send_func1(const gpsmdl_u8 *p_data, gpsmdl_u32 data_len);

bool gps_mcudl_ap2mcu_get_wait_read_flag(enum gps_mcudl_yid y_id);
void gps_mcudl_ap2mcu_set_wait_read_flag(enum gps_mcudl_yid y_id, bool flag);
bool gps_mcudl_ap2mcu_get_wait_write_flag(enum gps_mcudl_yid y_id);
void gps_mcudl_ap2mcu_set_wait_write_flag(enum gps_mcudl_yid y_id, bool flag);
bool gps_mcudl_ap2mcu_get_wait_flush_flag(enum gps_mcudl_yid y_id);
void gps_mcudl_ap2mcu_set_wait_flush_flag(enum gps_mcudl_yid y_id, bool flag);

void gps_mcudl_ap2mcu_try_to_wakeup_xlink_writer(enum gps_mcudl_yid y_id);
void gps_mcudl_mcu2ap_try_to_wakeup_xlink_reader(enum gps_mcudl_yid y_id, enum gps_mcudl_pkt_type type,
	const gpsmdl_u8 *payload_ptr, gpsmdl_u16 payload_len);

#endif /* _GPS_MCUDL_DATA_PKT_HOST_API_H */
