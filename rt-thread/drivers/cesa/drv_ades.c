/********************************************************************************************/
/* Fullhan Technology (Shanghai) Co., Ltd.                                                  */
/* Fullhan Proprietary and Confidential                                                     */
/* Copyright (c) 2017 Fullhan Technology Group Limited and its affiliated companies         */
/********************************************************************************************/
#include "drv_cesa_if.h"
#include "hw_cesa_if.h"

void drv_cesa_ades_lock(uint32_t session);
void drv_cesa_ades_unlock(uint32_t session);
bool drv_cesa_check_keep(uint32_t session, uint32_t same_diff_both);
void drv_cesa_ades_send_complete(void *cesa_aux);
int32_t drv_cesa_ades_wait_for_complete(uint32_t session);
void drv_cesa_ades_flush_invalidate_dcache(uint32_t buffer, uint32_t size);


static int32_t hw_cesa_ades_config_data_prepare(FH_CESA_ADES_CTRL_S *app_ctrl, HW_CESA_ADES_CTRL_S *hw_ctrl)
{
    switch (app_ctrl->work_mode)
    {
    case FH_CESA_ADES_WORK_MODE_ECB:
        hw_ctrl->work_mode = HW_CESA_ADES_WORK_MODE_ECB;
        break;
    case FH_CESA_ADES_WORK_MODE_CBC:
        hw_ctrl->work_mode = HW_CESA_ADES_WORK_MODE_CBC;
        break;
    case FH_CESA_AES_WORK_MODE_CTR:
        if (app_ctrl->algo_mode == FH_CESA_ADES_ALGO_MODE_DES
            || app_ctrl->algo_mode == FH_CESA_ADES_ALGO_MODE_TDES)
            return CESA_ADES_WORK_ALGO_MODE_CONFLICT;

        hw_ctrl->work_mode = HW_CESA_ADES_WORK_MODE_CTR;
        break;

    case FH_CESA_ADES_WORK_MODE_CFB8:
        hw_ctrl->work_mode = HW_CESA_ADES_WORK_MODE_CFB;
        break;
    case FH_CESA_ADES_WORK_MODE_OFB:
        hw_ctrl->work_mode = HW_CESA_ADES_WORK_MODE_OFB;
        break;
    default:
        return CESA_ADES_WORK_MODE_ERROR;
    }

    switch (app_ctrl->algo_mode)
    {
    case FH_CESA_ADES_ALGO_MODE_AES128:
        hw_ctrl->algo_mode = HW_CESA_ADES_ALGO_MODE_AES128;
        break;
    case FH_CESA_ADES_ALGO_MODE_AES192:
        hw_ctrl->algo_mode = HW_CESA_ADES_ALGO_MODE_AES192;
        break;
    case FH_CESA_ADES_ALGO_MODE_AES256:
        hw_ctrl->algo_mode = HW_CESA_ADES_ALGO_MODE_AES256;
        break;
    case FH_CESA_ADES_ALGO_MODE_DES:
        hw_ctrl->algo_mode = HW_CESA_ADES_ALGO_MODE_DES;
        break;
    case FH_CESA_ADES_ALGO_MODE_TDES:
        hw_ctrl->algo_mode = HW_CESA_ADES_ALGO_MODE_TDES;
        break;
    default:
        return CESA_ADES_ALGO_MODE_ERROR;
    }


    switch (app_ctrl->oper_mode)
    {
    case FH_CESA_ADES_OPER_MODE_ENCRYPT:
        hw_ctrl->oper_mode = HW_CESA_ADES_OPER_MODE_ENC;
        break;
    case FH_CESA_ADES_OPER_MODE_DECRYPT:
        hw_ctrl->oper_mode = HW_CESA_ADES_OPER_MODE_DEC;
        break;
    }

    memcpy(hw_ctrl->key, app_ctrl->key, KEY_MAX_SIZE);
    memcpy(hw_ctrl->iv_init, app_ctrl->iv, IV_MAX_SIZE);
    memset(hw_ctrl->iv_last, 0, IV_MAX_SIZE);

    hw_ctrl->efuse_para.mode &= ~(CRYPTO_CPU_SET_KEY | CRYPTO_EX_MEM_SET_KEY);
    hw_ctrl->efuse_para.mode |= app_ctrl->enKeySrc;

    hw_ades_config_selfkey(hw_ctrl, app_ctrl->self_key_gen, app_ctrl->self_key_text);

    return CESA_SUCCESS;
}

int32_t drv_cesa_ades_create(uint32_t *p_session, void *p_priv)
{
    *p_session = (uint32_t)rt_malloc(sizeof(HW_CESA_ADES_CTRL_S));

    if (*p_session == 0)
    {
        return CESA_ADES_HDLR_GET_FAILED;
    }

    memset((void *)(*p_session), 0, sizeof(HW_CESA_ADES_CTRL_S));
    ((HW_CESA_ADES_CTRL_S *)(*p_session))->priv = p_priv;

    if (hw_ades_ext_dma_req_chan((HW_CESA_ADES_CTRL_S *)(*p_session))) {
        rt_free((void *)(*p_session));
        return CESA_ADES_HDLR_GET_FAILED;
    }

    return CESA_SUCCESS;
}

int32_t drv_cesa_ades_destroy(uint32_t session)
{
    if (session == 0)
    {
        return CESA_ADES_HDLR_SET_INVALID;
    }

    drv_cesa_check_keep(session, 2);

    hw_ades_ext_dma_rel_chan((HW_CESA_ADES_CTRL_S *)session);

    rt_free((void *)session);

    return CESA_SUCCESS;
}

int32_t drv_cesa_ades_config(uint32_t session, FH_CESA_ADES_CTRL_S *p_ctrl)
{
    HW_CESA_ADES_CTRL_S *ades_ce = (HW_CESA_ADES_CTRL_S *)session;
    int32_t ret = CESA_SUCCESS;

    if (session == 0)
    {
        return CESA_ADES_HDLR_SET_INVALID;
    }

    ret = hw_cesa_ades_config_data_prepare(p_ctrl, ades_ce);
    if (ret == CESA_SUCCESS)
    {
        drv_cesa_ades_lock(session);

        drv_cesa_check_keep(session, 0);

        drv_cesa_ades_unlock(session);
    }

    return ret;
}

int32_t drv_cesa_ades_config_oper(uint32_t session, FH_CESA_ADES_OPER_MODE_E oper_mode)
{
    HW_CESA_ADES_CTRL_S *ades_ce = (HW_CESA_ADES_CTRL_S *)session;

    if (session == 0)
        return CESA_ADES_HDLR_SET_INVALID;

    switch (oper_mode)
    {
    case FH_CESA_ADES_OPER_MODE_ENCRYPT:
        if (ades_ce->oper_mode != HW_CESA_ADES_OPER_MODE_ENC)
        {
            ades_ce->oper_mode = HW_CESA_ADES_OPER_MODE_ENC;
            hw_cesa_ades_restore_lastiv(ades_ce);
            drv_cesa_ades_lock(session);
            drv_cesa_check_keep(session, 0);
            drv_cesa_ades_unlock(session);
        }
        break;
    case FH_CESA_ADES_OPER_MODE_DECRYPT:
        if (ades_ce->oper_mode != HW_CESA_ADES_OPER_MODE_DEC)
        {
            ades_ce->oper_mode = HW_CESA_ADES_OPER_MODE_DEC;
            hw_cesa_ades_restore_lastiv(ades_ce);
            drv_cesa_ades_lock(session);
            drv_cesa_check_keep(session, 0);
            drv_cesa_ades_unlock(session);
        }
        break;
    default:
        return CESA_ADES_OPER_MODE_ERROR;
    }

    return CESA_SUCCESS;
}

extern void drv_cesa_ades_invalidate_dcache(uint32_t buffer, uint32_t size);
int32_t drv_cesa_ades_process(uint32_t session, uint32_t phy_src_addr, uint32_t phy_dst_addr, uint32_t length)
{
    HW_CESA_ADES_CTRL_S *ades_ce = (HW_CESA_ADES_CTRL_S *)session;
    int32_t ret = CESA_SUCCESS;

    if (session == 0)
    {
        return CESA_ADES_HDLR_SET_INVALID;
    }

    drv_cesa_ades_lock(session);

    if (drv_cesa_check_keep(session, 1))
    {
        ret = hw_cesa_ades_config(ades_ce);
        if (ret)
            goto process_out;
    }

    drv_cesa_ades_flush_invalidate_dcache(phy_src_addr, length);
    drv_cesa_ades_flush_invalidate_dcache(phy_dst_addr, length);

    ret = hw_cesa_ades_process(ades_ce, phy_src_addr, phy_dst_addr, length);
    if (ret)
        goto process_out;

    ret = drv_cesa_ades_wait_for_complete(session);
    if (ret)
        goto process_out;

    drv_cesa_ades_invalidate_dcache(phy_dst_addr, length);

    hw_cesa_ades_save_lastiv(ades_ce);

process_out:
    drv_cesa_ades_unlock(session);
    return ret;
}

int32_t drv_cesa_ades_intr(void *cesa_aux)
{
    uint32_t intr_src = hw_cesa_ades_intr_src();

    if (intr_src & 0x02)
        ADES_WARN("dma rev hreap error...\n");
    if (intr_src & 0x04)
        ADES_WARN("dma stop src ..\n");
    if (intr_src & 0x01)
    {
        ADES_DBG("dma done..\n");
        drv_cesa_ades_send_complete(cesa_aux);
    }

    return CESA_SUCCESS;
}

int32_t drv_cesa_ades_get_infor(uint32_t session, FH_CESA_ADES_CTRL_S *p_ctrl)
{
    HW_CESA_ADES_CTRL_S *ades_ce = (HW_CESA_ADES_CTRL_S *)session;

    if (session == 0)
    {
        return CESA_ADES_HDLR_SET_INVALID;
    }

    switch (ades_ce->algo_mode)
    {
    case HW_CESA_ADES_ALGO_MODE_DES:
        p_ctrl->algo_mode = FH_CESA_ADES_ALGO_MODE_DES;
        break;
    case HW_CESA_ADES_ALGO_MODE_TDES:
        p_ctrl->algo_mode = FH_CESA_ADES_ALGO_MODE_TDES;
        break;
    case HW_CESA_ADES_ALGO_MODE_AES128:
        p_ctrl->algo_mode = FH_CESA_ADES_ALGO_MODE_AES128;
        break;
    case HW_CESA_ADES_ALGO_MODE_AES192:
        p_ctrl->algo_mode = FH_CESA_ADES_ALGO_MODE_AES192;
        break;
    case HW_CESA_ADES_ALGO_MODE_AES256:
        p_ctrl->algo_mode = FH_CESA_ADES_ALGO_MODE_AES256;
        break;
    default:
        return CESA_ADES_ALGO_MODE_ERROR;
    }

    switch (ades_ce->oper_mode)
    {
    case HW_CESA_ADES_OPER_MODE_ENC:
        p_ctrl->oper_mode = FH_CESA_ADES_OPER_MODE_ENCRYPT;
        break;
    case HW_CESA_ADES_OPER_MODE_DEC:
        p_ctrl->oper_mode = FH_CESA_ADES_OPER_MODE_DECRYPT;
        break;
    default:
        return CESA_ADES_OPER_MODE_ERROR;
    }

    switch (ades_ce->work_mode)
    {
    case HW_CESA_ADES_WORK_MODE_ECB:
        p_ctrl->work_mode = FH_CESA_ADES_WORK_MODE_ECB;
        break;
    case HW_CESA_ADES_WORK_MODE_CBC:
        p_ctrl->work_mode = FH_CESA_ADES_WORK_MODE_CBC;
        break;
    case HW_CESA_ADES_WORK_MODE_CTR:
        if (p_ctrl->algo_mode == FH_CESA_ADES_ALGO_MODE_DES
            || p_ctrl->algo_mode == FH_CESA_ADES_ALGO_MODE_TDES)
            return CESA_ADES_WORK_ALGO_MODE_CONFLICT;

        p_ctrl->work_mode = FH_CESA_AES_WORK_MODE_CTR;
        break;
    case HW_CESA_ADES_WORK_MODE_CFB:
        p_ctrl->work_mode = FH_CESA_ADES_WORK_MODE_CFB8;
        break;
    case HW_CESA_ADES_WORK_MODE_OFB:
        p_ctrl->work_mode = FH_CESA_ADES_WORK_MODE_OFB;
        break;
    default:
        return CESA_ADES_WORK_MODE_ERROR;
    }

    switch (ades_ce->efuse_para.mode & (CRYPTO_CPU_SET_KEY | CRYPTO_EX_MEM_SET_KEY))
    {
    case FH_CESA_ADES_KEY_SRC_USER:
        p_ctrl->enKeySrc = FH_CESA_ADES_KEY_SRC_USER;
        break;
    case FH_CESA_ADES_KEY_SRC_EFUSE:
        p_ctrl->enKeySrc = FH_CESA_ADES_KEY_SRC_EFUSE;
        break;
    default:
        return CESA_ADES_KEY_SRC_ERROR;
    }

    memcpy(p_ctrl->key, ades_ce->key, KEY_MAX_SIZE);
    memcpy(p_ctrl->iv, ades_ce->iv_init, IV_MAX_SIZE);

    return CESA_SUCCESS;
}

int32_t drv_cesa_ades_efuse_key(uint32_t session, FH_CESA_ADES_EFUSE_PARA_S *efuse_para)
{
    HW_CESA_ADES_CTRL_S *ades_ce = (HW_CESA_ADES_CTRL_S *)session;
    uint32_t mode_store;

    if (sizeof(ades_ce->efuse_para) != sizeof(*efuse_para))
        return CESA_INVALID_VALUE;

    mode_store = ades_ce->efuse_para.mode;
    memcpy(&(ades_ce->efuse_para), efuse_para, sizeof(*efuse_para));
    ades_ce->efuse_para.mode |= mode_store;

    drv_cesa_ades_lock(session);
    drv_cesa_check_keep(session, 0);
    drv_cesa_ades_unlock(session);

    return CESA_SUCCESS;
}

