/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author        Notes
 * 2019-02-01    qin&songyh     first version
 */

#ifndef __CLOCKSOURCE_H__
#define __CLOCKSOURCE_H__

#include <stdint.h>
#include <rtthread.h>

/*
 * Clock source flags bits::
 */
#define CLOCK_SOURCE_IS_CONTINUOUS 0x01
#define CLOCK_SOURCE_MUST_VERIFY 0x02

#define CLOCK_SOURCE_WATCHDOG 0x10
#define CLOCK_SOURCE_VALID_FOR_HRES 0x20
#define CLOCK_SOURCE_UNSTABLE 0x40

#ifndef ____cacheline_aligned
#define ____cacheline_aligned __attribute__((__aligned__(32)))
#endif

typedef uint64_t cycle_t;

struct clocksource
{
    /*
     * Hotpath data, fits in a single cache line when the
     * clocksource itself is cacheline aligned.
     */
    cycle_t (*read)(struct clocksource *cs);
    cycle_t cycle_last;
    cycle_t mask;
    rt_uint32_t mult;
    rt_uint32_t shift;
    uint64_t max_idle_ns;

    const char *name;
    rt_list_t list;
    int rating;
    cycle_t (*vread)(void);
    int (*enable)(struct clocksource *cs);
    void (*disable)(struct clocksource *cs);
    unsigned long flags;
    void (*suspend)(struct clocksource *cs);
    void (*resume)(struct clocksource *cs);

} ____cacheline_aligned;

int __clocksource_register_scale(struct clocksource *cs, rt_uint32_t scale,
                                 rt_uint32_t freq);
/**
 * clocksource_cyc2ns - converts clocksource cycles to nanoseconds
 *
 * Converts cycles to nanoseconds, using the given mult and shift.
 *
 * XXX - This could use some mult_lxl_ll() asm optimization
 */
static inline int64_t clocksource_cyc2ns(cycle_t cycles, rt_uint32_t mult,
                                         rt_uint32_t shift)
{
    return ((uint64_t)cycles * mult) >> shift;
}

static inline int clocksource_register_hz(struct clocksource *cs,
                                          rt_uint32_t hz)
{
    return __clocksource_register_scale(cs, 1, hz);
}

extern void timekeeping_notify(struct clocksource *clock);
void clocksource_select(void);
struct clocksource *__attribute__((weak)) clocksource_default_clock(void);
void clocksource_init(void);

#endif /* CLOCKSOURCE_H_ */
