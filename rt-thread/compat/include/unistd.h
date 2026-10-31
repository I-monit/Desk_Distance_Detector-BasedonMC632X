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
#ifndef __UNISTD_H_
#define __UNISTD_H_

#include_next <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

void sync(void);

int gethostname(char *name, size_t len);
pid_t gettid(void);

extern _off64_t lseek64(int fd, _off64_t pos, int whence);
#ifdef __cplusplus
}
#endif

#endif
