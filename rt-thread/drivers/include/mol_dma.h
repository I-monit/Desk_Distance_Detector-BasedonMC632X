/**
 * Copyright (c) 2015-2019 Shanghai Fullhan Microelectronics Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 *
 * Change Logs:
 * Date           Author       Notes
 * 2019-01-31     wangyl307    add license Apache-2.0
 */

#ifndef __MOL_DMA_H__
#define __MOL_DMA_H__

#include <rtthread.h>
#include "fh_arch.h"
#define DMA_CONTROLLER_NUMBER   1
typedef struct rt_list_node             mc_dma_list;
#define mc_dma_list_init(n)             rt_list_init(n)
#define mc_dma_list_insert_before(to, new)  rt_list_insert_before(new, to)
#define mc_dma_list_remove(n)           rt_list_remove(n)
#define mc_dma_scanf                    rt_sprintf

#define SCA_GAT_EN      (0x55)
#ifndef MIN
#define MIN(a, b) (a < b ? a : b)
#endif
#ifndef MAX
#define MAX(a, b) (a > b ? a : b)
#endif

typedef struct rt_semaphore     mc_dma_lock_t;
#define mc_dma_lock_init(p, name)   rt_sem_init(p, name, 1, RT_IPC_FLAG_FIFO)
#define mc_dma_lock(p, t)   rt_sem_take(p, t)
#define mc_dma_unlock(p)    rt_sem_release(p)
#define mc_dma_trylock(p)   rt_sem_trytake(p)
#define mc_dma_memset(s, c, cnt)    rt_memset(s, c, cnt)
#define mc_dma_malloc(s)    rt_malloc(s)
#define mc_dma_free(s)      rt_free(s)
#define mc_dma_min(d0, d1)  MIN(d0, d1)
#define MC_DMA_TICK_PER_SEC 100


static inline unsigned int  mc_dma_malloc_desc(void *pri, rt_int32_t size, unsigned int *phy_back)
{
    *phy_back = (unsigned int)rt_malloc(size);
    return *phy_back;
}

static inline void
mc_dma_free_desc(void *pri, rt_int32_t size, void *vir, rt_int32_t phy_add)
{
    rt_free(vir);
}
int fh_dma_init(void);

#endif
