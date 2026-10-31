
/* #include <common.h> */
/* #include <malloc.h> */
/* #include <memalign.h> */
#include <linux/compat.h>
#include "port_rt.h"
#define ARCH_DMA_MINALIGN 4

unsigned long copy_from_user(void *dest, const void *src,
                             unsigned long count)
{
    memcpy((void *)dest, (void *)src, count);
    return 0;
}

struct kmem_cache *get_mem(int element_sz)
{
    struct kmem_cache *ret;

    ret = kmalloc_align(sizeof(struct kmem_cache), ARCH_DMA_MINALIGN); /* memalign(ARCH_DMA_MINALIGN, sizeof(struct kmem_cache)); */
    ret->sz = element_sz;

    return ret;
}

void *kmem_cache_alloc(struct kmem_cache *obj, int flag)
{
    return kmalloc_align(obj->sz, ARCH_DMA_MINALIGN); /* malloc_cache_aligned(obj->sz); */
}

/**
 * kmemdup - duplicate region of memory
 *
 * @src: memory region to duplicate
 * @len: memory region length
 * @gfp: GFP mask to use
 *
 * Return: newly allocated copy of @src or %NULL in case of error
 */
void *kmemdup(const void *src, size_t len, gfp_t gfp)
{
    void *p;

    p = kmalloc(len, gfp);
    if (p)
        memcpy(p, src, len);
    return p;
}
