#ifndef _FH_OSAL_CACHE_H
#define _FH_OSAL_CACHE_H

void fh_osal_flush_kern_all(void);
void fh_osal_flush_user_all(void);
void fh_osal_flush_user_range(unsigned long start, unsigned long end, unsigned int flags);
void fh_osal_coherent_kern_range(unsigned long start, unsigned long end);
void fh_osal_flush_dcache_area(void *addr, size_t len);
void fh_osal_invalidate_dcache_area(void *addr, size_t len);
void fh_osal_outer_flush_all(void);

#endif
