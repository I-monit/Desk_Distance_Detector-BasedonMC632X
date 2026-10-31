/*
 *  * Copyright (c) 2015-2019 Shanghai Fullhan Microelectronics Co., Ltd.
 *  *
 *  * SPDX-License-Identifier: Apache-2.0
 *  *
 *  * Change Logs:
 *  * Date           Author       Notes
 *  * 2019-01-18      {fullhan}   the first version
 *  *
 */
#ifndef __LINUX_FS_H_
#define __LINUX_FS_H_

#ifdef __cplusplus
extern "C" {
#endif

#define BLKRRPART  (95)    /* re-read partition table */
#define BLKGETSIZE (96)    /* return device size /512 (long *arg) */
#define BLKGETSIZE64 (114)
#define BLKSSZGET    (104)
#define BLKPARTCLEANUP (111)
#define BLKPARTGET  (112)

#define BLOCK_SIZE_BITS 10
#define BLOCK_SIZE (1<<BLOCK_SIZE_BITS)

#ifdef __cplusplus
}
#endif

#endif
