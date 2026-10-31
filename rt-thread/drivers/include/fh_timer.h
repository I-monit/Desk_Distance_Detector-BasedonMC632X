/**
 * Copyright (c) 2015-2019 Shanghai Fullhan Microelectronics Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 *
 * Change Logs:
 * Date           Author       Notes
 * 2019-04-12               the first version
 *
 */

#ifndef __FH_TIMER_H__
#define __FH_TIMER_H__

/****************************************************************************
* #include section
*    add #include here if any
***************************************************************************/
#include "fh_def.h"

/****************************************************************************
* #define section
*    add constant #define here if any
***************************************************************************/
#define TIMER_CTRL_ENABLE (1u << 0)
#define TIMER_CTRL_MODE (1u << 1)
#define TIMER_CTRL_INTMASK (1u << 2)
#define TIMER_CTRL_PWMEN (1u << 3)

/****************************************************************************
* ADT section
*    add Abstract Data Type definition here
***************************************************************************/
typedef struct
{
    RwReg TIMER_LOAD_COUNT;
    RwReg TIMER_CURRENT_VALUE;
    RwReg TIMER_CTRL_REG;
    RwReg TIMER_EOI;
    RwReg TIMER_INT_STATUS;
} fh_timer;

static inline unsigned int timern_base(int n)
{
    unsigned int base = 0;

    switch (n)
    {
    case 0:
    default:
        base = TMR_REG_BASE;
        break;
    case 1:
        base = TMR_REG_BASE+0x14;
        break;
    case 2:
        base = TMR_REG_BASE+0x28;
        break;
    case 3:
        base = TMR_REG_BASE+0x3c;
        break;
    }
    return base;
}


#define TIMERN_REG_BASE(n)          (timern_base(n))

#define REG_TIMER_LOADCNT(n)        (timern_base(n) + 0x00)
#define REG_TIMER_CUR_VAL(n)        (timern_base(n) + 0x04)
#define REG_TIMER_CTRL_REG(n)       (timern_base(n) + 0x08)
#define REG_TIMER_EOI_REG(n)        (timern_base(n) + 0x0C)
#define REG_TIMER_INTSTATUS(n)      (timern_base(n) + 0x10)

#define REG_TIMERS_INTSTATUS    (0xa0)

#define TIMER_INTSTS_TIMER(n)     (0x1<<(n))


#endif /* #ifndef _TIMER_ */
