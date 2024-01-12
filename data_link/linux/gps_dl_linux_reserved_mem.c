/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */
#include "gps_dl_config.h"

#if GPS_DL_HAS_PLAT_DRV
#include "gps_dl_context.h"
#include "gps_dl_linux_plat_drv.h"
#include "gps_dl_linux_reserved_mem.h"
#include "gps_dl_emi.h"

#include <linux/of_reserved_mem.h>
#include <mt_emi_api.h>

#if defined(CONFIG_MACH_MT6885)
#define GPS_EMI_MPU_REGION 29
#endif

#define GPS_ICAP_MEM_SIZE (GPS_ICAP_BUF_SIZE)
#define GPS_RESERVED_MEM_PADDING_SIZE (4*1024)

struct gps_dl_reserved_mem_layout {
	unsigned char icap_buf[GPS_ICAP_MEM_SIZE];
	unsigned char padding1[GPS_RESERVED_MEM_PADDING_SIZE];
	unsigned char tx_dma_buf[LINK_STATE_NUM][GPS_DL_RX_BUF_SIZE + GPS_RESERVED_MEM_PADDING_SIZE];
	unsigned char rx_dma_buf[LINK_STATE_NUM][GPS_DL_RX_BUF_SIZE + GPS_RESERVED_MEM_PADDING_SIZE];
};

struct gps_dl_iomem_addr_map_entry g_gps_dl_res_emi;

void gps_dl_reserved_mem_init(void)
{
	void __iomem *host_virt_addr = NULL;
#if 0
	struct emi_region_info_t region_info;
#endif
	unsigned int min_size = sizeof(struct gps_dl_reserved_mem_layout);

	if (gGpsRsvMemPhyBase == (phys_addr_t)NULL || gGpsRsvMemSize < min_size) {
		GDL_LOGE("res_mem: base = 0x%llx, size = 0x%llx, min_size = %d, not enough",
			(unsigned long long)gGpsRsvMemPhyBase,
			(unsigned long long)gGpsRsvMemSize, min_size);
		return;
	}

	host_virt_addr = ioremap_nocache(gGpsRsvMemPhyBase, gGpsRsvMemSize);
	if (host_virt_addr == NULL) {
		GDL_LOGE("res_mem: base = 0x%llx, size = 0x%llx, iorempa fail",
			(unsigned long long)gGpsRsvMemPhyBase, (unsigned long long)gGpsRsvMemSize);
		return;
	}

	/* TODO: set EMI MPU permission */
#if 0
	GDL_LOGD("emi_mpu_set_protection - start");
	region_info.start = gGpsRsvMemPhyBase;
	region_info.end = gGpsRsvMemPhyBase + gGpsRsvMemSize - 1;
	region_info.region = GPS_EMI_MPU_REGION;
	SET_ACCESS_PERMISSION(region_info.apc, LOCK,
		FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN,
		FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN,
		NO_PROTECTION, FORBIDDEN, NO_PROTECTION);
	emi_mpu_set_protection(&region_info);
	GDL_LOGD("emi_mpu_set_protection - done");
#endif

	g_gps_dl_res_emi.host_phys_addr = gGpsRsvMemPhyBase;
	g_gps_dl_res_emi.host_virt_addr = host_virt_addr;
	g_gps_dl_res_emi.length = gGpsRsvMemSize;

	GDL_LOGI("phy_addr = 0x%08x, vir_addr = 0x%p, size = 0x%x, min_size = 0x%x",
		g_gps_dl_res_emi.host_phys_addr,
		g_gps_dl_res_emi.host_virt_addr,
		g_gps_dl_res_emi.length, min_size);
	gps_dl_reserved_mem_show_info();
	gps_icap_probe();
}

void gps_dl_reserved_mem_deinit(void)
{
	GDL_LOGI("phy_addr = 0x%08x, vir_addr = 0x%p, size = 0x%x",
		g_gps_dl_res_emi.host_virt_addr,
		g_gps_dl_res_emi.host_phys_addr,
		g_gps_dl_res_emi.length);

	if (g_gps_dl_res_emi.host_virt_addr != NULL)
		iounmap(g_gps_dl_res_emi.host_virt_addr);
	else
		GDL_LOGW("host_virt_addr already null, not do iounmap");

	g_gps_dl_res_emi.host_phys_addr = (dma_addr_t)NULL;
	g_gps_dl_res_emi.host_virt_addr = (void *)NULL;
	g_gps_dl_res_emi.length = 0;
}

bool gps_dl_reserved_mem_is_ready(void)
{
	return (g_gps_dl_res_emi.host_virt_addr != NULL);
}

void gps_dl_reserved_mem_get_range(unsigned int *p_min, unsigned int *p_max)
{
	*p_min = g_gps_dl_res_emi.host_phys_addr;
	*p_max = g_gps_dl_res_emi.host_phys_addr + g_gps_dl_res_emi.length;
}

void gps_dl_reserved_mem_show_info(void)
{
	unsigned int min_size = sizeof(struct gps_dl_reserved_mem_layout);
	struct gps_dl_reserved_mem_layout *p_mem_vir;
	unsigned int p_mem_phy;
	unsigned int p_buf_phy, offset;
	enum gps_dl_link_id_enum link_id;

	p_mem_phy = g_gps_dl_res_emi.host_phys_addr;
	p_mem_vir = (struct gps_dl_reserved_mem_layout *)g_gps_dl_res_emi.host_virt_addr;

	GDL_LOGI("phy_addr = 0x%08x, vir_addr = 0x%p, size = 0x%x, min_size = 0x%x",
		g_gps_dl_res_emi.host_phys_addr,
		g_gps_dl_res_emi.host_virt_addr,
		g_gps_dl_res_emi.length, min_size);

	offset = (unsigned int)((void *)&p_mem_vir->icap_buf[0] - (void *)p_mem_vir);
	GDL_LOGI("icap_buf: phy_addr = 0x%08x, vir_addr = 0x%p, size = 0x%x",
		p_mem_phy + offset, &p_mem_vir->icap_buf[0], GPS_ICAP_MEM_SIZE);

	for (link_id = 0; link_id < GPS_DATA_LINK_NUM; link_id++) {
		offset = (unsigned int)((void *)&p_mem_vir->tx_dma_buf[link_id][0] - (void *)p_mem_vir);
		p_buf_phy = p_mem_phy + offset;
		GDL_LOGXD(link_id, "tx_dma_buf: phy_addr = 0x%08x, vir_addr = 0x%p, size = 0x%x",
			p_buf_phy, &p_mem_vir->tx_dma_buf[link_id][0], GPS_DL_TX_BUF_SIZE);

		offset = (unsigned int)((void *)&p_mem_vir->rx_dma_buf[link_id][0] - (void *)p_mem_vir);
		p_buf_phy = p_mem_phy + offset;
		GDL_LOGXD(link_id, "rx_dma_buf: phy_addr = 0x%08x, vir_addr = 0x%p, size = 0x%x",
			p_buf_phy, &p_mem_vir->rx_dma_buf[link_id][0], GPS_DL_RX_BUF_SIZE);
	}
}

void gps_dl_reserved_mem_dma_buf_init(struct gps_dl_dma_buf *p_dma_buf,
	enum gps_dl_link_id_enum link_id, enum gps_dl_dma_dir dir, unsigned int len)
{
	struct gps_dl_reserved_mem_layout *p_mem_vir;
	unsigned int p_mem_phy, offset;

	p_mem_vir = (struct gps_dl_reserved_mem_layout *)g_gps_dl_res_emi.host_virt_addr;
	p_mem_phy = g_gps_dl_res_emi.host_phys_addr;

	memset(p_dma_buf, 0, sizeof(*p_dma_buf));
	p_dma_buf->dev_index = link_id;
	p_dma_buf->dir = dir;
	p_dma_buf->len = len;

	if (dir == GDL_DMA_A2D) {
		p_dma_buf->vir_addr = (void *)&p_mem_vir->tx_dma_buf[link_id][0];
	} else if (dir == GDL_DMA_D2A) {
		p_dma_buf->vir_addr = (void *)&p_mem_vir->rx_dma_buf[link_id][0];
	} else {
		GDL_LOGXE(link_id, "");
		return;
	}

	offset = (unsigned int)((void *)p_dma_buf->vir_addr - (void *)p_mem_vir);
	p_dma_buf->phy_addr = p_mem_phy + offset;

	GDL_LOGI("init gps dl dma buf(%d,%d) in arch64, addr: vir=0x%p, phy=0x%llx, len=%u\n",
		p_dma_buf->dev_index, p_dma_buf->dir,
		p_dma_buf->vir_addr,
		p_dma_buf->phy_addr, p_dma_buf->len);
}

void gps_dl_reserved_mem_dma_buf_deinit(struct gps_dl_dma_buf *p_dma_buf)
{
	memset(p_dma_buf, 0, sizeof(*p_dma_buf));
}

void *gps_dl_reserved_mem_icap_buf_get_vir_addr(void)
{
	struct gps_dl_reserved_mem_layout *p_mem_vir;
	unsigned int p_mem_phy, offset;

	p_mem_phy = g_gps_dl_res_emi.host_phys_addr;
	p_mem_vir = (struct gps_dl_reserved_mem_layout *)g_gps_dl_res_emi.host_virt_addr;

	if (p_mem_vir == NULL) {
		GDL_LOGE("gps_icap_buf: null");
		return NULL;
	}

	offset = (unsigned int)((void *)&p_mem_vir->icap_buf[0] - (void *)p_mem_vir);
	GDL_LOGI("gps_icap_buf: phy_addr = 0x%08x, vir_addr = 0x%p, size = 0x%x",
		p_mem_phy + offset, &p_mem_vir->icap_buf[0], GPS_ICAP_MEM_SIZE);

	return (void *)&p_mem_vir->icap_buf[0];
}

#endif /* GPS_DL_HAS_PLAT_DRV */

