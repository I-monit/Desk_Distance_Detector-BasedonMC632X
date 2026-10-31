/**
 * Copyright (c) 2015-2019 Shanghai Fullhan Microelectronics Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 *
 * Change Logs:
 * Date           Author       Notes
 * 2019-01-24     songyh    the first version
 *
 */

#ifndef __GPIO_H__
#define __GPIO_H__

/**
 * GPIO interrupt trigger type macro,
 * each represent an interrupt trigger mode
 *
 * @see gpio_set_irq_type();
 */
enum
{
    IRQ_TYPE_NONE         = 0x00000000, /**< none*/
    IRQ_TYPE_EDGE_RISING  = 0x00000001, /**< rising edge*/
    IRQ_TYPE_EDGE_FALLING = 0x00000002, /**< falling edge*/
    IRQ_TYPE_EDGE_BOTH    = (IRQ_TYPE_EDGE_FALLING | IRQ_TYPE_EDGE_RISING),
    IRQ_TYPE_LEVEL_HIGH   = 0x00000004, /**< high level*/
    IRQ_TYPE_LEVEL_LOW    = 0x00000008, /**< low level*/
    IRQ_TYPE_LEVEL_MASK   = (IRQ_TYPE_LEVEL_LOW | IRQ_TYPE_LEVEL_HIGH),
    IRQ_TYPE_TRIGGER_MASK = (IRQ_TYPE_LEVEL_HIGH | IRQ_TYPE_LEVEL_LOW |
                             IRQ_TYPE_EDGE_RISING | IRQ_TYPE_EDGE_FALLING),
};

/**
 * GPIO direction macro,
 * each represent a direction
 *
 * @see gpio_get_direction();
 * @see gpio_set_direction();
 */
#define GPIO_DIR_OUTPUT 1 /**< output*/
#define GPIO_DIR_INPUT 0  /**< input*/

/**
 * convert GPIO number to IRQ number
 * @param gpio GPIO number to be converted
 * @return IRQ number
 */
int gpio_to_irq(unsigned int gpio);

/**
 * disable GPIO's debounce mode
 * controls whether an external signal that is the source
 * of an interrupt needs to be debounced to remove any
 * spurious glitches.
 * @param gpio GPIO number
 */
int gpio_disable_debounce(unsigned int gpio);

/**
 * enable GPIO's debounce mode
 * controls whether an external signal that is the source
 * of an interrupt needs to be debounced to remove any
 * spurious glitches.
 * @param gpio GPIO number
 */
int gpio_enable_debounce(unsigned int gpio, unsigned int debounce);

/**
 * allows each GPIO to be configured for interrupts
 * it configures the corresponding GPIO to become an interrupt
 * @param gpio GPIO number
 */
void gpio_irq_enable(unsigned int irq);

/**
 * GPIO operates as a normal GPIO signal
 * interrupts are disabled
 * @param gpio GPIO number
 */
void gpio_irq_disable(unsigned int irq);

/**
 * it configures the interrupt type to be
 * falling-edge or active-low sensitive
 * rising-edge or active-high sensitive.
 * @param gpio GPIO number
 * @param type interrupt type
 * @return 0 if OK
 */
int gpio_set_irq_type(unsigned int gpio, unsigned int type);

/**
 * get corresponding GPIO's direction
 * @param gpio GPIO number
 * @return 0 - input
 *         1 - output
 */
int gpio_get_direction(unsigned int gpio);

/**
 * set corresponding GPIO's direction
 * @param gpio GPIO number
 * @return 0 - input
 *         1 - output
 */
void gpio_set_direction(unsigned int gpio, unsigned int direction);

/**
 * get corresponding GPIO's value
 * @param gpio GPIO number
 * @return GPIO value
 */
int gpio_get_value(unsigned int gpio);

/**
 * set corresponding GPIO's value
 * @param gpio GPIO number
 * @param val GPIO value
 */
void gpio_set_value(unsigned int gpio, int val);

/**
 * set corresponding GPIO's direction to input
 * @param gpio GPIO number
 * @return 0 if OK
 */
int gpio_direction_input(unsigned int gpio);

/**
 * set corresponding GPIO's value and set direction to output
 * @param gpio GPIO number
 * @param val GPIO value
 * @return 0 if OK
 */
int gpio_direction_output(unsigned int gpio, unsigned int val);

/**
 * request a GPIO
 * @param gpio GPIO number
 * @return 0 if OK
 */
int gpio_request(unsigned int gpio);

/**
 * release a GPIO
 * @param gpio GPIO number
 * @return 0 if OK
 */
int gpio_release(unsigned int gpio);

/**
 * initialize GPIO driver
 */
void rt_hw_gpio_init(void);

void set_gpio_pm_exclude(unsigned int excludenum);

#endif /* GPIO_H_ */
