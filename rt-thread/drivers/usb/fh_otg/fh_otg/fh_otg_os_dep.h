#ifndef __FH_OTG_OS_DEP_H__
#define __FH_OTG_OS_DEP_H__


/* #include <usb.h> */
#include <hcd.h>
#include <ch9.h>
#include <rtdef.h>
#include <rtthread.h>
#include <rtdevice.h>
#include <rthw.h>
#include "usb_errno.h"
#include <dma_mem.h>
#include <interrupt.h>
#include <sys/types.h>
#include "../fh_common_port/fh_os.h"

/**
 * @file
 *
 * This file contains OS dependent structures.
 *
 */
typedef signed char s8;
typedef unsigned char u8;

typedef signed short s16;
typedef unsigned short u16;

typedef signed int s32;
typedef unsigned int u32;

typedef signed long long s64;
typedef unsigned long long u64;

/** The OS page size */
#define FH_OS_PAGE_SIZE    PAGE_SIZE

typedef struct os_dependent
{
    /** Base address returned from ioremap() */
    void *base;

    /** Register offset for Diagnostic API */
    uint32_t reg_offset;
} os_dependent_t;

extern void usb_release_mem(void *mem);
extern void *usb_get_freemem();

#endif /* _FH_OS_DEP_H_ */
