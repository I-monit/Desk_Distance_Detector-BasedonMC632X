/**
 * Copyright (c) 2015-2019 Shanghai Fullhan Microelectronics Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 *
 * Change Logs:
 * Date           Author       Notes
 * 2019-04-12      qin         add license Apache-2.0
 */

/*****************************************************************************
 *  Include Section
 *  add all #include here
 *****************************************************************************/
#include <rtthread.h>
#include "fh_def.h"
#include "fh_arch.h"
#include "irqdesc.h"
#include "fh_ictl.h"
#include "plat_intc.h"
#ifdef RT_USING_PM
#include <pm.h>
#endif

/*****************************************************************************
 * Define section
 * add all #define here
 *****************************************************************************/
#define NR_IRQS    (NR_INTERNAL_IRQS + NR_EXTERNAL_IRQS)
#define INTC_DEV_NAME "intc_dev"


/**
 * This function will mask a interrupt.
 * @param vector the interrupt number
 */
static void intc_mask_irq(unsigned int irq)
{
    fh_intc *p = (fh_intc *)INTC_REG_BASE;

    ictl_mask_isr(p, irq);
}

static void intc_unmask_irq(unsigned int irq)
{
    fh_intc *p = (fh_intc *)INTC_REG_BASE;

    /* Enable irq on AIC */
    ictl_unmask_isr(p, irq);
}

static void intc_handle_irq(void)
{
    rt_uint32_t irqstat_l, irqstat_h, irq;

    fh_intc *p = (fh_intc *)INTC_REG_BASE;

    irqstat_l = p->IRQ_FINALSTATUS_L;
    irqstat_h = p->IRQ_FINALSTATUS_H;
    if (irqstat_l)
    {
        irq = __rt_ffs(irqstat_l) - 1;
    }
    else if (irqstat_h)
    {
        irq = __rt_ffs(irqstat_h) - 1 + 32;
    }
    else
    {
        rt_kprintf("No interrupt occur\n");
        return;
    }

    generic_handle_irq(irq);
}

struct irq_chip intc_chip = {
    .name       = "fh-intc",
    .irq_mask   = intc_mask_irq,
    .irq_unmask = intc_unmask_irq,
};

#ifdef RT_USING_PM
static unsigned int intc_status[4];
extern int rt_hw_irq_suspend(void);
extern void rt_hw_irq_resume(void);
int interrupt_suspend(const struct rt_device *device, rt_uint8_t mode)
{
    fh_intc *p = (fh_intc *)INTC_REG_BASE;

    intc_status[0] = p->IRQ_EN_L;
    intc_status[1] = p->IRQ_EN_H;
    intc_status[2] = p->IRQ_MASK_L;
    intc_status[3] = p->IRQ_MASK_H;

    rt_hw_irq_suspend();

    return RT_EOK;
}

void interrupt_resume(const struct rt_device *device, rt_uint8_t mode)
{
    fh_intc *p = (fh_intc *)INTC_REG_BASE;

    p->IRQ_EN_L = intc_status[0];
    p->IRQ_EN_H = intc_status[1];
    p->IRQ_MASK_L = intc_status[2];
    p->IRQ_MASK_H = intc_status[3];

    rt_hw_irq_resume();
}

struct rt_device_pm_ops intc_pm_ops = {
    .suspend_noirq = interrupt_suspend,
    .resume_noirq = interrupt_resume
};
#endif

#ifdef RT_USING_PM
static struct rt_device interrupt_device;
#endif
void intc_init(void)
{
    int idx;
    fh_intc *p = (fh_intc *)INTC_REG_BASE;

    ictl_close_all_isr(p);

    for (idx = 0; idx < NR_IRQS; idx++)
        irq_set_chip(idx, &intc_chip);

    set_handle_irq(intc_handle_irq);

#ifdef RT_USING_PM
    rt_memcpy(interrupt_device.parent.name, INTC_DEV_NAME, rt_strlen(INTC_DEV_NAME));
    rt_pm_device_register(&interrupt_device, &intc_pm_ops);
#endif
}
