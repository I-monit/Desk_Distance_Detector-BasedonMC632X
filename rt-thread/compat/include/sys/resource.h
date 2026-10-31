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
#ifndef __ADAPTER_RESOURCE_H__
#define __ADAPTER_RESOURCE_H__

#ifdef __cplusplus
extern "C" {
#endif

#define RLIMIT_NOFILE 7

struct rlimit
{
    unsigned long rlim_cur;  /* Soft limit */
    unsigned long rlim_max;  /* Hard limit (ceiling for rlim_cur) */
};


int getrlimit(int resource, struct rlimit *rlim);
int setrlimit(int resource, const struct rlimit *rlim);

#ifdef __cplusplus
}
#endif

#endif
