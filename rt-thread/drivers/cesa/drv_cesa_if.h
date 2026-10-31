/********************************************************************************************/
/* Fullhan Technology (Shanghai) Co., Ltd.                                                  */
/* Fullhan Proprietary and Confidential                                                     */
/* Copyright (c) 2017 Fullhan Technology Group Limited and its affiliated companies         */
/********************************************************************************************/
#ifndef __DRV_CESA_IF_H__
#define __DRV_CESA_IF_H__

#include "fh_cesa.h"


#ifdef ADES_DEBUG
#define ADES_DBG(fmt, args...)     \
    do                                  \
    {                                   \
        rt_kprintf("[ADES]: ");   \
        rt_kprintf(fmt, ## args);       \
    }                                   \
    while (0)
#else
#define ADES_DBG(fmt, args...)  do { } while (0)
#endif

#define ADES_ERR(fmt, args...)     \
    do                                  \
    {                                   \
        rt_kprintf("[ADES ERROR]: ");   \
        rt_kprintf(fmt, ## args);       \
    }                                   \
    while (0)

#define ADES_WARN(fmt, args...)     \
    do                                    \
    {                                    \
        rt_kprintf("[ADES WARNING]: ");   \
        rt_kprintf(fmt, ## args);        \
    }                                    \
    while (0)



/*** AUXILIARY ***/
typedef struct rt_semaphore os_sem_t;
typedef struct rt_completion os_cmpl_t;
typedef struct _FH_CESA_AUX_S
{
    uint32_t keep_ades;
    os_sem_t sem_ades;
    os_sem_t sem_sha;
    os_sem_t sem_rsa;

    os_cmpl_t completion_ades;
} FH_CESA_AUX_S, *FH_CESA_AUX_P;


void drv_cesa_aux_init(FH_CESA_AUX_S *p_aux);
void drv_cesa_aux_deinit(FH_CESA_AUX_S *p_aux);
void drv_cesa_aux_intr_install(int         vector, void *handler, void *param);

int32_t drv_cesa_ades_create(uint32_t *p_session, void *p_priv);
int32_t drv_cesa_ades_destroy(uint32_t session);
int32_t drv_cesa_ades_config(uint32_t session, FH_CESA_ADES_CTRL_S *p_ctrl);
int32_t drv_cesa_ades_config_oper(uint32_t session, FH_CESA_ADES_OPER_MODE_E oper_mode);
int32_t drv_cesa_ades_process(uint32_t session, uint32_t phy_src_addr, uint32_t phy_dst_addr, uint32_t length);
int32_t drv_cesa_ades_get_infor(uint32_t session, FH_CESA_ADES_CTRL_S *p_ctrl);
int32_t drv_cesa_ades_efuse_key(uint32_t session, FH_CESA_ADES_EFUSE_PARA_S *efuse_para);
int32_t drv_cesa_ades_intr(void *cesa_aux);
void drv_cesa_ades_invalidate_dcache(uint32_t buffer, uint32_t size);


#endif /*__DRV_CESA_IF_H__*/

