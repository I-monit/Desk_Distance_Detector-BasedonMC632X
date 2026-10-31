/**
 * Copyright (c) 2015-2019 Shanghai Fullhan Microelectronics Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 *
 * Change Logs:
 * Date           Author       Notes
 * 2024-2-28      zhangy       add first version driver
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

#include "mol_dma.h"
#include "delay.h"
#ifdef RT_USING_PM
#include <pm.h>
#endif


#ifndef BIT
#define BIT(x) (1 << (x))
#endif


#if defined(MC_DMA_DEBUG) && defined(RT_DEBUG)
#define FH_MC_DMA_PRINT(fmt, args...) rt_kprintf(fmt, ##args);
#else
#define FH_MC_DMA_PRINT(fmt, args...)
#endif

#define MC_DMA_ASSERT(expr) if (!(expr)) { \
        rt_kprintf("Assertion failed! %s:line %d\n", \
        __func__, __LINE__); \
        while (1)   \
           ;       \
        }

#define MC_DMA_NULL			(0)
#define MC_DESC_ALLIGN                64  /*DO NOT TOUCH!!!*/
#define MC_DESC_ALLIGN_BIT_MASK     0x3f  /*DO NOT TOUCH!!!*/
#define lift_shift_bit_num(bit_num) (1 << bit_num)
#define MC_DMA_MAX_NR_CHANNELS 32
#define MC_DMA_MAX_HAND_SHAKE_NO 64



#ifndef GENMASK
#define GENMASK(h, l) (((~0UL) << (l)) & (~0UL >> (sizeof(long) * 8 - 1 - (h))))
#endif
#ifndef GENMASK_ULL
#define GENMASK_ULL(h, l)                                                      \
	(((~0ULL) << (l)) & (~0ULL >> (sizeof(long long) * 8 - 1 - (h))))
#endif


#define __dma_raw_writeb(v, a) (*(volatile rt_uint8_t *)(a) = (v))
#define __dma_raw_writew(v, a) (*(volatile rt_uint16_t *)(a) = (v))
#define __dma_raw_writel(v, a) (*(volatile rt_uint32_t *)(a) = (v))

#define __dma_raw_readb(a) (*(volatile rt_uint8_t *)(a))
#define __dma_raw_readw(a) (*(volatile rt_uint16_t *)(a))
#define __dma_raw_readl(a) (*(volatile rt_uint32_t *)(a))

#define dw_readl(dw, name) \
    __dma_raw_readl(&(((struct dw_mc_dma_regs *)dw->regs)->name))
#define dw_writel(dw, name, val) \
    __dma_raw_writel((val), &(((struct dw_mc_dma_regs *)dw->regs)->name))
#define dw_readw(dw, name) \
    __dma_raw_readw(&(((struct dw_mc_dma_regs *)dw->regs)->name))
#define dw_writew(dw, name, val) \
    __dma_raw_writew((val), &(((struct dw_mc_dma_regs *)dw->regs)->name))

#define CONTROLLER_STATUS_CLOSED (0)
#define CONTROLLER_STATUS_OPEN (1)

#define CHANNEL_STATUS_CLOSED (0)
#define CHANNEL_STATUS_OPEN (1)
#define CHANNEL_STATUS_IDLE (2)
#define CHANNEL_STATUS_BUSY (3)
#define SINGLE_TRANSFER (0)
#define CYCLIC_TRANSFER (1)
#define DEFAULT_TRANSFER SINGLE_TRANSFER
#define CHANNEL_REAL_FREE (0)
#define CHANNEL_NOT_FREE (1)



#define DMA_PAUSE		BIT(0)
#define DMA_PAUSE_STAT		BIT(2)
#define GLB_REG_CLK_EN		BIT(16)
#define CHNL_REG_CLK_EN		BIT(17)
#define REQ_CID_CLK_EN		BIT(18)
#define INT_REG_CLK_EN		BIT(19)
#define AXI_MST_CLK_EN		BIT(20)
#define AUDIO_CNT_CLK_EN		BIT(21)

#define BLK_LEN_DUMMY					10
#define TRSC_LEN_DUMMY					10
#define FRAG_LEN_DUMMY					10

//channel def
#define CHAN_PAUSE	1
#define CHAN_ACTIVE	0
#define CHAN_PAUSE_BIT_MASK	1
#define LLIST_EN					(1<<4)

#define CH_ENABLE					1

#define CH_FRAGMENT_INT_EN				(0x1<<0)
#define CH_BLK_INT_EN					(0x1<<1)
#define CH_TRSC_INT_EN					(0x1<<2)
#define CH_LLIST_INT_EN					(0x1<<3)
#define CH_CFG_ERR_INT_EN				(0x1<<4)
#define CH_AUTO_CLOSE_EN			(0x1<<5)

#define CH_RAW_ISR_STATUS_MASK		(0x1f << 8)


#define CH_ALLINTRC_EN_MASK				CH_FRAGMENT_INT_EN|\
							CH_BLK_INT_EN|\
							CH_TRSC_INT_EN|\
							CH_LLIST_INT_EN|\
							CH_CFG_ERR_INT_EN
							

//desc def
#define BLK_LEN_MASK					0x1ffff
#define FRAG_LEN_MASK					0x1ffff
#define TRSC_LEN_MASK					0xfffffff

#define REQ_MODE_SHIFT					24
#define REQ_MODE_MASK					0x3

#define SRC_DST_AHB_SIZE_MASK			0xf
#define DST_AHB_SIZE_SHIFT				28
#define SRC_AHB_SIZE_SHIFT				30
#define ADDR_FIX_EN_SHIFT				20
#define ADDR_FIX_SEL_SHIFT				21
#define ADDR_WRAP_EN_SHIFT				22

#define WORD_STEP					0x4
#define DST_TRSF_STEP_SHIFT				16
#define TRSF_STEP_MASK					0xffffffff
#define LLIST_END						1
#define NOT_LLIST_END					0
#define LLIST_END_SHIFT					19

#define SWT_MODE_SHIFT              26
#define SWT_MODE_MASK               3

#define CHN_TRSC_LEN_LLIST_NODE_INT_EN			(0x1<<31)
#define CHN_TRSC_LEN_LLIST_NODE_INT_FRAG		(0x1<<28)
#define CHN_TRSC_LEN_LLIST_NODE_INT_BLK			(0x1<<29)
#define CHN_TRSC_LEN_LLIST_NODE_INT_TRSC		(0x1<<30)


#define ISR_LINK_DONE_RAW_STAT_BIT		(0x1<<11)
#define ISR_TRSC_DONE_RAW_STAT_BIT		(0x1<<10)
#define ISR_BLK_DONE_RAW_STAT_BIT		(0x1<<9)
#define ISR_FRAG_DONE_RAW_STAT_BIT		(0x1<<8)
#define VERSION_FH_MASK	(0xffff0000)
#define VERSION_NUM_MASK	(0x0000ffff)
#define VERSION_FH	(0x46480000)
#define mc_dma_list_entry(node, type, member) \
    ((type *)((char *)(node) - (unsigned long)(&((type *)0)->member)))

#define mc_dma_list_for_each_entry_safe(pos, n, head, member)                \
    for (pos = mc_dma_list_entry((head)->next, typeof(*pos), member),     \
        n    = mc_dma_list_entry(pos->member.next, typeof(*pos), member); \
         &pos->member != (head);                                      \
         pos = n, n = mc_dma_list_entry(n->member.next, typeof(*n), member))


#define DMA_COMMON_ISR_OPEN     1
#define DMA_COMMON_ISR_CLOSE    0

#define USR_DEF_MODE	0x55aaaa55
#define DMA_MEMCPY_FIX_LEN    0x80
extern void mmu_clean_invalidated_dcache(rt_uint32_t buffer, rt_uint32_t size);
#define mc_dma_invalidate_desc    mmu_clean_invalidated_dcache
#define mc_dma_clean_desc    mmu_clean_invalidated_dcache

#define PM_DMA_CHAN_WAIT_MAX_TIME    0xffff
/****************************************************************************
 * ADT section
 *  add definition of user defined Data Type that only be used in this file  here
 ***************************************************************************/
enum {
	MC_DMA_RET_OK    = 0,
	MC_DMA_RET_NO_MEM   = 1,
};

struct  mc_dma_lli {

	rt_uint32_t src_addr;
	rt_uint32_t dst_addr;
	rt_uint32_t frag_len;
	rt_uint32_t blk_len;
	rt_uint32_t trans_len;
	rt_uint32_t trsf_step;
	rt_uint32_t wrap_ptr;
	rt_uint32_t wrap_to;
	rt_uint32_t llist_ptr;
	rt_uint32_t frag_step;
	rt_uint32_t src_blk_step;
	rt_uint32_t dst_blk_step;
};

/* Hardware register definitions. */
struct dw_mc_dma_chan_regs {
	rt_uint32_t pause;//0
	rt_uint32_t req;
	rt_uint32_t cfg;
	rt_uint32_t interrupt;
	rt_uint32_t src_addr;//10
	rt_uint32_t dst_addr;
	rt_uint32_t frag_len;
	rt_uint32_t blk_len;
	rt_uint32_t trsc_len;//20
	rt_uint32_t trsf_step;
	rt_uint32_t wrap_ptr;
	rt_uint32_t wrap_to;
	rt_uint32_t llist_ptr;//30
	rt_uint32_t frag_step;
	rt_uint32_t src_blk_step;
	rt_uint32_t dst_blk_step;
};

struct dw_mc_dma_regs {
	rt_uint32_t pause;//0
	rt_uint32_t frag_wait;
	rt_uint32_t pend_0_en;
	rt_uint32_t pend_1_en;
	rt_uint32_t int_raw_stat;//10
	rt_uint32_t int_mask_stat;
	rt_uint32_t req_stat;
	rt_uint32_t en_stat;
	rt_uint32_t debug_stat;//20
	rt_uint32_t arb_sel_stat;
	rt_uint32_t ch_cfg_grp_1;
	rt_uint32_t ch_cfg_grp_2;
	rt_uint32_t ch_arprot;//30
	rt_uint32_t ch_awprot;
	rt_uint32_t ch_prot_flag;
	rt_uint32_t global_prot;
	rt_uint32_t pend_0_port;//40
	rt_uint32_t pend_1_port;
	rt_uint32_t reqid_0_port;
	rt_uint32_t reqid_1_port;
	rt_uint32_t sync_sec_n_normal;//50
	rt_uint32_t cnt_ch_sel;
	rt_uint32_t total_trans_cnt_1;
	rt_uint32_t total_trans_cnt_2;
	rt_uint32_t rev_0[4];//0x60 ~ 0x6c
	rt_uint32_t channel_num;
	rt_uint32_t rev_1;
	rt_uint32_t version;
	rt_uint32_t feature;//0x7c
	rt_uint32_t rev_2[992];
	struct dw_mc_dma_chan_regs ch[MC_DMA_MAX_NR_CHANNELS];
	rt_uint32_t rev_3[0x200];
	rt_uint32_t hand_cid[MC_DMA_MAX_HAND_SHAKE_NO];
};

struct dw_mc_dma {
	/* vadd */
	void *regs;
	rt_uint32_t channel_max_number;
	rt_uint32_t controller_status;
	rt_uint32_t id;
    rt_uint32_t irq;
	char name[20];
};

struct dma_channel {
	rt_uint32_t channel_status;  /* open, busy ,closed */
	rt_uint32_t desc_trans_size;
	mc_dma_lock_t channel_lock;
	rt_list_t queue;
	struct dma_transfer *active_trans;
	rt_uint32_t open_flag;
	rt_uint32_t desc_total_no;
	rt_uint32_t free_index;
	rt_uint32_t used_index;
	rt_uint32_t desc_left_cnt;
	/*malloc maybe not allign; driver will malloc (size + cache line) incase*/
	rt_uint32_t allign_malloc;
	unsigned int allign_phy;
	struct mc_dma_lli *base_lli;
	rt_uint32_t base_lli_phy;
	rt_uint32_t isr_enable_mode;
	rt_uint32_t isr_status;
};

struct mc_dma_ops {
	void (*mc_dma_isr_process)(struct fh_dma *param);
	void (*mc_dma_isr_enable_set)(struct fh_dma *param,
		rt_uint32_t enable);
	rt_int32_t(*mc_dma_control)(struct fh_dma *dma,
		rt_uint32_t cmd, struct dma_transfer *arg);
	void (*mc_dma_get_desc_para)(struct fh_dma *dma,
		struct dma_transfer *p_transfer,
		rt_uint32_t *desc_size, rt_uint32_t *base_phy_add,
		rt_uint32_t *each_desc_size, rt_uint32_t *each_desc_cap);
};

struct fh_dma {
	/* myown */
        /* core use ,this must be the first para!!!! */
    struct rt_dma_device parent;
	void *kernel_pri;// used for malloc........
	void *adapt_pri;// use for call adapt driver
	struct mc_dma_ops ops;
	struct dw_mc_dma dwc;
	/* channel obj */
	mc_dma_lock_t dma_lock;
#define MOL_DMA_STAT_NORMAL    0
#define MOL_DMA_STAT_SUSPNED    0x55aaaa55
    rt_uint32_t pm_stat;
	struct dma_channel dma_channel[MC_DMA_MAX_NR_CHANNELS];

};


enum mc_dma_req_mode{
	FRAGMENT,
	BLOCK,
	TRANSACTION,
	LINKLIST
};

/******************************************************************************
 * Function prototype section
 * add prototypes for all functions called by this file,execepting those
 * declared in header file
 *****************************************************************************/

/*****************************************************************************
 * Global variables section - Exported
 * add declaration of global variables that will be exported here
 * e.g.
 *  int8_t foo;
 ****************************************************************************/


/*****************************************************************************
 *  static fun;
 *****************************************************************************/
static void fh_mc_dma_cyclic_stop(struct dma_transfer *p);
static void fh_mc_dma_cyclic_start(struct dma_transfer *p);
static void fh_mc_dma_cyclic_free(struct dma_transfer *p);
static void fh_mc_dma_cyclic_pause(struct dma_transfer *p);
static void fh_mc_dma_cyclic_resume(struct dma_transfer *p);
static rt_uint32_t vir_lli_to_phy_lli(struct mc_dma_lli *base_lli,
rt_uint32_t base_lli_phy, struct mc_dma_lli *cal_lli);
static rt_uint32_t mol_dma_pm_stat_get(struct fh_dma *p_dma);
static void mol_dma_pm_stat_set(struct fh_dma *p_dma, rt_uint32_t pm_stat);
/*****************************************************************************
 * Global variables section - Local
 * define global variables(will be refered only in this file) here,
 * static keyword should be used to limit scope of local variable to this file
 * e.g.
 *  static uint8_t ufoo;
 *****************************************************************************/
static char *debug_channel_state_str[] = {
    "CLOSED",
    "OPEN",
    "IDLE",
    "BUSY",
};

/* function body */
/*****************************************************************************
 * Description:
 *      add funtion description here
 * Parameters:
 *      description for each argument, new argument starts at new line
 * Return:
 *      what does this function returned?
 *****************************************************************************/



#ifdef MC_DMA_DEBUG
/*
bit[21 : 20]
00(0) : src inc & dst inc
01(1) : src fix & dst inc
10(2) : src inc & dst inc
11(3) : src inc & dst fix 
*/
static char *debug_str_addr_src_inc_mode[] = {
	"INC",
	"FIX",
	"INC",
	"INC",
};
static char *debug_str_addr_dst_inc_mode[] = {
	"INC",
	"INC",
	"INC",
	"FIX",
};

static char *debug_str_addr_width[] = {
	"1B",
	"2B",
	"4B",
	"8B",
};


static int debug_addr_width[] = {
	1,
	2,
	4,
	8,
};


static void dump_lli_str(struct mc_dma_lli *p_lli, struct mc_dma_lli *base_lli, rt_uint32_t base_lli_phy)
{

	rt_kprintf("[0x%08x & %s] [%s]*[%d data] -> [0x%08x & %s] [%s]*[%d data] = [0x%x]B\n",
	(p_lli->src_addr), debug_str_addr_src_inc_mode[(p_lli->frag_len >> 20) & 3],
	debug_str_addr_width[(p_lli->frag_len >> 30) & 3],
	((p_lli->frag_len & 0x0001ffff) / debug_addr_width[(p_lli->frag_len >> 30) & 3]),
	(p_lli->dst_addr), debug_str_addr_dst_inc_mode[(p_lli->frag_len >> 20) & 3],
	debug_str_addr_width[(p_lli->frag_len >> 28) & 3],
	((p_lli->frag_len & 0x0001ffff) / debug_addr_width[(p_lli->frag_len >> 28) & 3]),
	(p_lli->trans_len));

}

static void dump_lli(struct mc_dma_lli *p_lli, struct mc_dma_lli *base_lli, rt_uint32_t base_lli_phy)
{

	rt_uint32_t phy_add;

	phy_add = vir_lli_to_phy_lli(base_lli, base_lli_phy, p_lli);

	rt_kprintf("lli add is [%08x]\n",(int)phy_add);
	rt_kprintf("\nSAR: 0x%08x DAR: 0x%08x FRAG_LEN: 0x%08x BLK_LEN: 0x%08x\n\
	TRANS_LEN: 0x%08x TRANS_STEP: 0x%08x WRAP_PTR: 0x%08x WRAP_TO: 0x%08x\n\
	LIST_PTR: 0x%08x FRAG_STEP: 0x%08x  SRC_BLK_STEP: 0x%08x  DST_BLK_STEP: 0x%08x\n",
	(p_lli->src_addr),
	(p_lli->dst_addr),
	(p_lli->frag_len),
	(p_lli->blk_len),
	(p_lli->trans_len),
	(p_lli->trsf_step),
	(p_lli->wrap_ptr),
	(p_lli->wrap_to),
	(p_lli->llist_ptr),
	(p_lli->frag_step),
	(p_lli->src_blk_step),
	(p_lli->dst_blk_step));
}


static void dump_channel_reg(struct fh_dma *p_dma, struct dma_transfer *p_transfer)
{

	struct dw_mc_dma *temp_dwc;

	rt_uint32_t chan_no = p_transfer->channel_number;

	temp_dwc = &p_dma->dwc;

	rt_kprintf("[CHAN : %d]\n\
	pause: 0x%08x\n\
	req: 0x%08x\n\
	cfg: 0x%08x\n\
	interrupt: %08x\n\
	src_addr: %08x\n\
	dst_addr :0x%08x\n\
	frag_len: 0x%08x\n\
	blk_len: 0x%08x\n\
	trsc_len: 0x%08x\n\
	trsf_step: 0x%08x\n\
	wrap_ptr: 0x%08x\n\
	wrap_to: 0x%08x\n\
	llist_ptr: 0x%08x\n\
	frag_step: 0x%08x\n\
	src_blk_step: 0x%08x\n\
	dst_blk_step: 0x%08x\n",
	chan_no,
	dw_readl(temp_dwc, ch[chan_no].pause),
	dw_readl(temp_dwc, ch[chan_no].req),
	dw_readl(temp_dwc, ch[chan_no].cfg),
	dw_readl(temp_dwc, ch[chan_no].interrupt),
	dw_readl(temp_dwc, ch[chan_no].src_addr),
	dw_readl(temp_dwc, ch[chan_no].dst_addr),
	dw_readl(temp_dwc, ch[chan_no].frag_len),
	dw_readl(temp_dwc, ch[chan_no].blk_len),
	dw_readl(temp_dwc, ch[chan_no].trsc_len),
	dw_readl(temp_dwc, ch[chan_no].trsf_step),
	dw_readl(temp_dwc, ch[chan_no].wrap_ptr),
	dw_readl(temp_dwc, ch[chan_no].wrap_to),
	dw_readl(temp_dwc, ch[chan_no].llist_ptr),
	dw_readl(temp_dwc, ch[chan_no].frag_step),
	dw_readl(temp_dwc, ch[chan_no].src_blk_step),
	dw_readl(temp_dwc, ch[chan_no].dst_blk_step)
	);

}

static void dump_dma_common_reg(struct fh_dma *p_dma)
{

	struct dw_mc_dma *temp_dwc;

	temp_dwc = &p_dma->dwc;
	if (!temp_dwc->regs)
		return;

	rt_kprintf("pause: 0x%08x\n\
	frag_wait: 0x%08x\n\
	pend_0_en: 0x%08x\n\
	pend_1_en: 0x%08x\n\
	int_raw_stat: 0x%08x\n\
	int_mask_stat :0x%08x\n\
	req_stat: 0x%08x\n\
	en_stat: 0x%08x\n\
	debug_stat: 0x%08x\n\
	arb_sel_stat: 0x%08x\n\
	ch_cfg_grp_1: 0x%08x\n\
	ch_cfg_grp_2: 0x%08x\n\
	ch_arprot: 0x%08x\n\
	ch_awprot: 0x%08x\n\
	ch_prot_flag: 0x%08x\n\
	global_prot: 0x%08x\n\
	pend_0_port: 0x%08x\n\
	pend_1_port: 0x%08x\n\
	reqid_0_port: 0x%08x\n\
	reqid_1_port: 0x%08x\n\
	sync_sec_n_normal: 0x%08x\n\
	cnt_ch_sel: 0x%08x\n\
	total_trans_cnt_1: 0x%08x\n\
	total_trans_cnt_2: 0x%08x\n",
	dw_readl(temp_dwc, pause),
	dw_readl(temp_dwc, frag_wait),
	dw_readl(temp_dwc, pend_0_en),
	dw_readl(temp_dwc, pend_1_en),
	dw_readl(temp_dwc, int_raw_stat),
	dw_readl(temp_dwc, int_mask_stat),
	dw_readl(temp_dwc, req_stat),
	dw_readl(temp_dwc, en_stat),
	dw_readl(temp_dwc, debug_stat),
	dw_readl(temp_dwc, arb_sel_stat),
	dw_readl(temp_dwc, ch_cfg_grp_1),
	dw_readl(temp_dwc, ch_cfg_grp_2),
	dw_readl(temp_dwc, ch_arprot),
	dw_readl(temp_dwc, ch_awprot),
	dw_readl(temp_dwc, ch_prot_flag),
	dw_readl(temp_dwc, global_prot),
	dw_readl(temp_dwc, pend_0_port),
	dw_readl(temp_dwc, pend_1_port),
	dw_readl(temp_dwc, reqid_0_port),
	dw_readl(temp_dwc, reqid_1_port),
	dw_readl(temp_dwc, sync_sec_n_normal),
	dw_readl(temp_dwc, cnt_ch_sel),
	dw_readl(temp_dwc, total_trans_cnt_1),
	dw_readl(temp_dwc, total_trans_cnt_2)
	);

}


static void dump_chan_xfer_info(struct fh_dma *p_dma, struct dma_transfer *p_transfer)
{
	rt_uint32_t i;
	struct mc_dma_lli *p_lli;

	p_lli = (struct mc_dma_lli *)p_transfer->first_lli;
	dump_dma_common_reg(p_dma);
	dump_channel_reg(p_dma, p_transfer);

}
#endif

static rt_uint32_t allign_func(rt_uint32_t in_addr, rt_uint32_t allign_size, rt_uint32_t *phy_back_allign)
{
	*phy_back_allign = (*phy_back_allign + allign_size - 1) & (~(allign_size - 1));
	return (in_addr + allign_size - 1) & (~(allign_size - 1));
}

static rt_uint32_t vir_lli_to_phy_lli(struct mc_dma_lli *base_lli,
rt_uint32_t base_lli_phy, struct mc_dma_lli *cal_lli)
{
	rt_uint32_t ret;

	ret = base_lli_phy + ((rt_uint32_t)cal_lli - (rt_uint32_t)base_lli);
	return ret;
}

struct mc_dma_lli *get_desc(struct fh_dma *p_dma,
struct dma_transfer *p_transfer, rt_uint32_t lli_size)
{
	struct mc_dma_lli *ret_lli;
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
	if (totoal_free_desc < lli_size) {
		FH_MC_DMA_PRINT("not enough desc to get...\n");
		FH_MC_DMA_PRINT("get size is %d,left is %d\n", lli_size, totoal_free_desc);
		return MC_DMA_NULL;
	}

	if (lli_size > allign_left) {
		/* if allign desc not enough...just reset null.... */
		if ((totoal_free_desc - allign_left) < lli_size) {
			FH_MC_DMA_PRINT("not enough desc to get...\n");
			FH_MC_DMA_PRINT(
				"app need size is %d, totoal left is %d, allign left is %d\n",
				lli_size, totoal_free_desc, allign_left);
			FH_MC_DMA_PRINT("from head to get desc size is %d, actual get is %d\n",
			(totoal_free_desc - allign_left),
			(allign_left + lli_size));
			return MC_DMA_NULL;
		} else {
			actual_get_desc = allign_left + lli_size;
			free_index = 0;
		}
	}

	ret_lli = &p_dma->dma_channel[p_transfer->channel_number].base_lli[free_index];
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

	lli_size = p_transfer->actual_lli_size;
	p_dma->dma_channel[p_transfer->channel_number].used_index += lli_size;
	p_dma->dma_channel[p_transfer->channel_number].used_index %=
	p_dma->dma_channel[p_transfer->channel_number].desc_total_no;
	p_dma->dma_channel[p_transfer->channel_number].desc_left_cnt += lli_size;
	p_transfer->lli_size = 0;
	p_transfer->actual_lli_size = 0;
	return MC_DMA_RET_OK;
}

static void mc_dma_reset(struct dw_mc_dma *mc_dma_obj)
{

}

static void mc_dma_ch_pause_set(struct dw_mc_dma *mc_dma_obj, rt_uint32_t no, rt_uint32_t val){
	int ret;

	ret = dw_readl(mc_dma_obj, ch[no].pause);
	ret &= ~(CHAN_PAUSE_BIT_MASK);
	ret |= val;
	dw_writel(mc_dma_obj, ch[no].pause, ret);
}

static void mc_dma_enable(struct dw_mc_dma *mc_dma_obj)
{
	rt_uint32_t ret;

	ret = dw_readl(mc_dma_obj, pause);
	ret |= (GLB_REG_CLK_EN | CHNL_REG_CLK_EN | 
			REQ_CID_CLK_EN | INT_REG_CLK_EN | 
			AXI_MST_CLK_EN | AUDIO_CNT_CLK_EN);
	dw_writel(mc_dma_obj, pause, ret);
}

static void mc_dma_ch_enable(struct dw_mc_dma *mc_dma_obj, rt_uint32_t no){
	rt_uint32_t ret;

	ret = dw_readl(mc_dma_obj, ch[no].cfg);
	ret |= CH_ENABLE;
	dw_writel(mc_dma_obj, ch[no].cfg, ret);

}

#if (0)
static int mc_dma_ch_get_isr_status(struct dw_mc_dma *mc_dma_obj, rt_uint32_t no){
	rt_uint32_t ret;

	ret = dw_readl(mc_dma_obj, ch[no].interrupt);
	
	return ret;
}

static int mc_dma_ch_get_isr_enable(struct dw_mc_dma *mc_dma_obj, rt_uint32_t no){
	rt_uint32_t ret;

	ret = dw_readl(mc_dma_obj, ch[no].interrupt);
	ret &= CH_ALLINTRC_EN_MASK;	
	return ret;
}
#endif

static void mc_dma_ch_clear_isr_status(struct dw_mc_dma *mc_dma_obj, rt_uint32_t no){
	rt_uint32_t ret;

	ret = dw_readl(mc_dma_obj, ch[no].interrupt);
	ret |= (0x1f << 24);
	dw_writel(mc_dma_obj, ch[no].interrupt, ret);
}



static void mc_dma_ch_disable(struct dw_mc_dma *mc_dma_obj, rt_uint32_t no){
	rt_uint32_t ret;

	ret = dw_readl(mc_dma_obj, ch[no].cfg);
	ret &= ~CH_ENABLE;
	dw_writel(mc_dma_obj, ch[no].cfg, ret);
}

static void mc_dma_ch_isr_set(struct dw_mc_dma *mc_dma_obj, rt_uint32_t no, rt_uint32_t enable)
{
	rt_uint32_t ret;
	ret = dw_readl(mc_dma_obj, ch[no].interrupt);
	ret &= ~(CH_ALLINTRC_EN_MASK);
	ret |= enable;

	dw_writel(mc_dma_obj, ch[no].interrupt, ret);
}


void mc_dma_ch_llist_en(struct dw_mc_dma *mc_dma_obj, rt_uint32_t no){

	rt_uint32_t ret;
	ret = dw_readl(mc_dma_obj, ch[no].cfg);
	ret |= LLIST_EN;
	dw_writel(mc_dma_obj, ch[no].cfg, ret);
}


void mc_dma_ch_blk_len_set(struct dw_mc_dma *mc_dma_obj, rt_uint32_t no, rt_uint32_t len){
	rt_uint32_t ret;

	ret = dw_readl(mc_dma_obj, ch[no].blk_len);
	ret &= ~BLK_LEN_MASK;
	ret |= len;
	dw_writel(mc_dma_obj, ch[no].blk_len, ret);

}

void mc_dma_ch_trsc_len_set(struct dw_mc_dma *mc_dma_obj, rt_uint32_t no, rt_uint32_t len){

	rt_uint32_t ret;

	ret = dw_readl(mc_dma_obj, ch[no].trsc_len);
	ret &= ~TRSC_LEN_MASK;
	ret |= len;
	dw_writel(mc_dma_obj, ch[no].trsc_len, ret);

}

void mc_dma_ch_frag_len_set(struct dw_mc_dma *mc_dma_obj, rt_uint32_t no, rt_uint32_t len){

	rt_uint32_t ret;

	ret = dw_readl(mc_dma_obj, ch[no].frag_len);
	ret &= ~FRAG_LEN_MASK;
	ret |= len;
	dw_writel(mc_dma_obj, ch[no].frag_len, ret);

}

void mc_dma_ch_request(struct dw_mc_dma *mc_dma_obj, rt_uint32_t no, rt_uint32_t val){

	dw_writel(mc_dma_obj, ch[no].req, val);
}

void mc_dma_ch_llist_addr_set(struct dw_mc_dma *mc_dma_obj, rt_uint32_t no, rt_uint32_t list_addr){

	dw_writel(mc_dma_obj, ch[no].llist_ptr, list_addr);
}

void mc_dma_desc_set_src(struct mc_dma_lli *desc,rt_uint32_t val){
	desc->src_addr = val;
}

void mc_dma_desc_set_dst(struct mc_dma_lli *desc,rt_uint32_t val){
	desc->dst_addr = val;
}

void mc_dma_desc_set_trans_len(struct mc_dma_lli *desc, rt_uint32_t len){
	desc->trans_len &= ~TRSC_LEN_MASK;
	desc->trans_len |= len;
}


void mc_dma_desc_set_blk_len(struct mc_dma_lli *desc, rt_uint32_t len){
	desc->blk_len &= ~BLK_LEN_MASK;
	desc->blk_len |= len;
}

void mc_dma_desc_set_frag_len(struct mc_dma_lli *desc, rt_uint32_t len){
	desc->frag_len &= ~FRAG_LEN_MASK;
	desc->frag_len |= len;
}

void mc_dma_desc_set_req_mode(struct mc_dma_lli *desc,rt_uint32_t val){

	desc->frag_len &= ~(REQ_MODE_MASK<<REQ_MODE_SHIFT);
	desc->frag_len |= val<<REQ_MODE_SHIFT;
}


void mc_dma_desc_set_ahb_size(struct mc_dma_lli *desc,rt_uint32_t src_size,rt_uint32_t dest_size){
	desc->frag_len &= ~(SRC_DST_AHB_SIZE_MASK<<DST_AHB_SIZE_SHIFT);
	desc->frag_len |= src_size<< SRC_AHB_SIZE_SHIFT|dest_size<<DST_AHB_SIZE_SHIFT;
}

void mc_dma_desc_set_inc_mode(struct mc_dma_lli *desc, rt_uint32_t src_inc, rt_uint32_t dst_inc){

	if((src_inc == DW_DMA_SLAVE_DEC) || (dst_inc == DW_DMA_SLAVE_DEC))
		MC_DMA_ASSERT(0);
	//only one side could be in fix mode....
	if((src_inc == DW_DMA_SLAVE_FIX) && (dst_inc == DW_DMA_SLAVE_FIX))
		MC_DMA_ASSERT(0);
	
	if((src_inc == DW_DMA_SLAVE_INC) && (dst_inc == DW_DMA_SLAVE_INC)){
		//src inc & dst inc
		desc->frag_len &= ~(1 << ADDR_FIX_EN_SHIFT);
	}
	else if(src_inc == DW_DMA_SLAVE_INC){
		//src inc & dst fix
		desc->frag_len |= (1 << ADDR_FIX_EN_SHIFT);
		desc->frag_len |= 1 << ADDR_FIX_SEL_SHIFT;
	}
	else{
		desc->frag_len |= (1 << ADDR_FIX_EN_SHIFT);
		desc->frag_len &= ~(1 << ADDR_FIX_SEL_SHIFT);
	}

}

void mc_dma_desc_set_wrap_disable(struct mc_dma_lli *desc){
	desc->frag_len &= ~(1<<ADDR_WRAP_EN_SHIFT);
}

void mc_dma_desc_set_trsf_step(struct mc_dma_lli *desc,rt_uint32_t src_step,rt_uint32_t dest_step){
	desc->trsf_step &= ~TRSF_STEP_MASK;
	desc->trsf_step |= dest_step<<DST_TRSF_STEP_SHIFT|src_step;
}


void mc_dma_desc_set_step(struct mc_dma_lli *desc,
rt_uint32_t frag_step,rt_uint32_t src_blk_step,rt_uint32_t dest_blk_step) {
	desc->frag_step = frag_step;
	desc->src_blk_step = src_blk_step;
	desc->dst_blk_step = dest_blk_step;
}


void mc_dma_desc_set_swtich_mode(struct mc_dma_lli *desc,rt_uint32_t mode){
	desc->frag_len &= ~(SWT_MODE_MASK << SWT_MODE_SHIFT);
	mode &= SWT_MODE_MASK;
	desc->frag_len |= (mode << SWT_MODE_SHIFT);
}

void mc_dma_desc_set_llist_end(struct mc_dma_lli *desc,rt_uint32_t val){
	if(val == LLIST_END)
		desc->frag_len |= 1<<LLIST_END_SHIFT;
	else
		desc->frag_len &= ~(1<<LLIST_END_SHIFT);
}


void mc_dma_desc_set_isr_enable(struct mc_dma_lli *desc,rt_uint32_t isr_mode){
	desc->trans_len |= CHN_TRSC_LEN_LLIST_NODE_INT_EN;
	desc->trans_len |= isr_mode;
}

void mc_dma_desc_set_isr_disable(struct mc_dma_lli *desc){
	desc->trans_len &= ~CHN_TRSC_LEN_LLIST_NODE_INT_EN;
}

void mc_dma_desc_bind_next_desc(struct mc_dma_lli *desc,rt_uint32_t val){
	desc->llist_ptr = val;
}

#if (0)
static void mc_dma_isr_common_enable(struct dw_mc_dma *mc_dma_obj)
{

}

static void mc_dma_isr_common_disable(struct dw_mc_dma *mc_dma_obj)
{

}
#endif
static void handle_dma_open(struct fh_dma *p_dma)
{
	struct dw_mc_dma *temp_dwc;

	temp_dwc = &p_dma->dwc;

	mc_dma_enable(temp_dwc);
	p_dma->dwc.controller_status = CONTROLLER_STATUS_OPEN;
}

static void handle_dma_close(struct fh_dma *p_dma)
{
	rt_uint32_t i;
	struct dw_mc_dma *temp_dwc;

	temp_dwc = &p_dma->dwc;
	/* take lock */
	for (i = 0; i < p_dma->dwc.channel_max_number; i++)
		p_dma->dma_channel[i].channel_status = CHANNEL_STATUS_CLOSED;

	mc_dma_reset(temp_dwc);
	p_dma->dwc.controller_status = CONTROLLER_STATUS_CLOSED;

}


static rt_uint32_t check_channel_real_free(struct fh_dma *p_dma,
rt_uint32_t channel_number)
{
	struct dw_mc_dma *temp_dwc;
	rt_uint32_t ret_status;

	temp_dwc = &p_dma->dwc;
	MC_DMA_ASSERT(channel_number < p_dma->dwc.channel_max_number);

	ret_status = dw_readl(temp_dwc, en_stat);
	if (ret_status & lift_shift_bit_num(channel_number)) {
		FH_MC_DMA_PRINT("channel_number : %d is busy....\n", channel_number);
		return CHANNEL_NOT_FREE;
	}
	return CHANNEL_REAL_FREE;
}

static rt_int32_t handle_request_channel(struct fh_dma *p_dma,
struct dma_transfer *p_transfer)
{
	rt_uint32_t i;
	rt_int32_t ret_status = MC_DMA_RET_OK;
	if (p_transfer->channel_number == AUTO_FIND_CHANNEL) {
		FH_MC_DMA_PRINT("auto request channel go...\n");
		/* check each channel lock,find a free channel... */
		for (i = 0; i < p_dma->dwc.channel_max_number; i++) {
			ret_status = mc_dma_trylock(&p_dma->dma_channel[i].channel_lock);
			if (ret_status == MC_DMA_RET_OK)
				break;
		}

		if (i < p_dma->dwc.channel_max_number) {
			ret_status = check_channel_real_free(p_dma, i);
			if (ret_status != CHANNEL_REAL_FREE) {
				FH_MC_DMA_PRINT("auto request channel error\n");
				MC_DMA_ASSERT(ret_status == CHANNEL_REAL_FREE);
			}
			/* caution : channel is already locked here.... */
			p_transfer->channel_number = i;
			/* bind to the controller. */
			/* p_transfer->dma_controller = p_dma; */
			p_dma->dma_channel[i].channel_status = CHANNEL_STATUS_OPEN;
		} else {
			FH_MC_DMA_PRINT("[dma]: auto request err, no free channel\n");
			return -MC_DMA_RET_NO_MEM;
		}
	}
    /* request channel by user */
	else {
		MC_DMA_ASSERT(p_transfer->channel_number < p_dma->dwc.channel_max_number);
		ret_status = mc_dma_lock(
			&p_dma->dma_channel[p_transfer->channel_number].channel_lock,
			MC_DMA_TICK_PER_SEC * 2);
		if (ret_status != MC_DMA_RET_OK) {
			FH_MC_DMA_PRINT("[dma]: request %d channel err.\n", p_transfer->channel_number);
			return -MC_DMA_RET_NO_MEM;
		}

		/*enter_critical();*/
		ret_status = check_channel_real_free(p_dma, p_transfer->channel_number);
		if (ret_status != CHANNEL_REAL_FREE) {
			FH_MC_DMA_PRINT("user request channel error\n");
			MC_DMA_ASSERT(ret_status == CHANNEL_REAL_FREE);
		}
		/* bind to the controller */
		/* p_transfer->dma_controller = p_dma; */
		p_dma->dma_channel[p_transfer->channel_number].channel_status =
		CHANNEL_STATUS_OPEN;
		/* exit_critical(); */
	}

	/* malloc desc for this one channel... */
	/* fix me.... */
    p_dma->dma_channel[p_transfer->channel_number].allign_malloc =
    mc_dma_malloc_desc(p_dma->kernel_pri,
    (p_dma->dma_channel[p_transfer->channel_number].desc_total_no *
    sizeof(struct mc_dma_lli)) +
    MC_DESC_ALLIGN, &(p_dma->dma_channel[p_transfer->channel_number].allign_phy));

	if (!p_dma->dma_channel[p_transfer->channel_number].allign_malloc) {
		/* release channel */
		FH_MC_DMA_PRINT("[dma]: no mem to malloc channel%d desc..\n",
		p_transfer->channel_number);
		p_dma->dma_channel[p_transfer->channel_number].channel_status =
		CHANNEL_STATUS_CLOSED;
		mc_dma_unlock(
			&p_dma->dma_channel[p_transfer->channel_number].channel_lock);
		return -MC_DMA_RET_NO_MEM;
	}

	p_dma->dma_channel[p_transfer->channel_number].base_lli_phy =
	p_dma->dma_channel[p_transfer->channel_number].allign_phy;
	p_dma->dma_channel[p_transfer->channel_number].base_lli =
	(struct mc_dma_lli *)allign_func(
		p_dma->dma_channel[p_transfer->channel_number].allign_malloc,
		MC_DESC_ALLIGN, &p_dma->dma_channel[p_transfer->channel_number].base_lli_phy);

	if (!p_dma->dma_channel[p_transfer->channel_number].base_lli) {
		FH_MC_DMA_PRINT("request desc failed..\n");
		MC_DMA_ASSERT(p_dma->dma_channel[p_transfer->channel_number].base_lli !=
		MC_DMA_NULL);
	}

	if ((rt_uint32_t)p_dma->dma_channel[p_transfer->channel_number].base_lli %
	MC_DESC_ALLIGN) {
		FH_MC_DMA_PRINT("malloc is not cache allign..");
	}

	/* mc_dma_memset((void *)dma_trans_desc->first_lli, 0, lli_size * sizeof(struct */
	/* mc_dma_lli)); */
	mc_dma_memset((void *)p_dma->dma_channel[p_transfer->channel_number].base_lli,
	0, p_dma->dma_channel[p_transfer->channel_number].desc_total_no *
	sizeof(struct mc_dma_lli));

	p_dma->dma_channel[p_transfer->channel_number].desc_left_cnt =
	p_dma->dma_channel[p_transfer->channel_number].desc_total_no;
	p_dma->dma_channel[p_transfer->channel_number].free_index = 0;
	p_dma->dma_channel[p_transfer->channel_number].used_index = 0;

	FH_MC_DMA_PRINT("[chan %d] :: base_lli : desc_total_no = %x : %x\n",
	p_transfer->channel_number,
	p_dma->dma_channel[p_transfer->channel_number].base_lli,
	p_dma->dma_channel[p_transfer->channel_number].desc_total_no);
	return MC_DMA_RET_OK;
}

static void fhc_chan_able_set(struct fh_dma *p_dma,
struct dma_transfer *p_transfer, rt_int32_t enable)
{
	struct dw_mc_dma *temp_dwc;

	temp_dwc = &p_dma->dwc;
	if (enable == 1)
		mc_dma_ch_enable(temp_dwc, p_transfer->channel_number);
	else 
		mc_dma_ch_disable(temp_dwc, p_transfer->channel_number);

}

static void fhc_chan_susp_set(struct fh_dma *p_dma,
struct dma_transfer *p_transfer, rt_int32_t enable)
{
	struct dw_mc_dma *temp_dwc;
	temp_dwc = &p_dma->dwc;

	if (enable)
		mc_dma_ch_pause_set(temp_dwc, p_transfer->channel_number, CHAN_PAUSE);
	else
		mc_dma_ch_pause_set(temp_dwc, p_transfer->channel_number, CHAN_ACTIVE);
}

static rt_uint32_t handle_release_channel(struct fh_dma *p_dma,
struct dma_transfer *p_transfer)
{

	rt_uint32_t ret_status;
	/* enter_critical(); */
	ret_status = p_dma->dma_channel[p_transfer->channel_number].channel_status;

	MC_DMA_ASSERT(p_transfer->channel_number < p_dma->dwc.channel_max_number);

	if (ret_status == CHANNEL_STATUS_CLOSED) {
		FH_MC_DMA_PRINT(
			"release channel error,reason: release a closed channel!!\n");
		MC_DMA_ASSERT(ret_status != CHANNEL_STATUS_CLOSED);
	}
	fhc_chan_able_set(p_dma, p_transfer, 0);


	/* p_transfer->dma_controller = MC_DMA_NULL; */
	p_dma->dma_channel[p_transfer->channel_number].channel_status =
	CHANNEL_STATUS_CLOSED;
	p_dma->dma_channel[p_transfer->channel_number].open_flag =
	0;
	/* exit_critical(); */

	mc_dma_free_desc(p_dma->kernel_pri,
	p_dma->dma_channel[p_transfer->channel_number].desc_total_no *
	sizeof(struct mc_dma_lli) + MC_DESC_ALLIGN,
	(void *)p_dma->dma_channel[p_transfer->channel_number].allign_malloc,
	p_dma->dma_channel[p_transfer->channel_number].allign_phy);

	p_dma->dma_channel[p_transfer->channel_number].allign_malloc = MC_DMA_NULL;
	p_dma->dma_channel[p_transfer->channel_number].base_lli = MC_DMA_NULL;
	p_dma->dma_channel[p_transfer->channel_number].allign_phy = MC_DMA_NULL;
	p_dma->dma_channel[p_transfer->channel_number].base_lli_phy = MC_DMA_NULL;

	p_dma->dma_channel[p_transfer->channel_number].desc_left_cnt =
	p_dma->dma_channel[p_transfer->channel_number].desc_total_no;
	p_dma->dma_channel[p_transfer->channel_number].free_index = 0;
	p_dma->dma_channel[p_transfer->channel_number].used_index = 0;
	p_dma->dma_channel[p_transfer->channel_number].active_trans = 0;
	mc_dma_unlock(&p_dma->dma_channel[p_transfer->channel_number].channel_lock);
	return MC_DMA_RET_OK;
}

void mc_dma_ctl_init(rt_uint32_t *low, rt_uint32_t *high)
{

}

void mc_dma_cfg_init(rt_uint32_t *low, rt_uint32_t *high)
{

}

rt_uint32_t fix_frag_len(struct dma_transfer *dma_trans_desc)
{
	rt_uint32_t ret = 4;
	rt_uint32_t parse_msize = 0;

	if (dma_trans_desc->ot_len_flag == USR_DEFINE_ONE_TIME_LEN)
		return dma_trans_desc->ot_len_len;

    switch (dma_trans_desc->fc_mode)
    {
	case DMA_P2M:
    case DMA_M2M:
		if (dma_trans_desc->src_msize == DW_DMA_SLAVE_MSIZE_1)
			parse_msize = 0;
		else
			parse_msize = dma_trans_desc->src_msize + 1;
		ret = (1 << dma_trans_desc->src_width) * (1 << parse_msize);
    break;
    case DMA_M2P:
		if (dma_trans_desc->dst_msize == DW_DMA_SLAVE_MSIZE_1)
			parse_msize = 0;
		else
			parse_msize = dma_trans_desc->dst_msize + 1;
		ret = (1 << dma_trans_desc->dst_width) * (1 << parse_msize);
    break;

    default:
    break;
    }
	return ret;
}

rt_uint32_t fix_usrdef_frag_len(struct usrdef_trans_desc *usr_desc)
{
	return usr_desc->ot_len_len;
}


void force_reset_channel(struct fh_dma *dma, rt_uint32_t chan)
{
	struct dw_mc_dma_chan_regs *p_ch;
	struct dw_mc_dma_regs *p_dma;

	p_dma = (struct dw_mc_dma_regs *)dma->dwc.regs;
	p_ch = &p_dma->ch[chan];
    mc_dma_memset(p_ch, 0, sizeof(struct dw_mc_dma_chan_regs));
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

    //here set hw ot size to size with width
    c_msize = c_msize >> p_transfer->src_width;
    channel_max_trans_per_lli = (p_transfer->period_len != 0) ?
    (MIN(p_transfer->period_len, c_msize)) : (c_msize);
    lli_number = (p_transfer->trans_len % channel_max_trans_per_lli) ? 1 : 0;
    lli_number += p_transfer->trans_len / channel_max_trans_per_lli;
    FH_MC_DMA_PRINT("cal lli size is %x\n",lli_number);
    return lli_number;

}

#if(0)
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

#define SWT_ABCD_ABCD	0
#define SWT_ABCD_DCBA	1
#define SWT_ABCD_BADC	2
#define SWT_ABCD_DCAB	3

	rt_uint32_t data_switch;
	#define USR_DEFINE_ONE_TIME_LEN    0x55aaaa55
	rt_uint32_t ot_len_flag;
	rt_uint32_t ot_len_len;

	rt_uint32_t sca_to_gat_en;
	rt_uint32_t sca_src_step;
};
#endif

void prep_usrdef_desc(	struct fh_dma *p_dma,
struct dma_transfer *p_transfer){
	int i;
    struct dma_transfer *dma_trans_desc;
    rt_uint32_t temp_src_add = 0;
    rt_uint32_t temp_dst_add = 0;
	rt_uint32_t lli_phy_add;
	rt_uint32_t lli_phy_base;
	rt_uint32_t lli_size;
	rt_uint32_t frag_len;
	struct mc_dma_lli *p_lli_base;
	struct mc_dma_lli *p_lli = MC_DMA_NULL;
	struct usrdef_trans_desc *usr_desc = MC_DMA_NULL;

	dma_trans_desc = p_transfer->dma_controller->dma_channel[p_transfer->channel_number].active_trans;
	usr_desc = dma_trans_desc->p_desc;
	lli_size = dma_trans_desc->desc_size;


	lli_phy_base = p_dma->dma_channel[p_transfer->channel_number].base_lli_phy;
	p_lli_base = p_dma->dma_channel[p_transfer->channel_number].base_lli;
	p_lli = dma_trans_desc->first_lli;

	for (i = 0; i < lli_size; i++, usr_desc++)
	{
		/* parse trans para... */
		/* para add: */
		temp_dst_add = usr_desc->dst_add;
		temp_src_add = usr_desc->src_add;

		mc_dma_desc_set_src(&p_lli[i], temp_src_add);
		mc_dma_desc_set_dst(&p_lli[i], temp_dst_add);
		/* para ctl */
		


		frag_len = fix_usrdef_frag_len(usr_desc);
		//if sca en ,len = len / (step + 1)
		//frag_len = 4;
		mc_dma_desc_set_trans_len(&p_lli[i], usr_desc->size);
		mc_dma_desc_set_blk_len(&p_lli[i], frag_len);
		mc_dma_desc_set_frag_len(&p_lli[i], frag_len);
		if (dma_trans_desc->fc_mode == DMA_M2M)
			mc_dma_desc_set_req_mode(&p_lli[i], LINKLIST);
		else
			mc_dma_desc_set_req_mode(&p_lli[i], FRAGMENT);

		mc_dma_desc_set_ahb_size(&p_lli[i], usr_desc->src_width, usr_desc->dst_width);


		mc_dma_desc_set_trsf_step(&p_lli[i],
		1 << usr_desc->src_width, 1 << usr_desc->dst_width);
		mc_dma_desc_set_wrap_disable(&p_lli[i]);
		mc_dma_desc_set_inc_mode(&p_lli[i],
		usr_desc->src_inc_mode, usr_desc->dst_inc_mode);

		if((usr_desc->data_switch & SWT_MAGIC) == SWT_MAGIC)
			mc_dma_desc_set_swtich_mode(&p_lli[i],
			usr_desc->data_switch & 0xf);
		mc_dma_desc_set_step(&p_lli[i], 0, 0, 0);
		mc_dma_desc_set_isr_disable(&p_lli[i]);
		if ((i + 1) < lli_size) {
			lli_phy_add = vir_lli_to_phy_lli(p_lli_base,
			lli_phy_base, &p_lli[i + 1]);
			mc_dma_desc_set_llist_end(&p_lli[i], NOT_LLIST_END);
			mc_dma_desc_bind_next_desc(&p_lli[i], lli_phy_add);

		} else {
			if (p_dma->dma_channel[p_transfer->channel_number].open_flag == SINGLE_TRANSFER)
				mc_dma_desc_set_llist_end(&p_lli[i], LLIST_END);
			else {
				/*cyclic make a ring..*/
				lli_phy_add = vir_lli_to_phy_lli(p_lli_base,
				lli_phy_base, &p_lli[0]);
				mc_dma_desc_bind_next_desc(&p_lli[i], lli_phy_add);
				//p_lli[i].llp_lo = lli_phy_add;
			}
		}
		mc_dma_clean_desc((rt_uint32_t)&p_lli[i],
		sizeof(struct mc_dma_lli));
#ifdef MC_DMA_DEBUG
		dump_lli_str(&p_lli[i], p_lli_base, lli_phy_base);
		//dump_lli(&p_lli[i], p_lli_base, lli_phy_base);
#endif
	}

}

void handle_transfer(struct fh_dma *p_dma,
struct dma_transfer *p_transfer)
{
	rt_uint32_t i;
	rt_uint32_t lli_phy_add;
	rt_uint32_t frag_len;
	struct mc_dma_lli *p_lli_base;
	volatile rt_uint32_t ret_status;
	rt_uint32_t lli_phy_base;
	struct mc_dma_lli *p_lli = MC_DMA_NULL;
	struct dw_mc_dma *temp_dwc;
    struct dma_transfer *dma_trans_desc;
    rt_uint32_t temp_src_add = 0;
    rt_uint32_t temp_dst_add = 0;
	rt_uint32_t lli_size, max_trans_size;
	rt_uint32_t trans_total_len = 0;
	rt_uint32_t temp_trans_size = 0;
	rt_uint32_t usrdef_flag = 0;
	rt_uint32_t pm_stat;

	dma_trans_desc = p_transfer;
	temp_dwc = &p_dma->dwc;

    pm_stat = mol_dma_pm_stat_get(p_dma);
    if (pm_stat == MOL_DMA_STAT_SUSPNED)
    {
        rt_kprintf("%s :: is suspend...ignore chan[%d] transfer\n",
        p_dma->dwc.name, p_transfer->channel_number);
        return;
    }
	p_transfer->dma_controller = p_dma;
	p_transfer->dma_controller->dma_channel[p_transfer->channel_number].active_trans = dma_trans_desc;
    RT_ASSERT(p_transfer->channel_number < p_dma->dwc.channel_max_number);
    RT_ASSERT(p_transfer->dma_number < DMA_CONTROLLER_NUMBER);
    /* when the dma transfer....the lock should be 0!!!! */
    /* or user may not request the channel... */
    RT_ASSERT(p_dma->dma_channel[p_transfer->channel_number].channel_lock.value == 0);

    ret_status = p_dma->dma_channel[p_transfer->channel_number].channel_status;
    if (ret_status == CHANNEL_STATUS_CLOSED)
    {
        RT_ASSERT(ret_status != CHANNEL_STATUS_CLOSED);
    }



	if(p_transfer->p_desc != MC_DMA_NULL)
		usrdef_flag = USR_DEF_MODE;

    max_trans_size =
        p_transfer->dma_controller->dma_channel[p_transfer->channel_number]
            .desc_trans_size;
    if(p_transfer->period_len != 0){
        max_trans_size = MIN(max_trans_size, p_transfer->period_len);
    }


	force_reset_channel(p_dma, p_transfer->channel_number);
	mc_dma_ch_isr_set(temp_dwc, p_transfer->channel_number, CH_AUTO_CLOSE_EN);

	trans_total_len = p_transfer->trans_len;
	/* handle desc */
	/* step1:cal lli size... */
	if(usrdef_flag == USR_DEF_MODE)
		lli_size = p_transfer->desc_size;
	else
		lli_size = cal_lli_size(dma_trans_desc);

	if (p_dma->dma_channel[p_transfer->channel_number].open_flag == CYCLIC_TRANSFER)
		dma_trans_desc->cyclic_periods = lli_size;
	/* step2:malloc lli_size  mem */
	/* dma_trans_desc->first_lli = (struct axi_dma_lli *)rt_malloc(lli_size * */
	/* sizeof(struct axi_dma_lli)); */
	dma_trans_desc->first_lli = get_desc(p_dma, p_transfer, lli_size);
	//fhc_chan_able_set(p_dma, dma_trans_desc, 0);
	/* not enough mem.. */
	if (dma_trans_desc->first_lli == MC_DMA_NULL) {
		FH_MC_DMA_PRINT("transfer error,reason: not enough mem..\n");
		MC_DMA_ASSERT(dma_trans_desc->first_lli != MC_DMA_NULL);
	}
	/* not enough mem.. */
	if (dma_trans_desc->first_lli == RT_NULL)
	{
		RT_ASSERT(dma_trans_desc->first_lli != RT_NULL);
	}
	mc_dma_invalidate_desc((rt_uint32_t)dma_trans_desc->first_lli,
	lli_size * sizeof(struct mc_dma_lli));
	mc_dma_memset((void *)dma_trans_desc->first_lli, 0,
	lli_size * sizeof(struct mc_dma_lli));
	lli_phy_base = p_dma->dma_channel[p_transfer->channel_number].base_lli_phy;
	p_lli_base = p_dma->dma_channel[p_transfer->channel_number].base_lli;
	p_lli = dma_trans_desc->first_lli;

	MC_DMA_ASSERT(((rt_uint32_t)p_lli & MC_DESC_ALLIGN_BIT_MASK) == 0);
	/* step3: set the mem.. */
	if(usrdef_flag == USR_DEF_MODE){
		prep_usrdef_desc(p_dma, p_transfer);
	}
	else{
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


			mc_dma_desc_set_src(&p_lli[i], temp_src_add);
			mc_dma_desc_set_dst(&p_lli[i], temp_dst_add);
			/* para ctl */
			temp_trans_size = (trans_total_len / max_trans_size)
									? max_trans_size
									: (trans_total_len % max_trans_size);
			trans_total_len -= temp_trans_size;

			frag_len = fix_frag_len(p_transfer);
			//if sca en ,len = len / (step + 1)
			//frag_len = 4;
			mc_dma_desc_set_trans_len(&p_lli[i], temp_trans_size * (1 << dma_trans_desc->src_width));
			if (dma_trans_desc->fc_mode == DMA_M2M)
				frag_len = MIN(DMA_MEMCPY_FIX_LEN,
				temp_trans_size * (1 << dma_trans_desc->src_width));
			
			mc_dma_desc_set_blk_len(&p_lli[i], frag_len);
			mc_dma_desc_set_frag_len(&p_lli[i], frag_len);
			if (dma_trans_desc->fc_mode == DMA_M2M)
				mc_dma_desc_set_req_mode(&p_lli[i], LINKLIST);
			else
				mc_dma_desc_set_req_mode(&p_lli[i], FRAGMENT);

			mc_dma_desc_set_ahb_size(&p_lli[i], dma_trans_desc->src_width, dma_trans_desc->dst_width);


			mc_dma_desc_set_trsf_step(&p_lli[i],
			1 << dma_trans_desc->src_width, 1 << dma_trans_desc->dst_width);
			mc_dma_desc_set_wrap_disable(&p_lli[i]);
			mc_dma_desc_set_inc_mode(&p_lli[i],
			dma_trans_desc->src_inc_mode, dma_trans_desc->dst_inc_mode);

			if((dma_trans_desc->data_switch & SWT_MAGIC) == SWT_MAGIC)
				mc_dma_desc_set_swtich_mode(&p_lli[i],
				dma_trans_desc->data_switch & 0xf);
			mc_dma_desc_set_step(&p_lli[i], 0, 0, 0);
			mc_dma_desc_set_isr_disable(&p_lli[i]);
			if ((i + 1) < lli_size) {
				lli_phy_add = vir_lli_to_phy_lli(p_lli_base,
				lli_phy_base, &p_lli[i + 1]);
				mc_dma_desc_set_llist_end(&p_lli[i], NOT_LLIST_END);
				mc_dma_desc_bind_next_desc(&p_lli[i], lli_phy_add);

			} else {
				if (p_dma->dma_channel[p_transfer->channel_number].open_flag == SINGLE_TRANSFER)
					mc_dma_desc_set_llist_end(&p_lli[i], LLIST_END);
				else {
					/*cyclic make a ring..*/
					lli_phy_add = vir_lli_to_phy_lli(p_lli_base,
					lli_phy_base, &p_lli[0]);
					mc_dma_desc_bind_next_desc(&p_lli[i], lli_phy_add);
					//p_lli[i].llp_lo = lli_phy_add;
				}
			}
			mc_dma_clean_desc((rt_uint32_t)&p_lli[i],
			sizeof(struct mc_dma_lli));
	#ifdef MC_DMA_DEBUG
			dump_lli_str(&p_lli[i], p_lli_base, lli_phy_base);
			//dump_lli(&p_lli[i], p_lli_base, lli_phy_base);
	#endif
		}
	}

	if (p_dma->dma_channel[p_transfer->channel_number].open_flag == SINGLE_TRANSFER)
		mc_dma_ch_isr_set(temp_dwc, p_transfer->channel_number, CH_LLIST_INT_EN);
	else
		mc_dma_ch_isr_set(temp_dwc, p_transfer->channel_number, CH_TRSC_INT_EN);
	/* only hw handshaking need this.. */
	switch (dma_trans_desc->fc_mode)
	{
		case DMA_M2M:
			mc_dma_ch_request(temp_dwc, dma_trans_desc->channel_number, 1);
			break;
		case DMA_M2P:
			mc_dma_ch_request(temp_dwc, dma_trans_desc->channel_number, 0);
			dw_writel(temp_dwc, hand_cid[dma_trans_desc->dst_per], dma_trans_desc->channel_number + 1);
			break;
		case DMA_P2M:
			mc_dma_ch_request(temp_dwc, dma_trans_desc->channel_number, 0);
			dw_writel(temp_dwc, hand_cid[dma_trans_desc->src_per], dma_trans_desc->channel_number + 1);
			break;
		default:
			break;
	}


	dma_trans_desc->dma_controller
	->dma_channel[dma_trans_desc->channel_number]
	.channel_status = CHANNEL_STATUS_BUSY;

	if (dma_trans_desc->prepare_callback)
		dma_trans_desc->prepare_callback(dma_trans_desc->prepare_para);

	lli_phy_add = vir_lli_to_phy_lli(p_lli_base, lli_phy_base, &p_lli[0]);
	mc_dma_ch_llist_addr_set(temp_dwc, dma_trans_desc->channel_number, lli_phy_add);
	mc_dma_ch_llist_en(temp_dwc, dma_trans_desc->channel_number);

	if (p_dma->dma_channel[p_transfer->channel_number].open_flag == SINGLE_TRANSFER)
		fhc_chan_able_set(p_dma, dma_trans_desc, 1);
}


char * cmd_buf [] = {
 "RT_DEVICE_CTRL_DMA_NULL_CMD" ,
 "RT_DEVICE_CTRL_DMA_OPEN" ,
 "RT_DEVICE_CTRL_DMA_CLOSE ",
 "RT_DEVICE_CTRL_DMA_REQUEST_CHANNEL",
 "RT_DEVICE_CTRL_DMA_RELEASE_CHANNEL" ,
 "RT_DEVICE_CTRL_DMA_SINGLE_TRANSFER",

 "RT_DEVICE_CTRL_DMA_CYCLIC_PREPARE",
 "RT_DEVICE_CTRL_DMA_CYCLIC_START ",
 "RT_DEVICE_CTRL_DMA_CYCLIC_STOP ",
 "RT_DEVICE_CTRL_DMA_CYCLIC_FREE ",
 "RT_DEVICE_CTRL_DMA_PAUSE ",
 "RT_DEVICE_CTRL_DMA_RESUME" ,
 "RT_DEVICE_CTRL_DMA_GET_DAR" ,
 "RT_DEVICE_CTRL_DMA_GET_SAR ",
};


rt_int32_t mc_dma_driver_control(struct fh_dma *dma,
rt_uint32_t cmd, struct dma_transfer *arg)
{
	struct fh_dma *my_own = dma;
	struct dw_mc_dma *temp_dwc = &my_own->dwc;
	rt_int32_t ret = MC_DMA_RET_OK;
	struct dma_transfer *p_dma_transfer = arg;

	//rt_kprintf("%s :: cmd is %s\n",__func__,cmd_buf[cmd]);

	switch (cmd) {
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
		ret = handle_request_channel(my_own, p_dma_transfer);
		break;
	case RT_DEVICE_CTRL_DMA_RELEASE_CHANNEL:
		/* release a channel */
		MC_DMA_ASSERT(p_dma_transfer != MC_DMA_NULL);
		ret = handle_release_channel(my_own, p_dma_transfer);
		break;
	case RT_DEVICE_CTRL_DMA_SINGLE_TRANSFER:
		/* make a channel to transfer data. */
		MC_DMA_ASSERT(p_dma_transfer != MC_DMA_NULL);
		/* check if the dma channel is open,or return error. */
		my_own->dma_channel[p_dma_transfer->channel_number].open_flag =
		SINGLE_TRANSFER;
		handle_transfer(my_own, p_dma_transfer);
		/* then wait for the channel is complete.. */
		/* caution that::we should be in the "enter_critical()"when set the */
		/* dma to work. */
		break;

	case RT_DEVICE_CTRL_DMA_CYCLIC_PREPARE:
		MC_DMA_ASSERT(p_dma_transfer != MC_DMA_NULL);
		my_own->dma_channel[p_dma_transfer->channel_number].open_flag =
		CYCLIC_TRANSFER;
		handle_transfer(my_own, p_dma_transfer);
		break;

	case RT_DEVICE_CTRL_DMA_CYCLIC_START:
		fh_mc_dma_cyclic_start(p_dma_transfer);
		break;

	case RT_DEVICE_CTRL_DMA_CYCLIC_STOP:
		fh_mc_dma_cyclic_stop(p_dma_transfer);
		break;

	case RT_DEVICE_CTRL_DMA_CYCLIC_FREE:
		fh_mc_dma_cyclic_free(p_dma_transfer);
		break;
	case RT_DEVICE_CTRL_DMA_PAUSE:
		fh_mc_dma_cyclic_pause(p_dma_transfer);
		break;
	case RT_DEVICE_CTRL_DMA_RESUME:
		fh_mc_dma_cyclic_resume(p_dma_transfer);
		break;

	case RT_DEVICE_CTRL_DMA_GET_DAR:
		return dw_readl(temp_dwc, ch[p_dma_transfer->channel_number].dst_addr);
	case RT_DEVICE_CTRL_DMA_GET_SAR:
		return dw_readl(temp_dwc, ch[p_dma_transfer->channel_number].src_addr);

	case RT_DEVICE_CTRL_DMA_USR_DEF:
		//fh_mc_dma_cyclic_resume(p_dma_transfer);
		/* make a channel to transfer data. */
		MC_DMA_ASSERT(p_dma_transfer != MC_DMA_NULL);
		MC_DMA_ASSERT(p_dma_transfer->p_desc != MC_DMA_NULL);
		/* check if the dma channel is open,or return error. */
		my_own->dma_channel[p_dma_transfer->channel_number].open_flag =
		SINGLE_TRANSFER;
		handle_transfer(my_own, p_dma_transfer);

		break;


	default:
		break;
	}

	return ret;
}

static void fh_isr_single_process(struct fh_dma *my_own,
struct dma_transfer *p_transfer)
{

	// if (p_transfer->isr_prepare_callback)
	// 	p_transfer->isr_prepare_callback(p_transfer->isr_prepare_para);

	if (p_transfer->complete_callback)
		p_transfer->complete_callback(p_transfer->complete_para);
	p_transfer->dma_controller
	->dma_channel[p_transfer->channel_number]
	.channel_status = CHANNEL_STATUS_IDLE;
	put_desc(my_own, p_transfer);
}

static void fh_isr_cyclic_process(struct fh_dma *my_own,
struct dma_transfer *p_transfer)
{
	struct dw_mc_dma *temp_dwc;
	struct mc_dma_lli *p_lli;

	temp_dwc = &my_own->dwc;
	p_lli = MC_DMA_NULL;

	// if (p_transfer->isr_prepare_callback)
	// 	p_transfer->isr_prepare_callback(p_transfer->isr_prepare_para);

	if (p_transfer->complete_callback)
		p_transfer->complete_callback(p_transfer->complete_para);
	p_lli = p_transfer->first_lli;
	/*invaild desc mem to cache...*/
	mc_dma_invalidate_desc((uint32_t)p_lli,
	sizeof(struct mc_dma_lli) * p_transfer->cyclic_periods);
	/*flush cache..*/
	mc_dma_clean_desc((uint32_t)p_lli,
	sizeof(struct mc_dma_lli) * p_transfer->cyclic_periods);

	mc_dma_ch_isr_set(temp_dwc, p_transfer->channel_number, CH_TRSC_INT_EN);
	/*kick dma again.*/
	//dw_writel(temp_dwc, CHAN[p_transfer->channel_number].BLK_TFR_RESU, 1);

}

void fh_mc_dma_isr(int vector, void *param)
{
    rt_uint32_t i;
    rt_uint32_t isr_rec;
    struct dw_mc_dma *temp_dwc;
    struct fh_dma *my_own;

    my_own = (struct fh_dma *)param;
    temp_dwc = &my_own->dwc;

	isr_rec = dw_readl(temp_dwc, int_mask_stat);
	for (i = 0; i < my_own->dwc.channel_max_number; i++) {
		if ((1 << i) & isr_rec) {
			/* close isr... */
			mc_dma_ch_isr_set(temp_dwc, i, 0);
			/* clear isr status */
			mc_dma_ch_clear_isr_status(temp_dwc, i);
			if (my_own->dma_channel[i].open_flag == SINGLE_TRANSFER) {
				if (my_own->dma_channel[i].active_trans)
					fh_isr_single_process(my_own,
					my_own->dma_channel[i].active_trans);
			}
			if (my_own->dma_channel[i].open_flag == CYCLIC_TRANSFER) {
				if (my_own->dma_channel[i].active_trans)
					fh_isr_cyclic_process(my_own,
					my_own->dma_channel[i].active_trans);
			}
		}	
	}

}

const char *channel_lock_name[MC_DMA_MAX_NR_CHANNELS] = {
	"channel_0_lock", "channel_1_lock",
	"channel_2_lock", "channel_3_lock",
	"channel_4_lock", "channel_5_lock",
	"channel_6_lock", "channel_7_lock",
	"channel_8_lock", "channel_9_lock",
	"channel_10_lock", "channel_11_lock",
	"channel_12_lock", "channel_13_lock",
	"channel_14_lock", "channel_15_lock",
	"channel_16_lock", "channel_17_lock",
	"channel_18_lock", "channel_19_lock",
	"channel_20_lock", "channel_21_lock",
	"channel_22_lock", "channel_23_lock",
	"channel_24_lock", "channel_25_lock",
	"channel_26_lock", "channel_27_lock",
	"channel_28_lock", "channel_29_lock",
	"channel_30_lock", "channel_31_lock",
};

static void fh_mc_dma_cyclic_pause(struct dma_transfer *p)
{
	struct fh_dma *my_own = p->dma_controller;

	fhc_chan_susp_set(my_own, p, 1);
}

static void fh_mc_dma_cyclic_resume(struct dma_transfer *p)
{
	struct fh_dma *my_own = p->dma_controller;

	fhc_chan_susp_set(my_own, p, 0);
}

static void fh_mc_dma_cyclic_stop(struct dma_transfer *p)
{
	struct fh_dma *my_own = p->dma_controller;

	/*fhc_chan_susp_set(my_own, p, 1);*/
	fhc_chan_able_set(my_own, p, 0);
}

static void fh_mc_dma_cyclic_start(struct dma_transfer *p)
{

	struct fh_dma *my_own = p->dma_controller;
	struct mc_dma_lli *p_lli = MC_DMA_NULL;

	p_lli = p->first_lli;
	/*fhc_chan_susp_set(my_own, p, 0);*/

	/* warnning!!!!must check if the add is 64Byte ally... */
	MC_DMA_ASSERT(((rt_uint32_t)p_lli & MC_DESC_ALLIGN_BIT_MASK) == 0);
	fhc_chan_able_set(my_own, p, 1);

}

#if (0)
static void fh_mc_dma_cyclic_prep(struct fh_dma *p_dma,
struct dma_transfer *p_transfer)
{

}
#endif

static void fh_mc_dma_cyclic_free(struct dma_transfer *p)
{
	struct fh_dma *my_own = p->dma_controller;
	put_desc(my_own, p);
}

rt_uint32_t cal_mc_dma_channel(void *regs)
{
	rt_uint32_t ret;

	ret = __dma_raw_readl(&(((struct dw_mc_dma_regs *)regs)->version));
	if ((ret & VERSION_FH_MASK) == VERSION_FH) {
		ret = __dma_raw_readl(&(((struct dw_mc_dma_regs *)regs)->feature));
		rt_kprintf("FEATURE :: %s : \n", ret & 1 ?
		"DMA was separate into 2 controllers" : "DMA keeps single");
		ret = __dma_raw_readl(&(((struct dw_mc_dma_regs *)regs)->channel_num));
		return ret;
	} else
		return MC_DMA_MAX_NR_CHANNELS;
}


struct mc_dma_ops *get_fh_mc_dma_ops(struct fh_dma *p_mc_dma)
{
	MC_DMA_ASSERT(p_mc_dma != MC_DMA_NULL);
	return &p_mc_dma->ops;
}


void get_desc_para(struct fh_dma *dma, struct dma_transfer *p_transfer,
rt_uint32_t *desc_size, rt_uint32_t *base_phy_add,
rt_uint32_t *each_desc_size, rt_uint32_t *each_desc_cap)
{
	*desc_size = dma->dma_channel[p_transfer->channel_number]
	.desc_total_no;
	*base_phy_add = dma->dma_channel[p_transfer->channel_number]
	.base_lli_phy;
	*each_desc_size = sizeof(struct mc_dma_lli);
	*each_desc_cap = dma->dma_channel[p_transfer->channel_number]
	.desc_trans_size;
}


static rt_uint32_t mol_dma_pm_stat_get(struct fh_dma *p_dma)
{
    rt_uint32_t ret;

    mc_dma_lock(&p_dma->dma_lock, RT_WAITING_FOREVER);
    ret = p_dma->pm_stat;
    mc_dma_unlock(&p_dma->dma_lock);
    return ret;
}

static void mol_dma_pm_stat_set(struct fh_dma *p_dma, rt_uint32_t pm_stat)
{
    mc_dma_lock(&p_dma->dma_lock, RT_WAITING_FOREVER);
    p_dma->pm_stat = pm_stat;
    mc_dma_unlock(&p_dma->dma_lock);
}


void debug_sw_state(struct fh_dma *fh_dma_p)
{
    int i;

    if (!fh_dma_p)
        return;

    rt_kprintf("%s :: info below\n",fh_dma_p->dwc.name);
    rt_kprintf("CONTROLLER :: [%s] : [%s]\n",
    (fh_dma_p->dwc.controller_status == CONTROLLER_STATUS_CLOSED) ? "CLOSED" : "OPEN",
    (mol_dma_pm_stat_get(fh_dma_p) == MOL_DMA_STAT_SUSPNED) ? "SUSPEND" : "NORMAL");
    for (i = 0; i < fh_dma_p->dwc.channel_max_number; i++)
    {
        rt_kprintf("\tCHAN[%2d] :: ",i);
        if (fh_dma_p->dma_channel[i].open_flag == CYCLIC_TRANSFER)
            rt_kprintf("CYCLIC :: ");
		else
            rt_kprintf("SINGLE :: ");
        rt_kprintf("SW %s :: ", debug_channel_state_str[fh_dma_p->dma_channel[i].channel_status]);
        //dump hw state....
        rt_kprintf("HW %s\n", check_channel_real_free(fh_dma_p, i) == CHANNEL_REAL_FREE ? "DISABLE" : "ENABLE");
    }
}

#ifdef RT_USING_PM

void mol_dma_suspend_process_single(struct fh_dma *fh_dma_p, int chan)
{
    int hw_chan_stat;
    struct dw_mc_dma *temp_dwc;
    int chan_wait_timeout = PM_DMA_CHAN_WAIT_MAX_TIME;
    temp_dwc = &fh_dma_p->dwc;
    do
    {
        hw_chan_stat = check_channel_real_free(fh_dma_p, chan);
        if(hw_chan_stat == CHANNEL_REAL_FREE)
            break;
        else
        {
            fh_udelay(1);
            chan_wait_timeout--;
        }
    } while(chan_wait_timeout);

    if (!chan_wait_timeout)
    {
        rt_kprintf("%s :: [%d] wait timeout..will force close chan..\n",
        fh_dma_p->dwc.name, chan);
        mc_dma_ch_disable(temp_dwc, chan);
    }
}

void mol_dma_suspend_process_cyclic(struct fh_dma *fh_dma_p, int chan)
{
    //force pause..then close chan..
    struct dw_mc_dma *temp_dwc;

    temp_dwc = &fh_dma_p->dwc;
    mc_dma_ch_pause_set(temp_dwc, chan, CHAN_PAUSE);
    mc_dma_ch_disable(temp_dwc, chan);
}

void mol_dma_suspend_process(struct fh_dma *fh_dma_p)
{
    int i;

    if (fh_dma_p->dwc.controller_status == CONTROLLER_STATUS_CLOSED)
		return;

    //wait each chan idle....
    for (i = 0; i < fh_dma_p->dwc.channel_max_number; i++)
	{
        if(fh_dma_p->dma_channel[i].open_flag == CYCLIC_TRANSFER)
            mol_dma_suspend_process_cyclic(fh_dma_p, i);
        else
            mol_dma_suspend_process_single(fh_dma_p, i);
    }
}

int mol_dma_suspend(const struct rt_device *device, rt_uint8_t mode)
{
    struct fh_dma *fh_dma_p;

    fh_dma_p = (struct fh_dma *)device->user_data;
    if (!fh_dma_p)
		MC_DMA_ASSERT(0);

    //debug_sw_state(fh_dma_p);
    //set pm stat first...ignore any new request now....
    mol_dma_pm_stat_set(fh_dma_p, MOL_DMA_STAT_SUSPNED);
    //wait all single tranfer done or cyclic force pause...then if timeout close all chan..
    mol_dma_suspend_process(fh_dma_p);
    return RT_EOK;
}

void mol_dma_resume(const struct rt_device *device, rt_uint8_t mode)
{
	//rt_kprintf("%s :: go....\n",__func__);
    int i;
    struct fh_dma *fh_dma_p;
    struct dw_mc_dma *temp_dwc;
    fh_dma_p = (struct fh_dma *)device->user_data;
    temp_dwc = &fh_dma_p->dwc;

	for (i = 0; i < fh_dma_p->dwc.channel_max_number; i++)
	{
        //chan hw close first...
        mc_dma_ch_disable(temp_dwc, i);
        mc_dma_ch_isr_set(temp_dwc, i , 0);
        mc_dma_ch_pause_set(temp_dwc, i, CHAN_ACTIVE);
        mc_dma_ch_clear_isr_status(temp_dwc, i);
    }
	//if pre open control...reopen now.
    if (fh_dma_p->dwc.controller_status == CONTROLLER_STATUS_OPEN)
        mc_dma_enable(temp_dwc);
    //at last set normal...
    mol_dma_pm_stat_set(fh_dma_p, MOL_DMA_STAT_NORMAL);
}

struct rt_device_pm_ops mol_dma_pm_ops =
{
    .suspend_late = mol_dma_suspend,
    .resume_late = mol_dma_resume
};
#endif


static rt_err_t init(struct rt_dma_device *dma)
{
    return RT_EOK;
}

static rt_err_t control(struct rt_dma_device *dma, int cmd, void *arg){

    struct fh_dma *my_own = (struct fh_dma *)dma->parent.user_data;
    struct dma_transfer *p_dma_transfer = (struct dma_transfer *)arg;


    return mc_dma_driver_control(my_own, cmd, p_dma_transfer);
}

static struct rt_dma_ops fh_dma_ops = {init, control};

static int mol_dma_probe(void *priv_data)
{
    rt_uint32_t i;
    struct fh_dma *fh_dma_p;
    struct dma_platform_data *plat = (struct dma_platform_data *)priv_data;
    struct rt_dma_device *rt_dma;
    struct dw_mc_dma *temp_dwc;
    char dma_dev_name[8]       = {0};


    fh_dma_p = (struct fh_dma *)rt_malloc(sizeof(struct fh_dma));

    if (!fh_dma_p)
    {
        rt_kprintf("ERROR: %s, malloc failed\n", __func__);
        return -RT_EFULL;
    }
    mc_dma_memset(fh_dma_p, 0, sizeof(struct fh_dma));
    rt_sprintf(dma_dev_name, "%s%d", plat->name, plat->id);

    rt_dma      = &fh_dma_p->parent;
    rt_dma->ops = &fh_dma_ops;

    /* soc para set */
    //fh_dma_p->dwc.name               = dma_dev_name;
    rt_memcpy(fh_dma_p->dwc.name, dma_dev_name,
    sizeof(dma_dev_name));
    fh_dma_p->dwc.regs               = (void *)plat->base;
	//fh_dma_p->dwc.irq                = 68;
    fh_dma_p->dwc.irq                = plat->irq;
    //fh_dma_p->dwc.channel_max_number = cal_axi_dma_channel(&fh_dma_p->dwc);
    fh_dma_p->dwc.channel_max_number = plat->channel_max_number;
    fh_dma_p->dwc.controller_status  = CONTROLLER_STATUS_CLOSED;
    fh_dma_p->dwc.id                 = plat->id;
    rt_kprintf("[mol_dma : %d : %08x] has channel %d\n",
	fh_dma_p->dwc.id, fh_dma_p->dwc.regs,fh_dma_p->dwc.channel_max_number);

    if (plat->dma_init)
        plat->dma_init();
    /* channel set */
    temp_dwc = &fh_dma_p->dwc;
    mc_dma_reset(temp_dwc);
    for (i = 0; i < plat->channel_max_number; i++)
    {
        fh_dma_p->dma_channel[i].channel_status = CHANNEL_STATUS_CLOSED;
        fh_dma_p->dma_channel[i].desc_total_no  = DESC_MAX_SIZE;

        mc_dma_list_init(&(fh_dma_p->dma_channel[i].queue));
        fh_dma_p->dma_channel[i].desc_trans_size =
            FH_CHANNEL_MAX_TRANSFER_SIZE;
        mc_dma_lock_init(&fh_dma_p->dma_channel[i].channel_lock,
                    channel_lock_name[i]);
        mc_dma_ch_isr_set(temp_dwc, i , 0);
        mc_dma_ch_pause_set(temp_dwc, i, CHAN_ACTIVE);
        mc_dma_ch_disable(temp_dwc, i);
        mc_dma_ch_clear_isr_status(temp_dwc, i);
    }
    mc_dma_lock_init(&fh_dma_p->dma_lock, fh_dma_p->dwc.name);
    mol_dma_pm_stat_set(fh_dma_p, MOL_DMA_STAT_NORMAL);
#ifdef RT_USING_PM
    rt_pm_device_register(&rt_dma->parent, &mol_dma_pm_ops);
#endif
    /* isr */
    rt_hw_interrupt_install(fh_dma_p->dwc.irq, fh_mc_dma_isr,
                            (void *)fh_dma_p, "dma_isr");
    rt_hw_interrupt_umask(fh_dma_p->dwc.irq);

    return rt_hw_dma_register(rt_dma, dma_dev_name, RT_DEVICE_FLAG_RDWR,
                              fh_dma_p);
}


static int mol_dma_exit(void *priv_data)
{
    return 0;
}

struct fh_board_ops dma_driver_ops = {
    .probe = mol_dma_probe, .exit = mol_dma_exit,
};


int fh_dma_init(void)
{
    return fh_board_driver_register("fh_dma", &dma_driver_ops);
}
