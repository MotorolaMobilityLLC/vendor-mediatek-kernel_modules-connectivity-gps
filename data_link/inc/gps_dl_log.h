#ifndef _GPS_DL_LOG_H
#define _GPS_DL_LOG_H

#include "gps_dl_config.h"

#if GPS_DL_ON_LINUX
#include <linux/printk.h>
#define GDL_LOGE(fmt, ...) pr_notice("GDL[E] [%s: %d]: "fmt, __func__, __LINE__, ##__VA_ARGS__)
#define GDL_LOGW(fmt, ...) pr_notice("GDL[W] [%s: %d]: "fmt, __func__, __LINE__, ##__VA_ARGS__)
#define GDL_LOGI(fmt, ...) pr_info("GDL[I] [%s: %d]: "fmt, __func__, __LINE__, ##__VA_ARGS__)
#define GDL_LOGD(fmt, ...) pr_info("GDL[D] [%s: %d]: "fmt, __func__, __LINE__, ##__VA_ARGS__)

#define GDL_LOGXE(link_id, fmt, ...) pr_notice("GDL-%d[E] [%s: %d]: "fmt, \
	link_id, __func__, __LINE__, ##__VA_ARGS__)

#define GDL_LOGXW(link_id, fmt, ...) pr_notice("GDL-%d[W] [%s: %d]: "fmt, \
		link_id, __func__, __LINE__, ##__VA_ARGS__)

#define GDL_LOGXI(link_id, fmt, ...) pr_info("GDL-%d[I] [%s: %d]: "fmt, \
	link_id, __func__, __LINE__, ##__VA_ARGS__)

#define GDL_LOGXD(link_id, fmt, ...) pr_info("GDL-%d[D] [%s: %d]: "fmt, \
	link_id, __func__, __LINE__, ##__VA_ARGS__)
#elif GPS_DL_ON_CTP
#include "gps_dl_ctp_log.h"
#endif /* GPS_DL_ON_XX */


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

