/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author        Notes
 * 2019-02-01    qin&songyh     first version
 */

#ifndef __KTIME_H__
#define __KTIME_H__

#include <stdint.h>
#include <rtdef.h>
#include <time.h>

#ifndef likely
#define likely(x) (__builtin_expect(!!(x), 1))
#endif
#ifndef unlikely
#define unlikely(x) (__builtin_expect(!!(x), 0))
#endif

#define NSEC_PER_USEC   1000L
#define MSEC_PER_SEC    1000L
#define USEC_PER_MSEC   1000L
#define USEC_PER_SEC    1000000L

#ifndef NSEC_PER_SEC
#define NSEC_PER_SEC (1000000000ULL)
#endif

#ifndef NSEC_PER_MSEC
#define NSEC_PER_MSEC (1000000L)
#endif

/*
 * ktime_t:
 *
 * On 64-bit CPUs a single 64-bit variable is used to store the hrtimers
 * internal representation of time values in scalar nanoseconds. The
 * design plays out best on 64-bit CPUs, where most conversions are
 * NOPs and most arithmetic ktime_t operations are plain arithmetic
 * operations.
 *
 * On 32-bit CPUs an optimized representation of the timespec structure
 * is used to avoid expensive conversions from and to timespecs. The
 * endian-aware order of the tv struct members is chosen to allow
 * mathematical operations on the tv64 member of the union too, which
 * for certain operations produces better code.
 *
 * For architectures with efficient support for 64/32-bit conversions the
 * plain scalar nanosecond based representation can be selected by the
 * config switch CONFIG_KTIME_SCALAR.
 */
union ktime
{
    int64_t tv64;
    struct
    {
        rt_int32_t nsec, sec;
    } tv;
};

typedef union ktime ktime_t; /* Kill this */

#define KTIME_MAX ((int64_t) ~((uint64_t)1 << 63))
#define KTIME_SEC_MAX (2147483647L)

#define TICK_NSEC (NSEC_PER_SEC / RT_TICK_PER_SECOND)
#define LOW_RES_NSEC (TICK_NSEC)
#define KTIME_LOW_RES \
    (ktime_t) { .tv64 = LOW_RES_NSEC }

/**
 * div_s64_rem - signed 64bit divide with 32bit divisor with remainder
 */
static inline rt_int64_t div_s64_rem(rt_int64_t dividend, rt_int32_t divisor, rt_int32_t *remainder)
{
    *remainder = dividend % divisor;
    return dividend / divisor;
}

/**
 * ns_to_timespec - Convert nanoseconds to timespec
 * @nsec:       the nanoseconds value to be converted
 *
 * Returns the timespec representation of the nsec parameter.
 */
static inline struct timespec ns_to_timespec(const rt_int64_t nsec)
{
    struct timespec ts;
    rt_int32_t rem;

    if (!nsec)
        return (struct timespec) {0, 0};

    ts.tv_sec = div_s64_rem(nsec, NSEC_PER_SEC, &rem);
    if (unlikely(rem < 0)) {
        ts.tv_sec--;
        rem += NSEC_PER_SEC;
    }
    ts.tv_nsec = rem;

    return ts;
}

/**
 * ns_to_timeval - Convert nanoseconds to timeval
 * @nsec:       the nanoseconds value to be converted
 *
 * Returns the timeval representation of the nsec parameter.
 */
static inline struct timeval ns_to_timeval(const rt_int64_t nsec)
{
    struct timespec ts = ns_to_timespec(nsec);
    struct timeval tv;

    tv.tv_sec = ts.tv_sec;
    tv.tv_usec = (rt_int64_t) ts.tv_nsec / 1000;

    return tv;
}

/**
 * ktime_set - Set a ktime_t variable from a seconds/nanoseconds value
 * @secs:   seconds to set
 * @nsecs:  nanoseconds to set
 *
 * Return the ktime_t representation of the value
 */
static inline ktime_t ktime_set(const long secs, const unsigned long nsecs)
{
    return (ktime_t){.tv64 = (int64_t)secs * NSEC_PER_SEC + (int64_t)nsecs};
}

/* Subtract two ktime_t variables. rem = lhs -rhs: */
#define ktime_sub(lhs, rhs) ({ (ktime_t){.tv64 = (lhs).tv64 - (rhs).tv64}; })

/* Add two ktime_t variables. res = lhs + rhs: */
#define ktime_add(lhs, rhs) ({ (ktime_t){.tv64 = (lhs).tv64 + (rhs).tv64}; })

/*
 * Add a ktime_t variable and a scalar nanosecond value.
 * res = kt + nsval:
 */
#define ktime_add_ns(kt, nsval) ({ (ktime_t){.tv64 = (kt).tv64 + (nsval)}; })

/*
 * Subtract a scalar nanosecod from a ktime_t variable
 * res = kt - nsval:
 */
#define ktime_sub_ns(kt, nsval) ({ (ktime_t){.tv64 = (kt).tv64 - (nsval)}; })

/* convert a timespec to ktime_t format: */
static inline ktime_t timespec_to_ktime(const struct timespec ts)
{
    return ktime_set(ts.tv_sec, ts.tv_nsec);
}

/* convert a timeval to ktime_t format: */
static inline ktime_t timeval_to_ktime(struct timeval tv)
{
    return ktime_set(tv.tv_sec, tv.tv_usec * NSEC_PER_USEC);
}

/* Map the ktime_t to timespec conversion to ns_to_timespec function */
#define ktime_to_timespec(kt)        ns_to_timespec((kt).tv64)

/* Map the ktime_t to timeval conversion to ns_to_timeval function */
#define ktime_to_timeval(kt)         ns_to_timeval((kt).tv64)

/* Convert ktime_t to nanoseconds - NOP in the scalar storage format: */
#define ktime_to_ns(kt)              ((kt).tv64)


static inline int ktime_equal(const ktime_t cmp1, const ktime_t cmp2)
{
    return cmp1.tv64 == cmp2.tv64;
}

static inline rt_int64_t ktime_to_us(const ktime_t kt)
{
    struct timeval tv = ktime_to_timeval(kt);
    return (rt_int64_t)tv.tv_sec * USEC_PER_SEC + tv.tv_usec;
}

static inline rt_int64_t ktime_to_ms(const ktime_t kt)
{
    struct timeval tv = ktime_to_timeval(kt);
    return (rt_int64_t)tv.tv_sec * MSEC_PER_SEC + tv.tv_usec / USEC_PER_MSEC;
}

static inline rt_int64_t ktime_us_delta(const ktime_t later, const ktime_t earlier)
{
    return ktime_to_us(ktime_sub(later, earlier));
}

static inline ktime_t ktime_add_us(const ktime_t kt, const rt_uint64_t usec)
{
    return ktime_add_ns(kt, usec * 1000);
}

static inline ktime_t ktime_sub_us(const ktime_t kt, const rt_uint64_t usec)
{
    return ktime_sub_ns(kt, usec * 1000);
}

static inline ktime_t ns_to_ktime(rt_uint64_t ns)
{
    static const ktime_t ktime_zero = { .tv64 = 0 };
    return ktime_add_ns(ktime_zero, ns);
}

uint64_t ktime_divns(const ktime_t kt, int64_t div);
ktime_t ktime_add_safe(const ktime_t lhs, const ktime_t rhs);

#endif /* KTIME_H_ */
