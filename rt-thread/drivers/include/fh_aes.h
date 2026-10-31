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

#ifndef __FH_AES_H__
#define __FH_AES_H__

#include "algorithm_core.h"
#include "rtdevice.h"
#ifndef NULL
#define NULL 0
#endif

#define MAX_EFUSE_MAP_SIZE              8

#define IOCTL_EFUSE_CHECK_PRO           0
#define IOCTL_EFUSE_WRITE_KEY           1
#define IOCTL_EFUSE_CHECK_LOCK          2
#define IOCTL_EFUSE_TRANS_KEY           3
#define IOCTL_EFUSE_CHECK_ERROR         4
#define IOCTL_EFUSE_READ_ENTRY          5
#define IOCTL_EFUSE_SET_LOCK_DATA       6
#define IOCTL_EFUSE_GET_LOCK_DATA       7
#define IOCTL_EFUSE_SET_MAP_PARA_4_TO_1     9
#define IOCTL_EFUSE_SET_MAP_PARA_1_TO_1     10
#define IOCTL_EFUSE_CLR_MAP_PARA            11
#define IOCTL_EFUSE_WRITE_ENTRY    20
#define IOCTL_EFUSE_READ_ENTRY_RAW    21

#define OPEN_4_ENTRY_TO_1_KEY           0x55
#define OPEN_1_ENTRY_TO_1_KEY           0xaa

#define IOCTL_EFUSE_DEBUG           55
#define IOCTL_EFUSE_DUMP_REGISTER       120

struct fh_aes_reg
{
    rt_uint32_t encrypt_control;        /* 0 */
    rt_uint32_t reserved_4_8;           /* 4 */
    rt_uint32_t fifo_status;            /* 8 */
    rt_uint32_t parity_error;           /* c */
    rt_uint32_t security_key0;          /* 10 */
    rt_uint32_t security_key1;          /* 14 */
    rt_uint32_t security_key2;          /* 18 */
    rt_uint32_t security_key3;          /* 1c */
    rt_uint32_t security_key4;          /* 20 */
    rt_uint32_t security_key5;          /* 24 */
    rt_uint32_t security_key6;          /* 28 */
    rt_uint32_t security_key7;          /* 2c */
    rt_uint32_t initial_vector0;        /* 30 */
    rt_uint32_t initial_vector1;        /* 34 */
    rt_uint32_t initial_vector2;        /* 38 */
    rt_uint32_t initial_vector3;        /* 3c */
    rt_uint32_t reserved_40_44;         /* 40 */
    rt_uint32_t reserved_44_48;         /* 44 */
    rt_uint32_t dma_src_add;            /* 48 */
    rt_uint32_t dma_dst_add;            /* 4c */
    rt_uint32_t dma_trans_size;         /* 50 */
    rt_uint32_t dma_control;            /* 54 */
    rt_uint32_t fifo_threshold;         /* 58 */
    rt_uint32_t intr_enable;            /* 5c */
    rt_uint32_t intr_src;               /* 60 */
    rt_uint32_t mask_intr_status;       /* 64 */
    rt_uint32_t intr_clear_status;      /* 68 */
    rt_uint32_t reserved_6c_70;         /* 6c */
    rt_uint32_t revision;               /* 70 */
    rt_uint32_t feature;                /* 74 */
    rt_uint32_t reserved_78_7c;         /* 78 */
    rt_uint32_t reserved_7c_80;         /* 7c */
    rt_uint32_t last_initial_vector0;   /* 80 */
    rt_uint32_t last_initial_vector1;   /* 84 */
    rt_uint32_t last_initial_vector2;   /* 88 */
    rt_uint32_t last_initial_vector3;   /* 8c */
};

struct fh_aes_platform_data
{
    rt_uint32_t id;
    rt_uint32_t irq;
    rt_uint32_t base;

    rt_uint32_t efuse_base;
    rt_uint32_t aes_support_flag;
};
struct wrap_efuse_obj;
struct fh_aes_driver
{

    void *regs;
    rt_uint32_t id;
    rt_uint32_t irq_no;                 /* board info... */
    rt_uint32_t open_flag;
    /* below are driver use */
    rt_uint32_t irq_en;
    /* sys lock */
    struct rt_semaphore lock;
    /* isr */
    struct rt_completion transfer_completion;
    rt_uint32_t control_mode;
    rt_bool_t iv_flag;

    /* driver could find the core data... */
    struct rt_crypto_obj *cur_crypto;
    struct rt_crypto_request *cur_request;

    //bind one status to efuse ..
    struct wrap_efuse_obj *p_efuse;

    /*add check if need efuse transkey*/
    #define EFUSE_NEED_TRANS    0x55
    rt_uint32_t efuse_trans_flag;
    /* for old driver just check size. for have mapping func, should check the adv_info para.*/
    rt_uint32_t old_size;
    struct rt_crypto_request old_usr_request;
};

/*add efuse_aes_map struct*/
struct efuse_aes_map {
    rt_uint32_t aes_key_no;
    rt_uint32_t efuse_entry;
};

struct efuse_aes_map_para {
    rt_uint32_t open_flag;
    rt_uint32_t map_size;
    struct efuse_aes_map map[MAX_EFUSE_MAP_SIZE];
};

struct efuse_status
{
    /* bit 1 means could write..0 not write */
    rt_uint32_t protect_bits[2];
    /* bit 1 means cpu couldn't read efuse entry data... */
    rt_uint32_t efuse_apb_lock;
    rt_uint32_t aes_ahb_lock;
    rt_uint32_t error;
};

typedef struct {
    /* write key */
    rt_uint32_t efuse_entry_no; /* from 0 ~ 31 */
    rt_uint8_t *key_buff;
    rt_uint32_t key_size;
    /* trans key */
    rt_uint32_t trans_key_start_no; /* from 0 ~ 7 */
    rt_uint32_t trans_key_size;     /* max 8 */
    /* status */
    struct efuse_status status;
/* rt_uint32_t status;           //process status back to app. */
} EFUSE_INFO;

struct wrap_efuse_obj {
    void *regs;
    /*write key*/
    rt_uint32_t efuse_entry_no;     /*from 0~31*/
    rt_uint8_t *key_buff;
    rt_uint32_t key_size;
    /*trans key*/
    rt_uint32_t trans_key_start_no; /*from 0~7*/
    rt_uint32_t trans_key_size;     /*max 8*/
    /*status*/
    struct efuse_status status;

} ;


#define AES_FLAG_SUPPORT_CPU_SET_KEY            CRYPTO_CPU_SET_KEY
#define AES_FLAG_SUPPORT_EFUSE_SET_KEY          CRYPTO_EX_MEM_SET_KEY

long fh_efuse_ioctl(EFUSE_INFO *efuse_user_info, unsigned int cmd,
        unsigned long arg);
void rt_hw_aes_init(void);
unsigned char efuse_read_debug_entry(rt_uint32_t entry);
#endif
/* FH_AES_H_ */
