#include "gps_each_device.h"
#include "gps_each_link.h"
#if GPS_DL_MOCK_HAL
#include "gps_mock_mvcd.h"
#endif
#include "gps_dl_hal.h"
#include "gps_dl_ctrld.h"
#include "gps_data_link_devices.h"
#include "gps_dl_subsys_reset.h"

static ssize_t gps_each_device_read(struct file *filp,
	char __user *buf, size_t count, loff_t *f_pos)
{
	int retval;
	int i_len;
	struct gps_each_device *dev;

	dev = (struct gps_each_device *)filp->private_data;

	GDL_LOGXW(dev->index, "buf_len = %d, pid = %d", count, current->pid);

#if GPS_DL_HAS_LINK_LAYER
	i_len = gps_each_link_read((enum gps_dl_link_id_enum)dev->index,
		&dev->i_buf[0], count);
	if (i_len < 0)
		return i_len;
	retval = copy_to_user(buf, &dev->i_buf[0], i_len);
#else
	if (dev->i_len == 0) {
#if 0
		if (filp->f_flags & O_NONBLOCK) {
			retval = -EAGAIN;
			return retval;
		}
#endif
		retval = wait_event_interruptible(dev->r_wq, dev->i_len > 0);
		if (retval) {
			if (-ERESTARTSYS == retval)
				GDL_LOGXI(dev->index, "signaled by -ERESTARTSYS(%ld)\n ", retval);
			else
				GDL_LOGXI(dev->index, "signaled by %ld\n ", retval);

			return retval;
		}
	}
#if GPS_DL_CTRLD_MOCK_LINK_LAYER
	/* TODO: check buffer size */
	i_len = dev->i_len;
	retval = copy_to_user(buf, &dev->i_buf[0], i_len);
	dev->i_len = 0;
#else
	i_len = gps_each_link_receive_data(&dev->i_buf[0], GPS_DL_RX_BUFFER_SIZE, dev->index);
	retval = copy_to_user(buf, &dev->i_buf[0], i_len);
	dev->i_len = 0;
#endif /* GPS_DL_CTRLD_MOCK_LINK_LAYER */
#endif /* GPS_DL_HAS_LINK_LAYER */

	if (retval)
		return -EFAULT;

	GDL_LOGXI(dev->index, "ret_len = %d", i_len);
	return i_len;
}

static ssize_t gps_each_device_write(struct file *filp,
	const char __user *buf, size_t count, loff_t *f_pos)
{
	int retval;
	int copy_size;
	struct gps_each_device *dev;

	dev = (struct gps_each_device *)filp->private_data;

	GDL_LOGXW(dev->index, "len = %d, pid = %d", count, current->pid);

	if (count > 0) {
		/* TODO: this size GPS_DATA_PATH_BUF_MAX for what? */
		copy_size = (count > GPS_DATA_PATH_BUF_MAX) ? GPS_DATA_PATH_BUF_MAX : count;
		retval = copy_from_user(&dev->o_buf[0], &buf[0], copy_size);
		if (retval)
			return -EFAULT;

#if GPS_DL_HAS_LINK_LAYER
		gps_each_link_write((enum gps_dl_link_id_enum)dev->index,
			&dev->o_buf[0], copy_size);
#elif GPS_DL_CTRLD_MOCK_LINK_LAYER
		gps_each_link_send_data(&dev->o_buf[0], copy_size, dev->index);
#else
		mock_output(&dev->o_buf[0], copy_size, dev->index);
#endif
	}

	return count;
}

#if 0
void gps_each_device_data_submit(unsigned char *buf, unsigned int len, int index)
{
	struct gps_each_device *dev;

	dev = gps_dl_device_get(index);

	GDL_LOGI("gps_each_device_data_submit len = %d, index = %d, dev = %p",
		len, index, dev);

	if (!dev)
		return;

	if (!dev->is_open)
		return;

#if GPS_DL_CTRLD_MOCK_LINK_LAYER
	/* TODO: using mutex, len check */
	memcpy(&dev->i_buf[0], buf, len);
	dev->i_len = len;
	wake_up(&dev->r_wq);
#else
	gps_dl_add_to_rx_queue(buf, len, index);
	/* wake_up(&dev->r_wq); */
#endif

	GDL_LOGI("gps_each_device_data_submit copy and wakeup done");
}
#endif

static int gps_each_device_open(struct inode *inode, struct file *filp)
{
	struct gps_each_device *dev; /* device information */
	int retval;

	dev = container_of(inode->i_cdev, struct gps_each_device, cdev);
	filp->private_data = dev; /* for other methods */

	GDL_LOGXW(dev->index, "major = %d, minor = %d, pid = %d",
		imajor(inode), iminor(inode), current->pid);

	if (!dev->is_open) {
		retval = gps_each_link_open((enum gps_dl_link_id_enum)dev->index);

		if (0 == retval)
			dev->is_open = true;
		else
			return retval;
	}

	return 0;
}

static int gps_each_device_release(struct inode *inode, struct file *filp)
{
	struct gps_each_device *dev;

	dev = (struct gps_each_device *)filp->private_data;
	dev->is_open = false;

	GDL_LOGXW(dev->index, "major = %d, minor = %d, pid = %d",
		imajor(inode), iminor(inode), current->pid);

	gps_each_link_close((enum gps_dl_link_id_enum)dev->index);

	return 0;
}

#define GPSDL_IOC_GPS_HWVER            6
#define GPSDL_IOC_GPS_IC_HW_VERSION    7
#define GPSDL_IOC_GPS_IC_FW_VERSION    8
#define GPSDL_IOC_D1_EFUSE_GET         9
#define GPSDL_IOC_RTC_FLAG             10
#define GPSDL_IOC_CO_CLOCK_FLAG        11
#define GPSDL_IOC_TRIGGER_ASSERT       12
#define GPSDL_IOC_QUERY_STATUS         13
#define GPSDL_IOC_TAKE_GPS_WAKELOCK    14
#define GPSDL_IOC_GIVE_GPS_WAKELOCK    15
#define GPSDL_IOC_GET_GPS_LNA_PIN      16
#define GPSDL_IOC_GPS_FWCTL            17

static int gps_each_device_ioctl_inner(struct file *filp, unsigned int cmd, unsigned long arg, bool is_compat)
{
	struct gps_each_device *dev; /* device information */
	int retval;

#if 0
	dev = container_of(inode->i_cdev, struct gps_each_device, cdev);
	filp->private_data = dev; /* for other methods */
#endif
	dev = (struct gps_each_device *)(filp->private_data);

	GDL_LOGXD(dev->index, "cmd = %d, is_compat = %d", cmd, is_compat);
#if 0
	int retval = 0;
	ENUM_WMTHWVER_TYPE_T hw_ver_sym = WMTHWVER_INVALID;
	UINT32 hw_version = 0;
	UINT32 fw_version = 0;
	UINT32 gps_lna_pin = 0;
#endif
	switch (cmd) {
	case GPSDL_IOC_TRIGGER_ASSERT:
		/* Trigger FW assert for debug */
		GDL_LOGXD(dev->index, "GPSDL_IOC_TRIGGER_ASSERT, reason = %d", arg);

		/* TODO: assert dev->is_open */
		if (dev->index == GPS_DATA_LINK_ID0)
			retval = gps_dl_trigger_gps_subsys_reset(false);
		else
			retval = gps_each_link_reset(dev->index);
		break;
	case GPSDL_IOC_QUERY_STATUS:
		retval = gps_each_link_check(dev->index);
		GDL_LOGXD(dev->index, "GPSDL_IOC_QUERY_STATUS, status = %d", retval);
		break;
#if 0
	case GPSDL_IOC_GPS_HWVER:
		/*get combo hw version */
		/* hw_ver_sym = mtk_wcn_wmt_hwver_get(); */

		GPS_DBG_FUNC("GPS_ioctl(): get hw version = %d, sizeof(hw_ver_sym) = %zd\n",
			      hw_ver_sym, sizeof(hw_ver_sym));
		if (copy_to_user((int __user *)arg, &hw_ver_sym, sizeof(hw_ver_sym)))
			retval = -EFAULT;

		break;
	case GPSDL_IOC_GPS_IC_HW_VERSION:
		/*get combo hw version from ic,  without wmt mapping */
		hw_version = mtk_wcn_wmt_ic_info_get(WMTCHIN_HWVER);

		GPS_DBG_FUNC("GPS_ioctl(): get hw version = 0x%x\n", hw_version);
		if (copy_to_user((int __user *)arg, &hw_version, sizeof(hw_version)))
			retval = -EFAULT;

		break;

	case GPSDL_IOC_GPS_IC_FW_VERSION:
		/*get combo fw version from ic, without wmt mapping */
		fw_version = mtk_wcn_wmt_ic_info_get(WMTCHIN_FWVER);

		GPS_DBG_FUNC("GPS_ioctl(): get fw version = 0x%x\n", fw_version);
		if (copy_to_user((int __user *)arg, &fw_version, sizeof(fw_version)))
			retval = -EFAULT;

		break;
	case GPSDL_IOC_RTC_FLAG:

		retval = rtc_GPS_low_power_detected();

		GPS_DBG_FUNC("low power flag (%d)\n", retval);
		break;
	case GPSDL_IOC_CO_CLOCK_FLAG:
#if SOC_CO_CLOCK_FLAG
		retval = mtk_wcn_wmt_co_clock_flag_get();
#endif
		GPS_DBG_FUNC("GPS co_clock_flag (%d)\n", retval);
		break;
	case GPSDL_IOC_D1_EFUSE_GET:
#if defined(CONFIG_MACH_MT6735)
		do {
			char *addr = ioremap(0x10206198, 0x4);

			retval = *(unsigned int *)addr;
			GPS_DBG_FUNC("D1 efuse (0x%x)\n", retval);
			iounmap(addr);
		} while (0);
#elif defined(CONFIG_MACH_MT6763)
		do {
			char *addr = ioremap(0x11f10048, 0x4);

			retval = *(unsigned int *)addr;
			GPS_DBG_FUNC("MT6763 efuse (0x%x)\n", retval);
			iounmap(addr);
		} while (0);
#else
		GPS_ERR_FUNC("Read Efuse not supported in this platform\n");
#endif
		break;

	case GPSDL_IOC_TAKE_GPS_WAKELOCK:
		GPS_INFO_FUNC("Ioctl to take gps wakelock\n");
		gps_hold_wake_lock(1);
		if (wake_lock_acquired == 1)
			retval = 0;
		else
			retval = -EAGAIN;
		break;
	case GPSDL_IOC_GIVE_GPS_WAKELOCK:
		GPS_INFO_FUNC("Ioctl to give gps wakelock\n");
		gps_hold_wake_lock(0);
		if (wake_lock_acquired == 0)
			retval = 0;
		else
			retval = -EAGAIN;
		break;
#ifdef GPS_FWCTL_SUPPORT
	case GPSDL_IOC_GPS_FWCTL:
		GPS_INFO_FUNC("GPSDL_IOC_GPS_FWCTL\n");
		retval = GPS_fwctl((struct gps_fwctl_data *)arg);
		break;
#endif
	case GPSDL_IOC_GET_GPS_LNA_PIN:
		gps_lna_pin = mtk_wmt_get_gps_lna_pin_num();
		GPS_DBG_FUNC("GPS_ioctl(): get gps lna pin = %d\n", gps_lna_pin);
		if (copy_to_user((int __user *)arg, &gps_lna_pin, sizeof(gps_lna_pin)))
			retval = -EFAULT;
		break;
#endif
	default:
		retval = -EFAULT;
		GDL_LOGXW(dev->index, "cmd = %d, not support", cmd);
		break;
	}

	return retval;
}


static long gps_each_device_unlocked_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	return gps_each_device_ioctl_inner(filp, cmd, arg, false);
}

static long gps_each_device_compat_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	return gps_each_device_ioctl_inner(filp, cmd, arg, true);
}

static const struct file_operations gps_each_device_fops = {
	.owner = THIS_MODULE,
	.open = gps_each_device_open,
	.read = gps_each_device_read,
	.write = gps_each_device_write,
	.release = gps_each_device_release,
	.unlocked_ioctl = gps_each_device_unlocked_ioctl,
	.compat_ioctl = gps_each_device_compat_ioctl,
};

int gps_dl_cdev_setup(struct gps_each_device *dev, int index)
{
	int result;

	init_waitqueue_head(&dev->r_wq);
	dev->i_len = 0;

	dev->index = index;
	/* assert dev->index == dev->cfg.index */

	cdev_init(&dev->cdev, &gps_each_device_fops);
	dev->cdev.owner = THIS_MODULE;
	dev->cdev.ops = &gps_each_device_fops;

	result = cdev_add(&dev->cdev, dev->devno, 1);
	if (result) {
		GDL_LOGE("cdev_add error %d on index %d", result, index);
		return result;
	}

	dev->cls = class_create(THIS_MODULE, dev->cfg.dev_name);
	if (IS_ERR(dev->cls)) {
		GDL_LOGE("class_create fail on %s", dev->cfg.dev_name);
		return -1;
	}

	dev->dev = device_create(dev->cls, NULL, dev->devno, NULL, dev->cfg.dev_name);
	if (IS_ERR(dev->dev)) {
		GDL_LOGE("device_create fail on %s", dev->cfg.dev_name);
		return -1;
	}

	return 0;
}

void gps_dl_cdev_cleanup(struct gps_each_device *dev, int index)
{
	if (dev->dev) {
		device_destroy(dev->cls, dev->devno);
		dev->dev = NULL;
	}

	if (dev->cls) {
		class_destroy(dev->cls);
		dev->cls = NULL;
	}

	cdev_del(&dev->cdev);
}

