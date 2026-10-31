#ifndef _FH_OSAL_MALLOC_H
#define _FH_OSAL_MALLOC_H


#define FH_OSAL_GFP_KERNEL 0
#define FH_OSAL_GFP_ATOMIC 1
void *fh_osal_vmalloc(unsigned long size);
void  fh_osal_vfree(const void *addr);
void *fh_osal_kmalloc(unsigned long size, unsigned int fh_osal_gfp_flag);
void *fh_osal_kzalloc(unsigned long size, unsigned int fh_osal_gfp_flag);
void *fh_osal_krealloc(const void *p, size_t new_size, unsigned int fh_osal_gfp_flag);
void  fh_osal_kfree(const void * addr);

#endif