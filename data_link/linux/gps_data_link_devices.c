#include <linux/dma-mapping.h>
#include <linux/of_device.h>

#include <linux/device.h>
#include <linux/platform_device.h>

#include "gps_dl_config.h"
#include "gps_dl_context.h"
#include "gps_dl_hw_api.h"
#include "gps_dl_isr.h"
#include "gps_data_link_devices.h"
#include "gps_each_link.h"
#if GPS_DL_HAS_PLAT_DRV
#include "gps_dl_linux_plat_drv.h"
#endif
#if GPS_DL_MOCK_HAL
#include "gps_mock_hal.h"
#endif
#include "gps_dl_procfs.h"

#define GPS_DATA_LINK_DEV_NAME "gps_data_link_cdev"
int gps_dl_devno_major;
int gps_dl_devno_minor;

void gps_dl_dma_buf_free(struct gps_dl_dma_buf *p_dma_buf, int dev_index)
{
	struct gps_each_device *p_dev;

	p_dev = gps_dl_device_get(dev_index);
	if (p_dma_buf->vir_addr)
		dma_free_coherent(p_dev->dev,
			p_dma_buf->len, p_dma_buf->vir_addr, p_dma_buf->phy_addr);

	memset(p_dma_buf, 0, sizeof(*p_dma_buf));
}

int gps_dl_dma_buf_alloc(struct gps_dl_dma_buf *p_dma_buf, int dev_index,
	enum gps_dl_dma_dir dir, unsigned int len)
{
	struct gps_each_device *p_dev;
	struct device *p_linux_plat_dev;

	p_dev = gps_dl_device_get(dev_index);
	p_linux_plat_dev = (struct device *)p_dev->private_data;

	memset(p_dma_buf, 0, sizeof(*p_dma_buf));
	p_dma_buf->dev_index = dev_index;
	p_dma_buf->dir = dir;
	p_dma_buf->len = len;

	GDL_LOGI("p_linux_plat_dev = 0x%p", p_linux_plat_dev);
	if (p_linux_plat_dev == NULL) {
		p_dma_buf->vir_addr = dma_zalloc_coherent(
			p_dev->dev, len, &p_dma_buf->phy_addr, GFP_DMA | GFP_DMA32);
	} else {
		p_dma_buf->vir_addr = dma_zalloc_coherent(
			p_linux_plat_dev, len, &p_dma_buf->phy_addr, GFP_DMA);/* | GFP_DMA32); */
	}

	if (sizeof(dma_addr_t) == sizeof(unsigned long long))
		GDL_LOGI("alloc gps dl dma buf(%d,%d) in arch64, addr: vir=0x%p, phy=0x%llx, len=%u\n",
			p_dma_buf->dev_index, p_dma_buf->dir,
			p_dma_buf->vir_addr, p_dma_buf->phy_addr, p_dma_buf->len);
	else
		GDL_LOGI("alloc gps dl dma buf(%d,%d) in arch32, addr: vir=0x%p, phy=0x%08x, len=%u\n",
			p_dma_buf->dev_index, p_dma_buf->dir,
			p_dma_buf->vir_addr, p_dma_buf->phy_addr, p_dma_buf->len);


	if (NULL == p_dma_buf->vir_addr) {
		GDL_LOGXE(dev_index, "alloc gps dl dma buf(%d,%d)(len = %u) fail\n", dev_index, dir, len);
		/* force move forward even fail */
		/* return -ENOMEM; */
	}

	return 0;
}

void gps_dl_ctx_links_deinit(void)
{
	enum gps_dl_link_id_enum link_id;

	struct gps_each_device *p_dev;
	struct gps_each_link *p_link;

	for (link_id = 0; link_id < GPS_DATA_LINK_NUM; link_id++) {
		p_dev = gps_dl_device_get(link_id);
		p_link = gps_dl_link_get(link_id);

		gps_dl_dma_buf_free(&p_link->rx_dma_buf, link_id);

		/* un-binding each device and link */
		p_link->p_device = NULL;
		p_dev->p_link = NULL;
		gps_each_link_deinit(link_id);
	}
}

int gps_dl_ctx_links_init(void)
{
	int retval;
	enum gps_dl_link_id_enum link_id;
	struct gps_each_device *p_dev;
	struct gps_each_link *p_link;
	enum gps_each_link_waitable_type j;

	for (link_id = 0; link_id < GPS_DATA_LINK_NUM; link_id++) {
		p_dev = gps_dl_device_get(link_id);
		p_link = gps_dl_link_get(link_id);

		of_dma_configure(p_dev->dev, p_dev->dev->of_node);

		if (!p_dev->dev->coherent_dma_mask)
			p_dev->dev->coherent_dma_mask = DMA_BIT_MASK(32);

		if (!p_dev->dev->dma_mask)
			p_dev->dev->dma_mask = &p_dev->dev->coherent_dma_mask;


		retval = gps_dl_dma_buf_alloc(
			&p_link->tx_dma_buf, link_id, GDL_DMA_A2D, p_link->cfg.tx_buf_size);

		if (retval)
			return retval;

		retval = gps_dl_dma_buf_alloc(
			&p_link->rx_dma_buf, link_id, GDL_DMA_D2A, p_link->cfg.rx_buf_size);

		if (retval)
			return retval;

		for (j = 0; j < GPS_DL_WAIT_NUM; j++)
			gps_dl_link_waitable_init(&p_link->waitables[j], j);
		/* binding each device and link */
		p_link->p_device = p_dev;
		p_dev->p_link = p_link;


		/* Todo: MNL read buf is 512, here is work-around */
		/* the solution should be make on gdl_dma_buf_get */
		gps_dl_set_rx_transfer_max(link_id, GPS_LIBMNL_READ_MAX);
		gps_each_link_init(link_id);
	}

	/* TODO: move it to open, rather at init */
	gps_dl_remap_ctx_cal_and_set();

	return 0;
}

static void gps_dl_devices_exit(void)
{
	int i;
	dev_t devno = MKDEV(gps_dl_devno_major, gps_dl_devno_minor);

	gps_dl_device_context_deinit();

#if GPS_DL_HAS_PLAT_DRV
	gps_dl_linux_plat_drv_unregister();
#endif

	for (i = 0; i < GPS_DATA_LINK_NUM; i++)
		gps_dl_cdev_cleanup(gps_dl_device_get(i), i);

	unregister_chrdev_region(devno, GPS_DATA_LINK_NUM);
}

void gps_dl_device_context_deinit(void)
{
	gps_dl_procfs_remove();

	gps_dl_irq_deinit();

#if GPS_DL_HAS_CTRLD
	gps_dl_ctrld_deinit();
#endif

#if GPS_DL_MOCK_HAL
	gps_dl_mock_deinit();
#endif

	gps_dl_ctx_links_deinit();
}

#if GPS_DL_CTRLD_MOCK_LINK_LAYER
void gps_dl_rx_event_cb(int data_link_num)
{
	struct gps_each_device *device;

	if (data_link_num >= GPS_DATA_LINK_NUM)
		return;
	device = gps_dl_device_get(data_link_num);

	if (device != NULL) {
		GDL_LOGI("wake up read thread\n ");
		wake_up(&(device->r_wq));
		device->i_len = 1;
	} else
		GDL_LOGI("device NULL\n ");
}
#endif

int gps_dl_irq_init(void)
{
#if 0
	enum gps_dl_irq_index_enum irq_idx;

	for (irq_idx = 0; irq_idx < GPS_DL_IRQ_NUM; irq_idx++)
		;
#endif

	gps_dl_linux_irqs_register(gps_dl_irq_get(0), GPS_DL_IRQ_NUM);

	return 0;
}

int gps_dl_irq_deinit(void)
{
	gps_dl_linux_irqs_unregister(gps_dl_irq_get(0), GPS_DL_IRQ_NUM);
	return 0;
}

static int gps_dl_devices_init(void)
{
	int result, i;
	dev_t devno = 0;
	struct gps_each_device *p_dev;

	result = alloc_chrdev_region(&devno, gps_dl_devno_minor,
		GPS_DATA_LINK_NUM, GPS_DATA_LINK_DEV_NAME);

	gps_dl_devno_major = MAJOR(devno);

	if (result < 0) {
		GDL_LOGE("fail to get major %d\n", gps_dl_devno_major);
		return result;
	}

	GDL_LOGE("success to get major %d\n", gps_dl_devno_major);

	for (i = 0; i < GPS_DATA_LINK_NUM; i++) {
		devno = MKDEV(gps_dl_devno_major, gps_dl_devno_minor + i);
		p_dev = gps_dl_device_get(i);
		p_dev->devno = devno;
		result = gps_dl_cdev_setup(p_dev, i);
		if (result) {
			/* error happened */
			gps_dl_devices_exit();
			return result;
		}
	}


#if GPS_DL_HAS_PLAT_DRV
	gps_dl_linux_plat_drv_register();
#else
	gps_dl_device_context_init();
#endif

	return 0;
}

void gps_dl_device_context_init(void)
{
	gps_dl_ctx_links_init();

#if GPS_DL_MOCK_HAL
	gps_dl_mock_init();
#endif

#if GPS_DL_HAS_CTRLD
	gps_dl_ctrld_init();
#endif

#if (!(GPS_DL_NO_USE_IRQ || GPS_DL_HW_IS_MOCK))
	/* must after gps_dl_ctx_links_init */
	gps_dl_irq_init();
#endif

	gps_dl_procfs_setup();
}

void mtk_gps_data_link_devices_exit(void)
{
	GDL_LOGI("mtk_gps_data_link_devices_exit");
	gps_dl_devices_exit();
}

int mtk_gps_data_link_devices_init(void)
{
	GDL_LOGI("mtk_gps_data_link_devices_init");
	gps_dl_devices_init();
	GDL_ASSERT(false, 0, "test assert");
	return 0;
}

