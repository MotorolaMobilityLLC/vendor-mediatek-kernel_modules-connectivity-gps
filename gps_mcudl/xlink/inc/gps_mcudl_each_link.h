/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#ifndef _GPS_MCUDL_EACH_LINK_H
#define _GPS_MCUDL_EACH_LINK_H

#include "gps_each_link.h"
#include "gps_mcudl_xlink.h"
struct gps_mcudl_each_link {
	struct gps_each_link_cfg cfg;
	struct gps_mcudl_each_device *p_device;
	struct gps_dl_dma_buf tx_dma_buf;
	struct gps_dl_dma_buf rx_dma_buf;
	struct gps_each_link_waitable waitables[GPS_DL_WAIT_NUM];
	struct gps_dl_osal_sleepable_lock mutexes[GPS_DL_MTX_NUM];
	struct gps_dl_osal_unsleepable_lock spin_locks[GPS_DL_SPINLOCK_NUM];
	struct gps_each_link_state_list sub_states;
	enum gps_each_link_state_enum state_for_user;
	enum gps_each_link_reset_level reset_level;
	int session_id;
	bool epoll_flag;
};

void gps_mcudl_each_link_init(enum gps_mcudl_xid link_id);
void gps_mcudl_each_link_deinit(enum gps_mcudl_xid link_id);
void gps_mcudl_each_link_context_init(enum gps_mcudl_xid link_id);
void gps_mcudl_each_link_context_clear(enum gps_mcudl_xid link_id);
void gps_mcudl_each_link_inc_session_id(enum gps_mcudl_xid link_id);
int gps_mcudl_each_link_get_session_id(enum gps_mcudl_xid link_id);

int gps_mcudl_each_link_open(enum gps_mcudl_xid link_id);
int gps_mcudl_each_link_close(enum gps_mcudl_xid link_id);
int gps_mcudl_each_link_check(enum gps_mcudl_xid link_id, int reason);
int gps_mcudl_each_link_reset(enum gps_mcudl_xid link_id);
int gps_mcudl_each_link_send_reset_evt(enum gps_mcudl_xid link_id);

int gps_mcudl_each_link_write(enum gps_mcudl_xid link_id,
	unsigned char *buf, unsigned int len);
int gps_mcudl_each_link_write_with_opt(enum gps_mcudl_xid link_id,
	unsigned char *buf, unsigned int len, bool wait_tx_done);
int gps_mcudl_each_link_read(enum gps_mcudl_xid link_id,
	unsigned char *buf, unsigned int len);
int gps_mcudl_each_link_read_with_timeout(enum gps_mcudl_xid link_id,
	unsigned char *buf, unsigned int len, int timeout_usec, bool *p_is_nodata);

#endif /* _GPS_MCUDL_EACH_LINK_H */

