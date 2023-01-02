/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#ifndef _GPS_MCUDL_EMI_LAYOUT_H
#define _GPS_MCUDL_EMI_LAYOUT_H

struct gps_mcudl_emi_layout {
	unsigned char mcu_bin[0x020000];    /*0xF000_0000 ~ 0xF002_0000, 128KB*/
	unsigned char gps_bin[0x032000];    /*0xF002_0000 ~ 0xF005_2000, 200KB*/
	unsigned char dsp_bin[0x080000];    /*0xF005_2000 ~ 0xF00D_2000, 512KB*/
	unsigned char mnl_bin[0x36B000];    /*0xF00D_2000 ~ 0xF043_D000, 3500KB*/
	unsigned char mpe_bin[0x0C0000];    /*0xF043_D000 ~ 0xF04F_D000, 768KB*/
	unsigned char mcu_ro_rsv[0x013000]; /*0xF04F_D000 ~ 0xF051_0000, 76KB*/

	unsigned char mcu_gps_rw[0x07EC00]; /*0xF051_0000 ~ 0xF058_EC00, 507KB*/
	unsigned char scp_batch[0x04B000];  /*0xF058_EC00 ~ 0xF05D_9C00, 300KB*/
	unsigned char mcu_rw_rsv[0x016400]; /*0xF05D_9C00 ~ 0xF0F0_0000, 89KB*/
	unsigned char mcu_share[0x040000];  /*0xF0F0_0000 ~ 0xF063_0000, 256KB*/
	unsigned char gps_legacy[0x078000]; /*0xF063_0000 ~ 0xF0A8_8000, 480KB*/
	unsigned char gps_nv_emi[0x0C0000]; /*0xF0A8_8000 ~ 0xF072_8000, 768KB*/
	unsigned char gps_ap2mcu[0x004000]; /*0xF072_8000 ~ 0xF072_C000, 16KB*/
	unsigned char gps_mcu2ap[0x004000]; /*0xF072_C000 ~ 0xF073_0000, 16KB*/
};

void gps_mcudl_show_emi_layout(void);
unsigned int gps_mcudl_get_offset_from_conn_base(void *p);

#endif /* _GPS_MCUDL_EMI_LAYOUT_H */

