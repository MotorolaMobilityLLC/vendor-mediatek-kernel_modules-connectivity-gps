#include "gps_dl_linux.h"
#include "gps_dl_hal.h"
#include "gps_dl_log.h"

void gps_dl_irq_mask(int irq_id, enum gps_dl_irq_ctrl_from from)
{
	/* TODO: */
	/* threaded_irq thread_fn still need to call disable_irq_nosync,
	 * otherwise it might be hung.
	 */
	if (from == GPS_DL_IRQ_CTRL_FROM_ISR)
		disable_irq_nosync(irq_id);
	else
		/* It returns until irq done */
		disable_irq(irq_id);
}

void gps_dl_irq_unmask(int irq_id, enum gps_dl_irq_ctrl_from from)
{
	/* TODO: */
	enable_irq(irq_id);
}

static int gps_dl_linux_irq_index_to_id(enum gps_dl_irq_index_enum index)
{
	/* TODO: fill the real number or the value from dts */
	return gps_dl_irq_index_to_id(index);
}

/* TODO: call it when module init */
int gps_dl_linux_irqs_register(struct gps_each_irq *p_irqs, int irq_num)
{
	int irq_id, i_ret, i;
	unsigned int sys_irq_trig_type;

	for (i = 0; i < irq_num; i++) {
		irq_id = gps_dl_linux_irq_index_to_id(p_irqs[i].cfg.index);

		if (irq_id == 0) {
			GDL_LOGE("i = %d, irq_id = %d, name = %s, bypass",
				i, irq_id, p_irqs[i].cfg.name);
			continue;
		}

		if (p_irqs[i].cfg.trig_type == GPS_DL_IRQ_TRIG_LEVEL_HIGH)
			sys_irq_trig_type = IRQF_TRIGGER_HIGH;
		else if (p_irqs[i].cfg.trig_type == GPS_DL_IRQ_TRIG_EDGE_RISING)
			sys_irq_trig_type = IRQF_TRIGGER_RISING;
		else
			return -1;

		/* TODO: Need to mask the irq here? */
		/* TODO: Use the dts to auto request the irqs */
#if GPS_DL_USE_THREADED_IRQ
		/* IRQF_ONESHOT is required for threaded irq */
		i_ret = request_threaded_irq(irq_id, NULL,
			(irq_handler_t)p_irqs[i].cfg.isr,
			sys_irq_trig_type | IRQF_ONESHOT,
			p_irqs[i].cfg.name, &p_irqs[i]);
#else
		i_ret = request_irq(irq_id,
			(irq_handler_t)p_irqs[i].cfg.isr, /* gps_dl_linux_irq_dispatcher */
			sys_irq_trig_type, p_irqs[i].cfg.name, &p_irqs[i]);
#endif
		/* The init status is unmask, mask them here */
		gps_dl_irq_mask(irq_id, GPS_DL_IRQ_CTRL_FROM_THREAD);

		GDL_LOGE("i = %d, irq_id = %d, name = %s, ret = %d",
			i, irq_id, p_irqs[i].cfg.name, i_ret);

		if (i_ret) {
			/* show error log */
			/* return i_ret; */
			continue; /* not stop even fail */
		}

		p_irqs[i].register_done = true;
		p_irqs[i].reg_irq_id = irq_id;
	}

	return 0;
}

int gps_dl_linux_irqs_unregister(struct gps_each_irq *p_irqs, int irq_num)
{
	int irq_id, i;

	for (i = 0; i < irq_num; i++) {
		if (p_irqs[i].register_done) {
			irq_id = gps_dl_linux_irq_index_to_id(p_irqs[i].cfg.index);
			/* assert irq_id = p_irqs[i].reg_irq_id */
			free_irq(irq_id, &p_irqs[i]);

			p_irqs[i].register_done = false;
			p_irqs[i].reg_irq_id = 0;
		}
	}

	return 0;
}

irqreturn_t gps_dl_linux_irq_dispatcher(int irq, void *data)
{
	struct gps_each_irq *p_irq;

	p_irq = (struct gps_each_irq *)data;

	switch (p_irq->cfg.index) {
	case GPS_DL_IRQ_LINK0_DATA:
		gps_dl_isr_usrt_has_data(GPS_DATA_LINK_ID0);
		break;
	case GPS_DL_IRQ_LINK0_NODATA:
		gps_dl_isr_usrt_has_nodata(GPS_DATA_LINK_ID0);
		break;
	case GPS_DL_IRQ_LINK0_MCUB:
		gps_dl_isr_mcub(GPS_DATA_LINK_ID0);
		break;

	case GPS_DL_IRQ_LINK1_DATA:
		gps_dl_isr_usrt_has_data(GPS_DATA_LINK_ID1);
		break;
	case GPS_DL_IRQ_LINK1_NODATA:
		gps_dl_isr_usrt_has_nodata(GPS_DATA_LINK_ID1);
		break;
	case GPS_DL_IRQ_LINK1_MCUB:
		gps_dl_isr_mcub(GPS_DATA_LINK_ID1);
		break;

	case GPS_DL_IRQ_DMA:
		gps_dl_isr_dma_done();
		break;

	default:
		break;
	}

	return IRQ_HANDLED;
}

