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

#ifndef __MOL_TIMER_H__
#define __MOL_TIMER_H__

#include "fh_def.h"

#define TMR_LOAD_LO        (0x00)
#define TMR_LOAD_HI        (0x04)
#define TMR_VALUE_LO       (0x08)
#define TMR_VALUE_HI       (0x0c)
#define TMR_CTRL           (0x10)
#define TMR_INT            (0x14)
#define TMR_VAL_SHDW_LO    (0x18)
#define TMR_VAL_SHDW_HI    (0x1C)

#define TMR_BASE_OFFSET(n)      (0x0 + 0x20 * n)

#define TMR_LOAD_LO_OFFSET(n)       (TMR_BASE_OFFSET(n) + TMR_LOAD_LO)
#define TMR_LOAD_HI_OFFSET(n)       (TMR_BASE_OFFSET(n) + TMR_LOAD_HI)
#define TMR_VALUE_LO_OFFSET(n)      (TMR_BASE_OFFSET(n) + TMR_VALUE_LO)
#define TMR_VALUE_HI_OFFSET(n)      (TMR_BASE_OFFSET(n) + TMR_VALUE_HI)
#define TMR_CTRL_OFFSET(n)          (TMR_BASE_OFFSET(n) + TMR_CTRL)
#define TMR_INT_OFFSET(n)           (TMR_BASE_OFFSET(n) + TMR_INT)
#define TMR_VAL_SHDW_LO_OFFSET(n)   (TMR_BASE_OFFSET(n) + TMR_VAL_SHDW_LO)
#define TMR_VAL_SHDW_HI_OFFSET(n)   (TMR_BASE_OFFSET(n) + TMR_VAL_SHDW_HI)

#define TMR_CHANNEL_0     (0)
#define TMR_CHANNEL_1     (1)
#define TMR_CHANNEL_2     (2)
#define TMR_CHANNEL_3     (3)

#define TMR_WIDTH_32B     (0)
#define TMR_WIDTH_64B     (1)
#define TMR_WIDTH_SHIFT   (16)

#define TMR_ENABLE        (0x1)
#define TMR_DISABLE       (0x0)
#define TMR_RUN_SHIFT     (1)

#define TMR_PERIOD_MODE   (0x1)
#define TMR_ONESHOT_MODE  (0x0)
#define TMR_MODE_SHIFT    (0)

#define TMR_IRQ_EN        (0x1)
#define TMR_IRQ_DIS       (0x0)
#define TMR_IRQEN_SHIFT   (0x0)

#define TMR_IRQ_RAW_EN    (0x1)
#define TMR_IRQ_RAW_SHIFT (2)

#define TMR_IRQ_CLR       (0x1)
#define TMR_CLR_SHIFT     (3)

#endif /* #ifndef _TIMER_ */
