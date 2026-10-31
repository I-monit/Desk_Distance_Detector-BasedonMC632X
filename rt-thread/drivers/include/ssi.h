/**
 * Copyright (c) 2015-2019 Shanghai Fullhan Microelectronics Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 *
 * Change Logs:
 * Date           Author       Notes
 * 2019-04-12     zhangy        add license Apache-2.0
 */

#ifndef __SSI_H__
#define __SSI_H__
// #include "Libraries/inc/fh_driverlib.h"
#include <rtthread.h>
#include <drivers/spi.h>
#include <rtdevice.h>
#include "fh_spi.h"
#include "fh_dma.h"
#define SPI_PRIV(drv) ((struct fh_spi_obj)(drv->priv))

#define FH_SPI_SLAVE_MAX_NO 2

struct spi_controller;
/* platform use below */
struct spi_slave_platform_data
{
    rt_uint32_t cs_pin;
#define ACTIVE_LOW 1
#define ACTIVE_HIGH 2
    rt_uint32_t actice_level;
};

#define ONE_WIRE_SUPPORT        (1<<0)
#define DUAL_WIRE_SUPPORT       (1<<1)
#define QUAD_WIRE_SUPPORT       (1<<2)
#define MULTI_WIRE_SUPPORT      (1<<8)

#define SPI_DATA_DIR_IN         (0xaa)
#define SPI_DATA_DIR_OUT        (0xbb)
#define SPI_DATA_DIR_DUOLEX     (0xcc)

struct spi_control_platform_data
{
    rt_uint32_t id;
    rt_uint32_t irq;
    rt_uint32_t base;
    rt_uint32_t max_hz;
    rt_uint32_t slave_no;
    rt_uint32_t clk_in;
    /* handshake no... */
    rt_uint32_t rx_hs_no;
    rt_uint32_t tx_hs_no;

    char *dma_name;
    char *clk_name;
/* isr will be the default... */
#define USE_ISR_TRANSFER 0
#define USE_DMA_TRANSFER 1
#define USE_POLL_TRANSFER 2
    rt_uint32_t transfer_mode;
    rt_uint32_t dma_xfer_threshold;
    struct spi_controller *control;
    struct spi_slave_platform_data plat_slave[FH_SPI_SLAVE_MAX_NO];
    /* add support wire width*/
    rt_uint32_t ctl_wire_support;
    rt_uint32_t data_reg_offset;
#define INC_SUPPORT    0x55
    rt_uint32_t data_increase_support;
    rt_uint32_t data_field_size;
    #define SWAP_SUPPORT 0x55
    rt_uint32_t swap_support;

#define SPI_DMA_PROTCTL_ENABLE    0x55
    rt_uint32_t dma_protctl_enable;
    rt_uint32_t dma_protctl_data;

#define SPI_DMA_MASTER_SEL_ENABLE    0x55
    rt_uint32_t dma_master_sel_enable;
    rt_uint32_t dma_master_ctl_sel;
    rt_uint32_t dma_master_mem_sel;
};

struct spi_controller;
/* driver use below....... */
struct spi_slave_info
{
    /*here have 2 dev, one for classic, another for qspi*/
    struct rt_spi_device spi_device;
    /*new add qspi*/
#ifdef RT_SFUD_USING_QSPI
    struct rt_qspi_device qspi_dev;
#endif
    struct spi_controller *control;
    struct spi_slave_platform_data plat_slave;
    rt_uint32_t id;
    /* spi control will use to find all the slave info.. */
    struct spi_slave_info *next;
};

#ifdef RT_SFUD_USING_QSPI
struct s_qspi_send_data
{
#ifdef FH_USING_NAND_FLASH
    #define MAX_QSPI_SEND_BUF_SIZE  0x884
#else
    #define MAX_QSPI_SEND_BUF_SIZE  0x200
#endif
    rt_uint8_t buf[MAX_QSPI_SEND_BUF_SIZE];
    rt_uint32_t data_size;
    rt_uint32_t data_lines;
    rt_uint32_t dir;
};
#endif

struct spi_dma
{
    char *dma_name;
#define DMA_BIND_OK 0
#define DMA_BIND_ERROR 1
    rt_uint32_t dma_flag;
    /* bind to the dma dev.. */
    rt_uint32_t rx_hs;
    rt_uint32_t tx_hs;
    rt_uint8_t *rx_dummy_buff;
    rt_uint8_t *tx_dummy_buff;
    struct rt_dma_device *dma_dev;
    struct dma_transfer tx_trans;
    struct dma_transfer rx_trans;
    struct spi_controller *control;
};


struct spi_controller
{
    rt_uint32_t id;
    rt_uint32_t irq;
    rt_uint32_t base;
    rt_uint32_t max_hz;
    rt_uint32_t slave_no;
    rt_uint32_t clk_in;
    /* bind to the platform data.... */
    struct spi_control_platform_data *plat_data;

/* rt_uint32_t dma_xfer_flag; */

#define XFER_USE_ISR 0
#define XFER_USE_DMA 1
#define XFER_USE_POLL 2
    rt_uint32_t xfer_mode;

    struct spi_dma dma;
    rt_uint32_t dma_complete_times;
    struct rt_spi_bus spi_bus;
    struct spi_slave_info *spi_slave;

    struct rt_spi_message *current_message;
#ifdef RT_SFUD_USING_QSPI

    struct rt_qspi_message *cur_qspi_m;
    struct s_qspi_send_data qspi_send_data;
#endif
    struct rt_completion transfer_completion;
    struct rt_semaphore xfer_lock;
    struct fh_spi_obj obj;
    rt_uint32_t received_len;
    rt_uint32_t transfered_len;
    /*add multi*/
    rt_uint32_t active_wire_width;
    rt_uint32_t dir;
    struct rt_spi_device *active_spi_dev;
    int  (*multi_wire_func_init)(struct rt_spi_bus *p_master);
    void (*change_to_1_wire)(struct rt_spi_bus *p_master);
    void (*change_to_2_wire)(struct rt_spi_bus *p_master, unsigned int dir);
    void (*change_to_4_wire)(struct rt_spi_bus *p_master, unsigned int dir);
    void *priv;
};

static inline void *fh_spi_get_private_data(struct rt_spi_bus *master)
{
    return master->_private;
}
static inline void fh_spi_set_private_data(struct rt_spi_bus *master, void *pri_data)
{
    master->_private = pri_data;
}

void rt_hw_spi_init(void);

#endif /* SPI_H_ */
