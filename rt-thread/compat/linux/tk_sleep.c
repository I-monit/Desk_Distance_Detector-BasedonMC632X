#include <rtthread.h>
#include <rthw.h>
#include <time.h>
#include <sys/time.h>
#ifdef RT_USING_TIMEKEEPING
#include "ktime.h"
#include "hrtimer.h"
#endif

static int do_usleep_range(unsigned long min, unsigned long max)
{
    ktime_t kmin;
    unsigned long delta;

    kmin = ktime_set(0, min * NSEC_PER_USEC);
    delta = (max - min) * NSEC_PER_USEC;
    return schedule_hrtimeout_range(&kmin, delta, HRTIMER_MODE_REL);
}

void usleep_range(unsigned long min, unsigned long max)
{
    do_usleep_range(min, max);
}

int usleep(useconds_t microseconds)
{
    if (microseconds > USEC_PER_SEC)
    {
        rt_set_errno(EINVAL);
        return -1;
    }

    usleep_range(microseconds, (microseconds + 1000));
    return 0;
}

void msleep(unsigned int msecs)
{
    rt_thread_delay(msecs / (MSEC_PER_SEC / RT_TICK_PER_SECOND));

    return;
}

int stime(time_t *t)
{
    struct timespec tv;
    struct timeval now;

    now.tv_sec = *t;
    now.tv_usec  = 0;
    settimeofday(&now, RT_NULL);

    tv.tv_sec  = *t;
    tv.tv_nsec = 0;
    clock_settime(CLOCK_REALTIME, &tv);

    return 0;
}

