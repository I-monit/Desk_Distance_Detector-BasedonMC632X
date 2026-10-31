/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author        Notes
 * 2019-02-01    qin&songyh     first version
 */

#include "timekeeping.h"
#include <rtthread.h>
#include <time.h>
#include "tick.h"
#include "ktime.h"
#ifdef RT_USING_PM
#include <pm.h>
#endif

/* Structure holding internal timekeeping values. */
struct timekeeper
{
    /* Current clocksource used for timekeeping. */
    struct clocksource *clock;
    /* The shift value of the current clocksource. */
    int shift;

    /* Number of clock cycles in one NTP interval. */
    cycle_t cycle_interval;
    /* Number of clock shifted nano seconds in one NTP interval. */
    uint64_t xtime_interval;
    /* shifted nano seconds left over when rounding cycle_interval */
    int64_t xtime_remainder;
    /* Raw nano seconds accumulated per NTP interval. */
    rt_uint32_t raw_interval;

    /* Clock shifted nano seconds remainder not stored in xtime.tv_nsec. */
    uint64_t xtime_nsec;
    /* Difference between accumulated time and NTP time in ntp
     * shifted nano seconds. */
    int64_t ntp_error;
    /* Shift conversion between clock shifted nano seconds and
     * ntp shifted nano seconds. */
    int ntp_error_shift;
    /* NTP adjusted clock multiplier */
    rt_uint32_t mult;
};

static struct timekeeper timekeeper;
static struct timespec xtime __attribute__((aligned(16)));
static struct timespec wall_to_monotonic __attribute__((aligned(16)));
static struct rt_mutex timekeeping_mutex;

/**
 * set_normalized_timespec - set timespec sec and nsec parts and normalize
 *
 * @ts:     pointer to timespec variable to be set
 * @sec:    seconds to set
 * @nsec:   nanoseconds to set
 *
 * Set seconds and nanoseconds field of a timespec variable and
 * normalize to the timespec storage format
 *
 * Note: The tv_nsec part is always in the range of
 *  0 <= tv_nsec < NSEC_PER_SEC
 * For negative values only the tv_sec field is negative !
 */
void set_normalized_timespec(struct timespec *ts, time_t sec, long nsec)
{
    while (nsec >= NSEC_PER_SEC)
    {
        /*
         * The following asm() prevents the compiler from
         * optimising this loop into a modulo operation. See
         * also __iter_div_u64_rem() in include/linux/time.h
         */
        asm("" : "+rm"(nsec));
        nsec -= NSEC_PER_SEC;
        ++sec;
    }
    while (nsec < 0)
    {
        asm("" : "+rm"(nsec));
        nsec += NSEC_PER_SEC;
        --sec;
    }
    ts->tv_sec  = sec;
    ts->tv_nsec = nsec;
}

/**
 * get_xtime_and_monotonic_and_sleep_offset() - get xtime, wall_to_monotonic,
 *    and sleep offsets.
 * @xtim:   pointer to timespec to be set with xtime
 * @wtom:   pointer to timespec to be set with wall_to_monotonic
 * @sleep:  pointer to timespec to be set with time in suspend
 */
void get_xtime_and_monotonic_and_sleep_offset(struct timespec *xtim, struct timespec *wtom, struct timespec *sleep)
{
    /* todo: rt_mutex_take(&timekeeping_mutex, RT_WAITING_FOREVER); */

    *xtim = xtime;
    *wtom = wall_to_monotonic;

    /* todo: rt_mutex_release(&timekeeping_mutex); */
}

/**
 * timekeeping_forward_now - update clock to the current time
 *
 * Forward the current clock to update its state since the last call to
 * update_wall_time(). This is useful before significant clock changes,
 * as it avoids having to deal with this time offset explicitly.
 */
static void timekeeping_forward_now(void)
{
    cycle_t cycle_now, cycle_delta;
    struct clocksource *clock;
    int64_t nsec;

    clock             = timekeeper.clock;
    cycle_now         = clock->read(clock);
    cycle_delta       = (cycle_now - clock->cycle_last) & clock->mask;
    clock->cycle_last = cycle_now;

    nsec = clocksource_cyc2ns(cycle_delta, timekeeper.mult, timekeeper.shift);

    timespec_add_ns(&xtime, nsec);

    /* nsec = clocksource_cyc2ns(cycle_delta, clock->mult, clock->shift); */
    /* timespec_add_ns(&raw_time, nsec); */
}

/* Timekeeper helper functions. */
static inline int64_t timekeeping_get_ns(void)
{
    cycle_t cycle_now, cycle_delta, cycle_last;
    struct clocksource *clock;

    /* read clocksource: */
    clock      = timekeeper.clock;
    cycle_last = clock->cycle_last;
    cycle_now  = clock->read(clock);

    /* calculate the delta since the last update_wall_time: */
    cycle_delta = (cycle_now - cycle_last) & clock->mask;

    /* return delta convert to nanoseconds using ntp adjusted mult. */
    return clocksource_cyc2ns(cycle_delta, timekeeper.mult, timekeeper.shift);
}

ktime_t ktime_get(void)
{
    int64_t secs, nsecs;

    secs  = xtime.tv_sec + wall_to_monotonic.tv_sec;
    nsecs = xtime.tv_nsec + wall_to_monotonic.tv_nsec;
    nsecs += timekeeping_get_ns();

    /*
     * Use ktime_set/ktime_add_ns to create a proper ktime on
     * 32-bit architectures without CONFIG_KTIME_SCALAR.
     */
    return ktime_add_ns(ktime_set(secs, 0), nsecs);
}

/**
 * getnstimeofday - Returns the time of day in a timespec
 * @ts:     pointer to the timespec to be set
 *
 * Returns the time of day in a timespec.
 */
void getnstimeofday(struct timespec *ts)
{
    /* unsigned long seq; */
    int64_t nsecs;

    rt_mutex_take(&timekeeping_mutex, RT_WAITING_FOREVER);

    *ts   = xtime;
    nsecs = timekeeping_get_ns();

    rt_mutex_release(&timekeeping_mutex);

    timespec_add_ns(ts, nsecs);
}

/**
 * ktime_get_real - get the real (wall-) time in ktime_t format
 *
 * returns the time in ktime_t format
 */
ktime_t ktime_get_real(void)
{
    struct timespec now;

    getnstimeofday(&now);

    return timespec_to_ktime(now);
}

/**
 * do_gettimeofday - Returns the time of day in a timeval
 * @tv:     pointer to the timeval to be set
 *
 * NOTE: Users should be converted to using getnstimeofday()
 */
void do_gettimeofday(struct timeval *tv)
{
    struct timespec now;

    getnstimeofday(&now);
    tv->tv_sec  = now.tv_sec;
    tv->tv_usec = now.tv_nsec / 1000;
}

/**
 * do_settimeofday - Sets the time of day
 * @tv:     pointer to the timespec variable containing the new time
 *
 * Sets the time of day to the new time and update NTP and notify hrtimers
 */
int do_settimeofday(const struct timespec *tv)
{
    struct timespec ts_delta;
    /* unsigned long flags; */

    if ((unsigned long)tv->tv_nsec >= NSEC_PER_SEC)
        return -RT_ERROR;

    rt_mutex_take(&timekeeping_mutex, RT_WAITING_FOREVER);

    timekeeping_forward_now();

    ts_delta.tv_sec   = tv->tv_sec - xtime.tv_sec;
    ts_delta.tv_nsec  = tv->tv_nsec - xtime.tv_nsec;
    wall_to_monotonic = timespec_sub(wall_to_monotonic, ts_delta);

    xtime = *tv;
#ifdef NTP
    timekeeper.ntp_error = 0;
    ntp_clear();

    update_vsyscall(&xtime, &wall_to_monotonic, timekeeper.clock, timekeeper.mult);
#endif
    rt_mutex_release(&timekeeping_mutex);

    return 0;
}

/**
 * timekeeper_setup_internals - Set up internals to use clocksource clock.
 *
 * @clock:      Pointer to clocksource.
 *
 * Calculates a fixed cycle/nsec interval for a given clocksource/adjustment
 * pair and interval request.
 *
 * Unless you're the timekeeping code, you should not be using this!
 */
static void timekeeper_setup_internals(struct clocksource *clock)
{
    cycle_t interval;
    uint64_t tmp, ntpinterval;

    timekeeper.clock  = clock;
    clock->cycle_last = clock->read(clock);

    /* Do the ns -> cycle conversion first, using original mult */
    tmp = NTP_INTERVAL_LENGTH;
    tmp <<= clock->shift;
    ntpinterval = tmp;
    tmp += clock->mult / 2;
    tmp /= clock->mult;
    if (tmp == 0)
        tmp = 1;

    interval                  = (cycle_t)tmp;
    timekeeper.cycle_interval = interval;

    /* Go back from cycles -> shifted ns */
    timekeeper.xtime_interval  = (uint64_t)interval * clock->mult;
    timekeeper.xtime_remainder = ntpinterval - timekeeper.xtime_interval;
    timekeeper.raw_interval    = ((uint64_t)interval * clock->mult) >> clock->shift;

    timekeeper.xtime_nsec = 0;
    timekeeper.shift      = clock->shift;
#if RT_USING_NTP
    timekeeper.ntp_error       = 0;
    timekeeper.ntp_error_shift = NTP_SCALE_SHIFT - clock->shift;
#endif
    /*
     * The timekeeper keeps its own mult values for the currently
     * active clocksource. These value will be adjusted via NTP
     * to counteract clock drifting.
     */
    timekeeper.mult = clock->mult;
}

/**
 * read_persistent_clock -  Return time from the persistent clock.
 *
 * Weak dummy function for arches that do not yet support it.
 * Reads the time from the battery backed persistent clock.
 * Returns a timespec with tv_sec=0 and tv_nsec=0 if unsupported.
 *
 *  XXX - Do be sure to remove it once all arches implement it.
 */
void __attribute__((weak)) read_persistent_clock(struct timespec *ts)
{
    ts->tv_sec  = 0;
    ts->tv_nsec = 0;
}

/**
 * change_clocksource - Swaps clocksources if a new one is available
 *
 * Accumulates current time interval and initializes new clocksource
 */
static int change_clocksource(void *data)
{
    struct clocksource *new, *old;

    new = (struct clocksource *)data;

    if (timekeeper.clock)
    {
        timekeeping_forward_now();
    }

    if (!new->enable || new->enable(new) == 0)
    {
        timekeeper_setup_internals(new);
        if (timekeeper.clock)
        {
            old = timekeeper.clock;
            if (old->disable)
                old->disable(old);
        }
    }
    return 0;
}

/**
 * timekeeping_notify - Install a new clock source
 * @clock:      pointer to the clock source
 *
 * This function is called from clocksource.c after a new, better clock
 * source has been registered. The caller holds the clocksource_mutex.
 */
void timekeeping_notify(struct clocksource *clock)
{
    if (timekeeper.clock == clock)
        return;

    change_clocksource(clock);

    /* stop_machine(change_clocksource, clock, NULL); */
    /* tick_clock_notify(); */
}

/**
 * update_wall_time - Uses the current clocksource to increment the wall time
 *
 * Called from the timer interrupt, must hold a write on xtime_lock.
 */
void update_wall_time(void)
{
    struct clocksource *clock;
    cycle_t offset;

    int shift RT_UNUSED = 0, maxshift RT_UNUSED;

    if (!timekeeper.clock)
        return;

    clock = timekeeper.clock;

    offset = (clock->read(clock) - clock->cycle_last) & clock->mask;

    timekeeper.xtime_nsec = (int64_t)xtime.tv_nsec << timekeeper.shift;

    while (offset >= timekeeper.cycle_interval)
    {
        /* accumulate one interval */
        offset -= timekeeper.cycle_interval;
        clock->cycle_last += timekeeper.cycle_interval;

        timekeeper.xtime_nsec += timekeeper.xtime_interval;
        if (timekeeper.xtime_nsec >= (uint64_t)NSEC_PER_SEC << timekeeper.shift)
        {
            timekeeper.xtime_nsec -= (uint64_t)NSEC_PER_SEC << timekeeper.shift;
            xtime.tv_sec++;
            /* second_overflow(); */
        }

        rt_tick_increase();
#if 0
         clock->raw_time.tv_nsec += clock->raw_interval;
        if (clock->raw_time.tv_nsec >= NSEC_PER_SEC)
        {
            clock->raw_time.tv_nsec -= NSEC_PER_SEC;
            clock->raw_time.tv_sec++;
        }

        /* accumulate error between NTP and clock interval */
        clock->error += tick_length;
        clock->error -= clock->xtime_interval << (NTP_SCALE_SHIFT - clock->shift);
#endif
    }

#ifdef RT_USING_TICKLESS

    /*
     * With NO_HZ we may have to accumulate many cycle_intervals
     * (think "ticks") worth of time at once. To do this efficiently,
     * we calculate the largest doubling multiple of cycle_intervals
     * that is smaller then the offset. We then accumulate that
     * chunk in one go, and then try to consume the next smaller
     * doubled multiple.
     */
    shift = ilog2(offset) - ilog2(timekeeper.cycle_interval);
    shift = max(0, shift);
    /* Bound shift to one less then what overflows tick_length */
    maxshift = (8 * sizeof(tick_length) - (ilog2(tick_length) + 1)) - 1;
    shift    = min(shift, maxshift);
    while (offset >= timekeeper.cycle_interval)
    {
        offset = logarithmic_accumulation(offset, shift);
        if (offset < timekeeper.cycle_interval << shift)
            shift--;
    }
#endif

#if 0
    /* correct the clock when NTP error is too big */
    timekeeping_adjust(offset);

    /*
     * Since in the loop above, we accumulate any amount of time
     * in xtime_nsec over a second into xtime.tv_sec, its possible for
     * xtime_nsec to be fairly small after the loop. Further, if we're
     * slightly speeding the clocksource up in timekeeping_adjust(),
     * its possible the required corrective factor to xtime_nsec could
     * cause it to underflow.
     *
     * Now, we cannot simply roll the accumulated second back, since
     * the NTP subsystem has been notified via second_overflow. So
     * instead we push xtime_nsec forward by the amount we underflowed,
     * and add that amount into the error.
     *
     * We'll correct this error next time through this function, when
     * xtime_nsec is not as small.
     */
    if (unlikely((int64_t)timekeeper.xtime_nsec < 0))
    {
        int64_t neg = -(int64_t)timekeeper.xtime_nsec;

        timekeeper.xtime_nsec = 0;
        timekeeper.ntp_error += neg << timekeeper.ntp_error_shift;
    }
#endif

    /*
     * Store full nanoseconds into xtime after rounding it up and
     * add the remainder to the error difference.
     */
    xtime.tv_nsec = ((int64_t)timekeeper.xtime_nsec >> timekeeper.shift) + 1;
    timekeeper.xtime_nsec -= (int64_t)xtime.tv_nsec << timekeeper.shift;
    timekeeper.ntp_error += timekeeper.xtime_nsec << timekeeper.ntp_error_shift;

    /*
     * Finally, make sure that after the rounding
     * xtime.tv_nsec isn't larger then NSEC_PER_SEC
     */
    if (unlikely(xtime.tv_nsec >= NSEC_PER_SEC))
    {
        xtime.tv_nsec -= NSEC_PER_SEC;
        xtime.tv_sec++;
        /* second_overflow(); */
    }
#if 0
    /* check to see if there is a new clocksource to use */
    update_vsyscall(&xtime, &wall_to_monotonic, timekeeper.clock,
                timekeeper.mult);
#endif
    /* clocksource_select(); */
}

/**
 * timekeeping_valid_for_hres - Check if timekeeping is suitable for hres
 */
int timekeeping_valid_for_hres(void)
{
    int ret;

    ret = timekeeper.clock->flags & CLOCK_SOURCE_VALID_FOR_HRES;

    return ret;
}

#ifdef RT_USING_PM
static void timekeeping_resume(const struct rt_device *device, rt_uint8_t mode)
{
    struct clocksource *clock = timekeeper.clock;

    /* update cycle_last, Ensure the TICK does not mutate */
    clock->cycle_last = clock->read(clock);
}

struct rt_device_pm_ops timekeeping_pm_ops = {
    .resume_noirq = timekeeping_resume,
};
#endif

/*
 * timekeeping_init - Initializes the clocksource and common timekeeping values
 */
void timekeeping_init(void)
{
    struct clocksource *clock;
    /* unsigned long flags; */
    struct timespec now, boot;

    rt_mutex_init(&timekeeping_mutex, "timekeeping_lock", RT_IPC_FLAG_FIFO);

    clocksource_init();
    clockevents_init();
    tickdevice_init();

    read_persistent_clock(&now);

    /* todo: ntp init */
    /* ntp_init(); */

    clock = clocksource_default_clock();
    if (clock->enable)
        clock->enable(clock);

    timekeeper_setup_internals(clock);
#ifdef RT_USING_PM
    rt_pm_device_register(NULL, &timekeeping_pm_ops);
#endif

    xtime.tv_sec  = now.tv_sec;
    xtime.tv_nsec = now.tv_nsec;

    boot.tv_sec  = xtime.tv_sec;
    boot.tv_nsec = xtime.tv_nsec;

    set_normalized_timespec(&wall_to_monotonic, -boot.tv_sec, -boot.tv_nsec);
}

void display_date(void)
{
    struct timeval tv;

    do_gettimeofday(&tv);

    rt_kprintf("%s\n", asctime(localtime(&tv.tv_sec)));
}

#ifdef RT_USING_FINSH
#include <finsh.h>
FINSH_FUNCTION_EXPORT(display_date, display date);
#endif
