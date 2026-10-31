/**
 * Copyright (c) 2015-2019 Shanghai Fullhan Microelectronics Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020-07-13     songyh    the first version
 *
 */


#ifndef __FH_STEPMOTOR_H__
#define __FH_STEPMOTOR_H__


#include "stepmotor.h"
struct fh_smt_obj
{
    int id;
    unsigned int base;
    int irq;
};
void rt_hw_stepmotor_init(void);

#endif
