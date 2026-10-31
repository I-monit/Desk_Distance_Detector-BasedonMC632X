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


#ifndef __FH_HASH_H__
#define __FH_HASH_H__

#include "hw_hash.h"
struct fh_hash_obj
{
    unsigned int base;
    int irq;
};
void rt_hw_hash_init(void);
#endif
