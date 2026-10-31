#ifndef __IOPAD_H__
#define __IOPAD_H__

#ifdef CONFIG_BOARD_FAST
#include "fast_board/fastboard_iopad.h"
#endif
#ifdef CONFIG_BOARD_APP
#include "app_board/appboard_iopad.h"
#endif
#ifdef CONFIG_BOARD_TEST
#include "test_board/testboard_iopad.h"
#endif

#include "fh8662v100_iopad.h"

#endif
