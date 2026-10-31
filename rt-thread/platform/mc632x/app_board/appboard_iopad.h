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

#ifndef __APP_IOPAD_H__
#define __APP_IOPAD_H__

char *fh_pinctrl_selected_devices[] = {
    "CSI", "EPHY", "I2C0",
    "SENSOR_CLK0", "SENSOR_CLK1",
    "GPIO4_4", "GPIO5_0", "SFC", "SD1",
    "UART0", "UART1", "USB_PWREN", "SENSOR_VS0", "SENSOR_VS1",
    "GPIO1_7", "GPIO2_5", "GPIO2_6", "PKG",
    "GPIO6_4", "GPIO6_5", "GPIO6_6", "GPIO0_5", "GPIO1_2",
    "GPIO1_3",
#ifdef WIFI_USING_SDIOWIFI
#if (WIFI_SDIO == 0)
"SD0_WIFI", "GPIO3_5"
#else
"SD0_NO_WP"
#endif
#else
"SD0_NO_WP"
#endif
};

#endif