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

#ifndef __MOL_GPIO_H__
#define __MOL_GPIO_H__

#include "gpiolib.h"

#define REG_GPIO_DATA                    (0x0000)
#define REG_GPIO_DMSK                    (0x0004)
#define REG_GPIO_DIR                     (0x0008)
#define REG_GPIO_IS                      (0x000C)
#define REG_GPIO_IBE                     (0x0010)
#define REG_GPIO_IEV                     (0x0014)
#define REG_GPIO_IE                      (0x0018)
#define REG_GPIO_RIS                     (0x001C)
#define REG_GPIO_MIS                     (0x0020)
#define REG_GPIO_IC                      (0x0024)
#define REG_GPIO_INEN                    (0x0028)
#define REG_GPIO_DB_EN                   (0x0030)
#define REG_GPIO_DB_BOTH                 (0x0034)
#define REG_GPIO_DB_INV                  (0x0038)

/*
 * GPIO Direction
 */
#define GPIO_DIR_INPUT          0
#define GPIO_DIR_OUTPUT         1

/*
 * GPIO Input status
 */
#define GPIO_INPUT_DISABLE      0
#define GPIO_INPUT_ENABLE       1

/*
 * GPIO Data mask
 */
#define GPIO_DATA_MASK          0
#define GPIO_DATA_UNMASK        1

/*
 * GPIO interrupt type
 */
#define GPIO_INT_TYPE_LEVEL     1
#define GPIO_INT_TYPE_EDGE      0

/*
 * GPIO interrupt polarity
 */
#define GPIO_INT_POL_LOW        0
#define GPIO_INT_POL_HIGH       1

#define GPIONUM_PER_CHIP        8

enum trigger_type {
    SOFTWARE,
    HARDWARE,
};

struct mol_gpio_chip
{
    struct gpio_chip chip;
    rt_list_t        list;
    unsigned int base;
    int irq;
    int id;
    enum trigger_type trigger_type;
    unsigned int pmchangedirmask;
    unsigned int pmsaveddirval;
};

#endif /* MOL_GPIO_H */
