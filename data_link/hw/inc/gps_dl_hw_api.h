#ifndef _GPS_DL_HW_API_H
#define _GPS_DL_HW_API_H

#include "gps_dl_config.h"
#include "gps_dl_dma_buf.h"
#include "gps_dl_hal_api.h"
#include "gps_dl_hal_type.h"

enum GDL_HW_RET {
	HW_OKAY,        /* hw CRs access okay */
	E_INV_PARAMS,   /* invalid parameters */
	E_RESETTING,    /* whole chip reset is on-going */
	E_POLL_TIMEOUT, /* timeout when waiting CR change to excepted value */
	E_MAX
};

#define GDL_HW_STATUS_POLL_INTERVAL_USEC	(2*1000) /* 2ms */


enum dsp_ctrl_enum {
	GPS_L1_DSP_ON,
	GPS_L1_DSP_OFF,
	GPS_L5_DSP_ON,
	GPS_L5_DSP_OFF,
	GPS_DSP_CTRL_MAX
};
void gps_dl_hw_gps_dsp_ctrl(enum dsp_ctrl_enum ctrl);

void gps_dl_hw_gps_common_on(void);
void gps_dl_hw_gps_common_off(void);

enum GDL_HW_RET gps_dl_hw_get_mcub_info(
	enum gps_dl_link_id_enum link_id, struct gps_dl_hal_mcub_info *p);

void gps_dl_hw_clear_mcub_d2a_flag(
	enum gps_dl_link_id_enum link_id, unsigned int d2a_flag);

unsigned int gps_dl_hw_get_mcub_a2d_flag(enum gps_dl_link_id_enum link_id);

enum GDL_RET_STATUS gps_dl_hw_mcub_dsp_read_request(
	enum gps_dl_link_id_enum link_id, u16 dsp_addr);

void gps_dl_hw_set_gps_emi_remapping(unsigned int _20msb_of_36bit_phy_addr);
unsigned int gps_dl_hw_get_gps_emi_remapping(void);

void gps_dl_remap_ctx_cal_and_set(void);
enum GDL_RET_STATUS gps_dl_remap_emi_phy_addr(unsigned int phy_addr, unsigned int *bus_addr);

void gps_dl_hw_set_dma_start(enum gps_dl_hal_dma_ch_index channel,
	struct gdl_hw_dma_transfer *p_transfer);

void gps_dl_hw_set_dma_stop(enum gps_dl_hal_dma_ch_index channel);

bool gps_dl_hw_get_dma_int_status(enum gps_dl_hal_dma_ch_index channel);

unsigned int gps_dl_hw_get_dma_left_len(enum gps_dl_hal_dma_ch_index channel);

struct gps_dl_hw_dma_status_struct {
	unsigned int wrap_count;
	unsigned int wrap_to_addr;
	unsigned int total_count;
	unsigned int config;
	unsigned int start_flag;
	unsigned int intr_flag;
	unsigned int left_count;
	unsigned int curr_addr;
	unsigned int state;
};

void gps_dl_hw_save_dma_status_struct(
	enum gps_dl_hal_dma_ch_index ch, struct gps_dl_hw_dma_status_struct *p);

void gps_dl_hw_print_dma_status_struct(
	enum gps_dl_hal_dma_ch_index ch, struct gps_dl_hw_dma_status_struct *p);

enum GDL_RET_STATUS gps_dl_hw_wait_until_dma_complete_and_stop_it(enum gps_dl_hal_dma_ch_index ch, int timeout_usec);


struct gps_dl_hw_usrt_status_struct {
	unsigned int ctrl_setting;
	unsigned int intr_enable;
	unsigned int state;
	unsigned int mcub_d2a_flag;
	unsigned int mcub_d2a_d0;
	unsigned int mcub_d2a_d1;
	unsigned int mcub_a2d_flag;
	unsigned int mcub_a2d_d0;
	unsigned int mcub_a2d_d1;
};

/* TODO: replace gps_dl_hw_usrt_status_struct */
struct gps_dl_hw_link_status_struct {
	bool usrt_has_data;
	bool usrt_has_nodata;
	bool rx_dma_done;
	bool tx_dma_done;
};

void gps_dl_hw_get_link_status(
	enum gps_dl_link_id_enum link_id, struct gps_dl_hw_link_status_struct *p);

void gps_dl_hw_save_usrt_status_struct(
	enum gps_dl_link_id_enum link_id, struct gps_dl_hw_usrt_status_struct *p);

void gps_dl_hw_print_usrt_status_struct(
	enum gps_dl_link_id_enum link_id, struct gps_dl_hw_usrt_status_struct *p);

void gps_dl_hw_print_hw_status(enum gps_dl_link_id_enum link_id);

void gps_dl_hw_switch_dsp_jtag(void);

void gps_dl_hw_usrt_rx_irq_enable(
	enum gps_dl_link_id_enum link_id, bool enable);
void gps_dl_hw_usrt_ctrl(enum gps_dl_link_id_enum link_id,
	bool is_on, bool is_dma_mode, bool is_1byte_mode);
void gps_dl_hw_usrt_clear_nodata_irq(enum gps_dl_link_id_enum link_id);
bool gps_dl_hw_usrt_has_set_nodata_flag(enum gps_dl_link_id_enum link_id);

#endif /* _GPS_DL_HW_API_H */

