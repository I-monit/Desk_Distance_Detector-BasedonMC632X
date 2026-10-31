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

#ifndef __FH_SPI_H__
#define __FH_SPI_H__

#include "fh_def.h"
#include "fh_arch.h"

#define OFFSET_SPI_CTRL0 (0x00)
#define OFFSET_SPI_CTRL1 (0x04)
#define OFFSET_SPI_SSIENR (0x08)
#define OFFSET_SPI_MWCR (0x0c)
#define OFFSET_SPI_SER (0x10)
#define OFFSET_SPI_BAUD (0x14)
#define OFFSET_SPI_TXFTLR (0x18)
#define OFFSET_SPI_RXFTLR (0x1c)
#define OFFSET_SPI_TXFLR (0x20)
#define OFFSET_SPI_RXFLR (0x24)
#define OFFSET_SPI_SR (0x28)
#define OFFSET_SPI_IMR (0x2c)
#define OFFSET_SPI_ISR (0x30)
#define OFFSET_SPI_RISR (0x34)
#define OFFSET_SPI_TXOIC (0x38)
#define OFFSET_SPI_RXOIC (0x3c)
#define OFFSET_SPI_RXUIC (0x40)
#define OFFSET_SPI_MSTIC (0x44)
#define OFFSET_SPI_ICR (0x48)
#define OFFSET_SPI_DMACTRL (0x4c)
#define OFFSET_SPI_DMATDL (0x50)
#define OFFSET_SPI_DMARDL (0x54)
#define OFFSET_SPI_IDR (0x58)
#define OFFSET_SPI_SSI_COMPVER (0x5c)
#define OFFSET_SPI_DR (0x60)
#define OFFSET_SPI_SAMPLE_DELAY (0xf0)
//spic new add reg.
#define OFFSET_SPI_CCFGR_OFFSET (0xf4)
#define OFFSET_SPI_OPCR_OFFSET  (0xf8)
#define OFFSET_SPI_TIMCR_OFFSET (0xfc)
#define OFFSET_SPI_BBAR0_OFFSET (0x100)
#define OFFSET_SPI_BBAR1_OFFSET (0x104)

#define SPI_FORMAT_MOTOROLA (0x00)
#define SPI_FORMAT_TI (0x10)
#define SPI_FORMAT_MICROWIRE (0x20)

#define SPI_MODE_TX_RX (0x000)
#define SPI_MODE_TX_ONLY (0x100)
#define SPI_MODE_RX_ONLY (0x200)
#define SPI_MODE_EEPROM (0x300)
#define SPI_TRANSFER_MODE_RANGE (0x300)

#define SPI_DATA_SIZE_4BIT (0x03)
#define SPI_DATA_SIZE_5BIT (0x04)
#define SPI_DATA_SIZE_6BIT (0x05)
#define SPI_DATA_SIZE_7BIT (0x06)
#define SPI_DATA_SIZE_8BIT (0x07)
#define SPI_DATA_SIZE_9BIT (0x08)
#define SPI_DATA_SIZE_10BIT (0x09)
#define SPI_DATA_SIZE_16BIT (0x0f)

#define SPI_POLARITY_HIGH (1 << 7)
#define SPI_POLARITY_LOW (0 << 7)

#define SPI_PHASE_RX_FIRST (0 << 6)
#define SPI_PHASE_TX_FIRST (1 << 6)

#define SPI_FIFO_DEPTH (32)

#define SPI_IRQ_TXEIM (1 << 0)
#define SPI_IRQ_TXOIM (1 << 1)
#define SPI_IRQ_RXUIM (1 << 2)
#define SPI_IRQ_RXOIM (1 << 3)
#define SPI_IRQ_RXFIM (1 << 4)
#define SPI_IRQ_MSTIM (1 << 5)
#define SPI_IRQ_ALL (0x3f)

#define SPI_ISR_FLAG \
    (SPI_IRQ_TXEIM | SPI_IRQ_TXOIM | SPI_IRQ_RXUIM | SPI_IRQ_RXOIM)
#define SPI_ISR_ERROR (SPI_IRQ_TXOIM | SPI_IRQ_RXUIM | SPI_IRQ_RXOIM)

#define SPI_STATUS_BUSY (1)

#define SPI_TX_DMA (1 << 1)
#define SPI_RX_DMA (1 << 0)

struct spi_config
{
    rt_uint32_t frame_format;
    rt_uint32_t transfer_mode;
    rt_uint32_t clk_polarity;
    rt_uint32_t clk_phase;
    rt_uint32_t data_size;
    rt_uint32_t clk_div;
    rt_uint32_t hs_mode;
};

struct fh_spi_obj
{
    int id;
    rt_uint32_t irq;
    rt_uint32_t base;
    rt_uint32_t rx_fifo_len;
    rt_uint32_t tx_fifo_len;
    rt_uint32_t transfered_len;
    rt_uint32_t received_len;
    rt_uint32_t cs_gpio_pin;

    struct spi_config config;
};

// read wire mode
typedef enum enum_spi_read_wire_mode {
    FH_STANDARD_READ = 0x00,
    FH_DUAL_OUTPUT   = 0x01,
    FH_DUAL_IO       = 0x02,
    FH_QUAD_OUTPUT   = 0x03,
    FH_QUAD_IO       = 0x04,
} spi_read_wire_mode_e;

// program wire mode
typedef enum enum_spi_prog_wire_mode {
    STANDARD_PROG = 0x00,
    QUAD_INPUT    = 0x01,
} spi_prog_wire_mode_e;

typedef struct {
    spi_read_wire_mode_e read;
    spi_prog_wire_mode_e prog;
} access_wire_mode_s;

// bus base address
enum {
    BUS_BASE_ADDR0 = 0xB0000000,
    BUS_BASE_ADDR1 = 0xB0000000,
};

// ahb Xip config
typedef enum enum_spi_xip_config {
    XIP_DISABLE = 0,
    XIP_ENABLE  = 1,
} spi_xip_config_e;

// ahb DPI config
typedef enum enum_spi_dpi_config {
    DPI_DISABLE = 0,
    DPI_ENABLE  = 1,
} spi_dpi_config_e;

// ahb QPI config
typedef enum enum_spi_qpi_config {
    QPI_DISABLE = 0,
    QPI_ENABLE  = 1,
} spi_qpi_config_e;
void SPI_EnableSlaveen(struct fh_spi_obj *spi_obj, rt_uint32_t port);
void SPI_DisableSlaveen(struct fh_spi_obj *spi_obj, rt_uint32_t port);
void SPI_SetTxLevel(struct fh_spi_obj *spi_obj, rt_uint32_t level);
rt_uint32_t SPI_GetTxLevel(struct fh_spi_obj *spi_obj);
void SPI_SetRxLevel(struct fh_spi_obj *spi_obj, rt_uint32_t level);
rt_uint32_t SPI_GetRxLevel(struct fh_spi_obj *spi_obj);
void SPI_EnableIrq(struct fh_spi_obj *spi_obj, rt_uint32_t flag);
void SPI_DisableIrq(struct fh_spi_obj *spi_obj, rt_uint32_t flag);
rt_uint32_t SPI_InterruptStatus(struct fh_spi_obj *spi_obj);
void SPI_ClearInterrupt(struct fh_spi_obj *spi_obj);
rt_uint32_t SPI_ReadTxFifoLevel(struct fh_spi_obj *spi_obj);
rt_uint32_t SPI_ReadRxFifoLevel(struct fh_spi_obj *spi_obj);
UINT8 SPI_ReadData(struct fh_spi_obj *spi_obj);
void SPI_WriteData(struct fh_spi_obj *spi_obj, UINT32 data);
rt_uint32_t SPI_ReadStatus(struct fh_spi_obj *spi_obj);
void SPI_Enable(struct fh_spi_obj *spi_obj, int enable);
void SPI_SetParameter(struct fh_spi_obj *spi_obj);
void SPI_ContinueReadNum(struct fh_spi_obj *spi_obj, rt_uint32_t num);
void SPI_SetTransferMode(struct fh_spi_obj *spi_obj, rt_uint32_t mode);
void SPI_EnableDma(struct fh_spi_obj *spi_obj, rt_uint32_t channel);
void SPI_DisableDma(struct fh_spi_obj *spi_obj, rt_uint32_t channel);
void SPI_EnableInterrupt(struct fh_spi_obj *spi_obj, rt_uint32_t flag);
void SPI_DisableInterrupt(struct fh_spi_obj *spi_obj, rt_uint32_t flag);
void SPI_WriteTxDmaLevel(struct fh_spi_obj *spi_obj, rt_uint32_t data);
void SPI_WriteRxDmaLevel(struct fh_spi_obj *spi_obj, rt_uint32_t data);
rt_int32_t Spi_SetApbReadWireMode(struct fh_spi_obj *spi_obj, spi_read_wire_mode_e mode);
rt_int32_t Spi_SetApbWriteWireMode(struct fh_spi_obj *spi_obj, spi_prog_wire_mode_e mode);
rt_int32_t Spi_SetAhbReadWireMode(struct fh_spi_obj *spi_obj, spi_read_wire_mode_e mode);
rt_int32_t Spi_SetAhbWriteWireMode(struct fh_spi_obj *spi_obj, spi_prog_wire_mode_e mode);
rt_int32_t Spi_SetXip(struct fh_spi_obj *spi_obj, spi_xip_config_e value);
rt_int32_t Spi_SetDPI(struct fh_spi_obj *spi_obj, spi_dpi_config_e value);
rt_int32_t Spi_SetQPI(struct fh_spi_obj *spi_obj, spi_qpi_config_e value);
rt_int32_t Spi_TimingConfigure(struct fh_spi_obj *spi_obj, rt_uint32_t value);
rt_int32_t Spi_SetBusBasAddr(struct fh_spi_obj *spi_obj);
void SPI_SampleDly(struct fh_spi_obj *spi_obj, int delay);
rt_uint32_t SPI_RawInterruptStatus(struct fh_spi_obj *spi_obj);
rt_int32_t Spi_SetSwap(struct fh_spi_obj *spi_obj, unsigned int value);
rt_int32_t Spi_SetWidth(struct fh_spi_obj *spi_obj, unsigned int value);
unsigned short SPI_ReadData_16(struct fh_spi_obj *spi_obj);
unsigned int SPI_ReadData_32(struct fh_spi_obj *spi_obj);
rt_int32_t Spi_SetClk_Masken(struct fh_spi_obj *spi_obj, unsigned int value);
rt_int32_t SPI_SetRxCaptureMode(struct fh_spi_obj *spi_obj, rt_uint32_t enable);
#endif /* FH_SPI_H_ */
