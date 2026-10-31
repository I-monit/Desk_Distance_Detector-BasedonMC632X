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
#ifndef __SYS_STATFS_H_
#define __SYS_STATFS_H_

#ifdef __cplusplus
extern "C" {
#endif

struct statfs
{
    unsigned long f_bsize;      /* block size */
    unsigned long f_blocks;  /* total data blocks in file system */
    unsigned long f_bfree;     /* free blocks in file system */
};

int statfs(const char *path, struct statfs *buf);


#ifdef __cplusplus
}
#endif

#endif
