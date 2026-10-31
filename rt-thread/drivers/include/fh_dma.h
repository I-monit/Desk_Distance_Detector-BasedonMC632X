/**
 * Copyright (c) 2015-2019 Shanghai Fullhan Microelectronics Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 *
 * Change Logs:
 * Date           Author       Notes
 * 2019-01-31     wangyl307    add license Apache-2.0
 */

#ifndef __FH_DMA_H__
#define __FH_DMA_H__

#undef DESC_MAX_SIZE
#undef FH_CHANNEL_MAX_TRANSFER_SIZE

#include <rtconfig.h>


#ifdef FH_USING_DMA
#include "fh_ahb_dma.h"
#define FH_CHANNEL_MAX_TRANSFER_SIZE (2048)
#define DESC_MAX_SIZE (4096)
#elif defined FH_USING_MOL_DMA
#include "mol_dma.h"
#define FH_CHANNEL_MAX_TRANSFER_SIZE (32 * 0x100000)
#define DESC_MAX_SIZE (256)
#elif defined FH_USING_AXI_DMA
#include "fh_axi_dma.h"
#define FH_CHANNEL_MAX_TRANSFER_SIZE (2048)
#define DESC_MAX_SIZE (4096)
#endif


#define PROTCTL_ENABLE    0x55
#define MASTER_SEL_ENABLE    0x55


/* user use the dma could use callback function,when the dma make the work */
/* done... */
typedef void (*dma_complete_callback)(void *complete_para);

typedef void (*user_prepare)(void *prepare_para);


/* controller private para... */
struct fh_dma;

#define CH_CTL_L_AR_CACHE(n) ((n) << 22)
#define CH_CTL_L_AW_CACHE(n) ((n) << 26)
#define CH_CTL_H_ARLEN_EN(n) ((n) << 6)
#define CH_CTL_H_ARLEN(n) ((n) << 7)
#define CH_CTL_H_AWLEN_EN(n) ((n) << 15)
#define CH_CTL_H_AWLEN(n) ((n) << 16)

#define CH_CFG_H_DST_OSR_LMT(n) ((n) << 27)
#define CH_CFG_H_SRC_OSR_LMT(n) ((n) << 23)
struct s_spdup_info
{
#define AXI_SPD_UP_ACTIVE      0x55aaaa55
    unsigned int spdup_flag;
    unsigned int spdup_ctl_low;
    unsigned int spdup_ctl_high;
    unsigned int spdup_cfg_low;
    unsigned int spdup_cfg_high;
};

struct dma_platform_data
{
    int id;
    char *name;
    int irq;
    rt_uint32_t base;
    int channel_max_number;
    void (*dma_init)(void);
    struct s_spdup_info spdup;
};


struct usrdef_trans_desc {
	rt_uint32_t src_width;
	rt_uint32_t src_msize;
	rt_uint32_t src_add;
	rt_uint32_t src_inc_mode;

	rt_uint32_t dst_width;
	rt_uint32_t dst_msize;
	rt_uint32_t dst_add;
	rt_uint32_t dst_inc_mode;

	rt_uint32_t size;

	rt_uint32_t data_switch;
	#define USR_DEFINE_ONE_TIME_LEN    0x55aaaa55
	rt_uint32_t ot_len_flag;
	rt_uint32_t ot_len_len;
};

/* transfer use below */
struct dma_transfer
{
    /* this is private for the dma drive....app don't touch it,the driver will */
    /* manger it */
    /* link interface for more transfer to the controller... */
    rt_list_t transfer_list;
    struct fh_dma *dma_controller;
    /* this the mem add....the dma controller will load the setting to move data */
    /* .... */
    /* user don't touch it */
#ifdef FH_USING_DMA
    struct dw_lli *first_lli;
#elif defined FH_USING_MOL_DMA
    struct mc_dma_lli *first_lli;
#else
    struct axi_dma_lli *first_lli;
#endif

    rt_uint32_t lli_size;
    /* new add for allign get desc... */
    rt_uint32_t actual_lli_size;

/* user could set paras below~~~ */
#define AUTO_FIND_CHANNEL (0xff)
    /* transfer with which dma channel...if the data is 0xff, the driver will */
    /* auto find a free channel. */
    rt_uint32_t channel_number;
    /* which dma you want to use...for fh81....only 0!!! */
    rt_uint32_t dma_number;

/* user should set the para below */
#define DMA_M2M (0)       /* MEM <=> MEM */
#define DMA_M2P (1)       /* MEM => peripheral A */
#define DMA_P2M (2)       /* MEM <= peripheral A */
#define DMA_P2P (3)       /* peripheral A <=> peripheral B */
    rt_uint32_t fc_mode;  /* ip->mem. mem->mem. mem->ip */

#define DMA_HW_HANDSHAKING (0)
#define DMA_SW_HANDSHAKING (1)
    rt_uint32_t src_hs;  /* src */
    /* if use hw handshaking ,you need to set the hw handshaking number, this */
    /* SOC defined */
    rt_uint32_t src_per;  /* src hw handshake number */
/* rt_uint32_t            irq_mode;//for each transfer,irq maybe not same. */
/* suggest for the default(transfer isr) */

#define DW_DMA_SLAVE_WIDTH_8BIT (0)
#define DW_DMA_SLAVE_WIDTH_16BIT (1)
#define DW_DMA_SLAVE_WIDTH_32BIT (2)
    rt_uint32_t src_width;

/* the user should reference the hw handshaking watermark.. */
#define DW_DMA_SLAVE_MSIZE_1 (0)
#define DW_DMA_SLAVE_MSIZE_4 (1)
#define DW_DMA_SLAVE_MSIZE_8 (2)
#define DW_DMA_SLAVE_MSIZE_16 (3)
#define DW_DMA_SLAVE_MSIZE_32 (4)
#define DW_DMA_SLAVE_MSIZE_64 (5)
#define DW_DMA_SLAVE_MSIZE_128 (6)
#define DW_DMA_SLAVE_MSIZE_256 (7)
    rt_uint32_t src_msize;
    rt_uint32_t src_add;
#define DW_DMA_SLAVE_INC (0)
#define DW_DMA_SLAVE_DEC (1)
#define DW_DMA_SLAVE_FIX (2)
    rt_uint32_t src_inc_mode;  /* increase mode: increase or not change */

    /* #define DMA_DST_HW_HANDSHAKING    (0) */
    /* #define DMA_DST_SW_HANDSHAKING    (1) */
    rt_uint32_t dst_hs;  /* src */
    /* if use hw handshaking ,you need to set the hw handshaking number, this */
    /* SOC defined */
    rt_uint32_t dst_per;  /* dst hw handshake number */
    /* #define    DW_DMA_SLAVE_WIDTH_8BIT     (0) */
    /* #define    DW_DMA_SLAVE_WIDTH_16BIT     (1) */
    /* #define    DW_DMA_SLAVE_WIDTH_32BIT     (2) */
    rt_uint32_t dst_width;
    /* #define DW_DMA_SLAVE_MSIZE_1         (0) */
    /* #define DW_DMA_SLAVE_MSIZE_4         (1) */
    /* #define DW_DMA_SLAVE_MSIZE_8         (2) */
    /* #define DW_DMA_SLAVE_MSIZE_16         (3) */
    /* #define DW_DMA_SLAVE_MSIZE_32         (4) */
    /* #define DW_DMA_SLAVE_MSIZE_64         (5) */
    /* #define DW_DMA_SLAVE_MSIZE_128         (6) */
    /* #define DW_DMA_SLAVE_MSIZE_256         (7) */
    rt_uint32_t dst_msize;
    rt_uint32_t dst_add;
    /* #define DW_DMA_SLAVE_INC        (0) */
    /* #define DW_DMA_SLAVE_DEC        (1) */
    /* #define DW_DMA_SLAVE_FIX        (2) */
    rt_uint32_t dst_inc_mode;  /* increase mode: increase or not change */
    #define ADDR_RELOAD      (0x55)
    rt_uint32_t src_reload_flag;
    rt_uint32_t dst_reload_flag;
    /* total sizes, unit: src_width/DW_DMA_SLAVE_WIDTH_8BIT... */
    /* exg: src_width = DW_DMA_SLAVE_WIDTH_32BIT. trans_len = 2...means that: */
    /* the dma will transfer 2*4 bytes.. */
    /* exg: src_width = DW_DMA_SLAVE_WIDTH_8BIT. trans_len = 6...means that: the */
    /* dma will transfer 1*6 bytes.. */
    rt_uint32_t trans_len;

    /* this is used when dma finish transfer job */
    dma_complete_callback complete_callback;
    void *complete_para;  /* for the driver data use the dma driver. */

    /* this is used when dma before work..the user maybe need to set his own */
    /* private para.. */
    user_prepare prepare_callback;
    void *prepare_para;

    /* add cyclic para... */
    /* period len.. */
    rt_uint32_t period_len;
    /*add master sel*/
    rt_uint32_t master_flag;
    rt_uint32_t src_master;
    rt_uint32_t dst_master;
    /*add protctl set, but driver should know if the soc set already*/
    rt_uint32_t protctl_flag;
    rt_uint32_t protctl_data;
    rt_uint32_t cyclic_periods;

#define SWT_MAGIC       0xdead0000

#define SWT_ABCD_ABCD	(0 | SWT_MAGIC)
#define SWT_ABCD_DCBA	(1 | SWT_MAGIC)
#define SWT_ABCD_BADC	(2 | SWT_MAGIC)
#define SWT_ABCD_DCAB	(3 | SWT_MAGIC)

    rt_uint32_t data_switch;
#define USR_DEFINE_ONE_TIME_LEN    0x55aaaa55
    rt_uint32_t ot_len_flag;
    rt_uint32_t ot_len_len;

    struct usrdef_trans_desc *p_desc;
    rt_uint32_t desc_size;
};

int fh_dma_init(void);
#endif /* FH81_DMA_H_ */
