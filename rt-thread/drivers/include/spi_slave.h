/*
 * spi_slave.h
 *
 *  Created on: May 5, 2017
 *      Author: duobao
 */

#ifndef __SPI_SLAVE_H__
#define __SPI_SLAVE_H__

#include <rtdevice.h>
#include "board_info.h"
#include "ssi.h"
#define SPI_SLAVE_IOCTL_SET_DATA            0x1            /**< set data */
#define SPI_SLAVE_IOCTL_XFER_DATA           0x2            /**< xfer data */
#define SPI_SLAVE_IOCTL_WAIT_DATA_DONE      0x3            /**< xfer data */
struct spi_slave_xfer
{
    rt_uint32_t rx_buf_addr;
    rt_uint32_t tx_buf_addr;
    rt_uint32_t transfer_length;
    void (*call_back)(void *cb_para);
    void *cb_para;
};

struct spi_slave_driver
{
    struct dma_transfer *dma_tx;
    struct dma_transfer *dma_rx;
    rt_uint8_t *dma_buff_tx;
    rt_uint8_t *dma_buff_rx;
    struct rt_dma_device *dma_dev;
    struct rt_completion transfer_completion;
    int dma_complete_count;
    struct spi_slave_xfer xfer;
    struct rt_semaphore xfer_lock;
    struct fh_spi_obj obj;
    struct spi_control_platform_data *plat_data;
    void *priv;
};

void rt_hw_spi_slave_init(void);

#endif /* SPI_SLAVE_H_ */
