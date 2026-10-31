/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author        Notes
 * 2019-02-01    qin&songyh     first version
 */

#include "clockevents.h"
#include "timekeeping.h"
#include "tick.h"

static struct rt_mutex clockevents_mutex;

/* The registered clock event devices */
rt_list_t clockevents_devices  = RT_LIST_OBJECT_INIT(clockevents_devices);
rt_list_t clockevents_released = RT_LIST_OBJECT_INIT(clockevents_released);

/**
 * clockevents_program_event - Reprogram the clock event device.
 * @expires:    absolute expiry time (monotonic clock)
 *
 * Returns 0 on success, -ETIME when the event is in the past.
 */
int clockevents_program_event(struct clock_event_device *dev, ktime_t expires,
                              ktime_t now)
{
    unsigned long long clc;
    int64_t delta;

    if (unlikely(expires.tv64 < 0))
    {
        rt_kprintf("clockevent warning: expires.tv64 < 0\n");
        return -RT_ETIMEOUT;
    }

    delta = ktime_to_ns(ktime_sub(expires, now));

    if (delta <= 0)
        return -RT_ETIMEOUT;

    dev->next_event = expires;

    if (dev->mode == CLOCK_EVT_MODE_SHUTDOWN)
        return 0;

    if (delta > dev->max_delta_ns)
        delta = dev->max_delta_ns;
    if (delta < dev->min_delta_ns)
        delta = dev->min_delta_ns;

    clc = delta * dev->mult;
    clc >>= dev->shift;

    return dev->set_next_event((unsigned long)clc, dev);
}

/**
 * clockevents_set_mode - set the operating mode of a clock event device
 * @dev:    device to modify
 * @mode:   new mode
 *
 * Must be called with interrupts disabled !
 */
void clockevents_set_mode(struct clock_event_device *dev,
                          enum clock_event_mode mode)
{
    if (dev->mode != mode)
    {
        dev->set_mode(mode, dev);
        dev->mode = mode;

        /*
         * A nsec2cyc multiplicator of 0 is invalid and we'd crash
         * on it, so fix it up and emit a warning:
         */
        if (mode == CLOCK_EVT_MODE_ONESHOT)
        {
            if (unlikely(!dev->mult))
            {
                dev->mult = 1;
                rt_kprintf("clockevent warning: dev->mult == 0\n");
            }
        }
    }
}

/**
 * clockevents_shutdown - shutdown the device and clear next_event
 * @dev:    device to shutdown
 */
void clockevents_shutdown(struct clock_event_device *dev)
{
    clockevents_set_mode(dev, CLOCK_EVT_MODE_SHUTDOWN);
    dev->next_event.tv64 = KTIME_MAX;
}

/**
 * clockevents_exchange_device - release and request clock devices
 * @old:    device to release (can be NULL)
 * @new:    device to request (can be NULL)
 *
 * Called from the notifier chain. clockevents_lock is held already
 */
void clockevents_exchange_device(struct clock_event_device *old,
                                 struct clock_event_device *new)
{
   /* rt_interrupt_enter(); */
    /*
     * Caller releases a clock event device. We queue it into the
     * released list and do a notify add later.
     */
    rt_base_t level;

    level = rt_hw_interrupt_disable();
    if (old)
    {
        clockevents_set_mode(old, CLOCK_EVT_MODE_UNUSED);
        rt_list_remove(&old->list);
        rt_list_insert_after(&old->list, &clockevents_released);
    }

    if (new)
    {
        clockevents_shutdown(new);
    }
   /* rt_interrupt_leave(); */
    rt_hw_interrupt_enable(level);
}

/**
 * clockevents_delta2ns - Convert a latch value (device ticks) to nanoseconds
 * @latch:  value to convert
 * @evt:    pointer to clock event device descriptor
 *
 * Math helper, returns latch value converted to nanoseconds (bound checked)
 */
uint64_t clockevent_delta2ns(unsigned long latch,
                             struct clock_event_device *evt)
{
    uint64_t clc = (uint64_t)latch << evt->shift;

    if (unlikely(!evt->mult))
    {
        evt->mult = 1;
        rt_kprintf("clockevent warning: dev->mult == 0\n");
    }

    clc /= evt->mult;
    if (clc < 1000)
        clc = 1000;
    if (clc > KTIME_MAX)
        clc = KTIME_MAX;

    return clc;
}

/**
 * clockevents_register_device - register a clock event device
 * @dev:    device to register
 */
void clockevents_register_device(struct clock_event_device *dev)
{
    rt_mutex_take(&clockevents_mutex, RT_WAITING_FOREVER);
    rt_list_insert_after(&clockevents_devices, &dev->list);
    tick_check_new_device(dev);
    rt_mutex_release(&clockevents_mutex);
}

void clockevents_init(void)
{
    rt_mutex_init(&clockevents_mutex, "clockevents_lock", RT_IPC_FLAG_FIFO);
    rt_list_init(&clockevents_devices);
    rt_list_init(&clockevents_released);
    /* clocksource_register(&clocksource_jiffies); */
}
