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

#ifndef __FH_ICTL_H__
#define __FH_ICTL_H__

#include "fh_def.h"

typedef struct
{
    RwReg IRQ_EN_L;
    RwReg IRQ_EN_H;
    RwReg IRQ_MASK_L;
    RwReg IRQ_MASK_H;
    RwReg IRQ_FORCE_L;
    RwReg IRQ_FORCE_H;
    RwReg IRQ_RAWSTARUS_L;
    RwReg IRQ_RAWSTARUS_H;
    RwReg IRQ_STATUS_L;
    RwReg IRQ_STATUS_H;
    RwReg IRQ_MASKSTATUS_L;
    RwReg IRQ_MASKSTATUS_H;
    RwReg IRQ_FINALSTATUS_L;
    RwReg IRQ_FINALSTATUS_H;
    RwReg IRQ_VECTOR;
} fh_intc;

void ictl_close_all_isr(fh_intc *p);

void ictl_mask_isr(fh_intc *p, int irq);

void ictl_unmask_isr(fh_intc *p, int irq);

#endif
