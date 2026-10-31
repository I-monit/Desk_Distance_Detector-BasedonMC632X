/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
   2020-02-27                  First Version
 */

#include <rtthread.h>
#include <rtdef.h>
#include <rthw.h>
#include <irqdesc.h>
#include <interrupt.h>

#ifndef MAX_HANDLERS
#define MAX_HANDLERS       512
#endif

#define INTC_FLAG_WAKEUP        0x1
#define INTC_FLAG_MASK          0x2
#define INTC_FLAG_UMASK         0x4

#ifndef RT_USING_SMP
extern rt_uint8_t rt_interrupt_nest;
#endif
extern void (*handle_arch_irq)(void);

struct rt_irq_desc irq_desc[MAX_HANDLERS];
rt_uint32_t rt_interrupt_from_thread, rt_interrupt_to_thread;
rt_uint32_t rt_thread_switch_interrupt_flag;

/**
 * This function will initialize hardware interrupt
 */

struct rt_irq_desc *irq_to_desc(unsigned int irq)
{
    return &irq_desc[irq];
}

rt_isr_handler_t rt_hw_interrupt_handle(rt_uint32_t vector, void *param)
{
    rt_kprintf("Unhandled interrupt %d occurred!!!\n", vector);
    return RT_NULL;
}

int rt_hw_enable_irq_wake(unsigned int irq)
{
    register rt_base_t level;

    if (irq >= MAX_HANDLERS)
        return -1;

    level = rt_hw_interrupt_disable();
    irq_desc[irq].intc_flag |= INTC_FLAG_WAKEUP;
    rt_hw_interrupt_enable(level);

    return 0;
}

int rt_hw_disable_irq_wake(unsigned int irq)
{
    register rt_base_t level;

    if (irq >= MAX_HANDLERS)
        return -1;

    level = rt_hw_interrupt_disable();
    irq_desc[irq].intc_flag &= (~INTC_FLAG_WAKEUP);
    rt_hw_interrupt_enable(level);

    return 0;
}

#ifdef RT_USING_PM
void rt_hw_irq_suspend(void)
{
    int idx = 0;

    for (idx = 0; idx < MAX_HANDLERS; idx++)
    {
        if (!(irq_desc[idx].intc_flag & (INTC_FLAG_MASK | INTC_FLAG_WAKEUP)))
        {
            irq_desc[idx].intc_flag |= INTC_FLAG_UMASK;
            rt_hw_interrupt_mask(idx);
        }
    }
}

void rt_hw_irq_resume(void)
{
    int idx = 0;

    for (idx = 0; idx < MAX_HANDLERS; idx++)
    {
        if (irq_desc[idx].intc_flag & (INTC_FLAG_UMASK | INTC_FLAG_WAKEUP))
        {
            rt_hw_interrupt_umask(idx);
            irq_desc[idx].intc_flag &= (~INTC_FLAG_UMASK);
        }
    }
}
#endif

void rt_hw_interrupt_init(void)
{
    register rt_uint32_t idx;

    rt_hw_vector_init();

    for (idx = 0; idx < MAX_HANDLERS; idx++)
    {
        irq_desc[idx].handler = (rt_isr_handler_t)rt_hw_interrupt_handle;
        irq_desc[idx].param   = RT_NULL;
        irq_desc[idx].data    = RT_NULL;
#ifdef RT_USING_INTERRUPT_INFO
        rt_snprintf(irq_desc[idx].name, RT_NAME_MAX - 1, "default");
        irq_desc[idx].counter = 0;
        irq_desc[idx].process = RT_FALSE;
#endif
        irq_desc[idx].intc_flag = INTC_FLAG_MASK;
    }
    /* init interrupt nest, and context in thread sp */
#ifndef RT_USING_SMP
    rt_interrupt_nest               = 0;
#endif
    rt_interrupt_from_thread        = 0;
    rt_interrupt_to_thread          = 0;
    rt_thread_switch_interrupt_flag = 0;
}

/**
 * This function will install a interrupt service routine to a interrupt.
 * @param vector the interrupt number
 * @param new_handler the interrupt service routine to be installed
 * @param old_handler the old interrupt service routine
 */
rt_isr_handler_t rt_hw_interrupt_install(int vector, rt_isr_handler_t handler,
    void *param, const char *name)
{
    rt_isr_handler_t old_handler = RT_NULL;

    if (vector < MAX_HANDLERS)
    {
        old_handler = irq_desc[vector].handler;

        if (handler != RT_NULL)
        {
#ifdef RT_USING_INTERRUPT_INFO
            rt_strncpy(irq_desc[vector].name, name, RT_NAME_MAX);
#endif /* RT_USING_INTERRUPT_INFO */
            irq_desc[vector].handler = handler;
            irq_desc[vector].param = param;
        }
    }

    return old_handler;
}

void set_handle_irq(void (*handle_irq)(void))
{
    if (handle_arch_irq)
        return;

    handle_arch_irq = handle_irq;
}

/**
 * generic_handle_irq - Invoke the handler for a particular irq
 * @irq:    The irq number to handle
 *
 */
extern int trigger_uart_output_nolock(void);
int generic_handle_irq(unsigned int irq)
{
#ifdef RT_USING_SMP
    register int id = rt_hw_cpu_id() & 3;
#else
    register int id = 0;
#endif /* RT_USING_SMP */


    struct rt_irq_desc *desc = irq_to_desc(irq);
#ifdef RT_USING_INTERRUPT_INFO
    desc->counter++;
    desc->process = RT_TRUE;
#endif
    desc->handler(irq, desc->param);
    if (id == 0)
        trigger_uart_output_nolock();
#ifdef RT_USING_INTERRUPT_INFO
    desc->process = RT_FALSE;
#endif

    return 0;
}

void rt_hw_interrupt_mask(int irq)
{
    register rt_base_t temp;
    struct rt_irq_desc *desc = irq_to_desc(irq);
    struct irq_chip *chip;

    if (desc)
    {
        chip = desc->data;
    }
    temp = rt_hw_interrupt_disable();
    if (chip && chip->irq_mask)
    {
        chip->irq_mask(irq);
        irq_desc[irq].intc_flag |= INTC_FLAG_MASK;
    }
    rt_hw_interrupt_enable(temp);
}
RTM_EXPORT(rt_hw_interrupt_mask);

void rt_hw_interrupt_umask(int irq)
{
    register rt_base_t temp;
    struct rt_irq_desc *desc = irq_to_desc(irq);
    struct irq_chip *chip;

    if (desc)
        chip = desc->data;

    temp = rt_hw_interrupt_disable();
    if (chip && chip->irq_unmask)
    {
        chip->irq_unmask(irq);
        irq_desc[irq].intc_flag &= (~INTC_FLAG_MASK);
    }
    rt_hw_interrupt_enable(temp);
}
RTM_EXPORT(rt_hw_interrupt_umask);

/**
 * irq_set_chip - set the irq chip for an irq
 * @irq:    irq number
 * @chip:   pointer to irq chip description structure
 */
int irq_set_chip(unsigned int irq, struct irq_chip *chip)
{
    struct rt_irq_desc *desc = irq_to_desc(irq);

    desc->data = chip;

    return 0;
}
RTM_EXPORT(irq_set_chip);

#ifdef RT_USING_INTERRUPT_INFO
int irq_in_process(void)
{
    int vector;
    struct rt_irq_desc *desc;

    for (vector = 0; vector < MAX_HANDLERS; vector++)
    {
        desc = irq_to_desc(vector);
        if (desc->process)
            break;
    }

    return vector;

}
RTM_EXPORT(irq_in_process);
#endif

#ifdef RT_USING_FINSH
void list_irq(void)
{
#ifdef RT_USING_INTERRUPT_INFO
    int irq;

    rt_kprintf("number\tcount\tname\n");
    for (irq = 0; irq < MAX_HANDLERS; irq++)
    {
        if (rt_strncmp(irq_desc[irq].name, "default", sizeof("default")))
        {
            rt_kprintf("%02ld: %10ld  %s\n", irq, irq_desc[irq].counter,
                       irq_desc[irq].name);
        }
    }
#else
    rt_kprintf("please open 'RT_USING_INTERRUPT_INFO'\n");

#endif
}
#include <finsh.h>
FINSH_FUNCTION_EXPORT(list_irq, list system irq);
#endif
