/**
 * Copyright (c) 2015-2019 Shanghai Fullhan Microelectronics Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 *
 * Change Logs:
 * Date           Author       Notes
 * 2019-01-24     tangyh    the first version
 *
 */

#include "dma_mem.h"

#include <rthw.h>

#define DEFAULT_MAOLLOC_NAME  "NONAME"
#define DCACHE_LINE_SIZE 64

#define DMA_MEM_DECLARE_LOCK() rt_ubase_t level
#define DMA_MEM_LOCK() level = rt_hw_interrupt_disable()
#define DMA_MEM_UNLOCK() rt_hw_interrupt_enable(level)


#define DMA_MEM_MAX_NAME_LEN        16

struct dma_mem_control
{
    /*here must be the first...*/
    rt_list_t dma_mem_node_list;

    char name[DMA_MEM_MAX_NAME_LEN];

    rt_uint32_t align;
    rt_uint32_t begin_addr;
    rt_uint32_t end_addr;   /*include end_addr itself*/
    rt_uint32_t alloc_addr; /*current alloc address*/
};

static struct dma_mem_control dma_mem_manager;

struct dma_mem_node
{
    /*here must be the first...*/
    rt_list_t list;

    char name[DMA_MEM_MAX_NAME_LEN];
    rt_uint32_t base_addr;
    rt_uint32_t size;
    rt_uint32_t align;
};

#define dma_mem_list_for_each_safe(pos, n, head) \
    for (pos = (head)->next, n = pos->next; pos != (head); \
        pos = n, n = pos->next)

#define dma_mem_list_for_each(pos, head) \
    for (pos = (head)->next; pos != (head); pos = pos->next)

static int align_check(rt_uint32_t align)
{
    rt_uint32_t bits = 0;

    if ((align >> 28) != 0) /*align cann't be too large*/
    {
        return 0;
    }

    while (align != 0)
    {
        if ((align & 1) != 0)
            bits++;
        align >>= 1;
    }

    if (bits == 1)
        return 1;

    return 0;
}

static int dma_mem_init(
        struct dma_mem_control *p_cmm,
        char *name,
        rt_uint32_t beginAddr,
        rt_uint32_t bufSize,
        rt_uint32_t bufAllign)
{
    rt_uint32_t maxsz;

    DMA_MEM_DECLARE_LOCK();

    if (bufAllign < DCACHE_LINE_SIZE)
        bufAllign = DCACHE_LINE_SIZE;

    /*maxsz = ~0 - beginAddr + 1;*/
    maxsz = -beginAddr;
    if (bufSize == 0 ||
            bufSize >= maxsz || /*don't let end_addr reach 0xffffffff*/
            !align_check(bufAllign))
    {
        return -1;
    }

    DMA_MEM_LOCK();

    rt_list_init(&p_cmm->dma_mem_node_list);
    rt_strncpy(p_cmm->name, name, DMA_MEM_MAX_NAME_LEN);
    p_cmm->align      = bufAllign;
    p_cmm->begin_addr = beginAddr;
    p_cmm->end_addr   = beginAddr + (bufSize - 1);
    p_cmm->alloc_addr = beginAddr;

    DMA_MEM_UNLOCK();

    return 0;
}


static void *dma_mem_malloc_with_name(
        struct dma_mem_control *p_cmm,
        rt_uint32_t size,
        rt_uint32_t align,
        char *name)
{
    rt_uint32_t extra;
    struct dma_mem_node *node;

    DMA_MEM_DECLARE_LOCK();

    if (align < p_cmm->align)
        align = p_cmm->align;

    node = rt_malloc(sizeof(struct dma_mem_node));
    if (!node)
    {
        return RT_NULL;
    }

    DMA_MEM_LOCK();

    extra = (align - (p_cmm->alloc_addr & (align - 1))) & (align - 1);
    if (!align_check(align) ||
            !p_cmm->alloc_addr ||
            (int)size <= 0 ||
            size + extra > p_cmm->end_addr + 1 - p_cmm->alloc_addr)
    {
        goto _unlock;
    }

    if (!name)
        name = DEFAULT_MAOLLOC_NAME;

    rt_strncpy(node->name, name, DMA_MEM_MAX_NAME_LEN);
    node->base_addr = p_cmm->alloc_addr + extra;
    node->size = size;
    node->align = align;
    rt_list_insert_before(&p_cmm->dma_mem_node_list, &node->list);

    p_cmm->alloc_addr += (extra + size);

    DMA_MEM_UNLOCK();
    return (void *)node->base_addr;

_unlock:
    DMA_MEM_UNLOCK();
    rt_free(node);
    return RT_NULL;
}

static void dma_mem_dump_all_info(struct dma_mem_control *p_cmm)
{
    rt_uint32_t sz = 0;
    rt_uint32_t alloced;
    rt_uint32_t total;
    struct dma_mem_node *node;
    rt_list_t *pos;
    rt_list_t *head = &p_cmm->dma_mem_node_list;

    if (!p_cmm->begin_addr)
        return;

    rt_kprintf("malloc info:\n");

    dma_mem_list_for_each(pos, head)
    {
        node = (struct dma_mem_node *)pos;
        rt_kprintf("name:[%s]\t", node->name);
        rt_kprintf("begin add:0x%x\t", node->base_addr);
        rt_kprintf("align:%d\t\t", node->align);
        rt_kprintf("size:0x%x\n", node->size);
        sz += node->size;
    }

    alloced  = p_cmm->alloc_addr - p_cmm->begin_addr;
    total = p_cmm->end_addr - p_cmm->begin_addr + 1;

    rt_kprintf("\n");
    rt_kprintf("------------------------------------\n");
    rt_kprintf("dma mem used size:%d(KB)\t", sz/1024);
    rt_kprintf("dma mem alloced size:%d(KB)\t", alloced/1024);
    rt_kprintf("dma mem total size:%d(KB)\n", total/1024);
}


static int dma_handle = 0;

/* function body */
/*****************************************************************************
 * Description:
 *      add funtion description here
 * Parameters:
 *      description for each argument, new argument starts at new line
 * Return:
 *      what does this function returned?
 *****************************************************************************/
rt_err_t fh_dma_mem_init(rt_uint32_t *mem_start, rt_uint32_t size)
{
    int ret = -1;
    if (dma_handle == 0)
    {
        ret = dma_mem_init(&dma_mem_manager, "dma mem", (unsigned int)mem_start, size, DCACHE_LINE_SIZE);

        if (ret == 0)
            dma_handle = 1;
    }
    return ret;
}

void *fh_dma_mem_malloc_align(rt_uint32_t size, rt_uint32_t align, char *name)
{
    if (!dma_handle)
        return RT_NULL;
    return dma_mem_malloc_with_name(&dma_mem_manager, size, align, name);
}

void *fh_dma_mem_malloc(rt_uint32_t size)
{
    return fh_dma_mem_malloc_align(size, 4, "dma_heap");
    /* return rt_memheap_alloc(&dma_heap, size); */
}

void fh_dma_mem_free(void *ptr)
{
    /* rt_memheap_free(ptr); */
    /* fixme: use uinit to free all; */
    rt_kprintf("%s: Not implimentation.\n", __func__);
}

void fh_dma_mem_dump(void)
{
    dma_mem_dump_all_info(&dma_mem_manager);
}

void fh_dma_mem_uninit(void) { rt_kprintf("%s: Not implimentation.\n", __func__); }
#ifdef FH_TEST_DMA_MEM
int dma_mem_debug(void *ptr)
{
    /* rt_memheap_free(ptr); */
    rt_kprintf("dma mem start 0x%08x\n", (rt_uint32_t)dma_heap.start_addr);
    rt_kprintf("dma mem total size 0x%08x\n", dma_heap.pool_size);
    rt_kprintf("dma mem left size 0x%08x\n", dma_heap.available_size);
    rt_kprintf("dma mem max use size 0x%08x\n", dma_heap.max_used_size);
    return 0;
}
#endif

#ifdef RT_USING_FINSH
#include <finsh.h>
#ifdef FH_TEST_DMA_MEM
FINSH_FUNCTION_EXPORT(dma_mem_debug, dma_start & left size & max_use);
#endif
#endif

