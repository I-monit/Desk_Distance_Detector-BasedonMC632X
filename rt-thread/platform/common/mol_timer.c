/**
 * Copyright (c) 2015-2019 Shanghai Fullhan Microelectronics Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 *
 * Change Logs:
 * Date           Author       Notes
 * 2019-04-12                  add license Apache-2.0
 */


#include <rtdevice.h>
#include "fh_arch.h"
#include "mol_timer.h"
#ifdef RT_USING_TIMEKEEPING
#include <timekeeping.h>
#include <hrtimer.h>
#endif

#include "fh_chip.h"
#include "fh_clock.h"
#include "board.h"
#include "delay.h"
#include "timer.h"
#include "fh_pmu.h"


#define PAE_PTS_CLK (1000000)
typedef uint64_t cycle_t;



#ifdef RT_USING_TIMEKEEPING
static cycle_t fh_pts_read(struct clocksource *cs)
{
#else
static cycle_t fh_pts_read(void *cs)
{
#endif
    return fh_get_pts64();
}
uint64_t read_pts(void) {return fh_pts_read(NULL);}

#ifdef RT_USING_TIMEKEEPING
struct clocksource clocksource_pts = {
    .name = "pts", .rating = 299, .read = fh_pts_read, .mask = 0xffffffff,
};

void clocksource_pts_register(void)
{
    struct clk *ptsclk = clk_get(NULL, "pts_clk");

    if (ptsclk != RT_NULL)
        clk_enable(ptsclk);

    fh_pmu_set_user();
    clocksource_register_hz(&clocksource_pts, PAE_PTS_CLK);
}

static int mol_set_next_event(unsigned long cycles,
                             struct clock_event_device *dev)
{
    unsigned int val;
    unsigned int mask;

    val = (TMR_DISABLE) << TMR_RUN_SHIFT;
    mask = BIT(TMR_RUN_SHIFT);
    SET_REG_M(TMR_REG_BASE + TMR_CTRL_OFFSET(0), val, mask);

    SET_REG(TMR_REG_BASE + TMR_LOAD_LO_OFFSET(0), TIMER_CLOCK / RT_TICK_PER_SECOND);
    val = (TMR_ENABLE) << TMR_RUN_SHIFT;
    SET_REG(TMR_REG_BASE + TMR_CTRL_OFFSET(0), val);

    val = (TMR_ENABLE) << TMR_RUN_SHIFT;
    mask = BIT(TMR_RUN_SHIFT);
    SET_REG_M(TMR_REG_BASE + TMR_CTRL_OFFSET(0), val, mask);

    return 0;
}

static void mol_set_mode(enum clock_event_mode mode,
                        struct clock_event_device *dev)
{
    unsigned int val;
    unsigned int mask;

    switch (mode)
    {
    case CLOCK_EVT_MODE_PERIODIC:
        val = (TMR_DISABLE) << TMR_RUN_SHIFT;
        mask = BIT(TMR_RUN_SHIFT);
        SET_REG_M(TMR_REG_BASE + TMR_CTRL_OFFSET(0), val, mask);

        val = TIMER_MODE_PERIODIC << TMR_MODE_SHIFT;
        mask = BIT(TMR_MODE_SHIFT);
        SET_REG_M(TMR_REG_BASE + TMR_CTRL_OFFSET(0), val, mask);

        SET_REG(TMR_REG_BASE + TMR_LOAD_LO_OFFSET(0), TIMER_CLOCK / RT_TICK_PER_SECOND);
        val = (TMR_ENABLE) << TMR_RUN_SHIFT;
        SET_REG_M(TMR_REG_BASE + TMR_CTRL_OFFSET(0), val, mask);

        val = (TMR_ENABLE) << TMR_RUN_SHIFT;
        mask = BIT(TMR_RUN_SHIFT);
        SET_REG_M(TMR_REG_BASE + TMR_CTRL_OFFSET(0), val, mask);
        break;
    case CLOCK_EVT_MODE_ONESHOT:
        val = (TMR_DISABLE) << TMR_RUN_SHIFT;
        mask = BIT(TMR_RUN_SHIFT);
        SET_REG_M(TMR_REG_BASE + TMR_CTRL_OFFSET(0), val, mask);

        val = TIMER_MODE_ONESHOT << TMR_MODE_SHIFT;
        mask = BIT(TMR_MODE_SHIFT);
        SET_REG_M(TMR_REG_BASE + TMR_CTRL_OFFSET(0), val, mask);

        SET_REG(TMR_REG_BASE + TMR_LOAD_LO_OFFSET(0), TIMER_CLOCK / RT_TICK_PER_SECOND);
        val = (TMR_ENABLE) << TMR_RUN_SHIFT;
        SET_REG(TMR_REG_BASE + TMR_CTRL_OFFSET(0), val);

        val = (TMR_ENABLE) << TMR_RUN_SHIFT;
        mask = BIT(TMR_RUN_SHIFT);
        SET_REG_M(TMR_REG_BASE + TMR_CTRL_OFFSET(0), val, mask);
        break;
    case CLOCK_EVT_MODE_UNUSED:
    case CLOCK_EVT_MODE_SHUTDOWN:
        val = (TMR_DISABLE) << TMR_RUN_SHIFT;
        mask = BIT(TMR_RUN_SHIFT);
        SET_REG_M(TMR_REG_BASE + TMR_CTRL_OFFSET(0), val, mask);
        break;
    case CLOCK_EVT_MODE_RESUME:
        val = (TMR_ENABLE) << TMR_RUN_SHIFT;
        mask = BIT(TMR_RUN_SHIFT);
        SET_REG_M(TMR_REG_BASE + TMR_CTRL_OFFSET(0), val, mask);
        break;
    }
}

static struct clock_event_device clockevent_timer0 = {
    .name           = "mol_clockevent",
    .features       = CLOCK_EVT_FEAT_PERIODIC | CLOCK_EVT_FEAT_ONESHOT,
    .shift          = 32,
    .set_next_event = mol_set_next_event,
    .set_mode       = mol_set_mode,
};

void clockevent_handler(int vector, void *param)
{
    unsigned int val;

    clockevent_timer0.event_handler(&clockevent_timer0);
    val = GET_REG(TMR_REG_BASE + TMR_INT_OFFSET(0));
    val |= (TMR_IRQ_CLR) << TMR_CLR_SHIFT;
    SET_REG(TMR_REG_BASE + TMR_INT_OFFSET(0), val);
}

void clockevent_timer0_register(void)
{
    clockevent_timer0.mult =
        div_sc(TIMER_CLOCK, NSEC_PER_SEC, clockevent_timer0.shift);
    clockevent_timer0.max_delta_ns =10000000;
        /* clockevent_delta2ns(0xffffffff, &clockevent_timer0); */
    clockevent_timer0.min_delta_ns =10000;
       /* clockevent_delta2ns(0xf, &clockevent_timer0);*/
    clockevents_register_device(&clockevent_timer0);

    rt_hw_interrupt_install(TMR0_IRQn, clockevent_handler,
                            (void *)&clockevent_timer0, "sys_tick");
}
#endif

