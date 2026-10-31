
/* #include <linux/compat.h> */
/* #include <linux/list.h> */
#include <ctype.h>
#include <pthread.h>
#include <port_rt.h>
/* #include <stdlib.h> */
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <ubiport_common.h>
#include <rtthread.h>
#include <semaphore.h>
#include <rthw.h>

#define SEMWAITTIME 0x01000000

unsigned long long simple_strtoull(const char *cp, char **endp, unsigned int base)
{
    unsigned long long result = 0, value;

    if (*cp == '0')
    {
        cp++;
        if ((toupper(*cp) == 'X') && isxdigit(cp[1]))
        {
            base = 16;
            cp++;
        }
        if (!base)
        {
            base = 8;
        }
    }
    if (!base)
    {
        base = 10;
    }
    while (isxdigit(*cp) && (value = isdigit(*cp) ? *cp - '0' : (islower(*cp) ? toupper(*cp) : *cp) - 'A' + 10) < base)
    {
        result = result * base + value;
        cp++;
    }
    if (endp)
        *endp = (char *)cp;
    return result;
}
unsigned long simple_strtoul(const char *cp, char **endp, unsigned int base)
{
    unsigned long result = 0, value;

    if (!base)
    {
        base = 10;
        if (*cp == '0')
        {
            base = 8;
            cp++;
            if ((toupper(*cp) == 'X') && isxdigit(cp[1]))
            {
                cp++;
                base = 16;
            }
        }
    }
    else if (base == 16)
    {
        if (cp[0] == '0' && toupper(cp[1]) == 'X')
            cp += 2;
    }
    while (isxdigit(*cp) &&
           (value = isdigit(*cp) ? *cp - '0' : toupper(*cp) - 'A' + 10) < base)
    {
        result = result * base + value;
        cp++;
    }
    if (endp)
        *endp = (char *)cp;
    return result;
}
/**
 * simple_strtol - convert a string to a signed long
 * @cp: The start of the string
 * @endp: A pointer to the end of the parsed string will be placed here
 * @base: The number base to use
 */
long simple_strtol(const char *cp, char **endp, unsigned int base)
{
    if (*cp == '-')
        return -simple_strtoul(cp + 1, endp, base);
    return simple_strtoul(cp, endp, base);
}

/* static __cacheline_aligned_in_smp DEFINE_SPINLOCK(inode_hash_lock); */
/*
 * find_inode_fast is the fast path version of find_inode, see the comment at
 * iget_locked for details.
 */
/*
static struct inode *find_inode_fast(struct super_block *sb,
                struct hlist_head *head, unsigned long ino)
{
    struct inode *inode = NULL;

repeat:
    hlist_for_each_entry(inode, head, i_hash) {
        if (inode->i_ino != ino)
            continue;
        if (inode->i_sb != sb)
            continue;
        spin_lock(&inode->i_lock);
        if (inode->i_state & (I_FREEING|I_WILL_FREE)) {
            __wait_on_freeing_inode(inode);
            goto repeat;
        }
        __iget(inode);
        spin_unlock(&inode->i_lock);
        return inode;
    }
    return NULL;
}
struct inode *ilookup(struct super_block *sb, unsigned long ino)
{
    struct hlist_head *head = inode_hashtable + hash(sb, ino);
    struct inode *inode;

    //spin_lock(&inode_hash_lock);
    inode = find_inode_fast(sb, head, ino);
    //spin_unlock(&inode_hash_lock);

    if (inode)
        wait_on_inode(inode);
    return inode;
}
*/

void imutex_init(struct mutex *pMutex, char *name)
{
    if (pMutex == NULL)
    {
        ubiPort_Err("%s,%d pMutex null\n", __FUNCTION__, __LINE__);
        return;
    }
    if (sizeof(pMutex->mMutex) < sizeof(struct rt_mutex))
    {
        ubiPort_Err("%s,%d mutex size too small!!.%d,%d\n", __FUNCTION__, __LINE__, sizeof(pMutex->mMutex), sizeof(struct rt_mutex));
        exit(-1);
    }
    char mname[16] = {0};
    int i = 0, j = 0;
    for (i = 0; i < strlen(name) && i < sizeof(mname); i++)
    {
        if (name[i] != '&' && name[i] != '>')
        {
            mname[j] = name[i];
            j++;
        }
    }
    rt_mutex_t pMtx = (rt_mutex_t)pMutex->mMutex;
    rt_mutex_init(pMtx, mname, RT_IPC_FLAG_FIFO);
}
void mutex_uninit(struct mutex *pMutex)
{
    if (pMutex == NULL)
    {
        ubiPort_Err("%s,%d pMutex null.%p\n", __FUNCTION__, __LINE__, pMutex);
        return;
    }
    rt_mutex_t pMtx = (rt_mutex_t)pMutex->mMutex;
    rt_mutex_detach(pMtx);
}
void mutex_destroy(struct mutex *pMutex)
{
    if (pMutex == NULL)
    {
        ubiPort_Err("%s,%d pMutex null.%p\n", __FUNCTION__, __LINE__, pMutex);
        return;
    }
    rt_mutex_t pMtx = (rt_mutex_t)pMutex->mMutex;
    rt_mutex_delete(pMtx);
}

void mutex_lock(struct mutex *pMutex)
{
    if (pMutex == NULL)
    {
        ubiPort_Err("%s,%d pMutex null.%p\n", __FUNCTION__, __LINE__, pMutex);
        return;
    }
    rt_mutex_t pMtx = (rt_mutex_t)pMutex->mMutex;
    rt_mutex_take(pMtx, SEMWAITTIME);
}
/* success:1 ,fail 0: */
int mutex_trylock(struct mutex *pMutex) /* add for compile */
{
    if (pMutex == NULL)
    {
        ubiPort_Err("%s,%d pMutex null.%p\n", __FUNCTION__, __LINE__, pMutex);
        return 0;
    }
    rt_mutex_t pMtx = (rt_mutex_t)pMutex->mMutex;
    int err = rt_mutex_take(pMtx, 0);
    if (err == RT_EOK)
    {
        return 1;
    }
    return 0;
}
void mutex_unlock(struct mutex *pMutex)
{
    if (pMutex == NULL)
    {
        ubiPort_Err("%s,%d pMutex null.%p\n", __FUNCTION__, __LINE__, pMutex);
        return;
    }
    rt_mutex_t pMtx = (rt_mutex_t)pMutex->mMutex;
    rt_mutex_release(pMtx);
}
int mutex_is_locked(struct mutex *pMutex)
{
    if (pMutex == NULL)
    {
        ubiPort_Err("%s,%d pMutex null.%p\n", __FUNCTION__, __LINE__, pMutex);
        return 0;
    }
    rt_mutex_t pMtx = (rt_mutex_t)pMutex->mMutex;
    return pMtx->hold == 0 ? 0 : 1; /* pMtx->owner == NULL ? 0 : 1; */
}
/* lock when write, get sem when only read. */
typedef struct rt_port_rwsemaphore
{
    volatile int activity; /* -1 write;0 not use; + read */
    struct rt_semaphore rtSem;
} rt_port_sem;
void iInit_rwsem(struct rw_semaphore *pSem, char *name)
{
    if (pSem == NULL)
    {
        ubiPort_Err("%s,%d pSem null\n", __FUNCTION__, __LINE__);
        return;
    }
    if (sizeof(pSem->mSem) < sizeof(rt_port_sem))
    {
        ubiPort_Err("%s,%d sem size too small!!.%d,%d\n", __FUNCTION__, __LINE__, sizeof(pSem->mSem), sizeof(rt_port_sem));
        exit(-1);
    }
    char mname[16] = {0};
    int i = 0, j = 0;
    for (i = 0; i < strlen(name) && i < sizeof(mname); i++)
    {
        if (name[i] != '&' && name[i] != '>')
        {
            mname[j] = name[i];
            j++;
        }
    }
    rt_port_sem *pportSem = (rt_port_sem *)pSem->mSem;
    if (rt_sem_init(&(pportSem->rtSem), mname, 1, RT_IPC_FLAG_FIFO))
    {
        ubiPort_Err("%s,%d sem_init error l\n", __FUNCTION__, __LINE__);
    }
    pportSem->activity = 0;
/*    ubiPort_Log("sem %s,%p\n", __FUNCTION__, pSem);*/
}
void uninit_rwsem(struct rw_semaphore *pSem)
{
    if (pSem == NULL)
    {
        ubiPort_Err("%s,%d pSem null\n", __FUNCTION__, __LINE__);
        return;
    }
    rt_port_sem *pportSem = (rt_port_sem *)pSem->mSem;
    /* rt_sem_t sem = (rt_sem_t)pSem->mSem; */
    ubiPort_Log("sem %s,%p\n", __FUNCTION__, pSem);/*
*/    rt_sem_detach(&(pportSem->rtSem));
    pportSem->activity = 0;
}

void down_read(struct rw_semaphore *pSem)
{
    if (pSem == NULL)
    {
        ubiPort_Err("%s,%d pSem null\n", __FUNCTION__, __LINE__);
        return;
    }
    rt_port_sem *pportSem = (rt_port_sem *)pSem->mSem;
    /* rt_sem_t sem = (rt_sem_t )pSem->mSem; */
/*    ubiPort_Log("sem %s,%p\n", __FUNCTION__, pSem);*/
    /* rt_uint32_t  level = rt_hw_interrupt_disable(); */
    if (pportSem->activity > 0)
    {
        pportSem->activity++;
        /* rt_hw_interrupt_enable(level); */
    }
    else /* writing or no take */
    {
        /* rt_hw_interrupt_enable(level); */
        rt_sem_take(&(pportSem->rtSem), SEMWAITTIME);
        if (pportSem->activity < 0)
        {
            rt_thread_mdelay(50);
            ubiPort_Err("%s,%d wait w over\n", __FUNCTION__, __LINE__);
        }
        /* level = rt_hw_interrupt_disable(); */
        pportSem->activity++;
        /* rt_hw_interrupt_enable(level); */
    }
}
void down_write(struct rw_semaphore *pSem)
{
    if (pSem == NULL)
    {
        ubiPort_Err("%s,%d pSem null\n", __FUNCTION__, __LINE__);
        return;
    }
    rt_port_sem *pportSem = (rt_port_sem *)pSem->mSem;
    /* rt_sem_t sem = (rt_sem_t )pSem->mSem; */
/*    ubiPort_Log("sem %s,%p\n", __FUNCTION__, pSem);*/
    /*if (pportSem->activity > 0) //reading,wait
    {
        while (pportSem->activity > 0)
        {
            rt_thread_mdelay(50);
            ubiPort_Err("%s,%d wait r over,%d\n", __FUNCTION__, __LINE__, pportSem->activity);
        }
    }*/
    rt_sem_take(&(pportSem->rtSem), SEMWAITTIME);
    /* rt_uint32_t  level = rt_hw_interrupt_disable(); */
    pportSem->activity--;
    /* rt_hw_interrupt_enable(level); */
}
/* Success 1,fail:0 */
int down_write_trylock(struct rw_semaphore *pSem)
{
    if (pSem == NULL)
    {
        ubiPort_Err("%s,%d pSem null\n", __FUNCTION__, __LINE__);
        return 0;
    }
    rt_port_sem *pportSem = (rt_port_sem *)pSem->mSem;
    /* rt_sem_t sem = (rt_sem_t)pSem->mSem; */
/*    ubiPort_Log("sem %s,%p\n", __FUNCTION__, pSem);*/
    /* rt_uint32_t  level = rt_hw_interrupt_disable(); */
    if (pportSem->activity <= 0)
    {
        int err = rt_sem_trytake(&(pportSem->rtSem));
        if (err == RT_EOK)
        {
            pportSem->activity--;
            /* rt_hw_interrupt_enable(level); */
            return 1;
        }
    }
    /* rt_hw_interrupt_enable(level); */
    return 0;
}
void up_read(struct rw_semaphore *pSem)
{
    if (pSem == NULL)
    {
        ubiPort_Err("%s,%d pSem null\n", __FUNCTION__, __LINE__);
        return;
    }
    rt_port_sem *pportSem = (rt_port_sem *)pSem->mSem;
    /* rt_sem_t sem = (rt_sem_t )pSem->mSem; */
/*    ubiPort_Log("sem %s,%p\n", __FUNCTION__, pSem);*/
    /* rt_uint32_t  level = rt_hw_interrupt_disable(); */
    if (pportSem->activity > 0) /* reading */
    {
        pportSem->activity--;
        if (pportSem->activity == 0)
        {
            rt_sem_release(&(pportSem->rtSem));
        }
    }
    else
    {
        ubiPort_Err("%s,%d error!! read status error. %d \n", __FUNCTION__, __LINE__, pportSem->activity);
    }
    /* rt_hw_interrupt_enable(level); */
    /* rt_sem_release(&(pportSem->rtSem)); */
}
void up_write(struct rw_semaphore *pSem)
{
    if (pSem == NULL)
    {
        ubiPort_Err("%s,%d pSem null\n", __FUNCTION__, __LINE__);
        return;
    }
    rt_port_sem *pportSem = (rt_port_sem *)pSem->mSem;
    /* rt_sem_t sem = (rt_sem_t )pSem->mSem; */
/*    ubiPort_Log("sem %s,%p\n", __FUNCTION__, pSem);*/
    /* rt_uint32_t  level = rt_hw_interrupt_disable(); */
    if (pportSem->activity < 0)
    {
        rt_sem_release(&(pportSem->rtSem));
        pportSem->activity++;
    }
    else
    {
        rt_sem_release(&(pportSem->rtSem));
        pportSem->activity++;
        ubiPort_Err("%s,%d error!! upwrite status error. %d,%s \n", __FUNCTION__, __LINE__, pportSem->activity, pportSem->rtSem.parent.parent.name);
    }
    /* rt_hw_interrupt_enable(level); */
}
void spin_lock_init(spinlock_t *lock)
{
    /* mutex_init(&lock->spin_mutex); */
    lock->lock = 0;
    ubiPort_Log("%p lock = 0\n", lock);
}
void spin_lock_prefetch(spinlock_t *lock)
{
    /* mutex_init(&lock->spin_mutex); */
    lock->lock = 0;
    ubiPort_Log("%p lock = 0\n", lock);
}
void spin_lock(spinlock_t *lock)
{
    /* if (lock->spin_mutex.mMutex[0] == 0 && lock->spin_mutex.mMutex[1] == 0)
    {
        mutex_init(&lock->spin_mutex);
    }
    mutex_lock(&lock->spin_mutex);*/
    while (lock->lock == 1)
    {
#ifdef __linux__
        usleep(100000);
#else
        rt_thread_mdelay(100);
#endif
        ubiPort_Err("%p locking,should wait\n", lock);
    }
    lock->lock = 1;
/*    ubiPort_Log("%p locked\n", lock);*/
}
void spin_unlock(spinlock_t *lock)
{
    /* mutex_unlock(&lock->spin_mutex); */
    lock->lock = 0;
/*    ubiPort_Log("%p unlock\n", lock);*/
}
struct p_current cur = {
    .pid = 1,
};
/* struct p_current *current = &cur; */

struct p_current *currentThread()
{
    rt_thread_t pThr = rt_thread_self();
    cur.pid = (int)pThr;
    ubiPort_Log1("%s:%d ", pThr->name, pThr->current_priority);
    return &cur;
}
struct task_struct *kthread_create(int (*threadfn)(void *data),
                                   void *data,
                                   const char namefmt[], char *name)
{
    rt_thread_t tid1 = rt_thread_create(name,
                                        (void (*)(void *))threadfn, data,
                                        0x10000, RT_THREAD_PRIORITY_MAX / 3 - 1, 20);
    ubiPort_Log("create thread %s\n", name);
    return (struct task_struct *)tid1;
}
int kthread_stop(struct task_struct *k)
{
    rt_thread_t tid = (rt_thread_t)k;
    if (tid != NULL)
    {
        if (rt_thread_delete(tid))
        {
            ubiPort_Err("stop thread error\n");
        }
        return 0;
    }
    else
    {
        ubiPort_Err("thread handle error\n");
    }
    return -1;
}
int kthread_should_stop(void)
{
    rt_thread_t pSelf = rt_thread_self();
    rt_uint8_t stat;
    stat = (pSelf->stat & RT_THREAD_STAT_MASK);
    if (RT_THREAD_CLOSE == stat)
    {
        return 1;
    }
    return 0;
    /* if (stat == RT_THREAD_READY)        rt_kprintf(" ready  ");
     else if (stat == RT_THREAD_SUSPEND) rt_kprintf(" suspend");
     else if (stat == RT_THREAD_INIT)    rt_kprintf(" init   ");
     else if (stat == RT_THREAD_CLOSE)   rt_kprintf(" close  "); */
}
void wake_up_process(struct task_struct *k)
{
    rt_thread_t tid = (rt_thread_t)k;
    if (tid != NULL)
    {
        if ((tid->stat & RT_THREAD_STAT_MASK) == RT_THREAD_INIT)
        {
            rt_thread_startup(tid);
        }
        else
        {
            rt_thread_resume(tid);
        }
    }
    else
    {
        ubiPort_Err("thread handle error\n");
    }
}

/*void set_current_state(int state)
{
    TASK_INTERRUPTIBLE;
}*/
void schedule(void)
{
#ifdef __linux__
    rt_thread_suspend(rt_thread_self());
    rt_schedule();
    usleep(10000);
#else
    rt_schedule();
    rt_thread_mdelay(100);
#endif
}
void yield(void)
{
    rt_thread_yield();
#ifdef __linux__
    usleep(10000);
#else
/* rt_thread_mdelay(400); */
#endif
}
void port_rt_sleep(int ms)
{
    rt_thread_mdelay(ms);
}
int setThreadPriority(struct task_struct *pTid, unsigned char priority)
{
    rt_uint8_t threadPriority = priority;
    rt_thread_t tid = (rt_thread_t)pTid;
    ubiPort_Err("set pid:%p %d,%d\n", tid, threadPriority, priority);
    return rt_thread_control(tid, RT_THREAD_CTRL_CHANGE_PRIORITY, &threadPriority);
}

/*#define init_waitqueue_head(...)    do { } while (0)
#define wait_event_interruptible(...)    0
#define wake_up_interruptible(...)    do { } while (0)
#define dump_stack(...)            do { } while (0)

#define task_pid_nr(x)            0*/
/* #define set_freezable(...)        do { } while (0) */
/* #define try_to_freeze(...)        0 */
/* #define set_current_state(...)        do { } while (0) */

void wake_up(wait_queue_head_t *wait)
{
/*    ubiPort_Err("call wakeup \n");*/
}
unsigned long iget_seconds(void)
{
    time_t now;
    now = time(RT_NULL);
    return now;
}

void *malloc_cache_aligned(size_t size)
{
    return rt_malloc_align(size, ARCH_DMA_MINALIGN);
}
void free_cache_aligned(void *ptr)
{
    return rt_free_align(ptr);
}
void kfree(const void *block)
{
    rt_free((void *)block);
}
void vfree(const void *addr)
{
    rt_free((void *)addr);
}
void kmem_cache_free(struct kmem_cache *cachep, void *obj)
{
    rt_free_align(obj);
}
void kmem_cache_destroy(struct kmem_cache *cachep)
{
    rt_free_align(cachep);
    /* rt_free(cachep); */
}
void *kmalloc(size_t size, int flags)
{
    void *p;

    p = rt_malloc(size); /* malloc_cache_aligned(size); */
    if (p && flags & __GFP_ZERO)
        memset(p, 0, size);

    return p;
}

void *kmalloc_align(size_t size, int alignSize)
{
    return rt_malloc_align(size, alignSize);
}
void kfree_align(void *paddr)
{
    rt_free_align(paddr);
}

void ubi_printf(const char *fmt, ...)
{
    va_list args;
    /* rt_size_t length; */
    /* static char rt_log_buf[RT_CONSOLEBUF_SIZE]; */

    va_start(args, fmt);
    rt_kprintf(fmt, args);
    /* length = rt_vsnprintf(rt_log_buf, sizeof(rt_log_buf) - 1, fmt, args); */
    va_end(args);
}
