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

/*****************************************************************************
 *  Include Section
 *  add all #include here
 *****************************************************************************/
// #include "inc/fh_driverlib.h"
#include <rtthread.h>
#include "fh_arch.h"
#include "fh_timer.h"
#include "board.h"
#include "timer.h"
#include "fh_clock.h"
#include <rthw.h>
#ifdef RT_USING_PM
#include <pm.h>
#endif

/*****************************************************************************
 * Define section
 * add all #define here
 *****************************************************************************/

/****************************************************************************
 * ADT section
 *  add definition of user defined Data Type that only be used in this file here
 ***************************************************************************/

/******************************************************************************
 * Function prototype section
 * add prototypes for all functions called by this file,execepting those
 * declared in header file
 *****************************************************************************/

/*****************************************************************************
 * Global variables section - Exported
 * add declaration of global variables that will be exported here
 * e.g.
 *  int8_t foo;
 ****************************************************************************/

/*****************************************************************************
 * Global variables section - Local
 * define global variables(will be refered only in this file) here,
 * static keyword should be used to limit scope of local variable to this file
 * e.g.
 *  static uint8_t ufoo;
 *****************************************************************************/

/* function body */

/*****************************************************************************
 * Description:
 *      add funtion description here
 * Parameters:
 *      description for each argument, new argument starts at new line
 * Return:
 *      what does this function returned?
 *****************************************************************************/

#define TIMER_DEV_NAME "timer_dev"

static timer g_timer = {.rate = TIMER_CLOCK, .base = (void *)TMR_REG_BASE};

int timer_init(timer *tim)
{
    fh_timer *t = (fh_timer *)tim->base;

    t->TIMER_CTRL_REG = 0;
    return 0;
}

int timer_set_mode(timer *tim, enum timer_mode mode)
{
    fh_timer *t = (fh_timer *)tim->base;
    switch (mode)
    {
    case TIMER_MODE_PERIODIC:
    case TIMER_MODE_ONESHOT:
        t->TIMER_CTRL_REG |= TIMER_CTRL_MODE;
        break;
    default:
        rt_kprintf("Not support TIMER mode\n");
        return -1;
        break;
    }

    return 0;
}

void timer_set_period(timer *tim, unsigned int period, unsigned int clock)
{
    fh_timer *t = (fh_timer *)tim->base;

    t->TIMER_LOAD_COUNT = clock / period;
}

void timer_enable(timer *tim)
{
    fh_timer *t = (fh_timer *)tim->base;

    t->TIMER_CTRL_REG |= TIMER_CTRL_ENABLE;
    /*解决timer跨时钟域问题*/
}

void timer_waitfor_disable(int id)
{
    int sync_cnt = 0;

    /* zy/ticket/100 : update apb Timer LOADCNT */
    /* CURRENTVALE could,t be start from new LOADCOUNT */
    /* cause is timer clk 1M hz and apb is 150M hz */
    /* check current cnt for it is disabled */
    while (GET_REG(REG_TIMER_CUR_VAL(id)) != 0)
    {
        sync_cnt++;
        if (sync_cnt >= 50)
        {
            /* typical cnt is 5 when in 1M timer clk */
            /* so here use 50 to check whether it is err */
            rt_kprintf("timer %x problem,can't disable\n",id);
        }
    }


}

void timer_disable(timer *tim)
{
    fh_timer *t = (fh_timer *)tim->base;

    t->TIMER_CTRL_REG &= ~TIMER_CTRL_ENABLE;
    timer_waitfor_disable(0);
}
void timer_enable_irq(timer *tim)
{
    fh_timer *t = (fh_timer *)tim->base;

    t->TIMER_CTRL_REG &= ~TIMER_CTRL_INTMASK;
}

void timer_disable_irq(timer *tim)
{
    fh_timer *t = (fh_timer *)tim->base;

    t->TIMER_CTRL_REG |= TIMER_CTRL_INTMASK;
}

unsigned int timer_get_status(timer *tim)
{
    fh_timer *t = (fh_timer *)tim->base;

    return t->TIMER_INT_STATUS;
}

unsigned int timer_get_eoi(timer *tim)
{
    fh_timer *t = (fh_timer *)tim->base;

    return t->TIMER_EOI;
}

void timer_set_count(timer *tim, unsigned int count)
{
    fh_timer *t = (fh_timer *)tim->base;

    t->TIMER_LOAD_COUNT = count;
}

unsigned int timer_get_value(timer *tim)
{
    fh_timer *t = (fh_timer *)tim->base;

    return t->TIMER_LOAD_COUNT - t->TIMER_CURRENT_VALUE;
}

void rt_timer_handler(int vector, void *param)
{
    timer *tim = (timer *)param;
    unsigned int int_status;

    int_status = GET_REG(tim->base + REG_TIMERS_INTSTATUS);
    if (int_status & TIMER_INTSTS_TIMER(0))
    {
        timer_get_eoi(tim);
        /* when timekeeping is enabled */
        if (tim->irq_handler)
            tim->irq_handler();
        else
            rt_tick_increase();
    }

}

timer *get_timer(void)
{
    return &g_timer;
}

void timer_set_irq_handler(timer *tim, void (*handler)(void))
{
    tim->irq_handler = handler;
}

#ifdef RT_USING_PM
static int timer_regs[3];

static int timer_suspend(const struct rt_device *device, rt_uint8_t mode)
{
    fh_timer *t = (fh_timer *)g_timer.base;

    timer_regs[0] = t->TIMER_LOAD_COUNT;
    timer_regs[1] = t->TIMER_CURRENT_VALUE;
    timer_regs[2] = t->TIMER_CTRL_REG;

    return RT_EOK;
}

static void timer_resume(const struct rt_device *device, rt_uint8_t mode)
{
    fh_timer *t = (fh_timer *)g_timer.base;

    timer_disable(&g_timer);
    t->TIMER_LOAD_COUNT = timer_regs[0];
    t->TIMER_CURRENT_VALUE = timer_regs[1];
    t->TIMER_CTRL_REG = timer_regs[2];
}

struct rt_device_pm_ops fh_timer_pm_ops = {
    .suspend_noirq = timer_suspend,
    .resume_noirq = timer_resume
};
#endif

/**
 * This function will init pit for system ticks
 */
#ifdef RT_USING_PM
static struct rt_device timer_device;
#endif
void rt_hw_timer_init(void)
{
    timer *tim = &g_timer;
    struct clk *tmrclk = clk_get(NULL, "tmr_clk");

    if (tmrclk != RT_NULL)
    {
        clk_set_rate(tmrclk, TIMER_CLOCK);
        clk_enable(tmrclk);
    }

    timer_init(tim);
    /* install interrupt handler */
    rt_hw_interrupt_install(TMR0_IRQn, rt_timer_handler, (void *)tim,
                            "sys_tick");
    rt_hw_interrupt_umask(TMR0_IRQn);
    timer_enable_irq(tim);
    timer_set_mode(tim, TIMER_MODE_PERIODIC);
    timer_set_period(tim, RT_TICK_PER_SECOND, TIMER_CLOCK);
    /*timer_enable_irq(tim);*/
    timer_enable(tim);

#ifdef RT_USING_PM
    rt_memcpy(timer_device.parent.name, TIMER_DEV_NAME, rt_strlen(TIMER_DEV_NAME));
    rt_pm_device_register(&timer_device, &fh_timer_pm_ops);
#endif
}
