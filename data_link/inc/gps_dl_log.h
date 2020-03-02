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
#ifndef _GPS_DL_LOG_H
#define _GPS_DL_LOG_H

#include "gps_dl_config.h"

#if GPS_DL_ON_LINUX
#include <linux/printk.h>
#define __GDL_LOGE(fmt, ...) pr_notice("GDL[E] [%s: %d]: "fmt, __func__, __LINE__, ##__VA_ARGS__)
#define __GDL_LOGW(fmt, ...) pr_notice("GDL[W] [%s: %d]: "fmt, __func__, __LINE__, ##__VA_ARGS__)
#define __GDL_LOGI(fmt, ...) pr_info("GDL[I] [%s: %d]: "fmt, __func__, __LINE__, ##__VA_ARGS__)
#define __GDL_LOGD(fmt, ...) pr_info("GDL[D] [%s: %d]: "fmt, __func__, __LINE__, ##__VA_ARGS__)

#define __GDL_LOGXE(link_id, fmt, ...) pr_notice("GDL-%d[E] [%s: %d]: "fmt, \
	link_id, __func__, __LINE__, ##__VA_ARGS__)

#define __GDL_LOGXW(link_id, fmt, ...) pr_notice("GDL-%d[W] [%s: %d]: "fmt, \
		link_id, __func__, __LINE__, ##__VA_ARGS__)

#define __GDL_LOGXI(link_id, fmt, ...) pr_info("GDL-%d[I] [%s: %d]: "fmt, \
	link_id, __func__, __LINE__, ##__VA_ARGS__)

#define __GDL_LOGXD(link_id, fmt, ...) pr_info("GDL-%d[D] [%s: %d]: "fmt, \
	link_id, __func__, __LINE__, ##__VA_ARGS__)
#elif GPS_DL_ON_CTP
#include "gps_dl_ctp_log.h"
#endif /* GPS_DL_ON_XX */


enum gps_dl_log_level_enum {
	GPS_DL_LOG_LEVEL_DBG  = 1,
	GPS_DL_LOG_LEVEL_INFO = 3,
	GPS_DL_LOG_LEVEL_WARN = 5,
	GPS_DL_LOG_LEVEL_ERR  = 7,
};

#define GPS_DL_LOG_LEVEL_DEFAULT GPS_DL_LOG_LEVEL_INFO
enum gps_dl_log_level_enum gps_dl_log_level_get(void);
void gps_dl_log_level_set(enum gps_dl_log_level_enum level);

#define _GDL_LOGE(...) \
	do { if (gps_dl_log_level_get() <= GPS_DL_LOG_LEVEL_ERR) __GDL_LOGE(__VA_ARGS__); } while (0)
#define _GDL_LOGW(...) \
	do { if (gps_dl_log_level_get() <= GPS_DL_LOG_LEVEL_WARN) __GDL_LOGW(__VA_ARGS__); } while (0)
#define _GDL_LOGI(...) \
	do { if (gps_dl_log_level_get() <= GPS_DL_LOG_LEVEL_INFO) __GDL_LOGI(__VA_ARGS__); } while (0)
#define _GDL_LOGD(...) \
	do { if (gps_dl_log_level_get() <= GPS_DL_LOG_LEVEL_DBG) __GDL_LOGD(__VA_ARGS__); } while (0)
#define _GDL_LOGXE(...) \
	do { if (gps_dl_log_level_get() <= GPS_DL_LOG_LEVEL_ERR) __GDL_LOGXE(__VA_ARGS__); } while (0)
#define _GDL_LOGXW(...) \
	do { if (gps_dl_log_level_get() <= GPS_DL_LOG_LEVEL_WARN) __GDL_LOGXW(__VA_ARGS__); } while (0)
#define _GDL_LOGXI(...) \
	do { if (gps_dl_log_level_get() <= GPS_DL_LOG_LEVEL_INFO) __GDL_LOGXI(__VA_ARGS__); } while (0)
#define _GDL_LOGXD(...) \
	do { if (gps_dl_log_level_get() <= GPS_DL_LOG_LEVEL_DBG) __GDL_LOGXD(__VA_ARGS__); } while (0)

#define GDL_LOGE(...)  _GDL_LOGE(__VA_ARGS__)
#define GDL_LOGXE(...) _GDL_LOGXE(__VA_ARGS__)

#define GDL_LOGW(...)  _GDL_LOGW(__VA_ARGS__)
#define GDL_LOGXW(...) _GDL_LOGXW(__VA_ARGS__)


enum gps_dl_log_module_enum {
	GPS_DL_LOG_MOD_DEFAULT    = 0,
	GPS_DL_LOG_MOD_OPEN_CLOSE = 1,
	GPS_DL_LOG_MOD_READ_WRITE = 2,

	GPS_DL_LOG_MOD_MAX        = 32
};

bool gps_dl_log_mod_is_on(enum gps_dl_log_module_enum mod);
void gps_dl_log_mod_bitmask_set(unsigned int bitmask);
unsigned int gps_dl_log_mod_bitmask_get(void);
void gps_dl_log_info_show(void);

#define GDL_LOGI(...) \
	do { if (gps_dl_log_mod_is_on(GPS_DL_LOG_MOD_DEFAULT)) _GDL_LOGI(__VA_ARGS__); } while (0)
#define GDL_LOGD(...) \
	do { if (gps_dl_log_mod_is_on(GPS_DL_LOG_MOD_DEFAULT)) _GDL_LOGD(__VA_ARGS__); } while (0)

#define GDL_LOGXI(...) \
	do { if (gps_dl_log_mod_is_on(GPS_DL_LOG_MOD_DEFAULT)) _GDL_LOGXI(__VA_ARGS__); } while (0)
#define GDL_LOGXD(...) \
	do { if (gps_dl_log_mod_is_on(GPS_DL_LOG_MOD_DEFAULT)) _GDL_LOGXD(__VA_ARGS__); } while (0)

#define GDL_LOGI2(mod, ...) \
		do { if (gps_dl_log_mod_is_on(mod)) _GDL_LOGI(__VA_ARGS__); } while (0)
#define GDL_LOGD2(mod, ...) \
		do { if (gps_dl_log_mod_is_on(mod)) _GDL_LOGD(__VA_ARGS__); } while (0)

#define GDL_LOGXI2(mod, ...) \
		do { if (gps_dl_log_mod_is_on(mod)) _GDL_LOGXI(__VA_ARGS__); } while (0)
#define GDL_LOGXD2(mod, ...) \
		do { if (gps_dl_log_mod_is_on(mod)) _GDL_LOGXD(__VA_ARGS__); } while (0)


#define GDL_ASSERT(cond, ret, fmt, ...)                   \
	do { if (!(cond)) {                                   \
		GDL_LOGE("{** GDL_ASSERT: [(%s) = %d] **}: "fmt,  \
			#cond, (cond), ##__VA_ARGS__);                \
		return ret;                                       \
	} } while (0)

void GDL_VOIDF(void);

#define GDL_ASSERT_RET_OKAY(gdl_ret) \
	GDL_ASSERT((callee_ret) != GDL_OKAY, GDL_FAIL_ASSERT, "")

#define GDL_ASSERT_RET_NOT_ASSERT(callee_ret, ret_to_caller) \
	GDL_ASSERT((callee_ret) == GDL_FAIL_ASSERT, ret_to_caller, "")

#define ASSERT_NOT_ZERO(val, ret)\
	GDL_ASSERT(val != 0, ret, "%s should not be 0!", #val)

#define ASSERT_NOT_NULL(ptr, ret)\
	GDL_ASSERT(ptr != NULL, ret, "null ptr!")

#define ASSERT_LINK_ID(link_id, ret) \
	GDL_ASSERT(LINK_ID_IS_VALID(link_id), ret, "invalid link_id: %d", link_id)

#endif /* _GPS_DL_LOG_H */

