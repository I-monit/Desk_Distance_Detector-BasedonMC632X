/**
 * Copyright (c) 2015-2019 Shanghai Fullhan Microelectronics Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 *
 * Change Logs:
 * Date           Author       Notes
 * 2019-02-15     wangyl       add signal init in rtthread_startup()
 *
 */

#include <rthw.h>
#include <rtthread.h>
#include <mmu.h>
#ifdef RT_USING_FINSH
#include <finsh.h>
#endif

#include "board.h"
#include "platform_def.h"
#include "fh_def.h"

#ifdef FH_USING_DMA_MEM
#include "dma_mem.h"
#endif

#ifdef RT_USING_PM
#include "fh_pm.h"
#endif

#include "timer.h"      /* for read_pts() */

extern void rt_hw_interrupt_init(void);
extern void rt_system_timer_init(void);
extern void rt_system_scheduler_init(void);
extern void rt_thread_idle_init(void);
extern void mmu_invalidate_icache(void);
extern void rt_hw_cpu_icache_enable(void);
extern void rt_show_version(void);
extern void rt_system_heap_init(void *begin_addr, void *end_addr);
extern void rt_hw_finsh_init(void);
extern void rt_application_init(void);

/*@{*/
#if defined(__CC_ARM)
extern int Image$$ER_ZI$$ZI$$Base;
extern int Image$$ER_ZI$$ZI$$Length;
extern int Image$$ER_ZI$$ZI$$Limit;
#elif (defined(__GNUC__))
rt_uint8_t _irq_stack_start[1024];
rt_uint8_t _fiq_stack_start[1024];
rt_uint8_t _undefined_stack_start[512];
rt_uint8_t _abort_stack_start[512];
rt_uint8_t _svc_stack_start[4096] SECTION(".nobss");
extern unsigned char __bss_start;
extern unsigned char __bss_end;
#endif

#ifdef RT_USING_FINSH
extern void finsh_system_init(void);
#endif

/**
 * This function will startup RT-Thread RTOS.
 */
#ifdef CONFIG_STARTUP_TIMECOST
static unsigned int g_os_startup_time;
unsigned int get_os_startup_time(void)
{
    return g_os_startup_time;
}
#endif
void startup_log(unsigned int val)
{
    SET_REG(UART0_REG_BASE, val);
}

#ifdef FH_FAST_BOOT
static void _do_fast_boot_init(void)
{
    volatile unsigned int *pgpio0;

    pgpio0 = (volatile unsigned int *)GPIO1_REG_BASE;
    pgpio0[0] |= 1 << (CONFIG_CIS0_RESET_GPIO - 32);
    pgpio0[0] |= 1 << (CONFIG_CIS1_RESET_GPIO - 32);
}
#endif

void rtthread_startup(void)
{
    unsigned int sztbl;
    struct mem_desc *mtbl;
#ifdef RT_USING_PM
    struct pm_param pm_param;
#endif
#ifdef CONFIG_STARTUP_TIMECOST
    g_os_startup_time = read_pts();
#endif
#ifdef FH_FAST_BOOT
    _do_fast_boot_init();
#endif

    /* disable interrupt first */
    rt_hw_interrupt_disable();

    /* initialize mmu */
    sztbl = get_mmu_table(&mtbl);
    rt_hw_init_mmu_table(mtbl, sztbl);
    rt_hw_mmu_init();

    rt_system_heap_init((void *)&__bss_end, (void *)FH_RTT_OS_MEM_END);

#ifdef RT_USING_PM
    rt_memset(&pm_param, 0, sizeof(pm_param));
#ifdef CONFIG_ARCH_FH8626V300
    pm_param.reg_cpu_addr = REG_CPU_ADDR;
#endif
    pm_param.reg_coh_addr = REG_COH_ADDR;
    pm_param.arm_in_slp = ARM_IN_SLP;
    fh_hw_pm_init((void *)&pm_param);
#endif
    /* initialize hardware interrupt */
    rt_hw_interrupt_init();

    /* initialize board */
    rt_hw_board_init();
    /* fixme: rt_kprintf("malloc start at:%x\n",(unsigned int)&__bss_end); */

#ifdef CP_DO_LOAD_CODE
    extern void start_load_arc(void);
    start_load_arc();
#endif

    /* show version */
#ifndef FH_FAST_BOOT
    rt_show_version();
#endif

    /* initialize tick */
    rt_system_tick_init();

    /* initialize kernel object */
    rt_system_object_init();

    /* initialize timer system */
    rt_system_timer_init();
/* initialize heap memory system */
#ifdef RT_USING_MODULE
    /* initialize module system*/
    rt_system_dlmodule_init();
#endif

#ifdef RT_USING_SIGNALS
    /* signal system initialization */
    rt_system_signal_init();
#endif

    /* initialize scheduler system */
    rt_system_scheduler_init();

    /* initialize application */
    rt_application_init();

    /* initialize system timer thread */
    rt_system_timer_thread_init();

    /* initialize idle thread */
    rt_thread_idle_init();

#ifdef RT_USING_SMP
    rt_hw_mpcore_up();
#endif /* RT_USING_SMP */

#ifdef RT_USING_SMP
    rt_hw_spin_lock(&_cpus_lock);
#endif

    /* start scheduler */
    rt_system_scheduler_start();

    /* never reach here */

    return;
}

int main(void)
{
    /* startup RT-Thread RTOS */
    rtthread_startup();

    return 0;
}

/*@}*/
