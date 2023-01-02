/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#include <linux/arm-smccc.h>
#include <linux/soc/mediatek/mtk_sip_svc.h>


enum conn_smc_opid {
	/* gps_hw_ops */
	SMC_GPS_WAKEUP_CONNINFRA_TOP_OFF_OPID = 1,
};

bool gps_dl_hw_gps_force_wakeup_conninfra_top_off(bool enable);

