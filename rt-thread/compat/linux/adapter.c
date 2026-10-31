/*
 *  * Copyright (c) 2015-2019 Shanghai Fullhan Microelectronics Co., Ltd.
 *  *
 *  * SPDX-License-Identifier: Apache-2.0
 *  *
 *  * Change Logs:
 *  * Date           Author       Notes
 *  * 2019-01-18      {fullhan}   the first version
 *  *
 */

#include <rtthread.h>
#include <stdlib.h>

#include "sys/sysinfo.h"
#include "board_info.h"

#define LINUX_REBOOT_CMD_RESTART 0x01234567

extern unsigned long long rt_uptime_tick_get(void);

pid_t gettid(void)
{
    return (pid_t) rt_thread_self();
}

#if 0
pid_t getpid(void)
{
    return (pid_t) rt_thread_self();
}
#endif

#ifndef random
long int random(void)
{
    return rand();
}
#endif

#ifndef srandom
void srandom(unsigned int __seed)
{
    srand(__seed);
}
#endif

int reboot(int cmd)
{
    extern void rt_hw_cpu_reset(void);

    if (cmd == LINUX_REBOOT_CMD_RESTART)
        rt_hw_cpu_reset();
    return 0;
}

int cmd_reboot(int argc, char **argv)
{
    fh_driver_shutdown();

    extern void machine_reset(void);
    machine_reset();

    return 0;
}

/* rt_memory_info is defined if RT_MEM_STATS defined */
/* yet this macro is hardcoded in RTT */
/* so declare it as extern directly here. */
extern void rt_memory_info(rt_uint32_t *total,
        rt_uint32_t *used, rt_uint32_t *max_used);
int sysinfo(struct sysinfo *info)
{
    rt_uint32_t mem_used, mem_totl;
    unsigned long long systick;

    systick = rt_uptime_tick_get();
    info->uptime = systick / RT_TICK_PER_SECOND + 1;

    rt_memory_info(&mem_totl, &mem_used, RT_NULL);
    info->totalram = mem_totl;
    info->freeram = mem_totl - mem_used;

    return 0;
}

const char *gai_strerror(int errcode)
{
    return "fh-unknown";
}

#ifdef RT_USING_FINSH
#include <finsh.h>
FINSH_FUNCTION_EXPORT(reboot, reboot);
#ifdef FINSH_USING_MSH
FINSH_FUNCTION_EXPORT_ALIAS(cmd_reboot, __cmd_reboot, Reboot system);
#endif
#endif
