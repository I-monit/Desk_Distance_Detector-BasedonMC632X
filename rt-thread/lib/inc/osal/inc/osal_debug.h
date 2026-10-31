#ifndef _FH_OSAL_DEBUG_H
#define _FH_OSAL_DEBUG_H

//#define OSAL_MALLOC 1

#ifdef OSAL_MALLOC
void print_osal_dbg_info(void);
void osal_dbg_init(void);
void osal_dbg_insert_alloc_info(void* allocptr,unsigned long size);
void osal_dbg_del_alloc_info(void* allocptr);
#endif

#endif
