/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2010-10-26     Bernard      the first version
 */

#ifndef __POSIX_SEMAPHORE_H__
#define __POSIX_SEMAPHORE_H__

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define  SEM_VALUE_MAX      (0xffff)

struct posix_sem
{
    /* reference count and unlinked */
    unsigned short refcount;
    unsigned char unlinked;
    unsigned char unamed;

    void *sem;

    /* next posix semaphore */
    struct posix_sem *next;
};
typedef struct posix_sem sem_t;

int sem_close(sem_t *sem);
int sem_destroy(sem_t *sem);
int sem_getvalue(sem_t *sem, int *sval);
int sem_init(sem_t *sem, int pshared, unsigned int value);
sem_t *sem_open(const char *name, int oflag, ...);
int sem_post(sem_t *sem);
int sem_timedwait(sem_t *sem, const struct timespec *abs_timeout);
int sem_trywait(sem_t *sem);
int sem_unlink(const char *name);
int sem_wait(sem_t *sem);

#ifdef __cplusplus
}
#endif

#endif
