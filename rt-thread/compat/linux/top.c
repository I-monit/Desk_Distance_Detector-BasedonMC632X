/**
 * Copyright (c) 2015-2019 Shanghai Fullhan Microelectronics Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2019-01-22     panzm        first version
 */

#include <rtthread.h>

#if defined FINSH_USING_MSH && defined FH_USING_PROFILING

#include <finsh.h>
#include "msh.h"
#include <unistd.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>

/**
 * ref busybox top
Usage: top [-b] [-nCOUNT] [-dSECONDS] [-m]

Provide a view of process activity in real time.
Read the status of all processes from /proc each SECONDS
and display a screenful of them.
Keys:
        N/M/P/T: show CPU usage, sort by pid/mem/cpu/time
        S: show memory
        R: reverse sort
        H: toggle threads, 1: toggle SMP
        Q,^C: exit

Options:
        -b      Batch mode
        -n N    Exit after N iterations
        -d N    Delay between updates
        -m      Same as 's' key
 */
static void clearScreen(void)
{
    const char *CLEAR_SCREEN_ANSI = "\e[1;1H\e[2J";
    write(STDOUT_FILENO, CLEAR_SCREEN_ANSI, 12);
}

int wait_input(int count)
{
    int ch;
    struct timeval tWaitTime;
    int n = (int) STDIN_FILENO + 1;
    fd_set fdInput;

    tWaitTime.tv_sec = count;
    tWaitTime.tv_usec = 0;
    FD_ZERO(&fdInput);
    FD_SET(STDIN_FILENO, &fdInput);
    if (!select(n, &fdInput, NULL, NULL, &tWaitTime))
    {
        ch = 0;
    }
    else
    {
        ch = getc(stdin);
    }
    return ch;
}

void top_usage(void)
{
    rt_kprintf("system performance viewer\n\n");
    rt_kprintf("Usage:\n");
    rt_kprintf("  top -h:          show this help\n");
    rt_kprintf("  top [-d count]   refresh every {count} seconds[default: 2]\n");
}

extern void prof_view(void);
extern void prof_init(void);
int cmd_top(int argc, char *argv[])
{
    int count = 1;

    if (argc > 1)
    {
        if (strcmp(argv[1], "-h") == 0)
        {
            top_usage();
            return 0;
        }
        if (strcmp(argv[1], "-d") == 0)
        {
            if (argc < 3)
            {
                rt_kprintf("less args.\n");
                top_usage();
                return 0;
            }
            count = atoi(argv[2]);
            if (count == 0)
            {
                count = 2;
            }
        }
        else
        {
            rt_kprintf("un-recog param\n");
            top_usage();
            return 0;
        }
    }
    else
    {
        count = 2;
    }
    prof_init();
    rt_thread_delay(RT_TICK_PER_SECOND * 2);
    do
    {
        clearScreen();
        prof_view();
        rt_kprintf("\nPress 'q' to quit.\n");
    } while ('q' != wait_input(count));

    return 0;
}

FINSH_FUNCTION_EXPORT_ALIAS(cmd_top, __cmd_top,  provide a view of process activity in real time);

#endif /* FINSH_USING_MSH */
