/*
 * File      : runlog.c
 */

#include <rthw.h>
#include <rtthread.h>

#include <wdt.h>

#ifdef RT_ENABLE_RUNNING_LOG
#include "drivers/watchdog.h"

extern uint64_t read_pts(void);

struct trace_head
{
    int   unused; /*for kgd test...*/
    int   magic1;
    int   magic2;
    int   pos;    /*current pos...*/
    char  name[64-16];
};

struct trace_node
{
    int   unused; /*for kgd test...*/
    unsigned int pts;
    int   line;
    void *pfunc;
    char  name[64-16];
};

static unsigned int g_trace_mem_vaddr;
static unsigned int g_trace_pos;
static unsigned int g_trace_mem_len;

extern void mmu_clean_dcache(unsigned int addr, unsigned int size);

#define _WDT_STOP    (0)
#define _WDT_RESTORE (1)
static void _wdt_stop_or_restore(int op, int *save)
{
    int timeout;
    rt_device_t wdt_dev;

    wdt_dev = rt_device_find("fh_wdt0");
    if (wdt_dev == RT_NULL)
    {
        rt_kprintf("WDT Device Not Found!\n");
        return;
    }

    rt_device_open(wdt_dev, 0);

    if (op == _WDT_STOP)
    {
        rt_device_control(wdt_dev, RT_DEVICE_CTRL_WDT_GET_TIMEOUT, save);
        timeout = 79 * 60; /*in unit of second, max support 79 minutes...*/
        rt_device_control(wdt_dev, RT_DEVICE_CTRL_WDT_SET_TIMEOUT, &timeout);
    }
    else
    {
        rt_device_control(wdt_dev, RT_DEVICE_CTRL_WDT_SET_TIMEOUT, save);
    }
}


void runlog_init(int size)
{
    int num;
    char *buf;

    if (size < 1024 || size > 0x800000)
    {
        rt_kprintf("Invalid log buffer size %d.\n", size);
        return;
    }

    num = (size - sizeof(struct trace_head))/sizeof(struct trace_node);
    size = sizeof(struct trace_head) + num * sizeof(struct trace_node);

    buf = rt_malloc(size);
    if (!buf)
    {
        rt_kprintf("cann't alloc log buffer!\n");
        return;
    }

    rt_memset(buf, 0, size);
    ((struct trace_head *)buf)->magic1 = 0xacdf1949;
    ((struct trace_head *)buf)->magic2 = 0xacdf1950;
    g_trace_pos = sizeof(struct trace_head);
    g_trace_mem_len = size;
    g_trace_mem_vaddr = (unsigned int)buf;

    rt_kprintf("log buff: %p, length=%d.\n", buf, size);
}

void runlog_add_node(int line, void *pfunc, char *name, char *name2)
{
    struct trace_node *n;
    char *dst;
    char  chr;
    int   cnt;
    int   flagname1 = 1;

    register rt_base_t level;

    if (!g_trace_mem_vaddr)
        return;

    level = rt_hw_interrupt_disable();

    ((struct trace_head *)g_trace_mem_vaddr)->pos = g_trace_pos;
    n = (struct trace_node *)(g_trace_mem_vaddr + g_trace_pos);

    if (!name)
        name = "";
    if (!name2)
        name2 = "";

    n->line  = line;
    n->pts   =  (unsigned int)read_pts(); /*get PTS*/
    n->pfunc = pfunc;

    dst = n->name;
    cnt = sizeof(n->name);
    while (cnt > 0)
    {
        cnt--;
        if (cnt <= 0)
        {
            *(dst++) = 0;
        }
        else
        {
            chr = *(name++);
            if (chr)
            {
                *(dst++) = chr;
            }
            else
            {
                if (flagname1)
                {
                    *(dst++) = ':';
                    name = name2;
                    flagname1 = 0;
                }
                else
                {
                    *(dst++) = chr;
                    break;
                }
            }
        }
    }

    while (cnt > 0)
    {
        *(dst++) = 0;
        cnt--;
    }

    g_trace_pos += sizeof(struct trace_node);
    if (g_trace_pos >= g_trace_mem_len)
    {
        g_trace_pos = sizeof(struct trace_head);
    }

    mmu_clean_dcache((unsigned int)n, 64);
    mmu_clean_dcache((unsigned int)g_trace_mem_vaddr, 64);

    rt_hw_interrupt_enable(level);
}

void runlog_export(void)
{
    unsigned int pos;
    struct trace_node *n;
    int num;
    struct trace_head *head = (struct trace_head *)g_trace_mem_vaddr;
    int wdtbk;
    register rt_base_t level;

    if (!g_trace_mem_vaddr)
        return;

    level = rt_hw_interrupt_disable();

    _wdt_stop_or_restore(_WDT_STOP, &wdtbk);

    rt_kprintf("------------------begin export running log------------------\n");

    rt_kprintf("log buff: %p, length=%d, nextpos=%d.\n", g_trace_mem_vaddr, g_trace_mem_len, g_trace_pos);
    rt_kprintf("magic1=%08x,magic2=%08x,pos=%d\n", head->magic1, head->magic2, head->pos);

    pos = g_trace_pos;
    if (!pos)
    {
        rt_kprintf("invalid pos %d.\n", pos);
        goto Exit;
    }

    if (!(pos >= sizeof(struct trace_head) && pos < g_trace_mem_len))
    {
        rt_kprintf("invalid pos %d.\n", pos);
        goto Exit;
    }


    num = (g_trace_mem_len - sizeof(struct trace_head))/sizeof(struct trace_node);
    while (num-- > 0)
    {
        n = (struct trace_node *)(g_trace_mem_vaddr + pos);
        if (n->line)
        {
            rt_kprintf("[mem:%p][pts:%08x], line=%d, func=%p, name=%s.\n", n, n->pts, n->line, n->pfunc, n->name);
        }

        pos += sizeof(struct trace_node);
        if (pos >= g_trace_mem_len)
        {
            pos = sizeof(struct trace_head);
        }
    }


Exit:
    rt_kprintf("------------------end export running log------------------\n");
    _wdt_stop_or_restore(_WDT_RESTORE, &wdtbk);

    rt_hw_interrupt_enable(level);
}

void runlog_deinit(void)
{
    char *buf;

    buf = (char *)g_trace_mem_vaddr;
    rt_kprintf("release log buff: %p\n", buf);
    rt_free(buf);
    g_trace_mem_vaddr = 0;
    g_trace_pos = 0;
    g_trace_mem_len = 0;
}

#ifdef RT_USING_FINSH
#include <finsh.h>
FINSH_FUNCTION_EXPORT(runlog_init, start log footprint into memory.);
FINSH_FUNCTION_EXPORT(runlog_export, print the memory log.);
FINSH_FUNCTION_EXPORT(runlog_deinit, stop log footprint into memory.);
#endif

#endif /* RT_ENABLE_RUNNING_LOG */
