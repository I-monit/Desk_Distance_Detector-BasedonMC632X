/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author        Notes
 * 2019-02-01    qin&songyh     first version
 */

#ifndef __CLOCKEVENTS_H__
#define __CLOCKEVENTS_H__

#include <stdint.h>
#include <rtthread.h>
#include "ktime.h"
#include <rthw.h>
/*
 * Clock event features
 */
#define CLOCK_EVT_FEAT_PERIODIC 0x000001
#define CLOCK_EVT_FEAT_ONESHOT 0x000002

/* Clock event mode commands */
enum clock_event_mode
{
    CLOCK_EVT_MODE_UNUSED = 0,
    CLOCK_EVT_MODE_SHUTDOWN,
    CLOCK_EVT_MODE_PERIODIC,
    CLOCK_EVT_MODE_ONESHOT,
    CLOCK_EVT_MODE_RESUME,
};

/**
 * struct clock_event_device - clock event device descriptor
 * @event_handler:  Assigned by the framework to be called by the low
 *          level handler of the event source
 * @set_next_event: set next event function
 * @next_event:     local storage for the next event in oneshot mode
 * @max_delta_ns:   maximum delta value in ns
 * @min_delta_ns:   minimum delta value in ns
 * @mult:       nanosecond to cycles multiplier
 * @shift:      nanoseconds to cycles divisor (power of two)
 * @mode:       operating mode assigned by the management code
 * @features:       features
 * @retries:        number of forced programming retries
 * @set_mode:       set mode function
 * @broadcast:      function to broadcast events
 * @min_delta_ticks:    minimum delta value in ticks stored for reconfiguration
 * @max_delta_ticks:    maximum delta value in ticks stored for reconfiguration
 * @name:       ptr to clock event name
 * @rating:     variable to rate clock event devices
 * @irq:        IRQ number (only for non CPU local devices)
 * @cpumask:        cpumask to indicate for which CPUs this device works
 * @list:       list head for the management code
 */
struct clock_event_device
{
    void (*event_handler)(struct clock_event_device *);
    int (*set_next_event)(unsigned long evt, struct clock_event_device *);
    ktime_t next_event;
    uint64_t max_delta_ns;
    uint64_t min_delta_ns;
    rt_uint32_t mult;
    rt_uint32_t shift;
    enum clock_event_mode mode;
    unsigned int features;
    unsigned long retries;

    void (*set_mode)(enum clock_event_mode mode, struct clock_event_device *);
    unsigned long min_delta_ticks;
    unsigned long max_delta_ticks;

    const char *name;
    int rating;
    int irq;
    const struct cpumask *cpumask;
    rt_list_t list;
} ____cacheline_aligned;

void clockevents_init(void);
void clockevents_register_device(struct clock_event_device *dev);
uint64_t clockevent_delta2ns(unsigned long latch,
                             struct clock_event_device *evt);

/*
 * Calculate a multiplication factor for scaled math, which is used to convert
 * nanoseconds based values to clock ticks:
 *
 * clock_ticks = (nanoseconds * factor) >> shift.
 *
 * div_sc is the rearranged equation to calculate a factor from a given clock
 * ticks / nanoseconds ratio:
 *
 * factor = (clock_ticks << shift) / nanoseconds
 */
static inline unsigned long div_sc(unsigned long ticks, unsigned long nsec,
                                   int shift)
{
    uint64_t tmp = ((uint64_t)ticks) << shift;

    tmp /= nsec;
    return (unsigned long)tmp;
}

void clockevents_exchange_device(struct clock_event_device *old,
                                 struct clock_event_device *new);
void clockevents_set_mode(struct clock_event_device *dev,
                          enum clock_event_mode mode);
int clockevents_program_event(struct clock_event_device *dev, ktime_t expires,
                              ktime_t now);
#endif /* CLOCKEVENTS_H_ */
