/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author        Notes
 * 2019-02-01    qin&songyh     first version
 */

#include "ktime.h"

/*
 * Divide a ktime value by a nanosecond value
 */
uint64_t ktime_divns(const ktime_t kt, int64_t div)
{
    uint64_t dclc;
    int sft = 0;

    dclc = ktime_to_ns(kt);
    /* Make sure the divisor is less than 2^32: */
    while (div >> 32)
    {
        sft++;
        div >>= 1;
    }
    dclc >>= sft;
    dclc /= (unsigned long)div;

    return dclc;
}

/*
 * Add two ktime values and do a safety check for overflow:
 */
ktime_t ktime_add_safe(const ktime_t lhs, const ktime_t rhs)
{
    ktime_t res = ktime_add(lhs, rhs);

    /*
     * We use KTIME_SEC_MAX here, the maximum timeout which we can
     * return to user space in a timespec:
     */
    if (res.tv64 < 0 || res.tv64 < lhs.tv64 || res.tv64 < rhs.tv64)
        res = ktime_set(KTIME_SEC_MAX, 0);

    return res;
}
