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
#ifndef _GPS_DL_HAL_H
#define _GPS_DL_HAL_H

#include "gps_dl_config.h"
#include "gps_dl_dma_buf.h"
#include "gps_dl_hal_type.h"

/* for gps_each_device.c */


/* for gps_each_link.c */
void gps_dl_hal_conn_infra_driver_off(void);
int gps_dl_hal_conn_power_ctrl(enum gps_dl_link_id_enum link_id, int op);
int gps_dl_hal_link_power_ctrl(enum gps_dl_link_id_enum link_id, int op);
#if GPS_DL_ON_LINUX
bool gps_dl_hal_md_blanking_init_pta(void);
void gps_dl_hal_md_blanking_deinit_pta(void);
#endif
void gps_dl_hal_a2d_tx_dma_start(enum gps_dl_link_id_enum link_id,
	struct gdl_dma_buf_entry *p_entry);
void gps_dl_hal_d2a_rx_dma_start(enum gps_dl_link_id_enum link_id,
	struct gdl_dma_buf_entry *p_entry);

void gps_dl_hal_a2d_tx_dma_stop(enum gps_dl_link_id_enum link_id);
void gps_dl_hal_d2a_rx_dma_stop(enum gps_dl_link_id_enum link_id);

unsigned int gps_dl_hal_d2a_rx_dma_get_rx_len(enum gps_dl_link_id_enum link_id);
enum GDL_RET_STATUS gps_dl_hal_d2a_rx_dma_get_write_index(
	enum gps_dl_link_id_enum link_id, unsigned int *p_write_index);

enum GDL_RET_STATUS gps_dl_hal_a2d_tx_dma_wait_until_done_and_stop_it(
	enum gps_dl_link_id_enum link_id, int timeout_usec);
enum GDL_RET_STATUS gps_dl_hal_d2a_rx_dma_wait_until_done(
	enum gps_dl_link_id_enum link_id, int timeout_usec);
enum GDL_RET_STATUS gps_dl_hal_wait_and_handle_until_usrt_has_data(
	enum gps_dl_link_id_enum link_id, int timeout_usec);
enum GDL_RET_STATUS gps_dl_hal_wait_and_handle_until_usrt_has_nodata_or_rx_dma_done(
	enum gps_dl_link_id_enum link_id, int timeout_usec);

enum GDL_RET_STATUS gps_dl_hal_poll_event(
	unsigned int L1_evt_in, unsigned int L5_evt_in,
	unsigned int *pL1_evt_out, unsigned int *pL5_evt_out, unsigned int timeout_usec);

int gps_dl_hal_usrt_direct_write(enum gps_dl_link_id_enum link_id,
	unsigned char *buf, unsigned int len);
int gps_dl_hal_usrt_direct_read(enum gps_dl_link_id_enum link_id,
	unsigned char *buf, unsigned int len);

void gps_each_dsp_reg_read_ack(
	enum gps_dl_link_id_enum link_id, const struct gps_dl_hal_mcub_info *p_d2a);
void gps_each_dsp_reg_gourp_read_init(enum gps_dl_link_id_enum link_id);
void gps_each_dsp_reg_gourp_read_start(enum gps_dl_link_id_enum link_id);

enum GDL_RET_STATUS gps_each_dsp_reg_read_request(
	enum gps_dl_link_id_enum link_id, unsigned int reg_addr);
void gps_each_dsp_reg_gourp_read_next(enum gps_dl_link_id_enum link_id, bool restart);


#endif /* _GPS_DL_HAL_H */

