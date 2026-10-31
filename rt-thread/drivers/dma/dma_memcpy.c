#include <rtdevice.h>
#include "fh_arch.h"
#include "board_info.h"
#include "dma.h"
#include "dma_mem.h"
#include "mmu.h"
#include <rtdevice.h>
#include "fh_dma.h"

struct str_dma_m2m
{
    struct rt_dma_device *dma_dev;
    struct dma_transfer m2m_trans;
    unsigned int dma_channel;
    struct rt_completion transfer_completion;
    int (*usr_done_backcall)(void *para);
    void *para;
    struct rt_semaphore xfer_lock;
};


struct str_dma_m2m g_dma_m2m = {
    .dma_channel = AUTO_FIND_CHANNEL,
};

int dma_m2m_init(struct str_dma_m2m *p_dma_m2m, char *dma_dev_name)
{
    int ret;
    struct rt_dma_device *rt_dma_dev;
    struct dma_transfer *p_m2m_trans;

    rt_memset(p_dma_m2m, 0, sizeof(struct str_dma_m2m));
    p_dma_m2m->dma_channel = AUTO_FIND_CHANNEL;
    rt_completion_init(&p_dma_m2m->transfer_completion);
    rt_sem_init(&p_dma_m2m->xfer_lock, "dma_memcpy", 1,
    RT_IPC_FLAG_FIFO);
    p_dma_m2m->dma_dev = (struct rt_dma_device *)rt_device_find(dma_dev_name);
    RT_ASSERT(p_dma_m2m->dma_dev != RT_NULL);

    rt_dma_dev = p_dma_m2m->dma_dev;
    p_m2m_trans = &p_dma_m2m->m2m_trans;
    p_m2m_trans->channel_number = p_dma_m2m->dma_channel;
    rt_dma_dev->ops->control(rt_dma_dev, RT_DEVICE_CTRL_DMA_OPEN, RT_NULL);
    ret = rt_dma_dev->ops->control(rt_dma_dev,
                       RT_DEVICE_CTRL_DMA_REQUEST_CHANNEL, (void *)p_m2m_trans);
    RT_ASSERT(ret == RT_EOK);

    return ret;
}

static void dma_m2m_done(void *arg)
{
    struct str_dma_m2m *p_dma_m2m = (struct str_dma_m2m *)arg;

    rt_completion_done(&p_dma_m2m->transfer_completion);
}

extern void mmu_clean_dcache(rt_uint32_t buffer, rt_uint32_t size);
extern void mmu_invalidate_dcache(rt_uint32_t buffer, rt_uint32_t size);
extern void mmu_clean_invalidated_dcache(rt_uint32_t buffer, rt_uint32_t size);
void dma_mem_to_mem(struct str_dma_m2m *p_dma_m2m, const unsigned char *src, unsigned char *dst, unsigned int cnt)
{
    struct dma_transfer *trans;
    unsigned int data_width = 0;
    unsigned int xfer_cnt = 0;
    unsigned int t_src = (rt_uint32_t)src;
    unsigned int t_dst = (rt_uint32_t)dst;

    rt_sem_take(&p_dma_m2m->xfer_lock, RT_WAITING_FOREVER);
    if (!(t_src % 4) && !(t_dst % 4) && !(cnt % 4))
    {
        data_width = DW_DMA_SLAVE_WIDTH_32BIT;
        xfer_cnt = cnt / 4;
    }
    else if (!(t_src % 2) && !(t_dst % 2) && !(cnt % 2))
    {
        data_width = DW_DMA_SLAVE_WIDTH_16BIT;
        xfer_cnt = cnt / 2;
    }
    else
    {
        data_width = DW_DMA_SLAVE_WIDTH_8BIT;
        xfer_cnt = cnt;
    }

    trans = &p_dma_m2m->m2m_trans;

    trans->dma_number = 0;
    trans->fc_mode = DMA_M2M;

    trans->dst_add = (rt_uint32_t)dst;
    trans->dst_inc_mode = DW_DMA_SLAVE_INC;
    trans->dst_msize = DW_DMA_SLAVE_MSIZE_8;
    trans->dst_width = data_width;

    trans->src_add = (rt_uint32_t)src;
    trans->src_inc_mode = DW_DMA_SLAVE_INC;
    trans->src_msize = DW_DMA_SLAVE_MSIZE_8;
    trans->src_width = data_width;

    trans->trans_len = xfer_cnt;
    trans->complete_callback = (void *)dma_m2m_done;
    trans->complete_para = (void *)p_dma_m2m;
    rt_completion_init(&p_dma_m2m->transfer_completion);
    /*invalid dst mem.*/
    mmu_clean_dcache((rt_uint32_t)src, cnt);
    mmu_clean_invalidated_dcache((rt_uint32_t)dst, cnt);

    /*dma go...*/
    p_dma_m2m->dma_dev->ops->control(p_dma_m2m->dma_dev,
                                     RT_DEVICE_CTRL_DMA_SINGLE_TRANSFER,
                                     (void *)&p_dma_m2m->m2m_trans);

    /*wait done*/
    rt_completion_wait(&p_dma_m2m->transfer_completion, RT_WAITING_FOREVER);
    mmu_clean_invalidated_dcache((rt_uint32_t)dst, cnt);
    if (p_dma_m2m->usr_done_backcall)
    {
        p_dma_m2m->usr_done_backcall(p_dma_m2m->para);
    }
    rt_sem_release(&p_dma_m2m->xfer_lock);
}

static int init_flag = 0;
int fh_dma_memcpy_init(void)
{
    if (!init_flag)
    {
        dma_m2m_init(&g_dma_m2m, "fh_dma0");
        init_flag = 1;
    }

    return 0;
}

void *fh_dma_memcpy(void *dst, const void *src, int len)
{
#ifdef RT_USING_DMA_MEMCPY
    fh_dma_memcpy_init();
    if (init_flag && dst && src && len > 0)
    {
        dma_mem_to_mem(&g_dma_m2m, src, dst, len);
    }

    return dst;
#else
    return rt_memcpy(dst, src, (rt_ubase_t)len);
#endif
}


static void print_content(unsigned char *p, int len)
{
    int i;

    for (i = 0; i < len; i++)
    {
        if (i % 16 == 0)
        {
            rt_kprintf("\n");
        }
        rt_kprintf("%02x ", p[i]);
    }
}

void dma_mem_test(unsigned int size)
{
    int ret;

    static __attribute__((aligned(32))) unsigned char dma_test_src_buff[256];
    static __attribute__((aligned(32))) unsigned char dma_test_dst_buff[256];

    rt_memset(dma_test_src_buff, 0x55, sizeof(dma_test_src_buff));
    rt_memset(dma_test_dst_buff, 0xaa, sizeof(dma_test_dst_buff));

    ret = fh_dma_memcpy_init();
    if (ret != 0)
    {
        rt_kprintf("dma memcpy init failed!\n");
        return;
    }

    fh_dma_memcpy(dma_test_dst_buff, dma_test_src_buff, size);
    print_content(dma_test_dst_buff, size);
}


#if 0       /* test code, DO NOT export in release */
#ifdef RT_USING_FINSH
#include <finsh.h>
FINSH_FUNCTION_EXPORT(dma_mem_test, dma_mem_test);
#ifdef FINSH_USING_MSH
void dma_memcpy(int argc, char *argv[])
{
    int sz = 1024;

    if (argc > 1)
        sz = atoi(argv[1]);
    dma_mem_test(sz);
}
FINSH_FUNCTION_EXPORT_ALIAS(dma_memcpy, __cmd_dma_memcpy, test dma memory copy);
#endif
#endif
#endif
