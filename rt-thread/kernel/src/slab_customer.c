
/* for lwip */
void *lwip_malloc(rt_size_t size)
{
    return fhkmalloc(FHMEM_TYPE_NET, size, MM_ALIGN_4);
}

void lwip_free(void *ptr)
{
    fhkfree(FHMEM_TYPE_NET, ptr);
}

void *lwip_calloc(rt_size_t count, rt_size_t size)
{
    void *p;
    rt_uint64_t len = count * size;

    if (len > 1*1024*1024*1024)
        return RT_NULL;

    size = len;
    p = lwip_malloc(size);
    if (p)
        rt_memset(p, 0, size);

    return p;
}

/* for jffs2 */
void *jffs2_malloc(rt_size_t size)
{
    return fhkmalloc(FHMEM_TYPE_JFFS2, size, MM_ALIGN_4);
}

void jffs2_free(void *ptr)
{
    fhkfree(FHMEM_TYPE_JFFS2, ptr);
}

/* for app */
void *app_malloc(rt_size_t size)
{
    return fhkmalloc(FHMEM_TYPE_APP, size, MM_ALIGN_4);
}

void app_free(void *ptr)
{
    fhkfree(FHMEM_TYPE_APP, ptr);
}

void *app_memalign(rt_size_t boundary, rt_size_t size)
{
    rt_uint32_t align_bits;

    if (boundary <= 2)
        boundary = 4;

    for (align_bits = 0; align_bits <= MM_PAGE_BITS; align_bits++)
    {
        if (boundary == (1 << align_bits))
        {
            return fhkmalloc(FHMEM_TYPE_APP, size, align_bits);
        }
    }

    BUG(boundary);
    return RT_NULL;
}

void *app_realloc(void *ptr, rt_size_t size)
{
    void *nptr;

    if (!ptr)
    {
        return app_malloc(size);
    }

    if (!size)
    {
        app_free(ptr);
        return RT_NULL;
    }

    nptr = app_malloc(size);
    if (nptr)
    {
        rt_memcpy(nptr, ptr, size);
        app_free(ptr);
    }

    return nptr;
}

void *app_calloc(rt_size_t count, rt_size_t size)
{
    void *p;
    rt_uint64_t len = count * size;

    if (len > 1*1024*1024*1024)
        return RT_NULL;

    size = len;
    p = app_malloc(size);
    if (p)
        rt_memset(p, 0, size);

    return p;
}
