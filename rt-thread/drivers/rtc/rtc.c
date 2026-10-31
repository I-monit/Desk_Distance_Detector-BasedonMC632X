/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2012-01-29     aozima       first version.
 * 2012-04-12     aozima       optimization: find rtc device only first.
 * 2012-04-16     aozima       add scheduler lock for set_date and set_time.
 * 2018-02-16     armink       add auto sync time by NTP
 */

#include <time.h>
#include <string.h>
#include <rtthread.h>
#include <stdlib.h>
#ifdef RT_USING_RTC

/* Using NTP auto sync RTC time */
#ifdef RTC_SYNC_USING_NTP
/* NTP first sync delay time for network connect, unit: second */
#ifndef RTC_NTP_FIRST_SYNC_DELAY
#define RTC_NTP_FIRST_SYNC_DELAY                 (30)
#endif
/* NTP sync period, unit: second */
#ifndef RTC_NTP_SYNC_PERIOD
#define RTC_NTP_SYNC_PERIOD                      (1L*60L*60L)
#endif
#endif /* RTC_SYNC_USING_NTP */

extern int clock_time_system_init(void);

static rt_err_t _update_rtc(time_t now)
{
    rt_err_t ret = -RT_ERROR;
    rt_device_t device;

    if (now < 0)
    {
        rt_kprintf("update rtc time error, time=%lx\n", now);
        return -EINVAL;
    }

    device = rt_device_find("rtc");
    if (device == RT_NULL)
    {
        return -RT_ERROR;
    }

    /* update to RTC device. */
    ret = rt_device_control(device, RT_DEVICE_CTRL_RTC_SET_TIME, &now);
    if (ret == RT_EOK)
    {
        clock_time_system_init();
    }
    return ret;
}
/**
 * Set system date(time not modify).
 *
 * @param rt_uint32_t year  e.g: 2012.
 * @param rt_uint32_t month e.g: 12 (1~12).
 * @param rt_uint32_t day   e.g: 31.
 *
 * @return rt_err_t if set success, return RT_EOK.
 *
 */
rt_err_t set_date(rt_uint32_t year, rt_uint32_t month, rt_uint32_t day)
{
    time_t now;
    struct tm *p_tm;
    struct tm tm_new;

    /* get current time */
    now = time(RT_NULL);

    /* lock scheduler. */
    rt_enter_critical();
    /* converts calendar time time into local time. */
    p_tm = localtime(&now);
    /* copy the statically located variable */
    memcpy(&tm_new, p_tm, sizeof(struct tm));
    /* unlock scheduler. */
    rt_exit_critical();

    /* update date. */
    tm_new.tm_year = year - 1900;
    tm_new.tm_mon  = month - 1; /* tm_mon: 0~11 */
    tm_new.tm_mday = day;

    /* converts the local time in time to calendar time. */
    now = mktime(&tm_new);

    return _update_rtc(now);
}

/**
 * Set system time(date not modify).
 *
 * @param rt_uint32_t hour   e.g: 0~23.
 * @param rt_uint32_t minute e.g: 0~59.
 * @param rt_uint32_t second e.g: 0~59.
 *
 * @return rt_err_t if set success, return RT_EOK.
 *
 */
rt_err_t set_time(rt_uint32_t hour, rt_uint32_t minute, rt_uint32_t second)
{
    time_t now;
    struct tm *p_tm;
    struct tm tm_new;

    /* get current time */
    now = time(RT_NULL);

    /* lock scheduler. */
    rt_enter_critical();
    /* converts calendar time time into local time. */
    p_tm = localtime(&now);
    /* copy the statically located variable */
    memcpy(&tm_new, p_tm, sizeof(struct tm));
    /* unlock scheduler. */
    rt_exit_critical();

    /* update time. */
    tm_new.tm_hour = hour;
    tm_new.tm_min  = minute;
    tm_new.tm_sec  = second;

    /* converts the local time in time to calendar time. */
    now = mktime(&tm_new);

    return _update_rtc(now);
}


/**
 * Set system date and time.
 *
 * @param rt_uint32_t year  e.g: 2012.
 * @param rt_uint32_t month e.g: 12 (1~12).
 * @param rt_uint32_t day   e.g: 31.
 * @param rt_uint32_t hour   e.g: 0~23.
 * @param rt_uint32_t minute e.g: 0~59.
 * @param rt_uint32_t second e.g: 0~59.
 *
 * @return rt_err_t if set success, return RT_EOK.
 *
 */
rt_err_t set_datetime(rt_uint32_t year, rt_uint32_t month, rt_uint32_t day,
                      rt_uint32_t hour, rt_uint32_t minute, rt_uint32_t second)
{
    time_t now;
    struct tm tm_new;

    /* update date. */
    tm_new.tm_year = year - 1900;
    tm_new.tm_mon  = month - 1; /* tm_mon: 0~11 */
    tm_new.tm_mday = day;
    /* update time. */
    tm_new.tm_hour = hour;
    tm_new.tm_min  = minute;
    tm_new.tm_sec  = second;

    /* converts the local time in time to calendar time. */
    now = mktime(&tm_new);

    return _update_rtc(now);
}

#ifdef RTC_SYNC_USING_NTP
static void ntp_sync_thread_enrty(void *param)
{
    extern time_t ntp_sync_to_rtc(const char *host_name);
    /* first sync delay for network connect */
    rt_thread_delay(RTC_NTP_FIRST_SYNC_DELAY * RT_TICK_PER_SECOND);

    while (1)
    {
        ntp_sync_to_rtc(NULL);
        rt_thread_delay(RTC_NTP_SYNC_PERIOD * RT_TICK_PER_SECOND);
    }
}

int rt_rtc_ntp_sync_init(void)
{
    static rt_bool_t init_ok = RT_FALSE;
    rt_thread_t thread;

    if (init_ok)
    {
        return 0;
    }

    thread = rt_thread_create("ntp_sync", ntp_sync_thread_enrty, RT_NULL, 1536, 26, 2);
    if (thread)
    {
        rt_thread_startup(thread);
    }
    else
    {
        return -RT_ENOMEM;
    }

    init_ok = RT_TRUE;

    return RT_EOK;
}
INIT_COMPONENT_EXPORT(rt_rtc_ntp_sync_init);
#endif /* RTC_SYNC_USING_NTP */

#endif /* RT_USING_RTC */
