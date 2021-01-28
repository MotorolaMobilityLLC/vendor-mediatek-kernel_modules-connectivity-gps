#ifndef _GPS_DL_LINUX_H
#define _GPS_DL_LINUX_H

#include "linux/interrupt.h"
#include "gps_dl_isr.h"
#include "gps_each_link.h"

/* put linux proprietary items here otherwise put into gps_dl_osal.h */
irqreturn_t gps_dl_linux_irq_dispatcher(int irq, void *data);
int gps_dl_linux_irqs_register(struct gps_each_irq *p_irqs, int irq_num);
int gps_dl_linux_irqs_unregister(struct gps_each_irq *p_irqs, int irq_num);

#endif /* _GPS_DL_LINUX_H */

