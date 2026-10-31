/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2011-09-23     Bernard      the first version
 * 2011-10-05     Bernard      add thumb mode
 */
#include <rtthread.h>
#include <armv7.h>

/**
 * @addtogroup AM33xx
 */
/*@{*/

/**
 * This function will initialize thread stack
 *
 * @param tentry the entry of thread
 * @param parameter the parameter of entry
 * @param stack_addr the beginning stack address
 * @param texit the function will be called when thread exit
 *
 * @return stack address
 */
rt_uint8_t *rt_hw_stack_init(void *tentry, void *parameter,
                             rt_uint8_t *stack_addr, void *texit)
{
    rt_uint32_t *stk;

    stack_addr += sizeof(rt_uint32_t);
    stack_addr  = (rt_uint8_t *)RT_ALIGN_DOWN((rt_uint32_t)stack_addr, 8);
    stk      = (rt_uint32_t *)stack_addr;
    *(--stk) = (rt_uint32_t)tentry;         /* entry point */
    *(--stk) = (rt_uint32_t)texit;          /* lr */
    *(--stk) = 0xdeadbeef;                  /* r12 */
    *(--stk) = 0xdeadbeef;                  /* r11 */
    *(--stk) = 0xdeadbeef;                  /* r10 */
    *(--stk) = 0xdeadbeef;                  /* r9 */
    *(--stk) = 0xdeadbeef;                  /* r8 */
    *(--stk) = 0xdeadbeef;                  /* r7 */
    *(--stk) = 0xdeadbeef;                  /* r6 */
    *(--stk) = 0xdeadbeef;                  /* r5 */
    *(--stk) = 0xdeadbeef;                  /* r4 */
    *(--stk) = 0xdeadbeef;                  /* r3 */
    *(--stk) = 0xdeadbeef;                  /* r2 */
    *(--stk) = 0xdeadbeef;                  /* r1 */
    *(--stk) = (rt_uint32_t)parameter;      /* r0 : argument */
    /* cpsr */
    if ((rt_uint32_t)tentry & 0x01)
        *(--stk) = SVCMODE | 0x20;          /* thumb mode */
    else
        *(--stk) = SVCMODE;                 /* arm mode   */
#ifdef RT_USING_LWP
    /* TODO */
#endif

#ifdef RT_USING_VFP
    *(--stk) = 0xdeadbeef;      /* s63 : argument */
    *(--stk) = 0xdeadbeef;      /* s62 : argument */
    *(--stk) = 0xdeadbeef;      /* s61 : argument */
    *(--stk) = 0xdeadbeef;      /* s60 : argument */
    *(--stk) = 0xdeadbeef;      /* s59 : argument */
    *(--stk) = 0xdeadbeef;      /* s58 : argument */
    *(--stk) = 0xdeadbeef;      /* s57 : argument */
    *(--stk) = 0xdeadbeef;      /* s56 : argument */
    *(--stk) = 0xdeadbeef;      /* s55 : argument */
    *(--stk) = 0xdeadbeef;      /* s54 : argument */
    *(--stk) = 0xdeadbeef;      /* s53 : argument */
    *(--stk) = 0xdeadbeef;      /* s52 : argument */
    *(--stk) = 0xdeadbeef;      /* s51 : argument */
    *(--stk) = 0xdeadbeef;      /* s50 : argument */
    *(--stk) = 0xdeadbeef;      /* s49 : argument */
    *(--stk) = 0xdeadbeef;      /* s48 : argument */
    *(--stk) = 0xdeadbeef;      /* s47 : argument */
    *(--stk) = 0xdeadbeef;      /* s46 : argument */
    *(--stk) = 0xdeadbeef;      /* s45 : argument */
    *(--stk) = 0xdeadbeef;      /* s44 : argument */
    *(--stk) = 0xdeadbeef;      /* s43 : argument */
    *(--stk) = 0xdeadbeef;      /* s42 : argument */
    *(--stk) = 0xdeadbeef;      /* s41 : argument */
    *(--stk) = 0xdeadbeef;      /* s40 : argument */
    *(--stk) = 0xdeadbeef;      /* s39 : argument */
    *(--stk) = 0xdeadbeef;      /* s38 : argument */
    *(--stk) = 0xdeadbeef;      /* s37 : argument */
    *(--stk) = 0xdeadbeef;      /* s36 : argument */
    *(--stk) = 0xdeadbeef;      /* s35 : argument */
    *(--stk) = 0xdeadbeef;      /* s34 : argument */
    *(--stk) = 0xdeadbeef;      /* s33 : argument */
    *(--stk) = 0xdeadbeef;      /* s32 : argument */
    *(--stk) = 0xdeadbeef;      /* s31 : argument */
    *(--stk) = 0xdeadbeef;      /* s30 : argument */
    *(--stk) = 0xdeadbeef;      /* s29 : argument */
    *(--stk) = 0xdeadbeef;      /* s28 : argument */
    *(--stk) = 0xdeadbeef;      /* s27 : argument */
    *(--stk) = 0xdeadbeef;      /* s26 : argument */
    *(--stk) = 0xdeadbeef;      /* s25 : argument */
    *(--stk) = 0xdeadbeef;      /* s24 : argument */
    *(--stk) = 0xdeadbeef;      /* s23 : argument */
    *(--stk) = 0xdeadbeef;      /* s22 : argument */
    *(--stk) = 0xdeadbeef;      /* s21 : argument */
    *(--stk) = 0xdeadbeef;      /* s20 : argument */
    *(--stk) = 0xdeadbeef;      /* s19 : argument */
    *(--stk) = 0xdeadbeef;      /* s18 : argument */
    *(--stk) = 0xdeadbeef;      /* s17 : argument */
    *(--stk) = 0xdeadbeef;      /* s16 : argument */
    *(--stk) = 0xdeadbeef;      /* s15 : argument */
    *(--stk) = 0xdeadbeef;      /* s14 : argument */
    *(--stk) = 0xdeadbeef;      /* s13: argument */
    *(--stk) = 0xdeadbeef;      /* s12 : argument */
    *(--stk) = 0xdeadbeef;      /* s11 : argument */
    *(--stk) = 0xdeadbeef;      /* s10 : argument */
    *(--stk) = 0xdeadbeef;      /* s9 : argument */
    *(--stk) = 0xdeadbeef;      /* s8 : argument */
    *(--stk) = 0xdeadbeef;      /* s7 : argument */
    *(--stk) = 0xdeadbeef;      /* s6 : argument */
    *(--stk) = 0xdeadbeef;      /* s5 : argument */
    *(--stk) = 0xdeadbeef;      /* s4 : argument */
    *(--stk) = 0xdeadbeef;      /* s3 : argument */
    *(--stk) = 0xdeadbeef;      /* s2 : argument */
    *(--stk) = 0xdeadbeef;      /* s1 : argument */
    *(--stk) = 0xdeadbeef;      /* s0 : argument */
    *(--stk) = 0;      /* fpscr */
    *(--stk) = 0x40000000;      /*fpexc */
#endif

    /* return task's current stack address */
    return (rt_uint8_t *)stk;
}

/*@}*/
