/********************************************************************************************/
/* Fullhan Technology (Shanghai) Co., Ltd.                                                  */
/* Fullhan Proprietary and Confidential                                                     */
/* Copyright (c) 2017 Fullhan Technology Group Limited and its affiliated companies         */
/********************************************************************************************/
#ifndef __HW_CESA_IF_H__
#define __HW_CESA_IF_H__

#include "fh_cesa.h"

extern FH_CESA_DEV_P g_cesa;

/*** HW ADES ***/
#ifndef ADES_REG32
#define ADES_REG32(addr) (*(volatile unsigned int *)(g_cesa->ades_reg + addr))
#endif

typedef enum  _HW_CESA_ADES_OPER_MODE_E
{
    HW_CESA_ADES_OPER_MODE_ENC,        /* 00 */
    HW_CESA_ADES_OPER_MODE_DEC,        /* 01 */
}HW_CESA_ADES_OPER_MODE_E;

typedef enum  _HW_CESA_ADES_ALGO_MODE_E
{
    HW_CESA_ADES_ALGO_MODE_DES = 0,        /* 000 */
    HW_CESA_ADES_ALGO_MODE_TDES = 1,       /* 001 */
    HW_CESA_ADES_ALGO_MODE_AES128 = 4,     /* 100 */
    HW_CESA_ADES_ALGO_MODE_AES192 = 5,     /* 101 */
    HW_CESA_ADES_ALGO_MODE_AES256 = 6,     /* 110 */
}HW_CESA_ADES_ALGO_MODE_E;

typedef enum  _HW_CESA_ADES_WORK_MODE_E
{
    HW_CESA_ADES_WORK_MODE_ECB = 0,        /* 000 */
    HW_CESA_ADES_WORK_MODE_CBC = 1,        /* 001 */
    HW_CESA_ADES_WORK_MODE_CTR = 2,        /* 010 -------- AES128 only */
    HW_CESA_ADES_WORK_MODE_CFB = 4,        /* 100 -------- AES:CFB8     DES:CFB8 */
    HW_CESA_ADES_WORK_MODE_OFB = 5,        /* 101 -------- AES:OFB128   DES:OFB64 */
}HW_CESA_ADES_WORK_MODE_E;

typedef struct _HW_CESA_ADES_EFUSE_PARA_S
{
    uint32_t mode;
    uint32_t map_size;
    struct
    {
        uint32_t crypto_key_no;
        uint32_t ex_mem_entry;
    } map[MAX_EX_KEY_MAP_SIZE];
} HW_CESA_ADES_EFUSE_PARA_S;

typedef struct _HW_CESA_ADES_CTRL_S
{
    void *priv;

    #define IV_MAX_SIZE        16
    #define KEY_MAX_SIZE    32

    uint8_t iv_init[IV_MAX_SIZE];
    uint8_t key[KEY_MAX_SIZE];
    uint8_t iv_last[IV_MAX_SIZE];

    HW_CESA_ADES_OPER_MODE_E oper_mode;
    HW_CESA_ADES_WORK_MODE_E work_mode;
    HW_CESA_ADES_ALGO_MODE_E algo_mode;

    HW_CESA_ADES_EFUSE_PARA_S efuse_para;

    struct rt_dma_device *ext_dma_dev;
    struct dma_transfer *src_chan;
    struct dma_transfer *dst_chan;
    uint32_t src_done;
    uint32_t dst_done;

    bool self_key_gen;
    uint8_t __attribute__((aligned(32))) self_key_text[32];
} HW_CESA_ADES_CTRL_S;

int32_t hw_cesa_ades_config(HW_CESA_ADES_CTRL_S *hw_ctrl);
int32_t hw_cesa_ades_process(HW_CESA_ADES_CTRL_S *hw_ctrl,
                                   uint32_t phy_src_addr,
                                   uint32_t phy_dst_addr,
                                   uint32_t length);
uint32_t hw_cesa_ades_intr_src(void);
void hw_cesa_ades_save_lastiv(HW_CESA_ADES_CTRL_S *hw_ctrl);
void hw_cesa_ades_restore_lastiv(HW_CESA_ADES_CTRL_S *hw_ctrl);

int32_t hw_cesa_ades_process_start(HW_CESA_ADES_CTRL_S *hw_ctrl,
                                           uint32_t p_src_addr,
                                           uint32_t p_dst_addr,
                                           uint32_t length);
int32_t hw_cesa_ades_process_polling(HW_CESA_ADES_CTRL_S *hw_ctrl, uint32_t time_out);
int32_t hw_cesa_ades_process_stop(HW_CESA_ADES_CTRL_S *hw_ctrl);
int32_t hw_ades_ext_dma_req_chan(HW_CESA_ADES_CTRL_S *hw_ctrl);
void hw_ades_ext_dma_rel_chan(HW_CESA_ADES_CTRL_S *hw_ctrl);
int32_t hw_ades_ext_dma_done(HW_CESA_ADES_CTRL_S *hw_ctrl);
void hw_ades_config_selfkey(HW_CESA_ADES_CTRL_S *hw_ctrl, bool use_self_key, uint8_t *self_key_text);

/*** HW HASH ***/

/*** HW RSA ***/




#endif    /*__HW_CESA_IF_H__*/

