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
#include "gps_dl_hw_priv_util.h"

#include "conn_infra/conn_host_csr_top.h"

void gps_dl_hw_set_gps_emi_remapping(unsigned int _20msb_of_36bit_phy_addr)
{
	GDL_HW_SET_CONN_INFRA_ENTRY(
		CONN_HOST_CSR_TOP_CONN2AP_REMAP_GPS_EMI_BASE_ADDR_CONN2AP_REMAP_GPS_EMI_BASE_ADDR,
		_20msb_of_36bit_phy_addr);
}

unsigned int gps_dl_hw_get_gps_emi_remapping(void)
{
	return GDL_HW_GET_CONN_INFRA_ENTRY(
		CONN_HOST_CSR_TOP_CONN2AP_REMAP_GPS_EMI_BASE_ADDR_CONN2AP_REMAP_GPS_EMI_BASE_ADDR);
}

void gps_dl_hw_print_hw_status(enum gps_dl_link_id_enum link_id)
{
	struct gps_dl_hw_dma_status_struct a2d_dma_status, d2a_dma_status;
	struct gps_dl_hw_usrt_status_struct usrt_status;
	enum gps_dl_hal_dma_ch_index a2d_dma_ch, d2a_dma_ch;
	unsigned int value;

	if (gps_dl_show_reg_wait_log())
		GDL_LOGXD(link_id, "");

	if (link_id == GPS_DATA_LINK_ID0) {
		a2d_dma_ch = GPS_DL_DMA_LINK0_A2D;
		d2a_dma_ch = GPS_DL_DMA_LINK0_D2A;
	} else if (link_id == GPS_DATA_LINK_ID1) {
		a2d_dma_ch = GPS_DL_DMA_LINK1_A2D;
		d2a_dma_ch = GPS_DL_DMA_LINK1_D2A;
	} else
		return;

	gps_dl_hw_save_usrt_status_struct(link_id, &usrt_status);
	gps_dl_hw_print_usrt_status_struct(link_id, &usrt_status);

	gps_dl_hw_save_dma_status_struct(a2d_dma_ch, &a2d_dma_status);
	gps_dl_hw_print_dma_status_struct(a2d_dma_ch, &a2d_dma_status);

	gps_dl_hw_save_dma_status_struct(d2a_dma_ch, &d2a_dma_status);
	gps_dl_hw_print_dma_status_struct(d2a_dma_ch, &d2a_dma_status);

	value = GDL_HW_RD_GPS_REG(0x80073160);
	value = GDL_HW_GET_CONN_INFRA_ENTRY(CONN_HOST_CSR_TOP_HOST2GPS_DEGUG_SEL_HOST2GPS_DEGUG_SEL);
	value = GDL_HW_GET_CONN_INFRA_ENTRY(CONN_HOST_CSR_TOP_GPS_CFG2HOST_DEBUG_GPS_CFG2HOST_DEBUG);
}

