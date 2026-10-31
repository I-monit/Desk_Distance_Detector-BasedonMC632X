
/* This is the Linux kernel implementation of the FH platform library. */

#include "fh_os.h"
#include "fh_list.h"
#include <rtthread.h>

/* MISC */
void fh_otgudelay(int n)
{
        otgudelay(n);
}

void fh_otgmdelay(int n)
{
        otgudelay(n*1000);
}

void *FH_MEMSET(void *dest, uint8_t byte, uint32_t size)
{
    return rt_memset(dest, byte, size);
}

void *FH_MEMCPY(void *dest, void const *src, uint32_t size)
{
    return rt_memcpy(dest, src, size);
}

void *FH_MEMMOVE(void *dest, void *src, uint32_t size)
{
    return rt_memmove(dest, src, size);
}

int FH_MEMCMP(void *m1, void *m2, uint32_t size)
{
    return rt_memcmp(m1, m2, size);
}

int FH_STRNCMP(void *s1, void *s2, uint32_t size)
{
    return rt_strncmp(s1, s2, size);
}

int FH_STRCMP(void *s1, void *s2)
{
    return rt_strcmp(s1, s2);
}

int FH_STRLEN(char const *str)
{
    return rt_strlen(str);
}

/*
char *FH_STRCPY(char *to, char const *from)
{
    return rt_strcpy(to, from);
}
*/

char *FH_STRDUP(char const *str)
{
    int len = FH_STRLEN(str) + 1;
    char *new = FH_ALLOC_ATOMIC(len);

    if (!new)
    {
        return NULL;
    }

    FH_MEMCPY(new, str, len);
    return new;
}

/* fh_debug.h */

void FH_VPRINTF(char *format, va_list args)
{
    rt_kprintf(format, args);
}

int FH_VSNPRINTF(char *str, int size, char *format, va_list args)
{
    return rt_vsnprintf(str, size, format, args);
}

void FH_PRINTF(char *format, ...)
{
    char buffer[128];
    va_list args;

    va_start(args, format);
    rt_vsprintf(buffer, format, args);
    va_end(args);
    if (buffer)
        rt_kprintf("%s", buffer);
}

int FH_SPRINTF(char *buffer, char *format, ...)
{
    int retval;
    va_list args;

    va_start(args, format);
    retval = rt_vsprintf(buffer, format, args);
    va_end(args);
    return retval;
}

int FH_SNPRINTF(char *buffer, int size, char *format, ...)
{
    int retval;
    va_list args;

    va_start(args, format);
    retval = rt_vsnprintf(buffer, size, format, args);
    va_end(args);
    return retval;
}

void __FH_WARN(char *format, ...)
{
    va_list args;

    va_start(args, format);
    FH_PRINTF(KERN_WARNING);
    FH_VPRINTF(format, args);
    va_end(args);
}

void __FH_ERROR(char *format, ...)
{
    va_list args;

    va_start(args, format);
    FH_PRINTF(KERN_ERR);
    FH_VPRINTF(format, args);
    va_end(args);
}

void FH_EXCEPTION(char *format, ...)
{
    va_list args;

    va_start(args, format);
    FH_PRINTF(KERN_ERR);
    FH_VPRINTF(format, args);
    va_end(args);
    BUG_ON(1);
}

#ifdef DEBUG
void __FH_DEBUG(char *format, ...)
{
    va_list args;

    va_start(args, format);
    FH_PRINTF(KERN_DEBUG);
    FH_VPRINTF(format, args);
    va_end(args);
}
#endif


/* fh_mem.h */

void *__FH_DMA_ALLOC(void *dma_ctx, uint32_t size, fh_dma_t *dma_addr)
{
    /*
    void *buf = dma_alloc_coherent(dma_ctx, (size_t)size, dma_addr, GFP_KERNEL | GFP_DMA32);
    if (!buf) {
        return NULL;
    }

    memset(buf, 0, (size_t)size);
    return buf;
    */
    *dma_addr = (int)fh_dma_mem_malloc_align(4, 4, "dma_heap");

    void *buf = fh_dma_mem_malloc_align(size, 2048, "dma_heap");
    /* *dma_addr = (int)buf; */
    if (!buf)
        rt_memset(buf, 0, (size_t)size);

    return buf;
}

void *__FH_DMA_ALLOC_ATOMIC(void *dma_ctx, uint32_t size, fh_dma_t *dma_addr)
{
    /*
    void *buf = dma_alloc_coherent(NULL, (size_t)size, dma_addr, GFP_ATOMIC);
    if (!buf) {
        return NULL;
    }
    memset(buf, 0, (size_t)size);
    return buf;
    */
    return fh_dma_mem_malloc_align(size, 4, "dma_heap");
}

void __FH_DMA_FREE(void *dma_ctx, uint32_t size, void *virt_addr, fh_dma_t dma_addr)
{
    /* dma_free_coherent(dma_ctx, size, virt_addr, dma_addr); */
}
extern rt_int32_t g_mem_debug;
void *__FH_ALLOC(void *mem_ctx, uint32_t size)
{
    /* return kzalloc(size, GFP_KERNEL); */
    /* return fh_dma_mem_malloc_align(size, 4, "dma_heap"); */
    /* RT_DEBUG_LOG(1,("rt_malloc:%d,size:%d\n",g_mem_debug++,size)); */
    return rt_malloc(size + 32/*cache line*/);
}

void *__FH_ALLOC_ATOMIC(void *mem_ctx, uint32_t size)
{
    /* return kzalloc(size, GFP_ATOMIC); */
    /* return fh_dma_mem_malloc_align(size, 4, "dma_heap"); */
    /* RT_DEBUG_LOG(1,("rt_malloc:%d,size:%d\n",g_mem_debug++,size)); */
    return rt_malloc(size + 32/*cache line*/);
}

void __FH_FREE(void *mem_ctx, void *addr)
{
    /* RT_DEBUG_LOG(1,("rt_free:%d,addr:%x\n",--g_mem_debug,addr)); */
    rt_free(addr);
}


/* Byte Ordering Conversions */

uint32_t FH_CPU_TO_LE32(uint32_t *p)
{
#ifdef __LITTLE_ENDIAN
    return *p;
#else
    uint8_t *u_p = (uint8_t *)p;

    return (u_p[3] | (u_p[2] << 8) | (u_p[1] << 16) | (u_p[0] << 24));
#endif
}

uint32_t FH_CPU_TO_BE32(uint32_t *p)
{
#ifdef __BIG_ENDIAN
    return *p;
#else
    uint8_t *u_p = (uint8_t *)p;

    return (u_p[3] | (u_p[2] << 8) | (u_p[1] << 16) | (u_p[0] << 24));
#endif
}

uint32_t FH_LE32_TO_CPU(uint32_t *p)
{
#ifdef __LITTLE_ENDIAN
    return *p;
#else
    uint8_t *u_p = (uint8_t *)p;

    return (u_p[3] | (u_p[2] << 8) | (u_p[1] << 16) | (u_p[0] << 24));
#endif
}

uint32_t FH_BE32_TO_CPU(uint32_t *p)
{
#ifdef __BIG_ENDIAN
    return *p;
#else
    uint8_t *u_p = (uint8_t *)p;

    return (u_p[3] | (u_p[2] << 8) | (u_p[1] << 16) | (u_p[0] << 24));
#endif
}

uint16_t FH_CPU_TO_LE16(uint16_t *p)
{
#ifdef __LITTLE_ENDIAN
    return *p;
#else
    uint8_t *u_p = (uint8_t *)p;

    return (u_p[1] | (u_p[0] << 8));
#endif
}

uint16_t FH_CPU_TO_BE16(uint16_t *p)
{
#ifdef __BIG_ENDIAN
    return *p;
#else
    uint8_t *u_p = (uint8_t *)p;

    return (u_p[1] | (u_p[0] << 8));
#endif
}

uint16_t FH_LE16_TO_CPU(uint16_t *p)
{
#ifdef __LITTLE_ENDIAN
    return *p;
#else
    uint8_t *u_p = (uint8_t *)p;

    return (u_p[1] | (u_p[0] << 8));
#endif
}

uint16_t FH_BE16_TO_CPU(uint16_t *p)
{
#ifdef __BIG_ENDIAN
    return *p;
#else
    uint8_t *u_p = (uint8_t *)p;

    return (u_p[1] | (u_p[0] << 8));
#endif
}


/* Registers */

uint32_t FH_READ_REG32(uint32_t volatile * reg)
{
    return readl(reg);
}


void FH_WRITE_REG32(uint32_t volatile * reg, uint32_t value)
{
    writel(value, reg);
}

void FH_MODIFY_REG32(uint32_t volatile * reg, uint32_t clear_mask, uint32_t set_mask)
{
    writel((readl(reg) & ~clear_mask) | set_mask, reg);
}

#if 0
/* Locking */

fh_spinlock_t *FH_SPINLOCK_ALLOC(void)
{
    spinlock_t *sl = (spinlock_t *)1;

#if defined(CONFIG_PREEMPT) || defined(CONFIG_SMP)
    sl = FH_ALLOC(sizeof(*sl));
    if (!sl)
    {
        FH_ERROR("Cannot allocate memory for spinlock\n");
        return NULL;
    }

    spin_lock_init(sl);
#endif
    return (fh_spinlock_t *)sl;
}

void FH_SPINLOCK_FREE(fh_spinlock_t *lock)
{
#if defined(CONFIG_PREEMPT) || defined(CONFIG_SMP)
    FH_FREE(lock);
#endif
}

void FH_SPINLOCK(fh_spinlock_t *lock)
{
#if defined(CONFIG_PREEMPT) || defined(CONFIG_SMP)
    spin_lock((spinlock_t *)lock);
#endif
}

void FH_SPINUNLOCK(fh_spinlock_t *lock)
{
#if defined(CONFIG_PREEMPT) || defined(CONFIG_SMP)
    spin_unlock((spinlock_t *)lock);
#endif
}

void FH_SPINLOCK_IRQSAVE(fh_spinlock_t *lock, fh_irqflags_t *flags)
{
    fh_irqflags_t f;

#if defined(CONFIG_PREEMPT) || defined(CONFIG_SMP)
    spin_lock_irqsave((spinlock_t *)lock, f);
#else
    local_irq_save(f);
#endif
    *flags = f;
}

void FH_SPINUNLOCK_IRQRESTORE(fh_spinlock_t *lock, fh_irqflags_t flags)
{
#if defined(CONFIG_PREEMPT) || defined(CONFIG_SMP)
    spin_unlock_irqrestore((spinlock_t *)lock, flags);
#else
    local_irq_restore(flags);
#endif
}

fh_mutex_t *FH_MUTEX_ALLOC(void)
{
    struct mutex *m;
    fh_mutex_t *mutex = (fh_mutex_t *)FH_ALLOC(sizeof(struct mutex));

    if (!mutex)
    {
        FH_ERROR("Cannot allocate memory for mutex\n");
        return NULL;
    }

    m = (struct mutex *)mutex;
    mutex_init(m);
    return mutex;
}

#if (defined(FH_LINUX) && defined(CONFIG_DEBUG_MUTEXES))
#else
void FH_MUTEX_FREE(fh_mutex_t *mutex)
{
    mutex_destroy((struct mutex *)mutex);
    FH_FREE(mutex);
}
#endif

void FH_MUTEX_LOCK(fh_mutex_t *mutex)
{
    struct mutex *m = (struct mutex *)mutex;

    mutex_lock(m);
}

int FH_MUTEX_TRYLOCK(fh_mutex_t *mutex)
{
    struct mutex *m = (struct mutex *)mutex;

    return mutex_trylock(m);
}

void FH_MUTEX_UNLOCK(fh_mutex_t *mutex)
{
    struct mutex *m = (struct mutex *)mutex;

    mutex_unlock(m);
}
#endif

/* Timers */

struct fh_timer
{
    /* struct timer_list *t; */
    rt_timer_t t;
    char *name;
    fh_timer_callback_t cb;
    void *data;
    uint8_t scheduled;
    /* fh_spinlock_t *lock; */
};

static void timer_callback(void *data)
{
    fh_timer_t *timer = (fh_timer_t *)data;
    /* fh_irqflags_t flags; */

    /* FH_SPINLOCK_IRQSAVE(timer->lock, &flags); */
    timer->scheduled = 0;
    /* FH_SPINUNLOCK_IRQRESTORE(timer->lock, flags); */
    FH_DEBUG("Timer %s callback", timer->name);
    timer->cb(timer->data);
}

fh_timer_t *FH_TIMER_ALLOC(char *name, fh_timer_callback_t cb, void *data)
{
    fh_timer_t *t = FH_ALLOC(sizeof(*t));

    if (!t)
    {
        FH_ERROR("Cannot allocate memory for timer");
        return NULL;
    }

    t->name = FH_STRDUP(name);
    if (!t->name)
    {
        FH_ERROR("Cannot allocate memory for timer->name");
        return NULL;
    }

    t->scheduled = 0;
    t->cb = cb;
    t->data = data;

    t->t = rt_timer_create(t->name,   /* 定时器名字是 timer2 */
            timer_callback, /* 超时时回调的处理函数 */
            NULL, /* 超时函数的入口参�?*/
            0, /* 定时长度�?0个OS Tick */
            RT_TIMER_FLAG_ONE_SHOT); /* 单次定时�?*/

    if (t->t == NULL)
    {
        FH_FREE(t);
        FH_ERROR("timer init failed\n");
        return NULL;
    }

    return t;
}

void FH_TIMER_FREE(fh_timer_t *timer)
{
    /* fh_irqflags_t flags; */

    /* FH_SPINLOCK_IRQSAVE(timer->lock, &flags); */

    if (timer->scheduled)
    {
        rt_timer_delete(timer->t);
        timer->scheduled = 0;
    } else
		rt_timer_delete(timer->t);

    /* FH_SPINUNLOCK_IRQRESTORE(timer->lock, flags); */
    /* FH_SPINLOCK_FREE(timer->lock); */
    /* FH_FREE(timer->t); */
    FH_FREE(timer->name);
    FH_FREE(timer);
}

void FH_TIMER_SCHEDULE(fh_timer_t *timer, uint32_t time)
{
    /* fh_irqflags_t flags; */

    /* FH_SPINLOCK_IRQSAVE(timer->lock, &flags); */
    rt_tick_t tick = time * RT_TICK_PER_SECOND / 1000;

    if (!timer->scheduled)
    {
        timer->scheduled = 1;
        FH_DEBUG("Scheduling timer %s to expire in +%d msec", timer->name, time);
        rt_timer_control(timer->t, RT_TIMER_CTRL_SET_TIME, &tick);
        rt_timer_start(timer->t);
        /* timer->t->expires = jiffies + msecs_to_jiffies(time); */
        /* add_timer(timer->t); */
    }
    else
    {
        FH_DEBUG("Modifying timer %s to expire in +%d msec", timer->name, time);
        /* mod_timer(timer->t, jiffies + msecs_to_jiffies(time)); */
        rt_timer_control(timer->t, RT_TIMER_CTRL_SET_TIME, &tick);
    }

    /* FH_SPINUNLOCK_IRQRESTORE(timer->lock, flags); */
}

void FH_TIMER_CANCEL(fh_timer_t *timer)
{
    rt_timer_stop(timer->t);
}

