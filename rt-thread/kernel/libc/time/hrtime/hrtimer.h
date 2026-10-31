/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author        Notes
 * 2019-02-01    qin&songyh     first version
 */

#ifndef __HRTIMER_H__
#define __HRTIMER_H__

#include "ktime.h"
#include "timerqueue.h"
#include "clockevents.h"
#include <rtthread.h>
#include <stdint.h>
#include <stddef.h>
#include <time.h>
#include <rthw.h>

struct hrtimer_clock_base;
struct hrtimer_cpu_base;

/*
 * The IDs of the various system clocks (for POSIX.1b interval timers):
 */
#ifndef CLOCK_BOOTTIME
#define CLOCK_BOOTTIME 7
#endif

#define MAX_CLOCKS 16

#define HIGH_RES_NSEC 1
#define KTIME_HIGH_RES \
    (ktime_t) { .tv64 = HIGH_RES_NSEC }
#define HRTIMER_STATE_INACTIVE 0x00
#define HRTIMER_STATE_ENQUEUED 0x01
#define HRTIMER_STATE_CALLBACK 0x02
#define HRTIMER_STATE_MIGRATE 0x04

/*
 * Mode arguments of xxx_hrtimer functions:
 */
enum hrtimer_mode
{
    HRTIMER_MODE_ABS        = 0x0,  /* Time value is absolute */
    HRTIMER_MODE_REL        = 0x1,  /* Time value is relative to now */
    HRTIMER_MODE_PINNED     = 0x02, /* Timer is bound to CPU */
    HRTIMER_MODE_ABS_PINNED = 0x02,
    HRTIMER_MODE_REL_PINNED = 0x03,
};

/*
 * Return values for the callback function
 */
enum hrtimer_restart
{
    HRTIMER_NORESTART, /* Timer is not restarted */
    HRTIMER_RESTART,   /* Timer must be restarted */
};

enum hrtimer_base_type
{
    HRTIMER_BASE_MONOTONIC,
    HRTIMER_BASE_REALTIME,
    HRTIMER_BASE_BOOTTIME,
    HRTIMER_MAX_CLOCK_BASES,
};

struct hrtimer_clock_base;
struct hrtimer_cpu_base;

/**
 * struct hrtimer_clock_base - the timer base for a specific clock
 * @cpu_base:       per cpu clock base
 * @index:      clock type index for per_cpu support when moving a
 *          timer to a base on another cpu.
 * @clockid:        clock id for per_cpu support
 * @active:     red black tree root node for the active timers
 * @resolution:     the resolution of the clock, in nanoseconds
 * @get_time:       function to retrieve the current time of the clock
 * @softirq_time:   the time when running the hrtimer queue in the softirq
 * @offset:     offset of this clock to the monotonic base
 */
struct hrtimer_clock_base
{
    struct hrtimer_cpu_base *cpu_base;
    int index;
    int clockid;
    struct timerqueue_head active;
    ktime_t resolution;
    ktime_t (*get_time)(void);
    ktime_t softirq_time;
    ktime_t offset;
};

/*
 * struct hrtimer_cpu_base - the per cpu clock bases
 * @lock:       lock protecting the base and associated clock bases
 *          and timers
 * @active_bases:   Bitfield to mark bases with active timers
 * @expires_next:   absolute time of the next event which was scheduled
 *          via clock_set_next_event()
 * @hres_active:    State of high resolution mode
 * @hang_detected:  The last hrtimer interrupt detected a hang
 * @nr_events:      Total number of hrtimer interrupt events
 * @nr_retries:     Total number of hrtimer interrupt retries
 * @nr_hangs:       Total number of hrtimer interrupt hangs
 * @max_hang_time:  Maximum time spent in hrtimer_interrupt
 * @clock_base:     array of clock bases for this cpu
 */
struct hrtimer_cpu_base
{
    struct rt_mutex lock;
    unsigned long active_bases;
    ktime_t expires_next;
    int hres_active;
    int hang_detected;
    unsigned long nr_events;
    unsigned long nr_retries;
    unsigned long nr_hangs;
    ktime_t max_hang_time;
    struct hrtimer_clock_base clock_base[HRTIMER_MAX_CLOCK_BASES];
};

/**
 * struct hrtimer - the basic hrtimer structure
 * @node:   timerqueue node, which also manages node.expires,
 *      the absolute expiry time in the hrtimers internal
 *      representation. The time is related to the clock on
 *      which the timer is based. Is setup by adding
 *      slack to the _softexpires value. For non range timers
 *      identical to _softexpires.
 * @_softexpires: the absolute earliest expiry time of the hrtimer.
 *      The time which was given as expiry time when the timer
 *      was armed.
 * @function:   timer expiry callback function
 * @base:   pointer to the timer base (per cpu and per clock)
 * @state:  state information (See bit values above)
 * @start_site: timer statistics field to store the site where the timer
 *      was started
 * @start_comm: timer statistics field to store the name of the process which
 *      started the timer
 * @start_pid: timer statistics field to store the pid of the task which
 *      started the timer
 *
 * The hrtimer structure must be initialized by hrtimer_init()
 */
struct hrtimer
{
    struct timerqueue_node node;
    ktime_t _softexpires;
    enum hrtimer_restart (*function)(struct hrtimer *);
    struct hrtimer_clock_base *base;
    unsigned long state;
    void *data;
};

/**
 * struct hrtimer_sleeper - simple sleeper structure
 * @timer:	embedded timer structure
 * @task:	task to wake up
 *
 * task is set to NULL, when the timer expires.
 */
struct hrtimer_sleeper {
    struct hrtimer timer;
    struct rt_thread *task;
};

/*
 * Helper function to check, whether the timer is on one of the queues
 */
static inline int hrtimer_is_queued(struct hrtimer *timer)
{
    return timer->state & HRTIMER_STATE_ENQUEUED;
}

/*
 * Helper function to check, whether the timer is running the callback
 * function
 */
static inline int hrtimer_callback_running(struct hrtimer *timer)
{
    return timer->state & HRTIMER_STATE_CALLBACK;
}

static inline void hrtimer_set_expires(struct hrtimer *timer, ktime_t time)
{
    timer->node.expires = time;
    timer->_softexpires = time;
}

static inline void hrtimer_set_expires_range(struct hrtimer *timer,
                                             ktime_t time, ktime_t delta)
{
    timer->_softexpires = time;
    timer->node.expires = ktime_add_safe(time, delta);
}

static inline void hrtimer_set_expires_range_ns(struct hrtimer *timer,
                                                ktime_t time,
                                                unsigned long delta)
{
    timer->_softexpires = time;
    timer->node.expires = ktime_add_safe(time, ns_to_ktime(delta));
}

static inline void hrtimer_set_expires_tv64(struct hrtimer *timer, int64_t tv64)
{
    timer->node.expires.tv64 = tv64;
    timer->_softexpires.tv64 = tv64;
}

static inline void hrtimer_add_expires(struct hrtimer *timer, ktime_t time)
{
    timer->node.expires = ktime_add_safe(timer->node.expires, time);
    timer->_softexpires = ktime_add_safe(timer->_softexpires, time);
}

static inline void hrtimer_add_expires_ns(struct hrtimer *timer, uint64_t ns)
{
    timer->node.expires = ktime_add_ns(timer->node.expires, ns);
    timer->_softexpires = ktime_add_ns(timer->_softexpires, ns);
}

static inline ktime_t hrtimer_get_expires(const struct hrtimer *timer)
{
    return timer->node.expires;
}

static inline ktime_t hrtimer_get_softexpires(const struct hrtimer *timer)
{
    return timer->_softexpires;
}

static inline int64_t hrtimer_get_expires_tv64(const struct hrtimer *timer)
{
    return timer->node.expires.tv64;
}
static inline int64_t hrtimer_get_softexpires_tv64(const struct hrtimer *timer)
{
    return timer->_softexpires.tv64;
}

static inline int64_t hrtimer_get_expires_ns(const struct hrtimer *timer)
{
    return ktime_to_ns(timer->node.expires);
}

static inline ktime_t hrtimer_expires_remaining(const struct hrtimer *timer)
{
    return ktime_sub(timer->node.expires, timer->base->get_time());
}

static inline int hrtimer_active(const struct hrtimer *timer)
{
    return timer->state != HRTIMER_STATE_INACTIVE;
}

void hrtimer_run_pending(void);
void hrtimer_init(struct hrtimer *timer, int clock_id, enum hrtimer_mode mode);
uint64_t hrtimer_forward(struct hrtimer *timer, ktime_t now, ktime_t interval);
extern void hrtimer_interrupt(struct clock_event_device *dev);
int hrtimer_start_range_ns(struct hrtimer *timer, ktime_t tim,
                           unsigned long delta_ns,
                           const enum hrtimer_mode mode);
int hrtimer_cancel(struct hrtimer *timer);

static inline int hrtimer_start_expires(struct hrtimer *timer,
                                        enum hrtimer_mode mode)
{
    unsigned long delta;
    ktime_t soft, hard;

    soft  = hrtimer_get_softexpires(timer);
    hard  = hrtimer_get_expires(timer);
    delta = ktime_to_ns(ktime_sub(hard, soft));
    return hrtimer_start_range_ns(timer, soft, delta, mode);
}

void hrtimers_init(void);
int schedule_hrtimeout_range(ktime_t *expires, unsigned long delta,
                            const enum hrtimer_mode mode);
int schedule_hrtimeout_range_clock(ktime_t *expires,
        unsigned long delta, const enum hrtimer_mode mode, int clock);

#endif /* HRTIMER_NEW_H_ */
