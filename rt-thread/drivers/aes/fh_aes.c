/**
 * Copyright (c) 2015-2019 Shanghai Fullhan Microelectronics Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 *
 * Change Logs:
 * Date           Author       Notes
 * 2019-04-12     zhangy       the first version
 */

/*****************************************************************************
 *  Include Section
 *  add all #include here
 *****************************************************************************/

#include <rthw.h>
#include "fh_aes.h"
#include "board_info.h"
#include "fh_def.h"
#include <rtthread.h>
// #include "interrupt.h"
#include "fh_clock.h"
/* #include "pipe_mem.h" */
#ifdef CPU_ARC
#include "arc6xx.h"
#endif
/*****************************************************************************
 * Define section
 * add all #define here
 *****************************************************************************/
/* #define AES_DEBUG */
#ifdef AES_DEBUG
#define AES_DBG(fmt, args...)     \
    do                                  \
    {                                   \
        rt_kprintf("[AES]: ");   \
        rt_kprintf(fmt, ## args);       \
    }                                   \
    while (0)
#else
#define AES_DBG(fmt, args...)  do { } while (0)
#endif

#define AES_ERR(fmt, args...)     \
    do                                  \
    {                                   \
        rt_kprintf("[AES ERROR]: ");   \
        rt_kprintf(fmt, ## args);       \
    }                                   \
    while (0)

#define CRYPTO_QUEUE_LEN    (1)
#define CRYPTION_POS        (0)
#define METHOD_POS          (1)
#define EMODE_POS           (4)
#define EFUSE_TIMEOUT           100000

#define __raw_writeb(v, a)       (*(volatile unsigned char  *)(a) = (v))
#define __raw_writew(v, a)       (*(volatile unsigned short *)(a) = (v))
#define __raw_writel(v, a)       (*(volatile unsigned int   *)(a) = (v))

#define __raw_readb(a)          (*(volatile unsigned char  *)(a))
#define __raw_readw(a)          (*(volatile unsigned short *)(a))
#define __raw_readl(a)          (*(volatile unsigned int   *)(a))

#define aes_readl(aes, name) \
    __raw_readl(&(((struct fh_aes_reg *)aes->regs)->name))

#define aes_writel(aes, name, val) \
    __raw_writel((val), &(((struct fh_aes_reg *)aes->regs)->name))

#define aes_readw(aes, name) \
    __raw_readw(&(((struct fh_aes_reg *)aes->regs)->name))

#define aes_writew(aes, name, val) \
    __raw_writew((val), &(((struct fh_aes_reg *)aes->regs)->name))

#define aes_readb(aes, name) \
    __raw_readb(&(((struct fh_aes_reg *)aes->regs)->name))

#define aes_writeb(aes, name, val) \
    __raw_writeb((val), &(((struct fh_aes_reg *)aes->regs)->name))

#define wrap_readl(wrap, name) \
    __raw_readl(&(((struct wrap_efuse_reg *)wrap->regs)->name))

#define wrap_writel(wrap, name, val) \
    __raw_writel((val), &(((struct wrap_efuse_reg *)wrap->regs)->name))

#define wrap_readw(wrap, name) \
    __raw_readw(&(((struct wrap_efuse_reg *)wrap->regs)->name))

#define wrap_writew(wrap, name, val) \
    __raw_writew((val), &(((struct wrap_efuse_reg *)wrap->regs)->name))

#define wrap_readb(wrap, name) \
    __raw_readb(&(((struct wrap_efuse_reg *)wrap->regs)->name))

#define wrap_writeb(wrap, name, val) \
    __raw_writeb((val), &(((struct wrap_efuse_reg *)wrap->regs)->name))

#define ASSERT(expr) if (!(expr)) { \
        rt_kprintf("Assertion failed! %s:line %d\n", \
        __func__, __LINE__); \
        while (1)   \
           ;       \
        }

#define DMA_MAX_PROCESS_SIZE            2048

struct wrap_efuse_reg
{
    rt_uint32_t efuse_cmd;              /* 0x0 */
    rt_uint32_t efuse_config;           /* 0x4 */
    rt_uint32_t efuse_match_key;        /* 0x8 */
    rt_uint32_t efuse_timing0;          /* 0xc */
    rt_uint32_t efuse_timing1;          /* 0x10 */
    rt_uint32_t efuse_timing2;          /* 0x14 */
    rt_uint32_t efuse_timing3;          /* 0x18 */
    rt_uint32_t efuse_timing4;          /* 0x1c */
    rt_uint32_t efuse_timing5;          /* 0x20 */
    rt_uint32_t efuse_timing6;          /* 0x24 */
    rt_uint32_t efuse_dout;             /* 0x28 */
    rt_uint32_t efuse_status0;          /* 0x2c */
    rt_uint32_t efuse_status1;          /* 0x30 */
    rt_uint32_t efuse_status2;          /* 0x34 */
    rt_uint32_t efuse_status3;          /* 0x38 */
    rt_uint32_t efuse_status4;          /* 0x3c */
    rt_uint32_t efuse_mem_info;
};

struct wrap_efuse_obj s_efuse_obj = { 0 };
#define EFUSE_MAX_ENTRY         60
#define EFUSE_DEBUG_ENTRY_NO_MIN			60
#define EFUSE_DEBUG_ENTRY_NO_MAX			62

#if defined CONFIG_ARCH_FH8626V100
#define MATCH_KEY    0x53d485a7
#else
#define MATCH_KEY    0x92fc0025
#endif

#define SECURE_BOOT_ENTRY    (48)
#define SECURE_BOOT_ACTIVE_VAL    (1<<0)
#define SOC_BOOT_MODE_WITH_SECURE    0xaa5555aa
#define SECURE_PROTECT_EFUSE_ENTRY_MAX_INDEX    16
/****************************************************************************
 * ADT section
 *  add definition of user defined Data Type that only be used in this file  here
 ***************************************************************************/
enum {
    CMD_TRANS_AESKEY = 4,
    CMD_WFLGA_AUTO = 8,
};

enum {
    ENCRYPT = 0 << CRYPTION_POS,
    DECRYPT = 1 << CRYPTION_POS,
};

enum {
    ECB_MODE = 0 << EMODE_POS,
    CBC_MODE = 1 << EMODE_POS,
    CTR_MODE = 2 << EMODE_POS,
    CFB_MODE = 4 << EMODE_POS,
    OFB_MODE = 5 << EMODE_POS,
};

enum {
    DES_METHOD = 0 << METHOD_POS,
    TRIPLE_DES_METHOD = 1 << METHOD_POS,
    AES_128_METHOD = 4 << METHOD_POS,
    AES_192_METHOD = 5 << METHOD_POS,
    AES_256_METHOD = 6 << METHOD_POS,
};

#define EFUSE_ERR_WERR          1<<0
#define EFUSE_ERR_RERR          1<<1
#define EFUSE_ERR_AESERR        1<<2
#define EFUSE_ERR_CMDERR        1<<3
#define EFUSE_ERR_OPERR         1<<4
#define EFUSE_ERR_MATCHERR      1<<5
#define EFUSE_ERR_READLOCK      1<<6
/*****************************************************************************

 *  static fun;
 *****************************************************************************/
static int fh_aes_set_key(struct rt_crypto_obj *obj_p, struct rt_crypto_request *request_p);
static int fh_aes_encrypt(struct rt_crypto_obj *obj_p, struct rt_crypto_request *request_p);
static int fh_aes_decrypt(struct rt_crypto_obj *obj_p, struct rt_crypto_request *request_p);
static void fh_set_aes_key_reg(struct rt_crypto_obj *obj_p, struct rt_crypto_request *request_p);
static int fh_aes_cbc_encrypt(struct rt_crypto_obj *obj_p, struct rt_crypto_request *request_p);
static int fh_aes_cbc_decrypt(struct rt_crypto_obj *obj_p, struct rt_crypto_request *request_p);
static int fh_aes_ctr_encrypt(struct rt_crypto_obj *obj_p,struct rt_crypto_request *request_p);
static int fh_aes_ctr_decrypt(struct rt_crypto_obj *obj_p,struct rt_crypto_request *request_p);
static int fh_aes_ofb_encrypt(struct rt_crypto_obj *obj_p,struct rt_crypto_request *request_p);
static int fh_aes_ofb_decrypt(struct rt_crypto_obj *obj_p,struct rt_crypto_request *request_p);
static int fh_aes_cfb_encrypt(struct rt_crypto_obj *obj_p,struct rt_crypto_request *request_p);
static int fh_aes_cfb_decrypt(struct rt_crypto_obj *obj_p,struct rt_crypto_request *request_p);
static int fh_des_ecb_encrypt(struct rt_crypto_obj *obj_p,struct rt_crypto_request *request_p);
static int fh_des_ecb_decrypt(struct rt_crypto_obj *obj_p,struct rt_crypto_request *request_p);
static int fh_des_cbc_encrypt(struct rt_crypto_obj *obj_p,struct rt_crypto_request *request_p);
static int fh_des_cbc_decrypt(struct rt_crypto_obj *obj_p,struct rt_crypto_request *request_p);
static int fh_des_ofb_encrypt(struct rt_crypto_obj *obj_p,struct rt_crypto_request *request_p);
static int fh_des_ofb_decrypt(struct rt_crypto_obj *obj_p,struct rt_crypto_request *request_p);
static int fh_des_cfb_encrypt(struct rt_crypto_obj *obj_p,struct rt_crypto_request *request_p);
static int fh_des_cfb_decrypt(struct rt_crypto_obj *obj_p,struct rt_crypto_request *request_p);
static int fh_des_tri_ecb_encrypt(struct rt_crypto_obj *obj_p,struct rt_crypto_request *request_p);
static int fh_des_tri_ecb_decrypt(struct rt_crypto_obj *obj_p,struct rt_crypto_request *request_p);
static int fh_des_tri_cbc_encrypt(struct rt_crypto_obj *obj_p,struct rt_crypto_request *request_p);
static int fh_des_tri_cbc_decrypt(struct rt_crypto_obj *obj_p,struct rt_crypto_request *request_p);
static int fh_des_tri_ofb_encrypt(struct rt_crypto_obj *obj_p,struct rt_crypto_request *request_p);
static int fh_des_tri_ofb_decrypt(struct rt_crypto_obj *obj_p,struct rt_crypto_request *request_p);
static int fh_des_tri_cfb_encrypt(struct rt_crypto_obj *obj_p,struct rt_crypto_request *request_p);
static int fh_des_tri_cfb_decrypt(struct rt_crypto_obj *obj_p,struct rt_crypto_request *request_p);

static void aes_biglittle_swap(rt_uint8_t *buf);
void efuse_trans_key(struct fh_aes_driver *p_driver,struct rt_crypto_request *request_p);
void open_efuse_power(struct wrap_efuse_obj *obj);
void efuse_buffer_set(struct wrap_efuse_obj *obj, unsigned int value);
void refresh_efuse(struct wrap_efuse_obj *obj);
void efuse_read_entry(struct wrap_efuse_obj *obj, rt_uint32_t key, rt_uint32_t start_entry, rt_uint8_t *buff,
rt_uint32_t size);
int fh_efuse_secure_check(struct fh_aes_driver *p_driver, struct wrap_efuse_obj *obj,
unsigned int start_no, unsigned int size, struct rt_crypto_request *request_p);
/*****************************************************************************
 * Global variables section - Local
 * define global variables(will be refered only in this file) here,
 * static keyword should be used to limit scope of local variable to this file
 * e.g.
 *  static uint8_t ufoo;
 *****************************************************************************/
extern void mmu_clean_invalidated_dcache(rt_uint32_t buffer, rt_uint32_t size);



struct rt_crypto_obj crypto_table[] = {
        {
                .name = "aes-ecb",
                .flag = RT_FALSE,
                .block_size = 16,
                .src_allign_size = 4,
                .dst_allign_size = 4,
                .max_key_size = AES_MAX_KEY_SIZE,
                .min_key_size = AES_MIN_KEY_SIZE,
                .otp_size = DMA_MAX_PROCESS_SIZE,
                .ops.set_key = fh_aes_set_key,
                .ops.encrypt = fh_aes_encrypt,
                .ops.decrypt = fh_aes_decrypt,
        },
        {
                .name = "aes-cbc",
                .flag = RT_TRUE,
                .block_size = 16,
                .src_allign_size = 4,
                .dst_allign_size = 4,
                .max_key_size = AES_MAX_KEY_SIZE,
                .min_key_size = AES_MIN_KEY_SIZE,
                .otp_size = DMA_MAX_PROCESS_SIZE,
                .ops.set_key = fh_aes_set_key,
                .ops.encrypt = fh_aes_cbc_encrypt,
                .ops.decrypt = fh_aes_cbc_decrypt,
        },
        {
                .name = "aes-ctr",
                .flag = RT_TRUE,
                .block_size = 16,
                .src_allign_size = 4,
                .dst_allign_size = 4,
                .max_key_size = AES_MAX_KEY_SIZE,
                .min_key_size = AES_MIN_KEY_SIZE,
                .otp_size = DMA_MAX_PROCESS_SIZE,
                .ops.set_key =fh_aes_set_key,
                .ops.encrypt = fh_aes_ctr_encrypt,
                .ops.decrypt = fh_aes_ctr_decrypt,
        },
        {
                .name = "aes-ofb",
                .flag = RT_TRUE,
                .block_size = 16,
                .src_allign_size = 4,
                .dst_allign_size = 4,
                .max_key_size = AES_MAX_KEY_SIZE,
                .min_key_size = AES_MIN_KEY_SIZE,
                .otp_size = DMA_MAX_PROCESS_SIZE,
                .ops.set_key =fh_aes_set_key,
                .ops.encrypt = fh_aes_ofb_encrypt,
                .ops.decrypt = fh_aes_ofb_decrypt,
        },
        {
                .name = "aes-cfb",
                .flag = RT_TRUE,
                .block_size = 16,
                .src_allign_size = 4,
                .dst_allign_size = 4,
                .max_key_size = AES_MAX_KEY_SIZE,
                .min_key_size = AES_MIN_KEY_SIZE,
                .otp_size = DMA_MAX_PROCESS_SIZE,
                .ops.set_key =fh_aes_set_key,
                .ops.encrypt = fh_aes_cfb_encrypt,
                .ops.decrypt = fh_aes_cfb_decrypt,
        },
        {
                .name = "des-ecb",
                .flag = RT_FALSE,
                .block_size = 16,
                .src_allign_size = 4,
                .dst_allign_size = 4,
                .max_key_size = DES_KEY_SIZE,
                .min_key_size = DES_KEY_SIZE,
                .otp_size = DMA_MAX_PROCESS_SIZE,
                .ops.set_key =fh_aes_set_key,
                .ops.encrypt = fh_des_ecb_encrypt,
                .ops.decrypt = fh_des_ecb_decrypt,
        },
        {
                .name = "des-cbc",
                .flag = RT_TRUE,
                .block_size = 8,
                .src_allign_size = 4,
                .dst_allign_size = 4,
                .max_key_size = DES_KEY_SIZE,
                .min_key_size = DES_KEY_SIZE,
                .otp_size = DMA_MAX_PROCESS_SIZE,
                .ops.set_key =fh_aes_set_key,
                .ops.encrypt = fh_des_cbc_encrypt,
                .ops.decrypt = fh_des_cbc_decrypt,
        },
        {
                .name = "des-ofb",
                .flag = RT_TRUE,
                .block_size = 8,
                .src_allign_size = 4,
                .dst_allign_size = 4,
                .max_key_size = DES_KEY_SIZE,
                .min_key_size = DES_KEY_SIZE,
                .otp_size = DMA_MAX_PROCESS_SIZE,
                .ops.set_key =fh_aes_set_key,
                .ops.encrypt = fh_des_ofb_encrypt,
                .ops.decrypt = fh_des_ofb_decrypt,
        },
        {
                .name = "des-cfb",
                .flag = RT_TRUE,
                .block_size = 8,
                .src_allign_size = 4,
                .dst_allign_size = 4,
                .max_key_size = DES_KEY_SIZE,
                .min_key_size = DES_KEY_SIZE,
                .otp_size = DMA_MAX_PROCESS_SIZE,
                .ops.set_key =fh_aes_set_key,
                .ops.encrypt = fh_des_cfb_encrypt,
                .ops.decrypt = fh_des_cfb_decrypt,
        },

        {
                .name = "des3-ecb",
                .flag = RT_FALSE,
                .block_size = 8,
                .src_allign_size = 4,
                .dst_allign_size = 4,
                .max_key_size = DES3_EDE_KEY_SIZE,
                .min_key_size = DES3_EDE_KEY_SIZE,
                .otp_size = DMA_MAX_PROCESS_SIZE,
                .ops.set_key =fh_aes_set_key,
                .ops.encrypt = fh_des_tri_ecb_encrypt,
                .ops.decrypt = fh_des_tri_ecb_decrypt,
        },

        {
                .name = "des3-cbc",
                .flag = RT_TRUE,
                .block_size = 8,
                .src_allign_size = 4,
                .dst_allign_size = 4,
                .max_key_size = DES3_EDE_KEY_SIZE,
                .min_key_size = DES3_EDE_KEY_SIZE,
                .otp_size = DMA_MAX_PROCESS_SIZE,
                .ops.set_key =fh_aes_set_key,
                .ops.encrypt = fh_des_tri_cbc_encrypt,
                .ops.decrypt = fh_des_tri_cbc_decrypt,
        },


        {
                .name = "des3-ofb",
                .flag = RT_TRUE,
                .block_size = 8,
                .src_allign_size = 4,
                .dst_allign_size = 4,
                .max_key_size = DES3_EDE_KEY_SIZE,
                .min_key_size = DES3_EDE_KEY_SIZE,
                .otp_size = DMA_MAX_PROCESS_SIZE,
                .ops.set_key =fh_aes_set_key,
                .ops.encrypt = fh_des_tri_ofb_encrypt,
                .ops.decrypt = fh_des_tri_ofb_decrypt,
        },

        {
                .name = "des3-cfb",
                .flag = RT_TRUE,
                .block_size = 8,
                .src_allign_size = 4,
                .dst_allign_size = 4,
                .max_key_size = DES3_EDE_KEY_SIZE,
                .min_key_size = DES3_EDE_KEY_SIZE,
                .otp_size = DMA_MAX_PROCESS_SIZE,
                .ops.set_key =fh_aes_set_key,
                .ops.encrypt = fh_des_tri_cfb_encrypt,
                .ops.decrypt = fh_des_tri_cfb_decrypt,
        },
};

/*if use (encrypt & secure boot & use efuse 0~15 entry). return err*/
int fh_aes_secure_check(struct fh_aes_driver *p_driver, struct rt_crypto_request *request_p)
{
    if (request_p->control_mode & DECRYPT)
    {
        return 0;
    }
    return fh_efuse_secure_check(p_driver, &s_efuse_obj,
    s_efuse_obj.trans_key_start_no,
    s_efuse_obj.trans_key_size, request_p);
}

static void fh_set_aes_key_reg(struct rt_crypto_obj *obj_p, struct rt_crypto_request *request_p)
{
    struct fh_aes_driver *p_driver;
    int i;
    rt_uint32_t method;
    rt_uint8_t *key;
    rt_uint32_t temp_key_buf[32];
    rt_uint32_t temp_iv_buf[32];
    rt_uint32_t *p_dst = NULL;
    rt_uint32_t key_size = 0;
    key = request_p->key;
    p_driver = (struct fh_aes_driver *) obj_p->driver_private;
    if (request_p->iv_flag == RT_TRUE)
    {
        /* set iv */
        /* if aes mode ....set 128 bit iv,  des set 64bit iv.. */
        AES_DBG("set iv reg\n");
        if ((request_p->control_mode & AES_128_METHOD)
                || ((request_p->control_mode & AES_192_METHOD))
                || (request_p->control_mode & AES_256_METHOD))
        {
            ASSERT(request_p->iv != RT_NULL);
            AES_DBG("aes iv mode...\n");
            rt_memcpy((rt_uint8_t *) &temp_iv_buf[0], request_p->iv, 16);
            p_dst = &temp_iv_buf[0];
            for (i = 0; i < 16 / sizeof(rt_uint32_t); i++)
            {
                aes_biglittle_swap((rt_uint8_t *) (p_dst + i));

            }
            rt_memcpy((rt_uint8_t *) &((struct fh_aes_reg *) p_driver->regs)->initial_vector0,
                    temp_iv_buf, 16);
#ifdef CPU_ARC
            fh_hw_flush_dcache((rt_uint32_t) &((struct fh_aes_reg *) p_driver->regs)->initial_vector0, 16);
#else
            mmu_clean_invalidated_dcache((rt_uint32_t) &((struct fh_aes_reg *) p_driver->regs)->initial_vector0, 16);
#endif

        }
        else
        {
            AES_DBG("des iv mode...\n");

            rt_memcpy((rt_uint8_t *) &temp_iv_buf[0], request_p->iv, 8);
            p_dst = &temp_iv_buf[0];
            for (i = 0; i < 8 / sizeof(rt_uint32_t); i++)
            {
                aes_biglittle_swap((rt_uint8_t *) (p_dst + i));
            }
            rt_memcpy((rt_uint8_t *) &((struct fh_aes_reg *) p_driver->regs)->initial_vector0,
                    temp_iv_buf, 8);
#ifdef CPU_ARC
            fh_hw_flush_dcache((rt_uint32_t) &((struct fh_aes_reg *) p_driver->regs)->initial_vector0, 8);
#else
            mmu_clean_invalidated_dcache((rt_uint32_t) &((struct fh_aes_reg *) p_driver->regs)->initial_vector0, 8);
#endif
        }
    }
    /* set key... */
    method = request_p->control_mode & 0x0e;
    AES_DBG("set key reg\n");
    switch (method)
    {
    case AES_128_METHOD:
        AES_DBG("set key aes 128 mode..\n");

        key_size = 16;
        break;
    case AES_192_METHOD:
        AES_DBG("set key aes 192 mode..\n");

        key_size = 24;
        break;

    case AES_256_METHOD:
        AES_DBG("set key aes 256 mode..\n");

        key_size = 32;

        break;

    case DES_METHOD:
        AES_DBG("set key des normal mode..\n");

        key_size = 8;
        break;

    case TRIPLE_DES_METHOD:
        AES_DBG("set key des triple mode..\n");

        key_size = 24;

        break;

    default:
        AES_DBG("error method!!\n");
        break;
    }
    ASSERT(key_size == request_p->key_size);
    if ((request_p->key_flag & CRYPTO_EX_MEM_SET_KEY) &&
    (p_driver->open_flag & CRYPTO_EX_MEM_SET_KEY))
    {
        AES_DBG("use ex mem transkey..\n");
        if (fh_aes_secure_check(p_driver, request_p))
        {
            rt_kprintf("secure check failed..\n");
            return;
        }
        efuse_trans_key(p_driver,request_p);
    }
    else
    {
        AES_DBG("use cpu transkey..\n");
        p_driver->old_usr_request.key_flag &= ~CRYPTO_EX_MEM_SET_KEY;
        p_driver->old_usr_request.key_flag |= CRYPTO_CPU_SET_KEY;
        rt_memcpy((rt_uint8_t *) &temp_key_buf[0], key, key_size);
        p_dst = &temp_key_buf[0];
        for (i = 0; i < key_size / sizeof(rt_uint32_t); i++)
        {
            aes_biglittle_swap((rt_uint8_t *) (p_dst + i));

        }
        rt_memcpy((rt_uint8_t *) &((struct fh_aes_reg *) p_driver->regs)->security_key0, temp_key_buf,
                key_size);

#ifdef CPU_ARC
        fh_hw_flush_dcache((rt_uint32_t) &((struct fh_aes_reg *) p_driver->regs)->security_key0, key_size);
#else
        mmu_clean_invalidated_dcache((rt_uint32_t) &((struct fh_aes_reg *) p_driver->regs)->security_key0, key_size);
#endif

    }
}

static void aes_biglittle_swap(rt_uint8_t *buf)
{
    rt_uint8_t tmp, tmp1;

    tmp = buf[0];
    tmp1 = buf[1];
    buf[0] = buf[3];
    buf[1] = buf[2];
    buf[2] = tmp1;
    buf[3] = tmp;
}

static int fh_set_indata(struct rt_crypto_obj *obj_p, struct rt_crypto_request *request_p)
{

    /* mmu_clean_invalidated_dcache(request_p->data_src, 128); */
#ifdef CPU_ARC
    fh_hw_flush_dcache((rt_uint32_t) (request_p->data_src), request_p->data_size);
#else
    mmu_clean_invalidated_dcache((rt_uint32_t) (request_p->data_src), request_p->data_size);
#endif
    return 0;
}

static int fh_set_outdata(struct rt_crypto_obj *obj_p, struct rt_crypto_request *request_p)
{
    mmu_clean_invalidated_dcache((rt_uint32_t) (request_p->data_dst), request_p->data_size);
    return 0;
}

static void fh_aes_update_request_data_size(struct rt_crypto_request *request_p,
rt_uint32_t max_xfer_size, rt_uint32_t *total_size, rt_uint32_t *first)
{
    rt_uint32_t step_size = 0;

    step_size = MIN(max_xfer_size, *total_size);
    if (*first == 0)
    {
        *first = 1;
    }
    *total_size -= step_size;
    request_p->data_size = step_size;
}


static void fh_aes_update_request_data_add(struct rt_crypto_request *request_p)
{
    request_p->data_src += request_p->data_size;
    request_p->data_dst += request_p->data_size;
}

/* static void fh_unset_indata(struct rt_crypto_obj *obj_p, struct rt_crypto_request *request_p) */
/* { */
/*  */
/* } */

/* static void fh_unset_outdata(struct rt_crypto_obj *obj_p, struct rt_crypto_request *request_p) */
/* { */
/*  */
/* } */

static void fh_set_dma_indata(struct rt_crypto_obj *obj_p, struct rt_crypto_request *request_p)
{
    struct fh_aes_driver *p_driver;

    p_driver = (struct fh_aes_driver *) obj_p->driver_private;
    aes_writel(p_driver, dma_src_add, (rt_uint32_t)request_p->data_src);
    AES_DBG("dma indata add is :%x\n", (rt_uint32_t)request_p->data_src);

    aes_writel(p_driver, dma_trans_size, request_p->data_size);
    AES_DBG("dma trans size is :%x\n", request_p->data_size);
}

static void fh_set_dma_outdata(struct rt_crypto_obj *obj_p, struct rt_crypto_request *request_p)
{
    struct fh_aes_driver *p_driver;

    p_driver = (struct fh_aes_driver *) obj_p->driver_private;
    aes_writel(p_driver, dma_dst_add, (rt_uint32_t)request_p->data_dst);
    AES_DBG("dma outdata add is :%x\n", (rt_uint32_t)request_p->data_dst);
    aes_writel(p_driver, dma_trans_size, request_p->data_size);
    AES_DBG("dma trans size is :%x\n", request_p->data_size);
}

static void fh_aes_handle_request(struct rt_crypto_obj *obj_p, struct rt_crypto_request *request_p)
{

    struct fh_aes_driver *p_driver;
    rt_uint32_t *mode;
    rt_uint32_t outfifo_thold;
    rt_uint32_t infifo_thold;
    rt_uint32_t isr;
    rt_uint32_t ret;
    rt_uint32_t total_size;
    rt_uint32_t first_in = 0;

    p_driver = (struct fh_aes_driver *) obj_p->driver_private;

    rt_sem_take(&p_driver->lock, RT_WAITING_FOREVER);

    total_size = request_p->data_size;
    if(!(request_p->key_flag <= p_driver->open_flag)){
        AES_ERR("request flag : %x; driver flag:%x\n",request_p->key_flag,p_driver->open_flag);
        ASSERT(request_p->key_flag <= p_driver->open_flag);
    }
    while (total_size != 0)
    {
        rt_completion_init(&p_driver->transfer_completion);
        /* explain the software para(mode) to the hardware para */
        mode = &request_p->control_mode;
        if (((*mode & CBC_MODE) || (*mode & CTR_MODE) || (*mode & CFB_MODE)
                || (*mode & OFB_MODE)) && (first_in == 0))
        {
            *mode |= 1 << 7;
            request_p->iv_flag = RT_TRUE;
        }
        else
        {
            *mode &= ~(1 << 7);
            request_p->iv_flag = RT_FALSE;
        }
        if (first_in == 0)
            fh_set_aes_key_reg(obj_p, request_p);
        fh_aes_update_request_data_size(request_p, obj_p->otp_size, &total_size, &first_in);
        /* emode & method */
        /* iv & key already get... */
        /* fifo threshold .. */
        outfifo_thold = 0;
        infifo_thold = 8;
        isr = p_driver->irq_en;
        /* here set the hardware reg.....lock now.... */
        /* set key... */
        p_driver->cur_request = request_p;
        p_driver->cur_crypto = obj_p;
        aes_writel(p_driver, encrypt_control, *mode);
        fh_set_indata(obj_p, request_p);
        fh_set_outdata(obj_p, request_p);

        /* set hw para... */
        fh_set_dma_indata(obj_p, request_p);
        fh_set_dma_outdata(obj_p, request_p);

        /* set fifo.. */
        AES_DBG("outfifo thold:%x\n", outfifo_thold);
        AES_DBG("infifo thold:%x\n", infifo_thold);
        aes_writel(p_driver, fifo_threshold, outfifo_thold << 8 | infifo_thold);

        /* set isr.. */
        AES_DBG("intr enable:%x\n", isr);
        aes_writel(p_driver, intr_enable, isr);
        /* enable dma go.. */
        aes_writel(p_driver, dma_control, 1);
        ret = rt_completion_wait(&p_driver->transfer_completion, RT_TICK_PER_SECOND * 20);
        if (ret)
        {
            AES_ERR("%s, transfer timeout\n", __func__);
            break;
        }

        /* mmu_clean_invalidated_dcache(request_p->data_dst,  request_p->data_size); */
    #ifdef CPU_ARC
        fh_hw_invalidate_dcache((rt_uint32_t)request_p->data_dst, request_p->data_size);
    #else
        mmu_clean_invalidated_dcache((rt_uint32_t)request_p->data_dst, request_p->data_size);
    #endif
        fh_aes_update_request_data_add(request_p);
    }

    rt_sem_release(&p_driver->lock);

}

static int fh_aes_set_key(struct rt_crypto_obj *obj_p, struct rt_crypto_request *request_p)
{
    return 0;
}

static int fh_aes_encrypt(struct rt_crypto_obj *obj_p, struct rt_crypto_request *request_p)
{

    struct fh_aes_driver *p_driver;

    p_driver = (struct fh_aes_driver *) obj_p->driver_private;

    switch (request_p->key_size)
    {
    case AES_KEYSIZE_128:
        p_driver->control_mode = AES_128_METHOD;
        request_p->control_mode = AES_128_METHOD;
        break;
    case AES_KEYSIZE_192:
        p_driver->control_mode = AES_192_METHOD;
        request_p->control_mode = AES_192_METHOD;
        break;
    case AES_KEYSIZE_256:
        p_driver->control_mode = AES_256_METHOD;
        request_p->control_mode = AES_256_METHOD;
        break;
    default:
        AES_ERR("%s wrong key size..\n", __func__);
        ASSERT(0)
        ;
        break;
    }
    p_driver->control_mode |= ECB_MODE | ENCRYPT;
    request_p->control_mode |= ECB_MODE | ENCRYPT;
    fh_aes_handle_request(obj_p, request_p);
    return 0;

}

static int fh_aes_decrypt(struct rt_crypto_obj *obj_p, struct rt_crypto_request *request_p)
{

    struct fh_aes_driver *p_driver;

    p_driver = (struct fh_aes_driver *) obj_p->driver_private;

    switch (request_p->key_size)
    {
    case AES_KEYSIZE_128:
        p_driver->control_mode = AES_128_METHOD;
        request_p->control_mode = AES_128_METHOD;
        break;
    case AES_KEYSIZE_192:
        p_driver->control_mode = AES_192_METHOD;
        request_p->control_mode = AES_192_METHOD;
        break;
    case AES_KEYSIZE_256:
        p_driver->control_mode = AES_256_METHOD;
        request_p->control_mode = AES_256_METHOD;
        break;
    default:
        AES_ERR("%s wrong key size..\n", __func__);
        ASSERT(0)
        ;
    }
    p_driver->control_mode |= ECB_MODE | DECRYPT;
    request_p->control_mode |= ECB_MODE | DECRYPT;
    fh_aes_handle_request(obj_p, request_p);

    return 0;

}

static int fh_aes_cbc_encrypt(struct rt_crypto_obj *obj_p, struct rt_crypto_request *request_p)
{

    struct fh_aes_driver *p_driver;

    p_driver = (struct fh_aes_driver *) obj_p->driver_private;

    switch (request_p->key_size)
    {
    case AES_KEYSIZE_128:
        p_driver->control_mode = AES_128_METHOD;
        request_p->control_mode = AES_128_METHOD;
        break;
    case AES_KEYSIZE_192:
        p_driver->control_mode = AES_192_METHOD;
        request_p->control_mode = AES_192_METHOD;
        break;
    case AES_KEYSIZE_256:
        p_driver->control_mode = AES_256_METHOD;
        request_p->control_mode = AES_256_METHOD;
        break;
    default:
        AES_ERR("%s wrong key size..\n", __func__);
        ASSERT(0)
        ;
        break;
    }
    p_driver->control_mode |= CBC_MODE | ENCRYPT;
    request_p->control_mode |= CBC_MODE | ENCRYPT;

    fh_aes_handle_request(obj_p, request_p);
    return 0;

}

static int fh_aes_cbc_decrypt(struct rt_crypto_obj *obj_p, struct rt_crypto_request *request_p)
{
    struct fh_aes_driver *p_driver;

    p_driver = (struct fh_aes_driver *) obj_p->driver_private;

    switch (request_p->key_size)
    {
    case AES_KEYSIZE_128:
        p_driver->control_mode = AES_128_METHOD;
        request_p->control_mode = AES_128_METHOD;
        break;
    case AES_KEYSIZE_192:
        p_driver->control_mode = AES_192_METHOD;
        request_p->control_mode = AES_192_METHOD;
        break;
    case AES_KEYSIZE_256:
        p_driver->control_mode = AES_256_METHOD;
        request_p->control_mode = AES_256_METHOD;
        break;
    default:
        AES_ERR("%s wrong key size..\n", __func__);
        ASSERT(0)
        ;
        break;
    }
    p_driver->control_mode |= CBC_MODE | DECRYPT;
    request_p->control_mode |= CBC_MODE | DECRYPT;
    fh_aes_handle_request(obj_p, request_p);
    return 0;
}


static int fh_aes_ctr_encrypt(struct rt_crypto_obj *obj_p,struct rt_crypto_request *request_p)
{
    struct fh_aes_driver *p_driver;

    p_driver = (struct fh_aes_driver *)obj_p->driver_private;

    switch (request_p->key_size)
    {
    case AES_KEYSIZE_128:
        p_driver->control_mode = AES_128_METHOD;
        request_p->control_mode = AES_128_METHOD;
        break;
    case AES_KEYSIZE_192:
        p_driver->control_mode = AES_192_METHOD;
        request_p->control_mode = AES_192_METHOD;
        break;
    case AES_KEYSIZE_256:
        p_driver->control_mode = AES_256_METHOD;
        request_p->control_mode = AES_256_METHOD;
        break;
    default:
        AES_ERR("%s wrong key size..\n",__func__);
        ASSERT(0);
        break;
    }
    p_driver->control_mode |= CTR_MODE | ENCRYPT;
    request_p->control_mode |= CTR_MODE | ENCRYPT;
    fh_aes_handle_request(obj_p,request_p);
    return 0;
}


static int fh_aes_ctr_decrypt(struct rt_crypto_obj *obj_p,struct rt_crypto_request *request_p)
{
    struct fh_aes_driver *p_driver;

    p_driver = (struct fh_aes_driver *)obj_p->driver_private;

    switch (request_p->key_size)
    {
    case AES_KEYSIZE_128:
        p_driver->control_mode = AES_128_METHOD;
        request_p->control_mode = AES_128_METHOD;
        break;
    case AES_KEYSIZE_192:
        p_driver->control_mode = AES_192_METHOD;
        request_p->control_mode = AES_192_METHOD;
        break;
    case AES_KEYSIZE_256:
        p_driver->control_mode = AES_256_METHOD;
        request_p->control_mode = AES_256_METHOD;
        break;
    default:
        AES_ERR("%s wrong key size..\n",__func__);
        ASSERT(0);
        break;
    }
    p_driver->control_mode |= CTR_MODE | DECRYPT;
    request_p->control_mode |= CTR_MODE | DECRYPT;
    fh_aes_handle_request(obj_p,request_p);
    return 0;
}


static int fh_aes_ofb_encrypt(struct rt_crypto_obj *obj_p,struct rt_crypto_request *request_p)
{
    struct fh_aes_driver *p_driver;

    p_driver = (struct fh_aes_driver *)obj_p->driver_private;

    switch (request_p->key_size)
    {
    case AES_KEYSIZE_128:
        p_driver->control_mode = AES_128_METHOD;
        request_p->control_mode = AES_128_METHOD;
        break;
    case AES_KEYSIZE_192:
        p_driver->control_mode = AES_192_METHOD;
        request_p->control_mode = AES_192_METHOD;
        break;
    case AES_KEYSIZE_256:
        p_driver->control_mode = AES_256_METHOD;
        request_p->control_mode = AES_256_METHOD;
        break;
    default:
        AES_ERR("%s wrong key size..\n",__func__);
        ASSERT(0);
        break;
    }
    p_driver->control_mode |= OFB_MODE | ENCRYPT;
    request_p->control_mode |= OFB_MODE | ENCRYPT;

    fh_aes_handle_request(obj_p,request_p);
    return 0;
}


static int fh_aes_ofb_decrypt(struct rt_crypto_obj *obj_p,struct rt_crypto_request *request_p)
{
    struct fh_aes_driver *p_driver;

    p_driver = (struct fh_aes_driver *)obj_p->driver_private;

    switch (request_p->key_size)
    {
    case AES_KEYSIZE_128:
        p_driver->control_mode = AES_128_METHOD;
        request_p->control_mode = AES_128_METHOD;
        break;
    case AES_KEYSIZE_192:
        p_driver->control_mode = AES_192_METHOD;
        request_p->control_mode = AES_192_METHOD;
        break;
    case AES_KEYSIZE_256:
        p_driver->control_mode = AES_256_METHOD;
        request_p->control_mode = AES_256_METHOD;
        break;
    default:
        AES_ERR("%s wrong key size..\n",__func__);
        ASSERT(0);
        break;
    }
    p_driver->control_mode |= OFB_MODE | DECRYPT;
    request_p->control_mode |= OFB_MODE | DECRYPT;

    fh_aes_handle_request(obj_p,request_p);
    return 0;
}



static int fh_aes_cfb_encrypt(struct rt_crypto_obj *obj_p,struct rt_crypto_request *request_p)
{
    struct fh_aes_driver *p_driver;

    p_driver = (struct fh_aes_driver *)obj_p->driver_private;

    switch (request_p->key_size)
    {
    case AES_KEYSIZE_128:
        p_driver->control_mode = AES_128_METHOD;
        request_p->control_mode = AES_128_METHOD;
        break;
    case AES_KEYSIZE_192:
        p_driver->control_mode = AES_192_METHOD;
        request_p->control_mode = AES_192_METHOD;
        break;
    case AES_KEYSIZE_256:
        p_driver->control_mode = AES_256_METHOD;
        request_p->control_mode = AES_256_METHOD;
        break;
    default:
        AES_ERR("%s wrong key size..\n",__func__);
        ASSERT(0);
        break;
    }
    p_driver->control_mode |= CFB_MODE | ENCRYPT;
    request_p->control_mode |= CFB_MODE | ENCRYPT;

    fh_aes_handle_request(obj_p,request_p);
    return 0;
}


static int fh_aes_cfb_decrypt(struct rt_crypto_obj *obj_p,struct rt_crypto_request *request_p)
{
    struct fh_aes_driver *p_driver;

    p_driver = (struct fh_aes_driver *)obj_p->driver_private;

    switch (request_p->key_size)
    {
    case AES_KEYSIZE_128:
        p_driver->control_mode = AES_128_METHOD;
        request_p->control_mode = AES_128_METHOD;
        break;
    case AES_KEYSIZE_192:
        p_driver->control_mode = AES_192_METHOD;
        request_p->control_mode = AES_192_METHOD;
        break;
    case AES_KEYSIZE_256:
        p_driver->control_mode = AES_256_METHOD;
        request_p->control_mode = AES_256_METHOD;
        break;
    default:
        AES_ERR("%s wrong key size..\n",__func__);
        ASSERT(0);
        break;
    }
    p_driver->control_mode |= CFB_MODE | DECRYPT;
    request_p->control_mode |= CFB_MODE | DECRYPT;

    fh_aes_handle_request(obj_p,request_p);
    return 0;
}



static int fh_des_ecb_encrypt(struct rt_crypto_obj *obj_p,struct rt_crypto_request *request_p)
{
    struct fh_aes_driver *p_driver;
    p_driver = (struct fh_aes_driver *)obj_p->driver_private;
    p_driver->control_mode = DES_METHOD;
    request_p->control_mode = DES_METHOD;
    p_driver->control_mode |= ECB_MODE | ENCRYPT;
    request_p->control_mode |= ECB_MODE | ENCRYPT;
    fh_aes_handle_request(obj_p,request_p);
    return 0;
}


static int fh_des_ecb_decrypt(struct rt_crypto_obj *obj_p,struct rt_crypto_request *request_p)
{
    struct fh_aes_driver *p_driver;

    p_driver = (struct fh_aes_driver *)obj_p->driver_private;
    p_driver->control_mode = DES_METHOD;
    request_p->control_mode = DES_METHOD;
    p_driver->control_mode |= ECB_MODE | DECRYPT;
    request_p->control_mode |= ECB_MODE | DECRYPT;
    fh_aes_handle_request(obj_p,request_p);
    return 0;
}


static int fh_des_cbc_encrypt(struct rt_crypto_obj *obj_p,struct rt_crypto_request *request_p)
{
    struct fh_aes_driver *p_driver;

    p_driver = (struct fh_aes_driver *)obj_p->driver_private;
    p_driver->control_mode = DES_METHOD;
    request_p->control_mode = DES_METHOD;
    p_driver->control_mode |= CBC_MODE | ENCRYPT;
    request_p->control_mode |= CBC_MODE | ENCRYPT;

    fh_aes_handle_request(obj_p,request_p);
    return 0;
}


static int fh_des_cbc_decrypt(struct rt_crypto_obj *obj_p,struct rt_crypto_request *request_p)
{
    struct fh_aes_driver *p_driver;

    p_driver = (struct fh_aes_driver *)obj_p->driver_private;
    p_driver->control_mode = DES_METHOD;
    request_p->control_mode = DES_METHOD;
    p_driver->control_mode |= CBC_MODE | DECRYPT;
    request_p->control_mode |= CBC_MODE | DECRYPT;
    fh_aes_handle_request(obj_p,request_p);
    return 0;
}


static int fh_des_ofb_encrypt(struct rt_crypto_obj *obj_p,struct rt_crypto_request *request_p)
{
    struct fh_aes_driver *p_driver;

    p_driver = (struct fh_aes_driver *)obj_p->driver_private;
    p_driver->control_mode = DES_METHOD;
    request_p->control_mode = DES_METHOD;
    p_driver->control_mode |= OFB_MODE | ENCRYPT;
    request_p->control_mode |= OFB_MODE | ENCRYPT;
    fh_aes_handle_request(obj_p,request_p);
    return 0;
}


static int fh_des_ofb_decrypt(struct rt_crypto_obj *obj_p,struct rt_crypto_request *request_p)
{
    struct fh_aes_driver *p_driver;

    p_driver = (struct fh_aes_driver *)obj_p->driver_private;
    p_driver->control_mode = DES_METHOD;
    request_p->control_mode = DES_METHOD;
    p_driver->control_mode |= OFB_MODE | DECRYPT;
    request_p->control_mode |= OFB_MODE | DECRYPT;
    fh_aes_handle_request(obj_p,request_p);
    return 0;
}


static int fh_des_cfb_encrypt(struct rt_crypto_obj *obj_p,struct rt_crypto_request *request_p)
{
    struct fh_aes_driver *p_driver;

    p_driver = (struct fh_aes_driver *)obj_p->driver_private;
    p_driver->control_mode = DES_METHOD;
    request_p->control_mode = DES_METHOD;
    p_driver->control_mode |= CFB_MODE | ENCRYPT;
    request_p->control_mode |= CFB_MODE | ENCRYPT;
    fh_aes_handle_request(obj_p,request_p);
    return 0;
}


static int fh_des_cfb_decrypt(struct rt_crypto_obj *obj_p,struct rt_crypto_request *request_p)
{
    struct fh_aes_driver *p_driver;

    p_driver = (struct fh_aes_driver *)obj_p->driver_private;
    p_driver->control_mode = DES_METHOD;
    request_p->control_mode = DES_METHOD;
    p_driver->control_mode |= CFB_MODE | DECRYPT;
    request_p->control_mode |= CFB_MODE | DECRYPT;
    fh_aes_handle_request(obj_p,request_p);
    return 0;
}


static int fh_des_tri_ecb_encrypt(struct rt_crypto_obj *obj_p,struct rt_crypto_request *request_p)
{
    struct fh_aes_driver *p_driver;

    p_driver = (struct fh_aes_driver *)obj_p->driver_private;
    p_driver->control_mode = TRIPLE_DES_METHOD;
    request_p->control_mode = TRIPLE_DES_METHOD;
    p_driver->control_mode |= ECB_MODE | ENCRYPT;
    request_p->control_mode |= ECB_MODE | ENCRYPT;
    fh_aes_handle_request(obj_p,request_p);
    return 0;
}


static int fh_des_tri_ecb_decrypt(struct rt_crypto_obj *obj_p,struct rt_crypto_request *request_p)
{
    struct fh_aes_driver *p_driver;

    p_driver = (struct fh_aes_driver *)obj_p->driver_private;
    p_driver->control_mode = TRIPLE_DES_METHOD;
    request_p->control_mode = TRIPLE_DES_METHOD;
    p_driver->control_mode |= ECB_MODE | DECRYPT;
    request_p->control_mode |= ECB_MODE | DECRYPT;
    fh_aes_handle_request(obj_p,request_p);
    return 0;
}

static int fh_des_tri_cbc_encrypt(struct rt_crypto_obj *obj_p,struct rt_crypto_request *request_p)
{
    struct fh_aes_driver *p_driver;

    p_driver = (struct fh_aes_driver *)obj_p->driver_private;
    p_driver->control_mode = TRIPLE_DES_METHOD;
    request_p->control_mode = TRIPLE_DES_METHOD;
    p_driver->control_mode |= CBC_MODE | ENCRYPT;
    request_p->control_mode |= CBC_MODE | ENCRYPT;
    fh_aes_handle_request(obj_p,request_p);
    return 0;

}


static int fh_des_tri_cbc_decrypt(struct rt_crypto_obj *obj_p,struct rt_crypto_request *request_p)
{
    struct fh_aes_driver *p_driver;

    p_driver = (struct fh_aes_driver *)obj_p->driver_private;
    p_driver->control_mode = TRIPLE_DES_METHOD;
    request_p->control_mode = TRIPLE_DES_METHOD;
    p_driver->control_mode |= CBC_MODE | DECRYPT;
    request_p->control_mode |= CBC_MODE | DECRYPT;
    fh_aes_handle_request(obj_p,request_p);
    return 0;
}


static int fh_des_tri_ofb_encrypt(struct rt_crypto_obj *obj_p,struct rt_crypto_request *request_p)
{
    struct fh_aes_driver *p_driver;

    p_driver = (struct fh_aes_driver *)obj_p->driver_private;
    p_driver->control_mode = TRIPLE_DES_METHOD;
    request_p->control_mode = TRIPLE_DES_METHOD;
    p_driver->control_mode |= OFB_MODE | ENCRYPT;
    request_p->control_mode |= OFB_MODE | ENCRYPT;
    fh_aes_handle_request(obj_p,request_p);
    return 0;

}


static int fh_des_tri_ofb_decrypt(struct rt_crypto_obj *obj_p,struct rt_crypto_request *request_p)
{
    struct fh_aes_driver *p_driver;

    p_driver = (struct fh_aes_driver *)obj_p->driver_private;
    p_driver->control_mode = TRIPLE_DES_METHOD;
    request_p->control_mode = TRIPLE_DES_METHOD;
    p_driver->control_mode |= OFB_MODE | DECRYPT;
    request_p->control_mode |= OFB_MODE | DECRYPT;
    fh_aes_handle_request(obj_p,request_p);
    return 0;
}


static int fh_des_tri_cfb_encrypt(struct rt_crypto_obj *obj_p,struct rt_crypto_request *request_p)
{
    struct fh_aes_driver *p_driver;

    p_driver = (struct fh_aes_driver *)obj_p->driver_private;
    p_driver->control_mode = TRIPLE_DES_METHOD;
    request_p->control_mode = TRIPLE_DES_METHOD;
    p_driver->control_mode |= CFB_MODE | ENCRYPT;
    request_p->control_mode |= CFB_MODE | ENCRYPT;
    fh_aes_handle_request(obj_p,request_p);
    return 0;

}


static int fh_des_tri_cfb_decrypt(struct rt_crypto_obj *obj_p,struct rt_crypto_request *request_p)
{
    struct fh_aes_driver *p_driver;

    p_driver = (struct fh_aes_driver *)obj_p->driver_private;
    p_driver->control_mode = TRIPLE_DES_METHOD;
    request_p->control_mode = TRIPLE_DES_METHOD;
    p_driver->control_mode |= CFB_MODE | DECRYPT;
    request_p->control_mode |= CFB_MODE | DECRYPT;
    fh_aes_handle_request(obj_p,request_p);
    return 0;
}



static void fh_aes_interrupt(int irq, void *param)
{

    rt_uint32_t isr_status;
    /* unsigned long flags; */
    struct fh_aes_driver *p_driver = (struct fh_aes_driver *) param;

/* struct fh_aes_dev *dev = platform_get_drvdata(pdev); */
    /* rt_uint32_t isr = p_driver->irq_en; */
    aes_writel(p_driver, dma_control, 0);
    isr_status = aes_readl(p_driver, intr_src);
    /* close the isr.. */
    aes_writel(p_driver, intr_enable, 0);
    /* clear isr bit.. */
    aes_writel(p_driver, intr_clear_status, 0x07);

    if (isr_status & 0x02)
    {
        /* add dma rev hreap error handle... */
        AES_ERR("dma rev hreap error...\n");
    }
    if (isr_status & 0x04)
    {
        /* add dma stop src handle... */
        AES_ERR("dma stop src ..\n");
    }
    if (isr_status & 0x01)
    {
        /* release sem */
        rt_completion_done(&p_driver->transfer_completion);
    }

}

int fh_aes_probe(void *priv_data)
{

    struct fh_aes_driver *fh_aes_driver_obj;
    struct fh_aes_platform_data *plat_data;
    char aes_lock_name[20] = { 0 };
    char aes_isr_name[20] = { 0 };
    struct rt_crypto_obj *p_crypto;
    rt_uint32_t i;
    struct clk *clk_efuse;
    plat_data = (struct fh_aes_platform_data *) priv_data;
    /* malloc data */
    fh_aes_driver_obj = (struct fh_aes_driver *) rt_malloc(sizeof(struct fh_aes_driver));
    if (!fh_aes_driver_obj)
    {
        AES_ERR("ERROR:no mem to malloc the aes..\n");
        return -RT_ENOMEM;
    }
    rt_memset(fh_aes_driver_obj, 0, sizeof(struct fh_aes_driver));
    if (plat_data->aes_support_flag & CRYPTO_EX_MEM_SET_KEY)
    {
       fh_aes_driver_obj->p_efuse =  rt_malloc(sizeof(struct wrap_efuse_obj));
       if (!fh_aes_driver_obj->p_efuse)
       {
           AES_ERR("ERROR:no mem to malloc the efuse..\n");
           rt_free(fh_aes_driver_obj);
           return -RT_ENOMEM;
       }
        rt_memset(fh_aes_driver_obj->p_efuse, 0, sizeof(struct wrap_efuse_obj));
       clk_efuse = (struct clk *)clk_get(NULL, "efuse_clk");
       if (clk_efuse)
       {
           clk_set_rate(clk_efuse, 25000000);
           clk_enable(clk_efuse);
       }
       fh_aes_driver_obj->p_efuse->regs = (void *)plat_data->efuse_base;
       s_efuse_obj.regs = (void *) plat_data->efuse_base;
       if (plat_data->aes_support_flag & CRYPTO_EX_MEM_INDEP_POWER)
       {
           /*efuse power up*/
           open_efuse_power(fh_aes_driver_obj->p_efuse);
       }
#ifdef EFUSE_BUFFER_OPEN
       efuse_buffer_set(fh_aes_driver_obj->p_efuse, 1);
#endif
    }

    fh_aes_driver_obj->id = plat_data->id;
    fh_aes_driver_obj->irq_no = plat_data->irq;
    fh_aes_driver_obj->regs = (void *) plat_data->base;
    fh_aes_driver_obj->open_flag = plat_data->aes_support_flag;

    rt_sprintf(aes_lock_name, "%s%d", "aes_lock", fh_aes_driver_obj->id);
    rt_sem_init(&fh_aes_driver_obj->lock, aes_lock_name, 1, RT_IPC_FLAG_FIFO);
    /* rt_completion_init(&fh_aes_driver_obj->transfer_completion); */
    AES_DBG("id:%d, irq:%d, regs:%x..\n", fh_aes_driver_obj->id, fh_aes_driver_obj->irq_no,
            (rt_uint32_t)fh_aes_driver_obj->regs);
    fh_aes_driver_obj->irq_en = 1 << 0;

    /* isr... */
    rt_sprintf(aes_isr_name, "%s%d", "aes_isr", fh_aes_driver_obj->id);
    rt_hw_interrupt_install(fh_aes_driver_obj->irq_no, fh_aes_interrupt,
            (void *) fh_aes_driver_obj, aes_isr_name);

    rt_hw_interrupt_umask(fh_aes_driver_obj->irq_no);
    AES_DBG("isr name is :%s\n", aes_isr_name);
    AES_DBG("aes register crypto...\n");
    /* register the algo .... */
    for (i = 0, p_crypto = &crypto_table[0]; i < ARRAY_SIZE(crypto_table); i++, p_crypto++)
    {
        rt_algo_register(RT_Algo_Class_Crypto, p_crypto, p_crypto->name, (void *) fh_aes_driver_obj);
    }

    return RT_EOK;
}

int fh_aes_exit(void *priv_data)
{
    return 0;
}

struct fh_board_ops aes_driver_ops = {

                .probe = fh_aes_probe,
                .exit = fh_aes_exit,
        };

void rt_hw_aes_init(void)
{
    /* int ret; */
    fh_board_driver_register("fh_aes", &aes_driver_ops);
}

/* efuse driver.... */
int efuse_detect_complete(struct wrap_efuse_obj *obj, int pos)
{
    unsigned int rdata;
    unsigned int time = 0;
    /* rt_kprintf("efuse wait pos %x...\n",pos); */
    do
    {
        time++;
        rdata = wrap_readl(obj, efuse_status0);
        if (time > EFUSE_TIMEOUT)
        {
            rt_kprintf("[efuse]:detect time out...pos: 0x%x\n", pos);
            return -1;
        }

    } while ((rdata & (1 << pos)) != 1 << pos);
    return 0;
    /* rt_kprintf("efuse wait pos done...\n",pos); */
}

void efuse_dump_register(struct wrap_efuse_obj *obj)
{
    rt_kprintf("cmd      :\t%08x ...SW:WR, HW:RO\n", wrap_readl(obj, efuse_cmd));
    rt_kprintf("config   :\t%08x ...SW:WR, HW:RO\n", wrap_readl(obj, efuse_config));
    rt_kprintf("match_key:\t%08x ...SW:WR, HW:RO\n", wrap_readl(obj, efuse_match_key));
    rt_kprintf("timing0  :\t%08x ...SW:WR, HW:RO\n", wrap_readl(obj, efuse_timing0));
    rt_kprintf("timing1  :\t%08x ...SW:WR, HW:RO\n", wrap_readl(obj, efuse_timing1));
    rt_kprintf("timing2  :\t%08x ...SW:WR, HW:RO\n", wrap_readl(obj, efuse_timing2));
    rt_kprintf("timing3  :\t%08x ...SW:WR, HW:RO\n", wrap_readl(obj, efuse_timing3));
    rt_kprintf("timing4  :\t%08x ...SW:WR, HW:RO\n", wrap_readl(obj, efuse_timing4));
    rt_kprintf("timing5  :\t%08x ...SW:WR, HW:RO\n", wrap_readl(obj, efuse_timing5));
    rt_kprintf("timing6  :\t%08x ...SW:WR, HW:RO\n", wrap_readl(obj, efuse_timing6));
    rt_kprintf("dout     :\t%08x ...SW:RO, HW:WR\n", wrap_readl(obj, efuse_dout));
    rt_kprintf("status0  :\t%08x ...SW:RO, HW:WO\n", wrap_readl(obj, efuse_status0));
    rt_kprintf("status1  :\t%08x ...SW:RO, HW:WO\n", wrap_readl(obj, efuse_status1));
    rt_kprintf("status2  :\t%08x ...SW:RO, HW:WO\n", wrap_readl(obj, efuse_status2));
    rt_kprintf("status3  :\t%08x ...SW:RO, HW:WO\n", wrap_readl(obj, efuse_status3));
    rt_kprintf("status4  :\t%08x ...SW:RO, HW:WO\n", wrap_readl(obj, efuse_status4));

}

void open_efuse_power(struct wrap_efuse_obj *obj)
{
    unsigned int data;
    data = wrap_readl(obj, efuse_config);
    data |= 1 << 27;
    data = wrap_writel(obj, efuse_config, data);
}

void efuse_buffer_set(struct wrap_efuse_obj *obj, unsigned int value)
{
    unsigned int data;
    data = wrap_readl(obj, efuse_config);
    data &= ~(1 << 4);
    data |= value << 4;
    data = wrap_writel(obj, efuse_config, data);

}
void auto_check_efuse_pro_bits(struct wrap_efuse_obj *obj, rt_uint32_t *buff)
{
    buff[0] = wrap_readl(obj, efuse_status1);
    buff[1] = wrap_readl(obj, efuse_status2);
}

void efuse_write_key_byte(struct wrap_efuse_obj *obj, rt_uint32_t entry, rt_uint8_t data)
{

    rt_uint32_t temp = 0;

    temp = (rt_uint32_t) data;
    /* 0~255 */
    temp &= ~0xffffff00;
    temp <<= 12;
    /* 0~63 */
    entry &= 0x3f;
    temp |= (entry << 4) | (0x02);

    wrap_writel(obj, efuse_cmd, temp);
    efuse_detect_complete(obj, 2);
}

void efuse_load_usrcmd(struct wrap_efuse_obj *obj)
{

    wrap_writel(obj, efuse_cmd, 1);
    efuse_detect_complete(obj, 1);

}

void auto_check_err_bits(struct wrap_efuse_obj *obj, rt_uint32_t *buff)
{

    /* first set auto check cmd */
    rt_uint32_t data;
    /* efuse_load_usrcmd(obj); */
    data = wrap_readl(obj, efuse_status3);
    *buff = data;

}


void efuse_check_map_para(unsigned int size, struct ex_key_map *p_key_map)
{
    int loop;
    ASSERT(size <= MAX_EX_KEY_MAP_SIZE);
    for (loop = 0; loop < size; loop++)
    {
        if ((p_key_map[loop].ex_mem_entry % 4 != 0)
                || (p_key_map[loop].crypto_key_no != loop))
        {
            AES_ERR("map[%d]:entry[0x%x]:aes key[0x%x] para error..\n", loop,
                    p_key_map[loop].ex_mem_entry,
                    p_key_map[loop].crypto_key_no);
            ASSERT(0);
        }
    }
}


static int get_soc_crypto_load_mode_from_efuse(struct wrap_efuse_obj *p_efuse)
{
    unsigned char ret_buf;

    refresh_efuse(p_efuse);
    efuse_read_entry(p_efuse, MATCH_KEY,
    SECURE_BOOT_ENTRY, &ret_buf, 1);
    if (ret_buf & SECURE_BOOT_ACTIVE_VAL)
        return SOC_BOOT_MODE_WITH_SECURE;
    return -1;
}

/*
 * 0      :means do not use secure boot or decrypt. all efuse could be transfered to aes
 * others :means use secure boot mode. efuse entry 0~15 DO NOT USE
 */
int fh_efuse_secure_check(struct fh_aes_driver *p_driver, struct wrap_efuse_obj *obj,
unsigned int start_no, unsigned int size, struct rt_crypto_request *request_p)
{
    int ret;
    int i;
    int key_map_size;
    struct ex_key_map *p_key_map;

    ret = get_soc_crypto_load_mode_from_efuse(obj);
    if (ret != SOC_BOOT_MODE_WITH_SECURE)
        return 0;

    if (p_driver->open_flag & CRYPTO_EX_MEM_SWITCH_KEY)
    {
        if (request_p->key_flag & CRYPTO_EX_MEM_SWITCH_KEY)
        {
            if (request_p->key_flag & CRYPTO_EX_MEM_4_ENTRY_1_KEY)
            {
                key_map_size = request_p->adv.ex_key_para.map_size;
                p_key_map = &request_p->adv.ex_key_para.map[0];
                /* check para. */
                for (i = 0; i < key_map_size; i++, p_key_map++)
                {
                    if (p_key_map->ex_mem_entry <
                    SECURE_PROTECT_EFUSE_ENTRY_MAX_INDEX)
                    {
                        rt_kprintf("Do not use secure area\n");
                        return -1;
                    }
                }
            }
        }
        else
        {
            /*efuse 0 ~ 15 must be used*/
            return -2;
        }
    }
    else
    {
        /* efuse 0 ~ 15 must be used*/
        return -3;
    }
    return 0;
}



void efuse_check_update_trans_flag(struct fh_aes_driver *p_driver,
struct rt_crypto_request *request_p)
{
    int i;
    struct ex_key_map *p_key_map;
    struct ex_key_map *old_p_key_map;
    int key_map_size;
    struct rt_crypto_request *p_old_usr_request;

    p_old_usr_request = &p_driver->old_usr_request;

    /* first check if use cpu mode */
    if (p_old_usr_request->key_flag != request_p->key_flag)
    {
        p_driver->efuse_trans_flag = EFUSE_NEED_TRANS;
        p_old_usr_request->key_flag = request_p->key_flag;
    }

    if (p_driver->open_flag & CRYPTO_EX_MEM_SWITCH_KEY)
    {
        /*chip need set the map...and usr set ~~~*/
        if (request_p->key_flag & CRYPTO_EX_MEM_SWITCH_KEY)
        {
            /* parse efuse map.. */
            if (request_p->key_flag & CRYPTO_EX_MEM_4_ENTRY_1_KEY)
            {
                AES_DBG("parse efuse map...map size: %d\n",request_p->adv.ex_key_para.map_size);
                key_map_size = request_p->adv.ex_key_para.map_size;
                p_key_map = &request_p->adv.ex_key_para.map[0];
                old_p_key_map = &p_old_usr_request->adv.ex_key_para.map[0];
                /* check map buf data... */
                efuse_check_map_para(key_map_size, p_key_map);

                /* check map size */
                if (key_map_size != p_old_usr_request->adv.ex_key_para.map_size)
                {
                /* cpy new to old */
                    rt_memcpy(p_old_usr_request, request_p, sizeof(struct rt_crypto_request));
                    p_driver->efuse_trans_flag = EFUSE_NEED_TRANS;
                    return;
                }
                /* check para. */
                for (i = 0; i < key_map_size; i++, p_key_map++, old_p_key_map++)
                {
                    if (rt_memcmp(p_key_map, old_p_key_map, sizeof(struct ex_key_map)))
                    {
                    /* cmp error */
                        rt_memcpy(p_old_usr_request, request_p, sizeof(struct rt_crypto_request));
                        p_driver->efuse_trans_flag = EFUSE_NEED_TRANS;
                        return;
                    }

                }

            }
        }
        else
        {
            /*chip need set the map...and usr not set ~~~*/
            AES_DBG("usr not set efuse map...key size:%d\n",request_p->key_size);
            key_map_size = request_p->key_size;
            if (key_map_size != p_old_usr_request->key_size)
            {
                p_old_usr_request->key_size = key_map_size;
                p_driver->efuse_trans_flag = EFUSE_NEED_TRANS;
            }
        }
    }
    else
    {
        AES_DBG("compatible LINBAO & DUOBAO\n");
        key_map_size = request_p->key_size;
        if (key_map_size != p_old_usr_request->key_size)
        {
            p_old_usr_request->key_size = key_map_size;
            p_driver->efuse_trans_flag = EFUSE_NEED_TRANS;
        }
    }
}

void efuse_trans_key(struct fh_aes_driver *p_driver,struct rt_crypto_request *request_p)
{
    int i;
    struct wrap_efuse_obj *obj;
    struct ex_key_map *p_key_map;
    int key_map_size;
    unsigned int temp_reg;
    obj = p_driver->p_efuse;

    /* add check func */
    efuse_check_update_trans_flag(p_driver, request_p);

    if (p_driver->efuse_trans_flag != EFUSE_NEED_TRANS)
    {
        /* rt_kprintf("=====DO NOT need to update efuse para to aes...=======\n"); */
        return;
    }
    else
    {
        /* clear update flag. */
        p_driver->efuse_trans_flag = 0;
        /* rt_kprintf("<<<<<need to update efuse para to aes...>>>>>>\n"); */
    }


    if (p_driver->open_flag & CRYPTO_EX_MEM_SWITCH_KEY)
    {
        /*chip need set the map...and usr set ~~~*/
        if (request_p->key_flag & CRYPTO_EX_MEM_SWITCH_KEY)
        {
            //parse efuse map..
            if (request_p->key_flag & CRYPTO_EX_MEM_4_ENTRY_1_KEY)
            {
                AES_DBG("parse efuse map...map size: %d\n",request_p->adv.ex_key_para.map_size);
                key_map_size = request_p->adv.ex_key_para.map_size;
                p_key_map = &request_p->adv.ex_key_para.map[0];
                //check map buf data...
                efuse_check_map_para(key_map_size,p_key_map);
                for (i = 0; i < key_map_size; i++,p_key_map++)
                {
                    refresh_efuse(obj);
                    temp_reg = wrap_readl(obj,efuse_config);
                    temp_reg &= ~(0xf <<28);
                    temp_reg |= (p_key_map->ex_mem_entry / 4) << 28;
                    wrap_writel(obj, efuse_config, temp_reg);
                    wrap_writel(obj, efuse_cmd, (i << 20) + 0x04);
                    efuse_detect_complete(obj, 4);
                }
            }
        }
        else{
            /*chip need set the map...and usr not set ~~~*/
            AES_DBG("usr not set efuse map...key size:%d\n",request_p->key_size);
            key_map_size = request_p->key_size;
            for (i = 0; i < key_map_size / 4; i++)
            {
                refresh_efuse(obj);
                temp_reg = wrap_readl(obj,efuse_config);
                temp_reg &= ~(0xf <<28);
                temp_reg |= i << 28;
                wrap_writel(obj, efuse_config, temp_reg);
                wrap_writel(obj, efuse_cmd, (i << 20) + 0x04);
                efuse_detect_complete(obj, 4);
            }
        }
    }
    else{
        AES_DBG("compatible LINBAO & DUOBAO\n");
        key_map_size = request_p->key_size;
        for (i = 0; i < key_map_size / 4; i++)
        {
            refresh_efuse(obj);
            wrap_writel(obj, efuse_cmd, (i << 20) + 0x04);
            efuse_detect_complete(obj, 4);
        }
    }
}

void efuse_get_lock_status(struct wrap_efuse_obj *obj, struct efuse_status *status)
{
    status->efuse_apb_lock = (wrap_readl(obj, efuse_status0) >> 20) & 0x0f;
    status->aes_ahb_lock = (wrap_readl(obj, efuse_status0) >> 24) & 0x0f;
}

void efuse_read_entry(struct wrap_efuse_obj *obj, rt_uint32_t key, rt_uint32_t start_entry, rt_uint8_t *buff,
        rt_uint32_t size)
{
    rt_uint32_t data, i;
    for (i = 0; i < size; i++)
    {
        wrap_writel(obj, efuse_match_key, key);
        wrap_writel(obj, efuse_cmd, ((start_entry + i) << 4) + 0x03);
        efuse_detect_complete(obj, 3);
        data = wrap_readl(obj, efuse_dout);
        *buff++ = (rt_uint8_t) data;
    }
}

unsigned char efuse_read_debug_entry(rt_uint32_t entry)
{
	unsigned char ret;

	if (entry < EFUSE_DEBUG_ENTRY_NO_MIN ||
	entry > EFUSE_DEBUG_ENTRY_NO_MAX) {
		rt_kprintf("Error entry %d\n", entry);
		return 0;
	}
	efuse_read_entry(&s_efuse_obj, 0, entry, &ret, 1);
	return ret;
}

void efuse_write_cmd(struct wrap_efuse_obj *obj, rt_uint32_t data)
{
    wrap_writel(obj, efuse_cmd, data);
}

void refresh_efuse(struct wrap_efuse_obj *obj)
{
    wrap_writel(obj, efuse_cmd, CMD_WFLGA_AUTO);
    efuse_detect_complete(obj, 8);
    efuse_load_usrcmd(obj);
}

long fh_efuse_ioctl(EFUSE_INFO *efuse_user_info, unsigned int cmd,
        unsigned long arg)
{

    int i;
    EFUSE_INFO efuse_info = { 0 };

    rt_uint32_t temp_swap_data[32] = { 0 };
    rt_uint32_t *p_dst;
    rt_uint8_t *p_dst_8;
    unsigned int data;

    rt_memcpy(&efuse_info, efuse_user_info, sizeof(EFUSE_INFO));
    if (s_efuse_obj.regs == NULL)
    {
        rt_kprintf("please init efuse first..\n");
        return -1;
    }
    refresh_efuse(&s_efuse_obj);

    switch (cmd)
    {
    case IOCTL_EFUSE_CHECK_PRO:
        auto_check_efuse_pro_bits(&s_efuse_obj, efuse_info.status.protect_bits);
        break;

    case IOCTL_EFUSE_WRITE_KEY:
        rt_memcpy((rt_uint8_t *) &temp_swap_data[0], efuse_info.key_buff, efuse_info.key_size);
        p_dst = &temp_swap_data[0];
        for (i = 0; i < efuse_info.key_size / sizeof(rt_uint32_t); i++)
        {
            aes_biglittle_swap((rt_uint8_t *) (p_dst + i));
        }
        p_dst_8 = (rt_uint8_t *) &temp_swap_data[0];

        for (i = 0; i < efuse_info.key_size; i++)
        {
            efuse_write_key_byte(&s_efuse_obj, efuse_info.efuse_entry_no + i, *(p_dst_8 + i));
        }

        break;

    case IOCTL_EFUSE_CHECK_LOCK:
        efuse_get_lock_status(&s_efuse_obj, &efuse_info.status);
        break;
    case IOCTL_EFUSE_TRANS_KEY:
        rt_kprintf("please use efuse transkey with aes...\n");
        break;

    case IOCTL_EFUSE_CHECK_ERROR:
        auto_check_err_bits(&s_efuse_obj, &efuse_info.status.error);
        break;
    case IOCTL_EFUSE_READ_ENTRY:
        /*rt_kprintf("not support read efuse entry...\n");*/
    	efuse_read_entry(&s_efuse_obj, efuse_info.status.error,
		efuse_info.efuse_entry_no,
		efuse_info.key_buff, efuse_info.key_size);
		p_dst = (rt_uint32_t *)efuse_info.key_buff;
		for (i = 0; i < efuse_info.key_size / sizeof(rt_uint32_t); i++) {
			aes_biglittle_swap((rt_uint8_t *) (p_dst + i));
			/*printk("swap data is %x\n",*(p_dst + i));*/
		}
        break;

    case IOCTL_EFUSE_SET_LOCK_DATA:
        /* parse lock data... */
        data = efuse_info.status.aes_ahb_lock;
        data <<= 4;
        data &= 0xf0;
        efuse_info.status.efuse_apb_lock &= 0x0f;
        data |= efuse_info.status.efuse_apb_lock;
        efuse_write_key_byte(&s_efuse_obj, 63, (rt_uint8_t) data);
        break;
    case IOCTL_EFUSE_GET_LOCK_DATA:
        rt_kprintf("not support get lock data ...\n");
        break;
    case IOCTL_EFUSE_SET_MAP_PARA_4_TO_1:
        rt_kprintf("please use with aes...\n");
         break;
    case IOCTL_EFUSE_SET_MAP_PARA_1_TO_1:
        rt_kprintf("not support this func now..\n");
        break;
    case IOCTL_EFUSE_CLR_MAP_PARA:
        rt_kprintf("not support here...\n");
        break;
    case IOCTL_EFUSE_DUMP_REGISTER:
        rt_kprintf("not support here...\n");
        break;

    case IOCTL_EFUSE_DEBUG:
        rt_kprintf("not support here...\n");
        break;

    case IOCTL_EFUSE_WRITE_ENTRY:
        rt_memcpy((rt_uint8_t *) &temp_swap_data[0], efuse_info.key_buff, efuse_info.key_size);
        p_dst_8 = (rt_uint8_t *)&temp_swap_data[0];
        for (i = 0; i < efuse_info.key_size; i++)
        {
            efuse_write_key_byte(&s_efuse_obj,
            efuse_info.efuse_entry_no + i, *(p_dst_8 + i));
        }
        break;

    case IOCTL_EFUSE_READ_ENTRY_RAW:
        efuse_read_entry(&s_efuse_obj, efuse_info.status.error,
        efuse_info.efuse_entry_no,
        (rt_uint8_t *)&temp_swap_data[0], efuse_info.key_size);
        rt_memcpy((rt_uint8_t *)efuse_info.key_buff, (void *) &temp_swap_data[0], efuse_info.key_size);
        break;

    default:
        break;
    }

    rt_memcpy(&efuse_user_info->status, &efuse_info.status, sizeof(struct efuse_status));
    return 0;
}



