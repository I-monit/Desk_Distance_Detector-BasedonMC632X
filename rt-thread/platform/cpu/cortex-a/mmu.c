/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2012-01-10     bernard      porting to AM1808
 */

#include <rtthread.h>
#include <rthw.h>
#include "mmu.h"

#include "cp15.h"


/* dump 2nd level page table */

/* level1 page table */
static volatile unsigned int _pgd_table[4*1024] ALIGN(16*1024);
/*
 * level2 page table
 * RT_MMU_PTE_SIZE must be 1024*n
 */
#ifndef RT_USING_SMP
#define RT_MMU_PTE_SIZE  4096
static volatile unsigned int _pte_table[RT_MMU_PTE_SIZE] ALIGN(1*1024);
#endif
void mmu_setttbase(register unsigned int i)
{
    /* Invalidates all TLBs.Domain access is selected as
     * client by configuring domain access register,
     * in that case access controlled by permission value
     * set by page table entry
     */

    asm ("mcr p15, 0, %0, c2, c0, 0"::"r"(i));
    asm ("dmb");
}

void mmu_create_pgd(struct mem_desc *mdesc)
{
    volatile unsigned int *pTT;
    volatile int i, nSec;
    pTT = (unsigned int *)_pgd_table + (mdesc->vaddr_start >> 20);
    nSec = (mdesc->vaddr_end >> 20) - (mdesc->vaddr_start >> 20);
    for (i = 0; i <= nSec; i++)
    {
        *pTT = mdesc->sect_attr | (((mdesc->paddr_start >> 20) + i) << 20);
        pTT++;
    }
}

void mmu_create_pte(struct mem_desc *mdesc)
{
    volatile unsigned int *pTT;
    volatile unsigned int *p_pteentry;
    int i;
    unsigned int vaddr;
    unsigned int total_page = 0;
    unsigned int pte_offset = 0;
    unsigned int sect_attr = 0;

    total_page = (mdesc->vaddr_end >> 12) - (mdesc->vaddr_start >> 12) + 1;
    pte_offset = mdesc->sect_attr & 0xfffffc00;
    sect_attr = mdesc->sect_attr & 0x3ff;
    vaddr = mdesc->vaddr_start;

    for (i = 0; i < total_page; i++)
    {
        pTT = (unsigned int *)_pgd_table + (vaddr >> 20);
        if (*pTT == 0) /* Level 1 page table item not used, now update pgd item */
        {
            *pTT = pte_offset | sect_attr;
            p_pteentry = (unsigned int *)pte_offset +
                ((vaddr & 0x000ff000) >> 12);
            pte_offset += 1024;
        }
        else /* using old Level 1 page table item */
        {
            p_pteentry = (unsigned int *)(*pTT & 0xfffffc00) +
                ((vaddr & 0x000ff000) >> 12);
        }


        *p_pteentry = mdesc->page_attr | (((mdesc->paddr_start >> 12) + i) << 12);
        vaddr += 0x1000;
    }
}

#ifndef RT_USING_SMP
static void build_pte_mem_desc(struct mem_desc *mdesc, unsigned int size)
{
    unsigned int pte_offset = 0;
    unsigned int nsec = 0;
    /* set page table */
    for (; size > 0; size--)
    {
        if (mdesc->mapped_mode == PAGE_MAPPED)
        {
            nsec = (RT_ALIGN(mdesc->vaddr_end, 0x100000) - RT_ALIGN_DOWN(mdesc->vaddr_start, 0x100000)) >> 20;
            mdesc->sect_attr |= (((unsigned int)_pte_table)& 0xfffffc00) + pte_offset;
            pte_offset += nsec << 10;
        }
        if (pte_offset >= RT_MMU_PTE_SIZE)
        {
            while (1)
                ;
        }

        mdesc++;
    }
}
#endif

unsigned long rt_hw_set_domain_register(unsigned long domain_val)
{
    unsigned long old_domain;

    asm volatile ("mrc p15, 0, %0, c3, c0\n" : "=r" (old_domain));
    asm volatile ("mcr p15, 0, %0, c3, c0\n" : :"r" (domain_val) : "memory");

    return old_domain;
}

void rt_hw_init_mmu_table(struct mem_desc *mdesc, unsigned int size)
{
    rt_memset((void *)_pgd_table, 0, 16*1024);
#ifndef RT_USING_SMP
    rt_memset((void *)_pte_table, 0, RT_MMU_PTE_SIZE);
    build_pte_mem_desc(mdesc, size);
#endif
    for (; size > 0; size--)
    {
        if (mdesc->mapped_mode == SECT_MAPPED)
        {
            mmu_create_pgd(mdesc);
        }
#ifndef RT_USING_SMP
        else
        {
            mmu_create_pte(mdesc);
        }
#endif
        mdesc++;
    }
}

extern void startup_log(unsigned int val);
/* void rt_hw_mmu_init(struct mem_desc *mdesc, unsigned int size)   */
void rt_hw_mmu_init(void)
{
#ifdef RT_USING_SMP
    rt_cpu_dcache_clean_flush();
    rt_cpu_icache_flush();
#endif
    rt_hw_cpu_dcache_disable();
    rt_hw_cpu_icache_disable();
    rt_cpu_mmu_disable();

    /*rt_hw_cpu_dump_page_table(MMUTable);*/
    rt_hw_set_domain_register(0x55555555);

    mmu_setttbase((unsigned int)_pgd_table);

    rt_cpu_mmu_enable();

    rt_hw_cpu_icache_enable();
    rt_hw_cpu_dcache_enable();
}
void rt_hw_mmu_resume(void)
{

    rt_hw_set_domain_register(0x55555555);

    mmu_setttbase((unsigned int)_pgd_table);

    rt_cpu_mmu_enable();

    rt_hw_cpu_icache_enable();
    rt_hw_cpu_dcache_enable();
}


int addr_all_cached(void *addr1, void *addr2)
{
    if (((_pgd_table[(rt_uint32_t)addr1 >> 20] & 0xfffff) == SECT_RWX_CB) &&
        ((_pgd_table[(rt_uint32_t)addr2 >> 20] & 0xfffff) == SECT_RWX_CB))
    {
        return 1;
    }

    return 0;
}

void mmu_clean_dcache(rt_uint32_t buffer, rt_uint32_t size)
{
    unsigned int ptr;

    ptr = buffer & ~(CACHE_LINE_SIZE - 1);

    while (ptr < buffer + size)
    {
        asm ("mcr p15, 0, %0, c7, c10, 1": :"r" (ptr));
        asm ("dmb");
        ptr += CACHE_LINE_SIZE;
    }
}

void mmu_invalidate_dcache(rt_uint32_t buffer, rt_uint32_t size)
{
    unsigned int ptr;

    ptr = buffer & ~(CACHE_LINE_SIZE - 1);

    while (ptr < buffer + size)
    {
        asm ("mcr p15, 0, %0, c7, c6, 1": :"r" (ptr));
        asm ("dmb");
        ptr += CACHE_LINE_SIZE;
    }
}

void mmu_clean_invalidated_dcache(rt_uint32_t buffer, rt_uint32_t size)
{
    unsigned int ptr;

    ptr = buffer & ~(CACHE_LINE_SIZE - 1);
    while (ptr < buffer + size)
    {
        asm ("mcr p15, 0, %0, c7, c14, 1": :"r" (ptr));
        asm ("dmb");
        ptr += CACHE_LINE_SIZE;
    }
}

