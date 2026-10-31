/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author        Notes
 * 2019-02-01    qin&songyh     first version
 */

#include "tick.h"
#include "clockevents.h"
#include "timekeeping.h"
ktime_t tick_next_period;
ktime_t tick_period;
struct tick_device cpu_tick_device/* = {0}*/;
static struct rt_mutex tickdevice_mutex;
static struct tick_sched tick_cpu_sched /*= {0}*/;

static ktime_t last_jiffies_update;

/* Limit min_delta to a jiffie */
#define MIN_DELTA_LIMIT (NSEC_PER_SEC / RT_TICK_PER_SECOND)

/*
 * We rearm the timer until we get disabled by the idle code.
 * Called with interrupts disabled and timer->base->cpu_base->lock held.
 */
static enum hrtimer_restart tick_sched_timer(struct hrtimer *timer)
{
    struct tick_sched *ts RT_UNUSED;
    ts = &tick_cpu_sched;
    ktime_t now           = ktime_get();

#ifdef CONFIG_NO_HZ
    /*
     * Check if the do_timer duty was dropped. We don't care about
     * concurrency: This happens only when the cpu in charge went
     * into a long sleep. If two cpus happen to assign themself to
     * this duty, then the jiffies update is still serialized by
     * xtime_lock.
     */
    if (unlikely(tick_do_timer_cpu == TICK_DO_TIMER_NONE))
        tick_do_timer_cpu = cpu;
#endif

    /* Check, if the jiffies need an update */

    update_wall_time();
#if 0
    /*
     * Do not call, when we are not in irq context and have
     * no valid regs pointer
     */
    if (regs)
    {
        /*
         * When we are idle and the tick is stopped, we have to touch
         * the watchdog as we might not schedule for a really long
         * time. This happens on complete idle SMP systems while
         * waiting on the login prompt. We also increment the "start of
         * idle" jiffy stamp so the idle accounting adjustment we do
         * when we go busy again does not account too much ticks.
         */
        if (ts->tick_stopped)
        {
            touch_softlockup_watchdog();
            ts->idle_jiffies++;
        }
        update_process_times(user_mode(regs));
        profile_tick(CPU_PROFILING);
    }
#endif
    hrtimer_forward(timer, now, tick_period);

    return HRTIMER_RESTART;
}

/*
 * Initialize and return retrieve the jiffies update.
 */
static ktime_t tick_init_jiffy_update(void)
{
    ktime_t period;

    /* Did we start the jiffies update yet ? */
    if (last_jiffies_update.tv64 == 0)
        last_jiffies_update = tick_next_period;
    period = last_jiffies_update;

    return period;
}

/**
 * tick_setup_sched_timer - setup the tick emulation timer
 */
void tick_setup_sched_timer(void)
{
    struct tick_sched *ts = &tick_cpu_sched;
    ktime_t now           = ktime_get();

    /*
     * Emulate tick processing via per-CPU hrtimers:
     */
    hrtimer_init(&ts->sched_timer, CLOCK_MONOTONIC, HRTIMER_MODE_ABS);
    ts->sched_timer.function = tick_sched_timer;

    /* Get the next period (per cpu) */
    hrtimer_set_expires(&ts->sched_timer, tick_init_jiffy_update());

    for (;;)
    {
        hrtimer_forward(&ts->sched_timer, now, tick_period);
        hrtimer_start_expires(&ts->sched_timer, HRTIMER_MODE_ABS_PINNED);
        /* Check, if the timer was already in the past */
        if (hrtimer_active(&ts->sched_timer))
            break;
        now = ktime_get();
    }

#ifdef CONFIG_NO_HZ
    if (tick_nohz_enabled)
    {
        ts->nohz_mode = NOHZ_MODE_HIGHRES;
        printk(KERN_INFO "Switched to NOHz mode on CPU #%d\n",
               smp_processor_id());
    }
#endif
}

/**
 * Check, if a change happened, which makes oneshot possible.
 *
 * Called cyclic from the hrtimer softirq (driven by the timer
 * softirq) allow_nohz signals, that we can switch into low-res nohz
 * mode, because high resolution timers are disabled (either compile
 * or runtime).
 */
int tick_check_oneshot_change(int allow_nohz)
{
#if 0
    struct tick_sched *ts = &__get_cpu_var(tick_cpu_sched);

    if (!test_and_clear_bit(0, &ts->check_clocks))
        return 0;

    if (ts->nohz_mode != NOHZ_MODE_INACTIVE)
        return 0;

    if (!timekeeping_valid_for_hres() || !tick_is_oneshot_available())
        return 0;

    if (!allow_nohz)
        return 1;

    tick_nohz_switch_to_nohz();
    return 0;
#endif
    if (!allow_nohz)
        return 1;
    return 0;
}

/**
 * tick_switch_to_oneshot - switch to oneshot mode
 */
int tick_switch_to_oneshot(void (*handler)(struct clock_event_device *))
{
    struct tick_device *td         = &cpu_tick_device;
    struct clock_event_device *dev = td->evtdev;

    if (!dev || !(dev->features & CLOCK_EVT_FEAT_ONESHOT))
    {
        rt_kprintf(
            "Clockevents: could not switch to one-shot mode:");
        if (!dev)
        {
            rt_kprintf(" no tick device\n");
        }
        else
        {
            rt_kprintf(" %s does not support one-shot mode.\n", dev->name);
        }
        return -RT_ERROR;
    }

    td->mode           = TICKDEV_MODE_ONESHOT;
    dev->event_handler = handler;
    clockevents_set_mode(dev, CLOCK_EVT_MODE_ONESHOT);
    /* todo: tick_broadcast_switch_to_oneshot(); */
    return 0;
}

/**
 * tick_init_highres - switch to high resolution mode
 *
 * Called with interrupts disabled.
 */
int tick_init_highres(void)
{
    return tick_switch_to_oneshot(hrtimer_interrupt);
}

static int tick_increase_min_delta(struct clock_event_device *dev)
{
    /* Nothing to do if we already reached the limit */
    if (dev->min_delta_ns >= MIN_DELTA_LIMIT)
        return -RT_ETIMEOUT;

    if (dev->min_delta_ns < 5000)
        dev->min_delta_ns = 5000;
    else
        dev->min_delta_ns += dev->min_delta_ns >> 1;

    if (dev->min_delta_ns > MIN_DELTA_LIMIT)
        dev->min_delta_ns = MIN_DELTA_LIMIT;

    rt_kprintf("CE: %s increased min_delta_ns to %llu nsec\n",
               dev->name ? dev->name : "?",
               (unsigned long long)dev->min_delta_ns);
    return 0;
}

/**
 * tick_program_event internal worker function
 */
int tick_dev_program_event(struct clock_event_device *dev, ktime_t expires,
                           int force)
{
    ktime_t now = ktime_get();
    int i;

    for (i = 0;;)
    {
        int ret = clockevents_program_event(dev, expires, now);

        if (!ret || !force)
            return ret;

        dev->retries++;
        /*
         * We tried 3 times to program the device with the given
         * min_delta_ns. If that's not working then we increase it
         * and emit a warning.
         */
        if (++i > 2)
        {
            /* Increase the min. delta and try again */
            if (tick_increase_min_delta(dev))
            {
                /*
                 * Get out of the loop if min_delta_ns
                 * hit the limit already. That's
                 * better than staying here forever.
                 *
                 * We clear next_event so we have a
                 * chance that the box survives.
                 */
                rt_kprintf("CE: Reprogramming failure. Giving up\n");
                dev->next_event.tv64 = KTIME_MAX;
                return -RT_ETIMEOUT;
            }
            i = 0;
        }

        now     = ktime_get();
        expires = ktime_add_ns(now, dev->min_delta_ns);
    }
}

/**
 * tick_program_event
 */
int tick_program_event(ktime_t expires, int force)
{
    struct clock_event_device *dev = cpu_tick_device.evtdev;

    return tick_dev_program_event(dev, expires, force);
}

/**
 * tick_setup_oneshot - setup the event device for oneshot mode (hres or nohz)
 */
void tick_setup_oneshot(struct clock_event_device *newdev,
                        void (*handler)(struct clock_event_device *),
                        ktime_t next_event)
{
    newdev->event_handler = handler;
    clockevents_set_mode(newdev, CLOCK_EVT_MODE_ONESHOT);
    tick_dev_program_event(newdev, next_event, 1);
}

/*
 * Periodic tick
 */
static void tick_periodic(void)
{
    /* Keep track of the next tick event */
    tick_next_period = ktime_add(tick_next_period, tick_period);
    rt_tick_increase();
    update_wall_time();
    hrtimer_run_pending();
}

/*
 * Event handler for periodic ticks
 */
void tick_handle_periodic(struct clock_event_device *dev)
{
    ktime_t next;

    tick_periodic();

    if (dev->mode != CLOCK_EVT_MODE_ONESHOT)
        return;
    /*
     * Setup the next period for devices, which do not have
     * periodic mode:
     */
    next = ktime_add(dev->next_event, tick_period);
    for (;;)
    {
        if (!clockevents_program_event(dev, next, ktime_get()))
            return;
        /*
         * Have to be careful here. If we're in oneshot mode,
         * before we call tick_periodic() in a loop, we need
         * to be sure we're using a real hardware clocksource.
         * Otherwise we could get trapped in an infinite
         * loop, as the tick_periodic() increments jiffies,
         * when then will increment time, posibly causing
         * the loop to trigger again and again.
         */
        if (timekeeping_valid_for_hres())
            tick_periodic();
        next = ktime_add(next, tick_period);
    }
}

/*
 * Setup the device for a periodic tick
 */
void tick_setup_periodic(struct clock_event_device *dev)
{
    dev->event_handler = tick_handle_periodic;

    if (dev->features & CLOCK_EVT_FEAT_PERIODIC)
    {
        clockevents_set_mode(dev, CLOCK_EVT_MODE_PERIODIC);
    }
    else
    {
        ktime_t next;

        next = tick_next_period;
        clockevents_set_mode(dev, CLOCK_EVT_MODE_ONESHOT);

        for (;;)
        {
            if (!clockevents_program_event(dev, next, ktime_get()))
                return;
            next = ktime_add(next, tick_period);
        }
    }
}

/*
 * Noop handler when we shut down an event device
 */
void clockevents_handle_noop(struct clock_event_device *dev) {}
/*
 * Setup the tick device
 */
static void tick_setup_device(struct tick_device *td,
                              struct clock_event_device *newdev)
{
    ktime_t next_event;
    void (*handler)(struct clock_event_device *) = RT_NULL;

    /*
     * First device setup ?
     */
    if (!td->evtdev)
    {
        tick_next_period = ktime_get();
        tick_period      = ktime_set(0, NSEC_PER_SEC / RT_TICK_PER_SECOND);

        /*
         * Startup in periodic mode first.
         */
        td->mode = TICKDEV_MODE_PERIODIC;
    }
    else
    {
        handler                   = td->evtdev->event_handler;
        next_event                = td->evtdev->next_event;
        td->evtdev->event_handler = clockevents_handle_noop;
    }

    td->evtdev = newdev;

    if (td->mode == TICKDEV_MODE_PERIODIC)
        tick_setup_periodic(newdev);
    else
        tick_setup_oneshot(newdev, handler, next_event);
}

/*
 * Check, if the new registered device should be used.
 */
int tick_check_new_device(struct clock_event_device *newdev)
{
    struct clock_event_device *curdev;
    struct tick_device *td;
    int /*cpu,*/ ret = NOTIFY_OK;
/* unsigned long flags; */

    rt_mutex_take(&tickdevice_mutex, RT_WAITING_FOREVER);

    td     = &cpu_tick_device;
    curdev = td->evtdev;

    /*
     * If we have an active device, then check the rating and the oneshot
     * feature.
     */
    if (curdev)
    {
        /*
         * Prefer one shot capable devices !
         */
        if ((curdev->features & CLOCK_EVT_FEAT_ONESHOT) &&
            !(newdev->features & CLOCK_EVT_FEAT_ONESHOT))
            goto out_bc;
        /*
         * Check the rating
         */
        if (curdev->rating >= newdev->rating)
            goto out_bc;
    }

    /*
     * Replace the eventually existing device by the new
     * device.
     */
    clockevents_exchange_device(curdev, newdev);
    tick_setup_device(td, newdev);
    if (newdev->features & CLOCK_EVT_FEAT_ONESHOT)
        /* todo: tick_oneshot_notify(); */

        rt_mutex_release(&tickdevice_mutex);
    return NOTIFY_STOP;

out_bc:
    rt_mutex_release(&tickdevice_mutex);

    return ret;
}

void tickdevice_init(void)
{
    rt_mutex_init(&tickdevice_mutex, "tickdevice_lock", RT_IPC_FLAG_FIFO);
}
