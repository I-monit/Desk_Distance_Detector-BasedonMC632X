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

#ifndef __ALGORITHM_CORE_H__
#define __ALGORITHM_CORE_H__

#include <rtthread.h>
/****************************************************************************
 * #include section
 *   add #include here if any
 ***************************************************************************/

/* #define ALGO_DEBUG   */

#define RT_USING_CRYPTO

#define DES_KEY_SIZE            8
#define DES3_EDE_KEY_SIZE       (3 * DES_KEY_SIZE)

#define AES_KEYSIZE_128         16
#define AES_KEYSIZE_192         24
#define AES_KEYSIZE_256         32

#define AES_MIN_KEY_SIZE        AES_KEYSIZE_128
#define AES_MAX_KEY_SIZE        AES_KEYSIZE_256
/****************************************************************************
 * #define section
 *   add constant #define here if any
 ***************************************************************************/

enum rt_algo_class_type
{
    RT_Algo_Class_Crypto = 0, /**< The algo is a crypto. */
    /* RT_Algo_Class_Compress,                         The algo is a compress.  */
    RT_Algo_Class_Unknown, /**< The algo is unknown. */

};

/****************************************************************************
 * ADT section
 *   add Abstract Data Type definition here
 ***************************************************************************/
/* basic class */
struct rt_algo_information
{

    struct rt_device parent;
    char *class_name;
    enum rt_algo_class_type type;
    rt_list_t list_head;

};

/* crypto class.. */

/*add efuse_aes_map struct*/
#define MAX_EX_KEY_MAP_SIZE              8
struct ex_key_map {
    rt_uint32_t crypto_key_no;
    rt_uint32_t ex_mem_entry;
} ;

struct ex_key_map_para {
    rt_uint32_t map_size;
    struct ex_key_map map[MAX_EX_KEY_MAP_SIZE];
} ;

//add efuse request info....
struct crypto_adv_info{
    //rt_uint32_t adv_flag;
    struct ex_key_map_para ex_key_para;
};

struct rt_crypto_request
{
    rt_uint8_t *iv;                     /* core will check this if 'flag = USING_IV' */
    rt_uint32_t iv_size;
    //can't change......
#define CRYPTO_CPU_SET_KEY                      (1<<0)
#define CRYPTO_EX_MEM_SET_KEY                   (1<<1)
#define CRYPTO_EX_MEM_INDEP_POWER               (1<<2)
//bit 8~ex mem bit field..
#define CRYPTO_EX_MEM_SWITCH_KEY                (1<<8)
//if set ex mem set switch key..then parse below..
#define CRYPTO_EX_MEM_4_ENTRY_1_KEY             (1<<9)
    rt_uint32_t key_flag;
    rt_uint8_t *key;
    rt_uint32_t key_size;               /* core will check if this value larger than 'key_size' */

    rt_uint8_t *data_src;               /* core will check the allign */
    rt_uint8_t *data_dst;               /* core will check the allign */
    rt_uint32_t data_size;              /* core will check the if the data size is multi of the 'block_size' */
    rt_uint32_t control_mode;
    rt_bool_t iv_flag;
    //if key_flag set efuse...then parse the para below...
    struct crypto_adv_info adv;
};

struct rt_crypto_obj
{
    rt_list_t list;                 /* bind to the crypto list... */

    char name[RT_NAME_MAX];         /* each crypto copy name when register.. */
/* #define USING_IV          0x55 */
    /* RT_TRUE / RT_FALSE */
    rt_bool_t flag;
    /* uint : Byte */
    rt_uint32_t block_size;         /* each crypto should have its own process block size... ecb(des) = 8 Bytes */
    /* uint : Byte */
    rt_uint32_t src_allign_size;    /* each crypto may have the src allign... */
    /* uint : Byte */
    rt_uint32_t dst_allign_size;    /* each crypto may have the dst allign... */
    /* uint : Byte */
    rt_uint32_t max_key_size;       /* each crypto may have the key max size... */
    rt_uint32_t min_key_size;       /* each crypto may have the key max size... */
    /* uint : Byte */
    rt_uint32_t otp_size;           /* driver one time process how many bytes data.. */
    void *driver_private;                  /* driver could use the point to save private data...    fh aes 2048 Bytes */
    struct {
        int (*set_key)(struct rt_crypto_obj *obj_p, struct rt_crypto_request *request_p);
        int (*encrypt)(struct rt_crypto_obj *obj_p, struct rt_crypto_request *request_p);
        int (*decrypt)(struct rt_crypto_obj *obj_p, struct rt_crypto_request *request_p);
    } ops;
    void *user_private;
    int (*complete)(void *user_private);

};

/****************************************************************************
 *  extern variable declaration section
 ***************************************************************************/

/****************************************************************************
 *  section
 *   add function prototype here if any
 ***************************************************************************/
rt_err_t rt_algo_init(void);
rt_err_t rt_algo_register(rt_uint8_t type, void *obj, const char *obj_name, void *priv);
void *rt_algo_obj_find(rt_uint8_t type, char *name);
rt_err_t rt_algo_crypto_decrypt(struct rt_crypto_obj *obj_p, struct rt_crypto_request *request_p);
rt_err_t rt_algo_crypto_encrypt(struct rt_crypto_obj *obj_p, struct rt_crypto_request *request_p);
rt_err_t rt_algo_crypto_setkey(struct rt_crypto_obj *obj_p, struct rt_crypto_request *request_p);
/********************************End Of File********************************/
#endif

