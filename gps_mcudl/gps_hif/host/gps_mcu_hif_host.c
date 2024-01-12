/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#include "gps_dl_config.h"
#include "gps_mcudl_log.h"
#include "gps_mcu_hif_host.h"
#if GPS_DL_HAS_MCUDL_HAL
#include "gps_mcudl_hal_ccif.h"
#endif
#if GPS_DL_ON_LINUX
#include <asm/io.h>
#include "gps_dl_linux_reserved_mem_v2.h"
#else
#include <string.h>
#endif

union gps_mcu_hif_ap2mcu_shared_data_union *p_gps_mcu_hif_ap2mcu_region;
union gps_mcu_hif_mcu2ap_shared_data_union *p_gps_mcu_hif_mcu2ap_region;

struct gps_mcu_hif_recv_ch_context {
	bool is_listening;
	gps_mcu_hif_ch_on_recv_cb custom_cb;
};

struct gps_mcu_hif_recv_ch_context g_gps_mcu_hif_recv_contexts[GPS_MCU_HIF_CH_NUM];

void gps_mcu_hif_host_init_ch(enum gps_mcu_hif_ch hif_ch)
{
	unsigned char *p_buf;
	struct gps_mcu_hif_trans_start_desc start_desc;

	p_buf = gps_mcu_hif_get_ap2mcu_emi_buf_addr(hif_ch);
#if GPS_DL_ON_LINUX
	memset_io(p_buf, 0, gps_mcu_hif_get_ap2mcu_emi_buf_len(hif_ch));
#else
	memset(p_buf, 0, gps_mcu_hif_get_ap2mcu_emi_buf_len(hif_ch));
#endif
	start_desc.addr = 0;
	start_desc.len = 0;
	start_desc.id = 0;
	start_desc.zero = 0;
	gps_mcu_hif_set_ap2mcu_trans_start_desc(hif_ch, &start_desc);

	p_buf = gps_mcu_hif_get_mcu2ap_emi_buf_addr(hif_ch);
#if GPS_DL_ON_LINUX
	memset_io(p_buf, 0, gps_mcu_hif_get_mcu2ap_emi_buf_len(hif_ch));
#else
	memset(p_buf, 0, gps_mcu_hif_get_mcu2ap_emi_buf_len(hif_ch));
#endif
	start_desc.addr = 0;
	start_desc.len = 0;
	start_desc.id = 0;
	start_desc.zero = 0;
	gps_mcu_hif_set_mcu2ap_trans_start_desc(hif_ch, &start_desc);
}

unsigned int gps_mcu_hif_convert_ap_addr2mcu_addr(unsigned char *p_buf)
{
#if GPS_DL_ON_LINUX
	struct gps_mcudl_emi_layout *p_layout =
		gps_dl_get_conn_emi_layout_ptr();

	return (unsigned int)(p_buf - (unsigned char *)p_layout) + 0x70000000;
#else
	return (unsigned int)p_buf - 0x87000000 + 0x70000000;
#endif
}

void gps_mcu_hif_init(void)
{
#if GPS_DL_ON_LINUX
	struct gps_mcudl_emi_layout *p_layout =
		gps_dl_get_conn_emi_layout_ptr();

	p_gps_mcu_hif_ap2mcu_region = (union gps_mcu_hif_ap2mcu_shared_data_union *)&p_layout->gps_ap2mcu[0];
	p_gps_mcu_hif_mcu2ap_region = (union gps_mcu_hif_mcu2ap_shared_data_union *)&p_layout->gps_mcu2ap[0];
	MDL_LOGI("ap2mcu: p=0x%p, offset=0x%x, size=0x%x",
		p_gps_mcu_hif_ap2mcu_region,
		gps_mcudl_get_offset_from_conn_base(p_gps_mcu_hif_ap2mcu_region),
		sizeof(*p_gps_mcu_hif_ap2mcu_region));
	MDL_LOGI("mcu2ap: p=0x%p, offset=0x%x, size=0x%x",
		p_gps_mcu_hif_mcu2ap_region,
		gps_mcudl_get_offset_from_conn_base(p_gps_mcu_hif_mcu2ap_region),
		sizeof(*p_gps_mcu_hif_mcu2ap_region));
#else
	p_gps_mcu_hif_ap2mcu_region = (union gps_mcu_hif_ap2mcu_shared_data_union *)0x8707A000;
	p_gps_mcu_hif_mcu2ap_region = (union gps_mcu_hif_mcu2ap_shared_data_union *)0x8707E000;
#endif
	memset(&g_gps_mcu_hif_recv_contexts, 0, sizeof(g_gps_mcu_hif_recv_contexts));
	gps_mcu_hif_host_inf_init();
	gps_mcu_hif_host_init_ch(GPS_MCU_HIF_CH_DMALESS_MGMT);
	gps_mcu_hif_host_init_ch(GPS_MCU_HIF_CH_DMA_NORMAL);
	gps_mcu_hif_host_init_ch(GPS_MCU_HIF_CH_DMA_URGENT);
}

bool gps_mcu_hif_send(enum gps_mcu_hif_ch hif_ch,
	const unsigned char *p_data, unsigned int data_len)
{
	unsigned char *p_buf;
	struct gps_mcu_hif_trans_start_desc start_desc;
	enum gps_mcu_hif_trans trans_id;
	int i;

	trans_id = gps_mcu_hif_get_ap2mcu_trans(hif_ch);
	if (gps_mcu_hif_is_trans_req_sent(trans_id)) {
		MDL_LOGW("hif_ch=%d, len=%d, send fail due to last one not finished",
			hif_ch, data_len);
		/* TODO: Register resend for fail */
		return false;
	}
#if GPS_DL_HAS_MCUDL_HAL
	if (gps_mcudl_hal_ccif_tx_is_busy(GPS_MCUDL_CCIF_CH4)) {
		MDL_LOGW("hif_ch=%d, len=%d, send fail due to ccif busy",
			hif_ch, data_len);
		return false;
	}
	gps_mcudl_hal_ccif_tx_prepare(GPS_MCUDL_CCIF_CH4);
#endif
	gps_mcu_hif_host_clr_trans_req_sent(trans_id);
	p_buf = gps_mcu_hif_get_ap2mcu_emi_buf_addr(hif_ch);
	for (i = 0; i < data_len; i++)
		p_buf[i] = p_data[i];
	gps_mcu_hif_get_ap2mcu_trans_start_desc(hif_ch, &start_desc);
	start_desc.addr = gps_mcu_hif_convert_ap_addr2mcu_addr(p_buf);
	start_desc.len = data_len;
	start_desc.id++;
	gps_mcu_hif_set_ap2mcu_trans_start_desc(hif_ch, &start_desc);
	gps_mcu_hif_host_set_trans_req_sent(trans_id);
#if GPS_DL_HAS_MCUDL_HAL
	gps_mcudl_hal_ccif_tx_trigger(GPS_MCUDL_CCIF_CH4);
#endif
	return true;
}

void gps_mcu_hif_recv_start(enum gps_mcu_hif_ch hif_ch)
{
	unsigned char *p_buf;
	struct gps_mcu_hif_trans_start_desc start_desc;
	enum gps_mcu_hif_trans trans_id;

	trans_id = gps_mcu_hif_get_mcu2ap_trans(hif_ch);
	if (gps_mcu_hif_is_trans_req_sent(trans_id)) {
		MDL_LOGW("hif_ch=%d, trans_id=%d, rx_ongoing", hif_ch, trans_id);
		return;
	}
	p_buf = gps_mcu_hif_get_mcu2ap_emi_buf_addr(hif_ch);

#if GPS_DL_HAS_MCUDL_HAL
	if (gps_mcudl_hal_ccif_tx_is_busy(GPS_MCUDL_CCIF_CH4)) {
		MDL_LOGW("hif_ch=%d, recv fail due to ccif busy", hif_ch);
		return;
	}
	gps_mcudl_hal_ccif_tx_prepare(GPS_MCUDL_CCIF_CH4);
#endif
	gps_mcu_hif_get_mcu2ap_trans_start_desc(hif_ch, &start_desc);
	start_desc.addr = gps_mcu_hif_convert_ap_addr2mcu_addr(p_buf);
	start_desc.len = gps_mcu_hif_get_mcu2ap_emi_buf_len(hif_ch);
	start_desc.id++;
	gps_mcu_hif_set_mcu2ap_trans_start_desc(hif_ch, &start_desc);
	gps_mcu_hif_host_set_trans_req_sent(trans_id);
#if GPS_DL_HAS_MCUDL_HAL
	gps_mcudl_hal_ccif_tx_trigger(GPS_MCUDL_CCIF_CH4);
#endif
}

void gps_mcu_hif_recv_stop(enum gps_mcu_hif_ch hif_ch)
{
	enum gps_mcu_hif_trans trans_id;

	trans_id = gps_mcu_hif_get_mcu2ap_trans(hif_ch);
	if (!gps_mcu_hif_is_trans_req_sent(trans_id)) {
		MDL_LOGW("hif_ch=%d, trans_id=%d, no rx_ongoing", hif_ch, trans_id);
		return;
	}
	gps_mcu_hif_host_clr_trans_req_sent(trans_id);
}

void gps_mcu_hif_recv_listen_start(enum gps_mcu_hif_ch hif_ch, gps_mcu_hif_ch_on_recv_cb custom_cb)
{
	struct gps_mcu_hif_recv_ch_context *p_ctx;

	p_ctx = &g_gps_mcu_hif_recv_contexts[hif_ch];
	p_ctx->is_listening = true;
	p_ctx->custom_cb = custom_cb;
	gps_mcu_hif_recv_start(hif_ch);
}

void gps_mcu_hif_recv_listen_stop(enum gps_mcu_hif_ch hif_ch)
{
	struct gps_mcu_hif_recv_ch_context *p_ctx;

	p_ctx = &g_gps_mcu_hif_recv_contexts[hif_ch];
	p_ctx->is_listening = false;
	gps_mcu_hif_recv_stop(hif_ch);
	p_ctx->custom_cb = NULL;
}

void gps_mcu_hif_host_on_tx_finished(enum gps_mcu_hif_ch ch)
{
	MDL_LOGI("ch=%d", ch);
}

unsigned char gps_mcu_hif_on_recv_dispatcher_buf[GPS_MCU_HIF_EMI_BUF_SIZE];
void gps_mcu_hif_on_recv_dispatcher(enum gps_mcu_hif_ch hif_ch,
	const unsigned char *p_data, unsigned int data_len)
{
	struct gps_mcu_hif_recv_ch_context *p_ctx;

	MDL_LOGI("ch=%d", hif_ch);
	p_ctx = &g_gps_mcu_hif_recv_contexts[hif_ch];
	if (!p_ctx->is_listening)
		return;

	memcpy(&gps_mcu_hif_on_recv_dispatcher_buf[0], p_data, data_len);

	gps_mcu_hif_recv_start(hif_ch);
	if (!p_ctx->custom_cb)
		return;

	(*p_ctx->custom_cb)(&gps_mcu_hif_on_recv_dispatcher_buf[0], data_len);
}

void gps_mcu_hif_host_trans_finished(enum gps_mcu_hif_trans trans_id)
{
	struct gps_mcu_hif_trans_start_desc start_desc;
	struct gps_mcu_hif_trans_end_desc end_desc;
	enum gps_mcu_hif_ch hif_ch;

	hif_ch = gps_mcu_hif_get_trans_hif_ch(trans_id);
	gps_mcu_hif_get_trans_start_desc(trans_id, &start_desc);
	gps_mcu_hif_get_trans_end_desc(trans_id, &end_desc);
	if (start_desc.id != end_desc.id)
		return; /* bad one*/

	gps_mcu_hif_host_clr_trans_req_sent(trans_id);
	switch (trans_id) {
	case GPS_MCU_HIF_TRANS_AP2MCU_DMALESS_MGMT:
	case GPS_MCU_HIF_TRANS_AP2MCU_DMA_NORMAL:
	case GPS_MCU_HIF_TRANS_AP2MCU_DMA_URGENT:
		gps_mcu_hif_host_on_tx_finished(hif_ch);
		break;

	case GPS_MCU_HIF_TRANS_MCU2AP_DMALESS_MGMT:
	case GPS_MCU_HIF_TRANS_MCU2AP_DMA_NORMAL:
	case GPS_MCU_HIF_TRANS_MCU2AP_DMA_URGENT:
		gps_mcu_hif_on_recv_dispatcher(hif_ch,
			gps_mcu_hif_get_mcu2ap_emi_buf_addr(hif_ch), end_desc.len);
		break;

	default:
		break;
	}
}

void gps_mcu_hif_host_ccif_irq_handler_in_task(void)
{
	enum gps_mcu_hif_trans trans_id;

	for (trans_id = 0; trans_id < GPS_MCU_HIF_TRANS_NUM; trans_id++) {
		if (!gps_mcu_hif_is_trans_req_sent(trans_id))
			continue;
		if (gps_mcu_hif_is_trans_req_received(trans_id))
			continue;
		if (!gps_mcu_hif_is_trans_req_finished(trans_id))
			continue;
		gps_mcu_hif_host_trans_finished(trans_id);
	}
}

void gps_mcu_hif_host_ccif_isr(void)
{
	gps_mcu_hif_host_ccif_irq_handler_in_task();
}
