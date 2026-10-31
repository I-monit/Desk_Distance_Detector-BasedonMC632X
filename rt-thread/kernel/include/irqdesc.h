/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
   2020-02-27                  First Version
 */

#ifndef __IRQDESC_H__
#define __IRQDESC_H__

#include <rtthread.h>
#include <rtdef.h>
#include <rthw.h>

/*
 * Interrupt handler definition
 */

struct rt_irq_desc *irq_to_desc(unsigned int irq);
rt_isr_handler_t rt_hw_interrupt_handle(rt_uint32_t vector, void *param);
void rt_hw_interrupt_init(void);
rt_isr_handler_t rt_hw_interrupt_install(int vector, rt_isr_handler_t handler, void *param, const char *name);
int generic_handle_irq(unsigned int irq);
void set_handle_irq(void (*handle_irq)(void));
int irq_set_chip(unsigned int irq, struct irq_chip *chip);
#ifdef RT_USING_INTERRUPT_INFO
int irq_in_process(void);
#endif

#endif