#include "../kernel/linux/spinlock.h"
#include <rthw.h>
#include <rtthread.h>


void spin_lock(spinlock_t *lock)
{
    int times = 0;
    rt_base_t level;

    while (1)
    {
        level = rt_hw_interrupt_disable();
        if (!lock->count)
        {
            lock->count = 1;
            rt_hw_interrupt_enable(level);
            break;
        }
        rt_hw_interrupt_enable(level);

        if (++times >= 10 * RT_TICK_PER_SECOND)
        {
            if (times % RT_TICK_PER_SECOND == 0)
                rt_kprintf("JFFS2: error@spin_lock,count=%d.\n", lock->count);
        }
        rt_thread_delay(1);
    }
}

void spin_unlock(spinlock_t *lock)
{
    rt_base_t level;

    level = rt_hw_interrupt_disable();
    if (lock->count != 1)
    {
        rt_kprintf("JFFS2: error@spin_unlock,count=%d.\n", lock->count);
        *((int *)0) = 0; /*BUG*/
    }

    lock->count = 0;
    rt_hw_interrupt_enable(level);
}
