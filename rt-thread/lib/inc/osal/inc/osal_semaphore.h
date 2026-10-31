#ifndef _FH_OSAL_SEMAPHORE_H
#define _FH_OSAL_SEMAPHORE_H

typedef struct fh_osal_semaphore{
	void * sem;
}fh_osal_semaphore_t;

int fh_osal_sema_init(fh_osal_semaphore_t *sem, int val);
int fh_osal_down(fh_osal_semaphore_t *sem);
int fh_osal_down_interruptible(fh_osal_semaphore_t *sem);
int fh_osal_down_trylock(fh_osal_semaphore_t *sem);
int fh_osal_down_timeout(fh_osal_semaphore_t *sem, long ms);
void fh_osal_up(fh_osal_semaphore_t *sem);
void fh_osal_sema_destroy(fh_osal_semaphore_t *sem);

#endif

