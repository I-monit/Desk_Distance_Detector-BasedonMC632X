/**
 * Copyright (c) 2015-2019 Shanghai Fullhan Microelectronics Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 *
 * Change Logs:
 * Date           Author       Notes
 * 2019-01-08     songyh    the first version
 *
 */

#ifndef __FH8852V301_BOARD_DEF_H__
#define __FH8852V301_BOARD_DEF_H__

#if defined(CONFIG_BOARD_FAST)
#include "fast_board/board_def.h"
#elif defined(CONFIG_BOARD_APP)
#include "app_board/board_def.h"
#elif defined(CONFIG_BOARD_TEST)
#include "test_board/board_def.h"
#else
#error "board not defined for chip FH8852V301"
#endif


#endif /* BOARD_H_ */
