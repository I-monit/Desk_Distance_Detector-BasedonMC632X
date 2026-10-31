/* =========================================================================
 * $File: //dwh/usb_iip/dev/software/fh_common_port_2/fh_os.h $
 * $Revision: #15 $
 * $Date: 2015/06/12 $
 * $Change: 2859407 $
 *
 * Synopsys Portability Library Software and documentation
 * (hereinafter, "Software") is an Unsupported proprietary work of
 * Synopsys, Inc. unless otherwise expressly agreed to in writing
 * between Synopsys and you.
 *
 * The Software IS NOT an item of Licensed Software or Licensed Product
 * under any End User Software License Agreement or Agreement for
 * Licensed Product with Synopsys or any supplement thereto. You are
 * permitted to use and redistribute this Software in source and binary
 * forms, with or without modification, provided that redistributions
 * of source code must retain this notice. You may not view, use,
 * disclose, copy or distribute this file or any information contained
 * herein except pursuant to this license grant from Synopsys. If you
 * do not agree with this notice, including the disclaimer below, then
 * you are not authorized to use the Software.
 *
 * THIS SOFTWARE IS BEING DISTRIBUTED BY SYNOPSYS SOLELY ON AN "AS IS"
 * BASIS AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE HEREBY DISCLAIMED. IN NO EVENT SHALL
 * SYNOPSYS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 * ========================================================================= */
#ifndef __FH_OS_H__
#define __FH_OS_H__

#ifdef __cplusplus
extern "C" {
#endif

/** @file
 *
 * FH portability library, low level os-wrapper functions
 *
 */

/* These basic types need to be defined by some OS header file or custom header
 * file for your specific target architecture.
 *
 * uint8_t, int8_t, uint16_t, int16_t, uint32_t, int32_t, uint64_t, int64_t
 *
 * Any custom or alternate header file must be added and enabled here.
 */

#include <usb_errno.h>
#include <rtdef.h>
#include <rtthread.h>
#include <ktime.h>
#include <dma_mem.h>
#include <rtthread.h>

extern void __bad_udelay(void);
extern void __delay(int loops);
extern void __udelay(int loops);
extern void __const_udelay(rt_uint32_t);

#define MAX_UDELAY_MS 2

#define otgudelay(n)                                                          \
        (__builtin_constant_p(n)                                               \
        ? ((n) > (MAX_UDELAY_MS * 1000)                                   \
        ? __bad_udelay()                                           \
        : __const_udelay((n) *                                     \
        ((2199023U * RT_TICK_PER_SECOND) >> 11))) \
         : __udelay(n))

void fh_otgudelay(int n);
void fh_otgmdelay(int n);

/** @name Primitive Types and Values */
#define KERN_EMERG              "<0>" /* system is unusable */
#define KERN_ALERT              "<1>" /* action must be taken immediately */
#define KERN_CRIT               "<2>" /* critical conditions */
#define KERN_ERR                "<3>" /* error conditions */
#define KERN_WARNING            "<4>" /* warning conditions */
#define KERN_NOTICE             "<5>" /* normal but significant condition */
#define KERN_INFO               "<6>" /* informational */
#define KERN_DEBUG              "<7>" /* debug-level messages */
#define printk rt_kprintf/* diag_printf  //mod by prife */

#define BUG() do { printk("BUG() at %s %d\n", __FILE__, __LINE__); *(int *)0 = 0; } while (0)

#ifndef BUG_ON
#define BUG_ON(x) do { if (unlikely(x)) BUG(); } while(0)
#endif

#define readb(a)    (*(volatile unsigned char  *)(a))
#define readw(a)    (*(volatile unsigned short *)(a))
#define readl(a)    (*(volatile unsigned int   *)(a))

#define writeb(v, a) (*(volatile unsigned char  *)(a) = (v))
#define writew(v, a) (*(volatile unsigned short *)(a) = (v))
#define writel(v, a) (*(volatile unsigned int   *)(a) = (v))

/** We define a boolean type for consistency.  Can be either YES or NO */
/*
typedef signed char int8_t;
typedef unsigned char uint8_t;
typedef signed short int16_t;
typedef unsigned short uint16_t;
typedef signed int int32_t;
typedef unsigned int uint32_t;
*/

typedef rt_bool_t bool;
typedef uint8_t fh_bool_t;
#define YES  1
#define NO   0


/** @name Error Codes */
#define FH_E_INVALID        EINVAL
#define FH_E_NO_MEMORY        ENOMEM
#define FH_E_NO_DEVICE        ENODEV
#define FH_E_NOT_SUPPORTED    EOPNOTSUPP
#define FH_E_TIMEOUT        ETIMEDOUT
#define FH_E_BUSY        EBUSY
#define FH_E_AGAIN        EAGAIN
#define FH_E_RESTART        ERESTART
#define FH_E_ABORT        ECONNABORTED
#define FH_E_SHUTDOWN        ESHUTDOWN
#define FH_E_NO_DATA        ENODATA
#define FH_E_DISCONNECT    ECONNRESET
#define FH_E_UNKNOWN        EINVAL
#define FH_E_NO_STREAM_RES    ENOSR
#define FH_E_COMMUNICATION    ECOMM
#define FH_E_OVERFLOW        EOVERFLOW
#define FH_E_PROTOCOL        EPROTO
#define FH_E_IN_PROGRESS    EINPROGRESS
#define FH_E_PIPE        EPIPE
#define FH_E_IO        EIO
#define FH_E_NO_SPACE        ENOSPC


/**
 * A vprintf() clone.  Just call vprintf if you've got it.
 */
extern void FH_VPRINTF(char *format, va_list args);
#define fh_vprintf FH_VPRINTF

/**
 * A vsnprintf() clone.  Just call vprintf if you've got it.
 */
extern int FH_VSNPRINTF(char *str, int size, char *format, va_list args);
#define fh_vsnprintf FH_VSNPRINTF

/**
 * printf() clone.  Just call printf if you've go it.
 */
extern void FH_PRINTF(char *format, ...)
/* This provides compiler level static checking of the parameters if you're
 * using GCC. */
#ifdef __GNUC__
    __attribute__ ((format(printf, 1, 2)));
#else
    ;
#endif
#define fh_printf FH_PRINTF

/**
 * sprintf() clone.  Just call sprintf if you've got it.
 */
extern int FH_SPRINTF(char *string, char *format, ...)
#ifdef __GNUC__
    __attribute__ ((format(printf, 2, 3)));
#else
    ;
#endif
#define fh_sprintf FH_SPRINTF

/**
 * snprintf() clone.  Just call snprintf if you've got it.
 */
extern int FH_SNPRINTF(char *string, int size, char *format, ...)
#ifdef __GNUC__
    __attribute__ ((format(printf, 3, 4)));
#else
    ;
#endif
#define fh_snprintf FH_SNPRINTF

/**
 * Prints a WARNING message.  On systems that don't differentiate between
 * warnings and regular log messages, just print it.  Indicates that something
 * may be wrong with the driver.  Works like printf().
 *
 * Use the FH_WARN macro to call this function.
 */
extern void __FH_WARN(char *format, ...)
#ifdef __GNUC__
    __attribute__ ((format(printf, 1, 2)));
#else
    ;
#endif

/**
 * Prints an error message.  On systems that don't differentiate between errors
 * and regular log messages, just print it.  Indicates that something went wrong
 * with the driver.  Works like printf().
 *
 * Use the FH_ERROR macro to call this function.
 */
extern void __FH_ERROR(char *format, ...)
#ifdef __GNUC__
    __attribute__ ((format(printf, 1, 2)));
#else
    ;
#endif

/**
 * Prints an exception error message and takes some user-defined action such as
 * print out a backtrace or trigger a breakpoint.  Indicates that something went
 * abnormally wrong with the driver such as programmer error, or other
 * exceptional condition.  It should not be ignored so even on systems without
 * printing capability, some action should be taken to notify the developer of
 * it.  Works like printf().
 */
extern void FH_EXCEPTION(char *format, ...)
#ifdef __GNUC__
    __attribute__ ((format(printf, 1, 2)));
#else
    ;
#endif
#define fh_exception FH_EXCEPTION

#ifdef DEBUG
/**
 * Prints out a debug message.  Used for logging/trace messages.
 *
 * Use the FH_DEBUG macro to call this function
 */
extern void __FH_DEBUG(char *format, ...)
#ifdef __GNUC__
    __attribute__ ((format(printf, 1, 2)));
#else
    ;
#endif
#else
#define __FH_DEBUG(...)
#endif

/**
 * Prints out a Debug message.
 */
#define FH_DEBUG(_format, _args...) __FH_DEBUG("DEBUG:%s:" _format "\n", \
                         __func__,  ## _args)
#define fh_debug FH_DEBUG
/**
 * Prints out an informative message.
 */
#define FH_INFO(_format, _args...) FH_PRINTF("INFO: " _format "\n", \
                            ## _args)
#define fh_info FH_INFO
/**
 * Prints out a warning message.
 */
#define FH_WARN(_format, _args...) __FH_WARN("WARN:%s:%d: " _format "\n", \
                    __func__, __LINE__, ## _args)
#define fh_warn FH_WARN
/**
 * Prints out an error message.
 */
#define FH_ERROR(_format, _args...) __FH_ERROR("ERROR:%s:%d: " _format "\n", \
                    __func__, __LINE__, ## _args)
#define fh_error FH_ERROR

#define FH_PROTO_ERROR(_format, _args...) __FH_WARN("ERROR:%s:%d: " _format "\n", \
                         __func__, __LINE__, ## _args)
#define fh_proto_error FH_PROTO_ERROR

#ifdef DEBUG
/** Prints out a exception error message if the _expr expression fails.  Disabled
 * if DEBUG is not enabled. */
#define FH_ASSERT(_expr, _format, _args...) do { \
    if (!(_expr)) { FH_EXCEPTION("%s:%d: " _format "\n",  \
                      __FILE__, __LINE__, ## _args); } \
    } while (0)
#else
#define FH_ASSERT(_x...)
#endif
#define fh_assert FH_ASSERT


/** @name Register Read/Write
 *
 * The following six functions should be implemented to read/write registers of
 * 32-bit and 64-bit sizes.  All modules use this to read/write register values.
 * The reg value is a pointer to the register calculated from the void *base
 * variable passed into the driver when it is started.  */

/* Linux doesn't need any extra parameters for register read/write, so we
 * just throw away the IO context parameter.
 */
/** Reads the content of a 32-bit register. */
extern uint32_t FH_READ_REG32(uint32_t volatile * reg);
#define fh_read_reg32(_ctx_, _reg_) FH_READ_REG32(_reg_)

/** Reads the content of a 64-bit register. */
extern uint64_t FH_READ_REG64(uint64_t volatile * reg);
#define fh_read_reg64(_ctx_, _reg_) FH_READ_REG64(_reg_)

/** Writes to a 32-bit register. */
extern void FH_WRITE_REG32(uint32_t volatile * reg, uint32_t value);
#define fh_write_reg32(_ctx_, _reg_, _val_) FH_WRITE_REG32(_reg_, _val_)

/** Writes to a 64-bit register. */
extern void FH_WRITE_REG64(uint64_t volatile * reg, uint64_t value);
#define fh_write_reg64(_ctx_, _reg_, _val_) FH_WRITE_REG64(_reg_, _val_)

/**
 * Modify bit values in a register.  Using the
 * algorithm: (reg_contents & ~clear_mask) | set_mask.
 */
extern void FH_MODIFY_REG32(uint32_t volatile * reg, uint32_t clear_mask, uint32_t set_mask);
#define fh_modify_reg32(_ctx_, _reg_, _cmsk_, _smsk_) FH_MODIFY_REG32(_reg_, _cmsk_, _smsk_)
extern void FH_MODIFY_REG64(uint64_t volatile * reg, uint64_t clear_mask, uint64_t set_mask);
#define fh_modify_reg64(_ctx_, _reg_, _cmsk_, _smsk_) FH_MODIFY_REG64(_reg_, _cmsk_, _smsk_)



/** @cond */

/** @name Some convenience MACROS used internally.  Define FH_DEBUG_REGS to log the
 * register writes. */

# ifdef FH_DEBUG_REGS

#define fh_define_read_write_reg_n(_reg, _container_type) \
static inline uint32_t fh_read_##_reg##_n(_container_type *container, int num) { \
    return FH_READ_REG32(&container->regs->_reg[num]); \
} \
static inline void fh_write_##_reg##_n(_container_type *container, int num, uint32_t data) { \
    FH_DEBUG("WRITING %8s[%d]: %p: %08x", #_reg, num, \
          &(((uint32_t*)container->regs->_reg)[num]), data); \
    FH_WRITE_REG32(&(((uint32_t*)container->regs->_reg)[num]), data); \
}

#define fh_define_read_write_reg(_reg,_container_type) \
static inline uint32_t fh_read_##_reg(_container_type *container) { \
    return FH_READ_REG32(&container->regs->_reg); \
} \
static inline void fh_write_##_reg(_container_type *container, uint32_t data) { \
    FH_DEBUG("WRITING %11s: %p: %08x", #_reg, &container->regs->_reg, data); \
    FH_WRITE_REG32(&container->regs->_reg, data); \
}

# else    /* FH_DEBUG_REGS */

#define fh_define_read_write_reg_n(_reg,_container_type) \
static inline uint32_t fh_read_##_reg##_n(_container_type *container, int num) { \
    return FH_READ_REG32(&container->regs->_reg[num]); \
} \
static inline void fh_write_##_reg##_n(_container_type *container, int num, uint32_t data) { \
    FH_WRITE_REG32(&(((uint32_t*)container->regs->_reg)[num]), data); \
}

#define fh_define_read_write_reg(_reg,_container_type) \
static inline uint32_t fh_read_##_reg(_container_type *container) { \
    return FH_READ_REG32(&container->regs->_reg); \
} \
static inline void fh_write_##_reg(_container_type *container, uint32_t data) { \
    FH_WRITE_REG32(&container->regs->_reg, data); \
}

# endif    /* FH_DEBUG_REGS */


/** @endcond */



/** @name Memory Allocation
 *
 * These function provide access to memory allocation.  There are only 2 DMA
 * functions and 3 Regular memory functions that need to be implemented.  None
 * of the memory debugging routines need to be implemented.  The allocation
 * routines all ZERO the contents of the memory.
 *
 * Defining FH_DEBUG_MEMORY turns on memory debugging and statistic gathering.
 * This checks for memory leaks, keeping track of alloc/free pairs.  It also
 * keeps track of how much memory the driver is using at any given time. */

#define FH_PAGE_SIZE 4096
#define FH_PAGE_OFFSET(addr) (((uint32_t)addr) & 0xfff)
#define FH_PAGE_ALIGNED(addr) ((((uint32_t)addr) & 0xfff) == 0)

#define FH_INVALID_DMA_ADDR 0x0

/** Type for a DMA address */
typedef uint32_t dma_addr_t;
typedef dma_addr_t fh_dma_t;


/** Allocates a DMA capable buffer and zeroes its contents. */
extern void *__FH_DMA_ALLOC(void *dma_ctx, uint32_t size, fh_dma_t *dma_addr);

/** Allocates a DMA capable buffer and zeroes its contents in atomic contest */
extern void *__FH_DMA_ALLOC_ATOMIC(void *dma_ctx, uint32_t size, fh_dma_t *dma_addr);

/** Frees a previously allocated buffer. */
extern void __FH_DMA_FREE(void *dma_ctx, uint32_t size, void *virt_addr, fh_dma_t dma_addr);

/** Allocates a block of memory and zeroes its contents. */
extern void *__FH_ALLOC(void *mem_ctx, uint32_t size);

/** Allocates a block of memory and zeroes its contents, in an atomic manner
 * which can be used inside interrupt context.  The size should be sufficiently
 * small, a few KB at most, such that failures are not likely to occur.  Can just call
 * __FH_ALLOC if it is atomic. */
extern void *__FH_ALLOC_ATOMIC(void *mem_ctx, uint32_t size);

/** Frees a previously allocated buffer. */
extern void __FH_FREE(void *mem_ctx, void *addr);

#ifndef FH_DEBUG_MEMORY

#define FH_ALLOC(_size_) __FH_ALLOC(NULL, _size_)
#define FH_ALLOC_ATOMIC(_size_) __FH_ALLOC_ATOMIC(NULL, _size_)
#define FH_FREE(_addr_) __FH_FREE(NULL, _addr_)

#define FH_DMA_ALLOC(_size_, _dma_) __FH_DMA_ALLOC(NULL, _size_, _dma_)
#define FH_DMA_ALLOC_ATOMIC(_size_, _dma_) __FH_DMA_ALLOC_ATOMIC(NULL, _size_, _dma_)
#define FH_DMA_FREE(_size_, _virt_, _dma_) __FH_DMA_FREE(NULL, _size_, _virt_, _dma_)

#endif /* FH_DEBUG_MEMORY */

#define fh_alloc(_ctx_, _size_) FH_ALLOC(_size_)
#define fh_alloc_atomic(_ctx_, _size_) FH_ALLOC_ATOMIC(_size_)
#define fh_free(_ctx_, _addr_) FH_FREE(_addr_)

/* Linux doesn't need any extra parameters for DMA buffer allocation, so we
 * just throw away the DMA context parameter.
 */
#define fh_dma_alloc(_ctx_, _size_, _dma_) FH_DMA_ALLOC(_size_, _dma_)
#define fh_dma_alloc_atomic(_ctx_, _size_, _dma_) FH_DMA_ALLOC_ATOMIC(_size_, _dma_)
#define fh_dma_free(_ctx_, _size_, _virt_, _dma_) FH_DMA_FREE(_size_, _virt_, _dma_)

/** @name Memory and String Processing */

/** memset() clone */
extern void *FH_MEMSET(void *dest, uint8_t byte, uint32_t size);
#define fh_memset FH_MEMSET

/** memcpy() clone */
extern void *FH_MEMCPY(void *dest, void const *src, uint32_t size);
#define fh_memcpy FH_MEMCPY

/** memmove() clone */
extern void *FH_MEMMOVE(void *dest, void *src, uint32_t size);
#define fh_memmove FH_MEMMOVE

/** memcmp() clone */
extern int FH_MEMCMP(void *m1, void *m2, uint32_t size);
#define fh_memcmp FH_MEMCMP

/** strcmp() clone */
extern int FH_STRCMP(void *s1, void *s2);
#define fh_strcmp FH_STRCMP

/** strncmp() clone */
extern int FH_STRNCMP(void *s1, void *s2, uint32_t size);
#define fh_strncmp FH_STRNCMP

/** strlen() clone, for NULL terminated ASCII strings */
extern int FH_STRLEN(char const *str);
#define fh_strlen FH_STRLEN

/** strdup() clone.  If you wish to use memory allocation debugging, this
 * implementation of strdup should use the FH_* memory routines instead of
 * calling a predefined strdup.  Otherwise the memory allocated by this routine
 * will not be seen by the debugging routines. */
extern char *FH_STRDUP(char const *str);
#define fh_strdup(_ctx_, _str_) FH_STRDUP(_str_)

/** NOT an atoi() clone.  Read the description carefully.  Returns an integer
 * converted from the string str in base 10 unless the string begins with a "0x"
 * in which case it is base 16.  String must be a NULL terminated sequence of
 * ASCII characters and may optionally begin with whitespace, a + or -, and a
 * "0x" prefix if base 16.  The remaining characters must be valid digits for
 * the number and end with a NULL character.  If any invalid characters are
 * encountered or it returns with a negative error code and the results of the
 * conversion are undefined.  On sucess it returns 0.  Overflow conditions are
 * undefined.  An example implementation using atoi() can be referenced from the
 * Linux implementation. */


/** Type for the 'flags' argument to spinlock funtions */
typedef unsigned long fh_irqflags_t;

/** Returns an initialized lock variable.  This function should allocate and
 * initialize the OS-specific data structure used for locking.  This data
 * structure is to be used for the FH_LOCK and FH_UNLOCK functions and should
 * be freed by the FH_FREE_LOCK when it is no longer used. */


/** @name Timer
 *
 * Callbacks must be small and atomic.
 */
struct fh_timer;

/** Type for a timer */
typedef struct fh_timer fh_timer_t;

/** The type of the callback function to be called */
typedef void (*fh_timer_callback_t)(void *data);

/** Allocates a timer */
extern fh_timer_t *FH_TIMER_ALLOC(char *name, fh_timer_callback_t cb, void *data);
#define fh_timer_alloc(_ctx_, _name_, _cb_, _data_) FH_TIMER_ALLOC(_name_, _cb_, _data_)

/** Frees a timer */
extern void FH_TIMER_FREE(fh_timer_t *timer);
#define fh_timer_free FH_TIMER_FREE

/** Schedules the timer to run at time ms from now.  And will repeat at every
 * repeat_interval msec therafter
 *
 * Modifies a timer that is still awaiting execution to a new expiration time.
 * The mod_time is added to the old time.  */
extern void FH_TIMER_SCHEDULE(fh_timer_t *timer, uint32_t time);
#define fh_timer_schedule FH_TIMER_SCHEDULE

/** Disables the timer from execution. */
extern void FH_TIMER_CANCEL(fh_timer_t *timer);
#define fh_timer_cancel FH_TIMER_CANCEL


#if 0
/** @name Spinlocks
 *
 * These locks are used when the work between the lock/unlock is atomic and
 * short.  Interrupts are also disabled during the lock/unlock and thus they are
 * suitable to lock between interrupt/non-interrupt context.  They also lock
 * between processes if you have multiple CPUs or Preemption.  If you don't have
 * multiple CPUS or Preemption, then the you can simply implement the
 * FH_SPINLOCK and FH_SPINUNLOCK to disable and enable interrupts.  Because
 * the work between the lock/unlock is atomic, the process context will never
 * change, and so you never have to lock between processes.  */

struct fh_spinlock;

/** Type for a spinlock */
typedef struct fh_spinlock fh_spinlock_t;

/** Type for the 'flags' argument to spinlock funtions */
typedef unsigned long fh_irqflags_t;

/** Returns an initialized lock variable.  This function should allocate and
 * initialize the OS-specific data structure used for locking.  This data
 * structure is to be used for the FH_LOCK and FH_UNLOCK functions and should
 * be freed by the FH_FREE_LOCK when it is no longer used. */
extern fh_spinlock_t *FH_SPINLOCK_ALLOC(void);
#define fh_spinlock_alloc(_ctx_) FH_SPINLOCK_ALLOC()

/** Frees an initialized lock variable. */
extern void FH_SPINLOCK_FREE(fh_spinlock_t *lock);
#define fh_spinlock_free(_ctx_, _lock_) FH_SPINLOCK_FREE(_lock_)

/** Disables interrupts and blocks until it acquires the lock.
 *
 * @param lock Pointer to the spinlock.
 * @param flags Unsigned long for irq flags storage.
 */
extern void FH_SPINLOCK_IRQSAVE(fh_spinlock_t *lock, fh_irqflags_t *flags);
#define fh_spinlock_irqsave FH_SPINLOCK_IRQSAVE

/** Re-enables the interrupt and releases the lock.
 *
 * @param lock Pointer to the spinlock.
 * @param flags Unsigned long for irq flags storage.  Must be the same as was
 * passed into FH_LOCK.
 */
extern void FH_SPINUNLOCK_IRQRESTORE(fh_spinlock_t *lock, fh_irqflags_t flags);
#define fh_spinunlock_irqrestore FH_SPINUNLOCK_IRQRESTORE

/** Blocks until it acquires the lock.
 *
 * @param lock Pointer to the spinlock.
 */
extern void FH_SPINLOCK(fh_spinlock_t *lock);
#define fh_spinlock FH_SPINLOCK

/** Releases the lock.
 *
 * @param lock Pointer to the spinlock.
 */
extern void FH_SPINUNLOCK(fh_spinlock_t *lock);
#define fh_spinunlock FH_SPINUNLOCK


/** @name Mutexes
 *
 * Unlike spinlocks Mutexes lock only between processes and the work between the
 * lock/unlock CAN block, therefore it CANNOT be called from interrupt context.
 */

struct fh_mutex;

/** Type for a mutex */
typedef struct fh_mutex fh_mutex_t;

/* For Linux Mutex Debugging make it inline because the debugging routines use
 * the symbol to determine recursive locking.  This makes it falsely think
 * recursive locking occurs. */
#if defined(FH_LINUX) && defined(CONFIG_DEBUG_MUTEXES)
#define FH_MUTEX_ALLOC_LINUX_DEBUG(__mutexp) ({ \
    __mutexp = (fh_mutex_t *)FH_ALLOC(sizeof(struct mutex)); \
    mutex_init((struct mutex *)__mutexp); \
})
#endif

/** Allocate a mutex */
extern fh_mutex_t *FH_MUTEX_ALLOC(void);
#define fh_mutex_alloc(_ctx_) FH_MUTEX_ALLOC()

/* For memory leak debugging when using Linux Mutex Debugging */
#if defined(FH_LINUX) && defined(CONFIG_DEBUG_MUTEXES)
#define FH_MUTEX_FREE(__mutexp) do { \
    mutex_destroy((struct mutex *)__mutexp); \
    FH_FREE(__mutexp); \
} while (0)
#else
/** Free a mutex */
extern void FH_MUTEX_FREE(fh_mutex_t *mutex);
#define fh_mutex_free(_ctx_, _mutex_) FH_MUTEX_FREE(_mutex_)
#endif

/** Lock a mutex */
extern void FH_MUTEX_LOCK(fh_mutex_t *mutex);
#define fh_mutex_lock FH_MUTEX_LOCK

/** Non-blocking lock returns 1 on successful lock. */
extern int FH_MUTEX_TRYLOCK(fh_mutex_t *mutex);
#define fh_mutex_trylock FH_MUTEX_TRYLOCK

/** Unlock a mutex */
extern void FH_MUTEX_UNLOCK(fh_mutex_t *mutex);
#define fh_mutex_unlock FH_MUTEX_UNLOCK

#endif

#ifdef __cplusplus
}
#endif

#endif /* _FH_OS_H_ */
