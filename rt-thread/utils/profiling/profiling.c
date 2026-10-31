#include <rtthread.h>
#include <rthw.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fh_arch.h"
#ifdef RT_USING_FINSH
#include "finsh.h"
#endif
#include "timer.h"

static rt_int32_t  startTick;
static rt_int32_t  prevTick;

static void thread_tick_count(struct rt_thread* from, struct rt_thread* to)
{
    rt_int32_t tick;    

    tick = (rt_uint32_t)read_pts();
    from->total_ticks += (tick - prevTick);
    from->enter_times++;

    prevTick = tick;
}

void prof_init(void)
{
    register rt_base_t level;
    struct rt_object_information *info;
    rt_thread_t thread;
    rt_list_t *list;
    rt_list_t *node;

    level = rt_hw_interrupt_disable();
    
    info = rt_object_get_information(RT_Object_Class_Thread);
    list = &info->object_list;
    for (node = list->next; node != list; node = node->next)
    {
        thread = rt_list_entry(node, struct rt_thread, list);
        thread->enter_times = 0;
        thread->total_ticks = 0;
    }

    startTick = prevTick = (rt_uint32_t)read_pts();
    
    rt_hw_interrupt_enable(level);
    
    /* set idle thread hook */
    rt_scheduler_sethook(thread_tick_count);
}

static void _prof_view(void)
{    
    struct rt_object_information *info;
    struct rt_list_node *list;
    struct rt_list_node *node;
    struct rt_thread *thread;
    rt_uint32_t total_cnt;
    rt_uint32_t ms;
    rt_uint32_t per;
    rt_uint32_t pts;
    
    info = rt_object_get_information(RT_Object_Class_Thread);
    list = &info->object_list;

    rt_kprintf("%-*s", RT_NAME_MAX,"thread");
    rt_kprintf(" pri     enter   tick(ms)   pct.\n");
    rt_kprintf("%-*s", RT_NAME_MAX,"------");
    rt_kprintf(" ------- ------- ------- ----\n");

    pts = (rt_uint32_t)read_pts();

    total_cnt = pts - startTick;
    startTick = prevTick = pts;

    for (node = list->next; node != list; node = node->next)
    {
    	thread = rt_list_entry(node, struct rt_thread, list);
        ms = thread->total_ticks >> 10;
        per = 100*ms/(total_cnt>>10);
        
        rt_kprintf("%-*s ", RT_NAME_MAX, thread->name);
        rt_kprintf("%-7d %-7d %-10d %-7d\n",
            thread->current_priority,
            thread->enter_times,
            ms,
            per);
        thread->enter_times = 0;
        thread->total_ticks = 0;
    }
}

void prof_view(void)
{
    static int last_tick;
    int now;
    int diff;

    now  = rt_tick_get();
    diff = now - last_tick;

    if (!last_tick || diff > 10*60*RT_TICK_PER_SECOND) /*ten minutes*/
    {
        prof_init();
        rt_thread_delay(3);
    }

    _prof_view();

    last_tick = now;
}

#ifdef RT_USING_FINSH
FINSH_FUNCTION_EXPORT(prof_init, initialize thread profiling);
//FINSH_FUNCTION_EXPORT(prof_deinit, detach profiling hook);
//FINSH_FUNCTION_EXPORT(prof_reset, reset profiling counter);
FINSH_FUNCTION_EXPORT(prof_view, view profiling result);
#endif
