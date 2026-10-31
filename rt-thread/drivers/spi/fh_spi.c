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

#include <rtthread.h>
#include "fh_def.h"
#include "fh_arch.h"
#include "fh_spi.h"
// #include "inc/fh_driverlib.h"

void SPI_EnableSlaveen(struct fh_spi_obj *spi_obj, rt_uint32_t port)
{
    rt_uint32_t reg;

    reg = GET_REG(spi_obj->base + OFFSET_SPI_SER);
    reg |= (1 << port);
    SET_REG(spi_obj->base + OFFSET_SPI_SER, reg);
}

void SPI_DisableSlaveen(struct fh_spi_obj *spi_obj, rt_uint32_t port)
{
    rt_uint32_t reg;

    reg = GET_REG(spi_obj->base + OFFSET_SPI_SER);
    reg &= ~(1 << port);
    SET_REG(spi_obj->base + OFFSET_SPI_SER, reg);
}

void SPI_SetTxLevel(struct fh_spi_obj *spi_obj, rt_uint32_t level)
{
    SET_REG(spi_obj->base + OFFSET_SPI_TXFTLR, level);
}

void SPI_SetRxLevel(struct fh_spi_obj *spi_obj, rt_uint32_t level)
{
    SET_REG(spi_obj->base + OFFSET_SPI_RXFTLR, level);
}

rt_uint32_t SPI_GetTxLevel(struct fh_spi_obj *spi_obj)
{
    return GET_REG(spi_obj->base + OFFSET_SPI_TXFTLR);
}

rt_uint32_t SPI_GetRxLevel(struct fh_spi_obj *spi_obj)
{
    return GET_REG(spi_obj->base + OFFSET_SPI_RXFTLR);
}

void SPI_EnableInterrupt(struct fh_spi_obj *spi_obj, rt_uint32_t flag)
{
    rt_uint32_t reg;

    reg = GET_REG(spi_obj->base + OFFSET_SPI_IMR);
    reg |= flag;
    SET_REG(spi_obj->base + OFFSET_SPI_IMR, reg);
}

void SPI_EnableDma(struct fh_spi_obj *spi_obj, rt_uint32_t channel)
{
    rt_uint32_t reg;

    reg = GET_REG(spi_obj->base + OFFSET_SPI_DMACTRL);
    reg |= channel;
    SET_REG(spi_obj->base + OFFSET_SPI_DMACTRL, reg);
}

void SPI_DisableDma(struct fh_spi_obj *spi_obj, rt_uint32_t channel)
{
    rt_uint32_t reg;

    reg = GET_REG(spi_obj->base + OFFSET_SPI_DMACTRL);
    reg &= ~channel;
    SET_REG(spi_obj->base + OFFSET_SPI_DMACTRL, reg);
}

void SPI_DisableInterrupt(struct fh_spi_obj *spi_obj, rt_uint32_t flag)
{
    rt_uint32_t reg;

    reg = GET_REG(spi_obj->base + OFFSET_SPI_IMR);
    reg &= ~flag;
    SET_REG(spi_obj->base + OFFSET_SPI_IMR, reg);
}

rt_uint32_t SPI_InterruptStatus(struct fh_spi_obj *spi_obj)
{
    return GET_REG(spi_obj->base + OFFSET_SPI_ISR);
}

void SPI_ClearInterrupt(struct fh_spi_obj *spi_obj)
{
    GET_REG(spi_obj->base + OFFSET_SPI_ICR);
}

rt_uint32_t SPI_ReadTxFifoLevel(struct fh_spi_obj *spi_obj)
{
    return GET_REG(spi_obj->base + OFFSET_SPI_TXFLR);
}

rt_uint32_t SPI_ReadRxFifoLevel(struct fh_spi_obj *spi_obj)
{
    return GET_REG(spi_obj->base + OFFSET_SPI_RXFLR);
}

UINT8 SPI_ReadData(struct fh_spi_obj *spi_obj)
{
    return GET_REG(spi_obj->base + OFFSET_SPI_DR) & 0xff;
}

unsigned short SPI_ReadData_16(struct fh_spi_obj *spi_obj)
{
    return GET_REG(spi_obj->base + OFFSET_SPI_DR);
}

unsigned int SPI_ReadData_32(struct fh_spi_obj *spi_obj)
{
    return GET_REG(spi_obj->base + OFFSET_SPI_DR);
}

void SPI_WriteData(struct fh_spi_obj *spi_obj, UINT32 data)
{
    SET_REG(spi_obj->base + OFFSET_SPI_DR, data);
}

rt_uint32_t SPI_ReadStatus(struct fh_spi_obj *spi_obj)
{
    return GET_REG(spi_obj->base + OFFSET_SPI_SR);
}

void SPI_Enable(struct fh_spi_obj *spi_obj, int enable)
{
    SET_REG(spi_obj->base + OFFSET_SPI_SSIENR, enable);
}

void SPI_WriteTxDmaLevel(struct fh_spi_obj *spi_obj, rt_uint32_t data)
{
    SET_REG(spi_obj->base + OFFSET_SPI_DMATDL, data);
}

void SPI_WriteRxDmaLevel(struct fh_spi_obj *spi_obj, rt_uint32_t data)
{
    SET_REG(spi_obj->base + OFFSET_SPI_DMARDL, data);
}

void SPI_SampleDly(struct fh_spi_obj *spi_obj, int delay)
{
    SET_REG(spi_obj->base + OFFSET_SPI_SAMPLE_DELAY, delay);
}

rt_uint32_t SPI_RawInterruptStatus(struct fh_spi_obj *spi_obj)
{
    return GET_REG(spi_obj->base + OFFSET_SPI_RISR);
}

void SPI_SetParameter(struct fh_spi_obj *spi_obj)
{
    rt_uint32_t reg;
    struct spi_config *config;

    config = &spi_obj->config;

    SET_REG(spi_obj->base + OFFSET_SPI_BAUD, config->clk_div);

    reg = GET_REG(spi_obj->base + OFFSET_SPI_CTRL0);

    reg &= ~(0x3ff);
    reg |= config->data_size | config->frame_format | config->clk_phase |
           config->clk_polarity | config->transfer_mode;

    SET_REG(spi_obj->base + OFFSET_SPI_CTRL0, reg);
}

void SPI_ContinueReadNum(struct fh_spi_obj *spi_obj,rt_uint32_t num)
{
    SET_REG(spi_obj->base + OFFSET_SPI_CTRL1, (num - 1));
}


void SPI_SetTransferMode(struct fh_spi_obj *spi_obj, rt_uint32_t mode)
{
    rt_uint32_t reg;

    reg = GET_REG(spi_obj->base + OFFSET_SPI_CTRL0);
    reg &= ~(rt_uint32_t) SPI_TRANSFER_MODE_RANGE;
    reg |= mode;
    SET_REG(spi_obj->base + OFFSET_SPI_CTRL0, reg);

}
/*spic new add code below..*/

static void reg_bit_process(rt_uint32_t *data, rt_uint32_t value, rt_uint32_t mask) {
    (*data) &= ~mask;
    (*data) |= value;
}

rt_int32_t Spi_SetApbReadWireMode(struct fh_spi_obj *spi_obj, spi_read_wire_mode_e mode)
{

    rt_uint32_t data = GET_REG(spi_obj->base + OFFSET_SPI_CCFGR_OFFSET);
    switch (mode)
    {
    case FH_STANDARD_READ:
        reg_bit_process(&data, 0 << 8, 7 << 8);
        reg_bit_process(&data, 0 << 2, 3 << 2);
        reg_bit_process(&data, 0 << 0, 3 << 0);
        reg_bit_process(&data, 0 << 4, 7 << 4);
        break;

    case FH_DUAL_OUTPUT:
        reg_bit_process(&data, 0 << 8, 7 << 8);
        reg_bit_process(&data, 3 << 2, 3 << 2);
        reg_bit_process(&data, 1 << 0, 3 << 0);
        reg_bit_process(&data, 0 << 4, 7 << 4);
        break;

    case FH_QUAD_OUTPUT:
        reg_bit_process(&data, 0 << 8, 7 << 8);
        reg_bit_process(&data, 3 << 2, 3 << 2);
        reg_bit_process(&data, 2 << 0, 3 << 0);
        reg_bit_process(&data, 0 << 4, 7 << 4);
        break;

    default:
        rt_kprintf("wrong mode now....\n");
    }
    data |= 1<<13;
    SET_REG(spi_obj->base + OFFSET_SPI_CCFGR_OFFSET, data);
    return 0;
}

rt_int32_t Spi_SetApbWriteWireMode(struct fh_spi_obj *spi_obj, spi_prog_wire_mode_e mode)
{

    rt_uint32_t data = GET_REG(spi_obj->base + OFFSET_SPI_CCFGR_OFFSET);

    switch (mode)
    {
    case STANDARD_PROG:
        reg_bit_process(&data, 0 << 8, 7 << 8);
        reg_bit_process(&data, 4 << 4, 7 << 4);
        reg_bit_process(&data, 0 << 2, 3 << 2);
        reg_bit_process(&data, 0 << 0, 3 << 0);
        break;
    case QUAD_INPUT:
        reg_bit_process(&data, 0 << 8, 7 << 8);
        reg_bit_process(&data, 4 << 4, 7 << 4);
        reg_bit_process(&data, 1 << 2, 3 << 2);
        reg_bit_process(&data, 2 << 0, 3 << 0);
        break;
    }
    SET_REG(spi_obj->base + OFFSET_SPI_CCFGR_OFFSET, data);
    return 0;
}


rt_int32_t Spi_SetAhbReadWireMode(struct fh_spi_obj *spi_obj, spi_read_wire_mode_e mode)
{


    rt_uint32_t data = GET_REG(spi_obj->base + OFFSET_SPI_CCFGR_OFFSET);
    rt_uint32_t data1 = GET_REG(spi_obj->base + OFFSET_SPI_OPCR_OFFSET);

    reg_bit_process(&data1, 1 << 18, 1 << 18);

    switch (mode)
    {
    case FH_STANDARD_READ:
        reg_bit_process(&data, 0 << 8, 7 << 8);
        reg_bit_process(&data, 0 << 7, 1 << 7);

        reg_bit_process(&data1, 0x03 << 0, 0xff << 0);
        break;
    case FH_DUAL_OUTPUT:
        reg_bit_process(&data, 2 << 8, 7 << 8);
        reg_bit_process(&data, 0 << 7, 1 << 7);

        reg_bit_process(&data1, 0x3b << 0, 0xff << 0);
        break;
    case FH_DUAL_IO:
        reg_bit_process(&data, 0 << 8, 7 << 8);
        reg_bit_process(&data, 0 << 7, 1 << 7);

        reg_bit_process(&data1, 0xbb << 0, 0xff << 0);
        break;
    case FH_QUAD_OUTPUT:
        reg_bit_process(&data, 4 << 8, 7 << 8);
        reg_bit_process(&data, 0 << 7, 1 << 7);

        reg_bit_process(&data1, 0x6b << 0, 0xff << 0);
        break;
    case FH_QUAD_IO:
        reg_bit_process(&data, 2 << 8, 7 << 8);
        reg_bit_process(&data, 0 << 7, 1 << 7);

        reg_bit_process(&data1, 0xeb << 0, 0xff << 0);
        break;
    }

    SET_REG(spi_obj->base + OFFSET_SPI_CCFGR_OFFSET, data);
    SET_REG(spi_obj->base + OFFSET_SPI_OPCR_OFFSET, data1);

    return 0;
}

/*****************************************************************************
 * Description:
 *      add funtion description here
 * Parameters:
 *      description for each argument, new argument starts at new line
 * Return:
 *      what does this function returned?
 *****************************************************************************/

rt_int32_t Spi_SetAhbWriteWireMode(struct fh_spi_obj *spi_obj, spi_prog_wire_mode_e mode)
{


    rt_uint32_t data = GET_REG(spi_obj->base + OFFSET_SPI_CCFGR_OFFSET);
    rt_uint32_t data1 = GET_REG(spi_obj->base + OFFSET_SPI_OPCR_OFFSET);

    reg_bit_process(&data1, 1 << 18, 1 << 18);

    switch (mode)
    {
    case STANDARD_PROG:
        reg_bit_process(&data, 0 << 7, 1 << 7);

        reg_bit_process(&data1, 0x02 << 8, 0xff << 8);
        break;
    case QUAD_INPUT:
        reg_bit_process(&data, 0 << 7, 1 << 7);

        reg_bit_process(&data1, 0x32 << 8, 0xff << 8);
        break;
    }

    SET_REG(spi_obj->base + OFFSET_SPI_CCFGR_OFFSET, data);
    SET_REG(spi_obj->base + OFFSET_SPI_OPCR_OFFSET, data1);

    return 0;
}

/*****************************************************************************
 * Description:
 *      add funtion description here
 * Parameters:
 *      description for each argument, new argument starts at new line
 * Return:
 *      what does this function returned?
 *****************************************************************************/
rt_int32_t Spi_SetXip(struct fh_spi_obj *spi_obj, spi_xip_config_e value)
{


    rt_uint32_t data = GET_REG(spi_obj->base + OFFSET_SPI_CCFGR_OFFSET);
    rt_uint32_t data1 = GET_REG(spi_obj->base + OFFSET_SPI_OPCR_OFFSET);

    if (value == XIP_ENABLE)
    {
        reg_bit_process(&data, XIP_ENABLE << 11, 1 << 11);
        reg_bit_process(&data1, 0x20 << 20, 0xff << 20);
    } else if (value == XIP_DISABLE)
    {
        reg_bit_process(&data, XIP_DISABLE << 11, 1 << 11);
        reg_bit_process(&data1, 0xff << 20, 0xff << 20);
    }
    SET_REG(spi_obj->base + OFFSET_SPI_CCFGR_OFFSET, data);
    SET_REG(spi_obj->base + OFFSET_SPI_OPCR_OFFSET, data1);

    return 0;
}

/*****************************************************************************
 * Description:
 *      add funtion description here
 * Parameters:
 *      description for each argument, new argument starts at new line
 * Return:
 *      what does this function returned?
 *****************************************************************************/
rt_int32_t Spi_SetDPI(struct fh_spi_obj *spi_obj, spi_dpi_config_e value)
{


    rt_uint32_t data = GET_REG(spi_obj->base + OFFSET_SPI_OPCR_OFFSET);

    reg_bit_process(&data, value << 16, 1 << 16);
    SET_REG(spi_obj->base + OFFSET_SPI_OPCR_OFFSET, data);

    return 0;
}

/*****************************************************************************
 * Description:
 *      add funtion description here
 * Parameters:
 *      description for each argument, new argument starts at new line
 * Return:
 *      what does this function returned?
 *****************************************************************************/
rt_int32_t Spi_SetQPI(struct fh_spi_obj *spi_obj, spi_qpi_config_e value)
{

    rt_uint32_t data = GET_REG(spi_obj->base + OFFSET_SPI_OPCR_OFFSET);

    reg_bit_process(&data, value << 17, 1 << 17);
    SET_REG(spi_obj->base + OFFSET_SPI_OPCR_OFFSET, data);

    return 0;
}

/*****************************************************************************
 * Description:
 *      add funtion description here
 * Parameters:
 *      description for each argument, new argument starts at new line
 * Return:
 *      what does this function returned?
 *****************************************************************************/

rt_int32_t Spi_TimingConfigure(struct fh_spi_obj *spi_obj, rt_uint32_t value)
{

    SET_REG(spi_obj->base + OFFSET_SPI_TIMCR_OFFSET, value);
    return 0;
}

/*****************************************************************************
 * Description:
 *      add funtion description here
 * Parameters:
 *      description for each argument, new argument starts at new line
 * Return:
 *      what does this function returned?
 *****************************************************************************/

rt_int32_t Spi_SetBusBasAddr(struct fh_spi_obj *spi_obj)
{
    SET_REG(spi_obj->base + OFFSET_SPI_BBAR0_OFFSET, BUS_BASE_ADDR0);
    SET_REG(spi_obj->base + OFFSET_SPI_BBAR1_OFFSET, BUS_BASE_ADDR1);
    return 0;
}

rt_int32_t Spi_SetSwap(struct fh_spi_obj *spi_obj, unsigned int value)
{
    rt_int32_t data;

    SPI_Enable(spi_obj, 0);
    data = GET_REG(spi_obj->base + OFFSET_SPI_CCFGR_OFFSET);
    data &= ~(1<<12);
    data |= (value<<12);
    SET_REG(spi_obj->base + OFFSET_SPI_CCFGR_OFFSET, data);
    SPI_Enable(spi_obj, 1);
    return 0;
}

rt_int32_t Spi_SetWidth(struct fh_spi_obj *spi_obj, unsigned int value)
{
    rt_uint32_t ccfgr;
    rt_uint32_t ctrl0;

    SPI_Enable(spi_obj, 0);


    ccfgr = GET_REG(spi_obj->base + OFFSET_SPI_CCFGR_OFFSET);
    ctrl0 = GET_REG(spi_obj->base + OFFSET_SPI_CTRL0);

    if (value == 32)
    {
        ccfgr |= (0x1 << 16);
        ctrl0 &= ~(0xf << 16);
        ctrl0 |= ((value - 1) << 16);
    }
    else
    {
        ccfgr &= ~(0x1 << 16);
        ctrl0 &= ~(0xf);
        ctrl0 |= ((value - 1) << 0);
    }

    SET_REG(spi_obj->base + OFFSET_SPI_CCFGR_OFFSET, ccfgr);
    SET_REG(spi_obj->base + OFFSET_SPI_CTRL0, ctrl0);

    SPI_Enable(spi_obj, 1);

    return 0;
}

rt_int32_t Spi_SetClk_Masken(struct fh_spi_obj *spi_obj, unsigned int value)
{
    rt_uint32_t data = GET_REG(spi_obj->base + OFFSET_SPI_CCFGR_OFFSET);

    data &= ~(1<<15);
    data |= (value<<15);
    SET_REG(spi_obj->base + OFFSET_SPI_CCFGR_OFFSET, data);

    return 0;
}

rt_int32_t SPI_SetRxCaptureMode(struct fh_spi_obj *spi_obj, rt_uint32_t enable)
{
    UINT32 data;

    data = GET_REG(spi_obj->base + OFFSET_SPI_CCFGR_OFFSET);
    data &= ~(0x1 << 21);
    enable &= 0x1;
    data |= (enable << 21);
    SET_REG(spi_obj->base + OFFSET_SPI_CCFGR_OFFSET, data);

    return 0;
}