/**
 * Copyright (c) 2015-2019 Shanghai Fullhan Microelectronics Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 *
 * Change Logs:
 * Date           Author       Notes
 * 2019-01-08     wangyl307    add micro OS_LIST_DEL
 *
 */


#ifndef __PINCTRL_OSDEP_H__
#define __PINCTRL_OSDEP_H__
#include "rtconfig.h"
#include "rtdebug.h"
#include "rtdef.h"
#include "fh_def.h"
#include <rtthread.h>
#include <string.h>

#define OS_LIST_INIT RT_LIST_OBJECT_INIT
#define OS_LIST rt_list_t
#define OS_PRINT rt_kprintf
#define OS_LIST_EMPTY rt_list_init
#define OS_LIST_DEL rt_list_remove
#define OS_NULL RT_NULL

#define PINCTRL_ADD_DEVICE(name)                    \
        rt_list_insert_after(list,   \
            &pinctrl_dev_##name.list)



#endif /* PINCTRL_OSDEP_H_ */
