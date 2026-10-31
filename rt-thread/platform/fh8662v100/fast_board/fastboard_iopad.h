/**
 * Copyright (c) 2015-2019 Shanghai Fullhan Microelectronics Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 *
 * Change Logs:
 * Date           Author       Notes
 * 2019-01-08     wangyl307    the first version, for fh8856&zy2
 * 2019-01-14     wangyl307    add pads fix funcs and defs for zy
 *
 */

#ifndef __APP_IOPAD_H__
#define __APP_IOPAD_H__

char *fh_pinctrl_selected_devices[] = {
"ETH", "I2C0", "I2C1", "I2C2", "MIPI", "PWM0", "PWM1",
    "SADC_IN0", "SADC_IN1", "SD0_NO_WP", "SENSOR0_CLK",
    "SENSOR0_VS", "SENSOR1_CLK", "SENSOR1_VS", "SPI0_4BIT",
    "SPI1", "STM1", "UART0", "UART1", "UART2", "GPIO4",
    "GPIO31", "GPIO32", "GPIO47", "GPIO48", "GPIO49",
};

#endif
