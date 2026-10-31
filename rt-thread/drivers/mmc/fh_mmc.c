/**
 * Copyright (c) 2015-2019 Shanghai Fullhan Microelectronics Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 *
 * Change Logs:
 * Date           Author       Notes
 * 2019-04-12     tangyh     the first version
 *
 */

// #include "inc/fh_driverlib.h"
#include <rtthread.h>
// #include <fh_def.h>
#include <fh_mmc.h>
#include "fh_clock.h"
#include <calibrate.h>
#include <unistd.h>

#define RETRY_COUNT 100
#define MMC_DESC_READBACK
/* *1: card off */
/* *0: card on */
inline rt_uint32_t MMC_GetCardStatus(struct fh_mmc_obj *mmc_obj)
{
    rt_uint32_t card_status = GET_REG(mmc_obj->base + OFFSET_SDC_CDETECT);

    return card_status & 0x1;
}

inline void MMC_StartDma(struct fh_mmc_obj *mmc_obj)
{
    rt_uint32_t reg;

    SET_REG(mmc_obj->base + OFFSET_SDC_DBADDR,
            (rt_uint32_t)mmc_obj->descriptors);
    reg = GET_REG(mmc_obj->base + OFFSET_SDC_BMOD);
    reg |= 1 << 7;
    reg |= MMC_BMOD_BURST_INCR;
    SET_REG(mmc_obj->base + OFFSET_SDC_BMOD, reg);
}

inline void MMC_StopDma(struct fh_mmc_obj *mmc_obj)
{
    rt_uint32_t reg;

    reg = GET_REG(mmc_obj->base + OFFSET_SDC_BMOD);
    reg &= ~(1 << 7);
    reg |= MMC_BMOD_RESET;
    SET_REG(mmc_obj->base + OFFSET_SDC_BMOD, reg);
}

volatile unsigned int mmc_desc0_readback;
void MMC_InitDescriptors(struct fh_mmc_obj *mmc_obj, rt_uint32_t *buf,
                         rt_uint32_t size)
{
    MMC_DMA_Descriptors *desc;
    rt_uint32_t len = 0;
    int desc_cnt = 0;

    desc = mmc_obj->descriptors;

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
            (rt_uint32_t)mmc_obj->descriptors +
            (desc_cnt + 1) * sizeof(MMC_DMA_Descriptors);

        size -= desc[desc_cnt].desc1.bit.buffer1_size;
        len += desc[desc_cnt].desc1.bit.buffer1_size;
#ifdef MMC_DESC_READBACK
        mmc_desc0_readback = *(volatile unsigned int *)&desc[desc_cnt].desc0; /* readback desc0 of each desc */
#endif
        desc_cnt++;
    }

    desc[0].desc0.bit.first_descriptor           = 1;
    desc[desc_cnt - 1].desc0.bit.last_descriptor = 1;
    desc[desc_cnt - 1].desc3.bit.buffer_addr1    = 0;
#ifdef MMC_DESC_READBACK
    mmc_desc0_readback = *(volatile unsigned int *)&desc[desc_cnt - 1].desc0; /* readback desc0 of final desc again */
#endif
}

inline rt_uint32_t MMC_GetWaterlevel(struct fh_mmc_obj *mmc_obj)
{
    return (GET_REG(mmc_obj->base + OFFSET_SDC_STATUS) >> 17) & 0x1fff;
}

inline rt_uint32_t MMC_GetStatus(struct fh_mmc_obj *mmc_obj)
{
    return GET_REG(mmc_obj->base + OFFSET_SDC_STATUS);
}

inline rt_uint32_t MMC_GetRawInterrupt(struct fh_mmc_obj *mmc_obj)
{
    return GET_REG(mmc_obj->base + OFFSET_SDC_RINTSTS);
}

inline rt_uint32_t MMC_GetUnmaskedInterrupt(struct fh_mmc_obj *mmc_obj)
{
    return GET_REG(mmc_obj->base + OFFSET_SDC_MINTSTS);
}

inline rt_uint32_t MMC_ClearRawInterrupt(struct fh_mmc_obj *mmc_obj,
                                         rt_uint32_t interrupts)
{
    return SET_REG(mmc_obj->base + OFFSET_SDC_RINTSTS, interrupts);
}

inline rt_uint32_t MMC_GetInterruptEnable(struct fh_mmc_obj *mmc_obj)
{
    return GET_REG(mmc_obj->base + OFFSET_SDC_INTMASK);
}

inline rt_uint32_t MMC_SetInterruptEnable(struct fh_mmc_obj *mmc_obj,
                                        rt_uint32_t mask)
{
    return SET_REG(mmc_obj->base + OFFSET_SDC_INTMASK, mask);
}

inline void MMC_SetByteCount(struct fh_mmc_obj *mmc_obj, rt_uint32_t bytes)
{
    SET_REG(mmc_obj->base + OFFSET_SDC_BYTCNT, bytes);
}

inline void MMC_SetBlockSize(struct fh_mmc_obj *mmc_obj, rt_uint32_t size)
{
    SET_REG(mmc_obj->base + OFFSET_SDC_BLKSIZ, size);
}

inline rt_uint32_t MMC_GetResponse(struct fh_mmc_obj *mmc_obj, int resp_num)
{
    return GET_REG(mmc_obj->base + OFFSET_SDC_RESP0 + resp_num * 4);
}

inline rt_uint32_t MMC_IsFifoEmpty(struct fh_mmc_obj *mmc_obj)
{
    return (GET_REG(mmc_obj->base + OFFSET_SDC_STATUS) >> 2) & 0x1;
}

inline rt_uint32_t MMC_IsDataStateBusy(struct fh_mmc_obj *mmc_obj)
{
    return (GET_REG(mmc_obj->base + OFFSET_SDC_STATUS) >> 10) & 0x1;
}

rt_uint32_t MMC_GetIdmaStete(struct fh_mmc_obj *mmc_obj)
{
    return GET_REG(mmc_obj->base + OFFSET_SDC_IDSTS);
}

rt_uint32_t MMC_GetCmd(struct fh_mmc_obj *mmc_obj)
{
    return GET_REG(mmc_obj->base + OFFSET_SDC_CMD);
}


int MMC_WriteData(struct fh_mmc_obj *mmc_obj, rt_uint32_t *buf,
                  rt_uint32_t size)
{
    int fifo_available, i, retries;

    for (i = 0; i < size / 4; i++)
    {
        retries = 0;
        do
        {
            fifo_available = mmc_obj->fifo_depth - MMC_GetWaterlevel(mmc_obj);
            if (retries++ > 10000)
            {
                rt_kprintf("ERROR: %s, get water level timeout\n", __func__);
                return -RT_ETIMEOUT;
            }
        } while (!fifo_available);
        SET_REG(mmc_obj->base + OFFSET_SDC_FIFO, *buf++);
    }

    retries = 0;
    while (MMC_IsDataStateBusy(mmc_obj))
    {
        if (retries++ > 10000)
        {
            rt_kprintf("ERROR: %s, timeout, data line keep being busy\n",
                       __func__);
            return -RT_ETIMEOUT;
        }
    }

    return 0;
}
int MMC_ReadData(struct fh_mmc_obj *mmc_obj, rt_uint32_t *buf, rt_uint32_t size)
{
    int fifo_available, i, retries;

    for (i = 0; i < size / 4; i++)
    {
        retries = 0;
        do
        {
            fifo_available = MMC_GetWaterlevel(mmc_obj);
            if (retries++ > 10000)
            {
                rt_kprintf("ERROR: %s, get water level timeout\n", __func__);
                return -RT_ETIMEOUT;
            }
        } while (!fifo_available);

        *buf++ = GET_REG(mmc_obj->base + OFFSET_SDC_FIFO);
    }

    retries = 0;
    while (MMC_IsDataStateBusy(mmc_obj))
    {
        if (retries++ > 10000)
        {
            rt_kprintf("ERROR: %s, timeout, data line keep being busy\n",
                       __func__);
            return -RT_ETIMEOUT;
        }
    }

    return 0;
}

int MMC_UpdateClockRegister(struct fh_mmc_obj *mmc_obj, int div)
{
    rt_uint32_t wait_retry_count = 0;
    rt_uint32_t reg;


    /* disable clock */
    reg = GET_REG(mmc_obj->base + OFFSET_SDC_CLKENA);
    reg &= 0x00000000;
    SET_REG(mmc_obj->base + OFFSET_SDC_CLKENA, reg);
    SET_REG(mmc_obj->base + OFFSET_SDC_CLKSRC, 0);

    /* inform CIU */
    SET_REG(mmc_obj->base + OFFSET_SDC_CMD, 1 << 31 | 1 << 21);
    while (GET_REG(mmc_obj->base + OFFSET_SDC_CMD) & 0x80000000)
    {
        wait_retry_count++;
        if (wait_retry_count > RETRY_COUNT)
        {
            rt_kprintf("ERROR: %s, update clock timeout0\n", __func__);
            return -RT_ETIMEOUT;
        }
        udelay(100);
    }

    /* set clock to desired speed */
    SET_REG(mmc_obj->base + OFFSET_SDC_CLKDIV, div);

    /* inform CIU */
    SET_REG(mmc_obj->base + OFFSET_SDC_CMD, 1 << 31 | 1 << 21);

    wait_retry_count = 0;
    while (GET_REG(mmc_obj->base + OFFSET_SDC_CMD) & 0x80000000)
    {
        wait_retry_count++;
        if (wait_retry_count > RETRY_COUNT)
        {
            rt_kprintf("ERROR: %s, update clock timeout1\n", __func__);
            return -RT_ETIMEOUT;
        }
        udelay(100);
    }

    /* enable clock */
    reg = GET_REG(mmc_obj->base + OFFSET_SDC_CLKENA);
    reg |= (1 << 0) | (1 << 16);
    SET_REG(mmc_obj->base + OFFSET_SDC_CLKENA, reg);

    /* inform CIU */
    SET_REG(mmc_obj->base + OFFSET_SDC_CMD, 1 << 31 | 1 << 21);

    wait_retry_count = 0;

    while (GET_REG(mmc_obj->base + OFFSET_SDC_CMD) & 0x80000000)
    {
        wait_retry_count++;
        if (wait_retry_count > RETRY_COUNT)
        {
            rt_kprintf("ERROR: %s, update clock timeout2\n", __func__);
            return -RT_ETIMEOUT;
        }
        udelay(100);
    }

    return 0;
}

int MMC_SetCardWidth(struct fh_mmc_obj *mmc_obj, int width)
{
    switch (width)
    {
    case MMC_CARD_WIDTH_1BIT:
        SET_REG(mmc_obj->base + OFFSET_SDC_CTYPE, 0);
        break;
    case MMC_CARD_WIDTH_4BIT:
        SET_REG(mmc_obj->base + OFFSET_SDC_CTYPE, 1);
        break;
    default:
        rt_kprintf("ERROR: %s, card width %d is not supported\n", __func__,
                   width);
        return -RT_ERROR;
        break;
    }
    return 0;
}

int MMC_SendCommand(struct fh_mmc_obj *mmc_obj, rt_uint32_t cmd,
                    rt_uint32_t arg, rt_uint32_t flags)
{
    rt_uint32_t reg_data = 0;
    rt_uint32_t wait_retry_count = 0;

    SET_REG(mmc_obj->base + OFFSET_SDC_RINTSTS, MMC_INT_STATUS_DATA);
    SET_REG(mmc_obj->base + OFFSET_SDC_CMDARG, arg);
    flags |= 1 << 31 | 1 << 29 | cmd;

    SET_REG(mmc_obj->base + OFFSET_SDC_CMD, flags);

    while (GET_REG(mmc_obj->base + OFFSET_SDC_CMD) & MMC_CMD_START_CMD)
    {
        reg_data = GET_REG(mmc_obj->base + OFFSET_SDC_RINTSTS);
        if (reg_data & MMC_INT_STATUS_HARDWARE_LOCKED)
        {
            reg_data &= (~MMC_INT_STATUS_SDIO);
            SET_REG(mmc_obj->base + OFFSET_SDC_RINTSTS, reg_data);
            rt_kprintf("Other CMD is running,please operate cmd again!");
            return 1;
        }
        wait_retry_count++;
        if (wait_retry_count > RETRY_COUNT)
        {
            rt_kprintf("ERROR: %s, send cmd timeout\n", __func__);
            return -RT_ETIMEOUT;
        }
        rt_udelay(100);
    }

    /* fixme: check HLE_INT_STATUS */
    return 0;
}

int MMC_ResetFifo(struct fh_mmc_obj *mmc_obj)
{
    rt_uint32_t reg;
    rt_uint32_t wait_retry_count = 0;

    reg = GET_REG(mmc_obj->base + OFFSET_SDC_CTRL);
    reg |= 1 << 1;
    SET_REG(mmc_obj->base + OFFSET_SDC_CTRL, reg);

    /* wait until fifo reset finish */
    while (GET_REG(mmc_obj->base + OFFSET_SDC_CTRL) & MMC_CTRL_FIFO_RESET)
    {
        wait_retry_count++;
        if (wait_retry_count > RETRY_COUNT)
        {
            rt_kprintf("ERROR: %s, FIFO reset timeout\n", __func__);
            return -RT_ETIMEOUT;
        }
    }

    return 0;
}

int MMC_Reset(struct fh_mmc_obj *mmc_obj)
{
    rt_uint32_t reg;
    rt_uint32_t wait_retry_count = 0;

    reg = GET_REG(mmc_obj->base + OFFSET_SDC_BMOD);
    reg |= MMC_BMOD_RESET;
    SET_REG(mmc_obj->base + OFFSET_SDC_BMOD, reg);

    while (GET_REG(mmc_obj->base + OFFSET_SDC_BMOD) & MMC_BMOD_RESET)
    {
        wait_retry_count++;
        if (wait_retry_count > RETRY_COUNT)
        {
            rt_kprintf("ERROR: %s, BMOD Software reset timeout\n", __func__);
            return -RT_ETIMEOUT;
        }
    }

    reg = GET_REG(mmc_obj->base + OFFSET_SDC_CTRL);
    reg |= MMC_CTRL_CONTROLLER_RESET | MMC_CTRL_FIFO_RESET | MMC_CTRL_DMA_RESET;
    SET_REG(mmc_obj->base + OFFSET_SDC_CTRL, reg);

    wait_retry_count = 0;
    while (
        GET_REG(mmc_obj->base + OFFSET_SDC_CTRL) &
        (MMC_CTRL_CONTROLLER_RESET | MMC_CTRL_FIFO_RESET | MMC_CTRL_DMA_RESET))
    {
        wait_retry_count++;
        if (wait_retry_count > RETRY_COUNT)
        {
            rt_kprintf("ERROR: %s, CTRL dma|fifo|ctrl reset timeout\n",
                       __func__);
            return -RT_ETIMEOUT;
        }
    }
    return 0;
}

void MMC_Init(struct fh_mmc_obj *mmc_obj)
{
    rt_uint32_t reg;
    struct clk *clk = RT_NULL;

    if (mmc_obj->id == 0)
        clk = clk_get(NULL, "sdc0_clk2x");
    else
        clk = clk_get(NULL, "sdc1_clk2x");

    if (clk == RT_NULL)
    {
        rt_kprintf("can not find sdc0_clk2x continue\n");
        mmc_obj->sdio_hz = 50000000;
    }
    else
    {
        clk_enable(clk);
        mmc_obj->sdio_hz = clk_get_rate(clk);
    }

    if (mmc_obj->mmc_reset)
        mmc_obj->mmc_reset(mmc_obj);

    MMC_Reset(mmc_obj);

    /* fixed burst */
    reg = GET_REG(mmc_obj->base + OFFSET_SDC_BMOD);
    reg |= 1 << 1;
    SET_REG(mmc_obj->base + OFFSET_SDC_BMOD, reg);

    /* fixme: power on ? ctrl by gpio ? */

    MMC_ClearRawInterrupt(mmc_obj, MMC_INT_STATUS_ALL);
    MMC_SetInterruptEnable(mmc_obj, 0x0);

    /* fixme: use_internal_dma */
    reg = GET_REG(mmc_obj->base + OFFSET_SDC_CTRL);
    reg |= MMC_CTRL_INT_ENABLE;
#ifdef MMC_USE_DMA
    reg |= MMC_CTRL_USE_DMA;
#endif
    SET_REG(mmc_obj->base + OFFSET_SDC_CTRL, reg);

    /* set timeout param */
    SET_REG(mmc_obj->base + OFFSET_SDC_TMOUT, 0xffffffff);

    /* set fifo */
#if defined(CONFIG_ARCH_FH8862) || defined(CONFIG_ARCH_FH885xV500) || defined(CONFIG_ARCH_MC632X) || defined(CONFIG_ARCH_FH8626V300)
    mmc_obj->fifo_depth = 128;
#else
    reg = GET_REG(mmc_obj->base + OFFSET_SDC_FIFOTH);
    reg = (reg >> 16) & 0xfff;
    mmc_obj->fifo_depth = reg + 1;
#endif

    reg = ((0x2 << 28) | ((mmc_obj->fifo_depth / 2) << 16) | ((mmc_obj->fifo_depth / 2 + 1) << 0));
    SET_REG(mmc_obj->base + OFFSET_SDC_FIFOTH, reg);
}
