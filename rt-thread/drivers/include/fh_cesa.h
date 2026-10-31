/********************************************************************************************/
/* Fullhan Technology (Shanghai) Co., Ltd.                                                  */
/* Fullhan Proprietary and Confidential                                                     */
/* Copyright (c) 2014 Fullhan Technology Group Limited and its affiliated companies         */
/********************************************************************************************/
#ifndef __FH_CESA_H__
#define __FH_CESA_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <rthw.h>
#include <board_info.h>
#include <rtdevice.h>
#include <stdbool.h>
#include <string.h>
#include <fh_clock.h>


#define MAX_EX_KEY_MAP_SIZE                     8
#define CRYPTO_CPU_SET_KEY                      (1<<0)
#define CRYPTO_EX_MEM_SET_KEY                   (1<<1)
#define CRYPTO_EX_MEM_INDEP_POWER               (1<<2)
/*bit 8~ex mem bit field..*/
#define CRYPTO_EX_MEM_SWITCH_KEY                (1<<8)
/*if set ex mem set switch key..then parse below..*/
#define CRYPTO_EX_MEM_4_ENTRY_1_KEY             (1<<9)
#define CESA_USE_EXT_DMA                        0x55AAAA55

struct fh_cesa_platform_data
{
    rt_uint32_t id;
    rt_uint32_t irq;
    rt_uint32_t base;
    rt_uint32_t ext_dma_enable;
    rt_uint32_t ext_dma_rx_handshake;
    rt_uint32_t ext_dma_tx_handshake;
    rt_uint32_t reset_base;
    rt_uint32_t reset_offset;
    rt_uint32_t reset_bit;
    char *ext_dma_name;
};

typedef struct _FH_CESA_DEV_S
{
    struct rt_device parent;
    void *p_priv;
    rt_uint32_t ades_reg;
    rt_uint32_t ades_irq;
    rt_uint32_t reset_base;
    rt_uint32_t reset_offset;
    rt_uint32_t reset_bit;
    struct clk *clk;
} FH_CESA_DEV_S, *FH_CESA_DEV_P;

/*** ADES ***/
typedef enum  _FH_CESA_ADES_OPER_MODE_E
{
    FH_CESA_ADES_OPER_MODE_ENCRYPT,
    FH_CESA_ADES_OPER_MODE_DECRYPT,
} FH_CESA_ADES_OPER_MODE_E;

typedef enum  _FH_CESA_ADES_ALGO_MODE_E
{
    FH_CESA_ADES_ALGO_MODE_AES128,
    FH_CESA_ADES_ALGO_MODE_AES192,
    FH_CESA_ADES_ALGO_MODE_AES256,
    FH_CESA_ADES_ALGO_MODE_DES,
    FH_CESA_ADES_ALGO_MODE_TDES,
} FH_CESA_ADES_ALGO_MODE_E;

typedef enum  _FH_CESA_ADES_WORK_MODE_E
{
    FH_CESA_ADES_WORK_MODE_ECB,
    FH_CESA_ADES_WORK_MODE_CBC,
    FH_CESA_AES_WORK_MODE_CTR,
    FH_CESA_ADES_WORK_MODE_CFB8,
    FH_CESA_ADES_WORK_MODE_OFB,
} FH_CESA_ADES_WORK_MODE_E;


typedef enum _FH_CESA_ADES_KEY_SRC_E
{
    FH_CESA_ADES_KEY_SRC_USER = CRYPTO_CPU_SET_KEY,
    FH_CESA_ADES_KEY_SRC_EFUSE = CRYPTO_EX_MEM_SET_KEY,
    FH_CESA_ADES_KEY_SRC_BUTT
} FH_CESA_ADES_KEY_SRC_E;

typedef struct _FH_CESA_ADES_CTRL_S
{
    FH_CESA_ADES_OPER_MODE_E oper_mode;
    FH_CESA_ADES_ALGO_MODE_E algo_mode;
    FH_CESA_ADES_WORK_MODE_E work_mode;
    uint8_t key[32];
    uint8_t iv[16];
    FH_CESA_ADES_KEY_SRC_E enKeySrc;
    bool self_key_gen;
    uint8_t self_key_text[32];
} FH_CESA_ADES_CTRL_S;

typedef struct _FH_CESA_ADES_EFUSE_PARA_S
{
    uint32_t mode;
    uint32_t map_size;
    struct {
        uint32_t crypto_key_no;
        uint32_t ex_mem_entry;
    } map[8];
} FH_CESA_ADES_EFUSE_PARA_S;

/*** SHA ***/

/*** RSA ***/

typedef struct _CMD_ADES_SESSION_S
{
    uint32_t session;
    int32_t ret;
} CMD_ADES_CREATE_S, CMD_ADES_DESTROY_S, CMD_ADES_PROCESS_STOP_S;

typedef struct _CMD_ADES_CTRL_S
{
    uint32_t session;
    FH_CESA_ADES_CTRL_S ctrl;
    int32_t ret;
} CMD_ADES_CTRL_S, CMD_ADES_INFO_S;

typedef struct _CMD_ADES_PROCESS_S
{
    uint32_t session;
    uint32_t p_src_addr;
    uint32_t p_dst_addr;
    uint32_t length;
    int32_t ret;
} CMD_ADES_PROCESS_S, CMD_ADES_PROCESS_START_S;

typedef struct _CMD_ADES_PROCESS_POLLING_S
{
    uint32_t session;
    uint32_t timeout;
    int32_t ret;
} CMD_ADES_PROCESS_POLLING_S;

typedef struct _CMD_ADES_EFUSE_KEY_S
{
    uint32_t session;
    FH_CESA_ADES_EFUSE_PARA_S efuse_para;
    int32_t ret;
} CMD_ADES_EFUSE_KEY_S;

#define CESA_IOC_ADES_CREATE             0
#define CESA_IOC_ADES_DESTROY            1
#define CESA_IOC_ADES_CONFIG             2
#define CESA_IOC_ADES_PROCESS            3
#define CESA_IOC_ADES_ENCRYPT            4
#define CESA_IOC_ADES_DECRYPT            5
#define CESA_IOC_ADES_GET_INFO           6
#define CESA_IOC_ADES_EFUSE_KEY          7




/*** Result Code ***/

#define CESA_SUCCESS    0

#define CESA_INVALID_VALUE                       -1
#define CESA_IOCTL_DEV_NOT_OPEN                  -2
#define CESA_IOCTL_CMD_NOT_SUPPORT               -3

#define CESA_ADES_HDLR_GET_FAILED                -100
#define CESA_ADES_HDLR_SET_INVALID               -101
#define CESA_ADES_OPER_MODE_ERROR                -102
#define CESA_ADES_ALGO_MODE_ERROR                -103
#define CESA_ADES_WORK_MODE_ERROR                -104
#define CESA_ADES_WORK_ALGO_MODE_CONFLICT        -105
#define CESA_ADES_DATA_LENGTH_ERROR              -106
#define CESA_ADES_DMA_ADDR_ERROR                 -107
#define CESA_ADES_DMA_DONE_TIMEOUT               -108
#define CESA_ADES_KEY_SRC_ERROR                  -109



#ifdef __cplusplus
}
#endif

#endif /* __FH_CESA_H__ */

