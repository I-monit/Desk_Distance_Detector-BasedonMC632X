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

#include "fh_def.h"
#include "gpio.h"
#include "mol_gpio.h"
#include "board_info.h"
#include "platform_def.h"
#include "irqdesc.h"
#ifdef RT_USING_PM
#include <pm.h>
#endif

rt_list_t gpio_chips = RT_LIST_OBJECT_INIT(gpio_chips);

int gpio_irq_type[NUM_OF_GPIO];


#if defined(RT_DEBUG) && defined(MOL_GPIO_DEBUG)
#define PRINT_GPIO_DBG(fmt, args...)   \
    do                                 \
    {                                  \
        rt_kprintf("GPIO_DEBUG: "); \
        rt_kprintf(fmt, ##args);       \
    } while (0)
#else
#define PRINT_GPIO_DBG(fmt, args...) \
    do                               \
    {                                \
    } while (0)
#endif

extern int __rt_ffs(int value);

#ifdef RT_USING_PM
int gpio_suspend(const struct rt_device *device, rt_uint8_t mode)
{
    struct mol_gpio_chip *gc = RT_NULL;

    rt_list_for_each_entry(gc, &gpio_chips, list)
        rt_hw_interrupt_mask(gc->irq);


    return RT_EOK;
}

void gpio_resume(const struct rt_device *device, rt_uint8_t mode)
{
    struct mol_gpio_chip *gc = RT_NULL;

    rt_list_for_each_entry(gc, &gpio_chips, list)
            rt_hw_interrupt_umask(gc->irq);
}

struct rt_device_pm_ops gpio_pm_ops = {
    .suspend_late = gpio_suspend,
    .resume_late = gpio_resume
};
#endif

void set_gpio_pm_exclude(unsigned int excludenum)
{
    struct gpio_chip *gc;
    struct mol_gpio_chip *chip;
    int off;

    gc   = gpio_to_chip(excludenum);

    if (!gc)
    {
        rt_kprintf("%s: ERROR, gpio num error!\n", __func__);
        return;
    }
    chip = rt_container_of(gc, struct mol_gpio_chip, chip);

    off = excludenum - gc->base;
    chip->pmchangedirmask  = chip->pmchangedirmask & (~(1<<off));

}

static unsigned int gpio_to_base(unsigned int gpio)
{
    struct gpio_chip *gc;
    struct mol_gpio_chip *chip;

    gc   = gpio_to_chip(gpio);
    chip = rt_container_of(gc, struct mol_gpio_chip, chip);

    return chip->base;
}

static int _gpio_to_irq(unsigned int gpio)
{
    return (MAX_HANDLERS - 1 - gpio);
}

static int chip_get_direction(struct gpio_chip *gc, unsigned int offset)
{
    struct mol_gpio_chip *chip;
    unsigned int reg;

    chip = rt_container_of(gc, struct mol_gpio_chip, chip);
    reg  = GET_REG(chip->base + REG_GPIO_DIR);
    reg  &= BIT(offset);
    reg  = reg >> offset;

    return reg;
}

static int chip_gpio_get(struct gpio_chip *gc, unsigned int offset)
{
    struct mol_gpio_chip *chip;
    unsigned int reg;

    chip = rt_container_of(gc, struct mol_gpio_chip, chip);
    reg = GET_REG(chip->base + REG_GPIO_DATA);
    reg &= BIT(offset);
    reg = reg >> offset;

    return reg;
}

static void chip_gpio_set(struct gpio_chip *gc, unsigned int offset, int val)
{
    struct mol_gpio_chip *chip;
    unsigned int reg;

    chip = rt_container_of(gc, struct mol_gpio_chip, chip);
    reg = GET_REG(chip->base + REG_GPIO_DMSK);
    reg |= BIT(offset);
    SET_REG(chip->base + REG_GPIO_DMSK, reg);
    reg  = GET_REG(chip->base + REG_GPIO_DATA);
    reg = val ? (reg | BIT(offset)) : (reg & ~BIT(offset));

    SET_REG(chip->base + REG_GPIO_DATA, reg);
}

static int chip_direction_input(struct gpio_chip *gc, unsigned int offset)
{
    struct mol_gpio_chip *chip;
    unsigned int reg;

    chip = rt_container_of(gc, struct mol_gpio_chip, chip);
    reg = GET_REG(chip->base + REG_GPIO_DIR);
    reg &= ~(1 << offset);
    SET_REG(chip->base + REG_GPIO_DIR, reg);

    reg = GET_REG(chip->base + REG_GPIO_DMSK);
    reg |= BIT(offset);
    SET_REG(chip->base + REG_GPIO_DMSK, reg);

    reg = GET_REG(chip->base + REG_GPIO_INEN);
    reg |= BIT(offset);
    SET_REG(chip->base + REG_GPIO_INEN, reg);

    return 0;
}

static int chip_direction_output(struct gpio_chip *gc, unsigned int offset, int value)
{
    struct mol_gpio_chip *chip;
    unsigned int reg;

    chip = rt_container_of(gc, struct mol_gpio_chip, chip);
    reg = GET_REG(chip->base + REG_GPIO_DIR);
    reg |= (1 << offset);
    SET_REG(chip->base + REG_GPIO_DIR, reg);

    reg = GET_REG(chip->base + REG_GPIO_DMSK);
    reg |= BIT(offset);
    SET_REG(chip->base + REG_GPIO_DMSK, reg);

    reg = GET_REG(chip->base + REG_GPIO_DATA);
    reg = value ? (reg | (1 << offset)) : (reg & ~(1 << offset));
    SET_REG(chip->base + REG_GPIO_DATA, reg);

    return 0;
}

static void chip_set_direction(struct gpio_chip *gc, unsigned int offset, unsigned direction)
{
    struct mol_gpio_chip *chip;
    unsigned int reg;

    chip = rt_container_of(gc, struct mol_gpio_chip, chip);
    reg = GET_REG(chip->base + REG_GPIO_DIR);

    reg = direction ? (reg | (1 << offset)) : (reg & ~(1 << offset));
    SET_REG(chip->base + REG_GPIO_DIR, reg);
}

static int chip_set_debounce(struct gpio_chip *chip, unsigned int offset, unsigned debounce)
{
    return 0;
}

static int chip_to_irq(struct gpio_chip *gc, unsigned int offset)
{
    return (MAX_HANDLERS - gc->base - offset - 1);
}

static int chip_set_irq_type(struct gpio_chip *gc, unsigned int offset, unsigned type)
{
    struct mol_gpio_chip *chip;

    chip = rt_container_of(gc, struct mol_gpio_chip, chip);
    int int_type     = GET_REG(chip->base + REG_GPIO_IS);
    int int_polarity = GET_REG(chip->base + REG_GPIO_IEV);
    int both_edge    = GET_REG(chip->base + REG_GPIO_IBE);
    both_edge        &= ~BIT(offset);

    switch (type & IRQ_TYPE_TRIGGER_MASK)
    {
    case IRQ_TYPE_EDGE_BOTH:
        both_edge |= BIT(offset);
        int_type &= ~BIT(offset);
        break;
    case IRQ_TYPE_EDGE_RISING:
        int_type &= ~BIT(offset);
        int_polarity |= BIT(offset);
        break;
    case IRQ_TYPE_EDGE_FALLING:
        int_type &= ~BIT(offset);
        int_polarity &= ~BIT(offset);
        break;
    case IRQ_TYPE_LEVEL_HIGH:
        int_type |= BIT(offset);
        int_polarity |= BIT(offset);
        break;
    case IRQ_TYPE_LEVEL_LOW:
        int_type |= BIT(offset);
        int_polarity &= ~BIT(offset);
        break;
    case IRQ_TYPE_NONE:
        return 0;
    default:
        return -RT_ERROR;
    }

    SET_REG(chip->base + REG_GPIO_IS, int_type);
        SET_REG(chip->base + REG_GPIO_IEV, int_polarity);
        SET_REG(chip->base + REG_GPIO_IBE, both_edge);
    
    gpio_irq_type[gc->base + offset] = type;
    return 0;
}

void chip_irq_enable(unsigned int irq)
{
    int reg;
    int gpio = MAX_HANDLERS - irq;
    int base = gpio_to_base(gpio);

    reg = GET_REG(base + REG_GPIO_RIS);
    SET_REG(base + REG_GPIO_IC, reg);

    reg = GET_REG(base + REG_GPIO_IE);
    reg |= BIT((MAX_HANDLERS - irq - 1) % 8);
    SET_REG(base + REG_GPIO_IE, reg);
}

void chip_irq_disable(unsigned int irq)
{
    int reg;
    int gpio = MAX_HANDLERS - irq;
    int base = gpio_to_base(gpio);

    reg = GET_REG(base + REG_GPIO_IE);
    reg &= ~BIT((MAX_HANDLERS - irq - 1) % 8);
    SET_REG(base + REG_GPIO_IE, reg);
}

static void mol_gpio_interrupt(int irq, void *param)
{
    unsigned int irq_status, offset, reg;
    struct mol_gpio_chip *chip;
    struct gpio_chip *gc = (struct gpio_chip *)param;

    chip = rt_container_of(gc, struct mol_gpio_chip, chip);

    irq_status = GET_REG(chip->base + REG_GPIO_MIS);
    offset = __rt_ffs(irq_status) - 1;/*look for who trigger the irq*/

    reg = GET_REG(chip->base + REG_GPIO_IE);
    reg &= ~BIT(offset);
    SET_REG(chip->base + REG_GPIO_IE, reg);/*disable the irq*/

    generic_handle_irq(chip_to_irq(gc, offset));
    SET_REG(chip->base + REG_GPIO_IC, (0x1 << offset));/*handle irq and clear irq flag*/

    reg |= BIT(offset);
    SET_REG(chip->base + REG_GPIO_IE, reg);/*enable the irq*/
}


static struct irq_chip gpio_int_chip = {
    .name       = "gpio_intc",
    .irq_unmask   = chip_irq_enable,
    .irq_mask = chip_irq_disable,
};

int mol_gpio_probe(void *priv_data)
{
    int err;
    int idx;
    struct mol_gpio_chip *gc = (struct mol_gpio_chip *)priv_data;
    char irqname[8] = {0};

    gc->chip.get_direction    = chip_get_direction;
    gc->chip.direction_input  = chip_direction_input;
    gc->chip.direction_output = chip_direction_output;
    gc->chip.get              = chip_gpio_get;
    gc->chip.set              = chip_gpio_set;
    gc->chip.set_direction    = chip_set_direction;
    gc->chip.set_debounce     = chip_set_debounce;
    gc->chip.set_type         = chip_set_irq_type;
    gc->chip.to_irq           = chip_to_irq;
    gc->pmchangedirmask       = 0xffffffff;
    gc->pmsaveddirval         = 0;

    err = gpiochip_add(&gc->chip);
    if (err)
    {
        rt_kprintf("GPIO controller load fail\n");
        return err;
    }
    rt_list_insert_after(&gpio_chips, &(gc->list));
    rt_sprintf(irqname, "gpio_%d", gc->id);
    rt_hw_interrupt_install(gc->irq, mol_gpio_interrupt, gc, irqname);
    for (idx = 0; idx < gc->chip.ngpio; idx++)
        irq_set_chip(_gpio_to_irq(gc->chip.base + idx), &gpio_int_chip);

    rt_hw_interrupt_umask(gc->irq);

#ifdef RT_USING_PM
    rt_pm_device_register((struct rt_device *)gc, &gpio_pm_ops);
#endif

    return 0;
}

int mol_gpio_exit(void *priv_data) { return 0; }
struct fh_board_ops gpio_driver_ops = {
    .probe = mol_gpio_probe, .exit = mol_gpio_exit,
};

void rt_hw_gpio_init(void)
{
    PRINT_GPIO_DBG("%s start\n", __func__);
    fh_board_driver_register("gpio", &gpio_driver_ops);
    PRINT_GPIO_DBG("%s end\n", __func__);
}
