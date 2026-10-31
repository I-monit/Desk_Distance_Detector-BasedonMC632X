/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author        Notes
 * 2019-02-01    qin&songyh     first version
 */

#ifndef __TICK_H__
#define __TICK_H__

#include "clockevents.h"
#include "hrtimer.h"

#define NOTIFY_DONE 0x0000      /* Don't care */
#define NOTIFY_OK 0x0001        /* Suits me */
#define NOTIFY_STOP_MASK 0x8000 /* Don't call further */
#define NOTIFY_BAD (NOTIFY_STOP_MASK | 0x0002)
/* Bad/Veto action */
/*
 * Clean way to return from the notifier and stop further calls.
 */
#define NOTIFY_STOP (NOTIFY_OK | NOTIFY_STOP_MASK)

enum tick_device_mode
{
    TICKDEV_MODE_PERIODIC,
    TICKDEV_MODE_ONESHOT,
};

struct tick_device
{
    struct clock_event_device *evtdev;
    enum tick_device_mode mode;
};

enum tick_nohz_mode
{
    NOHZ_MODE_INACTIVE,
    NOHZ_MODE_LOWRES,
    NOHZ_MODE_HIGHRES,
};

/**
 * struct tick_sched - sched tick emulation and no idle tick control/stats
 * @sched_timer:    hrtimer to schedule the periodic tick in high
 *          resolution mode
 * @idle_tick:      Store the last idle tick expiry time when the tick
 *          timer is modified for idle sleeps. This is necessary
 *          to resume the tick timer operation in the timeline
 *          when the CPU returns from idle
 * @tick_stopped:   Indicator that the idle tick has been stopped
 * @idle_jiffies:   jiffies at the entry to idle for idle time accounting
 * @idle_calls:     Total number of idle calls
 * @idle_sleeps:    Number of idle calls, where the sched tick was stopped
 * @idle_entrytime: Time when the idle call was entered
 * @idle_waketime:  Time when the idle was interrupted
 * @idle_exittime:  Time when the idle state was left
 * @idle_sleeptime: Sum of the time slept in idle with sched tick stopped
 * @iowait_sleeptime:   Sum of the time slept in idle with sched tick stopped,
 * with IO outstanding
 * @sleep_length:   Duration of the current idle sleep
 * @do_timer_lst:   CPU was the last one doing do_timer before going idle
 */
struct tick_sched
{
    struct hrtimer sched_timer;
    unsigned long check_clocks;
    enum tick_nohz_mode nohz_mode;
    ktime_t idle_tick;
    int inidle;
    int tick_stopped;
    unsigned long idle_jiffies;
    unsigned long idle_calls;
    unsigned long idle_sleeps;
    int idle_active;
    ktime_t idle_entrytime;
    ktime_t idle_waketime;
    ktime_t idle_exittime;
    ktime_t idle_sleeptime;
    ktime_t iowait_sleeptime;
    ktime_t sleep_length;
    unsigned long last_jiffies;
    unsigned long next_jiffies;
    ktime_t idle_expires;
    int do_timer_last;
};

extern struct tick_device cpu_tick_device;

int tick_init_highres(void);
int tick_check_new_device(struct clock_event_device *newdev);
void tickdevice_init(void);
int tick_program_event(ktime_t expires, int force);
void tick_setup_sched_timer(void);
int tick_check_oneshot_change(int allow_nohz);
#endif /* TICK_H_ */
