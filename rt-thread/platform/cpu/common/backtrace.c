/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2008-07-29     Bernard      first version from QiuYi implementation
 */

#include <rtthread.h>
#include "platform_def.h"

#ifdef __GNUC__
/*
-->High Address,Stack Top
PC<------|
LR          |
IP          |
FP          |
......         |
PC<-|     |
LR     |     |
IP     |     |
FP---|-- |
......     |
PC     |
LR     |
IP      |
FP---
-->Low Address,Stack Bottom
*/
#ifdef RT_USING_BACKTRACE
void rt_hw_backtrace(rt_uint32_t *fp, rt_uint32_t thread_entry)
{
    rt_uint32_t i, pc, func_entry;

    pc = *fp;
    rt_kprintf("1:[<0x%x>]\n", pc-0xC);

    for (i = 0; i < 10; i++)
    {
        if ((rt_uint32_t)fp < FH_DDR_START || (rt_uint32_t)fp >= FH_DDR_END)
            break;

        fp = (rt_uint32_t *)*(fp - 3);

        if ((rt_uint32_t)fp < FH_DDR_START || (rt_uint32_t)fp >= FH_DDR_END)
            break;

        pc = *fp;

        func_entry = pc - 0xC;

        if (func_entry <= FH_DDR_START)
            break;

        if (func_entry == thread_entry)
        {
            rt_kprintf("EntryPoint:[<0x%x>]\n", func_entry);
            break;
        }

        rt_kprintf("%d:[<0x%x>]\n", i+2, func_entry);
    }
}
#endif
#else
void rt_hw_backtrace(rt_uint32_t *fp, rt_uint32_t thread_entry)
{
    /* old compiler implementation */
}
#endif
