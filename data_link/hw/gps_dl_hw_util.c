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
#include "gps_dl_hw_priv_util.h"
#include "gps_dl_subsys_reset.h"

#if GPS_DL_ON_LINUX
#include <linux/delay.h>
#include <linux/io.h>
#include <asm/io.h>
#include <sync_write.h>
#if GPS_DL_HAS_PLAT_DRV
#include "gps_dl_linux_plat_drv.h"
#endif
#elif GPS_DL_ON_CTP
#include "kernel_to_ctp.h"
#endif

unsigned int gps_dl_bus_to_host_addr(enum GPS_DL_BUS_ENUM bus_id, unsigned int bus_addr)
{
	unsigned int host_addr = 0;

	switch (bus_id) {
	case GPS_DL_GPS_BUS:
		host_addr = gps_bus_to_host(bus_addr);
		break;
	case GPS_DL_BGF_BUS:
		host_addr = bgf_bus_to_host(bus_addr);
		break;
	case GPS_DL_CONN_INFRA_BUS:
		host_addr = bus_addr;
		break;
	default:
		host_addr = 0;
	}

	return host_addr;
}

void gps_dl_bus_write_opt(enum GPS_DL_BUS_ENUM bus_id, unsigned int bus_addr, unsigned int val, bool read_back)
{
	unsigned int host_addr = gps_dl_bus_to_host_addr(bus_id, bus_addr);

#if GPS_DL_ON_LINUX
#if GPS_DL_HAS_PLAT_DRV
	void __iomem *host_vir_addr = gps_dl_host_addr_to_virt(host_addr);
#else
	void __iomem *host_vir_addr = phys_to_virt(host_addr);
#endif

	if (host_vir_addr == NULL) {
		GDL_LOGW("gdl bus wr: id = %d, addr = 0x%p/0x%08x/0x%08x -> NULL!",
			bus_id, host_vir_addr, host_addr, bus_addr);
		return;
	}

	/* gps_dl_conninfra_not_readable_show_warning(host_addr); */
#if !GPS_DL_HW_IS_MOCK
	mt_reg_sync_writel(val, host_vir_addr);
#endif

	if (read_back) {
		/* readback for check */
		unsigned int read_back_val = 0;
#if !GPS_DL_HW_IS_MOCK
		read_back_val = __raw_readl(host_vir_addr);
#endif

		if (gps_dl_show_reg_rw_log())
			GDL_LOGI("gdl bus wr: id = %d, addr = 0x%p/0x%08x/0x%08x, w_val = 0x%08x, r_back = 0x%08x",
				bus_id, host_vir_addr, host_addr, bus_addr, val, read_back_val);
	} else {
		if (gps_dl_show_reg_rw_log())
			GDL_LOGI("gdl bus wr: id = %d, addr = 0x%p/0x%08x/0x%08x, w_val = 0x%08x",
				bus_id, host_vir_addr, host_addr, bus_addr, val);

	}
#else
	/* GDL_LOGD("gdl bus wr: id = %d, addr = 0x%08x/0x%08x, value = 0x%08x", bus_id, host_addr, bus_addr, val); */
	GPS_DL_HOST_REG_WR(host_addr, val);

	if (read_back) {
		/* readback for check */
		unsigned int read_back_val;

		read_back_val = GPS_DL_HOST_REG_RD(host_addr);

		if (gps_dl_show_reg_rw_log())
			GDL_LOGI("gdl bus wr: id = %d, addr = 0x%08x/0x%08x, w_val = 0x%08x, r_back = 0x%08x",
				bus_id, host_addr, bus_addr, val, read_back_val);
	} else {
		if (gps_dl_show_reg_rw_log())
			GDL_LOGI("gdl bus wr: id = %d, addr = 0x%08x/0x%08x, w_val = 0x%08x",
				bus_id, host_addr, bus_addr, val);
	}
#endif
}

void gps_dl_bus_write(enum GPS_DL_BUS_ENUM bus_id, unsigned int bus_addr, unsigned int val)
{
	gps_dl_bus_write_opt(bus_id, bus_addr, val, true);
}

void gps_dl_bus_write_no_rb(enum GPS_DL_BUS_ENUM bus_id, unsigned int bus_addr, unsigned int val)
{
	gps_dl_bus_write_opt(bus_id, bus_addr, val, false);
}

unsigned int gps_dl_bus_read(enum GPS_DL_BUS_ENUM bus_id, unsigned int bus_addr)
{
	unsigned int val = 0;
	unsigned int host_addr = gps_dl_bus_to_host_addr(bus_id, bus_addr);
#if GPS_DL_ON_LINUX
#if GPS_DL_HAS_PLAT_DRV
	void __iomem *host_vir_addr = gps_dl_host_addr_to_virt(host_addr);
#else
	void __iomem *host_vir_addr = phys_to_virt(host_addr);
#endif

	if (host_vir_addr == NULL) {
		GDL_LOGW("gdl bus rd: id = %d, addr = 0x%p/0x%08x/0x%08x -> NULL!",
			bus_id, host_vir_addr, host_addr, bus_addr);
		return 0;
	}

	/* gps_dl_conninfra_not_readable_show_warning(host_addr); */
#if !GPS_DL_HW_IS_MOCK
	val = __raw_readl(host_vir_addr);
#endif

	if (gps_dl_show_reg_rw_log())
		GDL_LOGI("gdl bus rd: id = %d, addr = 0x%p/0x%08x/0x%08x, r_val = 0x%08x",
			bus_id, host_vir_addr, host_addr, bus_addr, val);
#else
	val = GPS_DL_HOST_REG_RD(host_addr);

	if (gps_dl_show_reg_rw_log())
		GDL_LOGI("gdl bus rd: id = %d, addr = 0x%08x/0x%08x, r_val = 0x%08x",
			bus_id, host_addr, bus_addr, val);
#endif
	return val;
}

