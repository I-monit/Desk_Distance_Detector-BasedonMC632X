/*
 *  * Copyright (c) 2015-2019 Shanghai Fullhan Microelectronics Co., Ltd.
 *  *
 *  * SPDX-License-Identifier: Apache-2.0
 *  *
 *  * Change Logs:
 *  * Date           Author       Notes
 *  * 2019-01-18      {fullhan}   the first version
 *  *
 */
#ifndef __TIME_H_
#define __TIME_H_

#include_next <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/* posix clock and timer */

#ifndef CLOCK_PROCESS_CPUTIME_ID
#define CLOCK_PROCESS_CPUTIME_ID (2)
#endif
#ifndef CLOCK_THREAD_CPUTIME_ID
#define CLOCK_THREAD_CPUTIME_ID  (3)
#endif
#ifndef CLOCK_MONOTONIC
#define CLOCK_MONOTONIC          (4)
#endif

int stime(time_t *t);

int clock_gettime(clockid_t clockid, struct timespec *tp);
int clock_settime(clockid_t clockid, const struct timespec *tp);
int clock_getres(clockid_t clockid, struct timespec *res);


#ifndef difftime
#define difftime(t1, t0)   (double)((t1) - (t0))
#endif

#ifdef __cplusplus
}
#endif

#endif
