/**
 * Copyright (c) 2015-2019 Shanghai Fullhan Microelectronics Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020-4-3       zhangy       add first version driver
 */

#include "fh_dma.h"
#include "mmu.h"
#include "dma.h"
#include <stdint.h>
#include <rtdevice.h>
#include <rthw.h>
#include "fh_arch.h"
#include "mmu.h"
#include "fh_def.h"
#include "board_info.h"


/* #define DMA_DEBUG */
#if defined(DMA_DEBUG) && defined(RT_DEBUG)
#define FH_DMA_DEBUG(fmt, args...) rt_kprintf(fmt, ##args);
#else
#define FH_DMA_DEBUG(fmt, args...)
#endif
#define FH_DMA_ASSERT(expr) if (!(expr)) { \
        rt_kprintf("Assertion failed! %s:line %d\n", \
        __func__, __LINE__); \
        while (1)   \
           ;       \
        }

#define AXI_DESC_ALLIGN                64  /*DO NOT TOUCH!!!*/
#define AXI_DESC_ALLIGN_BIT_MASK     0x3f  /*DO NOT TOUCH!!!*/
#define DMA_CONTROLLER_NUMBER (1)

#define lift_shift_bit_num(bit_num) (1 << bit_num)

#define DMA_CONTROLLER_NUMBER (1)
#define FH_CHANNEL_MAX_TRANSFER_SIZE (2048)

#ifndef BIT
#define BIT(x) (1 << (x))
#endif
#ifndef GENMASK
#define GENMASK(h, l) (((~0UL) << (l)) & (~0UL >> (sizeof(long) * 8 - 1 - (h))))
#endif
#ifndef GENMASK_ULL
#define GENMASK_ULL(h, l)                                                      \
	(((~0ULL) << (l)) & (~0ULL >> (sizeof(long long) * 8 - 1 - (h))))
#endif


/* DMAC_CFG */
#define DMAC_EN_POS			0
#define DMAC_EN_MASK		BIT(DMAC_EN_POS)
#define INT_EN_POS			1
#define INT_EN_MASK			BIT(INT_EN_POS)

/* CH_CTL_H */
#define CH_CTL_H_IOC_BLKTFR		BIT(26)
#define CH_CTL_H_LLI_LAST		BIT(30)
#define CH_CTL_H_LLI_VALID		BIT(31)


enum {
	DWAXIDMAC_ARWLEN_1		= 0,
	DWAXIDMAC_ARWLEN_2		= 1,
	DWAXIDMAC_ARWLEN_4		= 3,
	DWAXIDMAC_ARWLEN_8		= 7,
	DWAXIDMAC_ARWLEN_16		= 15,
	DWAXIDMAC_ARWLEN_32		= 31,
	DWAXIDMAC_ARWLEN_64		= 63,
	DWAXIDMAC_ARWLEN_128		= 127,
	DWAXIDMAC_ARWLEN_256		= 255,
	DWAXIDMAC_ARWLEN_MIN		= DWAXIDMAC_ARWLEN_1,
	DWAXIDMAC_ARWLEN_MAX		= DWAXIDMAC_ARWLEN_256
};



/* CH_CTL_L */
#define CH_CTL_L_LAST_WRITE_EN		BIT(30)
#define CH_CTL_L_DST_WIDTH_POS		11
#define CH_CTL_L_SRC_WIDTH_POS		8
#define CH_CTL_L_DST_INC_POS		6
#define CH_CTL_L_SRC_INC_POS		4
#define CH_CTL_L_DST_MAST		BIT(2)
#define CH_CTL_L_SRC_MAST		BIT(0)

enum {
	DWAXIDMAC_BURST_TRANS_LEN_1	= 0,
	DWAXIDMAC_BURST_TRANS_LEN_4,
	DWAXIDMAC_BURST_TRANS_LEN_8,
	DWAXIDMAC_BURST_TRANS_LEN_16,
	DWAXIDMAC_BURST_TRANS_LEN_32,
	DWAXIDMAC_BURST_TRANS_LEN_64,
	DWAXIDMAC_BURST_TRANS_LEN_128,
	DWAXIDMAC_BURST_TRANS_LEN_256,
	DWAXIDMAC_BURST_TRANS_LEN_512,
	DWAXIDMAC_BURST_TRANS_LEN_1024
};

enum {
	DWAXIDMAC_CH_CTL_L_INC		= 0,
	DWAXIDMAC_CH_CTL_L_NOINC
};


/* CH_CFG_H */
#define CH_CFG_H_PRIORITY_POS		17
#define CH_CFG_H_HS_SEL_DST_POS		4
#define CH_CFG_H_HS_SEL_SRC_POS		3
enum {
	DWAXIDMAC_HS_SEL_HW		= 0,
	DWAXIDMAC_HS_SEL_SW
};

#define CH_CFG_H_TT_FC_POS		0
enum {
	DWAXIDMAC_TT_FC_MEM_TO_MEM_DMAC	= 0,
	DWAXIDMAC_TT_FC_MEM_TO_PER_DMAC,
	DWAXIDMAC_TT_FC_PER_TO_MEM_DMAC,
	DWAXIDMAC_TT_FC_PER_TO_PER_DMAC,
	DWAXIDMAC_TT_FC_PER_TO_MEM_SRC,
	DWAXIDMAC_TT_FC_PER_TO_PER_SRC,
	DWAXIDMAC_TT_FC_MEM_TO_PER_DST,
	DWAXIDMAC_TT_FC_PER_TO_PER_DST
};
/* CH_CFG_L */
#define CH_CFG_L_DST_MULTBLK_TYPE_POS	2
#define CH_CFG_L_SRC_MULTBLK_TYPE_POS	0
enum {
	DWAXIDMAC_MBLK_TYPE_CONTIGUOUS	= 0,
	DWAXIDMAC_MBLK_TYPE_RELOAD,
	DWAXIDMAC_MBLK_TYPE_SHADOW_REG,
	DWAXIDMAC_MBLK_TYPE_LL
};

/**
 * DW AXI DMA channel interrupts
 *
 * @DWAXIDMAC_IRQ_NONE: Bitmask of no one interrupt
 * @DWAXIDMAC_IRQ_BLOCK_TRF: Block transfer complete
 * @DWAXIDMAC_IRQ_DMA_TRF: Dma transfer complete
 * @DWAXIDMAC_IRQ_SRC_TRAN: Source transaction complete
 * @DWAXIDMAC_IRQ_DST_TRAN: Destination transaction complete
 * @DWAXIDMAC_IRQ_SRC_DEC_ERR: Source decode error
 * @DWAXIDMAC_IRQ_DST_DEC_ERR: Destination decode error
 * @DWAXIDMAC_IRQ_SRC_SLV_ERR: Source slave error
 * @DWAXIDMAC_IRQ_DST_SLV_ERR: Destination slave error
 * @DWAXIDMAC_IRQ_LLI_RD_DEC_ERR: LLI read decode error
 * @DWAXIDMAC_IRQ_LLI_WR_DEC_ERR: LLI write decode error
 * @DWAXIDMAC_IRQ_LLI_RD_SLV_ERR: LLI read slave error
 * @DWAXIDMAC_IRQ_LLI_WR_SLV_ERR: LLI write slave error
 * @DWAXIDMAC_IRQ_INVALID_ERR: LLI invalid error or Shadow register error
 * @DWAXIDMAC_IRQ_MULTIBLKTYPE_ERR: Slave Interface Multiblock type error
 * @DWAXIDMAC_IRQ_DEC_ERR: Slave Interface decode error
 * @DWAXIDMAC_IRQ_WR2RO_ERR: Slave Interface write to read only error
 * @DWAXIDMAC_IRQ_RD2RWO_ERR: Slave Interface read to write only error
 * @DWAXIDMAC_IRQ_WRONCHEN_ERR: Slave Interface write to channel error
 * @DWAXIDMAC_IRQ_SHADOWREG_ERR: Slave Interface shadow reg error
 * @DWAXIDMAC_IRQ_WRONHOLD_ERR: Slave Interface hold error
 * @DWAXIDMAC_IRQ_LOCK_CLEARED: Lock Cleared Status
 * @DWAXIDMAC_IRQ_SRC_SUSPENDED: Source Suspended Status
 * @DWAXIDMAC_IRQ_SUSPENDED: Channel Suspended Status
 * @DWAXIDMAC_IRQ_DISABLED: Channel Disabled Status
 * @DWAXIDMAC_IRQ_ABORTED: Channel Aborted Status
 * @DWAXIDMAC_IRQ_ALL_ERR: Bitmask of all error interrupts
 * @DWAXIDMAC_IRQ_ALL: Bitmask of all interrupts
 */
enum {
	DWAXIDMAC_IRQ_NONE		= 0,
	DWAXIDMAC_IRQ_BLOCK_TRF		= BIT(0),
	DWAXIDMAC_IRQ_DMA_TRF		= BIT(1),
	DWAXIDMAC_IRQ_SRC_TRAN		= BIT(3),
	DWAXIDMAC_IRQ_DST_TRAN		= BIT(4),
	DWAXIDMAC_IRQ_SRC_DEC_ERR	= BIT(5),
	DWAXIDMAC_IRQ_DST_DEC_ERR	= BIT(6),
	DWAXIDMAC_IRQ_SRC_SLV_ERR	= BIT(7),
	DWAXIDMAC_IRQ_DST_SLV_ERR	= BIT(8),
	DWAXIDMAC_IRQ_LLI_RD_DEC_ERR	= BIT(9),
	DWAXIDMAC_IRQ_LLI_WR_DEC_ERR	= BIT(10),
	DWAXIDMAC_IRQ_LLI_RD_SLV_ERR	= BIT(11),
	DWAXIDMAC_IRQ_LLI_WR_SLV_ERR	= BIT(12),
	DWAXIDMAC_IRQ_INVALID_ERR	= BIT(13),
	DWAXIDMAC_IRQ_MULTIBLKTYPE_ERR	= BIT(14),
	DWAXIDMAC_IRQ_DEC_ERR		= BIT(16),
	DWAXIDMAC_IRQ_WR2RO_ERR		= BIT(17),
	DWAXIDMAC_IRQ_RD2RWO_ERR	= BIT(18),
	DWAXIDMAC_IRQ_WRONCHEN_ERR	= BIT(19),
	DWAXIDMAC_IRQ_SHADOWREG_ERR	= BIT(20),
	DWAXIDMAC_IRQ_WRONHOLD_ERR	= BIT(21),
	DWAXIDMAC_IRQ_LOCK_CLEARED	= BIT(27),
	DWAXIDMAC_IRQ_SRC_SUSPENDED	= BIT(28),
	DWAXIDMAC_IRQ_SUSPENDED		= BIT(29),
	DWAXIDMAC_IRQ_DISABLED		= BIT(30),
	DWAXIDMAC_IRQ_ABORTED		= BIT(31),
	DWAXIDMAC_IRQ_ALL_ERR		= (GENMASK(21, 16) | GENMASK(14, 5)),
	DWAXIDMAC_IRQ_ALL		= GENMASK(31, 0)
};

#define AXI_DMA_CTLL_DST_WIDTH(n) ((n) << 11) /* bytes per element */
#define AXI_DMA_CTLL_SRC_WIDTH(n) ((n) << 8)

#define AXI_DMA_CTLL_DST_INC_MODE(n) ((n) << 6)
#define AXI_DMA_CTLL_SRC_INC_MODE(n) ((n) << 4)

#define AXI_DMA_CTLL_DST_MSIZE(n) ((n) << 18)
#define AXI_DMA_CTLL_SRC_MSIZE(n) ((n) << 14)


#define AXI_DMA_CTLL_DMS(n) ((n) << 2)
#define AXI_DMA_CTLL_SMS(n) ((n) << 0)

//caution ,diff with ahb dma
#define AXI_DMA_CFGH_FC(n) ((n) << 0)
#define AXI_DMA_CFGH_DST_PER(n) ((n) << 3)

/*********************************
 *
 *
 *
 *********************************/
/* this is the ip reg offset....don't change!!!!!!! */
#define DW_DMA_MAX_NR_CHANNELS 8

/*
 * Redefine this macro to handle differences between 32- and 64-bit
 * addressing, big vs. little endian, etc.
 */
#define DW_REG(name)  \
    rt_uint32_t name; \
    rt_uint32_t __pad_##name

#define __dma_raw_writeb(v, a) (*(volatile unsigned char *)(a) = (v))
#define __dma_raw_writew(v, a) (*(volatile unsigned short *)(a) = (v))
#define __dma_raw_writel(v, a) (*(volatile unsigned int *)(a) = (v))

#define __dma_raw_readb(a) (*(volatile unsigned char *)(a))
#define __dma_raw_readw(a) (*(volatile unsigned short *)(a))
#define __dma_raw_readl(a) (*(volatile unsigned int *)(a))

#define dw_readl(dw, name) \
    __dma_raw_readl(&(((struct dw_axi_dma_regs *)dw->regs)->name))
#define dw_writel(dw, name, val) \
    __dma_raw_writel((val), &(((struct dw_axi_dma_regs *)dw->regs)->name))
#define dw_readw(dw, name) \
    __dma_raw_readw(&(((struct dw_axi_dma_regs *)dw->regs)->name))
#define dw_writew(dw, name, val) \
    __dma_raw_writew((val), &(((struct dw_axi_dma_regs *)dw->regs)->name))


/****************************************************************************
 * ADT section
 *  add definition of user defined Data Type that only be used in this file here
 ***************************************************************************/

/* Hardware register definitions. */
struct dw_axi_dma_chan_regs
{
	DW_REG(SAR);			/* 0x0 ~ 0x7*/
    DW_REG(DAR);			/* 0x8 ~ 0xf*/
	DW_REG(BLOCK_TS);		/* 0x10 ~ 0x17*/
    rt_uint32_t CTL_LO;
    rt_uint32_t CTL_HI;				/* 0x18 ~ 0x1f*/
    rt_uint32_t CFG_LO;
    rt_uint32_t CFG_HI;				/* 0x20 ~ 0x27*/
    DW_REG(LLP);			/* 0x28 ~ 0x2f*/
    rt_uint32_t STATUS_LO;
    rt_uint32_t STATUS_HI;			/* 0x30 ~ 0x37*/
    DW_REG(SWHS_SRC);		/* 0x38 ~ 0x3f*/
	DW_REG(SWHS_DST);		/* 0x40 ~ 0x47*/
	DW_REG(BLK_TFR_RESU);
	DW_REG(ID);
	DW_REG(QOS);
	DW_REG(SSTAT);
	DW_REG(DSTAT);
	DW_REG(SSTATAR);
	DW_REG(DSTATAR);
	rt_uint32_t INTSTATUS_EN_LO;
	rt_uint32_t INTSTATUS_EN_HI;
	rt_uint32_t INTSTATUS_LO;
	rt_uint32_t INTSTATUS_HI;
	rt_uint32_t INTSIGNAL_LO;
	rt_uint32_t INTSIGNAL_HI;
	rt_uint32_t INTCLEAR_LO;
	rt_uint32_t INTCLEAR_HI;
	rt_uint32_t rev[24];
};

struct dw_axi_dma_regs
{
	DW_REG(ID);					/* 0x0 */
	DW_REG(COMPVER);			/* 0x8 */
	DW_REG(CFG);				/* 0x10 */
	rt_uint32_t CHEN_LO;				/* 0x18 */
	rt_uint32_t CHEN_HI;				/* 0x1c */
	DW_REG(reserved_20_27);		/* 0x20 */
	DW_REG(reserved_28_2f);		/* 0x28 */
	DW_REG(INTSTATUS);			/* 0x30 */
	DW_REG(COM_INTCLEAR);		/* 0x38 */
	DW_REG(COM_INTSTATUS_EN);	/* 0x40 */
	DW_REG(COM_INTSIGNAL_EN);	/* 0x48 */
	DW_REG(COM_INTSTATUS);		/* 0x50 */
	DW_REG(RESET);				/* 0x58 */
	rt_uint32_t reserved[40];			/* 0x60 */
	struct dw_axi_dma_chan_regs CHAN[DW_DMA_MAX_NR_CHANNELS];/* 0x100 */
};

struct dw_axi_dma
{
    /* vadd */
    void *regs;
    /* padd */
    rt_uint32_t paddr;
    rt_uint32_t irq;
    rt_uint32_t channel_max_number;

#define CONTROLLER_STATUS_CLOSED (0)
#define CONTROLLER_STATUS_OPEN (1)
    rt_uint32_t controller_status;
    rt_uint32_t id;
    char *name;
    rt_uint32_t channel_work_done;
};

struct dma_channel
{
#define CHANNEL_STATUS_CLOSED (0)
#define CHANNEL_STATUS_OPEN (1)
#define CHANNEL_STATUS_IDLE (2)
#define CHANNEL_STATUS_BUSY (3)

    rt_uint32_t channel_status;  /* open, busy ,closed */
    rt_uint32_t desc_trans_size;

    /* isr will set it complete. */
    struct rt_completion transfer_completion;
    /* add lock,when set the channel.lock it */
    struct rt_semaphore channel_lock;
    /* struct rt_mutex                 lock; */
    /* rt_enter_critical(); */
    rt_list_t queue;
    /* active transfer now!!! */
    struct dma_transfer *active_trans;

#define SINGLE_TRANSFER (0)
#define CYCLIC_TRANSFER (1)
#define DEFAULT_TRANSFER SINGLE_TRANSFER
    rt_uint32_t open_flag;
    /*  */

    /* new add para... */
    rt_uint32_t desc_total_no;
    rt_uint32_t free_index;
    rt_uint32_t used_index;
    rt_uint32_t desc_left_cnt;

    rt_uint32_t allign_malloc;
    struct axi_dma_lli *base_lli;
};


struct fh_dma
{
    /* core use ,this must be the first para!!!! */
    struct rt_dma_device parent;
    /* myown */
    struct dw_axi_dma dwc;
    /* channel obj */
    struct dma_channel dma_channel[DW_DMA_MAX_NR_CHANNELS];
    struct s_spdup_info dri_spdup_info;
    rt_uint32_t dma_ref_cnt;
    /* struct rt_workqueue* isr_workqueue; */
    /* struct rt_work *isr_work; */
};

#define list_for_each_entry_safe(pos, n, head, member)                \
    for (pos = rt_list_entry((head)->next, typeof(*pos), member),     \
        n    = rt_list_entry(pos->member.next, typeof(*pos), member); \
         &pos->member != (head);                                      \
         pos = n, n = rt_list_entry(n->member.next, typeof(*n), member))

static rt_err_t init(struct rt_dma_device *dma);
static rt_err_t control(struct rt_dma_device *dma, int cmd, void *arg);
static void rt_fh_axi_dma_cyclic_stop(struct dma_transfer *p);
static void rt_fh_axi_dma_cyclic_start(struct dma_transfer *p);
static void rt_fh_axi_dma_cyclic_prep(struct fh_dma *fh_dma_p,
                                  struct dma_transfer *p);
static void rt_fh_axi_dma_cyclic_free(struct dma_transfer *p);
static void rt_fh_axi_dma_cyclic_pause(struct dma_transfer *p);
static void rt_fh_axi_dma_cyclic_resume(struct dma_transfer *p);
void mmu_clean_invalidated_dcache(rt_uint32_t buffer, rt_uint32_t size);



#if 0
static void dump_lli(struct axi_dma_lli *p_lli)
{
		rt_kprintf("SAR: 0x%08x DAR: 0x%08x LLP: 0x%08x BTS 0x%08x CTL: 0x%08x:%08x LLI_PHY_ADD: 0x%x\n",
		(p_lli->sar_lo),
		(p_lli->dar_lo),
		(p_lli->llp_lo),
		(p_lli->block_ts_lo),
		(p_lli->ctl_hi),
		(p_lli->ctl_lo),
		(int)p_lli);
}

static void dump_channel_reg(struct fh_dma *p_dma, struct dma_transfer *p_transfer)
{
    struct dw_axi_dma *temp_dwc;
    rt_uint32_t chan_no = p_transfer->channel_number;

    temp_dwc = &p_dma->dwc;

    rt_kprintf("[CHAN : %d]SAR: 0x%x DAR: 0x%x LLP: 0x%x CTL: 0x%x:%08x CFG: 0x%x:%08x INTEN :0x%x INTSTATUS: 0x%x\n",
    chan_no,
    dw_readl(temp_dwc, CHAN[chan_no].SAR),
    dw_readl(temp_dwc, CHAN[chan_no].DAR),
    dw_readl(temp_dwc, CHAN[chan_no].LLP),
    dw_readl(temp_dwc, CHAN[chan_no].CTL_HI),
    dw_readl(temp_dwc, CHAN[chan_no].CTL_LO),
    dw_readl(temp_dwc, CHAN[chan_no].CFG_HI),
    dw_readl(temp_dwc, CHAN[chan_no].CFG_LO),
    dw_readl(temp_dwc, CHAN[chan_no].INTSTATUS_EN_LO),
    dw_readl(temp_dwc, CHAN[chan_no].INTSTATUS_LO)
    );
}

static void dump_dma_common_reg(struct fh_dma *p_dma)
{
    struct dw_axi_dma *temp_dwc;
    temp_dwc = &p_dma->dwc;

    if(!temp_dwc->regs)
        return;
    rt_kprintf("ID: 0x%x COMPVER: 0x%x CFG: 0x%x CHEN: 0x%x:%08x INTSTATUS: 0x%x \
            COM_INTSTATUS_EN : 0x%x COM_INTSIGNAL_EN : %x COM_INTSTATUS : %x\n",
            dw_readl(temp_dwc, ID),
            dw_readl(temp_dwc, COMPVER),
            dw_readl(temp_dwc, CFG),
            dw_readl(temp_dwc, CHEN_HI),
            dw_readl(temp_dwc, CHEN_LO),
            dw_readl(temp_dwc, INTSTATUS),
            dw_readl(temp_dwc, COM_INTSTATUS_EN),
            dw_readl(temp_dwc, COM_INTSIGNAL_EN),
            dw_readl(temp_dwc, COM_INTSTATUS)
            );

}
#endif

static struct rt_dma_ops fh_dma_ops = {init, control};

static rt_uint32_t allign_func(rt_uint32_t in_addr, rt_uint32_t allign_size)
{
    return (in_addr + allign_size - 1) & (~(allign_size - 1));
}

struct axi_dma_lli *get_desc(struct fh_dma *p_dma, struct dma_transfer *p_transfer,
                        rt_uint32_t lli_size)
{
    struct axi_dma_lli *ret_lli;
    rt_uint32_t free_index;
    rt_uint32_t allign_left;
    rt_uint32_t totoal_desc;
    rt_uint32_t actual_get_desc = 0;
    rt_uint32_t totoal_free_desc;

    totoal_free_desc =
        p_dma->dma_channel[p_transfer->channel_number].desc_left_cnt;
    free_index  = p_dma->dma_channel[p_transfer->channel_number].free_index;
    totoal_desc = p_dma->dma_channel[p_transfer->channel_number].desc_total_no;
    allign_left = totoal_desc - free_index;

    /* check first.. */
    if (totoal_free_desc < lli_size)
    {
        rt_kprintf("not enough desc to get...\n");
        rt_kprintf("get size is %d,left is %d\n", lli_size, totoal_free_desc);
        return RT_NULL;
    }

    if (lli_size > allign_left)
    {
        /* if allign desc not enough...just reset null.... */
        if ((totoal_free_desc - allign_left) < lli_size)
        {
            rt_kprintf("not enough desc to get...\n");
            rt_kprintf(
                "app need size is %d, totoal left is %d, allign left is %d\n",
                lli_size, totoal_free_desc, allign_left);
            rt_kprintf("from head to get desc size is %d, actual get is %d\n",
                       (totoal_free_desc - allign_left),
                       (allign_left + lli_size));
            return RT_NULL;
        }
        else
        {
            actual_get_desc = allign_left + lli_size;
            free_index      = 0;
        }
    }

    ret_lli =
        &p_dma->dma_channel[p_transfer->channel_number].base_lli[free_index];

    p_dma->dma_channel[p_transfer->channel_number].free_index +=
        actual_get_desc;
    p_dma->dma_channel[p_transfer->channel_number].free_index %=
        p_dma->dma_channel[p_transfer->channel_number].desc_total_no;
    p_dma->dma_channel[p_transfer->channel_number].desc_left_cnt -=
        actual_get_desc;
    p_transfer->lli_size = lli_size;
    p_transfer->actual_lli_size = actual_get_desc;
    return ret_lli;
}

rt_uint32_t put_desc(struct fh_dma *p_dma, struct dma_transfer *p_transfer)
{
    rt_uint32_t lli_size;

    lli_size   = p_transfer->actual_lli_size;
    p_dma->dma_channel[p_transfer->channel_number].used_index += lli_size;
    p_dma->dma_channel[p_transfer->channel_number].used_index %=
        p_dma->dma_channel[p_transfer->channel_number].desc_total_no;
    p_dma->dma_channel[p_transfer->channel_number].desc_left_cnt += lli_size;
    p_transfer->lli_size = 0;
    p_transfer->actual_lli_size = 0;
    return 0;
}

static rt_err_t init(struct rt_dma_device *dma)
{
    return RT_EOK;
}

#if defined(CONFIG_ARCH_FH8636_FH8852V20X) || defined(CONFIG_ARCH_FH8852V301_FH8662V100)
#define axi_dma_reset(...)
#else
static void axi_dma_reset(struct dw_axi_dma *axi_dma_obj)
{
	rt_uint32_t ret;
	dw_writel(axi_dma_obj, RESET, 1);
	do{
		ret = dw_readl(axi_dma_obj, RESET);
	}while(ret);
}
#endif

static void axi_dma_enable(struct dw_axi_dma *axi_dma_obj)
{
	rt_uint32_t ret;
	ret = dw_readl(axi_dma_obj, CFG);
	ret |= BIT(DMAC_EN_POS);
	dw_writel(axi_dma_obj, CFG, ret);
}

static void axi_dma_isr_common_enable(struct dw_axi_dma *axi_dma_obj)
{
	rt_uint32_t ret;
	ret = dw_readl(axi_dma_obj, CFG);
	ret |= BIT(INT_EN_POS);
	dw_writel(axi_dma_obj, CFG, ret);
}

static void axi_dma_isr_common_disable(struct dw_axi_dma *axi_dma_obj)
{
	rt_uint32_t ret;
	ret = dw_readl(axi_dma_obj, CFG);
	ret &= ~(BIT(INT_EN_POS));
	dw_writel(axi_dma_obj, CFG, ret);
}

static void handle_dma_open(struct fh_dma *p_dma)
{
    struct dw_axi_dma *temp_dwc;

    temp_dwc = &p_dma->dwc;
    axi_dma_enable(temp_dwc);
    axi_dma_isr_common_enable(temp_dwc);
    p_dma->dwc.controller_status = CONTROLLER_STATUS_OPEN;

}

static void handle_dma_close(struct fh_dma *p_dma)
{

    rt_uint32_t i;
#if !defined(CONFIG_ARCH_FH8636_FH8852V20X) && !defined(CONFIG_ARCH_FH8852V301_FH8662V100)
    struct dw_axi_dma *temp_dwc;
    temp_dwc = &p_dma->dwc;
#endif

    if (p_dma->dma_ref_cnt != 0)
    {
        rt_kprintf("can't close dma control. dma_ref_cnt is %d\n",p_dma->dma_ref_cnt);
        return;
    }

    /* take lock */
    for (i = 0; i < p_dma->dwc.channel_max_number; i++)
    {
        p_dma->dma_channel[i].channel_status = CHANNEL_STATUS_CLOSED;
    }
#if !defined(CONFIG_ARCH_FH8636_FH8852V20X) && !defined(CONFIG_ARCH_FH8852V301_FH8662V100)
    axi_dma_reset(temp_dwc);
#endif
    p_dma->dwc.controller_status = CONTROLLER_STATUS_CLOSED;

}

#define CHANNEL_REAL_FREE (0)
#define CHANNEL_NOT_FREE (1)
static rt_uint32_t check_channel_real_free(struct fh_dma *p_dma,
                                           rt_uint32_t channel_number)
{
    struct dw_axi_dma *temp_dwc;
    rt_uint32_t ret_status;

    temp_dwc = &p_dma->dwc;
    RT_ASSERT(channel_number < p_dma->dwc.channel_max_number);

    ret_status = dw_readl(temp_dwc, CHEN_LO);
    if (ret_status & lift_shift_bit_num(channel_number))
    {
        rt_kprintf("channel_number : %d is busy....\n",channel_number);
        return CHANNEL_NOT_FREE;
    }
    return CHANNEL_REAL_FREE;
}

static rt_err_t handle_request_channel(struct fh_dma *p_dma,
                                       struct dma_transfer *p_transfer)
{
    rt_uint32_t i;
    rt_err_t ret_status = RT_EOK;

    /* handle if auto check channel... */
    if (p_transfer->channel_number == AUTO_FIND_CHANNEL)
    {
        /* check each channel lock,find a free channel... */
        for (i = 0; i < p_dma->dwc.channel_max_number; i++)
        {
            ret_status = rt_sem_trytake(&p_dma->dma_channel[i].channel_lock);
            if (ret_status == RT_EOK)
            {
                break;
            }
        }

        if (i < p_dma->dwc.channel_max_number)
        {
            ret_status = check_channel_real_free(p_dma, i);
            if (ret_status != CHANNEL_REAL_FREE)
            {
                FH_DMA_DEBUG("auto request channel error\n");
                RT_ASSERT(ret_status == CHANNEL_REAL_FREE);
            }
            /* caution : channel is already locked here.... */
            p_transfer->channel_number = i;
            /* bind to the controller. */
            /* p_transfer->dma_controller = p_dma; */
            p_dma->dma_channel[i].channel_status = CHANNEL_STATUS_OPEN;
        }
        else
        {
            rt_kprintf("[dma]: auto request err, no free channel\n");
            return -RT_ENOMEM;
        }

    }

    /* request channel by user */
    else
    {
        /*  */
        RT_ASSERT(p_transfer->channel_number < p_dma->dwc.channel_max_number);
        ret_status = rt_sem_take(
            &p_dma->dma_channel[p_transfer->channel_number].channel_lock,
            RT_TICK_PER_SECOND * 2);
        if (ret_status != RT_EOK)
        {
            rt_kprintf("[dma]: request %d channel err.\n", p_transfer->channel_number);
            return -RT_ENOMEM;
        }

        // rt_enter_critical();
        ret_status = check_channel_real_free(p_dma, p_transfer->channel_number);
        if (ret_status != CHANNEL_REAL_FREE)
        {
            FH_DMA_DEBUG("user request channel error\n");
            RT_ASSERT(ret_status == CHANNEL_REAL_FREE);
        }

        /* bind to the controller */
        /* p_transfer->dma_controller = p_dma; */
        p_dma->dma_channel[p_transfer->channel_number].channel_status =
            CHANNEL_STATUS_OPEN;
        /* rt_exit_critical(); */
    }

    /* malloc desc for this one channel... */
    /* fix me.... */

    p_dma->dma_channel[p_transfer->channel_number].allign_malloc =
        (rt_uint32_t)rt_malloc(
            (p_dma->dma_channel[p_transfer->channel_number].desc_total_no *
             sizeof(struct axi_dma_lli)) +
            AXI_DESC_ALLIGN);

    if (!p_dma->dma_channel[p_transfer->channel_number].allign_malloc)
    {
        /* release channel */
        rt_kprintf("[dma]: no mem to malloc channel%d desc..\n",
                   p_transfer->channel_number);
        p_dma->dma_channel[p_transfer->channel_number].channel_status =
            CHANNEL_STATUS_CLOSED;
        rt_sem_release(
            &p_dma->dma_channel[p_transfer->channel_number].channel_lock);
        return -RT_ENOMEM;
    }

    p_dma->dma_channel[p_transfer->channel_number].base_lli =
        (struct axi_dma_lli *)allign_func(
            p_dma->dma_channel[p_transfer->channel_number].allign_malloc,
            AXI_DESC_ALLIGN);
    /* t1 = (UINT32)rt_malloc(GMAC_TX_RING_SIZE * */
    /* sizeof(Gmac_Tx_DMA_Descriptors) + AXI_DESC_ALLIGN); */

    if (!p_dma->dma_channel[p_transfer->channel_number].base_lli)
    {
        FH_DMA_DEBUG("request desc failed..\n");
        RT_ASSERT(p_dma->dma_channel[p_transfer->channel_number].base_lli !=
                  RT_NULL);
    }

    if ((rt_uint32_t)p_dma->dma_channel[p_transfer->channel_number].base_lli %
        AXI_DESC_ALLIGN)
    {
        rt_kprintf("malloc is not cache allign..");
    }

    /* rt_memset((void *)dma_trans_desc->first_lli, 0, lli_size * sizeof(struct */
    /* axi_dma_lli)); */
    rt_memset((void *)p_dma->dma_channel[p_transfer->channel_number].base_lli,
              0, p_dma->dma_channel[p_transfer->channel_number].desc_total_no *
                     sizeof(struct axi_dma_lli));

    p_dma->dma_channel[p_transfer->channel_number].desc_left_cnt =
        p_dma->dma_channel[p_transfer->channel_number].desc_total_no;
    p_dma->dma_channel[p_transfer->channel_number].free_index = 0;
    p_dma->dma_channel[p_transfer->channel_number].used_index = 0;
    p_dma->dma_ref_cnt++;
    return RT_EOK;
}


static  void fhc_chan_able_set(struct fh_dma *p_dma,
struct dma_transfer *p_transfer, int enable)
{

    struct dw_axi_dma *temp_dwc;
    int ret, ret_read;
    rt_uint32_t time_out = 0xffffff;
    rt_uint32_t chan_mask = lift_shift_bit_num(p_transfer->channel_number);
    temp_dwc = &p_dma->dwc;

    ret = dw_readl(temp_dwc, CHEN_LO);
    ret &= ~(chan_mask);
	ret |= enable ? ((chan_mask) | (chan_mask << 8)) :  (chan_mask << 8);

    dw_writel(temp_dwc, CHEN_LO, ret);

    /*enable == 1 do not check. maybe just isr break. the bit dma will self clear*/
    if (enable == 0)
    {
        do
        {
            ret_read = dw_readl(temp_dwc, CHEN_LO);
            time_out--;
        } while ((!!(ret_read & chan_mask) != enable) && time_out);
    }

    if (!time_out)
        rt_kprintf("fhc_chan_able_set %x timeout\n",enable);
}

static void fhc_chan_susp_set(struct fh_dma *p_dma,
struct dma_transfer *p_transfer, int enable)
{
    struct dw_axi_dma *temp_dwc;
    int ret;
    int chan_no = p_transfer->channel_number;
    rt_uint32_t chan_mask = lift_shift_bit_num(p_transfer->channel_number);
    temp_dwc = &p_dma->dwc;

    ret = dw_readl(temp_dwc, CHEN_LO);
    ret &= ~(chan_mask << 16);
	ret |= enable ? ((chan_mask << 16) | (chan_mask << 24)) :  (chan_mask << 24);
    dw_writel(temp_dwc, CHEN_LO, ret);

	if(enable) {
		do{
           ret = dw_readl(temp_dwc, CHAN[chan_no].INTSTATUS_LO);
		}while(!!(ret & DWAXIDMAC_IRQ_SUSPENDED) != enable);
		/*clear susp irp*/
		dw_writel(temp_dwc, CHAN[chan_no].INTCLEAR_LO, ret);
	}

}




static rt_uint32_t handle_release_channel(struct fh_dma *p_dma,
                                          struct dma_transfer *p_transfer)
{
    rt_uint32_t ret_status;

    /* rt_enter_critical(); */
    ret_status = p_dma->dma_channel[p_transfer->channel_number].channel_status;

    RT_ASSERT(p_transfer->channel_number < p_dma->dwc.channel_max_number);

    if (ret_status == CHANNEL_STATUS_CLOSED)
    {
        FH_DMA_DEBUG(
            "release channel error,reason: release a closed channel!!\n");
        RT_ASSERT(ret_status != CHANNEL_STATUS_CLOSED);
    }
    fhc_chan_able_set(p_dma, p_transfer, 0);

    rt_sem_release(
        &p_dma->dma_channel[p_transfer->channel_number].channel_lock);
    /* p_transfer->dma_controller = RT_NULL; */
    p_dma->dma_channel[p_transfer->channel_number].channel_status =
        CHANNEL_STATUS_CLOSED;
    p_dma->dma_channel[p_transfer->channel_number].open_flag = DEFAULT_TRANSFER;
    /* rt_exit_critical(); */

    /* release this channel malloc mem... */
    /* fix me..... */
    rt_free(
        (void *)p_dma->dma_channel[p_transfer->channel_number].allign_malloc);
    p_dma->dma_channel[p_transfer->channel_number].allign_malloc = RT_NULL;
    p_dma->dma_channel[p_transfer->channel_number].base_lli      = RT_NULL;
    p_dma->dma_channel[p_transfer->channel_number].desc_left_cnt =
        p_dma->dma_channel[p_transfer->channel_number].desc_total_no;
    p_dma->dma_channel[p_transfer->channel_number].free_index = 0;
    p_dma->dma_channel[p_transfer->channel_number].used_index = 0;
    p_dma->dma_ref_cnt--;
    return RT_EOK;

}

static rt_uint32_t cal_lli_size(struct dma_transfer *p_transfer)
{

    RT_ASSERT(p_transfer != RT_NULL);
    RT_ASSERT(p_transfer->dma_controller != RT_NULL);
    RT_ASSERT(p_transfer->src_width <= DW_DMA_SLAVE_WIDTH_32BIT);
    rt_uint32_t lli_number                = 0;
    rt_uint32_t channel_max_trans_per_lli = 0;
    rt_uint32_t c_msize;
    c_msize = p_transfer->dma_controller->dma_channel[p_transfer->channel_number]
            .desc_trans_size;
    channel_max_trans_per_lli = (p_transfer->period_len != 0) ?
    (MIN(p_transfer->period_len, c_msize)) : (c_msize);
    lli_number = (p_transfer->trans_len % channel_max_trans_per_lli) ? 1 : 0;
    lli_number += p_transfer->trans_len / channel_max_trans_per_lli;
    FH_DMA_DEBUG("cal lli size is %x\n",lli_number);
    return lli_number;

}

void axi_dma_ctl_init(struct fh_dma *p_dma, rt_uint32_t *low, rt_uint32_t *high, struct dma_transfer *p_transfer)
{
    if (p_transfer->fc_mode == DMA_M2M)
    {
        /*cache*/
        *low |= CH_CTL_L_AR_CACHE(3) | CH_CTL_L_AW_CACHE(0);
        /*burst len*/
        *high |= CH_CTL_H_ARLEN_EN(1) | CH_CTL_H_ARLEN(0xf) | CH_CTL_H_AWLEN_EN(0) | CH_CTL_H_AWLEN(0xf);
        return;
    }

    if (p_dma->dri_spdup_info.spdup_flag == AXI_SPD_UP_ACTIVE)
    {
        /*cache*/
        *low |= p_dma->dri_spdup_info.spdup_ctl_low;
        /*burst len*/
        *high |= p_dma->dri_spdup_info.spdup_ctl_high;
    }
    else
    {
        /*cache*/
        *low |= CH_CTL_L_AR_CACHE(3) | CH_CTL_L_AW_CACHE(0);
        /*burst len*/
        *high |= CH_CTL_H_ARLEN_EN(1) | CH_CTL_H_ARLEN(0xf) | CH_CTL_H_AWLEN_EN(0) | CH_CTL_H_AWLEN(0xf);
    }

}

void axi_dma_cfg_init(struct fh_dma *p_dma, rt_uint32_t *low, rt_uint32_t *high, struct dma_transfer *p_transfer)
{
    if (p_transfer->fc_mode == DMA_M2M)
    {
        /*out standing*/
        *high |= CH_CFG_H_DST_OSR_LMT(7) | CH_CFG_H_SRC_OSR_LMT(7);
        return;
    }

    if (p_dma->dri_spdup_info.spdup_flag == AXI_SPD_UP_ACTIVE)
    {
        *low |= p_dma->dri_spdup_info.spdup_cfg_low;
        *high |= p_dma->dri_spdup_info.spdup_cfg_high;
    }
    else
    {
        /*out standing*/
        *high |= CH_CFG_H_DST_OSR_LMT(7) | CH_CFG_H_SRC_OSR_LMT(7);
    }
}

static void handle_single_transfer(struct fh_dma *p_dma,
                                   struct dma_transfer *p_transfer)
{
    rt_uint32_t i;
    struct dw_axi_dma *temp_dwc;

    temp_dwc = &p_dma->dwc;
    volatile rt_uint32_t ret_status;
    rt_list_t *p_controller_list;
    rt_uint32_t lli_size, max_trans_size;
    struct axi_dma_lli *p_lli = RT_NULL;
    struct dma_transfer *dma_trans_desc;
    struct dma_transfer *_dma_trans_desc;
    rt_uint32_t cfg_low;
    rt_uint32_t cfg_high;
    rt_uint32_t temp_src_add = 0;
    rt_uint32_t temp_dst_add = 0;
    rt_uint32_t trans_total_len = 0;
    rt_uint32_t temp_trans_size = 0;
    rt_uint32_t src_inc_mode;
	rt_uint32_t dst_inc_mode;
    rt_uint32_t isr = DWAXIDMAC_IRQ_DMA_TRF | DWAXIDMAC_IRQ_BLOCK_TRF | DWAXIDMAC_IRQ_SUSPENDED;
    rt_uint32_t isr_sig = DWAXIDMAC_IRQ_DMA_TRF | DWAXIDMAC_IRQ_BLOCK_TRF;
    /* rt_uint32_t dma_channl_no = 0; */

    RT_ASSERT(p_transfer->channel_number < p_dma->dwc.channel_max_number);
    RT_ASSERT(p_transfer->dma_number < DMA_CONTROLLER_NUMBER);
    /* when the dma transfer....the lock should be 0!!!! */
    /* or user may not request the channel... */
    RT_ASSERT(
        p_dma->dma_channel[p_transfer->channel_number].channel_lock.value == 0);

    ret_status = p_dma->dma_channel[p_transfer->channel_number].channel_status;
    if (ret_status == CHANNEL_STATUS_CLOSED)
    {
        FH_DMA_DEBUG("transfer error,reason: use a closed channel..\n");
        RT_ASSERT(ret_status != CHANNEL_STATUS_CLOSED);
    }
    p_transfer->dma_controller = p_dma;

    rt_list_init(&p_transfer->transfer_list);
    max_trans_size =
        p_transfer->dma_controller->dma_channel[p_transfer->channel_number]
            .desc_trans_size;
    if(p_transfer->period_len != 0){
        max_trans_size = MIN(max_trans_size, p_transfer->period_len);
    }
    /* add transfer to the controller's queue list */
    /* here should  insert before and handle after....this could be a fifo... */
    rt_list_insert_before(&p_dma->dma_channel[p_transfer->channel_number].queue,
                          &p_transfer->transfer_list);

    p_controller_list = &p_dma->dma_channel[p_transfer->channel_number].queue;

    /* here the driver could make a queue to cache the transfer and kick a */
    /* thread to handle the queue~~~ */
    /* but now,this is a easy version...,just handle the transfer now!!! */
    list_for_each_entry_safe(dma_trans_desc, _dma_trans_desc, p_controller_list,
                             transfer_list)
    {
        /* the dma controller could see the active transfer ..... */
        p_transfer->dma_controller->dma_channel[p_transfer->channel_number]
            .active_trans = dma_trans_desc;

        trans_total_len = p_transfer->trans_len;

        /* handle desc */
        /* step1:cal lli size... */
        lli_size = cal_lli_size(dma_trans_desc);
        /* step2:malloc lli_size  mem */
        /* dma_trans_desc->first_lli = (struct axi_dma_lli *)rt_malloc(lli_size * */
        /* sizeof(struct axi_dma_lli)); */

        dma_trans_desc->first_lli = get_desc(p_dma, p_transfer, lli_size);

        /* not enough mem.. */
        if (dma_trans_desc->first_lli == RT_NULL)
        {
            FH_DMA_DEBUG("transfer error,reason: not enough mem..\n");
            RT_ASSERT(dma_trans_desc->first_lli != RT_NULL);
        }

        /* bug here.... */
        rt_memset((void *)dma_trans_desc->first_lli, 0,
                  lli_size * sizeof(struct axi_dma_lli));

        p_lli = dma_trans_desc->first_lli;

        /* warnning!!!!must check if the add is 64Byte ally... */
        RT_ASSERT(((rt_uint32_t)p_lli & AXI_DESC_ALLIGN_BIT_MASK) == 0);

        RT_ASSERT(dma_trans_desc->dst_inc_mode <= DW_DMA_SLAVE_FIX);
        RT_ASSERT(dma_trans_desc->src_inc_mode <= DW_DMA_SLAVE_FIX);
        /* step3: set the mem.. */

        for (i = 0; i < lli_size; i++)
        {
            /* parse trans para... */
            /* para add: */

            switch (dma_trans_desc->dst_inc_mode)
            {
            case DW_DMA_SLAVE_INC:
                temp_dst_add =
                    dma_trans_desc->dst_add +
                    i * max_trans_size * (1 << dma_trans_desc->src_width);
                break;
            case DW_DMA_SLAVE_DEC:
                temp_dst_add =
                    dma_trans_desc->dst_add -
                    i * max_trans_size * (1 << dma_trans_desc->src_width);
                break;
            case DW_DMA_SLAVE_FIX:
                temp_dst_add = dma_trans_desc->dst_add;
                break;
            }

            switch (dma_trans_desc->src_inc_mode)
            {
            case DW_DMA_SLAVE_INC:
                temp_src_add =
                    dma_trans_desc->src_add +
                    i * max_trans_size * (1 << dma_trans_desc->src_width);
                break;
            case DW_DMA_SLAVE_DEC:
                temp_src_add =
                    dma_trans_desc->src_add -
                    i * max_trans_size * (1 << dma_trans_desc->src_width);
                break;
            case DW_DMA_SLAVE_FIX:
                temp_src_add = dma_trans_desc->src_add;
                break;
            }
            if(p_transfer->src_reload_flag == ADDR_RELOAD){
                temp_src_add = dma_trans_desc->src_add;
            }
            if(p_transfer->dst_reload_flag == ADDR_RELOAD){
                temp_dst_add = dma_trans_desc->dst_add;
            }
            p_lli[i].sar_lo = temp_src_add;
            p_lli[i].dar_lo = temp_dst_add;

            /* para ctl */
            temp_trans_size = (trans_total_len / max_trans_size)
                                  ? max_trans_size
                                  : (trans_total_len % max_trans_size);
            trans_total_len -= temp_trans_size;

            RT_ASSERT(dma_trans_desc->dst_width <= DW_DMA_SLAVE_WIDTH_32BIT);
            RT_ASSERT(dma_trans_desc->src_width <= DW_DMA_SLAVE_WIDTH_32BIT);

            RT_ASSERT(dma_trans_desc->dst_msize <= DW_DMA_SLAVE_MSIZE_256);
            RT_ASSERT(dma_trans_desc->src_msize <= DW_DMA_SLAVE_MSIZE_256);
            RT_ASSERT(dma_trans_desc->fc_mode <= DMA_P2P);
            dst_inc_mode = (dma_trans_desc->dst_inc_mode == DW_DMA_SLAVE_INC) ? 0 : 1;
		    src_inc_mode = (dma_trans_desc->src_inc_mode == DW_DMA_SLAVE_INC) ? 0 : 1;

            p_lli[i].ctl_lo =
            AXI_DMA_CTLL_DST_WIDTH(dma_trans_desc->dst_width) |
            AXI_DMA_CTLL_SRC_WIDTH(dma_trans_desc->src_width) |
            AXI_DMA_CTLL_DST_MSIZE(dma_trans_desc->dst_msize) |
            AXI_DMA_CTLL_SRC_MSIZE(dma_trans_desc->src_msize) |
            AXI_DMA_CTLL_DST_INC_MODE(dst_inc_mode) |
            AXI_DMA_CTLL_SRC_INC_MODE(src_inc_mode);


            p_lli[i].ctl_hi = CH_CTL_H_LLI_VALID;
            // p_lli[i].ctl_hi |= CH_CTL_H_ARLEN_EN(1) | CH_CTL_H_ARLEN(7) |
            // CH_CTL_H_AWLEN_EN(1) | CH_CTL_H_AWLEN(7);
            axi_dma_ctl_init(p_dma, &p_lli[i].ctl_lo, &p_lli[i].ctl_hi, dma_trans_desc);
            /* block size */
            p_lli[i].block_ts_lo = temp_trans_size - 1;

            if (trans_total_len > 0)
                p_lli[i].llp_lo = (rt_uint32_t)&p_lli[i + 1];
            else
                p_lli[i].ctl_hi |= CH_CTL_H_LLI_LAST;

            /* flush cache to mem */
            mmu_clean_invalidated_dcache((rt_uint32_t)&p_lli[i],
                                         sizeof(struct axi_dma_lli));
            //dump_lli(&p_lli[i]);
        }

        /* set link enable */
        dw_writel(temp_dwc, CHAN[dma_trans_desc->channel_number].CFG_LO,
        3 << CH_CFG_L_DST_MULTBLK_TYPE_POS | 3 << CH_CFG_L_SRC_MULTBLK_TYPE_POS);
        /* set flow mode */
        dw_writel(temp_dwc, CHAN[dma_trans_desc->channel_number].CFG_HI,
        AXI_DMA_CFGH_FC(dma_trans_desc->fc_mode));
        /* set base link add */
        dw_writel(temp_dwc, CHAN[dma_trans_desc->channel_number].LLP,  (int)&p_lli[0]);
        /* clear isr */
        dw_writel(temp_dwc, CHAN[dma_trans_desc->channel_number].INTCLEAR_LO, 0xffffffff);
        dw_writel(temp_dwc, CHAN[dma_trans_desc->channel_number].INTCLEAR_HI, 0xffffffff);
        /* open isr */
        dw_writel(temp_dwc, CHAN[dma_trans_desc->channel_number].INTSTATUS_EN_LO, isr);
        dw_writel(temp_dwc, CHAN[dma_trans_desc->channel_number].INTSIGNAL_LO, isr_sig);
        RT_ASSERT(dma_trans_desc->dst_hs <= DMA_SW_HANDSHAKING);
        RT_ASSERT(dma_trans_desc->src_hs <= DMA_SW_HANDSHAKING);
        /* only hw handshaking need this.. */
        switch (dma_trans_desc->fc_mode)
        {
        case DMA_M2M:

            break;
        case DMA_M2P:
            ret_status = dw_readl(temp_dwc, CHAN[dma_trans_desc->channel_number].CFG_HI);
            ret_status &= ~0x18;
            ret_status |= dma_trans_desc->dst_per << 12;
            dw_writel(temp_dwc, CHAN[dma_trans_desc->channel_number].CFG_HI, ret_status);

            break;
        case DMA_P2M:
            ret_status = dw_readl(temp_dwc, CHAN[dma_trans_desc->channel_number].CFG_HI);
            ret_status &= ~0x18;
            ret_status |= dma_trans_desc->src_per << 7;
            dw_writel(temp_dwc, CHAN[dma_trans_desc->channel_number].CFG_HI, ret_status);

            break;

        default:
            break;
        }

        /*rewrite cfg..*/
        cfg_low = dw_readl(temp_dwc, CHAN[dma_trans_desc->channel_number].CFG_LO);
        cfg_high = dw_readl(temp_dwc, CHAN[dma_trans_desc->channel_number].CFG_HI);
        axi_dma_cfg_init(p_dma, &cfg_low, &cfg_high, dma_trans_desc);
        dw_writel(temp_dwc, CHAN[dma_trans_desc->channel_number].CFG_LO, cfg_low);
        dw_writel(temp_dwc, CHAN[dma_trans_desc->channel_number].CFG_HI, cfg_high);


        dma_trans_desc->dma_controller
            ->dma_channel[dma_trans_desc->channel_number]
            .channel_status = CHANNEL_STATUS_BUSY;

        if (dma_trans_desc->prepare_callback)
        {
            dma_trans_desc->prepare_callback(dma_trans_desc->prepare_para);
        }
        fhc_chan_able_set(p_dma, dma_trans_desc, 1);
    }

}

static rt_err_t control(struct rt_dma_device *dma, int cmd, void *arg)
{

    struct fh_dma *my_own = (struct fh_dma *)dma->parent.user_data;
    struct dw_axi_dma *temp_dwc = &my_own->dwc;

    rt_err_t ret = RT_EOK;

    struct dma_transfer *p_dma_transfer = (struct dma_transfer *)arg;

    /* FH_DMA_DEBUG("p_dma_transfer value:0x%x\n",(rt_uint32_t)p_dma_transfer); */

    RT_ASSERT(my_own != RT_NULL);

    switch (cmd)
    {
    case RT_DEVICE_CTRL_DMA_OPEN:

        /* open the controller.. */
        handle_dma_open(my_own);
        break;
    case RT_DEVICE_CTRL_DMA_CLOSE:

        /* close the controller.. */
        handle_dma_close(my_own);
        break;
    case RT_DEVICE_CTRL_DMA_REQUEST_CHANNEL:
        /* request a channel for the user */
        RT_ASSERT(p_dma_transfer != RT_NULL);
        ret = handle_request_channel(my_own, p_dma_transfer);

        break;
    case RT_DEVICE_CTRL_DMA_RELEASE_CHANNEL:
        /* release a channel */
        RT_ASSERT(p_dma_transfer != RT_NULL);

        ret = handle_release_channel(my_own, p_dma_transfer);

        break;

    case RT_DEVICE_CTRL_DMA_SINGLE_TRANSFER:
        /* make a channel to transfer data. */
        RT_ASSERT(p_dma_transfer != RT_NULL);
        /* check if the dma channel is open,or return error. */

        my_own->dma_channel[p_dma_transfer->channel_number].open_flag =
            SINGLE_TRANSFER;
        handle_single_transfer(my_own, p_dma_transfer);
        /* then wait for the channel is complete.. */
        /* caution that::we should be in the "rt_enter_critical()"when set the */
        /* dma to work. */
        break;

    case RT_DEVICE_CTRL_DMA_CYCLIC_PREPARE:
        RT_ASSERT(p_dma_transfer != RT_NULL);
        my_own->dma_channel[p_dma_transfer->channel_number].open_flag =
            CYCLIC_TRANSFER;
        rt_fh_axi_dma_cyclic_prep(my_own, p_dma_transfer);
        break;

    case RT_DEVICE_CTRL_DMA_CYCLIC_START:
        rt_fh_axi_dma_cyclic_start(p_dma_transfer);
        break;

    case RT_DEVICE_CTRL_DMA_CYCLIC_STOP:
        rt_fh_axi_dma_cyclic_stop(p_dma_transfer);
        break;

    case RT_DEVICE_CTRL_DMA_CYCLIC_FREE:
        rt_fh_axi_dma_cyclic_free(p_dma_transfer);
        break;
    case RT_DEVICE_CTRL_DMA_PAUSE:
        rt_fh_axi_dma_cyclic_pause(p_dma_transfer);
        break;
    case RT_DEVICE_CTRL_DMA_RESUME:
        rt_fh_axi_dma_cyclic_resume(p_dma_transfer);
        break;
    case RT_DEVICE_CTRL_DMA_GET_DAR:
        return dw_readl(temp_dwc, CHAN[p_dma_transfer->channel_number].DAR);
    case RT_DEVICE_CTRL_DMA_GET_SAR:
        return dw_readl(temp_dwc, CHAN[p_dma_transfer->channel_number].SAR);

    default:
        break;
    }

    return ret;
}


static void rt_fh_isr_single_process(struct fh_dma *my_own, struct dma_transfer *p_transfer){
        if (p_transfer->complete_callback)
            p_transfer->complete_callback(p_transfer->complete_para);
        p_transfer->dma_controller
            ->dma_channel[p_transfer->channel_number]
            .channel_status = CHANNEL_STATUS_IDLE;
        put_desc(my_own, p_transfer);
        rt_list_remove(&p_transfer->transfer_list);
}

static void rt_fh_isr_cyclic_process(struct fh_dma *my_own, struct dma_transfer *p_transfer){
    int index;
    struct dw_axi_dma *temp_dwc;
    temp_dwc = &my_own->dwc;
    struct axi_dma_lli *p_lli = RT_NULL;

    if (p_transfer->complete_callback)
        p_transfer->complete_callback(p_transfer->complete_para);
    p_lli = p_transfer->first_lli;
    //invaild desc mem to cache...
    mmu_clean_invalidated_dcache((uint32_t)p_lli,
    sizeof(struct axi_dma_lli) * p_transfer->cyclic_periods);
    for(index = 0; index < p_transfer->cyclic_periods; index++){
        if(!(p_lli[index].ctl_hi & CH_CTL_H_LLI_VALID)){
			p_lli[index].ctl_hi |= CH_CTL_H_LLI_VALID;
		}
    }
    //flush cache..
    mmu_clean_invalidated_dcache((uint32_t)p_lli,
    sizeof(struct axi_dma_lli) * p_transfer->cyclic_periods);
    //kick dma again.
    dw_writel(temp_dwc, CHAN[p_transfer->channel_number].BLK_TFR_RESU, 1);
}

static void rt_fh_dma_isr(int irq, void *param)
{
    rt_uint32_t status, i;
    struct dw_axi_dma *temp_dwc;
    struct fh_dma *my_own = (struct fh_dma *)param;

    temp_dwc = &my_own->dwc;
    axi_dma_isr_common_disable(temp_dwc);
    for (i = 0; i < my_own->dwc.channel_max_number; i++)
    {
        status = dw_readl(temp_dwc, CHAN[i].INTSTATUS_LO);
        dw_writel(temp_dwc, CHAN[i].INTCLEAR_LO, status);
        if(status & DWAXIDMAC_IRQ_ALL_ERR) {
            rt_kprintf("bug status is %x\n",status);
            FH_DMA_ASSERT(0);
        }

        if (my_own->dma_channel[i].open_flag == SINGLE_TRANSFER) {
            if(status & DWAXIDMAC_IRQ_DMA_TRF)
                rt_fh_isr_single_process(my_own,
                my_own->dma_channel[i].active_trans);
        }

        if (my_own->dma_channel[i].open_flag == CYCLIC_TRANSFER) {
            if(status & DWAXIDMAC_IRQ_BLOCK_TRF){
                rt_fh_isr_cyclic_process(my_own,
                my_own->dma_channel[i].active_trans);
			}
        }
    }
    axi_dma_isr_common_enable(temp_dwc);
}

const char *channel_lock_name[DW_DMA_MAX_NR_CHANNELS] = {
    "channel_0_lock", "channel_1_lock", "channel_2_lock", "channel_3_lock",
    "channel_4_lock", "channel_5_lock", "channel_6_lock", "channel_7_lock",
};

static void rt_fh_axi_dma_cyclic_pause(struct dma_transfer *p)
{
    struct fh_dma *my_own = p->dma_controller;
    fhc_chan_susp_set(my_own, p, 1);
}

static void rt_fh_axi_dma_cyclic_resume(struct dma_transfer *p)
{

    struct fh_dma *my_own = p->dma_controller;
    fhc_chan_susp_set(my_own, p, 0);
}

static void rt_fh_axi_dma_cyclic_stop(struct dma_transfer *p)
{
   struct fh_dma *my_own = p->dma_controller;

   fhc_chan_susp_set(my_own, p, 1);

   fhc_chan_able_set(my_own, p, 0);

}

static void rt_fh_axi_dma_cyclic_start(struct dma_transfer *p)
{

    struct fh_dma *my_own = p->dma_controller;
    struct axi_dma_lli *p_lli = RT_NULL;

    p_lli = p->first_lli;

    /* warnning!!!!must check if the add is 64Byte ally... */
    RT_ASSERT(((rt_uint32_t)p_lli & AXI_DESC_ALLIGN_BIT_MASK) == 0);
    fhc_chan_able_set(my_own, p, 1);

}

static void rt_fh_axi_dma_cyclic_prep(struct fh_dma *fh_dma_p,
                                  struct dma_transfer *p)
{
    /* bind the controller to the transfer */
    p->dma_controller = fh_dma_p;
    /* bind active transfer */
    fh_dma_p->dma_channel[p->channel_number].active_trans = p;
    struct fh_dma *my_own = p->dma_controller;
    struct dw_axi_dma *temp_dwc;
    rt_uint32_t src_inc_mode;
	rt_uint32_t dst_inc_mode;
    rt_uint32_t cfg_low;
    rt_uint32_t cfg_high;
    temp_dwc = &my_own->dwc;
    volatile uint32_t ret_status;
    struct axi_dma_lli *p_lli = RT_NULL;
    uint32_t periods, i;
    uint32_t temp_src_add = 0;
    uint32_t temp_dst_add = 0;
    uint32_t buf_len    = p->trans_len;
    uint32_t period_len = p->period_len;
    rt_uint32_t isr = DWAXIDMAC_IRQ_DMA_TRF | DWAXIDMAC_IRQ_BLOCK_TRF | DWAXIDMAC_IRQ_SUSPENDED;
    rt_uint32_t isr_sig = DWAXIDMAC_IRQ_DMA_TRF | DWAXIDMAC_IRQ_BLOCK_TRF;
    struct dma_transfer *dma_trans_desc = p;
    /* check first... */
    RT_ASSERT(buf_len % period_len == 0);

    /* cal the periods... */
    periods = buf_len / period_len;

    /* get desc.... */
    /* dma_trans_desc->first_lli = (struct axi_dma_lli *)rt_malloc(periods * */
    /* sizeof(struct axi_dma_lli)); */
    dma_trans_desc->first_lli = get_desc(fh_dma_p, dma_trans_desc, periods);

    if (dma_trans_desc->first_lli == RT_NULL)
    {
        FH_DMA_DEBUG("transfer error,reason: not enough mem..\n");
        RT_ASSERT(dma_trans_desc->first_lli != RT_NULL);
    }

    rt_memset((void *)dma_trans_desc->first_lli, 0,
              periods * sizeof(struct axi_dma_lli));
    p_lli = dma_trans_desc->first_lli;

    RT_ASSERT(((uint32_t)p_lli & AXI_DESC_ALLIGN_BIT_MASK) == 0);

    RT_ASSERT(dma_trans_desc->dst_inc_mode <= DW_DMA_SLAVE_FIX);
    RT_ASSERT(dma_trans_desc->src_inc_mode <= DW_DMA_SLAVE_FIX);
    dma_trans_desc->cyclic_periods = periods;
    /* step3: set the mem.. */
    for (i = 0; i < periods; i++)
    {
        /* parse trans para... */
        /* para add: */
        switch (dma_trans_desc->dst_inc_mode)
        {
        case DW_DMA_SLAVE_INC:
            temp_dst_add = dma_trans_desc->dst_add +
                           i * period_len * (1 << dma_trans_desc->dst_width);
            break;
        case DW_DMA_SLAVE_DEC:
            temp_dst_add = dma_trans_desc->dst_add -
                           i * period_len * (1 << dma_trans_desc->dst_width);
            break;
        case DW_DMA_SLAVE_FIX:
            temp_dst_add = dma_trans_desc->dst_add;
            break;
        }

        switch (dma_trans_desc->src_inc_mode)
        {
        case DW_DMA_SLAVE_INC:
            temp_src_add = dma_trans_desc->src_add +
                           i * period_len * (1 << dma_trans_desc->src_width);
            break;
        case DW_DMA_SLAVE_DEC:
            temp_src_add = dma_trans_desc->src_add -
                           i * period_len * (1 << dma_trans_desc->src_width);
            break;
        case DW_DMA_SLAVE_FIX:
            temp_src_add = dma_trans_desc->src_add;
            break;
        }

        p_lli[i].sar_lo = temp_src_add;
        p_lli[i].dar_lo = temp_dst_add;

        /* para ctl */

        RT_ASSERT(dma_trans_desc->dst_width <= DW_DMA_SLAVE_WIDTH_32BIT);
        RT_ASSERT(dma_trans_desc->src_width <= DW_DMA_SLAVE_WIDTH_32BIT);

        RT_ASSERT(dma_trans_desc->dst_msize <= DW_DMA_SLAVE_MSIZE_256);
        RT_ASSERT(dma_trans_desc->src_msize <= DW_DMA_SLAVE_MSIZE_256);
        RT_ASSERT(dma_trans_desc->fc_mode <= DMA_P2P);

        dst_inc_mode = (dma_trans_desc->dst_inc_mode == DW_DMA_SLAVE_INC) ? 0 : 1;
		src_inc_mode = (dma_trans_desc->src_inc_mode == DW_DMA_SLAVE_INC) ? 0 : 1;

        p_lli[i].ctl_lo =
        AXI_DMA_CTLL_DST_WIDTH(dma_trans_desc->dst_width) |
        AXI_DMA_CTLL_SRC_WIDTH(dma_trans_desc->src_width) |
        AXI_DMA_CTLL_DST_MSIZE(dma_trans_desc->dst_msize) |
        AXI_DMA_CTLL_SRC_MSIZE(dma_trans_desc->src_msize) |
        AXI_DMA_CTLL_DST_INC_MODE(dst_inc_mode) |
        AXI_DMA_CTLL_SRC_INC_MODE(src_inc_mode);


        p_lli[i].ctl_hi = CH_CTL_H_LLI_VALID | CH_CTL_H_IOC_BLKTFR;
        // p_lli[i].ctl_hi |= CH_CTL_H_ARLEN_EN(1) | CH_CTL_H_ARLEN(7) |
        // CH_CTL_H_AWLEN_EN(1) | CH_CTL_H_AWLEN(7);
        axi_dma_ctl_init(fh_dma_p, &p_lli[i].ctl_lo, &p_lli[i].ctl_hi, dma_trans_desc);
        /* block size */
        p_lli[i].block_ts_lo = period_len - 1;
        p_lli[i].llp_lo = (uint32_t)&p_lli[i + 1];
        /* flush cache to mem */
        mmu_clean_invalidated_dcache((uint32_t)&p_lli[i],
                                     sizeof(struct axi_dma_lli));
    }
    /* make a ring here */
    p_lli[periods - 1].llp_lo = (uint32_t)&p_lli[0];

    mmu_clean_invalidated_dcache((uint32_t)&p_lli[periods - 1],
                                 sizeof(struct axi_dma_lli));
    /* parse the handshake */
    RT_ASSERT(dma_trans_desc->dst_hs <= DMA_SW_HANDSHAKING);
    RT_ASSERT(dma_trans_desc->src_hs <= DMA_SW_HANDSHAKING);

    /* dst handshake */

    /* set link enable */
    dw_writel(temp_dwc, CHAN[dma_trans_desc->channel_number].CFG_LO,
    3 << CH_CFG_L_DST_MULTBLK_TYPE_POS | 3 << CH_CFG_L_SRC_MULTBLK_TYPE_POS);
    /* set flow mode */
    dw_writel(temp_dwc, CHAN[dma_trans_desc->channel_number].CFG_HI,
    AXI_DMA_CFGH_FC(dma_trans_desc->fc_mode));
    /* set base link add */
    dw_writel(temp_dwc, CHAN[dma_trans_desc->channel_number].LLP,  (int)&p_lli[0]);
    /* clear isr */
    dw_writel(temp_dwc, CHAN[dma_trans_desc->channel_number].INTCLEAR_LO, 0xffffffff);
    dw_writel(temp_dwc, CHAN[dma_trans_desc->channel_number].INTCLEAR_HI, 0xffffffff);
    /* open isr */
    dw_writel(temp_dwc, CHAN[dma_trans_desc->channel_number].INTSTATUS_EN_LO, isr);
    dw_writel(temp_dwc, CHAN[dma_trans_desc->channel_number].INTSIGNAL_LO, isr_sig);

    RT_ASSERT(dma_trans_desc->dst_hs <= DMA_SW_HANDSHAKING);
    RT_ASSERT(dma_trans_desc->src_hs <= DMA_SW_HANDSHAKING);

    /* only hw handshaking need this.. */
    switch (dma_trans_desc->fc_mode)
    {
    case DMA_M2M:

        break;
    case DMA_M2P:
        ret_status = dw_readl(temp_dwc, CHAN[dma_trans_desc->channel_number].CFG_HI);
        ret_status &= ~0x18;
        ret_status |= dma_trans_desc->dst_per << 12;
        dw_writel(temp_dwc, CHAN[dma_trans_desc->channel_number].CFG_HI, ret_status);

        break;
    case DMA_P2M:
        ret_status = dw_readl(temp_dwc, CHAN[dma_trans_desc->channel_number].CFG_HI);
        ret_status &= ~0x18;
        ret_status |= dma_trans_desc->src_per << 7;
        dw_writel(temp_dwc, CHAN[dma_trans_desc->channel_number].CFG_HI, ret_status);

        break;

    default:
        break;
    }


    /*rewrite cfg..*/
    cfg_low = dw_readl(temp_dwc, CHAN[dma_trans_desc->channel_number].CFG_LO);
    cfg_high = dw_readl(temp_dwc, CHAN[dma_trans_desc->channel_number].CFG_HI);
    axi_dma_cfg_init(fh_dma_p, &cfg_low, &cfg_high, dma_trans_desc);
    dw_writel(temp_dwc, CHAN[dma_trans_desc->channel_number].CFG_LO, cfg_low);
    dw_writel(temp_dwc, CHAN[dma_trans_desc->channel_number].CFG_HI, cfg_high);


    dma_trans_desc->dma_controller->dma_channel[dma_trans_desc->channel_number]
        .channel_status = CHANNEL_STATUS_BUSY;

    if (dma_trans_desc->prepare_callback)
    {
        dma_trans_desc->prepare_callback(dma_trans_desc->prepare_para);
    }

}

static void rt_fh_axi_dma_cyclic_free(struct dma_transfer *p)
{
    struct fh_dma *my_own = p->dma_controller;

    put_desc(my_own, p);
}


static int cal_axi_dma_channel(struct dw_axi_dma *axi_dma_obj)
{
#ifdef DMA_FIXED_CHANNEL_NUM
    return DMA_FIXED_CHANNEL_NUM;
#else
	int ret;
	int i;

    axi_dma_reset(axi_dma_obj);
    axi_dma_enable(axi_dma_obj);
    for (i = 0; i < DW_DMA_MAX_NR_CHANNELS; i++) {
		dw_writel(axi_dma_obj, CHEN_LO, 1 << i | 1 << (8 + i));
		ret = dw_readl(axi_dma_obj, CHEN_LO);
		if (ret < 1 << i)
			break;
	}
    axi_dma_reset(axi_dma_obj);
	return i;
#endif
}

static int fh_axi_dma_probe(void *priv_data)
{
    rt_uint32_t i;
    struct fh_dma *fh_dma_p;
    struct dma_platform_data *plat = (struct dma_platform_data *)priv_data;
    struct rt_dma_device *rt_dma;
    char dma_dev_name[8]       = {0};

    fh_dma_p = (struct fh_dma *)rt_malloc(sizeof(struct fh_dma));

    if (!fh_dma_p)
    {
        rt_kprintf("ERROR: %s, malloc failed\n", __func__);
        return -RT_EFULL;
    }

    rt_memset(fh_dma_p, 0, sizeof(struct fh_dma));

    rt_sprintf(dma_dev_name, "%s%d", plat->name, plat->id);

    rt_dma      = &fh_dma_p->parent;
    rt_dma->ops = &fh_dma_ops;

    /* soc para set */
    fh_dma_p->dwc.name               = dma_dev_name;
    fh_dma_p->dwc.regs               = (void *)plat->base;
    fh_dma_p->dwc.paddr              = plat->base;
    fh_dma_p->dwc.irq                = plat->irq;
    fh_dma_p->dwc.channel_max_number = cal_axi_dma_channel(&fh_dma_p->dwc);
    //fh_dma_p->dwc.channel_max_number = plat->channel_max_number;
    fh_dma_p->dwc.controller_status  = CONTROLLER_STATUS_CLOSED;
    fh_dma_p->dwc.id                 = plat->id;

    rt_memcpy(&fh_dma_p->dri_spdup_info, &plat->spdup,
    sizeof(struct s_spdup_info));

#ifndef FH_FAST_BOOT
    rt_kprintf("[axi_dma : %d] has channel %d\n", fh_dma_p->dwc.id, fh_dma_p->dwc.channel_max_number);
#endif
    if (plat->dma_init)
        plat->dma_init();

    /* channel set */
    for (i = 0; i < plat->channel_max_number; i++)
    {
        fh_dma_p->dma_channel[i].channel_status = CHANNEL_STATUS_CLOSED;
        fh_dma_p->dma_channel[i].desc_total_no  = DESC_MAX_SIZE;

        rt_list_init(&(fh_dma_p->dma_channel[i].queue));
        fh_dma_p->dma_channel[i].desc_trans_size =
            FH_CHANNEL_MAX_TRANSFER_SIZE;
        rt_sem_init(&fh_dma_p->dma_channel[i].channel_lock,
                    channel_lock_name[i], 1, RT_IPC_FLAG_FIFO);
    }

    /* isr */
    rt_hw_interrupt_install(fh_dma_p->dwc.irq, rt_fh_dma_isr,
                            (void *)fh_dma_p, "dma_isr");
    rt_hw_interrupt_umask(fh_dma_p->dwc.irq);

    return rt_hw_dma_register(rt_dma, dma_dev_name, RT_DEVICE_FLAG_RDWR,
                              fh_dma_p);
}


static int fh_axi_dma_exit(void *priv_data)
{
    return 0;
}

struct fh_board_ops dma_driver_ops = {
    .probe = fh_axi_dma_probe, .exit = fh_axi_dma_exit,
};

int fh_dma_init(void)
{
    return fh_board_driver_register("fh_dma", &dma_driver_ops);
}
