#ifndef __LINUX_SPINLOCK_H__
#define __LINUX_SPINLOCK_H__

#if defined (__GNUC__)    
typedef struct {volatile int count; } spinlock_t;

#define SPIN_LOCK_UNLOCKED (spinlock_t) {0}
#define DEFINE_SPINLOCK(x) spinlock_t x = SPIN_LOCK_UNLOCKED

#else
#error "please use a right C compiler"
#endif

extern void spin_lock(spinlock_t *lock);
extern void spin_unlock(spinlock_t *lock);

#endif /* __LINUX_SPINLOCK_H__ */
