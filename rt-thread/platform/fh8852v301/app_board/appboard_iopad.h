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
    "ETH", "I2C0", "MIPI", "PWM2", "PWM3", "PWM4", "PWM5",
    "SADC_IN0" "SENSOR0_CLK",
    "SPI0_4BIT", "STM1", "UART0", "UART1",
    "GPIO27", "GPIO30", "GPIO31",
    "GPIO32", "GPIO43", "GPIO44", "GPIO47", "GPIO48",
    "GPIO4",
#ifdef WIFI_USING_SDIOWIFI
#if (WIFI_SDIO == 0)
"SD0_WIFI", "GPIO17"
#else
"SD0_NO_WP"
#endif
#else
"SD0_NO_WP"
#endif
};

#endif
