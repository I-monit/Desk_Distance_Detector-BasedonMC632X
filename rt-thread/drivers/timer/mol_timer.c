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
#include "fh_arch.h"
#include "board.h"
#include "timer.h"
#include "mol_timer.h"
#include "fh_def.h"
#include <rthw.h>
#ifdef RT_USING_PM
#include <pm.h>
#endif

#define TIMER_DEV_NAME "timer_dev"

struct mol_timer 
{
    unsigned int base;
    unsigned int chn;
    unsigned int width;
    enum timer_mode mode;
};

static timer g_timer = {.rate = TIMER_CLOCK};

timer *get_timer(void)
{
    return &g_timer;
}

int timer_init(timer *tim)
{
    struct mol_timer *timer = tim->base;
    unsigned int val;

    val = GET_REG(timer->base + TMR_CTRL_OFFSET(timer->chn));
    val &= ~BIT(TMR_WIDTH_SHIFT);
    val |= timer->width << TMR_WIDTH_SHIFT;
    SET_REG(timer->base + TMR_CTRL_OFFSET(timer->chn), val);
    return 0;
}

int timer_set_mode(timer *tim, enum timer_mode mode)
{
    struct mol_timer *timer = tim->base;
    unsigned int val;

    timer->mode = mode;
    val = GET_REG(timer->base + TMR_CTRL_OFFSET(timer->chn));
    if (mode == TIMER_MODE_PERIODIC)
        val |= BIT(TMR_MODE_SHIFT);
    else
        val &= ~BIT(TMR_MODE_SHIFT);
    SET_REG(timer->base + TMR_CTRL_OFFSET(timer->chn), val);

    return 0;
}

void timer_set_period(timer *tim, unsigned int period, unsigned int clock)
{
    struct mol_timer *timer = tim->base;
    unsigned long long val;
    unsigned int low;
    unsigned int high;

    val = clock / period;
    low = (unsigned int)val;
    high = (unsigned int)(val >> 32);

    if (timer->width == TMR_WIDTH_64B)
    {
        SET_REG(timer->base + TMR_LOAD_LO_OFFSET(timer->chn), low);
        SET_REG(timer->base + TMR_LOAD_HI_OFFSET(timer->chn), high);
    }
    else
        SET_REG(timer->base + TMR_LOAD_LO_OFFSET(timer->chn), low);
}

void timer_set_count(timer *tim, unsigned int count)
{
    struct mol_timer *timer = tim->base;
    unsigned int low;
    unsigned int high;

    low = (unsigned int)count;
    high = 0;

    if (timer->width == TMR_WIDTH_64B)
    {
        SET_REG(timer->base + TMR_LOAD_LO_OFFSET(timer->chn), low);
        SET_REG(timer->base + TMR_LOAD_HI_OFFSET(timer->chn), high);
    }
    else
        SET_REG(timer->base + TMR_LOAD_LO_OFFSET(timer->chn), low);
}

void timer_enable(timer *tim)
{
    struct mol_timer *timer = tim->base;
    unsigned int val;

    val = GET_REG(timer->base + TMR_CTRL_OFFSET(timer->chn));
    val &= ~BIT(TMR_RUN_SHIFT);
    val |= (TMR_ENABLE) << TMR_RUN_SHIFT;
    SET_REG(timer->base + TMR_CTRL_OFFSET(timer->chn), val);
}

void timer_disable(timer *tim)
{ 
    struct mol_timer *timer = tim->base;
    unsigned int val;

    val = GET_REG(timer->base + TMR_CTRL_OFFSET(timer->chn));
    val &= ~BIT(TMR_RUN_SHIFT);
    val |= (TMR_DISABLE) << TMR_RUN_SHIFT;
    SET_REG(timer->base + TMR_CTRL_OFFSET(timer->chn), val);
}

void timer_enable_irq(timer *tim)
{ 
    struct mol_timer *timer = tim->base;
    unsigned int val;

    val = GET_REG(timer->base + TMR_INT_OFFSET(timer->chn));
    val &= ~BIT(TMR_IRQEN_SHIFT);
    val &= ~BIT(TMR_IRQ_RAW_SHIFT);
    val |= (TMR_IRQ_EN) << TMR_IRQEN_SHIFT;
    val |= (TMR_IRQ_RAW_EN) << TMR_IRQ_RAW_SHIFT;
    SET_REG(timer->base + TMR_INT_OFFSET(timer->chn), val);
}

void timer_disable_irq(timer *tim)
{ 
    struct mol_timer *timer = tim->base;
    unsigned int val;

    val = GET_REG(timer->base + TMR_INT_OFFSET(timer->chn));
    val &= ~BIT(TMR_IRQEN_SHIFT);
    val &= ~BIT(TMR_IRQ_RAW_SHIFT);
    val |= (TMR_IRQ_DIS) << TMR_IRQEN_SHIFT;
    SET_REG(timer->base + TMR_INT_OFFSET(timer->chn), val);
}

unsigned int timer_get_eoi(timer *tim)
{
    struct mol_timer *timer = tim->base;
    unsigned int val;

    val = GET_REG(timer->base + TMR_INT_OFFSET(timer->chn));
    val |= (TMR_IRQ_CLR) << TMR_CLR_SHIFT;
    SET_REG(timer->base + TMR_INT_OFFSET(timer->chn), val);
    return 0;
}

void timer_set_irq_handler(timer *tim, void (*handler)(void))
{
    tim->irq_handler = handler;
}

void rt_timer_handler(int vector, void *param)
{
    timer *tim = param;

    timer_get_eoi(tim);
    /* when timekeeping is enabled */
    if (tim->irq_handler)
        tim->irq_handler();
    else
        rt_tick_increase();
}

#ifdef RT_USING_PM
static int timer_regs[4];

static int timer_suspend(const struct rt_device *device, rt_uint8_t mode)
{
    struct mol_timer *timer = (struct mol_timer *)g_timer.base;

    timer_regs[0] = GET_REG(timer->base + TMR_LOAD_LO_OFFSET(timer->chn));
    timer_regs[1] = GET_REG(timer->base + TMR_LOAD_HI_OFFSET(timer->chn));
    timer_regs[2] = GET_REG(timer->base + TMR_CTRL_OFFSET(timer->chn));
    timer_regs[3] = GET_REG(timer->base + TMR_INT_OFFSET(timer->chn));

    return RT_EOK;
}

static void timer_resume(const struct rt_device *device, rt_uint8_t mode)
{
    struct mol_timer *timer = (struct mol_timer *)g_timer.base;

    timer_disable(&g_timer);
    SET_REG(timer->base + TMR_LOAD_LO_OFFSET(timer->chn), timer_regs[0]);
    SET_REG(timer->base + TMR_LOAD_HI_OFFSET(timer->chn), timer_regs[1]);
    SET_REG(timer->base + TMR_INT_OFFSET(timer->chn), timer_regs[3]);
    SET_REG(timer->base + TMR_CTRL_OFFSET(timer->chn), timer_regs[2]);
}

struct rt_device_pm_ops mol_timer_pm_ops = {
    .suspend_noirq = timer_suspend,
    .resume_noirq = timer_resume
};
#endif

/**
 * This function will init pit for system ticks
 */
static struct rt_device timer_device;
void rt_hw_timer_init(void)
{
    struct mol_timer *timer = rt_malloc(sizeof(*timer));
    if (!timer)
    {
        rt_kprintf("Timer Malloc Failed!\n");
        return;
    }
    rt_memset(timer, 0, sizeof(*timer));

    timer->base = TMR_REG_BASE;
    timer->chn = TMR_CHANNEL_0;
    timer->width = TMR_WIDTH_32B;
    g_timer.base = (void *)timer;
    timer_init(&g_timer);
    rt_hw_interrupt_install(TMR0_IRQn, rt_timer_handler, &g_timer, "sys_tick");
    rt_hw_interrupt_umask(TMR0_IRQn);
    timer_disable(&g_timer);
    timer_enable_irq(&g_timer);
    timer_set_mode(&g_timer, TIMER_MODE_PERIODIC);
    timer_set_period(&g_timer, RT_TICK_PER_SECOND, TIMER_CLOCK);
    timer_enable(&g_timer);

#ifdef RT_USING_PM
    rt_memcpy(timer_device.parent.name, TIMER_DEV_NAME, rt_strlen(TIMER_DEV_NAME));
    rt_pm_device_register(&timer_device, &mol_timer_pm_ops);
#endif
}
