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

#include <rtdef.h>
#include <rthw.h>
#include <gpio.h>
#include <gpiolib.h>
#include <irqdesc.h>

rt_list_t gpio_devices = RT_LIST_OBJECT_INIT(gpio_devices);

struct gpio_chip *gpio_to_chip(rt_uint32_t gpio)
{
    struct gpio_device *gd = RT_NULL;
    struct gpio_chip   *gc = RT_NULL;

    rt_list_for_each_entry(gd, &gpio_devices, list)
    {
        gc = gd->chip;
        if ((gc->base <= gpio) && ((gc->base + gc->ngpio) > gpio))
            return gc;
    }

    return RT_NULL;
}

static int gpio_chip_hwgpio(struct gpio_chip *chip, int gpio)
{
    return (gpio - chip->base);
}

int gpiochip_add(struct gpio_chip *chip)
{
    struct gpio_device *gdev;

    gdev = rt_malloc(sizeof(*gdev));
    if (!gdev)
        return -RT_ENOMEM;

    rt_memset(gdev, 0, sizeof(*gdev));
    gdev->chip    = chip;
    chip->gpiodev = gdev;
    rt_list_insert_after(&gpio_devices, &(gdev->list));

    return 0;
}

int gpio_request(unsigned int gpio)
{
    struct gpio_chip *gc = RT_NULL;

    gc = gpio_to_chip(gpio);
    if (!gc)
    {
        rt_kprintf("GPIO%d is invaild\n", gpio);
        return -RT_EBUSY;
    }

    if (gc->available & (0x1 << gpio_chip_hwgpio(gc, gpio)))
    {
        rt_kprintf("GPIO%d is already in use\n", gpio);
        return -RT_EBUSY;
    }

    gc->available |= (0x1 << gpio_chip_hwgpio(gc, gpio));

    return 0;
}

int gpio_release(unsigned int gpio)
{
    struct gpio_chip *gc = RT_NULL;

    gc = gpio_to_chip(gpio);

    if (!gc)
    {
        rt_kprintf("GPIO%d is invalid\n", gpio);
        return -RT_EBUSY;
    }

    gc->available &= ~(0x1 << gpio_chip_hwgpio(gc, gpio));
    return 0;
}

int gpio_direction_output(unsigned int gpio, unsigned int val)
{

    struct gpio_chip *gc = RT_NULL;

    gc = gpio_to_chip(gpio);

    if (gc)
        return gc->direction_output(gc, gpio_chip_hwgpio(gc, gpio), val);

    return -RT_EIO;

}

int gpio_direction_input(unsigned int gpio)
{
    struct gpio_chip *gc = RT_NULL;

    gc = gpio_to_chip(gpio);

    if (gc)
        return gc->direction_input(gc, gpio_chip_hwgpio(gc, gpio));

    return -RT_EIO;
}

int gpio_enable_debounce(unsigned int gpio, unsigned int debounce)
{
    struct gpio_chip *gc = RT_NULL;

    gc = gpio_to_chip(gpio);

    if (gc)
        return gc->set_debounce(gc, gpio_chip_hwgpio(gc, gpio), debounce);

    return -RT_EIO;
}

int gpio_disable_debounce(unsigned int gpio)
{
    struct gpio_chip *gc = RT_NULL;

    gc = gpio_to_chip(gpio);

    if (gc)
        return gc->set_debounce(gc, gpio_chip_hwgpio(gc, gpio), 0);

    return -RT_EIO;
}

int gpio_get_value(unsigned int gpio)
{
    struct gpio_chip *gc = RT_NULL;

    gc = gpio_to_chip(gpio);

    if (gc)
        return gc->get(gc, gpio_chip_hwgpio(gc, gpio));

    return -RT_EIO;
}

void gpio_set_value(unsigned int gpio, int val)
{
    struct gpio_chip *gc = RT_NULL;

    gc = gpio_to_chip(gpio);

    if (gc)
        return gc->set(gc, gpio_chip_hwgpio(gc, gpio), val);

    return;
}

int gpio_get_direction(unsigned int gpio)
{
    struct gpio_chip *gc = RT_NULL;

    gc = gpio_to_chip(gpio);

    if (gc)
        return gc->get_direction(gc, gpio_chip_hwgpio(gc, gpio));

    return -RT_EIO;
}

void gpio_set_direction(unsigned int gpio, unsigned int direction)
{
    struct gpio_chip *gc = RT_NULL;

    gc = gpio_to_chip(gpio);

    if (gc)
        return gc->set_direction(gc, gpio_chip_hwgpio(gc, gpio), direction);
}

int gpio_to_irq(unsigned int gpio)
{
    struct gpio_chip *gc = RT_NULL;

    gc = gpio_to_chip(gpio);

    if (gc)
        return gc->to_irq(gc, gpio_chip_hwgpio(gc, gpio));

    return -RT_EIO;
}

void gpio_irq_enable(unsigned int irq)
{
    rt_hw_interrupt_umask(irq);
}

void gpio_irq_disable(unsigned int irq)
{
    rt_hw_interrupt_mask(irq);
}

int gpio_set_irq_type(unsigned int gpio, unsigned int type)
{
    struct gpio_chip *gc = RT_NULL;

    gc = gpio_to_chip(gpio);

    if (gc)
        return gc->set_type(gc, gpio_chip_hwgpio(gc, gpio), type);

    return -RT_EIO;
}
