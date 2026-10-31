/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author        Notes
 * 2019-02-01    qin&songyh     first version
 */

#include "clocksource.h"
#include "timekeeping.h"
#include <limits.h>

#define NSEC_PER_JIFFY \
    ((rt_uint32_t)((((uint64_t)NSEC_PER_SEC) << 8) / RT_TICK_PER_SECOND))
#define JIFFIES_SHIFT 8

static struct rt_mutex clocksource_mutex;
rt_list_t clocksource_list = RT_LIST_OBJECT_INIT(clocksource_list);
static struct clocksource *curr_clocksource;

/*
 * Enqueue the clocksource sorted by rating
 */
static void clocksource_enqueue(struct clocksource *cs)
{
    struct rt_list_node *entry = &clocksource_list;
    struct clocksource *tmp;

    rt_list_for_each_entry(tmp, &clocksource_list, list)
        /* Keep track of the place, where to insert */
        if (tmp->rating >= cs->rating)
            entry = &tmp->list;

    rt_list_insert_after(entry, &cs->list);
}

/**
 * clocksource_select - Select the best clocksource available
 *
 * Private function. Must hold clocksource_mutex when called.
 *
 * Select the clocksource with the best rating, or the clocksource,
 * which is selected by userspace override.
 */
void clocksource_select(void)
{
    struct clocksource *best/*, *cs*/;

    if (rt_list_isempty(&clocksource_list))
        return;
    /* First clocksource on the list has the best rating. */
    best = rt_list_first_entry(&clocksource_list, struct clocksource, list);

    if (curr_clocksource != best)
    {
        /*  rt_kprintf("Switching to clocksource %s\n", best->name); */
        curr_clocksource = best;
        timekeeping_notify(curr_clocksource);
    }
}

/**
 * clocks_calc_mult_shift - calculate mult/shift factors for scaled math of
 * clocks
 * @mult:   pointer to mult variable
 * @shift:  pointer to shift variable
 * @from:   frequency to convert from
 * @to:     frequency to convert to
 * @maxsec: guaranteed runtime conversion range in seconds
 *
 * The function evaluates the shift/mult pair for the scaled math
 * operations of clocksources and clockevents.
 *
 * @to and @from are frequency values in HZ. For clock sources @to is
 * NSEC_PER_SEC == 1GHz and @from is the counter frequency. For clock
 * event @to is the counter frequency and @from is NSEC_PER_SEC.
 *
 * The @maxsec conversion range argument controls the time frame in
 * seconds which must be covered by the runtime conversion with the
 * calculated mult and shift factors. This guarantees that no 64bit
 * overflow happens when the input value of the conversion is
 * multiplied with the calculated mult factor. Larger ranges may
 * reduce the conversion accuracy by chosing smaller mult and shift
 * factors.
 */
void clocks_calc_mult_shift(rt_uint32_t *mult, rt_uint32_t *shift,
                            rt_uint32_t from, rt_uint32_t to,
                            rt_uint32_t maxsec)
{
    uint64_t tmp;
    rt_uint32_t sft, sftacc = 32;

    /*
     * Calculate the shift factor which is limiting the conversion
     * range:
     */
    tmp = ((uint64_t)maxsec * from) >> 32;
    while (tmp)
    {
        tmp >>= 1;
        sftacc--;
    }

    /*
     * Find the conversion shift/mult pair which has the best
     * accuracy and fits the maxsec conversion range:
     */
    for (sft = 32; sft > 0; sft--)
    {
        tmp = (uint64_t)to << sft;
        tmp += from / 2;
        tmp /= from;
        if ((tmp >> sftacc) == 0)
            break;
    }
    *mult  = tmp;
    *shift = sft;
}

/**
 * __clocksource_updatefreq_scale - Used update clocksource with new freq
 * @t:      clocksource to be registered
 * @scale:  Scale factor multiplied against freq to get clocksource hz
 * @freq:   clocksource frequency (cycles per second) divided by scale
 *
 * This should only be called from the clocksource->enable() method.
 *
 * This *SHOULD NOT* be called directly! Please use the
 * clocksource_updatefreq_hz() or clocksource_updatefreq_khz helper functions.
 */
void clocksource_updatefreq_scale(struct clocksource *cs, rt_uint32_t scale,
                                  rt_uint32_t freq)
{
    uint64_t sec;

    /*
     * Calc the maximum number of seconds which we can run before
     * wrapping around. For clocksources which have a mask > 32bit
     * we need to limit the max sleep time to have a good
     * conversion precision. 10 minutes is still a reasonable
     * amount. That results in a shift value of 24 for a
     * clocksource with mask >= 40bit and f >= 4GHz. That maps to
     * ~ 0.06ppm granularity for NTP. We apply the same 12.5%
     * margin as we do in clocksource_max_deferment()
     */
    sec = (cs->mask - (cs->mask >> 5));
    sec /= freq;
    sec /= scale;
    if (!sec)
        sec = 1;
    else if (sec > 600 && cs->mask > UINT_MAX)
        sec = 600;

    clocks_calc_mult_shift(&cs->mult, &cs->shift, freq, NSEC_PER_SEC / scale,
                           sec * scale);
}

/**
 * clocksource_register - Used to install new clocksources
 * @t:      clocksource to be registered
 *
 * Returns -EBUSY if registration fails, zero otherwise.
 */
int clocksource_register(struct clocksource *cs)
{
    rt_mutex_take(&clocksource_mutex, RT_WAITING_FOREVER);
    clocksource_enqueue(cs);
    clocksource_select();
    rt_mutex_release(&clocksource_mutex);
    return 0;
}

/**
 * __clocksource_register_scale - Used to install new clocksources
 * @t:      clocksource to be registered
 * @scale:  Scale factor multiplied against freq to get clocksource hz
 * @freq:   clocksource frequency (cycles per second) divided by scale
 *
 * Returns -EBUSY if registration fails, zero otherwise.
 *
 * This *SHOULD NOT* be called directly! Please use the
 * clocksource_register_hz() or clocksource_register_khz helper functions.
 */
int __clocksource_register_scale(struct clocksource *cs, rt_uint32_t scale,
                                 rt_uint32_t freq)
{
    /* Initialize mult/shift and max_idle_ns */
    clocksource_updatefreq_scale(cs, scale, freq);

    /* Add clocksource to the clcoksource list */
    rt_mutex_take(&clocksource_mutex, RT_WAITING_FOREVER);
    clocksource_enqueue(cs);
    clocksource_select();
    rt_mutex_release(&clocksource_mutex);
    return 0;
}

/**
 * clocksource_unregister - remove a registered clocksource
 */
void clocksource_unregister(struct clocksource *cs)
{
    rt_mutex_take(&clocksource_mutex, RT_WAITING_FOREVER);
    rt_list_remove(&cs->list);
    clocksource_select();
    rt_mutex_release(&clocksource_mutex);
}

static cycle_t tick_read(struct clocksource *cs)
{
    return (cycle_t)rt_tick_get();
}

struct clocksource clocksource_jiffies = {
    .name   = "rt_tick",
    .rating = 1, /* lowest valid rating*/
    .read   = tick_read,
    .mask   = 0xffffffff,                      /*32bits*/
    .mult   = NSEC_PER_JIFFY << JIFFIES_SHIFT, /* details above */
    .shift  = JIFFIES_SHIFT,
};

struct clocksource *__attribute__((weak)) clocksource_default_clock(void)
{
    return &clocksource_jiffies;
}

void clocksource_init(void)
{
    rt_mutex_init(&clocksource_mutex, "clocksource_lock", RT_IPC_FLAG_FIFO);
    rt_list_init(&clocksource_list);
    clocksource_register(&clocksource_jiffies);
}
