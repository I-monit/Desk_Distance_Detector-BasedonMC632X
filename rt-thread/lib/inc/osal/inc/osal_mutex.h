#ifndef _FH_OSAL_MUTEX_H
#define _FH_OSAL_MUTEX_H

typedef struct fh_osal_mutex {
	void * mutex;
}fh_osal_mutex_t;
int fh_osal_mutex_init(fh_osal_mutex_t *mutex);
int fh_osal_mutex_lock(fh_osal_mutex_t *mutex);
int fh_osal_mutex_lock_interruptible(fh_osal_mutex_t *mutex);
int fh_osal_mutex_trylock(fh_osal_mutex_t *mutex);
int fh_osal_mutex_is_locked(fh_osal_mutex_t *mutex);
void fh_osal_mutex_unlock(fh_osal_mutex_t *mutex);
void fh_osal_mutex_destroy(fh_osal_mutex_t *mutex);

#endif