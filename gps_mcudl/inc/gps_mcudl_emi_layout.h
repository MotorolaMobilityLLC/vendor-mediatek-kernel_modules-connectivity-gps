/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#ifndef _GPS_MCUDL_EMI_LAYOUT_H
#define _GPS_MCUDL_EMI_LAYOUT_H

struct gps_mcudl_emi_layout {
	unsigned char mcu_bin[0x020000];	/*0xF000_0000 ~ 0xF002_0000, 128KB*/
	unsigned char gps_bin[0x032000];	/*0xF002_0000 ~ 0xF005_2000, 200KB*/
	unsigned char dsp_bin[0x080000];	/*0xF005_2000 ~ 0xF00D_2000, 512KB*/
	unsigned char mnl_bin[0x300000];	/*0xF00D_2000 ~ 0xF03D_2000, 3072KB*/
	unsigned char mpe_bin[0x080000];	/*0xF03D_2000 ~ 0xF045_2000, 512KB*/
	unsigned char mcu_ro_rsv[0x01e000]; /*0xF045_2000 ~ 0xF047_0000, 120KB*/

	unsigned char mcu_gps_rw[0x07EC00];	/*0xF047_0000 ~ 0xF04E_EC00, 507KB*/
	unsigned char scp_batch[0x04B000];	/*0xF04E_EC00 ~ 0xF053_9C00, 300KB*/
	unsigned char mcu_rw_rsv[0x016400];	/*0xF053_9C00 ~ 0xF055_0000, 89KB*/
	unsigned char mcu_share[0x048000];	/*0xF055_0000 ~ 0xF059_0000, 288KB*/
	unsigned char gps_legacy[0x078000]; /*0xF059_0000 ~ 0xF060_8000, 480KB*/
	unsigned char gps_nv_emi[0x080000]; /*0xF060_8000 ~ 0xF068_8000, 512KB*/
	unsigned char gps_ap2mcu[0x004000]; /*0xF068_8000 ~ 0xF068_C000, 16KB*/
	unsigned char gps_mcu2ap[0x004000]; /*0xF068_C000 ~ 0xF069_0000, 16KB*/
#if 0
	unsigned char mnl_data[0x200000];	/*0xF069_0000 ~ 0xF089_0000, 2048KB*/
	unsigned char mpe_data[0x080000];	/*0xF089_0000 ~ 0xF091_0000, 512KB*/
#endif
};

void gps_mcudl_show_emi_layout(void);
unsigned int gps_mcudl_get_offset_from_conn_base(void *p);

#endif /* _GPS_MCUDL_EMI_LAYOUT_H */

