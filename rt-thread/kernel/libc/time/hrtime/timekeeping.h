/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author        Notes
 * 2019-02-01    qin&songyh     first version
 */

#ifndef __TIMEKEEPING_H__
#define __TIMEKEEPING_H__
#include <stdint.h>
#include <rtthread.h>
#include <sys/types.h>
#include <time.h>
#include "clocksource.h"
#include "clockevents.h"
#include "ktime.h"

#ifndef _SYS__TIMESPEC_H_
#define _SYS__TIMESPEC_H_
struct timespec
{
    long tv_sec;  /* seconds */
    long tv_nsec; /* nanoseconds */
};
#endif

#define NTP_INTERVAL_FREQ (RT_TICK_PER_SECOND)
#define NTP_INTERVAL_LENGTH (NSEC_PER_SEC / NTP_INTERVAL_FREQ)

static inline rt_uint32_t __iter_div_u64_rem(uint64_t dividend,
                                             rt_uint32_t divisor,
                                             uint64_t *remainder)
{
    rt_uint32_t ret = 0;

    while (dividend >= divisor)
    {
        /* The following asm() prevents the compiler from
           optimising this loop into a modulo operation.  */
        asm("" : "+rm"(dividend));

        dividend -= divisor;
        ret++;
    }

    *remainder = dividend;

    return ret;
}

/**
 * timespec_add_ns - Adds nanoseconds to a timespec
 * @a:      pointer to timespec to be incremented
 * @ns:     unsigned nanoseconds value to be added
 *
 * This must always be inlined because its used from the x86-64 vdso,
 * which cannot call other kernel functions.
 */
static inline void timespec_add_ns(struct timespec *a, uint64_t ns)
{
    a->tv_sec += __iter_div_u64_rem(a->tv_nsec + ns, NSEC_PER_SEC, &ns);
    a->tv_nsec = ns;
}

void set_normalized_timespec(struct timespec *ts, time_t sec, long nsec);
void timekeeping_notify(struct clocksource *clock);
void timekeeping_init(void);
void update_wall_time(void);
ktime_t ktime_get(void);
ktime_t ktime_get_real(void);
int timekeeping_valid_for_hres(void);
void get_xtime_and_monotonic_and_sleep_offset(struct timespec *xtim,
                                              struct timespec *wtom,
                                              struct timespec *sleep);
int do_settimeofday(const struct timespec *tv);
void do_gettimeofday(struct timeval *tv);
/*
 * sub = lhs - rhs, in normalized form
 */
static inline struct timespec timespec_sub(struct timespec lhs,
                                           struct timespec rhs)
{
    struct timespec ts_delta;

    set_normalized_timespec(&ts_delta, lhs.tv_sec - rhs.tv_sec,
                            lhs.tv_nsec - rhs.tv_nsec);
    return ts_delta;
}

#endif /* __TIMEKEEPING_H_ */
