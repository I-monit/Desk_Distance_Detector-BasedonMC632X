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
#include "timer.h"
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

static timer *timer0;

static int fh_set_next_event(unsigned long cycles,
                             struct clock_event_device *dev)
{
    timer_disable(timer0);
    timer_set_count(timer0, cycles);
    timer_enable(timer0);
    return 0;
}

static void fh_set_mode(enum clock_event_mode mode,
                        struct clock_event_device *dev)
{
    switch (mode)
    {
    case CLOCK_EVT_MODE_PERIODIC:
    case CLOCK_EVT_MODE_ONESHOT:
        timer_disable(timer0);
        timer_set_count(timer0, timer0->rate / RT_TICK_PER_SECOND);
        if (mode == CLOCK_EVT_MODE_PERIODIC)
            timer_set_mode(timer0, TIMER_MODE_PERIODIC);
        else
            timer_set_mode(timer0, TIMER_MODE_ONESHOT);
        timer_enable(timer0);
        break;
    case CLOCK_EVT_MODE_UNUSED:
    case CLOCK_EVT_MODE_SHUTDOWN:
        timer_disable(timer0);
        break;
    case CLOCK_EVT_MODE_RESUME:
        timer_set_mode(timer0, TIMER_MODE_PERIODIC);
        timer_enable(timer0);
        break;
    }
}

static struct clock_event_device clockevent_timer0 = {
    .name           = "fh_clockevent",
    .features       = CLOCK_EVT_FEAT_PERIODIC | CLOCK_EVT_FEAT_ONESHOT,
    .shift          = 32,
    .set_next_event = fh_set_next_event,
    .set_mode       = fh_set_mode,
};

void clockevent_handler(void)
{
    clockevent_timer0.event_handler(&clockevent_timer0);
}

void clockevent_timer0_register(void)
{
    timer0 = get_timer();
    clockevent_timer0.mult =
        div_sc(timer0->rate, NSEC_PER_SEC, clockevent_timer0.shift);
    clockevent_timer0.max_delta_ns =10000000;
        /* clockevent_delta2ns(0xffffffff, &clockevent_timer0); */
    clockevent_timer0.min_delta_ns =10000;
       /* clockevent_delta2ns(0xf, &clockevent_timer0);*/
    clockevents_register_device(&clockevent_timer0);

    timer_set_irq_handler(timer0, clockevent_handler);


}
#endif

