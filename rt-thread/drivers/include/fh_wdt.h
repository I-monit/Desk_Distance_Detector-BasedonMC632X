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

#ifndef __FH_WDT_H__
#define __FH_WDT_H__

#include "fh_def.h"

#define WDOG_CONTROL_REG_OFFSET            0x00
#define WDOG_CONTROL_REG_WDT_EN_MASK       0x01
#define WDOG_CONTROL_REG_RMOD_MASK         0x02
#define WDOG_TIMEOUT_RANGE_REG_OFFSET      0x04
#define WDOG_CURRENT_COUNT_REG_OFFSET      0x08
#define WDOG_INTERRUPT_STATUS_REG          0x14
#define WDOG_COUNTER_RESTART_REG_OFFSET    0x0c
#define WDOG_COUNTER_RESTART_KICK_VALUE    0x76

/* Hardware timeout in seconds */
#define WDT_HW_TIMEOUT 2

/* The maximum TOP (timeout period) value that can be set in the watchdog. */
#define FH_WDT_MAX_TOP 15

struct fh_wdt_obj
{
    int id;
    int irq;
    unsigned int base;
    struct clk *clk;
    void (*interrupt)(int irq, void *param);
};

void WDT_Enable(struct fh_wdt_obj *wdt_obj, int enable);
int WDT_IsEnable(struct fh_wdt_obj *wdt_obj);
void WDT_SetTopValue(struct fh_wdt_obj *wdt_obj, int top);
int WDT_GetTopValue(struct fh_wdt_obj *wdt_obj);
void WDT_SetCtrl(struct fh_wdt_obj *wdt_obj, UINT32 reg);
void WDT_Kick(struct fh_wdt_obj *wdt_obj);
unsigned int WDT_GetCurrCount(struct fh_wdt_obj *wdt_obj);

#endif /* FH_WDT_H_ */
