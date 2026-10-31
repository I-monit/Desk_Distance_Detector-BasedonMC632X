/**
 * Copyright (c) 2015-2019 Shanghai Fullhan Microelectronics Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 *
 * Change Logs:
 * Date           Author       Notes
 * 2019-04-12               the first version
 *
 */

#include <mmu.h>
#include <delay.h>
#include <rtthread.h>
#include <rthw.h>
#include "fh_chip.h"

#include "string.h"
#include "fh_sdio_reg.h"
#include "fh_sdio.h"
#include <drivers/sdio.h>
// #include "inc/fh_sdio.h"
#include "fh_arch.h"
#include "fh_pmu.h"
#include "dma_mem.h"
// #include "interrupt.h"
#include "fh_clock.h"
#include "fh_def.h"

/* #define DEBUG_SDIO */
#ifdef DEBUG_SDIO
#define SDIO_DBG(fmt, ...) do {rt_kprintf("%s-%d: " fmt, __func__, __LINE__, ##__VA_ARGS__); } while (0)
#else
#define SDIO_DBG(fmt, ...)
#endif

#define CMD_TIMEOUT_USEC 1000000
#define DATA_READY_TIMEOUT_USEC 2000000
#define DMA_TRANSFER_TIMEOUT_TICKS (RT_TICK_PER_SECOND * 2)

#define CIU_CLK 50000         /* 50MHz */
#define MMC_FOD_VALUE 125     /* 125 KHz */
#define MMC_FOD_DIVIDER_VALUE (((CIU_CLK + MMC_FOD_VALUE * 2 - 1) / (MMC_FOD_VALUE * 2)))


#define SDIO_EXEC_CMD(base, cmd, arg) \
    do \
    { \
        synopmob_set_register(base + CMDARG, arg); \
        synopmob_set_register(base + CMD, cmd | (CMD_START_BIT | CMD_HOLD_BIT)); \
    } while (0)


#define SDIO_STATASTIC_ENABLE
#ifdef SDIO_STATASTIC_ENABLE
#define SDIO_STATASTIC_INC(x) ((x)++)
#else
#define SDIO_STATASTIC_INC(x)
#endif

/*
#define SDIO_TIMING_DEBUG
#ifdef  SDIO_TIMING_DEBUG
#define SDIO_TIMING_BEGIN() \
    static int pts_mmax; \
    rt_base_t mtemp=rt_hw_interrupt_disable(); \
    int mpts=GET_PTS();

#define SDIO_TIMING_END(msg) \
    mpts=GET_PTS() - mpts; \
    rt_hw_interrupt_enable(mtemp); \
    if(mpts > pts_mmax){ \
        pts_mmax = mpts; \
        rt_kprintf(msg" take %d usec.\n", mpts); \
    }
#else
#define SDIO_TIMING_BEGIN()
#define SDIO_TIMING_END(msg)
#endif
*/

#ifdef FH_SDIO_IDMA_CHAIN
#define FH_SDIO_DESC_NUM_MAX 128
#define MMC_DMA_DESC_BUFF_SIZE (0x1f00)

typedef union {
    struct
    {
        UINT32 reserved : 1;                         /* 0~15 */
        UINT32 disable_interrupt_on_completion : 1;  /* 16~31 */
        UINT32 last_descriptor : 1;                  /* 0~15 */
        UINT32 first_descriptor : 1;                 /* 16~31 */
        UINT32 sencond_address_chained : 1;          /* 0~15 */
        UINT32 end_of_ring : 1;                      /* 16~31 */
        UINT32 reserved_29_6 : 24;                   /* 0~15 */
        UINT32 card_error_summary : 1;               /* 16~31 */
        UINT32 own : 1;                              /* 16~31 */
    } bit;
    UINT32 dw;
} MMC_DMA_Descriptor0;

typedef union {
    struct
    {
        UINT32 buffer1_size : 13;   /* 0~15 */
        UINT32 buffer2_size : 13;   /* 16~31 */
        UINT32 reserved_26_31 : 6;  /* 0~15 */
    } bit;
    UINT32 dw;
} MMC_DMA_Descriptor1;

typedef union {
    struct
    {
        UINT32 buffer_addr0 : 32;  /* 0~15 */
    } bit;
    UINT32 dw;
} MMC_DMA_Descriptor2;

typedef union {
    struct
    {
        UINT32 buffer_addr1 : 32; /* 0~15 */
    } bit;
    UINT32 dw;
} MMC_DMA_Descriptor3;

typedef struct
{
    MMC_DMA_Descriptor0 desc0; /* control and status information of descriptor */
    MMC_DMA_Descriptor1 desc1; /* buffer sizes */
    MMC_DMA_Descriptor2 desc2; /* physical address of the buffer 1 */
    MMC_DMA_Descriptor3 desc3; /* physical address of the buffer 2 */
} MMC_DMA_Descriptors;
#endif

typedef struct
{
    unsigned int wkmod;
    unsigned int addr_inc_mode;
#ifdef FH_SDIO_IDMA_CHAIN
    MMC_DMA_Descriptors *descriptors;
#else
    unsigned int pDmaDesc[(CACHE_LINE_SIZE*2 + sizeof(DmaDesc))/4];
#endif
    unsigned int rca;
    unsigned int ip_base;

#ifdef SDIO_STATASTIC_ENABLE
    unsigned int cmd52_read_cnt;
    unsigned int cmd52_write_cnt;
    unsigned int cmd53_rw_cnt;
#endif

    /*unsigned int rawstat;*/
    rt_sem_t sem;
    rt_sem_t mutex;
    void (*cb)(void);
} sdc_t;

extern void mmu_clean_dcache(rt_uint32_t buffer, rt_uint32_t size);
extern void mmu_invalidate_dcache(rt_uint32_t buffer, rt_uint32_t size);

static sdc_t *g_sdc_hdl[2];
static void OSSDCISR(int vector, void *param);

#ifdef SDIO_STATASTIC_ENABLE
static void SDIO_STATASTIC_SHOW(sdc_t *sdc)
{
    rt_kprintf("cmd52_rcount=%d,cmd52_wcount=%d,cmd53_rwcount=%d.\n", sdc->cmd52_read_cnt, sdc->cmd52_write_cnt, sdc->cmd53_rw_cnt);
}
#else
#define SDIO_STATASTIC_SHOW(h)
#endif

/* #define SDIO_TRACE_ENABLE */
#include "fh_sdio_trace.c"

static int synopmob_execute_command(unsigned int base, unsigned int cmd_register, unsigned int arg_register)
{
    int retries = CMD_TIMEOUT_USEC;

    synopmob_set_register(base + RINTSTS, 0xffff);  /*clear interrupts*/

    SDIO_EXEC_CMD(base, cmd_register, arg_register);

    while (retries-- > 0)
    {
        if (!(synopmob_read_register(base + CMD) & CMD_START_BIT))
        {
            return 0;
        }
        udelay(1);
    }

    return ERRCMDRETRIESOVER;
}

static int synopmob_wait_command_done(unsigned int base, unsigned int *inst, unsigned int mask)
{
    int retries = CMD_TIMEOUT_USEC;
    unsigned int sts;

    while (retries-- > 0)
    {
        sts = synopmob_read_register(base + RINTSTS);
        if ((sts & mask) == mask)
        {
            *inst = sts & (~INTMSK_SDIO);
            return 0;
        }
        udelay(1);
    }

    return ERRCMDRETRIESOVER;
}

static int synopmob_wait_data_ready(unsigned int base)
{
    int retries = DATA_READY_TIMEOUT_USEC;

    while (retries-- > 0)
    {
        if (!((synopmob_read_register(base + STATUS)) & STATUS_DATA_BUSY_BIT))
        {
            return 0;
        }

        udelay(1);
    }

    return ERRDATANOTREADY;
}

static int synopmob_handle_standard_rinsts(unsigned int raw_int_stat)
{
    int error_status = 0;

    if (raw_int_stat & INTMASK_ERROR)
    {
        if (raw_int_stat & INTMSK_RESP_ERR)
        {
            error_status = ERRRESPRECEP;
        }
        if (raw_int_stat & INTMSK_RCRC)
        {
            error_status = ERRRESPCRC;
        }
        if (raw_int_stat & INTMSK_DCRC)
        {
            error_status = ERRDCRC;
        }
        if (raw_int_stat & INTMSK_RTO)
        {
            error_status = ERRRESPTIMEOUT;
        }
        if (raw_int_stat & INTMSK_DTO)
        {
            error_status = ERRDRTIMEOUT;
        }
        if (raw_int_stat & INTMSK_HTO)
        {
            error_status = ERRUNDERWRITE;
        }
        if (raw_int_stat & INTMSK_FRUN)
        {
            error_status = ERROVERREAD;
        }
        if (raw_int_stat & INTMSK_HLE)
        {
            error_status = ERRHLE;
        }
        if (raw_int_stat & INTMSK_SBE)
        {
            error_status = ERRSTARTBIT;
        }
        if (raw_int_stat & INTMSK_EBE)
        {
            error_status = ERRENDBITERR;
        }
    }

    return error_status;
}

static int synopmob_check_r1_resp(unsigned int the_response)
{
    int retval = 0;

    if (the_response & R1CS_ERROR_OCCURED_MAP)
    {
        if (the_response & R1CS_ADDRESS_OUT_OF_RANGE)
        {
            retval = ERRADDRESSRANGE;
        }
        else if (the_response & R1CS_ADDRESS_MISALIGN)
        {
            retval = ERRADDRESSMISALIGN;
        }
        else if (the_response & R1CS_BLOCK_LEN_ERR)
        {
            retval = ERRBLOCKLEN;
        }
        else if (the_response & R1CS_ERASE_SEQ_ERR)
        {
            retval = ERRERASESEQERR;
        }
        else if (the_response & R1CS_ERASE_PARAM)
        {
            retval = ERRERASEPARAM;
        }
        else if (the_response & R1CS_WP_VIOLATION)
        {
            retval = ERRPROT;
        }
        else if (the_response & R1CS_CARD_IS_LOCKED)
        {
            retval = ERRCARDLOCKED;
        }
        else if (the_response & R1CS_LCK_UNLCK_FAILED)
        {
            retval = ERRCARDLOCKED;
        }
        else if (the_response & R1CS_COM_CRC_ERROR)
        {
            retval = ERRCRC;
        }
        else if (the_response & R1CS_ILLEGAL_COMMAND)
        {
            retval = ERRILLEGALCOMMAND;
        }
        else if (the_response & R1CS_CARD_ECC_FAILED)
        {
            retval = ERRECCFAILED;
        }
        else if (the_response & R1CS_CC_ERROR)
        {
            retval = ERRCCERR;
        }
        else if (the_response & R1CS_ERROR)
        {
            retval = ERRUNKNOWN;
        }
        else if (the_response & R1CS_UNDERRUN)
        {
            retval = ERRUNDERRUN;
        }
        else if (the_response & R1CS_OVERRUN)
        {
            retval = ERROVERRUN;
        }
        else if (the_response & R1CS_CSD_OVERWRITE)
        {
            retval = ERRCSDOVERWRITE;
        }
        else if (the_response & R1CS_WP_ERASE_SKIP)
        {
            retval = ERRPROT;
        }
        else if (the_response & R1CS_ERASE_RESET)
        {
            retval = ERRERASERESET;
        }
        else if (the_response & R1CS_SWITCH_ERROR)
        {
            retval = ERRFSMSTATE;
        }
    }

    return retval;
}

static int synopmob_check_r5_resp(unsigned int the_resp)
{
    int ret = 0;

    if (the_resp & R5_IO_ERR_BITS)
    {
        if (the_resp & R5_IO_GEN_ERR)
        {
            ret = ERRUNKNOWN;
        }
        else if (the_resp & R5_IO_FUNC_ERR)
        {
            ret = ERRBADFUNC;
        }
        else if (the_resp & R5_IO_OUT_RANGE)
        {
            ret = ERRADDRESSRANGE;
        }
    }

    return ret;
}

static int sd_send_cmd0(sdc_t *sdc)
{
    int ret;
    unsigned int intst;
    unsigned int base = sdc->ip_base;

    ret = synopmob_execute_command(base, 0|CMD_SEND_INITIALIZATION|CMD_WAIT_PRVDATA_COMP, 0);
    if (!ret)
    {
        ret = synopmob_wait_command_done(base, &intst, INTMSK_CMD_DONE);
        if (!ret)
        {
            synopmob_set_register(base + RINTSTS, intst);
            return synopmob_handle_standard_rinsts(intst);
        }
    }

    return ret;
}

static int sd_send_cmd3(sdc_t *sdc)
{
    int ret;
    unsigned int intst;
    unsigned int resp;
    unsigned int cmd_reg = CMD_WAIT_PRVDATA_COMP |CMD_CHECK_RESPONSE_CRC | CMD_WAIT_RESPONSE_EXPECT | 3/*CMD3*/;
    unsigned int base = sdc->ip_base;

    ret = synopmob_execute_command(base, cmd_reg, 0);
    if (!ret)
    {
        ret = synopmob_wait_command_done(base, &intst, INTMSK_CMD_DONE);
        if (!ret)
        {
            synopmob_set_register(base + RINTSTS, intst);
            ret = synopmob_handle_standard_rinsts(intst);
            if (!ret)
            {
                resp     = synopmob_read_register(base + RESP0);
                sdc->rca = resp >> 16;
            }
        }
    }

    return ret;
}

static int sd_send_cmd_r1(sdc_t *sdc, unsigned int cmd, unsigned int arg, unsigned int buzy)
{
    int ret;
    unsigned int intst;
    unsigned int resp;
    unsigned int base = sdc->ip_base;

    ret = synopmob_execute_command(base, cmd, arg);
    if (!ret)
    {
        ret = synopmob_wait_command_done(base, &intst, INTMSK_CMD_DONE);
        if (!ret)
        {
            synopmob_set_register(base + RINTSTS, intst);
            ret = synopmob_handle_standard_rinsts(intst);
            if (!ret)
            {
                resp = synopmob_read_register(base + RESP0);
                ret  = synopmob_check_r1_resp(resp);
                if (buzy && !ret)
                {
                    ret = synopmob_wait_data_ready(base);
                }
            }
        }
    }

    return ret;
}

static int sd_send_cmd7(sdc_t *sdc)
{
    unsigned int cmd_reg = CMD_WAIT_PRVDATA_COMP |CMD_CHECK_RESPONSE_CRC | CMD_WAIT_RESPONSE_EXPECT | 7/*CMD7*/;

    return sd_send_cmd_r1(sdc, cmd_reg, sdc->rca << 16, 1);
}

static int sd_send_cmd5(sdc_t *sdc, unsigned int arg, unsigned int *resp)
{
    unsigned int cmd_reg = CMD_WAIT_PRVDATA_COMP | CMD_WAIT_RESPONSE_EXPECT | 5/*CMD5*/;
    unsigned int intst;
    int ret;
    unsigned int base = sdc->ip_base;

    ret = synopmob_execute_command(base, cmd_reg, arg);
    if (!ret)
    {
        ret = synopmob_wait_command_done(base, &intst, INTMSK_CMD_DONE);
        if (!ret)
        {
            synopmob_set_register(base + RINTSTS, intst);
            ret = synopmob_handle_standard_rinsts(intst);
            if (!ret)
            {
                *resp = synopmob_read_register(base + RESP0);
            }
        }
    }

    return ret;
}

static int synopmob_send_clock_only_cmd(unsigned int base)
{
    return synopmob_execute_command(base, 0x202000, 0);
}

static int synopmob_disable_all_clocks(unsigned int base)
{
    synopmob_set_register(base + CLKENA, 0);
    return synopmob_send_clock_only_cmd(base);
}

static int synopmob_enable_clocks_with_val(unsigned int base, unsigned int val)
{
    synopmob_set_register(base + CLKENA, val);
    return synopmob_send_clock_only_cmd(base);
}

static int synopmob_set_clk_freq(sdc_t *sdc, unsigned int divider)
{
#define MAX_DIVIDER_VALUE 0xff

    unsigned int orig_clkena;
    int retval;
    unsigned int base = sdc->ip_base;

    if (divider > MAX_DIVIDER_VALUE)
    {
        return 0xffffffff;
    }

    /* To make sure we dont disturb enable/disable settings of the cards*/
    orig_clkena = synopmob_read_register(base + CLKENA);

    /* Disable all clocks before changing frequency the of card clocks */
    retval = synopmob_disable_all_clocks(base);
    if (retval != 0)
    {
        return retval;
    }
    /* Program the clock divider in our case it is divider 0 */
    synopmob_clear_bits(base + CLKDIV, MAX_DIVIDER_VALUE);
    synopmob_set_bits(base + CLKDIV, divider);

    /*Send the command to CIU using synopmob_send_clock_only_cmd and enable the
     * clocks in CLKENA register */
    retval = synopmob_send_clock_only_cmd(base);
    if (retval != 0)
    {
        synopmob_enable_clocks_with_val(base, orig_clkena);
        return retval;
    }

    return synopmob_enable_clocks_with_val(base, orig_clkena);
}

int sdio_drv_creg_read(HSDC handle, int addr, int fn, unsigned int *resp)
{
    sdc_t *sdc;
    unsigned int arg;
    unsigned int mreg;
    unsigned int cmd_reg = CMD_WAIT_PRVDATA_COMP |CMD_CHECK_RESPONSE_CRC | CMD_WAIT_RESPONSE_EXPECT | 52/*CMD52*/;
    unsigned int intst;
    int ret;
    unsigned int base;
    rt_err_t err;

    if (!resp || !handle)
    {
        return -1;
    }

    sdc = (sdc_t *)handle;
    base = sdc->ip_base;

    *resp = 0;

    err = rt_sem_take(sdc->mutex, RT_WAITING_FOREVER);
    if (err != RT_EOK)
    {
        return ERRNORES;
    }

    SDIO_STATASTIC_INC(sdc->cmd52_read_cnt);

    arg = (fn << 28) | (addr << 9);
    ret = synopmob_execute_command(base, cmd_reg, arg);
    if (!ret)
    {
        ret = synopmob_wait_command_done(base, &intst, INTMSK_CMD_DONE);
        if (!ret)
        {
            synopmob_set_register(base + RINTSTS, intst);  /* write to clear */
            ret = synopmob_handle_standard_rinsts(intst);
            if (!ret)
            {
                mreg = synopmob_read_register(base + RESP0);
                ret   = synopmob_check_r5_resp(mreg);
                if (!ret)
                {
                    *resp = mreg & 0xff;
                }
            }
        }
    }

    rt_sem_release(sdc->mutex);

    if (ret)
    {
        rt_kprintf("sdio_drv_creg_read[%08x-%08x]: failed with ret=%d!!!\n", fn, addr, ret);
        SDIO_STATASTIC_SHOW(sdc);
    }
    #ifdef SDIO_TRACE_ENABLE
    else
    {
        unsigned char mdata = *resp;
        sdio_add_trace_node(0/*CMD52*/, 0/*read*/, addr, fn, 0,  0, &mdata, 1);
    }
    #endif

    return ret;
}

int sdio_drv_creg_write(HSDC handle, int addr, int fn, unsigned char data,
        unsigned int *resp)
{
    sdc_t *sdc;
    unsigned int arg;
    unsigned int mrsp;
    unsigned int cmd_reg = CMD_WAIT_PRVDATA_COMP |CMD_CHECK_RESPONSE_CRC | CMD_WAIT_RESPONSE_EXPECT | 52/*CMD52*/;
    unsigned int intst;
    int ret;
    unsigned int base;
    rt_err_t err;

    if (!handle)
    {
        return -1;
    }

    sdc = (sdc_t *)handle;
    base = sdc->ip_base;

    arg = (1 << 31) | (fn << 28) | (addr << 9) | data;
    if (resp)
    {
        *resp = 0;
        arg |= (1 << 27); /*RAW flag, read after write...*/
    }

    err = rt_sem_take(sdc->mutex, RT_WAITING_FOREVER);
    if (err != RT_EOK)
    {
        return ERRNORES;
    }

    SDIO_STATASTIC_INC(sdc->cmd52_write_cnt);

    ret = synopmob_execute_command(base, cmd_reg, arg);
    if (!ret)
    {
        ret = synopmob_wait_command_done(base, &intst, INTMSK_CMD_DONE);
        if (!ret)
        {
            synopmob_set_register(base + RINTSTS, intst);  /* write to clear */
            ret = synopmob_handle_standard_rinsts(intst);
            if (!ret)
            {
                mrsp = synopmob_read_register(base + RESP0);
                ret   = synopmob_check_r5_resp(mrsp);
                if (!ret && resp)
                {
                    *resp =  mrsp & 0xFF;
                }
            }
        }
    }

    rt_sem_release(sdc->mutex);

    if (ret)
    {
        rt_kprintf("sdio_drv_creg_write[%08x-%08x]: failed with ret=%d!!!\n", fn, addr, ret);
        SDIO_STATASTIC_SHOW(sdc);
    }
    #ifdef SDIO_TRACE_ENABLE
    else
    {
        sdio_add_trace_node(0/*CMD52*/, 1/*write*/, addr, fn, 0,  0, &data, 1);
    }
    #endif

    return ret;
}

static void synopmob_setclr_intmsk(unsigned int base, int set, int mask)
{
    rt_base_t temp;

    temp = rt_hw_interrupt_disable();

    if (set)
    {
        synopmob_set_bits(base + INTMSK, mask);
    }
    else
    {
        synopmob_clear_bits(base + INTMSK, mask);
    }

    rt_hw_interrupt_enable(temp);
}

#ifdef FH_SDIO_IDMA_CHAIN
/* refer to MMC_InitDescriptors() in fh_mmc.c */
void MMC_InitDescriptors_(sdc_t *sdc, rt_uint32_t *buf,
                         rt_uint32_t size)
{
    MMC_DMA_Descriptors *desc;
    rt_uint32_t len = 0;
    int desc_cnt = 0;

    desc = sdc->descriptors;

    while (size > 0)
    {
        desc[desc_cnt].desc0.dw = 0;
        desc[desc_cnt].desc1.dw = 0;
        desc[desc_cnt].desc2.dw = 0;
        desc[desc_cnt].desc3.dw = 0;
        desc[desc_cnt].desc0.bit.own                     = 1;
        desc[desc_cnt].desc0.bit.sencond_address_chained = 1;
        desc[desc_cnt].desc1.bit.buffer1_size =
            MIN(MMC_DMA_DESC_BUFF_SIZE, size);
        desc[desc_cnt].desc2.bit.buffer_addr0 = (rt_uint32_t)buf + len;
        desc[desc_cnt].desc3.bit.buffer_addr1 =
            (rt_uint32_t)sdc->descriptors +
            (desc_cnt + 1) * sizeof(MMC_DMA_Descriptors);

        size -= desc[desc_cnt].desc1.bit.buffer1_size;
        len += desc[desc_cnt].desc1.bit.buffer1_size;
        desc_cnt++;
    }

    desc[0].desc0.bit.first_descriptor           = 1;
    desc[desc_cnt - 1].desc0.bit.last_descriptor = 1;
    desc[desc_cnt - 1].desc3.bit.buffer_addr1    = 0;
}
#endif

static int sdio_drv_read_write(sdc_t *sdc, unsigned int rw, unsigned int addr,
        unsigned int fn, unsigned int bcnt,
        unsigned int bsize, unsigned char *buf)
{
#ifdef FH_SDIO_IDMA_CHAIN
    int ret;
    unsigned int cmd = 0x2275;
    unsigned int base;
    unsigned int arg;
    unsigned int num;
    unsigned int rintst;
    rt_err_t err;

    if (!sdc)
    {
        return -1;
    }
    base = sdc->ip_base;
    arg = (fn << 28) | (addr << 9) | (sdc->addr_inc_mode << 26);

    if (bcnt == 1 && bsize <= 512)
        arg |= (bsize & 0x1ff);
    else
        arg |= ((1 << 27) | bcnt);

    if (rw)
    {
        cmd |= 0x400;
        arg |= (1 << 31);
    }
    num = bsize * bcnt;

    if (num > MMC_DMA_DESC_BUFF_SIZE * FH_SDIO_DESC_NUM_MAX)
    {
        rt_kprintf("%s-%d: failed with big packet(%d)!\n", __func__, __LINE__, num);
        return -2;
    }
    if (rw)
    {
        mmu_clean_dcache((rt_uint32_t)buf, (rt_uint32_t)num);
    }
    else
    {
        if (((unsigned long)buf & (CACHE_LINE_SIZE - 1)) != 0)
        {
            rt_kprintf("%s-%d: invalid addr %p.\n", __func__, __LINE__, buf);
            return -3;
        }

        mmu_invalidate_dcache((rt_uint32_t)buf, (rt_uint32_t)num);
    }

    err = rt_sem_take(sdc->mutex, RT_WAITING_FOREVER);
    if (err != RT_EOK)
    {
        return -4;
    }
    SDIO_STATASTIC_INC(sdc->cmd53_rw_cnt);

/*     pDmaDesc->desc0 = DescSecAddrChained | DescOwnByDma | DescFirstDesc | DescLastDesc;
    pDmaDesc->desc1 = ((num << DescBuf1SizeShift) & DescBuf1SizMsk);
    pDmaDesc->desc2 = (unsigned int)buf;
    pDmaDesc->desc3 = (unsigned int)(pDmaDesc); */
    MMC_InitDescriptors_(sdc, (rt_uint32_t *)buf, num);
    mmu_clean_dcache((rt_uint32_t)(sdc->descriptors), (rt_uint32_t)((sizeof(MMC_DMA_Descriptors)) * FH_SDIO_DESC_NUM_MAX));

    /* reset */
    synopmob_set_bits(base + CTRL, FIFO_RESET);  /* reset FIFO */
    while ((synopmob_read_register(base + CTRL) & FIFO_RESET) != 0)
        ;
    synopmob_set_register(base + RINTSTS, 0xffff);  /* clear interrupts */

    synopmob_set_register(base + DBADDR, (unsigned int)(sdc->descriptors));
    synopmob_set_register(base + BLKSIZ, bsize);
    synopmob_set_register(base + BYTCNT, num);
    synopmob_set_bits(base + BMOD, BMOD_DE | BMOD_BURST_INCR);

    synopmob_setclr_intmsk(base, 1/*set*/, INTMSK_DAT_ALL);

    SDIO_EXEC_CMD(base, cmd, arg);
    ret = rt_sem_take(sdc->sem, DMA_TRANSFER_TIMEOUT_TICKS);
    if (!ret)
    {
        ret = synopmob_wait_command_done(base, &rintst, INTMSK_CMD_DONE);
        if (!ret)
        {
            ret = synopmob_handle_standard_rinsts(rintst);
        }
        synopmob_set_register(base + RINTSTS, 0xffff);
    }
    else
    {
        ret = ERRIDMA;
    }

    if (ret) /*it will be cleared in interrupt when success.*/
    {
        /*it will be cleared here when failed...*/
        synopmob_setclr_intmsk(base, 0/*clear*/, INTMSK_DAT_ALL);
    }

    synopmob_clear_bits(base + BMOD, BMOD_DE);

    if (rw && !ret)
    {
        ret = synopmob_wait_data_ready(base);
    }

    synopmob_set_bits(base + BMOD, BMOD_SWR); /*reset DMA*/

    rt_sem_release(sdc->mutex);

    if (ret)
    {
        char *op = rw ? "write" : "read";
        rt_kprintf("%s-%d: sdio_drv_%s(cmd 0x%x arg 0x%x):ret=%d\n",
            __func__, __LINE__, op, (cmd | (CMD_START_BIT | CMD_HOLD_BIT)), arg, ret);
        SDIO_STATASTIC_SHOW(sdc);
    }

    return ret;
#else
    DmaDesc *pDmaDesc;
    int ret;
    unsigned int cmd    = 0x2275;
    unsigned int base;
    unsigned int arg;
    unsigned int num;
    unsigned int rintst;
    rt_err_t err;

    if (!sdc)
    {
        return -1;
    }

    base   = sdc->ip_base;
    pDmaDesc = (DmaDesc *)(((unsigned int)(sdc->pDmaDesc) + CACHE_LINE_SIZE - 1) & (~(CACHE_LINE_SIZE-1)));

    arg = (fn << 28) | (addr << 9) | (sdc->addr_inc_mode << 26);

    if (bcnt == 1 && bsize <= 512)
        arg |= (bsize & 0x1ff);
    else
        arg |= ((1 << 27) | bcnt);

    if (rw)
    {
        cmd |= 0x400;
        arg |= (1 << 31);
    }
    num = bsize * bcnt;

    /*length check, from linux driver...*/
    if (num > 0x1F00)
    {
        rt_kprintf("sdio_drv_read_write: failed with big packet(%d)!!!\n", num);
        return -1;
    }
    if (rw)
    {
        mmu_clean_dcache((rt_uint32_t)buf, (rt_uint32_t)num);
    }
    else
    {
        if (((unsigned long)buf & (CACHE_LINE_SIZE - 1)) != 0)
        {
            rt_kprintf("sdio_drv_read: invalid addr %p.\n", buf);
            return -1;
        }

        mmu_invalidate_dcache((rt_uint32_t)buf, (rt_uint32_t)num);
    }

    err = rt_sem_take(sdc->mutex, RT_WAITING_FOREVER);
    if (err != RT_EOK)
    {
        return ERRNORES;
    }

    SDIO_STATASTIC_INC(sdc->cmd53_rw_cnt);

    pDmaDesc->desc0 = DescSecAddrChained | DescOwnByDma | DescFirstDesc | DescLastDesc;
    pDmaDesc->desc1 = ((num << DescBuf1SizeShift) & DescBuf1SizMsk);
    pDmaDesc->desc2 = (unsigned int)buf;
    pDmaDesc->desc3 = (unsigned int)(pDmaDesc);
    mmu_clean_dcache((rt_uint32_t)pDmaDesc, (rt_uint32_t)(sizeof(DmaDesc)));

    /* reset */
    synopmob_set_bits(base + CTRL, FIFO_RESET);  /* reset FIFO */
    while ((synopmob_read_register(base + CTRL) & FIFO_RESET) != 0)
        ;
    synopmob_set_register(base + RINTSTS, 0xffff);  /* clear interrupts */

    synopmob_set_register(base + DBADDR, (unsigned int)pDmaDesc);
    synopmob_set_register(base + BLKSIZ, bsize);
    synopmob_set_register(base + BYTCNT, num);
    synopmob_set_bits(base + BMOD, BMOD_DE | BMOD_BURST_INCR);

    synopmob_setclr_intmsk(base, 1/*set*/, INTMSK_DAT_ALL);

    SDIO_EXEC_CMD(base, cmd, arg);
    ret = rt_sem_take(sdc->sem, DMA_TRANSFER_TIMEOUT_TICKS);
    if (!ret)
    {
        ret = synopmob_wait_command_done(base, &rintst, INTMSK_CMD_DONE);
        if (!ret)
        {
            ret = synopmob_handle_standard_rinsts(rintst);
        }
        synopmob_set_register(base + RINTSTS, 0xffff);
    }
    else
    {
        ret = ERRIDMA;
    }

    if (ret) /*it will be cleared in interrupt when success.*/
    {
        /*it will be cleared here when failed...*/
        synopmob_setclr_intmsk(base, 0/*clear*/, INTMSK_DAT_ALL);
    }

    synopmob_clear_bits(base + BMOD, BMOD_DE);

    if (rw && !ret)
    {
        ret = synopmob_wait_data_ready(base);
    }

    synopmob_set_bits(base + BMOD, BMOD_SWR); /*reset DMA*/

    rt_sem_release(sdc->mutex);

    if (ret)
    {
        char *op   = rw ? "write" :  "read";

        rt_kprintf("sdio_drv_%s: failed with ret=%d\n", op, ret);
        SDIO_STATASTIC_SHOW(sdc);
    }
    #ifdef SDIO_TRACE_ENABLE
    else
    {
        sdio_add_trace_node(1/*CMD53*/, rw, addr, fn, bcnt,  bsize, buf, num);
    }
    #endif

    return ret;
#endif
}

int sdio_drv_read(HSDC handle, unsigned int addr, unsigned int fn,
        unsigned int bcnt, unsigned int bsize, unsigned char *buf)
{
    return sdio_drv_read_write((sdc_t *)handle, 0, addr, fn, bcnt, bsize, buf);
}

int sdio_drv_write(HSDC handle, unsigned int addr, unsigned int fn,
        unsigned int bcnt, unsigned int bsize, unsigned char *buf)
{
    return sdio_drv_read_write((sdc_t *)handle, 1, addr, fn, bcnt, bsize, buf);
}

#ifdef DONT_COPY_NET_PAYLOAD_TO_SEND

static void dumpchain(DmaDesc *pChain)
{
    int i = 0;
    DmaDesc *tmp_pChain = pChain;

    while (tmp_pChain && i < 10)
    {
        rt_kprintf(
                "[%d]: chain =%p, buf = %p, size = %d, csi = %08x, next = %p\n", i,
                tmp_pChain, (DmaDesc *)tmp_pChain->desc2, tmp_pChain->desc1,
                tmp_pChain->desc0, (DmaDesc *)tmp_pChain->desc3);
        tmp_pChain = (DmaDesc *)tmp_pChain->desc3;
        i++;

        if (tmp_pChain == pChain)
            break;
    }
}

/* this function is only used by marvell wifi */
int sdio_drv_chain_write(HSDC handle, unsigned int addr, unsigned int fn,
        unsigned int bcnt, unsigned int bsize,
        buf_chain_t *chain)
{
    int ret;
    sdc_t *sdc = (sdc_t *)handle;
    unsigned int cmd    = 0x2275 | 0x400;
    unsigned int base   = sdc->ip_base;
    unsigned int arg;
    unsigned int num;
    unsigned int rintst;
    unsigned int chain_len          = 0;
    rt_err_t err;
    DmaDesc *tmpDesc  = (DmaDesc *)chain;
    DmaDesc *lastDesc = (void *)0;

    arg = (1 << 31) | (fn << 28) | (addr << 9);
    if (bcnt == 1 && bsize <= 512)
        arg |= (bsize & 0x1ff);
    else
        arg |= ((1 << 27) | bcnt);

    num = bsize * bcnt;

    err = rt_sem_take(sdc->mutex, RT_WAITING_FOREVER);
    if (err != RT_EOK)
    {
        return ERRNORES;
    }

    SDIO_STATASTIC_INC(sdc->cmd53_rw_cnt);

    /* reset */
    synopmob_set_bits(base + CTRL, FIFO_RESET);  /* reset FIFO */
    while ((synopmob_read_register(base + CTRL) & FIFO_RESET) != 0)
        ;
    synopmob_set_register(base + RINTSTS, 0xffff);  /* clear interrupts */

    while (tmpDesc != 0)
    {
        /* make sure size is little than DescBuf1SizMsk */
        if (tmpDesc->desc1 > (DescBuf1SizMsk >> DescBuf1SizeShift))
        {
            /* TBD... fix me */
            rt_sem_release(sdc->mutex);
            return 0;
        }
        /* TBD... fix me, we must align tmpDesc->desc2 to 4 ? */

        tmpDesc->desc0 = DescOwnByDma | DescSecAddrChained;

        /* is it last node? */
        if (tmpDesc->desc3 == 0 || tmpDesc->desc3 == (unsigned int)chain)
        {
            tmpDesc->desc0 |= DescLastDesc;
            lastDesc = tmpDesc;
        }
        else
        {
            tmpDesc->desc0 |= DescDisInt;  /* disable interrupt... */
        }

        /* is it first node? */
        if ((char *)tmpDesc == (char *)chain)
        {
            tmpDesc->desc0 |= DescFirstDesc;
        }

        mmu_clean_dcache((rt_uint32_t)tmpDesc->desc2, (rt_uint32_t)tmpDesc->desc1);

        tmpDesc = (DmaDesc *)tmpDesc->desc3;
        chain_len += sizeof(buf_chain_t);

        if ((char *)tmpDesc == (char *)chain)
        {
            break;
        }
    }

    lastDesc->desc3 = (unsigned int)chain;

    /* FIXME, chain must be continuous arrry. */
    mmu_clean_dcache((rt_uint32_t)chain, (rt_uint32_t)chain_len);

    synopmob_set_register(base + DBADDR, (unsigned int)(chain));
    synopmob_set_register(base + BLKSIZ, bsize);
    synopmob_set_register(base + BYTCNT, num);
    synopmob_set_bits(base + BMOD, BMOD_DE | BMOD_BURST_INCR);

    synopmob_setclr_intmsk(base, 1/*set*/, INTMSK_DAT_ALL);

    SDIO_EXEC_CMD(base, cmd, arg);
    ret = rt_sem_take(sdc->sem, DMA_TRANSFER_TIMEOUT_TICKS);
    if (!ret)
    {
        ret = synopmob_wait_command_done(base, &rintst, INTMSK_CMD_DONE);
        if (!ret)
        {
            ret = synopmob_handle_standard_rinsts(rintst);
        }
        synopmob_set_register(base + RINTSTS, 0xffff);
    }
    else
    {
        ret = ERRIDMA;
    }

    if (ret) /*it will be cleared in interrupt when success.*/
    {
        synopmob_setclr_intmsk(base, 0/*clear*/, INTMSK_DAT_ALL);
    }

    synopmob_clear_bits(base + BMOD, BMOD_DE);

    if (!ret)
    {
        ret = synopmob_wait_data_ready(base);
    }

    rt_sem_release(sdc->mutex);

    if (ret)
    {
        rt_kprintf("sdio_drv_chain_write: failed with ret=%d,size=%d*%d\n", ret, bsize, bcnt);
        dumpchain((DmaDesc *)chain);
    }

    return ret;
}

#endif /*DONT_COPY_NET_PAYLOAD_TO_SEND*/
HSDC g_handle;
static unsigned short g_sdio_vendor = 0;
static unsigned short g_sdio_product = 0;
rt_int32_t sdio_read_cis(unsigned short *vendor, unsigned short *product)
{
    rt_int32_t ret;
    rt_uint32_t i, cisptr = 0;
    unsigned int data;
    unsigned int tpl_code, tpl_link;
    rt_uint32_t func0_num = 0, func1_num = 0;
    unsigned int *buf;
    HSDC handle = g_handle;

    func0_num = 0;
    func1_num = 0;
    for (i = 0; i < 3; i++)
    {
        ret = sdio_drv_creg_read(handle, SDIO_REG_FBR_BASE(func1_num) + SDIO_REG_FBR_CIS + i, func0_num, &data);
        if (ret)
            return ret;
        cisptr |= data << (i * 8);
    }
    do
    {
        ret = sdio_drv_creg_read(handle, cisptr++, func0_num, &tpl_code);
        if (ret)
            break;
        ret = sdio_drv_creg_read(handle, cisptr++, func0_num, &tpl_link);
        if (ret)
            break;
        if ((tpl_code == CISTPL_END) || (tpl_link == 0xff))
            break;

        if (tpl_code == CISTPL_NULL)
            continue;

        buf = rt_malloc(tpl_link * 4);
        if (!buf)
            return -RT_ENOMEM;

        for (i = 0; i < tpl_link; i++)
        {
            ret = sdio_drv_creg_read(handle, cisptr + i, func0_num, &buf[i]);
            if (ret)
                break;
        }
        if (ret)
        {
            rt_free(buf);
            break;
        }

        switch (tpl_code)
        {
        case CISTPL_MANFID:
            if (tpl_link < 4)
            {
                rt_kprintf("bad CISTPL_MANFID length\n");
                break;
            }
            g_sdio_vendor = buf[0];
            g_sdio_vendor |= buf[1] << 8;
            g_sdio_product = buf[2];
            g_sdio_product |= buf[3] << 8;
            *vendor = g_sdio_vendor;
            *product = g_sdio_product;
            rt_kprintf("SDIO Vendor %04x: Product %04x\n", g_sdio_vendor, g_sdio_product);
            break;
        case CISTPL_FUNCE:
            rt_kprintf("TODO: CISTPL_FUNCE size %u "
                    "type %u\n", tpl_link, buf[0]);

            break;
        case CISTPL_VERS_1:
            if (tpl_link < 2)
            {
                rt_kprintf("CISTPL_VERS_1 too short\n");
            }
            break;
        default:
            /* this tuple is unknown to the core */
            rt_kprintf("TODO: function %d, CIS tuple code %#x, length %d\n",
                func1_num, tpl_code, tpl_link);
            break;
        }

        cisptr += tpl_link;
    } while (1);

    return ret;
}

static int enum_sdio_card(sdc_t *sdc)
{
    int ret;
    unsigned int resp;
    unsigned int base = sdc->ip_base;
    unsigned int loop_count = 0;

    /* change the sampling edge*/
    synopmob_set_register(base + CTYPE, ONE_BIT_MODE);

    synopmob_set_register(base + CLKENA, 0x00000001); /*enable clock, non-low-power mode*/
    ret = synopmob_set_clk_freq(sdc, MMC_FOD_DIVIDER_VALUE);

    if (!ret)
    {
        udelay(1000);  /*enough for 74 clock.*/
        ret = sd_send_cmd0(sdc);
    }

    if (!ret)
    {
        udelay(100);  /*enough for 74 clock.*/
        ret = sd_send_cmd5(sdc, 0, &resp);
        if (!ret)
        {
            resp &= (MMC_VDD_32_33 | MMC_VDD_33_34);
            ret = sd_send_cmd5(sdc, resp, &resp);
        }
    }

    if (!ret)
    {
        /*BCM43455 will try many times....???*/
        while (1)
        {
            /* get RCA */
            ret = sd_send_cmd3(sdc);
            if (!ret || ++loop_count >= 500)
                break;

            udelay(10);
        }
    }

    if (!ret)
    {
        ret = sd_send_cmd7(sdc);  /* select the card */
    }

    if (ret)
    {
        rt_kprintf("enum_sdio_card: failed with ret=%d.\n", ret);
    }

    sdc->addr_inc_mode = 1; /*default address increase mode, now only marvell use fixed address mode...*/

    synopmob_set_bits(base + CTRL, INT_ENABLE | CTRL_USE_IDMAC);

    return ret;
}

static int resume_sdio_card(sdc_t *sdc)
{
    int ret;
    unsigned int base = sdc->ip_base;

    SDIO_DBG("resume_sdio_card, enter\n");

    synopmob_set_register(base + CTYPE, FOUR_BIT_MODE);

    synopmob_set_register(base + CLKENA, 0x00000001); /*enable clock, non-low-power mode*/
    ret = synopmob_set_clk_freq(sdc, 1/*25Mhz*/);

    udelay(2);

    /* sdc->rca = 1; */
    sdc->addr_inc_mode = 1;

    synopmob_set_bits(base + CTRL, INT_ENABLE | CTRL_USE_IDMAC);

    return ret;
}

int sdio_high_speed_mode(HSDC handle, int bitwidth, int freq)
{
    int ret;
    unsigned int resp;
    unsigned int divider = 1; /*frequency=50MHz/(2*divider), default use 25MHz.*/
                              /*0[50MHz] 1[25MHz] 2[12.5MHz] 3[8.3MHz] 4[6.25MHz] 5[5Mhz]*/
    sdc_t *sdc = (sdc_t *)handle;

    if (!sdc)
    {
        return -1;
    }

    if (bitwidth == 4)
    {
        synopmob_set_register(sdc->ip_base + CTYPE, FOUR_BIT_MODE);
    }

    if (freq == 50000000)
    {
        ret = sdio_drv_creg_read(handle, 0x13, 0, &resp);
        if (!ret && (resp & 1)/*support high speed*/)
        {
            if (!(resp & (1<<1)))
            {
                resp |= (1<<1);
                sdio_drv_creg_write(handle, 0x13, 0, (unsigned char)resp, &resp);
            }
            divider = 0;
        }
    }

    ret = synopmob_set_clk_freq(sdc, divider);
    if (ret != 0)
    {
        rt_kprintf("sdio_high_speed_mode: fail with ret=%d\n", ret);
    }

    return ret;
}

/*FIXME, remove it later...*/
void fh_mmc_reset_by_id(int which)
{
    rt_uint32_t ret = 0;
    struct clk *set, *get;
    char *clkname = "sdc0_clk2x";
    char *clk_drv = "sdc0_clk_drv";
    char *clk_sample = "sdc0_clk_sample";
    rt_uint32_t rate_clk_div = 50000000;

    fh_pmu_sdc_reset(which);
    if (which != 0) /*SD1*/
    {
        clkname = "sdc1_clk2x";
        clk_drv = "sdc1_clk_drv";
        clk_sample = "sdc1_clk_sample";
    }

    set = clk_get(NULL, clkname);
    if (!set)
        rt_kprintf("clk_get(%s) fail!\n", clkname);
    /* sdio控制器的分频系数 */
    if (set)
    {
        ret = clk_set_rate(set, rate_clk_div);
        if (ret == 0)
        {
            get = clk_get(NULL, clkname);
            if ((get != RT_NULL) && (get->clk_out_rate == rate_clk_div))
                rt_kprintf("set %s clk rate :%d, success!\r\n", get->name, get->clk_out_rate);
            else
                rt_kprintf("set %s clk rate fail:%d!=%d!\r\n", set->name, set->clk_out_rate, rate_clk_div);
        }
    }

    set = clk_get(NULL, clk_drv);
    if (set)
#ifdef CONFIG_CHIP_FH8626V100
        fh_clk_set_phase(set, 0x2);/* now drv fixed to 180 */
#else
        fh_clk_set_phase(set, 0x8);/* now drv fixed to 180 */
#endif
    else
        rt_kprintf("clk_get(%s) fail!\n", clk_drv);
    set = clk_get(NULL, clk_sample);
    if (set)
        fh_clk_set_phase(set, 0x0);
    else
        rt_kprintf("clk_get(%s) fail!\n", clk_sample);
}

static int common_init(unsigned int which, unsigned int sdio,
        unsigned int wkmod, unsigned int *dma_desc,
        unsigned int resume, HSDC *phandle)
{
    int ret = ERRNORES;
    sdc_t *sdc = g_sdc_hdl[which];
    unsigned int base;
    unsigned int fifo_depth;
    unsigned int tmp_reg;
    int sd_irq[] = {SDC0_IRQn, SDC1_IRQn};
    unsigned int basex[] =  {SDC0_REG_BASE, SDC1_REG_BASE};
    rt_sem_t sem;
    rt_sem_t mutex;

    if (which > 1)
    {
        return ret;
    }

    if (!sdc)
    {
        sdc = rt_malloc(sizeof(*sdc));
        if (!sdc)
        {
            return ret;
        }
        rt_memset(sdc, 0, sizeof(*sdc));
        g_sdc_hdl[which] = sdc;
    }

    /*enable sdc's input clock*/
    if (1)
    {
        struct clk *clk;
        char *clkname = "sdc0_clk2x";

        if (which == 1)
        {
            clkname = "sdc1_clk2x";
        }
        clk = clk_get(NULL, clkname);
        if (clk)
        {
            clk_enable(clk);
        }
    }

    /*install interrupt here...*/
    rt_hw_interrupt_install(sd_irq[which], OSSDCISR, (void *)sdc, "SDIO");
    rt_hw_interrupt_umask(sd_irq[which]);

    /*reset controller, adjust clock phase...*/
    fh_mmc_reset_by_id(which);

    base = basex[which];

    *phandle = (HSDC)0;

    sem   = sdc->sem;
    mutex = sdc->mutex;
    memset((void *)sdc, 0, sizeof(*sdc));

    sdc->sem = sem;
    sdc->mutex = mutex;

    sdc->wkmod        = wkmod;
    sdc->ip_base      = base;
    sdc->rca          = 0;
    if (!sem)
    {
        sem = rt_sem_create("sdio_sem", 0, RT_IPC_FLAG_PRIO);
        if (!sem)
        {
            return ret;
        }
        sdc->sem = sem;
    }

    if (!mutex)
    {
        mutex = rt_sem_create("sdio_mutex", 1, RT_IPC_FLAG_PRIO);
        if (!mutex)
        {
            return ret;
        }
        sdc->mutex = mutex;
    }

#ifdef FH_SDIO_IDMA_CHAIN
    if (NULL == sdc->descriptors)
    {
        sdc->descriptors = (MMC_DMA_Descriptors *)fh_dma_mem_malloc_align(
            (sizeof(MMC_DMA_Descriptors)) * FH_SDIO_DESC_NUM_MAX, 128, "fh_sdio_idma");/* ?aligned to 128 */
        if (NULL == sdc->descriptors)
        {
            rt_kprintf("%s-%d sdc->descriptors malloc fail\n", __func__, __LINE__);
            return -1;
        }
        rt_memset(sdc->descriptors, 0, (sizeof(MMC_DMA_Descriptors)) * FH_SDIO_DESC_NUM_MAX);
    }
#endif

    synopmob_set_bits(base + CTRL, CTRL_RESET);  /* reset host controller */
    udelay(100);

    synopmob_set_bits(base + CTRL, DMA_RESET);
    udelay(100);
    synopmob_set_bits(base + CTRL, FIFO_RESET);
    udelay(100);
    synopmob_set_bits(base + BMOD, BMOD_SWR);
    udelay(100);

    synopmob_set_register(base + CTYPE, ONE_BIT_MODE);

    synopmob_set_register(base + RINTSTS, 0xffffffff);  /* clear interrupt. */
    synopmob_clear_bits(base + CTRL, INT_ENABLE);
    synopmob_set_register(base + INTMSK, 0);             /* mask all INTR */

    /* Set Data and Response timeout to Maximum Value */
    synopmob_set_register(base + TMOUT, 0xffffffff);

    /* Set the card Debounce to allow the CDETECT fluctuations to settle down*/
    synopmob_set_register(base + DEBNCE, 0x0FFFFF);

    /* Set FIFOTH*/
    fifo_depth = (synopmob_read_register(base + FIFOTH) >> 16) & 0xfff;
    if (fifo_depth > 15)
        tmp_reg = 0x5 << 28;
    else
        tmp_reg = 0x2 << 28;
    tmp_reg |= (((fifo_depth / 2) << 16) | (((fifo_depth / 2) + 1) << 0));
    synopmob_set_register(base + FIFOTH, tmp_reg);

    if (!sdio)
    {
        /*ret = enum_sd_card(sdc);*/
    }
    else
    {
        if (!resume)
        {
            ret = enum_sdio_card(sdc);
        }
        else
        {
            ret = resume_sdio_card(sdc);
        }
    }
    if (!ret)
    {
        *phandle = (HSDC)sdc;
    }
    g_handle = (HSDC)sdc;
    return ret;
}


int sdio_init(unsigned int which, unsigned int wkmod, unsigned int *dma_desc, HSDC *phandle)
{
    return common_init(which, 1, wkmod, dma_desc, 0,  phandle);
}

int sdio_set_cmd53_addr_mode(HSDC handle, int fixed_addr)
{
    sdc_t *sdc = (sdc_t *)handle;

    sdc->addr_inc_mode = !fixed_addr;

    return 0;
}

int sdio_resume(unsigned int which, unsigned int wkmod,
        unsigned int *dma_desc /*4Byte aligned,16 bytes space*/,
        HSDC *phandle)
{
    return common_init(which, 1, wkmod, dma_desc, 1, phandle);
}

int sdio_enable_card_int(HSDC handle, int enable)
{
    if (handle)
    {
        synopmob_setclr_intmsk(((sdc_t *)handle)->ip_base, enable, INTMSK_SDIO);
    }
    return 0;
}

int sdio_set_card_int_cb(HSDC handle, void (*cb)(void))
{
    if (handle)
    {
        ((sdc_t *)handle)->cb = cb;
    }

    return 0;
}

static void OSSDCISR(int vector, void *param)
{
    unsigned int sts;
    unsigned int base;
    sdc_t *sdc = (sdc_t *)param;

    base = sdc->ip_base;

    sts = synopmob_read_register(base + MINTSTS);
    if (sts & INTMSK_SDIO)
    {
        synopmob_clear_bits(base + INTMSK, INTMSK_SDIO);
        synopmob_set_register(base + RINTSTS, INTMSK_SDIO);
        synopmob_set_register(base + MINTSTS, INTMSK_SDIO);
        if (sdc->cb)
        {
            sdc->cb();
        }
    }

    if (sts & INTMSK_DAT_ALL)
    {
        synopmob_clear_bits(base + INTMSK, INTMSK_DAT_ALL);
        rt_sem_release(sdc->sem);
    }
}

void fh_sdio0_init(void)
{
}
void fh_sdio1_init(void)
{
}
void fh_sdio_init(void)
{
}
void fh_sdio_init_by_id(int id)
{
}

int sdc_deinit(HSDC handle)
{
    return -1;
}

unsigned int get_sdio_info(void)
{
    int ret;
    HSDC sdio_handle;
    int bitwidth = 4;
    int sdio_id = 0;
    unsigned int info = 0;
    unsigned short vendor = 0;
    unsigned short product = 0;
#ifdef WIFI_SDIO
    sdio_id = WIFI_SDIO;
    bitwidth = 4;
    #ifdef USING_1BIT_MODE
    bitwidth = 1;
    #endif
#endif
    if (!g_handle)
    {
        ret = sdio_init(sdio_id, bitwidth, NULL, &sdio_handle);
        if (ret)
        {
            rt_kprintf("%s-%d sdio_init, ret %d\r\n", __func__, __LINE__, ret);
            return ret;
        }
    }
    ret = sdio_read_cis(&vendor, &product);
    if (ret)
    {
        rt_kprintf("%s-%d sdio_read_cis, ret %d\r\n", __func__, __LINE__, ret);
    }
    if (!g_handle)
    {
        ret = sdio_resume(sdio_id, bitwidth, NULL, sdio_handle);
        if (ret)
        {
            rt_kprintf("%s-%d sdio_resume, ret %d\r\n", __func__, __LINE__, ret);
            return ret;
        }
    }
    info = vendor << 16 | product;
    return info;
}

MSH_CMD_EXPORT(get_sdio_info, get_sdio_info .....);
