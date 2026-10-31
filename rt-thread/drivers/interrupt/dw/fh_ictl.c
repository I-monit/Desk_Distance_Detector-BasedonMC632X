/**
 * Copyright (c) 2015-2019 Shanghai Fullhan Microelectronics Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 *
 * Change Logs:
 * Date           Author       Notes
 * 2019-04-12               the first version
 *
 */

#include <rtthread.h>
#include <fh_ictl.h>

void ictl_close_all_isr(fh_intc *p)
{
    if (p)
    {
        /* enable all interrupts */
        p->IRQ_EN_L = 0xffffffff;
        p->IRQ_EN_H = 0xffffffff;
        /* mask all  interrupts */
        p->IRQ_MASK_L = 0xffffffff;
        p->IRQ_MASK_H = 0xffffffff;
    }
}

void ictl_mask_isr(fh_intc *p, int irq)
{
    if (p)
    {
        if (irq < 32)
            p->IRQ_MASK_L |= (1 << irq);
        else
            p->IRQ_MASK_H |= (1 << (irq - 32));
    }
}

void ictl_unmask_isr(fh_intc *p, int irq)
{
    if (p)
    {
        if (irq < 32)
            p->IRQ_MASK_L &= ~(1 << irq);
        else
            p->IRQ_MASK_H &= ~(1 << (irq - 32));
    }
}
