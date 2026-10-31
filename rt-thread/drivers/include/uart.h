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

#ifndef __UART_H__
#define __UART_H__

#define FH_UART_FIFO_RESET (0x01)
#define FH_DMA_RESET       (0x02)

void rt_hw_uart_init(void);

#endif
