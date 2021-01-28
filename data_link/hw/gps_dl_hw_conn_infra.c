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

#define GPS_EMI_REMAP_BASE_MASK (0xFFFF0000)		/* start from 64KB boundary */
#define GPS_EMI_REMAP_LENGTH    (64 * 1024 * 1024UL)
#define GPS_EMI_BUS_BASE        (0x78000000)

void gps_dl_remap_ctx_cal_and_set(void)
{
	enum gps_dl_link_id_enum  i;
	struct gps_each_link *p_link;
	unsigned int emi_addr_high20_of_36bit = 0;

	unsigned int min_addr = 0xFFFFFFFF;
	unsigned int max_addr = 0;
	unsigned int tx_end;
	unsigned int rx_end;

	for (i = 0; i < GPS_DATA_LINK_NUM; i++) {
		p_link = gps_dl_link_get(i);

		min_addr = (p_link->rx_dma_buf.phy_addr < min_addr) ? p_link->rx_dma_buf.phy_addr : min_addr;
		min_addr = (p_link->tx_dma_buf.phy_addr < min_addr) ? p_link->tx_dma_buf.phy_addr : min_addr;

		rx_end = p_link->rx_dma_buf.phy_addr + p_link->rx_dma_buf.len;
		tx_end = p_link->tx_dma_buf.phy_addr + p_link->tx_dma_buf.len;

		max_addr = (rx_end > min_addr) ? rx_end : max_addr;
		max_addr = (tx_end > min_addr) ? tx_end : max_addr;
	}

	emi_addr_high20_of_36bit = (min_addr & GPS_EMI_REMAP_BASE_MASK);
	GDL_LOGD("min = 0x%09x, max = 0x%09x, base = 0x%09x",
		min_addr, max_addr, emi_addr_high20_of_36bit);

	if (max_addr - emi_addr_high20_of_36bit > GPS_EMI_REMAP_LENGTH) {
		emi_addr_high20_of_36bit = (gps_dl_hw_get_gps_emi_remapping() << 16);
		GDL_LOGD("emi_addr_high20_of_36bit = 0x%08x", emi_addr_high20_of_36bit);
	} else
		gps_dl_hw_set_gps_emi_remapping(emi_addr_high20_of_36bit >> 16);

	gps_dl_remap_ctx_get()->gps_emi_phy_high20 = emi_addr_high20_of_36bit;
}


enum GDL_RET_STATUS gps_dl_remap_emi_phy_addr(unsigned int phy_addr, unsigned int *bus_addr)
{
	unsigned int remap_setting = gps_dl_remap_ctx_get()->gps_emi_phy_high20;

	if ((phy_addr >= remap_setting) &&
		(phy_addr < (remap_setting + GPS_EMI_REMAP_LENGTH))) {
		*bus_addr = GPS_EMI_BUS_BASE + (phy_addr - remap_setting);
		return GDL_OKAY;
	}

	*bus_addr = 0;
	return GDL_FAIL;
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

