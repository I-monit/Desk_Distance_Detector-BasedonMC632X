
#ifndef __UBIPORT_COMMON_H__
#define __UBIPORT_COMMON_H__
#include <stdio.h>
/* #include <rtdef.h> */
extern void rt_kprintf(const char *fmt, ...);
/* void ubi_printf(const char *fmt, ...); */
#define UNUSED(x) (void)(x)

#ifndef UBIFS_MAX_PATH_LENGTH
#define UBIFS_MAX_PATH_LENGTH 256
#endif

/* #define UBIPORT_DEBUG */
#ifdef UBIPORT_DEBUG
#define ubiPort_Log1(x, ...)\
do\
{\
    rt_kprintf(x, ##__VA_ARGS__);\
}while (0)
#define ubiPort_Log(x, ...)\
do\
{\
    rt_kprintf("\033[42;31m""port [%s]:" x "\033[40;37m",__FUNCTION__, ##__VA_ARGS__);\
}while (0)
#else
#define ubiPort_Log(x, ...)
#define ubiPort_Log1(x, ...)
#endif

#define ubiPort_Err(x, ...)\
do\
{\
    rt_kprintf("\033[22;31m""err:[%s]:" x "\033[40;37m",__FUNCTION__, ##__VA_ARGS__);\
}while (0)

#endif


