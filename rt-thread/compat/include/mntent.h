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
#ifndef __MNTENT_H
#define __MNTENT_H

#ifdef __cplusplus
extern "C" {
#endif

#define MOUNTED ""

#include <stdio.h>

struct mntent
{
    char mnt_fsname[32];
};

FILE *setmntent(const char *filename, const char *type);

struct mntent *getmntent(FILE *fp);

int addmntent(FILE *fp, const struct mntent *mnt);

int endmntent(FILE *fp);

#ifdef __cplusplus
}
#endif

#endif
