#include <rtthread.h>
#include <rthw.h>
#include "fh_chip.h"
#ifdef RT_USING_FINSH
#include "finsh.h"
#endif
#include "timer.h"
#include "fh_pmu.h"


static unsigned int ipc_monitor_trace_pos;
static unsigned int ipc_monitor_trace_mem_len;
static unsigned int ipc_monitor_trace_mem_vaddr;

extern volatile rt_uint8_t rt_interrupt_nest;
extern struct rt_thread *rt_current_thread;

extern void rt_object_trytake_sethook(void (*hook)(struct rt_object *object));
extern void rt_object_put_sethook(void (*hook)(struct rt_object *object));

struct trace_head
{
    int   unused;
    int   magic1;
    int   magic2;
    int   pos;
};

struct trace_node
{
    int   resv;
    unsigned int pts;
    rt_thread_t ipc_thread;
    rt_object_t ipc_object;
    int take;
    int warn_on;
};

void ipc_take_monitor(struct rt_object *object)
{
    struct trace_node *n;

    register rt_base_t level;

    if (!ipc_monitor_trace_mem_vaddr)
        return;
    level = rt_hw_interrupt_disable();
    ((struct trace_head *)ipc_monitor_trace_mem_vaddr)->pos = ipc_monitor_trace_pos;
    n = (struct trace_node *)(ipc_monitor_trace_mem_vaddr + ipc_monitor_trace_pos);
    n->pts = read_pts();
    n->take = 1;
    n->ipc_thread = rt_current_thread;
    n->ipc_object = object;
    ipc_monitor_trace_pos += sizeof(struct trace_node);
    if (ipc_monitor_trace_pos >= ipc_monitor_trace_mem_len)
    {
        ipc_monitor_trace_pos = sizeof(struct trace_head);
    }

    rt_hw_interrupt_enable(level);
}


void ipc_release_monitor(struct rt_object *object)
{
    struct trace_node *n;
    register rt_base_t level;
    struct rt_ipc_object *obj;
    rt_list_t *list;
    rt_list_t *node;

    if (!ipc_monitor_trace_mem_vaddr)
        return;

    level = rt_hw_interrupt_disable();
    ((struct trace_head *)ipc_monitor_trace_mem_vaddr)->pos = ipc_monitor_trace_pos;
    n = (struct trace_node *)(ipc_monitor_trace_mem_vaddr + ipc_monitor_trace_pos);
    n->pts = read_pts();
    n->take = 0;
    n->warn_on = 0;
    if (rt_interrupt_nest == 0)
        n->ipc_thread = rt_current_thread;
    else
        n->ipc_thread = RT_NULL;
    n->ipc_object = object;
    obj = (struct rt_ipc_object *)rt_container_of(object, struct rt_ipc_object, parent);
    list = &obj->suspend_thread;
    for (node = list->next; node != list; node = node->next)
    {
        n->warn_on++;
    }
    ipc_monitor_trace_pos += sizeof(struct trace_node);
    if (ipc_monitor_trace_pos >= ipc_monitor_trace_mem_len)
    {
        ipc_monitor_trace_pos = sizeof(struct trace_head);
    }
    rt_hw_interrupt_enable(level);
}


void ipcmon_init(int size)
{
    int num;
    char *buf;

    if (size < 1024 || size > 0x800000)
    {
        rt_kprintf("Invalid monitor buffer size %d.\n", size);
        return;
    }

    num = (size - sizeof(struct trace_head))/sizeof(struct trace_node);
    size = sizeof(struct trace_head) + num * sizeof(struct trace_node);

    buf = rt_malloc(size);
    if (!buf)
    {
        rt_kprintf("cann't monitor log buffer!\n");
        return;
    }

    rt_memset(buf, 0, size);
    ((struct trace_head *)buf)->magic1 = 0xacdf1949;
    ((struct trace_head *)buf)->magic2 = 0xacdf1950;
    rt_object_trytake_sethook(ipc_take_monitor);
    rt_object_put_sethook(ipc_release_monitor);
    ipc_monitor_trace_pos = sizeof(struct trace_head);
    ipc_monitor_trace_mem_len = size;
    ipc_monitor_trace_mem_vaddr = (unsigned int)buf;

    rt_kprintf("monitor buff: %p, length=%d.\n", buf, size);
}

void ipcmon_export(void)
{
    unsigned int pos;
    struct trace_node *n;
    int num;
    struct trace_head *head = (struct trace_head *)ipc_monitor_trace_mem_vaddr;
    register rt_base_t level;

    if (!ipc_monitor_trace_mem_vaddr)
        return;

    level = rt_hw_interrupt_disable();

    fh_pmu_wdt_pause();
    rt_kprintf("------------------begin export ipc monitor log------------------\n");

    rt_kprintf("monitor buff: %p, length=%d, nextpos=%d.\n", ipc_monitor_trace_mem_vaddr, ipc_monitor_trace_mem_len, ipc_monitor_trace_pos);
    rt_kprintf("magic1=%08x,magic2=%08x,pos=%d\n", head->magic1, head->magic2, head->pos);

    pos = ipc_monitor_trace_pos;
    if (!pos)
    {
        rt_kprintf("invalid pos %d.\n", pos);
        goto Exit;
    }

    if (!(pos >= sizeof(struct trace_head) && pos < ipc_monitor_trace_mem_len))
    {
        rt_kprintf("invalid pos %d.\n", pos);
        goto Exit;
    }


    num = (ipc_monitor_trace_mem_len - sizeof(struct trace_head))/sizeof(struct trace_node);
    while (num-- > 0)
    {
        n = (struct trace_node *)(ipc_monitor_trace_mem_vaddr + pos);
        if (n->ipc_object != RT_NULL)
        {
            if (n->take)
            {
                rt_kprintf("Thread: %s try take %s at pts:%08x\n", n->ipc_thread->name,\
                            n->ipc_object->name, n->pts);
            }
            else
            {
                if (n->ipc_thread == RT_NULL)
                    rt_kprintf("Interrupt relese %s at pts:%08x,%d thread suspend on\n", n->ipc_object->name, n->pts, n->warn_on);
                else
                    rt_kprintf("Thread: %s release %s at pts:%08x,%d thread suspend on\n", n->ipc_thread->name,\
                                n->ipc_object->name, n->pts, n->warn_on);
            }
        }

        pos += sizeof(struct trace_node);
        if (pos >= ipc_monitor_trace_mem_len)
        {
            pos = sizeof(struct trace_head);
        }
    }


Exit:
    rt_kprintf("------------------end export ipc monitor log------------------\n");
    fh_pmu_wdt_resume();

    rt_hw_interrupt_enable(level);
}


void ipcmon_deinit(void)
{
    char *buf;

    buf = (char *)ipc_monitor_trace_mem_vaddr;
    rt_kprintf("release log buff: %p\n", buf);
    rt_free(buf);
    ipc_monitor_trace_mem_vaddr = 0;
    ipc_monitor_trace_pos = 0;
    ipc_monitor_trace_mem_len = 0;
}

#ifdef RT_USING_FINSH
#include <finsh.h>
FINSH_FUNCTION_EXPORT(ipcmon_init, start log footprint into memory.);
FINSH_FUNCTION_EXPORT(ipcmon_export, print the memory log.);
FINSH_FUNCTION_EXPORT(ipcmon_deinit, stop log footprint into memory.);
#endif
