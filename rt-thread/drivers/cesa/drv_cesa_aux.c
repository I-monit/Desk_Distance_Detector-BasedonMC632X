/********************************************************************************************/
/* Fullhan Technology (Shanghai) Co., Ltd.                                                  */
/* Fullhan Proprietary and Confidential                                                     */
/* Copyright (c) 2017 Fullhan Technology Group Limited and its affiliated companies         */
/********************************************************************************************/
#include "drv_cesa_if.h"
#include "hw_cesa_if.h"


void drv_cesa_aux_init(FH_CESA_AUX_S *p_aux)
{
    rt_sem_init(&p_aux->sem_ades, "sem_ades", 1, RT_IPC_FLAG_FIFO);
    rt_sem_init(&p_aux->sem_sha, "sem_ades", 1, RT_IPC_FLAG_FIFO);
    rt_sem_init(&p_aux->sem_rsa, "sem_ades", 1, RT_IPC_FLAG_FIFO);

    p_aux->keep_ades = 0;

    rt_completion_init(&p_aux->completion_ades);
}

void drv_cesa_aux_deinit(FH_CESA_AUX_S *p_aux)
{
    p_aux->keep_ades = 0;
}

void drv_cesa_aux_intr_install(int         vector,
                                       void *handler,
                                       void *param)
{
    rt_hw_interrupt_umask(vector);
    rt_hw_interrupt_install(vector, (rt_isr_handler_t)handler, param, "ades_isr");
}

void drv_cesa_ades_lock(uint32_t session)
{
    FH_CESA_AUX_S *p_aux = (FH_CESA_AUX_S *)((HW_CESA_ADES_CTRL_S *)session)->priv;

    if (p_aux)
    {
        if (rt_sem_take(&p_aux->sem_ades, RT_WAITING_FOREVER))
            rt_kprintf("Warning: ades lock failed!\n");
    }
}

void drv_cesa_ades_unlock(uint32_t session)
{
    FH_CESA_AUX_S *p_aux = (FH_CESA_AUX_S *)((HW_CESA_ADES_CTRL_S *)session)->priv;

    if (p_aux)
        rt_sem_release(&p_aux->sem_ades);
}

bool drv_cesa_check_keep(uint32_t session, uint32_t same_diff_both)
{
    FH_CESA_AUX_S *p_aux = (FH_CESA_AUX_S *)((HW_CESA_ADES_CTRL_S *)session)->priv;

    if (p_aux)
    {
        if (same_diff_both == 0)
        {
            if (p_aux->keep_ades == session)
            {
                p_aux->keep_ades = 0;
                return true;
            }
        }

        if (same_diff_both == 1)
        {
            if (p_aux->keep_ades != session)
            {
                p_aux->keep_ades = session;
                return true;
            }
        }

        if (same_diff_both == 2)
        {
            if (p_aux->keep_ades == session)
            {
                p_aux->keep_ades = 0;
                return true;
            }
            else
            {
                p_aux->keep_ades = session;
                return true;
            }
        }

        return false;
    }
    else
    {
        return true;
    }
}

void drv_cesa_ades_send_complete(void *cesa_aux)
{
    FH_CESA_AUX_S *p_aux = (FH_CESA_AUX_S *)cesa_aux;

    rt_completion_done(&p_aux->completion_ades);
}

int32_t drv_cesa_ades_wait_for_complete(uint32_t session)
{
    FH_CESA_AUX_S *p_aux = (FH_CESA_AUX_S *)((HW_CESA_ADES_CTRL_S *)session)->priv;

    if (rt_completion_wait(&p_aux->completion_ades, RT_TICK_PER_SECOND * 20))
        return CESA_ADES_DMA_DONE_TIMEOUT;

    return hw_ades_ext_dma_done((HW_CESA_ADES_CTRL_S *)session);
}

void drv_cesa_ades_flush_dcache(uint32_t buffer, uint32_t size)
{
#ifdef CPU_ARC
    rt_kprintf("%s is empty, pls finish me!\n", __func__);
#else
    extern void mmu_clean_dcache(rt_uint32_t buffer, rt_uint32_t size);
    mmu_clean_dcache(buffer, size);
#endif
}

void drv_cesa_ades_invalidate_dcache(uint32_t buffer, uint32_t size)
{
#ifdef CPU_ARC
    rt_kprintf("%s is empty, pls finish me!\n", __func__);
#else
    extern void mmu_invalidate_dcache(rt_uint32_t buffer, rt_uint32_t size);
    mmu_invalidate_dcache(buffer, size);
#endif
}

void drv_cesa_ades_flush_invalidate_dcache(uint32_t buffer, uint32_t size)
{
#ifdef CPU_ARC
    rt_kprintf("%s is empty, pls finish me!\n", __func__);
#else
    extern void mmu_clean_invalidated_dcache(rt_uint32_t buffer, rt_uint32_t size);
    mmu_clean_invalidated_dcache(buffer, size);
#endif
}

void drv_cesa_data_print(int8_t *string, uint8_t *data, uint32_t length, uint8_t align)
{
    uint32_t i;

    if (string != NULL)
    {
        rt_kprintf("%s:\n", string);
    }

    if (data != NULL)
    {
        for (i = 0; i < length; i++)
        {
            rt_kprintf("%02x ", data[i]);
            if (((i+1)%align) == 0) /* (0 == ((i+1) & (align-1))) */
            {
                rt_kprintf("\n");
            }
        }
        rt_kprintf("\n");
    }
}

