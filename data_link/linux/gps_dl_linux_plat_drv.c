#include "gps_dl_config.h"

#if GPS_DL_HAS_PLAT_DRV
#include <linux/of_reserved_mem.h>

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/platform_device.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/poll.h>

#include <linux/io.h>
#include <asm/io.h>
#include <sync_write.h>

#include <linux/module.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>

#include "gps_dl_linux_plat_drv.h"
#include "gps_dl_isr.h"
#include "gps_each_device.h"

/* #ifdef CONFIG_OF */
const struct of_device_id gps_dl_of_ids[] = {
	{ .compatible = "mediatek,mt6789-gps", },
	{}
};
/* #endif */
#define GPS_DL_IOMEM_NUM 2
struct gps_dl_iomem_addr_map_entry {
	void __iomem *host_virt_addr;
	unsigned int host_phys_addr;
	unsigned int length;
};

struct gps_dl_iomem_addr_map_entry g_gps_dl_iomem_arrary[GPS_DL_IOMEM_NUM];
struct gps_dl_iomem_addr_map_entry g_gps_dl_status_dummy_cr;

void __iomem *gps_dl_host_addr_to_virt(unsigned int host_addr)
{
	int i;
	int offset;
	struct gps_dl_iomem_addr_map_entry *p;

	for (i = 0; i < GPS_DL_IOMEM_NUM; i++) {
		p = &g_gps_dl_iomem_arrary[i];

		if (p->length == 0)
			continue;

		offset = host_addr - p->host_phys_addr;
		if (offset >= 0 && offset < p->length)
			return p->host_virt_addr + offset;
	}

	return (void __iomem *)0;
}

void gps_dl_update_status_for_md_blanking(bool gps_is_on)
{
	void __iomem *p = g_gps_dl_status_dummy_cr.host_virt_addr;
	unsigned int val = (gps_is_on ? 1 : 0);
	unsigned int val_old, val_new;

	if (p != NULL) {
		val_old = __raw_readl(p);
		mt_reg_sync_writel(val, p);
		val_new = __raw_readl(p);
		GDL_LOGI("dummy cr updated: %d -> %d, due to on = %d",
			val_old, val_new, gps_is_on);
	} else
		GDL_LOGW("dummy cr addr is invalid, can not update (on = %d)", gps_is_on);
}

static int gps_dl_probe(struct platform_device *pdev)
{
	struct resource *regs;
	struct resource *irq;
	struct gps_each_device *p_each_dev0 = gps_dl_device_get(0);
	struct gps_each_device *p_each_dev1 = gps_dl_device_get(1);
	void __iomem *p_base;
	int i;
	unsigned int of_ret, status_dummy_cr;

	of_ret = of_property_read_u32(pdev->dev.of_node, "status-dummy-cr", &status_dummy_cr);
	if (of_ret == 0) {
		g_gps_dl_status_dummy_cr.length = 4;
		g_gps_dl_status_dummy_cr.host_phys_addr = status_dummy_cr;
		g_gps_dl_status_dummy_cr.host_virt_addr = devm_ioremap(&pdev->dev,
			status_dummy_cr, g_gps_dl_status_dummy_cr.length);
		gps_dl_update_status_for_md_blanking(false);
	} else {
		g_gps_dl_status_dummy_cr.length = 0;
		g_gps_dl_status_dummy_cr.host_phys_addr = 0;
		g_gps_dl_status_dummy_cr.host_virt_addr = 0;
	}

	GDL_LOGE("status_dummy_cr, of_ret = %d, phy_addr = 0x%08x, size = 0x%x, vir_addr = 0x%p",
		of_ret, g_gps_dl_status_dummy_cr.host_phys_addr,
		g_gps_dl_status_dummy_cr.length, g_gps_dl_status_dummy_cr.host_virt_addr);


	for (i = 0; i < GPS_DL_IOMEM_NUM; i++) {
		regs = platform_get_resource(pdev, IORESOURCE_MEM, i);
		p_base = devm_ioremap(&pdev->dev, regs->start,
						 resource_size(regs));

		g_gps_dl_iomem_arrary[i].host_virt_addr = p_base;
		g_gps_dl_iomem_arrary[i].host_phys_addr = regs->start;
		g_gps_dl_iomem_arrary[i].length = resource_size(regs);

		GDL_LOGE("regs idx = %d, phy_addr = 0x%08x, size = 0x%x, vir_addr = 0x%p",
			i, g_gps_dl_iomem_arrary[i].host_phys_addr,
			g_gps_dl_iomem_arrary[i].length,
			g_gps_dl_iomem_arrary[i].host_virt_addr);
	}

	for (i = 0; i < GPS_DL_IRQ_NUM; i++) {
		irq = platform_get_resource(pdev, IORESOURCE_IRQ, i);
		if (irq == NULL) {
			GDL_LOGE("irq idx = %d, ptr = NULL!", i);
			continue;
		}

		GDL_LOGE("irq idx = %d, start = %lld, end = %lld, name = %s, flag = 0x%x",
			i, irq->start, irq->end, irq->name, irq->flags);
		gps_dl_irq_set_id(i, irq->start);
	}

	GDL_LOGE("do gps_dl_probe\n");
	platform_set_drvdata(pdev, p_each_dev0);
	p_each_dev0->private_data = (struct device *)&pdev->dev;
	p_each_dev1->private_data = (struct device *)&pdev->dev;

	gps_dl_device_context_init();

	return 0;
}

static int gps_dl_remove(struct platform_device *pdev)
{
	struct gps_each_device *p_each_dev = gps_dl_device_get(0);

	GDL_LOGE("do gps dl remove\n");
	platform_set_drvdata(pdev, NULL);
	p_each_dev->private_data = NULL;
	return 0;
}

static int gps_dl_drv_suspend(struct device *dev)
{
#if 0
	struct platform_device *pdev = to_platform_device(dev);
	pm_message_t state = PMSG_SUSPEND;

	return mtk_btif_suspend(pdev, state);
#endif
	return 0;
}

static int gps_dl_drv_resume(struct device *dev)
{
#if 0
	struct platform_device *pdev = to_platform_device(dev);

	return mtk_btif_resume(pdev);
#endif
	return 0;
}

static int gps_dl_plat_suspend(struct platform_device *pdev, pm_message_t state)
{
#if 0
	int i_ret = 0;
	struct _mtk_btif_ *p_btif = NULL;

	BTIF_DBG_FUNC("++\n");
	p_btif = platform_get_drvdata(pdev);
	i_ret = _btif_suspend(p_btif);
	BTIF_DBG_FUNC("--, i_ret:%d\n", i_ret);
	return i_ret;
#endif
	return 0;
}

static int gps_dl_plat_resume(struct platform_device *pdev)
{
#if 0
	int i_ret = 0;
	struct _mtk_btif_ *p_btif = NULL;

	BTIF_DBG_FUNC("++\n");
	p_btif = platform_get_drvdata(pdev);
	i_ret = _btif_resume(p_btif);
	BTIF_DBG_FUNC("--, i_ret:%d\n", i_ret);
	return i_ret;
#endif
	return 0;
}


const struct dev_pm_ops mtk_btif_drv_pm_ops = {
	.suspend = gps_dl_drv_suspend,
	.resume = gps_dl_drv_resume,
};

struct platform_driver gps_dl_dev_drv = {
	.probe = gps_dl_probe,
	.remove = gps_dl_remove,
/* #ifdef CONFIG_PM */
	.suspend = gps_dl_plat_suspend,
	.resume = gps_dl_plat_resume,
/* #endif */
	.driver = {
			.name = "gps", /* mediatek,gps */
			.owner = THIS_MODULE,
/* #ifdef CONFIG_PM */
			.pm = &mtk_btif_drv_pm_ops,
/* #endif */
/* #ifdef CONFIG_OF */
			.of_match_table = gps_dl_of_ids,
/* #endif */
	}
};

static ssize_t driver_flag_read(struct device_driver *drv, char *buf)
{
	return sprintf(buf, "gps dl driver debug level:%d\n", 1);
}

static ssize_t driver_flag_set(struct device_driver *drv,
				   const char *buffer, size_t count)
{


	GDL_LOGE("buffer = %s, count = %zd\n", buffer, count);

	return count;
}

#define DRIVER_ATTR(_name, _mode, _show, _store) \
	struct driver_attribute driver_attr_##_name = \
	__ATTR(_name, _mode, _show, _store)
static DRIVER_ATTR(flag, 0644, driver_flag_read, driver_flag_set);






int gps_dl_linux_plat_drv_register(void)
{
	int result;

	result = platform_driver_register(&gps_dl_dev_drv);
	/* if (result) */
	GDL_LOGE("platform_driver_register, ret(%d)\n", result);

	result = driver_create_file(&gps_dl_dev_drv.driver, &driver_attr_flag);
	/* if (result) */
	GDL_LOGE("driver_create_file, ret(%d)\n", result);

	return 0;
}

int gps_dl_linux_plat_drv_unregister(void)
{
	driver_remove_file(&gps_dl_dev_drv.driver, &driver_attr_flag);
	platform_driver_unregister(&gps_dl_dev_drv);

	return 0;
}

phys_addr_t g_gps_emi_mem_base;
int g_gps_emi_mem_size;

/*Reserved memory by device tree!*/

int reserve_memory_gps_fn(struct reserved_mem *rmem)
{
	GDL_LOGE("[W]%s: name: %s,base: 0x%llx,size: 0x%llx\n",
		__func__, rmem->name, (unsigned long long)rmem->base,
		(unsigned long long)rmem->size);
	g_gps_emi_mem_base = rmem->base;
	g_gps_emi_mem_size = rmem->size;
	return 0;
}

RESERVEDMEM_OF_DECLARE(gps_reserve_memory_test, "mediatek,gps-reserve-memory",
			reserve_memory_gps_fn);

#endif /* GPS_DL_HAS_PLAT_DRV */

