#include <rtthread.h>
#include <time.h>
#include <delay.h>

#define NSEC_PER_USEC   1000L
#define MSEC_PER_SEC    1000L
#define USEC_PER_MSEC   1000L
#define USEC_PER_SEC    1000000L
#ifndef NSEC_PER_SEC
#define NSEC_PER_SEC (1000000000ULL)
#endif

int usleep(useconds_t microseconds)
{
    if (microseconds > RT_UINT32_MAX)
        return -RT_ERROR;

    if (!microseconds)
        return 0;
    fh_udelay(microseconds);
    return 0;
}

void msleep(unsigned int msecs)
{
    if (msecs > (RT_UINT32_MAX / USEC_PER_MSEC))
        return;

    if (!msecs)
        msecs = (msecs + (NSEC_PER_SEC / MSEC_PER_SEC) - 1) / (NSEC_PER_SEC / MSEC_PER_SEC) + 1;
    rt_thread_delay(rt_tick_from_millisecond(msecs));

    return;
}

int stime(time_t *t)
{
    struct timespec tv;

    tv.tv_sec  = *t;
    tv.tv_nsec = 0;
    clock_settime(CLOCK_REALTIME, &tv);

    return 0;
}

