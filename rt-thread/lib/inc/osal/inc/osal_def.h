#ifndef FH_FH_OSAL_DEF_H
#define FH_FH_OSAL_DEF_H


#if defined(__linux__)

#else
// rtos 
#define __user  
#define loff_t unsigned long long
#define fmode_t unsigned
#define bool int
#define u64 unsigned long long
#define phys_addr_t unsigned long
#define __must_check 
#define __force
#define IS_ERR(err) ((unsigned long)err > (unsigned long)-1000L)

#if defined(CONFIG_CHIP_ARC)
typedef unsigned long   size_t;
typedef int ssize_t;
typedef int mode_t;
typedef rt_int32_t pid_t;

#ifndef NULL
#define NULL    (0)
#endif

#endif

#endif

#endif
