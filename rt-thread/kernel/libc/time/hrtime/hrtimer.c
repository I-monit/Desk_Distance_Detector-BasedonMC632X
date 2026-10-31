/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author        Notes
 * 2019-02-01    qin&songyh     first version
 */

#include "hrtimer.h"
#include "timekeeping.h"
#include "tick.h"

static int hrtimer_hres_enabled         = 1;
struct hrtimer_cpu_base global_cpu_base = {
    .clock_base = {
        {
            .index      = HRTIMER_BASE_MONOTONIC,
            .clockid    = CLOCK_MONOTONIC,
            .get_time   = &ktime_get,
            .resolution = KTIME_LOW_RES,
        },
        {
            .index      = HRTIMER_BASE_REALTIME,
            .clockid    = CLOCK_REALTIME,
            .get_time   = &ktime_get_real,
            .resolution = KTIME_LOW_RES,
        },
        {
            .index      = HRTIMER_BASE_BOOTTIME,
            .clockid    = CLOCK_BOOTTIME,
            .get_time   = &ktime_get_real,
            .resolution = KTIME_LOW_RES,
        },
    }};

/*
 * Is the high resolution mode active ?
 */
static inline int hrtimer_hres_active(void)
{
    return global_cpu_base.hres_active;
}

/**
 * hrtimer_forward - forward the timer expiry
 * @timer:  hrtimer to forward
 * @now:    forward past this time
 * @interval:   the interval to forward
 *
 * Forward the timer expiry so it will expire in the future.
 * Returns the number of overruns.
 */
uint64_t hrtimer_forward(struct hrtimer *timer, ktime_t now, ktime_t interval)
{
    uint64_t orun = 1;
    ktime_t delta;

    delta = ktime_sub(now, hrtimer_get_expires(timer));

    if (delta.tv64 < 0)
        return 0;

    if (interval.tv64 < timer->base->resolution.tv64)
        interval.tv64 = timer->base->resolution.tv64;

    if (unlikely(delta.tv64 >= interval.tv64))
    {
        int64_t incr = ktime_to_ns(interval);

        orun = ktime_divns(delta, incr);
        hrtimer_add_expires_ns(timer, incr * orun);
        if (hrtimer_get_expires_tv64(timer) > now.tv64)
            return orun;
        /*
         * This (and the ktime_add() below) is the
         * correction for exact:
         */
        orun++;
    }
    hrtimer_add_expires(timer, interval);

    return orun;
}

/*
 * Reprogram the event source with checking both queues for the
 * next event
 * Called with interrupts disabled and base->lock held
 */
static void hrtimer_force_reprogram(struct hrtimer_cpu_base *cpu_base,
                                    int skip_equal)
{
    int i;
    struct hrtimer_clock_base *base = cpu_base->clock_base;
    ktime_t expires, expires_next;

    expires_next.tv64 = KTIME_MAX;

    for (i = 0; i < HRTIMER_MAX_CLOCK_BASES; i++, base++)
    {
        struct hrtimer *timer;
        struct timerqueue_node *next;

        next = timerqueue_getnext(&base->active);
        if (!next)
            continue;
        timer = rt_container_of(next, struct hrtimer, node);

        expires = ktime_sub(hrtimer_get_expires(timer), base->offset);
        /*
         * clock_was_set() has changed base->offset so the
         * result might be negative. Fix it up to prevent a
         * false positive in clockevents_program_event()
         */
        if (expires.tv64 < 0)
            expires.tv64 = 0;
        if (expires.tv64 < expires_next.tv64)
            expires_next = expires;
    }

    if (skip_equal && expires_next.tv64 == cpu_base->expires_next.tv64)
        return;

    cpu_base->expires_next.tv64 = expires_next.tv64;

    if (cpu_base->expires_next.tv64 != KTIME_MAX)
        tick_program_event(cpu_base->expires_next, 1);
}

/*
 * Retrigger next event is called after clock was set
 *
 * Called with interrupts disabled via on_each_cpu()
 */
static void retrigger_next_event(void *arg)
{
    struct hrtimer_cpu_base *base = &global_cpu_base;
    struct timespec realtime_offset, xtim, wtm, sleep;

    if (!hrtimer_hres_active())
        return;

    /* Optimized out for !HIGH_RES */
    get_xtime_and_monotonic_and_sleep_offset(&xtim, &wtm, &sleep);
    set_normalized_timespec(&realtime_offset, -wtm.tv_sec, -wtm.tv_nsec);

    /* Adjust CLOCK_REALTIME offset */
    /* todo: rt_mutex_take(&base->lock, RT_WAITING_FOREVER); */
    base->clock_base[HRTIMER_BASE_REALTIME].offset =
        timespec_to_ktime(realtime_offset);

    hrtimer_force_reprogram(base, 0);
    /* todo: rt_mutex_release(&base->lock); */
}

/*
 * Switch to high resolution mode
 */
static int hrtimer_switch_to_hres(void)
{
    int i;
    /*unsigned long flags;*/
    rt_base_t level;

    if (global_cpu_base.hres_active)
        return 1;

   /* rt_interrupt_enter(); */
    level = rt_hw_interrupt_disable();

    if (tick_init_highres())
    {
       /* rt_interrupt_leave(); */
        rt_hw_interrupt_enable(level);
        rt_kprintf(
            "Could not switch to high resolution mode\n");
        return 0;
    }
    global_cpu_base.hres_active = 1;
    for (i = 0; i < HRTIMER_MAX_CLOCK_BASES; i++)
        global_cpu_base.clock_base[i].resolution = KTIME_HIGH_RES;

    tick_setup_sched_timer();

    /* "Retrigger" the interrupt to get things going */
    retrigger_next_event(RT_NULL);
    /* rt_interrupt_leave(); */
    rt_hw_interrupt_enable(level);
    return 1;
}

/*
 * Sleep related functions:
 */
static enum hrtimer_restart hrtimer_wakeup(struct hrtimer *timer)
{
	struct hrtimer_sleeper *t =
		rt_container_of(timer, struct hrtimer_sleeper, timer);
	struct rt_thread *task = t->task;

	rt_thread_resume(task);
	rt_schedule();

	return HRTIMER_NORESTART;
}

void hrtimer_init_sleeper(struct hrtimer_sleeper *sl, struct rt_thread *task)
{
	sl->timer.function = hrtimer_wakeup;
	sl->task = task;
}

/*
 * Called from timer softirq every jiffy, expire hrtimers:
 *
 * For HRT its the fall back code to run the softirq in the timer
 * softirq context in case the hrtimer initialization failed or has
 * not been done yet.
 */
void hrtimer_run_pending(void)
{
    if (hrtimer_hres_active())
        return;

    /*
     * This _is_ ugly: We have to check in the softirq context,
     * whether we can switch to highres and / or nohz mode. The
     * clocksource switch happens in the timer interrupt with
     * xtime_lock held. Notification from there only sets the
     * check bit in the tick_oneshot code, otherwise we might
     * deadlock vs. xtime_lock.
     */
    if (tick_check_oneshot_change(!hrtimer_hres_enabled))
        hrtimer_switch_to_hres();
}

static const int hrtimer_clock_to_base_table[MAX_CLOCKS] = {
        [CLOCK_REALTIME]  = HRTIMER_BASE_REALTIME,
        [CLOCK_MONOTONIC] = HRTIMER_BASE_MONOTONIC,
        [CLOCK_BOOTTIME]  = HRTIMER_BASE_BOOTTIME,
};

static inline int hrtimer_clockid_to_base(clockid_t clock_id)
{
    return hrtimer_clock_to_base_table[clock_id];
}

void hrtimer_init(struct hrtimer *timer, int clock_id, enum hrtimer_mode mode)
{
    struct hrtimer_cpu_base *base RT_UNUSED;
    int base_id;

    rt_memset(timer, 0, sizeof(struct hrtimer));

    base = &global_cpu_base;

    if (clock_id == CLOCK_REALTIME && mode != HRTIMER_MODE_ABS)
        clock_id = CLOCK_MONOTONIC;

    base_id     = hrtimer_clockid_to_base(clock_id);
    timer->base = &global_cpu_base.clock_base[base_id];
    timerqueue_init(&timer->node);
}

/*
 * local version of hrtimer_peek_ahead_timers() called with interrupts
 * disabled.
 */
static void __hrtimer_peek_ahead_timers(void)
{
    struct tick_device *td;

    if (!hrtimer_hres_active())
        return;

    td = &cpu_tick_device;
    if (td && td->evtdev)
        hrtimer_interrupt(td->evtdev);
}

/**
 * hrtimer_peek_ahead_timers -- run soft-expired timers now
 *
 * hrtimer_peek_ahead_timers will peek at the timer queue of
 * the current cpu and check if there are any timers for which
 * the soft expires time has passed. If any such timers exist,
 * they are run immediately and then removed from the timer queue.
 *
 */
void hrtimer_peek_ahead_timers(void)
{
    /* rt_interrupt_enter(); */
    rt_base_t level;
     level = rt_hw_interrupt_disable();
    __hrtimer_peek_ahead_timers();
    rt_hw_interrupt_enable(level);
    /* rt_interrupt_leave(); */
}

static void run_hrtimer_softirq(void) { hrtimer_peek_ahead_timers(); }
/*
 * __remove_hrtimer - internal function to remove a timer
 *
 * Caller must hold the base lock.
 *
 * High resolution timer mode reprograms the clock event device when the
 * timer is the one which expires next. The caller can disable this by setting
 * reprogram to zero. This is useful, when the context does a reprogramming
 * anyway (e.g. timer interrupt)
 */
static void __remove_hrtimer(struct hrtimer *timer,
                             struct hrtimer_clock_base *base,
                             unsigned long newstate, int reprogram)
{
    if (!(timer->state & HRTIMER_STATE_ENQUEUED))
        goto out;

    if (&timer->node == timerqueue_getnext(&base->active))
    {
        /* Reprogram the clock event device. if enabled */
        if (reprogram && hrtimer_hres_active())
        {
            ktime_t expires;

            expires = ktime_sub(hrtimer_get_expires(timer), base->offset);
            if (base->cpu_base->expires_next.tv64 == expires.tv64)
                hrtimer_force_reprogram(base->cpu_base, 1);
        }
    }
    timerqueue_del(&base->active, &timer->node);
    if (!timerqueue_getnext(&base->active))
        base->cpu_base->active_bases &= ~(1 << base->index);
out:
    timer->state = newstate;
}

/*
 * enqueue_hrtimer - internal function to (re)start a timer
 *
 * The timer is inserted in expiry order. Insertion into the
 * red black tree is O(log(n)). Must hold the base lock.
 *
 * Returns 1 when the new timer is the leftmost timer in the tree.
 */
static int enqueue_hrtimer(struct hrtimer *timer,
                           struct hrtimer_clock_base *base)
{
    timerqueue_add(&base->active, &timer->node);
    base->cpu_base->active_bases |= 1 << base->index;

    /*
     * HRTIMER_STATE_ENQUEUED is or'ed to the current state to preserve the
     * state of a possibly running callback.
     */
    timer->state |= HRTIMER_STATE_ENQUEUED;

    return (&timer->node == base->active.next);
}

static inline struct hrtimer_clock_base *lock_hrtimer_base(
    const struct hrtimer *timer, unsigned long *flags)
{
    struct hrtimer_clock_base *base = timer->base;

    rt_mutex_take(&base->cpu_base->lock, RT_WAITING_FOREVER);

    return base;
}

/*
 * Counterpart to lock_hrtimer_base above:
 */
static inline void unlock_hrtimer_base(const struct hrtimer *timer,
                                       unsigned long *flags)
{
    rt_mutex_release(&timer->base->cpu_base->lock);
}

#define switch_hrtimer_base(t, b, p) (b)

/*
 * remove hrtimer, called with base lock held
 */
static inline int remove_hrtimer(struct hrtimer *timer,
                                 struct hrtimer_clock_base *base)
{
    if (hrtimer_is_queued(timer))
    {
        unsigned long state;
        int reprogram;

        /*
         * Remove the timer and force reprogramming when high
         * resolution mode is active and the timer is on the current
         * CPU. If we remove a timer on another CPU, reprogramming is
         * skipped. The interrupt event on this CPU is fired and
         * reprogramming happens in the interrupt handler. This is a
         * rare case and less expensive than a smp call.
         */
        reprogram = base->cpu_base == &global_cpu_base;
        /*
         * We must preserve the CALLBACK state flag here,
         * otherwise we could move the timer base in
         * switch_hrtimer_base.
         */
        state = timer->state & HRTIMER_STATE_CALLBACK;
        __remove_hrtimer(timer, base, state, reprogram);
        return 1;
    }
    return 0;
}

/*
 * Shared reprogramming for clock_realtime and clock_monotonic
 *
 * When a timer is enqueued and expires earlier than the already enqueued
 * timers, we have to check, whether it expires earlier than the timer for
 * which the clock event device was armed.
 *
 * Called with interrupts disabled and base->cpu_base.lock held
 */
static int hrtimer_reprogram(struct hrtimer *timer,
                             struct hrtimer_clock_base *base)
{
    struct hrtimer_cpu_base *cpu_base = &global_cpu_base;
    ktime_t expires = ktime_sub(hrtimer_get_expires(timer), base->offset);
    int res;

    /*
     * When the callback is running, we do not reprogram the clock event
     * device. The timer callback is either running on a different CPU or
     * the callback is executed in the hrtimer_interrupt context. The
     * reprogramming is handled either by the softirq, which called the
     * callback or at the end of the hrtimer_interrupt.
     */
    if (hrtimer_callback_running(timer))
        return 0;

    /*
     * CLOCK_REALTIME timer might be requested with an absolute
     * expiry time which is less than base->offset. Nothing wrong
     * about that, just avoid to call into the tick code, which
     * has now objections against negative expiry values.
     */
    if (expires.tv64 < 0)
        return -RT_ETIMEOUT;

    if (expires.tv64 >= cpu_base->expires_next.tv64)
        return 0;

    /*
     * If a hang was detected in the last timer interrupt then we
     * do not schedule a timer which is earlier than the expiry
     * which we enforced in the hang detection. We want the system
     * to make progress.
     */
    if (cpu_base->hang_detected)
        return 0;

    /*
     * Clockevents returns -ETIME, when the event was in the past.
     */
    res = tick_program_event(expires, 0);
    /* todo: if (!IS_ERR_VALUE(res)) */
    cpu_base->expires_next = expires;
    return res;
}

/*
 * When High resolution timers are active, try to reprogram. Note, that in case
 * the state has HRTIMER_STATE_CALLBACK set, no reprogramming and no expiry
 * check happens. The timer gets enqueued into the rbtree. The reprogramming
 * and expiry check is done in the hrtimer_interrupt or in the softirq.
 */
static inline int hrtimer_enqueue_reprogram(struct hrtimer *timer,
                                            struct hrtimer_clock_base *base,
                                            int wakeup)
{
    if (base->cpu_base->hres_active && hrtimer_reprogram(timer, base))
    {
        /* __raise_softirq_irqoff(HRTIMER_SOFTIRQ); */
        run_hrtimer_softirq();
        return 1;
    }

    return 0;
}

int __hrtimer_start_range_ns(struct hrtimer *timer, ktime_t tim,
                             unsigned long delta_ns,
                             const enum hrtimer_mode mode, int wakeup)
{
    struct hrtimer_clock_base *base, *new_base;
/* unsigned long flags; */
    int ret, leftmost;

    /* todo: base = lock_hrtimer_base(timer, &flags); */
    base = timer->base;

    /* Remove an active timer from the queue: */
    ret = remove_hrtimer(timer, base);

    /* Switch the timer base, if necessary: */
    new_base = switch_hrtimer_base(timer, base, mode & HRTIMER_MODE_PINNED);

    if (mode & HRTIMER_MODE_REL)
    {
        tim = ktime_add_safe(tim, new_base->get_time());
        /*
         * CONFIG_TIME_LOW_RES is a temporary way for architectures
         * to signal that they simply return xtime in
         * do_gettimeoffset(). In this case we want to round up by
         * resolution when starting a relative timer, to avoid short
         * timeouts. This will go away with the GTOD framework.
         */
    }

    hrtimer_set_expires_range_ns(timer, tim, delta_ns);

    leftmost = enqueue_hrtimer(timer, new_base);

    /*
     * Only allow reprogramming if the new base is on this CPU.
     * (it might still be on another CPU if the timer was pending)
     *
     * XXX send_remote_softirq() ?
     */
    if (leftmost && new_base->cpu_base == &global_cpu_base)
        hrtimer_enqueue_reprogram(timer, new_base, wakeup);

    /* todo: unlock_hrtimer_base(timer, &flags); */

    return ret;
}

/**
 * hrtimer_start_range_ns - (re)start an hrtimer on the current CPU
 * @timer:  the timer to be added
 * @tim:    expiry time
 * @delta_ns:   "slack" range for the timer
 * @mode:   expiry mode: absolute (HRTIMER_ABS) or relative (HRTIMER_REL)
 *
 * Returns:
 *  0 on success
 *  1 when the timer was active
 */
int hrtimer_start_range_ns(struct hrtimer *timer, ktime_t tim,
                           unsigned long delta_ns, const enum hrtimer_mode mode)
{
    return __hrtimer_start_range_ns(timer, tim, delta_ns, mode, 1);
}

static void __run_hrtimer(struct hrtimer *timer, ktime_t *now)
{
    struct hrtimer_clock_base *base   = timer->base;
/* struct hrtimer_cpu_base *cpu_base = base->cpu_base; */
    enum hrtimer_restart (*fn)(struct hrtimer *);
    int restart;

    __remove_hrtimer(timer, base, HRTIMER_STATE_CALLBACK, 0);
    fn = timer->function;

    /*
     * Because we run timers from hardirq context, there is no chance
     * they get migrated to another cpu, therefore its safe to unlock
     * the timer base.
     */
    /* todo: rt_mutex_release(&cpu_base->lock); */
    restart = fn(timer);
    /* todo: rt_mutex_take(&cpu_base->lock, RT_WAITING_FOREVER); */

    /*
     * Note: We clear the CALLBACK bit after enqueue_hrtimer and
     * we do not reprogramm the event hardware. Happens either in
     * hrtimer_start_range_ns() or in hrtimer_interrupt()
     */
    if (restart != HRTIMER_NORESTART)
    {
        enqueue_hrtimer(timer, base);
    }

    timer->state &= ~HRTIMER_STATE_CALLBACK;
}

int hrtimer_try_to_cancel(struct hrtimer *timer)
{
    struct hrtimer_clock_base *base;
    unsigned long flags;
    int ret = -1;

    base = lock_hrtimer_base(timer, &flags);

    if (!hrtimer_callback_running(timer))
        ret = remove_hrtimer(timer, base);

    unlock_hrtimer_base(timer, &flags);

    return ret;
}

/**
 * hrtimer_cancel - cancel a timer and wait for the handler to finish.
 * @timer:  the timer to be cancelled
 *
 * Returns:
 *  0 when the timer was not active
 *  1 when the timer was active
 */
int hrtimer_cancel(struct hrtimer *timer)
{
    for (;;)
    {
        int ret = hrtimer_try_to_cancel(timer);

        if (ret >= 0)
            return ret;
    }
    return 0;
}

/*
 * High resolution timer interrupt
 * Called with interrupts disabled
 */
void hrtimer_interrupt(struct clock_event_device *dev)
{
    struct hrtimer_cpu_base *cpu_base = &global_cpu_base;
    ktime_t expires_next, now, entry_time, delta;
    int i, retries = 0;

    cpu_base->nr_events++;
    dev->next_event.tv64 = KTIME_MAX;

    entry_time = now = ktime_get();
retry:
    expires_next.tv64 = KTIME_MAX;

    /* todo: rt_mutex_take(&cpu_base->lock, RT_WAITING_FOREVER); */
    /*
     * We set expires_next to KTIME_MAX here with cpu_base->lock
     * held to prevent that a timer is enqueued in our queue via
     * the migration code. This does not affect enqueueing of
     * timers which run their callback and need to be requeued on
     * this CPU.
     */
    cpu_base->expires_next.tv64 = KTIME_MAX;

    for (i = 0; i < HRTIMER_MAX_CLOCK_BASES; i++)
    {
        struct hrtimer_clock_base *base;
        struct timerqueue_node *node;
        ktime_t basenow;

        if (!(cpu_base->active_bases & (1 << i)))
            continue;

        base    = cpu_base->clock_base + i;
        basenow = ktime_add(now, base->offset);

        while ((node = timerqueue_getnext(&base->active)))
        {
            struct hrtimer *timer;

            timer = rt_container_of(node, struct hrtimer, node);

            /*
             * The immediate goal for using the softexpires is
             * minimizing wakeups, not running timers at the
             * earliest interrupt after their soft expiration.
             * This allows us to avoid using a Priority Search
             * Tree, which can answer a stabbing querry for
             * overlapping intervals and instead use the simple
             * BST we already have.
             * We don't add extra wakeups by delaying timers that
             * are right-of a not yet expired timer, because that
             * timer will have to trigger a wakeup anyway.
             */

            if (basenow.tv64 < hrtimer_get_softexpires_tv64(timer))
            {
                ktime_t expires;

                expires = ktime_sub(hrtimer_get_expires(timer), base->offset);
                if (expires.tv64 < expires_next.tv64)
                    expires_next = expires;
                break;
            }

            __run_hrtimer(timer, &basenow);
        }
    }

    /*
     * Store the new expiry value so the migration code can verify
     * against it.
     */
    cpu_base->expires_next = expires_next;
    /* todo: rt_mutex_release(&cpu_base->lock); */

    /* Reprogramming necessary ? */
    if (expires_next.tv64 == KTIME_MAX || !tick_program_event(expires_next, 0))
    {
        cpu_base->hang_detected = 0;
        return;
    }

    /*
     * The next timer was already expired due to:
     * - tracing
     * - long lasting callbacks
     * - being scheduled away when running in a VM
     *
     * We need to prevent that we loop forever in the hrtimer
     * interrupt routine. We give it 3 attempts to avoid
     * overreacting on some spurious event.
     */
    now = ktime_get();
    cpu_base->nr_retries++;
    if (++retries < 3)
        goto retry;
    /*
     * Give the system a chance to do something else than looping
     * here. We stored the entry time, so we know exactly how long
     * we spent here. We schedule the next event this amount of
     * time away.
     */
    cpu_base->nr_hangs++;
    cpu_base->hang_detected = 1;
    delta                   = ktime_sub(now, entry_time);
    if (delta.tv64 > cpu_base->max_hang_time.tv64)
        cpu_base->max_hang_time = delta;
    /*
     * Limit it to a sensible value as we enforce a longer
     * delay. Give the CPU at least 100ms to catch up.
     */
    if (delta.tv64 > 100 * NSEC_PER_MSEC)
        expires_next = ktime_add_ns(now, 100 * NSEC_PER_MSEC);
    else
        expires_next = ktime_add(now, delta);
    tick_program_event(expires_next, 1);
    rt_kprintf("hrtimer: interrupt took %llu ns\n", ktime_to_ns(delta));
}

/*
 * Initialize the high resolution related parts of global_cpu_base
 */
static inline void hrtimer_init_hres(struct hrtimer_cpu_base *base)
{
    base->expires_next.tv64 = KTIME_MAX;
    base->hres_active       = 0;
}

void hrtimers_init(void)
{
    struct hrtimer_cpu_base *base = &global_cpu_base;
    int i;

    rt_mutex_init(&base->lock, "hrtimers_lock", RT_IPC_FLAG_FIFO);

    for (i = 0; i < HRTIMER_MAX_CLOCK_BASES; i++)
    {
        base->clock_base[i].cpu_base = base;
        timerqueue_init_head(&base->clock_base[i].active);
    }

    hrtimer_init_hres(base);
}

/**
 * schedule_hrtimeout_range_clock - sleep until timeout
 * @expires:	timeout value (ktime_t)
 * @delta:	slack in expires timeout (ktime_t)
 * @mode:	timer mode, HRTIMER_MODE_ABS or HRTIMER_MODE_REL
 * @clock:	timer clock, CLOCK_MONOTONIC or CLOCK_REALTIME
 */
int schedule_hrtimeout_range_clock(ktime_t *expires, unsigned long delta,
        const enum hrtimer_mode mode, int clock)
{
    int flags;
    struct hrtimer_sleeper t;

    /*
     * Optimize when a zero timeout value is given. It does not
     * matter whether this is an absolute or a relative time.
     */
    if (expires && !expires->tv64) {
        return 0;
    }

    /*
     * A NULL parameter means "infinite"
     */
    if (!expires) {
        rt_schedule();
        return -EINTR;
    }

    flags = rt_hw_interrupt_disable();
    hrtimer_init(&t.timer, clock, mode);
    hrtimer_set_expires_range_ns(&t.timer, *expires, delta);
    hrtimer_init_sleeper(&t, rt_thread_self());
    hrtimer_start_expires(&t.timer, mode);
    rt_thread_suspend(rt_thread_self());
    rt_schedule();
    rt_hw_interrupt_enable(flags);
    hrtimer_cancel(&t.timer);

    return 0;
}

/**
 * schedule_hrtimeout_range - sleep until timeout
 * @expires:	timeout value (ktime_t)
 * @delta:	slack in expires timeout (ktime_t)
 * @mode:	timer mode, HRTIMER_MODE_ABS or HRTIMER_MODE_REL
 *
 * Make the current task sleep until the given expiry time has
 * elapsed. The routine will return immediately unless
 * the current task state has been set (see set_current_state()).
 *
 * The @delta argument gives the kernel the freedom to schedule the
 * actual wakeup to a time that is both power and performance friendly.
 * The kernel give the normal best effort behavior for "@expires+@delta",
 * but may decide to fire the timer earlier, but no earlier than @expires.
 *
 * You can set the task state as follows -
 *
 * %TASK_UNINTERRUPTIBLE - at least @timeout time is guaranteed to
 * pass before the routine returns.
 *
 * %TASK_INTERRUPTIBLE - the routine may return early if a signal is
 * delivered to the current task.
 *
 * The current task state is guaranteed to be TASK_RUNNING when this
 * routine returns.
 *
 * Returns 0 when the timer has expired otherwise -EINTR
 */
int schedule_hrtimeout_range(ktime_t *expires, unsigned long delta,
                    const enum hrtimer_mode mode)
{
    return schedule_hrtimeout_range_clock(expires, delta, mode,
                            CLOCK_MONOTONIC);
}
