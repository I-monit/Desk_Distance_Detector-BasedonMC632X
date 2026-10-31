#ifndef _FH_OSAL_SPINLOCK_H
#define _FH_OSAL_SPINLOCK_H

typedef struct fh_osal_spinlock{
	void * lock;
}fh_osal_spinlock_t;
int fh_osal_spin_lock_init(fh_osal_spinlock_t *lock);
void fh_osal_spin_lock(fh_osal_spinlock_t *lock);
int fh_osal_spin_trylock(fh_osal_spinlock_t *lock);
void fh_osal_spin_unlock(fh_osal_spinlock_t *lock);
void fh_osal_spin_lock_irqsave(fh_osal_spinlock_t *lock, unsigned long *flags);
int fh_osal_spin_trylock_irqsave(fh_osal_spinlock_t *lock, unsigned long *flags);
void fh_osal_spin_unlock_irqrestore(fh_osal_spinlock_t *lock, unsigned long *flags);
void fh_osal_spin_lock_destroy(fh_osal_spinlock_t *lock);

#endif