/********************************************************************************************/
/* Fullhan Technology (Shanghai) Co., Ltd.                                                  */
/* Fullhan Proprietary and Confidential                                                     */
/* Copyright (c) 2014 Fullhan Technology Group Limited and its affiliated companies         */
/********************************************************************************************/
#ifndef __FH_EFUSE_H__
#define __FH_EFUSE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <rthw.h>
#include <board_info.h>
#include <rtdevice.h>
#include <stdbool.h>
#include <string.h>
#include <fh_clock.h>




struct fh_efuse_platform_data
{
    rt_uint32_t id;
    rt_uint32_t irq;
    rt_uint32_t base;
};

typedef struct _FH_EFUSE_DEV_S
{
    struct rt_device parent;
    rt_uint32_t efuse_reg;
    struct clk *clk;
    struct clk *pclk;
} FH_EFUSE_DEV_S, *FH_EFUSE_DEV_P;

struct efuse_status
{
    /* bit 1 means could write..0 not write */
    rt_uint32_t protect_bits[2];
    /* bit 1 means cpu couldn't read efuse entry data... */
    rt_uint32_t efuse_apb_lock;
    rt_uint32_t aes_ahb_lock;
    rt_uint32_t error;
#ifdef FH_EFUSE_V2
    rt_uint32_t efuse_write_lock[2];
#endif
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
#ifdef FH_EFUSE_V2
    rt_uint32_t bit_pos;
    rt_uint32_t bit_val;
#endif
} EFUSE_INFO;

#define IOCTL_EFUSE_CHECK_PRO                  100
#define IOCTL_EFUSE_WRITE_KEY                  101
#define IOCTL_EFUSE_CHECK_LOCK                 102
#define IOCTL_EFUSE_TRANS_KEY                  103
#define IOCTL_EFUSE_CHECK_ERROR                104
#define IOCTL_EFUSE_READ_KEY                   105
#define IOCTL_EFUSE_SET_LOCK_DATA              106
#define IOCTL_EFUSE_GET_LOCK_DATA              107
#define IOCTL_EFUSE_SET_MAP_PARA_4_TO_1        109
#define IOCTL_EFUSE_SET_MAP_PARA_1_TO_1        110
#define IOCTL_EFUSE_CLR_MAP_PARA               111
#define IOCTL_EFUSE_WRITE_ENTRY                120
#define IOCTL_EFUSE_READ_ENTRY                 121
#define IOCTL_EFUSE_DEBUG                      122
#define IOCTL_EFUSE_DUMP_REGISTER              123
#define IOCTL_EFUSE_WRITE_BIT                  124
#define IOCTL_EFUSE_SET_LOCK_WRITE_DATA        125

/*** Result Code ***/

#define EFUSE_SUCCESS    0

#define EFUSE_IOCTL_CMD_NOT_SUPPORT            -200
#define EFUSE_IOCTL_CMD_DUMMY                  -201
#define EFUSE_WAIT_CMD_READ_ERROR              -202
#define EFUSE_WAIT_CMD_WRITE_ERROR             -203
#define EFUSE_WAIT_CMD_LOAD_USERCMD_ERROR      -204
#define EFUSE_WAIT_CMD_WFLAG_AUTO_ERROR        -205


#ifdef __cplusplus
}
#endif

#endif /* __FH_EFUSE_H__ */

