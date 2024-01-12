#include <linux/init.h>
#include <linux/module.h>

#include "gps_dl_config.h"

#if GPS_DL_IS_MODULE
#include "gps_data_link_devices.h"

/*****************************************************************************/
static int __init gps_dl_mod_init(void)
{
	int ret = 0;

	mtk_gps_data_link_devices_init();

	return ret;
}

/*****************************************************************************/
static void __exit gps_dl_mod_exit(void)
{
	mtk_gps_data_link_devices_exit();
}

module_init(gps_dl_mod_init);
module_exit(gps_dl_mod_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Hua Fu");
#endif

