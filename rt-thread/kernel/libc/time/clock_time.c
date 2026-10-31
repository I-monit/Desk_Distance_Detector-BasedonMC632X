/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2012-12-08     Bernard      fix the issue of _timevalue.tv_usec initialization,
 *                             which found by Rob <rdent@iinet.net.au>
 */

#include <rtthread.h>
#include <stdlib.h>
#include <pthread.h>
#ifdef RT_USING_TIMEKEEPING
#include <timekeeping.h>
#endif
#include "clock_time.h"

#ifndef RT_TIMEZONE
#define RT_TIMEZONE "TZ=UTC-8"
#endif

#ifndef RT_USING_TIMEKEEPING
static struct timeval _timevalue;
#endif
int clock_time_system_init()
{
    time_t time;
    rt_device_t device;

    /* if TZ is already set or updated,
     * we should not initialize it again
     */
    if (getenv("TZ") == RT_NULL)
    {
        putenv(RT_TIMEZONE);
        tzset();
    }

    time = 0;
    device = rt_device_find("rtc");
    if (device != RT_NULL)
    {
        /* get realtime seconds */
        rt_device_control(device, RT_DEVICE_CTRL_RTC_GET_TIME, &time);
    }

#ifdef RT_USING_TIMEKEEPING
    {
        /* update wall time with rtc*/
        struct timespec tv;

        tv.tv_sec  = time;
        tv.tv_nsec = 0;
        do_settimeofday(&tv);
    }
#else
    /* get tick */
    rt_tick_t tick;
    tick = rt_tick_get();

    _timevalue.tv_usec = (tick%RT_TICK_PER_SECOND) * MICROSECOND_PER_TICK;
    _timevalue.tv_sec = time - tick/RT_TICK_PER_SECOND - 1;
#endif
    return 0;
}
INIT_COMPONENT_EXPORT(clock_time_system_init);

int clock_time_to_tick(const struct timespec *time)
{
    int tick;
    int nsecond, second;
    struct timespec tp;

    RT_ASSERT(time != RT_NULL);

    tick = RT_WAITING_FOREVER;
    if (time != NULL)
    {
        /* get current tp */
        clock_gettime(CLOCK_REALTIME, &tp);

        if ((time->tv_nsec - tp.tv_nsec) < 0)
        {
            nsecond = NANOSECOND_PER_SECOND - (tp.tv_nsec - time->tv_nsec);
            second  = time->tv_sec - tp.tv_sec - 1;
        }
        else
        {
            nsecond = time->tv_nsec - tp.tv_nsec;
            second  = time->tv_sec - tp.tv_sec;
        }

        /* to be strictly, here, we should judge the remainer to add one tick */
        tick = second * RT_TICK_PER_SECOND + nsecond / (NANOSECOND_PER_TICK);
        if (tick < 0) tick = 0;
    }

    return tick;
}
RTM_EXPORT(clock_time_to_tick);

int clock_getres(clockid_t clockid, struct timespec *res)
{
    int ret = 0;

    if (res == RT_NULL)
    {
        rt_set_errno(EINVAL);
        return -1;
    }

    switch (clockid)
    {
    case CLOCK_REALTIME:
        res->tv_sec = 0;
#ifdef RT_USING_TIMEKEEPING
        res->tv_nsec = 1000;
#else
        res->tv_nsec = NANOSECOND_PER_SECOND/RT_TICK_PER_SECOND;
#endif
        break;

#ifdef RT_USING_CPUTIME
    case CLOCK_PROCESS_CPUTIME_ID:
    case CLOCK_THREAD_CPUTIME_ID:
        res->tv_sec  = 0;
        res->tv_nsec = clock_cpu_getres();
        break;
#endif

    default:
        ret = -1;
        rt_set_errno(EINVAL);
        break;
    }

    return ret;
}
RTM_EXPORT(clock_getres);

int clock_gettime(clockid_t clockid, struct timespec *tp)
{
    int ret = 0;

    if (tp == RT_NULL)
    {
        rt_set_errno(EINVAL);
        return -1;
    }

    switch (clockid)
    {
    case CLOCK_REALTIME:
        {
#ifdef RT_USING_TIMEKEEPING
            struct timeval tv;
            do_gettimeofday(&tv);
            tp->tv_sec = tv.tv_sec;
            tp->tv_nsec = tv.tv_usec * 1000;
#else
            /* get tick */
            int tick = rt_tick_get();

            tp->tv_sec = _timevalue.tv_sec + tick / RT_TICK_PER_SECOND;
            tp->tv_nsec = (_timevalue.tv_usec + (tick % RT_TICK_PER_SECOND) * MICROSECOND_PER_TICK) * 1000;
#endif
        }
        break;

#ifdef RT_USING_CPUTIME
    case CLOCK_PROCESS_CPUTIME_ID:
    case CLOCK_THREAD_CPUTIME_ID:
        {
            float unit = 0;
            long long cpu_tick;

            unit = clock_cpu_getres();
            cpu_tick = clock_cpu_gettime();

            tp->tv_sec  = ((int)(cpu_tick * unit)) / NANOSECOND_PER_SECOND;
            tp->tv_nsec = ((int)(cpu_tick * unit)) % NANOSECOND_PER_SECOND;
        }
        break;
#endif
    case CLOCK_MONOTONIC:
    {
        rt_tick_t tknow = rt_tick_get();

        tp->tv_sec   = tknow / RT_TICK_PER_SECOND;
        tp->tv_nsec  = tknow % RT_TICK_PER_SECOND * MICROSECOND_PER_TICK * 1000;
    }
    break;
    default:
        rt_set_errno(EINVAL);
        ret = -1;
    }

    return ret;
}
RTM_EXPORT(clock_gettime);

int clock_settime(clockid_t clockid, const struct timespec *tp)
{
    int second;
    rt_device_t device;

    if ((clockid != CLOCK_REALTIME) || (tp == RT_NULL))
    {
        rt_set_errno(EINVAL);

        return -1;
    }

    /* get second */
    second = tp->tv_sec;
#ifdef RT_USING_TIMEKEEPING
    do_settimeofday(tp);
#else
    rt_tick_t tick;
    /* get tick */
    tick = rt_tick_get();

    /* update timevalue */
    _timevalue.tv_usec = MICROSECOND_PER_SECOND - (tick % RT_TICK_PER_SECOND) * MICROSECOND_PER_TICK;
    _timevalue.tv_sec = second - tick / RT_TICK_PER_SECOND;
#endif
    /* update for RTC device */
    device = rt_device_find("rtc");
    if (device != RT_NULL)
    {
        /* set realtime seconds */
        rt_device_control(device, RT_DEVICE_CTRL_RTC_SET_TIME, &second);
    }
    else
        return -1;

    return 0;
}
RTM_EXPORT(clock_settime);

#ifndef RT_USING_TIMEKEEPING
void do_gettimeofday(struct timeval *tv)
{
    struct timespec tp;

    clock_gettime(CLOCK_REALTIME, &tp);
    tv->tv_sec = tp.tv_sec;
    tv->tv_usec = tp.tv_nsec / 1000;
}
RTM_EXPORT(do_gettimeofday);

int do_settimeofday(const struct timeval *tv, void *ignore)
{
    struct timespec tp;
    tp.tv_sec = tv->tv_sec;
    tp.tv_nsec = tv->tv_usec * 1000;
    return clock_settime(CLOCK_REALTIME, &tp);
}
RTM_EXPORT(do_settimeofday);
#endif
