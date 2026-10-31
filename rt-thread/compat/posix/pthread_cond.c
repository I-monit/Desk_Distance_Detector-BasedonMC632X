/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2010-10-26     Bernard      the first version
 */

#include <pthread.h>
#include <time.h>
#include "pthread_internal.h"

#define BUILD_SIZE_CHK(cond)  __attribute__((unused)) static char __dumy__[-!(cond)]

BUILD_SIZE_CHK(sizeof(struct rt_semaphore) == PTHREAD_SEM_SIZE);
BUILD_SIZE_CHK(sizeof(struct rt_mutex) == PTHREAD_MTX_SIZE);

int pthread_condattr_destroy(pthread_condattr_t *attr)
{
    if (!attr)
        return EINVAL;

    return 0;
}
RTM_EXPORT(pthread_condattr_destroy);

int pthread_condattr_init(pthread_condattr_t *attr)
{
    if (!attr)
        return EINVAL;
    *attr = PTHREAD_PROCESS_PRIVATE;

    return 0;
}
RTM_EXPORT(pthread_condattr_init);

int pthread_condattr_getclock(const pthread_condattr_t *attr,
                              clockid_t                *clock_id)
{
    if (clock_id == RT_NULL)
        return EINVAL;
    *clock_id = *(clockid_t *)attr;

    return 0;
}
RTM_EXPORT(pthread_condattr_getclock);

int pthread_condattr_setclock(pthread_condattr_t *attr,
                              clockid_t           clock_id)
{
    if (attr == RT_NULL)
    {
        return EINVAL;
    }
    if (clock_id != CLOCK_MONOTONIC)
    {
        return EINVAL;
    }
    *(int *)attr = clock_id;
    return 0;
}
RTM_EXPORT(pthread_condattr_setclock);

int pthread_condattr_getpshared(const pthread_condattr_t *attr, int *pshared)
{
    if (!attr || !pshared)
        return EINVAL;

    *pshared = PTHREAD_PROCESS_PRIVATE;

    return 0;
}
RTM_EXPORT(pthread_condattr_getpshared);

int pthread_condattr_setpshared(pthread_condattr_t*attr, int pshared)
{
    if ((pshared != PTHREAD_PROCESS_PRIVATE) &&
        (pshared != PTHREAD_PROCESS_SHARED))
    {
        return EINVAL;
    }

    if (pshared != PTHREAD_PROCESS_PRIVATE)
        return ENOSYS;

    return 0;
}
RTM_EXPORT(pthread_condattr_setpshared);

int pthread_cond_init(pthread_cond_t *cond, const pthread_condattr_t *attr)
{
    rt_err_t result;
    char cond_name[RT_NAME_MAX];
    static rt_uint16_t cond_num = 0;
    rt_sem_t csem;

    /* parameter check */
    if (cond == RT_NULL)
        return EINVAL;
    /*
     *  NOTICE:
     *     pthread cond only supports PTHREAD_PROCESS_PRIVATE/SHARED attributes originnally.
     *  Now, pthread cond attribute CLOCK_MONOTONIC needs to be supported.
     *  since the attribute PTHREAD_PROCESS_PRIVATE/SHARED DOES NOT really use the attr member
     *  it is reused as "clock" attribute.
     */
    if ((attr != RT_NULL) && (*attr != PTHREAD_PROCESS_PRIVATE) && (*attr != CLOCK_MONOTONIC))
        return EINVAL;

    rt_snprintf(cond_name, sizeof(cond_name), "cond%02d", cond_num++);

	if (attr == RT_NULL) /* use default value */
		cond->attr = PTHREAD_PROCESS_PRIVATE;
	else 
	    cond->attr = *attr;

    result = rt_sem_init((rt_sem_t)(cond->lck), cond_name, 0, RT_IPC_FLAG_FIFO);
    /* result = rt_sem_init(&cond->sem, cond_name, 0, RT_IPC_FLAG_FIFO);    */
    if (result != RT_EOK)
        return EINVAL;

    /* detach the object from system object container */
    csem = (rt_sem_t)(&cond->lck);
    rt_object_detach(&(csem->parent.parent));
    /* rt_object_detach(&(cond->sem.parent.parent)); */
    csem->parent.parent.type = RT_Object_Class_Semaphore;
    /* cond->sem.parent.parent.type = RT_Object_Class_Semaphore;    */

    return 0;
}
RTM_EXPORT(pthread_cond_init);

int pthread_cond_destroy(pthread_cond_t *cond)
{
    rt_err_t result;
    rt_sem_t csem;

    if (cond == RT_NULL)
        return EINVAL;
    if (cond->attr == -1)
        return 0; /* which is not initialized */

    csem = (rt_sem_t)(&cond->lck);
    result = rt_sem_trytake(csem);
    /* result = rt_sem_trytake(&(cond->sem));   */
    if (result != RT_EOK)
        return EBUSY;

    /* clean condition */
    rt_memset(cond, 0, sizeof(pthread_cond_t));
    cond->attr = -1;

    return 0;
}
RTM_EXPORT(pthread_cond_destroy);

int pthread_cond_broadcast(pthread_cond_t *cond)
{
    rt_err_t result;
    rt_sem_t csem;

    if (cond == RT_NULL)
        return EINVAL;
    if (cond->attr == -1)
        pthread_cond_init(cond, RT_NULL);

    rt_enter_critical();
    while (1)
    {
        /* try to take condition semaphore */
        csem = (rt_sem_t)(&cond->lck);
        result = rt_sem_trytake(csem);
        /* result = rt_sem_trytake(&(cond->sem));   */
        if (result == -RT_ETIMEOUT)
        {
            /* it's timeout, release this semaphore */
            rt_sem_release(csem);
        }
        else if (result == RT_EOK)
        {
            /* has taken this semaphore, release it */
            rt_sem_release(csem);
            break;
        }
        else
        {
            rt_exit_critical();

            return EINVAL;
        }
    }
    rt_exit_critical();

    return 0;
}
RTM_EXPORT(pthread_cond_broadcast);

int pthread_cond_signal(pthread_cond_t *cond)
{
    rt_err_t result;
    rt_sem_t csem;

    if (cond == RT_NULL)
        return EINVAL;
    if (cond->attr == -1)
        pthread_cond_init(cond, RT_NULL);

    csem = (rt_sem_t)(&cond->lck);
    result = rt_sem_release(csem);
    /* result = rt_sem_release(&(cond->sem));   */
    if (result == RT_EOK)
        return 0;

    return 0;
}
RTM_EXPORT(pthread_cond_signal);

rt_err_t _pthread_cond_timedwait(pthread_cond_t  *cond,
                                 pthread_mutex_t *mutex,
                                 rt_int32_t       timeout)
{
    rt_err_t result;
    rt_sem_t csem;
    rt_mutex_t mtx;

    if (!cond || !mutex)
        return -RT_ERROR;
    /* check whether initialized */
    if (cond->attr == -1)
        pthread_cond_init(cond, RT_NULL);

    /* The mutex was not owned by the current thread at the time of the call. */
    mtx = (rt_mutex_t)&mutex->mtx;
    if (mtx->owner != rt_thread_self())
        return -RT_ERROR;
    /* unlock a mutex failed */
    if (pthread_mutex_unlock(mutex) != 0)
        return -RT_ERROR;

    csem = (rt_sem_t)(&cond->lck);
    result = rt_sem_take(csem, timeout);
    /* lock mutex again */
    pthread_mutex_lock(mutex);

    return result;
}
RTM_EXPORT(_pthread_cond_timedwait);

int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex)
{
    rt_err_t result;

    result = _pthread_cond_timedwait(cond, mutex, RT_WAITING_FOREVER);
    if (result == RT_EOK)
        return 0;

    return EINVAL;
}
RTM_EXPORT(pthread_cond_wait);

int pthread_cond_timedwait(pthread_cond_t        *cond,
                           pthread_mutex_t       *mutex,
                           const struct timespec *abstime)
{
    int timeout;
    rt_err_t result;
    pthread_condattr_t attr;
    struct timespec tnow;

    attr = cond->attr;
    if (attr == CLOCK_MONOTONIC)
    {
        clock_gettime(CLOCK_MONOTONIC, &tnow);
        /* check if the past time */
        if ((tnow.tv_sec > abstime->tv_sec) || ((tnow.tv_sec == abstime->tv_sec) && (tnow.tv_nsec >= abstime->tv_nsec)))
        {
            timeout = 0;
        }
        else
        {
            /* FIXME: 100 should be RT_TICKS_PER_SECOND */
            timeout = (abstime->tv_sec - tnow.tv_sec) * 100;
            if (abstime->tv_nsec < tnow.tv_nsec)
            {
                timeout -= 100;
                timeout += 100 - (tnow.tv_nsec - abstime->tv_nsec) / 10000000;
            }
            else
            {
                timeout += (abstime->tv_nsec - tnow.tv_nsec) / 10000000;
            }
        }
    }
    else
    {
        timeout = clock_time_to_tick(abstime);
    }
    result = _pthread_cond_timedwait(cond, mutex, timeout);
    if (result == RT_EOK)
        return 0;
    if (result == -RT_ETIMEOUT)
        return ETIMEDOUT;

    return EINVAL;
}
RTM_EXPORT(pthread_cond_timedwait);

