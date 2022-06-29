/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#include "gps_dl_config.h"
#include "gps_mcudl_config.h"
#include "gps_mcudl_log.h"
#include "gps_mcudl_each_link.h"
#include "gps_mcudl_plat_api.h"
#include "gps_mcudl_data_pkt_host_api.h"
#include "gps_mcudl_xlink.h"
#include "gps_mcudl_ylink.h"
#include "gps_mcusys_fsm.h"
#if GPS_DL_ON_LINUX
#include "gps_dl_subsys_reset.h"
#include "gps_dl_linux_reserved_mem_v2.h"
#endif
#include "gps_dl_hal.h"
#include "gps_dl_time_tick.h"
#include "gps_mcu_hif_api.h"
#include "gps_mcu_hif_host.h"
#if GPS_DL_HAS_MCUDL_FW
#include "gps_mcudl_fw_code.h"
#endif
#include "gps_mcu_hif_mgmt_cmd_send.h"
#include "gps_mcudl_hal_ccif.h"
#include "gps_dl_hw_dep_api.h"
#include "gps_mcusys_data_sync2target.h"
#include "gps_dl_hw_dep_api.h"

struct gps_mcudl_ystate {
	bool open;
	gpsmdl_u32 xstate_bitmask;
};

struct gps_mcudl_ystate g_gps_mcudl_ystate_list[GPS_MDLY_CH_NUM];

int gps_mcudl_plat_do_mcu_ctrl(enum gps_mcudl_yid yid, bool open)
{
	int retval = 0;

	if (yid == GPS_MDLY_NORMAL) {
		if (open)
			retval = gps_mcudl_plat_mcu_open();
		else
			retval = gps_mcudl_plat_mcu_close();
	}

	MDL_LOGYI(yid, "open=%d, ret=%d", open, retval);
	return retval;
}

unsigned int gps_dl_util_get_u32(const unsigned char *p_buffer)
{
	return ((unsigned int)(*p_buffer) +
		((unsigned int)(*(p_buffer + 1)) << 8) +
		((unsigned int)(*(p_buffer + 2)) << 16) +
		((unsigned int)(*(p_buffer + 3)) << 24));
}

#define GPS_MCU_FW_VER_STR_MAX_LEN (100)
bool gps_mcudl_link_drv_on_recv_mgmt_data(const unsigned char *p_data, unsigned int data_len)
{
	unsigned char cmd;
	unsigned char status = 0xFF;
	unsigned int addr, bytes, value;
	unsigned char fw_ver_str[GPS_MCU_FW_VER_STR_MAX_LEN] = {'\x00'};
	int i, j;

	/*TODO:*/
	if (data_len >= 4) {
		MDL_LOGW("data_len=%d, data[0~3]=0x%x 0x%x 0x%x 0x%x",
			data_len, p_data[0], p_data[1], p_data[2], p_data[3]);
	} else if (data_len == 3) {
		MDL_LOGW("data_len=%d, data[0~2]=0x%x 0x%x 0x%x",
			data_len, p_data[0], p_data[1], p_data[2]);
	} else if (data_len == 2) {
		MDL_LOGW("data_len=%d, data[0~1]=0x%x 0x%x",
			data_len, p_data[0], p_data[1]);
	} else if (data_len == 1) {
		MDL_LOGW("data_len=%d, data[0]=0x%x",
			data_len, p_data[0]);
	} else {
		MDL_LOGW("data_len=%d", data_len);
		return true;
	}

	cmd = p_data[0];
	if (data_len >= 2)
		status = p_data[1];

	switch (cmd) {
	case 1:
		gps_mcudl_mgmt_cmd_on_ack(GPS_MCUDL_CMD_OFL_INIT);
		break;
	case 2:
		gps_mcudl_mgmt_cmd_on_ack(GPS_MCUDL_CMD_OFL_DEINIT);
		break;
	case 3:
		gps_mcudl_mgmt_cmd_on_ack(GPS_MCUDL_CMD_FW_LOG_CTRL);
		break;
	case 5:
		if (data_len >= 16) {
			addr = gps_dl_util_get_u32(&p_data[4]);
			bytes = gps_dl_util_get_u32(&p_data[8]);
			value = gps_dl_util_get_u32(&p_data[12]);
			MDL_LOGW("mcu reg read: stat=%d, addr=0x%08x, bytes=%d, value[0]=0x%08x",
				status, addr, bytes, value);
		}
		break;
	case 6:
		if (status != 0)
			break;
		for (i = 0, j = 2; i < GPS_MCU_FW_VER_STR_MAX_LEN - 1; i++, j++) {
			if (j >= data_len)
				break;
			if (p_data[j] == '\x00')
				break;
			fw_ver_str[i] = p_data[j];
		}
		fw_ver_str[i] = '\x00';
		MDL_LOGW("fw_ver=%s", &fw_ver_str[0]);
		break;
	default:
		break;
	}

	return true;
}

bool gps_mcudl_link_drv_on_recv_normal_data(const unsigned char *p_data, unsigned int data_len)
{
	MDL_LOGD("data_len=%d, data0=0x%x", data_len, p_data[0]);
#if 1
	gps_mcudl_mcu2ap_ydata_recv(GPS_MDLY_NORMAL, p_data, data_len);
#else
	gps_mcudl_plat_mcu_ch1_read_proc2(p_data, data_len);
#endif
	return true;
}

int gps_mcudl_hal_link_power_ctrl(enum gps_mcudl_xid xid, int op)
{
	enum gps_mcudl_yid yid;
	struct gps_mcudl_ystate *p_ystate;
	gpsmdl_u32 old_xbitmask, new_xbitmask;
	bool do_mcu_ctrl = false;
	int mcu_ctrl_ret = 0;

	yid = GPS_MDL_X2Y(xid);
	p_ystate = &g_gps_mcudl_ystate_list[yid];
	old_xbitmask = p_ystate->xstate_bitmask;
	new_xbitmask = old_xbitmask;
	if (op)
		new_xbitmask |= (1UL << xid);
	else
		new_xbitmask &= ~(1UL << xid);
	if (xid == GPS_MDLX_LPPM && op == 0) {
		MDL_LOGYI(yid, "out_lpp_mode_notify_mcu");
		gps_mcusys_data_sync2target_lpp_mode_status_cmd(op);
	}

	if (new_xbitmask == old_xbitmask) {
		/* do nothing */
		;
	} else if (op && old_xbitmask == 0) {
		/* turn on */
		do_mcu_ctrl = true;

		MDL_LOGYI(yid, "gps_mcu_hif_init");
		gps_mcu_hif_init();
		gps_mcudl_mcu2ap_ydata_sta_init();
		MDL_LOGYI(yid, "gps_mcudl_ap2mcu_context_init");
		gps_mcudl_ap2mcu_context_init(yid);
		gps_mcusys_mnlbin_fsm(GPS_MCUSYS_MNLBIN_SYS_ON);
		gps_mcusys_gpsbin_state_set(GPS_MCUSYS_GPSBIN_PRE_ON);
		MDL_LOGYI(yid, "gps_mcudl_may_do_fw_loading, before");
		gps_mcudl_may_do_fw_loading();
		MDL_LOGYI(yid, "gps_mcudl_may_do_fw_loading, after");
		mcu_ctrl_ret = gps_mcudl_plat_do_mcu_ctrl(yid, true);
		if (mcu_ctrl_ret == 0)
			gps_mcusys_gpsbin_state_set(GPS_MCUSYS_GPSBIN_POST_ON);
		else {
			MDL_LOGYE(yid, "fail to turn on");
			gps_mcusys_gpsbin_state_set(GPS_MCUSYS_GPSBIN_PRE_OFF);
			(void)gps_mcudl_plat_do_mcu_ctrl(yid, false);
			gps_mcusys_gpsbin_state_set(GPS_MCUSYS_GPSBIN_POST_OFF);
			gps_mcudl_clear_fw_loading_done_flag();
		}
	} else if (!op && new_xbitmask == 0) {
		/* turn off */
		do_mcu_ctrl = true;
		gps_mcusys_gpsbin_state_set(GPS_MCUSYS_GPSBIN_PRE_OFF);
		mcu_ctrl_ret = gps_mcudl_plat_do_mcu_ctrl(yid, false);
		MDL_LOGYD(yid, "gps_mcudl_clear_fw_loading_done_flag");
		gps_mcusys_gpsbin_state_set(GPS_MCUSYS_GPSBIN_POST_OFF);
		gps_mcudl_clear_fw_loading_done_flag();
	}

	if (mcu_ctrl_ret == 0 || !op)
		p_ystate->xstate_bitmask = new_xbitmask;

	if (xid == GPS_MDLX_LPPM && op == 1) {
		MDL_LOGYI(yid, "in_lpp_mode_notify_mcu");
		gps_mcusys_data_sync2target_lpp_mode_status_cmd(op);
	}

	MDL_LOGYI(yid, "xid=%d, op=%d, xbitmask: 0x%08x -> 0x%08x, mcu_ctrl: do=%d, ret=%d",
		xid, op, old_xbitmask, new_xbitmask, do_mcu_ctrl, mcu_ctrl_ret);
	return mcu_ctrl_ret;
}

unsigned int g_conn_xuser;
bool g_gps_mcudl_ever_do_coredump;

int gps_mcudl_hal_conn_power_ctrl(enum gps_mcudl_xid xid, int op)
{
	MDL_LOGXI_ONF(xid,
		"sid = %d, op = %d, user = 0x%x,%d, tia_on = %d",
		gps_mcudl_each_link_get_session_id(xid),
		op, g_conn_xuser, g_gps_conninfa_on, g_gps_tia_on);

	if (1 == op) {
		if (g_conn_xuser == 0) {
			g_gps_mcudl_ever_do_coredump = false;
			gps_dl_log_info_show();
			if (!gps_dl_hal_conn_infra_driver_on())
				return -1;

			gps_dl_hal_load_clock_flag();
#if GPS_DL_HAS_PLAT_DRV
			/*gps_dl_wake_lock_hold(true);*/
#if GPS_DL_USE_TIA
			/*gps_dl_tia_gps_ctrl(true);*/
			/*g_gps_tia_on = true;*/
#endif
#endif
		}
		g_conn_xuser |= (1UL << xid);
	} else if (0 == op) {
		g_conn_xuser &= ~(1UL << xid);
		if (g_conn_xuser == 0) {
#if GPS_DL_HAS_PLAT_DRV
#if GPS_DL_USE_TIA
			/*if (g_gps_tia_on) {*/
			/*	gps_dl_tia_gps_ctrl(false);*/
			/*	g_gps_tia_on = false;*/
			/*}*/
#endif
			/*gps_dl_wake_lock_hold(false);*/
#endif
			gps_dl_hal_conn_infra_driver_off();
		}
	}

	return 0;
}

#if 0
void gps_mcudl_plat_mcu_ch1_event_cb(void)
{
	gps_mcudl_mcu2ap_ydata_notify(GPS_MDLY_NORMAL);
}

void gps_mcudl_plat_mcu_ch1_read_proc(void)
{
	enum gps_mcudl_yid y_id;
	gpsmdl_u8 tmp_buf[2048];
	int ret_len;

	y_id = GPS_MDLY_NORMAL;
	MDL_LOGYD(y_id, "");

	gps_mcudl_mcu2ap_set_wait_read_flag(y_id, false);
	do {
		ret_len = gps_mcudl_plat_mcu_ch1_read_nonblock(&tmp_buf[0], 2048);
		MDL_LOGYD(y_id, "read: len=%d", ret_len);
		if (ret_len > 0)
			gps_mcudl_mcu2ap_ydata_recv(y_id, &tmp_buf[0], ret_len);
	} while (ret_len > 0);
}

void gps_mcudl_plat_mcu_ch1_read_proc2(const unsigned char *p_data, unsigned int data_len)
{
	enum gps_mcudl_yid y_id;

	y_id = GPS_MDLY_NORMAL;
	gps_mcudl_mcu2ap_set_wait_read_flag(y_id, false);
	MDL_LOGYD(y_id, "read: len=%u", data_len);
	if (data_len > 0)
		gps_mcudl_mcu2ap_ydata_recv(y_id, p_data, data_len);
}
#endif

void gps_mcudl_plat_mcu_ch1_reset_start_cb(void)
{
	enum gps_mcudl_yid y_id = GPS_MDLY_NORMAL;

	MDL_LOGI("");
	gps_mcudl_clear_fw_loading_done_flag();
	/*gps_dl_on_pre_connsys_reset();*/
	gps_mcudl_ylink_event_send(y_id, GPS_MCUDL_YLINK_EVT_ID_MCU_RESET_START);
}

void gps_mcudl_plat_mcu_ch1_reset_end_cb(void)
{
	enum gps_mcudl_yid y_id = GPS_MDLY_NORMAL;

	MDL_LOGI("");
	/*gps_dl_on_post_connsys_reset();*/
	gps_mcudl_ylink_event_send(y_id, GPS_MCUDL_YLINK_EVT_ID_MCU_RESET_END);
}

enum gps_mcudl_plat_mcu_ctrl_status {
	GDL_MCU_CLOSED,
	GDL_MCU_OPEN_OKAY,
	GDL_MCU_OPEN_FAIL_ON_POS,
	GDL_MCU_OPEN_FAIL_ON_CMD,
	GDL_MCU_OPEN_STATUS_NUM
};

enum gps_mcudl_plat_mcu_ctrl_status g_gps_mcudl_mcu_ctrl_status;

int gps_mcudl_plat_mcu_open(void)
{
	bool is_okay;

#if (GPS_DL_HAS_MCUDL_FW && GPS_DL_HAS_MCUDL_HAL)
	is_okay = gps_mcudl_xlink_on(&c_gps_mcudl_rom_only_fw_list);
	if (!is_okay) {
		MDL_LOGI("gps_mcudl_xlink_on failed");
		g_gps_mcudl_mcu_ctrl_status = GDL_MCU_OPEN_FAIL_ON_POS;
		return -1;
	}
#endif
	gps_mcudl_mcu2ap_set_wait_read_flag(GPS_MDLY_NORMAL, false);
	gps_mcudl_mcu2ap_set_wait_read_flag(GPS_MDLY_URGENT, false);
	gps_mcudl_mgmt_cmd_state_init_all();

	gps_mcu_hif_recv_listen_start(GPS_MCU_HIF_CH_DMALESS_MGMT,
		&gps_mcudl_link_drv_on_recv_mgmt_data);
	gps_mcu_hif_recv_listen_start(GPS_MCU_HIF_CH_DMA_NORMAL,
		&gps_mcudl_link_drv_on_recv_normal_data);
	MDL_LOGI("add listeners, done");

	g_gps_ccif_irq_cnt = 0;
	g_gps_fw_log_irq_cnt = 0;
	if (g_gps_fw_log_is_on) {
		is_okay = gps_mcu_hif_mgmt_cmd_send_fw_log_ctrl(true);
		if (!is_okay) {
			MDL_LOGW("log_ctrl, is_ok=%d", is_okay);
			g_gps_mcudl_mcu_ctrl_status = GDL_MCU_OPEN_FAIL_ON_CMD;
			return -1;
		}
	}

	gps_mcudl_mgmt_cmd_pre_send(GPS_MCUDL_CMD_OFL_INIT);
	is_okay = gps_mcu_hif_send(GPS_MCU_HIF_CH_DMALESS_MGMT, "\x01", 1);
	MDL_LOGW("write cmd1, is_ok=%d", is_okay);
	if (!is_okay) {
		g_gps_mcudl_mcu_ctrl_status = GDL_MCU_OPEN_FAIL_ON_CMD;
		return -1;
	}

	is_okay = gps_mcudl_mgmt_cmd_wait_ack(GPS_MCUDL_CMD_OFL_INIT, 100);
	MDL_LOGW("write cmd1, wait_ok=%d", is_okay);
	if (!is_okay)
		g_gps_mcudl_mcu_ctrl_status = GDL_MCU_OPEN_FAIL_ON_CMD;
	else
		g_gps_mcudl_mcu_ctrl_status = GDL_MCU_OPEN_OKAY;

	return is_okay ? 0 : -1;
}

int gps_mcudl_plat_mcu_close(void)
{
	bool is_okay;
	bool do_adie_off_in_driver = false;

	if (g_gps_mcudl_mcu_ctrl_status != GDL_MCU_OPEN_OKAY)
		MDL_LOGW("mcu_status = %d", g_gps_mcudl_mcu_ctrl_status);

	if (g_gps_mcudl_mcu_ctrl_status == GDL_MCU_OPEN_OKAY) {
		gps_mcudl_mgmt_cmd_pre_send(GPS_MCUDL_CMD_OFL_DEINIT);
		is_okay = gps_mcu_hif_send(GPS_MCU_HIF_CH_DMALESS_MGMT, "\x02", 1);
		MDL_LOGW("write cmd2, is_ok=%d", is_okay);
		if (is_okay) {
			is_okay = gps_mcudl_mgmt_cmd_wait_ack(GPS_MCUDL_CMD_OFL_DEINIT, 100);
			MDL_LOGW("write cmd2, wait_ok=%d", is_okay);
		}

		if (!is_okay)
			do_adie_off_in_driver = true;
	}

	if (g_gps_mcudl_mcu_ctrl_status != GDL_MCU_CLOSED &&
		g_gps_mcudl_mcu_ctrl_status != GDL_MCU_OPEN_FAIL_ON_POS) {
		gps_mcu_hif_recv_listen_stop(GPS_MCU_HIF_CH_DMA_NORMAL);
		gps_mcu_hif_recv_listen_stop(GPS_MCU_HIF_CH_DMALESS_MGMT);
		MDL_LOGI("remove listeners, done");
	}

	if (g_gps_mcudl_mcu_ctrl_status != GDL_MCU_CLOSED) {
#if GPS_DL_HAS_MCUDL_HAL
		gps_mcudl_xlink_off();
#endif
	}

	if (do_adie_off_in_driver)
		gps_dl_hw_dep_gps_control_adie_off();

	g_gps_mcudl_mcu_ctrl_status = GDL_MCU_CLOSED;
	return 0;
}

int gps_mcudl_plat_mcu_ch1_write(const unsigned char *kbuf, unsigned int count)
{
	bool is_okay;

	is_okay = gps_mcu_hif_send(GPS_MCU_HIF_CH_DMA_NORMAL, kbuf, count);
	MDL_LOGW("write count=%d, is_ok=%d", count, is_okay);
	if (!is_okay)
		return 0;
	return count;
}

int gps_mcudl_plat_mcu_ch1_read_nonblock(unsigned char *kbuf, unsigned int count)
{
	return 0;
}

void gps_mcudl_plat_nv_emi_clear(void)
{
	struct gps_mcudl_emi_layout *p_layout;

	p_layout = gps_dl_get_conn_emi_layout_ptr();
	memset_io(&p_layout->gps_nv_emi[0], 0, sizeof(p_layout->gps_nv_emi));
}

void *gps_mcudl_plat_nv_emi_get_start_ptr(void)
{
	struct gps_mcudl_emi_layout *p_layout;

	p_layout = gps_dl_get_conn_emi_layout_ptr();
	return (void *)&p_layout->gps_nv_emi[0];
}

