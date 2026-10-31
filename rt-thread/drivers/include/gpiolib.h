/**
 * Copyright (c) 2015-2019 Shanghai Fullhan Microelectronics Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 *
 * Change Logs:
 * Date           Author       Notes
 *
 */

#ifndef __GPIOLIB_H__
#define __GPIOLIB_H__

#include "rtdef.h"

struct gpio_chip;
struct gpio_device;

struct gpio_device
{
    struct gpio_chip *chip;
    rt_list_t        list;
    void             *data;
};

struct gpio_chip
{
    struct gpio_device  *gpiodev;
    int                 base;
    char                ngpio;
    int                 available;
    int                 (*request)(struct gpio_chip *chip,
                        unsigned offset);
    void                (*free)(struct gpio_chip *chip,
                        unsigned offset);
    int                 (*get_direction)(struct gpio_chip *chip,
                        unsigned offset);
    int                 (*direction_input)(struct gpio_chip *chip,
                        unsigned offset);
    int                 (*direction_output)(struct gpio_chip *chip,
                        unsigned offset, int value);
    int                 (*get)(struct gpio_chip *chip,
                        unsigned offset);
    void                (*set)(struct gpio_chip *chip,
                        unsigned offset, int value);
    void                (*set_direction)(struct gpio_chip *chip,
                        unsigned offset, unsigned direction);
    int                 (*set_debounce)(struct gpio_chip *chip,
                        unsigned offset, unsigned debounce);
    int                 (*set_type)(struct gpio_chip *chip,
                        unsigned offset, unsigned type);
    int                 (*to_irq)(struct gpio_chip *chip,
                        unsigned offset);
};


struct gpio_chip *gpio_to_chip(rt_uint32_t gpio);
int gpiochip_add(struct gpio_chip *chip);

#endif
