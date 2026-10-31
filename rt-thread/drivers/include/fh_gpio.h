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

#ifndef __FH_GPIO_H__
#define __FH_GPIO_H__

#include "gpiolib.h"

#define REG_GPIO_SWPORTA_DR             (0x0000)
#define REG_GPIO_SWPORTA_DDR            (0x0004)
#define REG_GPIO_PORTA_CTL              (0x0008)
#define REG_GPIO_SWPORTB_DR             (0x000C)
#define REG_GPIO_SWPORTB_DDR            (0x0010)
#define REG_GPIO_PORTB_CTL              (0x0014)
#define REG_GPIO_INTEN                  (0x0030)
#define REG_GPIO_INTMASK                (0x0034)
#define REG_GPIO_INTTYPE_LEVEL          (0x0038)
#define REG_GPIO_INT_POLARITY           (0x003C)
#define REG_GPIO_INTSTATUS              (0x0040)
#define REG_GPIO_RAWINTSTATUS           (0x0044)
#define REG_GPIO_DEBOUNCE               (0x0048)
#define REG_GPIO_PORTA_EOI              (0x004C)
#define REG_GPIO_EXT_PORTA              (0x0050)
#define REG_GPIO_EXT_PORTB              (0x0054)
#define REG_GPIO_INT_BOTH               (0x0068)
#define CLK_GPIO_DB_CTRL                (0xf0000040)

enum trigger_type {
    SOFTWARE,
    HARDWARE,
};

struct fh_gpio_chip
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

#endif /* FH_GPIO_H_ */
