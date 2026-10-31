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
#ifndef __SYS_MOUNT_H_
#define __SYS_MOUNT_H_

#ifdef __cplusplus
extern "C" {
#endif

int mount(const char *source, const char *target,
             const char *filesystemtype, unsigned long mountflags,
             const void *data);

int umount(const char *target);

#ifdef __cplusplus
}
#endif

#endif
