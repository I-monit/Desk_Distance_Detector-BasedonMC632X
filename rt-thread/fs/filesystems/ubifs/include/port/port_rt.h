#ifndef __PORT_RT_H__
#define __PORT_RT_H__
/* #include <ubi_types.h> */
#define USE_SCHEDULE_WORK
#ifdef USE_SCHEDULE_WORK
struct work_struct;
typedef void (*work_func_t)(struct work_struct *work);

struct work_struct
{
    work_func_t func;
};
#define INIT_WORK(_work, _func)  \
    do                           \
    {                            \
        (_work)->func = (_func); \
    } while (0)
void schedule_work(struct work_struct *work);
void ubi_InitSchedule(void);
#else
struct work_struct
{
};
#define INIT_WORK(_work, _func) \
    do                          \
    {                           \
    } while (0)
#define ubi_InitSchedule() \
    do                     \
    {                      \
    } while (0)
#define schedule_work(x) \
    do                   \
    {                    \
    } while (0)
#endif

typedef unsigned int size_t;
#define GFP_ATOMIC ((unsigned)0)
#define GFP_KERNEL ((unsigned)0)
#define GFP_NOFS ((unsigned)0)
#define GFP_USER ((unsigned)0)
#define __GFP_NOWARN ((unsigned)0)
#define __GFP_ZERO ((unsigned)0x8000u) /* Return zeroed page on success */

#define TASK_RUNNING 0
#define TASK_INTERRUPTIBLE 1
#define TASK_UNINTERRUPTIBLE 2
#define __TASK_STOPPED 4
#define __TASK_TRACED 8

typedef int wait_queue_head_t;

unsigned long simple_strtoul(const char *cp, char **endp, unsigned int base);
/* long wait_event(wait_queue_head_t q, int state); */
unsigned long long simple_strtoull(const char *cp, char **endp, unsigned int base);
long simple_strtol(const char *cp, char **endp, unsigned int base);

struct mutex
{
    int mMutex[15]; /*pthread_mutex_t mMutex;*/
};                  /*just want to not include pthread.h
it will include rt_xx.h. must biger than pthread_mutext_t */
#define mutex_init(mux) imutex_init(mux, #mux)
void imutex_init(struct mutex *pMutex, char *name);
void mutex_uninit(struct mutex *pMutex);
void mutex_lock(struct mutex *pMutex);
int mutex_trylock(struct mutex *pMutex);
void mutex_unlock(struct mutex *pMutex);
int mutex_is_locked(struct mutex *pMutex);
#define mutex_lock_nested(mutex, p1) mutex_lock(mutex)
/* #define mutex_unlock_nested(...) */

struct rw_semaphore
{
    int mSem[12];
};
#define init_rwsem(sem) iInit_rwsem(sem, #sem)
void iInit_rwsem(struct rw_semaphore *pSem, char *name);
void uninit_rwsem(struct rw_semaphore *pSem);
void down_read(struct rw_semaphore *pSem);
void down_write(struct rw_semaphore *pSem);
int down_write_trylock(struct rw_semaphore *pSem);
void up_read(struct rw_semaphore *pSem);
void up_write(struct rw_semaphore *pSem);

typedef struct
{
    volatile int lock;
    /* struct mutex spin_mutex; */
} spinlock_t;
#if defined(__GNUC__)
/* typedef struct { } spinlock_t; */

#define SPIN_LOCK_UNLOCKED \
    (spinlock_t) { .lock = 0 }
#define DEFINE_SPINLOCK(x) spinlock_t x = SPIN_LOCK_UNLOCKED
#endif
void spin_lock_init(spinlock_t *lock);
void spin_lock(spinlock_t *lock);
void spin_unlock(spinlock_t *lock);
void spin_lock_prefetch(spinlock_t *lock);
#define spin_lock_irqsave(lock, flags) \
    do                                 \
    {                                  \
        debug("%lu\n", flags);         \
    } while (0)
#define spin_unlock_irqrestore(lock, flags) \
    do                                      \
    {                                       \
        flags = 0;                          \
    } while (0)

struct p_current
{
    int pid;
};
#define current currentThread()
/* extern struct p_current *current; */
struct p_current *currentThread();
struct task_struct *kthread_create(int (*threadfn)(void *data),
                                   void *data,
                                   const char namefmt[], char *name);
int kthread_stop(struct task_struct *k);
int kthread_should_stop(void);
void wake_up_process(struct task_struct *k);
void schedule(void);
void yield(void);
void port_rt_sleep(int ms);
void cond_resched(void);
void wake_up(wait_queue_head_t *wait);
int setThreadPriority(struct task_struct *pTid, unsigned char priority);

#define __wait_event(wq, condition) \
    do                              \
    {                               \
        for (;;)                    \
        {                           \
            port_rt_sleep(10);      \
            if (condition)          \
                break;              \
        }                           \
    } while (0)

#define wait_event(wq, condition)    \
    do                               \
    {                                \
        if (condition)               \
            break;                   \
        __wait_event(wq, condition); \
    } while (0)

#define init_waitqueue_head(...) \
    do                           \
    {                            \
    } while (0)
#define wait_event_interruptible(...) 0
#define wake_up_interruptible(...) \
    do                             \
    {                              \
    } while (0)
#define dump_stack(...) \
    do                  \
    {                   \
    } while (0)

#define task_pid_nr(x) 0
#define set_freezable(...) \
    do                     \
    {                      \
    } while (0)
#define try_to_freeze(...) 0
#define set_current_state(...) \
    do                         \
    {                          \
    } while (0)
#define cond_resched() \
    do                 \
    {                  \
    } while (0)

unsigned long iget_seconds(void);
struct kmem_cache
{
    int sz;
};
/* #define MEMDEBUG */
#define ARCH_DMA_MINALIGN 4
void *malloc_cache_aligned(size_t size);
void *kmalloc(size_t size, int flags);
void *kmalloc_align(size_t size, int alignSize);
void free_cache_aligned(void *ptr);
void kfree(const void *block);
void vfree(const void *addr);
void kmem_cache_free(struct kmem_cache *cachep, void *obj);
void kmem_cache_destroy(struct kmem_cache *cachep);
void kfree_align(void *paddr);

#endif
