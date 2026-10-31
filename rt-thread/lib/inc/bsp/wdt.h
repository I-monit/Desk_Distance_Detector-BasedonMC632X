/**
 * Copyright (c) 2015-2019 Shanghai Fullhan Microelectronics Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 *
 * Change Logs:
 * Date           Author       Notes
 * 2019-04-12     songyh         add license Apache-2.0
 */

#ifndef __WDT_H__
#define __WDT_H__

#define RT_DEVICE_CTRL_WDT_GET_TIMEOUT    (1) /* get timeout(in seconds) */
#define RT_DEVICE_CTRL_WDT_SET_TIMEOUT    (2) /* set timeout(in seconds) */
#define RT_DEVICE_CTRL_WDT_GET_TIMELEFT   (3) /* get the left time before reboot(in seconds) */
#define RT_DEVICE_CTRL_WDT_KEEPALIVE      (4) /* refresh watchdog */
#define RT_DEVICE_CTRL_WDT_START          (5) /* start watchdog */
#define RT_DEVICE_CTRL_WDT_STOP           (6) /* stop watchdog */

void rt_hw_wdt_init(void);

#endif /* WDT_H_ */
