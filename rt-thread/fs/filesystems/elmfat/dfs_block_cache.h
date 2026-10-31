/**
 * Copyright (c) 2015-2019 Shanghai Fullhan Microelectronics Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-05-12     zhangy       the first version
 */

#ifndef __DFS_BLOCK_CACHE_H__
#define __DFS_BLOCK_CACHE_H__
#include "rtdevice.h"
#define EACH_CACHE_NODE_SEC_SIZE    128
#define DFS_SEC_CACHE_TYPE_128_NO    (0x40)
#define DFS_SEC_CACHE_TYPE_64_NO    (0)
#define DFS_SEC_CACHE_TYPE_32_NO    (0)
#define DFS_SEC_CACHE_TYPE_16_NO    (0)
#define DFS_SEC_CACHE_TYPE_8_NO    (0)
#define DFS_SEC_CACHE_TYPE_4_NO    (0)
#define DFS_SEC_CACHE_TYPE_2_NO    (0)
#define DFS_SEC_CACHE_TYPE_1_NO    (0)

#define GUARD_THREAD_PREORITY    0x48
#define GUARD_THREAD_STACK_SIZE    1024

int api_dfs_block_cache_init(void);
void *api_dfs_get_cache_handle(void);
void api_dfs_focus_on_cache_area(void *p, rt_uint32_t st_lg_no, rt_uint32_t end_lg_no);
int api_dfs_bind_hw_io_handle(void *dfs_handle, void *io_handle);
int api_blk_cache_write(void *p, rt_uint32_t sec_lg_no, const void *usr_buf, rt_uint32_t sec_size);
int api_blk_cache_read(void *p, rt_uint32_t sec_lg_no, void *usr_buf, rt_uint32_t sec_size);
void api_flush_all_blk_cache(void *dfs_handle);
#endif
