#include "gps_dl_config.h"

#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/proc_fs.h>
#include "gps_dl_osal.h"
#include "gps_dl_procfs.h"
#include "gps_each_link.h"
#include "gps_dl_subsys_reset.h"

int gps_dl_procfs_trigger_reset(int par1, int par2, int par3)
{
	if (par2 == 0)
		gps_dl_trigger_connsys_reset();
	else if (par2 == 1)
		gps_dl_trigger_gps_subsys_reset((bool)par3);
	else if (par2 == 2 && (par3 >= 0 && par3 <= GPS_DATA_LINK_NUM))
		gps_each_link_reset(par3);

	return 0;
}

gps_dl_procfs_test_func_type g_gps_dl_proc_test_func_list[] = {
	[0x00] = gps_dl_procfs_trigger_reset,
};

#define UNLOCK_MAGIC 0xDB9DB9
#define PROCFS_WR_BUF_SIZE 256
ssize_t gps_dl_procfs_write(struct file *filp, const char __user *buffer, size_t count, loff_t *f_pos)
{
	size_t len = count;
	char buf[PROCFS_WR_BUF_SIZE];
	char *pBuf;
	char *pDelimiter = " \t";
	int x = 0, y = 0, z = 0;
	char *pToken = NULL;
	long res;

	/* static bool testEnabled; */

	GDL_LOGD("write parameter len = %d", (int)len);
	if (len >= PROCFS_WR_BUF_SIZE) {
		GDL_LOGE("input handling fail!\n");
		return -1;
	}

	if (copy_from_user(buf, buffer, len))
		return -EFAULT;

	buf[len] = '\0';
	GDL_LOGD("write parameter data = %s", buf);

	pBuf = buf;
	pToken = osal_strsep(&pBuf, pDelimiter);
	if (pToken != NULL) {
		osal_strtol(pToken, 16, &res);
		x = (int)res;
	} else {
		x = 0;
	}

	pToken = osal_strsep(&pBuf, "\t\n ");
	if (pToken != NULL) {
		osal_strtol(pToken, 16, &res);
		y = (int)res;
		GDL_LOGE("y = 0x%08x\n\r", y);
	} else {
		y = 3000;
		/*efuse, register read write default value */
		if (0x11 == x || 0x12 == x || 0x13 == x)
			y = 0x80000000;
	}

	pToken = osal_strsep(&pBuf, "\t\n ");
	if (pToken != NULL) {
		osal_strtol(pToken, 16, &res);
		z = (int)res;
	} else {
		z = 10;
		/*efuse, register read write default value */
		if (0x11 == x || 0x12 == x || 0x13 == x)
			z = 0xffffffff;
	}

	GDL_LOGI("x(0x%08x), y(0x%08x), z(0x%08x)\n\r", x, y, z);

	/* For eng and userdebug load, have to enable wmt_dbg by
	 * writing 0xDB9DB9 to * "/proc/driver/wmt_dbg" to avoid
	 * some malicious use
	 */
#if 0
	if (x == UNLOCK_MAGIC) {
		dbgEnabled = 1;
		return len;
	}
#endif

	if (ARRAY_SIZE(g_gps_dl_proc_test_func_list) > x && NULL != g_gps_dl_proc_test_func_list[x])
		(*g_gps_dl_proc_test_func_list[x]) (x, y, z);
	else
		GDL_LOGW("no handler defined for command id(0x%08x)\n\r", x);

	return len;
}

ssize_t gps_dl_procfs_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	return 0;
}

static const struct file_operations gps_dl_procfs_fops = {
	.owner = THIS_MODULE,
	.read = gps_dl_procfs_read,
	.write = gps_dl_procfs_write,
};

static struct proc_dir_entry *g_gps_dl_procfs_entry;
#define GPS_DL_PROCFS_NAME "driver/gpsdl_dbg"

int gps_dl_procfs_setup(void)
{

	int i_ret = 0;

	g_gps_dl_procfs_entry = proc_create(GPS_DL_PROCFS_NAME,
		0664, NULL, &gps_dl_procfs_fops);

	if (g_gps_dl_procfs_entry == NULL) {
		GDL_LOGE("Unable to create gps proc entry");
		i_ret = -1;
	}

	return i_ret;
}

int gps_dl_procfs_remove(void)
{
	if (g_gps_dl_procfs_entry != NULL)
		proc_remove(g_gps_dl_procfs_entry);
	return 0;
}

