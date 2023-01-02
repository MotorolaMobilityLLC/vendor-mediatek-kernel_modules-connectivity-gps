/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef _GPS_MCUDL_DATA_PKT_PAYLOAD_STRUCT_H
#define _GPS_MCUDL_DATA_PKT_PAYLOAD_STRUCT_H

struct gps_mcudl_data_pkt_mcu_sta {
	gpsmdl_u64 total_recv;

	/*the bellow is just for log print, to save bandwidth, still keep them uint32*/
	gpsmdl_u32 total_parse_proc;
	gpsmdl_u32 total_parse_drop;
	gpsmdl_u32 total_route_drop; /*for host route_drop always 0*/
	gpsmdl_u32 total_pkt_cnt;

	gpsmdl_u8 LUINT_L32_VALID_BIT;
};

#endif /* _GPS_MCUDL_DATA_PKT_PAYLOAD_STRUCT_H */
