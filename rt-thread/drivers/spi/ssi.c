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

#include <rtthread.h>
#include <rthw.h>
#include <drivers/spi.h>
#include "fh_arch.h"
#include "board_info.h"
#include "ssi.h"
#include "gpio.h"
// #include "inc/fh_driverlib.h"
#include "fh_clock.h"
#include "dma.h"
#include "dma_mem.h"
#include "mmu.h"
#include "fh_pmu.h"
#ifdef RT_USING_PM
#include <pm.h>
#endif
/* #define FH_SPI_DEBUG */


#define SSI_ASSERT(_expr) do { \
    if (!(_expr)) \
    { rt_kprintf("%s:%d:\n",  \
                      __FILE__, __LINE__);\
     while (1)\
      ;\
    } \
} while (0)

#if defined(FH_SPI_DEBUG) && defined(RT_DEBUG)
#define PRINT_SPI_DBG(fmt, args...)   \
    do                                \
    {                                 \
        rt_kprintf("FH_SPI_DEBUG: "); \
        rt_kprintf(fmt, ##args);      \
    } while (0)
#else
#define PRINT_SPI_DBG(fmt, args...) \
    do                              \
    {                               \
    } while (0)
#endif

#define RX_DMA_CHANNEL AUTO_FIND_CHANNEL
#define TX_DMA_CHANNEL AUTO_FIND_CHANNEL

/* if use write page .....not use dma....just when read .use dma.. */
#define DMA_OR_ISR_THRESHOLD 20
#define MALLOC_DMA_MEM_SIZE 2048

#define TX_FIFO_MAX_LEN 256
#define TX_FIFO_MIN_LEN 2

#define RX_FIFO_MAX_LEN 256
#define RX_FIFO_MIN_LEN 2

#define TX_DMA_TRIGGER 8
#define RX_DMA_TRIGGER 8
#define TX_DMA_BURST_SIZE DW_DMA_SLAVE_MSIZE_8
#define RX_DMA_BURST_SIZE DW_DMA_SLAVE_MSIZE_8
#define SPI_RX_ONLY_ONE_TIME_SIZE    (0x10000)

#ifdef RT_SFUD_USING_QSPI
static int poll_pump_tx_only_data_qspi(struct spi_controller *fh_spi);
static int poll_pump_rx_only_data_qspi(struct spi_controller *spi_control);
#endif
extern void mmu_invalidate_dcache(rt_uint32_t buffer, rt_uint32_t size);
extern void mmu_clean_dcache(rt_uint32_t buffer, rt_uint32_t size);
extern void fh_select_gpio(int gpio_no);

#ifdef RT_SFUD_USING_QSPI
static int get_multi_wire_info(struct spi_controller *spi_control)
{
    return !!(spi_control->plat_data->ctl_wire_support & MULTI_WIRE_SUPPORT);
}
#endif

void check_spi_ctl_multi_wire_info(struct spi_controller *spi_control)
{
    struct spi_control_platform_data *plat_data;

    plat_data = spi_control->plat_data;
    /* clear info first... */
    plat_data->ctl_wire_support = 0;
#ifdef RT_SFUD_USING_QSPI
    if (plat_data->id == 0)
    {
        {
            plat_data->ctl_wire_support |= ONE_WIRE_SUPPORT;
            #ifdef CTL_DUAL_WIRE_SUPPORT
            plat_data->ctl_wire_support |= DUAL_WIRE_SUPPORT | MULTI_WIRE_SUPPORT;
            #endif
            #ifdef CTL_QUAD_WIRE_SUPPORT
            plat_data->ctl_wire_support |= DUAL_WIRE_SUPPORT | QUAD_WIRE_SUPPORT | MULTI_WIRE_SUPPORT;
            #endif
            plat_data->data_increase_support = INC_SUPPORT;
            plat_data->data_reg_offset = 0x1000;
            plat_data->data_field_size = 0x1000;
        }
        {
            spi_control->spi_bus.dma_speed = RT_SPI_IO_CTL_DMA_HIGH_SPEED;
            spi_control->spi_bus.dma_xfer_mode = RT_SPI_IO_CTL_DMA_RX_ONLY_MODE;
        }
#ifdef CONFIG_CHIP_FHYG
        spi_control->spi_bus.dma_speed = RT_SPI_IO_CTL_DMA_HIGH_SPEED;
        spi_control->spi_bus.dma_xfer_mode = RT_SPI_IO_CTL_DMA_RX_ONLY_MODE;
        plat_data->ctl_wire_support |= ONE_WIRE_SUPPORT;
        #ifdef CTL_DUAL_WIRE_SUPPORT
        plat_data->ctl_wire_support |= DUAL_WIRE_SUPPORT | MULTI_WIRE_SUPPORT;
        #endif
        #ifdef CTL_QUAD_WIRE_SUPPORT
        plat_data->ctl_wire_support |= DUAL_WIRE_SUPPORT | QUAD_WIRE_SUPPORT | MULTI_WIRE_SUPPORT;
        #endif
        plat_data->data_increase_support = INC_SUPPORT;
        plat_data->data_reg_offset = 0x1000;
        plat_data->data_field_size = 0x1000;
#endif
    }
#endif
    PRINT_SPI_DBG("spi_bus%d ctl_wire_support is %x\n",plat_data->id, plat_data->ctl_wire_support);
}

void check_spi_ctl_swap_info(struct spi_controller *spi_control)
{
    struct spi_control_platform_data *plat_data;

    plat_data = spi_control->plat_data;
    /* clear info first... */
    plat_data->swap_support = 0;
    if (plat_data->id == 0)
    {
        {
            plat_data->swap_support = SWAP_SUPPORT;
        }
#ifdef CONFIG_CHIP_FHYG
        plat_data->swap_support = SWAP_SUPPORT;
#endif
    }
    PRINT_SPI_DBG("spi_bus%d swap_support is %x\n",plat_data->id, plat_data->swap_support);
}

void check_spi_ctl_protctl_info(struct spi_controller *spi_control)
{
    struct spi_control_platform_data *plat_data;

    plat_data = spi_control->plat_data;
    /* clear info first... */
    if (plat_data->id == 0)
    {
        if (fh_is_8626v100())
        {
            plat_data->dma_protctl_enable = SPI_DMA_PROTCTL_ENABLE;
            plat_data->dma_protctl_data = 6;
        }
#ifdef CONFIG_CHIP_FHYG
        plat_data->dma_protctl_enable = SPI_DMA_PROTCTL_ENABLE;
        plat_data->dma_protctl_data = 6;
#endif
    }

}


void *fh_get_spi_dev_pri_data(struct rt_spi_device *device)
{
    return device->parent.user_data;
}


void fix_multi_xfer_mode(struct spi_controller *fh_spi)
{
#if (0)
    struct rt_spi_device *p_spi_dev;

    p_spi_dev = fh_spi->active_spi_dev;
    if (p_spi_dev->dev_open_multi_wire_flag & MULTI_WIRE_SUPPORT)
    {
        if ((fh_spi->active_wire_width &
        (DUAL_WIRE_SUPPORT | QUAD_WIRE_SUPPORT))
        && (fh_spi->dir == SPI_DATA_DIR_OUT)) {
            fh_spi->obj.config.transfer_mode = SPI_MODE_TX_ONLY;
        }
        else if ((fh_spi->active_wire_width &
        (DUAL_WIRE_SUPPORT | QUAD_WIRE_SUPPORT)) &&
        (fh_spi->dir == SPI_DATA_DIR_IN)) {
            fh_spi->obj.config.transfer_mode = SPI_MODE_RX_ONLY;

        }
        /*do not parse one wire..*/
    }
#endif
}

static void spi_ctl_fix_pump_data_mode(struct spi_controller *fh_spi) {
    struct rt_spi_message *crt_mes;
    rt_uint8_t *rxp;
    rt_uint8_t *txp;

    crt_mes = fh_spi->current_message;
    rxp = crt_mes->recv_buf;
    txp = (rt_uint8_t *) crt_mes->send_buf;

    if ((rxp == RT_NULL) && (txp != RT_NULL))
        fh_spi->obj.config.transfer_mode = SPI_MODE_TX_ONLY;
    else if ((rxp != RT_NULL) && (txp == RT_NULL))
        fh_spi->obj.config.transfer_mode = SPI_MODE_RX_ONLY;
    else
        fh_spi->obj.config.transfer_mode = SPI_MODE_TX_RX;

}

static void spi_wait_tx_only_done(struct spi_controller *fh_spi){
    rt_uint32_t status;
    struct fh_spi_obj *spi_obj;
    spi_obj = &fh_spi->obj;
    do
    {
        status = SPI_ReadStatus(spi_obj);
    } while ((status & 0x01) || (!(status & 0x04)));
}

static rt_err_t fh_spi_configure(struct rt_spi_device *device,
        struct rt_spi_configuration *configuration)
{
    struct spi_slave_info *spi_slave;
    struct spi_controller *spi_control;
    struct fh_spi_obj *spi_obj;
    struct spi_config *config;
    rt_uint32_t status;
    rt_uint32_t spi_hz;
    rt_uint32_t capture_enable = RT_FALSE;

    spi_slave = (struct spi_slave_info *) fh_get_spi_dev_pri_data(device);
    spi_control = spi_slave->control;
    spi_obj = &spi_control->obj;
    config = &spi_obj->config;

    PRINT_SPI_DBG("configuration:\n");
    PRINT_SPI_DBG("\tmode: 0x%x\n", configuration->mode);
    PRINT_SPI_DBG("\tdata_width: 0x%x\n", configuration->data_width);
    PRINT_SPI_DBG("\tmax_hz: 0x%x\n", configuration->max_hz);

    do
    {
        status = SPI_ReadStatus(spi_obj);
    } while (status & SPI_STATUS_BUSY);

    /* data_width */
    if (configuration->data_width <= 8)
    {
        config->data_size = SPI_DATA_SIZE_8BIT;
    }
    else if (configuration->data_width <= 16)
    {
        config->data_size = SPI_DATA_SIZE_16BIT;
    }
    else
    {
        return -RT_ERROR;
    }

    if (configuration->max_hz > spi_control->max_hz)
        spi_hz = spi_control->max_hz;
    else
        spi_hz = configuration->max_hz;

    /* fixme: div */
    config->clk_div = spi_control->clk_in / spi_hz;
    /*config->clk_div = 10;*/
    PRINT_SPI_DBG("config hz:%d spi div:%d\n", spi_hz, config->clk_div);

    if ((config->clk_div != 2) || (spi_hz < 100000000))
        capture_enable = RT_FALSE;
    else
        capture_enable = RT_TRUE;

    /* CPOL */
    if (configuration->mode & RT_SPI_CPOL)
    {
        config->clk_polarity = SPI_POLARITY_HIGH;
    }
    else
    {
        config->clk_polarity = SPI_POLARITY_LOW;
    }

    /* CPHA */
    if (configuration->mode & RT_SPI_CPHA)
    {
        config->clk_phase = SPI_PHASE_TX_FIRST;
    }
    else
    {
        config->clk_phase = SPI_PHASE_RX_FIRST;
    }

    config->frame_format = SPI_FORMAT_MOTOROLA;
    config->transfer_mode = SPI_MODE_TX_RX;
    config->hs_mode = capture_enable;
    /* spi_control->dma_speed = configuration->dma_speed; */
    SPI_Enable(spi_obj, 0);
    SPI_SampleDly(spi_obj, configuration->sample_delay);
    SPI_SetParameter(spi_obj);
    SPI_DisableInterrupt(spi_obj, SPI_IRQ_ALL);
    if (capture_enable)
    {
        SPI_SetRxCaptureMode(spi_obj, 1);
        SPI_SampleDly(spi_obj, 3);
    }
    else
    {
        SPI_SetRxCaptureMode(spi_obj, 0);
    }
    SPI_Enable(spi_obj, 1);

    return RT_EOK;
}


static void fh_spi_tx_rx_dma_done_rx(void *arg)
{
    struct spi_controller *spi_control = (struct spi_controller *) arg;
    spi_control->dma_complete_times++;
    struct fh_spi_obj *spi_obj;
    spi_obj = &spi_control->obj;
    /* rt_kprintf("spi dma isr done.....\n"); */
    if (spi_control->dma_complete_times == 2)
    {
        spi_control->dma_complete_times = 0;
        SPI_DisableDma(spi_obj, SPI_TX_DMA | SPI_RX_DMA);
        rt_completion_done(&spi_control->transfer_completion);
    }
}

static void fh_spi_tx_rx_dma_done_tx(void *arg)
{
    struct spi_controller *spi_control = (struct spi_controller *) arg;
    spi_control->dma_complete_times++;
    struct fh_spi_obj *spi_obj;
    spi_obj = &spi_control->obj;
    /* rt_kprintf("spi dma isr done.....\n"); */
    if (spi_control->dma_complete_times == 2)
    {
        spi_control->dma_complete_times = 0;
        SPI_DisableDma(spi_obj, SPI_TX_DMA | SPI_RX_DMA);
        rt_completion_done(&spi_control->transfer_completion);
    }
}

static void fh_spi_tx_only_dma_done(void *arg)
{
    struct spi_controller *spi_control = (struct spi_controller *) arg;
    struct fh_spi_obj *spi_obj;
    spi_obj = &spi_control->obj;
    SPI_DisableDma(spi_obj, SPI_TX_DMA);
    rt_completion_done(&spi_control->transfer_completion);
}

static void fh_spi_rx_only_dma_done(void *arg)
{

    struct spi_controller *spi_control = (struct spi_controller *) arg;
    struct fh_spi_obj *spi_obj;
    //rt_kprintf("%s u got me...\n",__func__);
    spi_obj = &spi_control->obj;
    SPI_DisableDma(spi_obj, SPI_RX_DMA);
    rt_completion_done(&spi_control->transfer_completion);
}


void dma_set_tx_para(struct spi_controller *spi_control, rt_uint32_t size,
void (*call_back)(void *arg), rt_uint32_t data_width)
{

    struct dma_transfer *trans;
    rt_uint32_t hs_no;
    struct rt_spi_message *current_message = spi_control->current_message;

    trans = &spi_control->dma.tx_trans;
    hs_no = spi_control->dma.tx_hs;

    struct fh_spi_obj *spi_obj;
    spi_obj = &spi_control->obj;

    trans->dma_number = 0;
    trans->dst_add = (rt_uint32_t)(spi_obj->base + OFFSET_SPI_DR);
    trans->dst_hs = DMA_HW_HANDSHAKING;
    trans->dst_inc_mode = DW_DMA_SLAVE_FIX;
    trans->dst_msize = TX_DMA_BURST_SIZE;
    trans->dst_per = hs_no;
    trans->dst_width = DW_DMA_SLAVE_WIDTH_8BIT;
    trans->fc_mode = DMA_M2P;
    if (current_message->send_buf)
    {
        /*here need to flush src mem...
        means that sync the cache and ddr data..*/
        mmu_clean_dcache((rt_uint32_t)current_message->send_buf,size);
        trans->src_inc_mode = DW_DMA_SLAVE_INC;
        trans->src_add = (rt_uint32_t) current_message->send_buf;
    }
    else{
        trans->src_inc_mode = DW_DMA_SLAVE_FIX;
        trans->src_add = (rt_uint32_t) spi_control->dma.tx_dummy_buff;
    }
    trans->src_msize = TX_DMA_BURST_SIZE;
    trans->src_width = DW_DMA_SLAVE_WIDTH_8BIT;
    trans->trans_len = size;
    trans->complete_callback = (void *) call_back;
    trans->complete_para = (void *) spi_control;

}


void dma_set_tx_para_normal(struct spi_controller *spi_control, rt_uint32_t size,void (*call_back)(void *arg))
{

    struct dma_transfer *trans;
    rt_uint32_t hs_no;
    struct rt_spi_message *current_message = spi_control->current_message;

    trans = &spi_control->dma.tx_trans;
    hs_no = spi_control->dma.tx_hs;

    struct fh_spi_obj *spi_obj;
    spi_obj = &spi_control->obj;
    if (size > MALLOC_DMA_MEM_SIZE)
    {
        rt_kprintf("[spi_dma]message len too large..\n");
        rt_kprintf("[spi_dma] message len is %d,max len is %d\n", size,
        MALLOC_DMA_MEM_SIZE);
        SSI_ASSERT(size <= MALLOC_DMA_MEM_SIZE);
    }
    trans->dma_number = 0;
    trans->dst_add = (rt_uint32_t)(spi_obj->base + OFFSET_SPI_DR);
    trans->dst_hs = DMA_HW_HANDSHAKING;
    trans->dst_inc_mode = DW_DMA_SLAVE_FIX;
    trans->dst_msize = TX_DMA_BURST_SIZE;
    trans->dst_per = hs_no;
    trans->dst_width = DW_DMA_SLAVE_WIDTH_8BIT;
    trans->fc_mode = DMA_M2P;
    if (current_message->send_buf)
    {
        /*here need to flush src mem...
        means that sync the cache and ddr data..*/
        rt_memcpy(spi_control->dma.tx_dummy_buff, current_message->send_buf,
                size);
    }
    else{
        rt_memset((void *) spi_control->dma.tx_dummy_buff, 0xff, size);
    }
    trans->src_add = (rt_uint32_t) spi_control->dma.tx_dummy_buff;
    trans->src_inc_mode = DW_DMA_SLAVE_INC;
    trans->src_msize = TX_DMA_BURST_SIZE;
    trans->src_width = DW_DMA_SLAVE_WIDTH_8BIT;
    trans->trans_len = size;
    trans->complete_callback = (void *) call_back;
    trans->complete_para = (void *) spi_control;

}

void dma_set_rx_para(struct spi_controller *spi_control, rt_uint32_t size,
void (*call_back)(void *arg), rt_uint32_t data_width)
{

    struct dma_transfer *trans;
    rt_uint32_t hs_no;
    trans = &spi_control->dma.rx_trans;
    hs_no = spi_control->dma.rx_hs;
    struct fh_spi_obj *spi_obj;
    spi_obj = &spi_control->obj;
    struct rt_spi_message *current_message = spi_control->current_message;
    trans->dma_number = 0;
    trans->fc_mode = DMA_P2M;
    if (current_message->recv_buf)
    {
        /* here need to flush src mem...
        incase cache write data to ddr when dma work..*/
        if(((unsigned int)current_message->recv_buf % 32) ||
        ((data_width / 8 * size) % 32)){
            rt_kprintf("%s : %d ; buf add: 0x%x or size : 0x%x should 32B allign..\n"
            ,__func__,__LINE__,(unsigned int)current_message->recv_buf,size);
        }
        mmu_clean_dcache((rt_uint32_t)current_message->recv_buf, data_width / 8 * size);
        trans->dst_inc_mode = DW_DMA_SLAVE_INC;
        trans->dst_add = (rt_uint32_t) current_message->recv_buf;
    }
    else{
        trans->dst_inc_mode = DW_DMA_SLAVE_FIX;
        trans->dst_add = (rt_uint32_t) spi_control->dma.rx_dummy_buff;
    }
    trans->dst_msize = RX_DMA_BURST_SIZE;
    if (spi_control->plat_data->data_reg_offset != 0)
        trans->src_add = (rt_uint32_t)(spi_obj->base + spi_control->plat_data->data_reg_offset);
    else
        trans->src_add = (rt_uint32_t)(spi_obj->base + OFFSET_SPI_DR);

    if (spi_control->plat_data->data_increase_support == INC_SUPPORT)
    {
#ifdef FH_USING_MOL_DMA
        trans->src_inc_mode = DW_DMA_SLAVE_FIX;
        trans->period_len = spi_control->plat_data->data_field_size;
        trans->dst_width = DW_DMA_SLAVE_WIDTH_32BIT;
#else
        trans->src_inc_mode = DW_DMA_SLAVE_INC;
        trans->period_len = spi_control->plat_data->data_field_size;
        trans->src_reload_flag = ADDR_RELOAD;
        trans->dst_width = DW_DMA_SLAVE_WIDTH_32BIT;
#endif

    }
    else
    {
        trans->src_inc_mode = DW_DMA_SLAVE_FIX;
        trans->period_len = 0;
        trans->dst_width = DW_DMA_SLAVE_WIDTH_8BIT;
    }

    if (spi_control->plat_data->dma_protctl_enable ==
        SPI_DMA_PROTCTL_ENABLE)
    {
        trans->protctl_flag = PROTCTL_ENABLE;
        trans->protctl_data = spi_control->plat_data->dma_protctl_data;
    }

    if (spi_control->plat_data->dma_master_sel_enable ==
        SPI_DMA_MASTER_SEL_ENABLE)
    {
        trans->master_flag = MASTER_SEL_ENABLE;
        trans->src_master = spi_control->plat_data->dma_master_ctl_sel;
        trans->dst_master = spi_control->plat_data->dma_master_mem_sel;
    }

    trans->src_msize = RX_DMA_BURST_SIZE;
    if (data_width == 8)
        trans->src_width = DW_DMA_SLAVE_WIDTH_8BIT;
    else if (data_width == 16)
        trans->src_width = DW_DMA_SLAVE_WIDTH_16BIT;
    else if (data_width == 32)
        trans->src_width = DW_DMA_SLAVE_WIDTH_32BIT;
    else
    {
        rt_kprintf("data width:%d err\n",data_width);
        SSI_ASSERT(0);
    }
    trans->src_hs = DMA_HW_HANDSHAKING;
    trans->src_per = hs_no;
    trans->trans_len = size;
    trans->complete_callback = (void *) call_back;
    trans->complete_para = (void *) spi_control;

}


void dma_set_rx_para_normal(struct spi_controller *spi_control,
                            rt_uint32_t size, void (*call_back)(void *arg))
{

    struct dma_transfer *trans;
    rt_uint32_t hs_no;
    trans = &spi_control->dma.rx_trans;
    hs_no = spi_control->dma.rx_hs;
    struct fh_spi_obj *spi_obj;
    spi_obj = &spi_control->obj;
    trans->dma_number = 0;
    trans->fc_mode = DMA_P2M;
    if (size > MALLOC_DMA_MEM_SIZE)
    {
        rt_kprintf("[spi_dma]message len too large..\n");
        rt_kprintf("[spi_dma] message len is %d,max len is %d\n", size,
        MALLOC_DMA_MEM_SIZE);
        SSI_ASSERT(size <= MALLOC_DMA_MEM_SIZE);
    }
    trans->dst_add = (rt_uint32_t) spi_control->dma.rx_dummy_buff;
    trans->dst_inc_mode = DW_DMA_SLAVE_INC;

    trans->dst_msize = RX_DMA_BURST_SIZE;
    if (spi_control->plat_data->data_reg_offset != 0)
        trans->src_add = (rt_uint32_t)(spi_obj->base + spi_control->plat_data->data_reg_offset);
    else
        trans->src_add = (rt_uint32_t)(spi_obj->base + OFFSET_SPI_DR);

    if (spi_control->plat_data->data_increase_support == INC_SUPPORT)
    {
        trans->src_inc_mode = DW_DMA_SLAVE_INC;
        trans->period_len = spi_control->plat_data->data_field_size;
        trans->src_reload_flag = ADDR_RELOAD;
        trans->dst_width = DW_DMA_SLAVE_WIDTH_32BIT;
    }
    else
    {
        trans->src_inc_mode = DW_DMA_SLAVE_FIX;
        trans->period_len = 0;
        trans->dst_width = DW_DMA_SLAVE_WIDTH_8BIT;
    }

    trans->src_msize = RX_DMA_BURST_SIZE;
    trans->src_width = DW_DMA_SLAVE_WIDTH_8BIT;
    trans->src_hs = DMA_HW_HANDSHAKING;
    trans->src_per = hs_no;
    trans->trans_len = size;
    trans->complete_callback = (void *) call_back;
    trans->complete_para = (void *) spi_control;
}


static int dma_pump_tx_rx_data(struct spi_controller *spi_control)
{

    //rt_kprintf("%s ...\n",__func__);
    int ret;
    struct fh_spi_obj *spi_obj;
    spi_obj = &spi_control->obj;
    /* one time xfer bytes... */
    struct rt_dma_device *dma_dev = spi_control->dma.dma_dev;
    rt_completion_init(&spi_control->transfer_completion);

    dma_set_tx_para(spi_control, spi_control->current_message->length, fh_spi_tx_rx_dma_done_tx, 8);
    dma_set_rx_para(spi_control, spi_control->current_message->length, fh_spi_tx_rx_dma_done_rx, 8);
    SPI_Enable(spi_obj, 0);
    /* this para should ref the dma para..... */
    SPI_WriteTxDmaLevel(spi_obj, spi_obj->tx_fifo_len - 8);
    SPI_WriteRxDmaLevel(spi_obj, 7);
    SPI_Enable(spi_obj, 1);
    dma_dev->ops->control(dma_dev,
    RT_DEVICE_CTRL_DMA_SINGLE_TRANSFER,
            (void *) &spi_control->dma.rx_trans);
    dma_dev->ops->control(dma_dev,
    RT_DEVICE_CTRL_DMA_SINGLE_TRANSFER,
            (void *) &spi_control->dma.tx_trans);
    SPI_EnableDma(spi_obj,SPI_TX_DMA | SPI_RX_DMA);
    ret = rt_completion_wait(&spi_control->transfer_completion,
    RT_TICK_PER_SECOND * 50);
    if (ret)
    {
        rt_kprintf("ERROR: %s, transfer timeout\n", __func__);
        return -RT_ETIMEOUT;
    }
    if (spi_control->current_message->recv_buf)
    {
        /* make ddr data sync with cache*/
        mmu_invalidate_dcache((rt_uint32_t)spi_control->current_message->recv_buf,
        spi_control->current_message->length);
    }

    return 0;
}


static int dma_pump_tx_rx_data_normal(struct spi_controller *spi_control)
{
    int ret;
    struct fh_spi_obj *spi_obj;
    spi_obj = &spi_control->obj;
    /* one time xfer bytes... */
    struct rt_dma_device *dma_dev = spi_control->dma.dma_dev;
    rt_uint32_t otp_xfer_bytes;
    while (spi_control->transfered_len != spi_control->current_message->length)
    {
        rt_completion_init(&spi_control->transfer_completion);
        otp_xfer_bytes = MIN(MALLOC_DMA_MEM_SIZE,
                (spi_control->current_message->length
                        - spi_control->transfered_len));
        /* tx data prepare */
        dma_set_tx_para_normal(spi_control, otp_xfer_bytes,fh_spi_tx_rx_dma_done_tx);
        dma_set_rx_para_normal(spi_control, otp_xfer_bytes,fh_spi_tx_rx_dma_done_rx);
        /* rx data prepare */
        /* dma go... */
        SPI_Enable(spi_obj, 0);
        /* this para should ref the dma para..... */
        SPI_WriteTxDmaLevel(spi_obj, spi_obj->tx_fifo_len - 8);
        SPI_WriteRxDmaLevel(spi_obj, 7);
        SPI_Enable(spi_obj, 1);

        dma_dev->ops->control(dma_dev,
        RT_DEVICE_CTRL_DMA_SINGLE_TRANSFER,
                (void *) &spi_control->dma.rx_trans);
        dma_dev->ops->control(dma_dev,
        RT_DEVICE_CTRL_DMA_SINGLE_TRANSFER,
                (void *) &spi_control->dma.tx_trans);
        SPI_EnableDma(spi_obj,SPI_TX_DMA | SPI_RX_DMA);
        ret = rt_completion_wait(&spi_control->transfer_completion,
        RT_TICK_PER_SECOND * 50);

        if (ret)
        {
            rt_kprintf("ERROR: %s, transfer timeout\n", __func__);
            return -RT_ETIMEOUT;
        }

        if (spi_control->current_message->send_buf)
        {
            spi_control->current_message->send_buf += otp_xfer_bytes;
        }
        if (spi_control->current_message->recv_buf)
        {
            rt_memcpy((void *) spi_control->current_message->recv_buf,
                    (void *) spi_control->dma.rx_dummy_buff, otp_xfer_bytes);
            spi_control->current_message->recv_buf += otp_xfer_bytes;
        }
        spi_control->transfered_len += otp_xfer_bytes;
    }

    return 0;
}

static int dma_pump_tx_only_data(struct spi_controller *spi_control)
{
    int ret;
    struct fh_spi_obj *spi_obj;
    spi_obj = &spi_control->obj;
    /* one time xfer bytes... */
    struct rt_dma_device *dma_dev = spi_control->dma.dma_dev;
    rt_completion_init(&spi_control->transfer_completion);
    dma_set_tx_para(spi_control, spi_control->current_message->length, fh_spi_tx_only_dma_done, 8);
    SPI_Enable(spi_obj, 0);
    /* this para should ref the dma para..... */
    SPI_WriteTxDmaLevel(spi_obj, spi_obj->tx_fifo_len - 8);
    SPI_Enable(spi_obj, 1);
    dma_dev->ops->control(dma_dev,
    RT_DEVICE_CTRL_DMA_SINGLE_TRANSFER,
            (void *) &spi_control->dma.tx_trans);
    SPI_EnableDma(spi_obj,SPI_TX_DMA);
    ret = rt_completion_wait(&spi_control->transfer_completion,
    RT_TICK_PER_SECOND * 50);
    if (ret)
    {
        rt_kprintf("ERROR: %s, transfer timeout\n", __func__);
        return -RT_ETIMEOUT;
    }
    /*wait hw done...*/
    spi_wait_tx_only_done(spi_control);
    return 0;
}

static int dma_pump_tx_only_data_normal(struct spi_controller *spi_control){
    int ret;
    struct fh_spi_obj *spi_obj;
    spi_obj = &spi_control->obj;
    rt_uint32_t otp_xfer_bytes = 0;
    /* one time xfer bytes... */
    struct rt_dma_device *dma_dev = spi_control->dma.dma_dev;
    while (spi_control->transfered_len != spi_control->current_message->length)
    {
        rt_completion_init(&spi_control->transfer_completion);
        otp_xfer_bytes = MIN(MALLOC_DMA_MEM_SIZE,
                (spi_control->current_message->length
                        - spi_control->transfered_len));
        /* tx data prepare */
        dma_set_tx_para_normal(spi_control, otp_xfer_bytes,fh_spi_tx_only_dma_done);
        /* rx data prepare */
        /* dma go... */
        SPI_Enable(spi_obj, 0);
        SPI_WriteTxDmaLevel(spi_obj, spi_obj->tx_fifo_len - 8);
        SPI_Enable(spi_obj, 1);
        dma_dev->ops->control(dma_dev,
        RT_DEVICE_CTRL_DMA_SINGLE_TRANSFER,
                (void *) &spi_control->dma.tx_trans);
        SPI_EnableDma(spi_obj, SPI_TX_DMA);
        ret = rt_completion_wait(&spi_control->transfer_completion,
        RT_TICK_PER_SECOND * 50);

        if (ret)
        {
            rt_kprintf("ERROR: %s, transfer timeout\n", __func__);
            return -RT_ETIMEOUT;
        }
        /* wait hw fifo done... */
        spi_wait_tx_only_done(spi_control);

        if (spi_control->current_message->send_buf)
        {
            spi_control->current_message->send_buf += otp_xfer_bytes;
        }
        spi_control->transfered_len += otp_xfer_bytes;
    }



    return 0;
}

static int rx_only_fix_data_width(struct spi_controller *spi_control, unsigned int size)
{
    unsigned int data_width = 0;
    unsigned int rx_address = (unsigned int)spi_control->current_message->recv_buf;

    if (spi_control->plat_data->swap_support != SWAP_SUPPORT)
        return 8;

    if (((rx_address % 4) == 0) && ((size % 4) == 0))
        data_width = 32;
    else if (((rx_address % 2) == 0) && ((size % 2) == 0))
        data_width = 16;
    else
        data_width = 8;

    return data_width;

}

static int dma_pump_rx_only_data(struct spi_controller *spi_control){

    int ret;
    struct fh_spi_obj *spi_obj;
    unsigned int data_width = 0;

    spi_obj = &spi_control->obj;
    /* one time xfer bytes... */
    rt_uint32_t otp_xfer_bytes = 0;
    struct rt_dma_device *dma_dev = spi_control->dma.dma_dev;
    while (spi_control->received_len != spi_control->current_message->length)
    {
        rt_completion_init(&spi_control->transfer_completion);
        otp_xfer_bytes = MIN(SPI_RX_ONLY_ONE_TIME_SIZE,
                (spi_control->current_message->length
                        - spi_control->received_len));
        data_width = rx_only_fix_data_width(spi_control, otp_xfer_bytes);
        /*cal size / data width */
        otp_xfer_bytes = otp_xfer_bytes / (data_width / 8);
        dma_set_rx_para(spi_control, otp_xfer_bytes, fh_spi_rx_only_dma_done, data_width);
        if (data_width == 16)
        {
            Spi_SetSwap(spi_obj, 1);
            Spi_SetWidth(spi_obj, 16);
        }
        else if (data_width == 32)
        {
            Spi_SetSwap(spi_obj, 1);
            Spi_SetWidth(spi_obj, 32);
        }

        SPI_Enable(spi_obj, 0);
        /* this para should ref the dma para..... */
        SPI_WriteRxDmaLevel(spi_obj, 7);
        SPI_ContinueReadNum(spi_obj,otp_xfer_bytes);
        SPI_Enable(spi_obj, 1);
        SPI_EnableDma(spi_obj,SPI_RX_DMA);
        dma_dev->ops->control(dma_dev,
        RT_DEVICE_CTRL_DMA_SINGLE_TRANSFER,
                (void *) &spi_control->dma.rx_trans);
        SPI_WriteData(spi_obj, 0xffffffff);
        ret = rt_completion_wait(&spi_control->transfer_completion,
        RT_TICK_PER_SECOND * 50);
        if (ret)
        {
            rt_kprintf("ERROR: %s, transfer timeout\n", __func__);
            if ((data_width == 16) || (data_width == 32))
            {
                Spi_SetSwap(spi_obj, 0);
                Spi_SetWidth(spi_obj, 8);
            }
            return -RT_ETIMEOUT;
        }
        /* add memcpy to user buff */
        if (spi_control->current_message->recv_buf)
        {
            /* make ddr data sync with cache*/
            mmu_invalidate_dcache((rt_uint32_t)spi_control->current_message->recv_buf, data_width / 8 * otp_xfer_bytes);
            spi_control->current_message->recv_buf += data_width / 8 * otp_xfer_bytes;
        }
        spi_control->received_len += data_width / 8 * otp_xfer_bytes;
    }
    if ((data_width == 16) || (data_width == 32))
    {
        Spi_SetSwap(spi_obj, 0);
        Spi_SetWidth(spi_obj, 8);
    }
    return 0;
}


static int dma_pump_rx_only_data_normal(struct spi_controller *spi_control){

    int ret;

    struct fh_spi_obj *spi_obj;
    spi_obj = &spi_control->obj;
    /* one time xfer bytes... */
    rt_uint32_t otp_xfer_bytes = 0;
    struct rt_dma_device *dma_dev = spi_control->dma.dma_dev;
    while (spi_control->received_len != spi_control->current_message->length)
    {
        rt_completion_init(&spi_control->transfer_completion);
        otp_xfer_bytes = MIN(MALLOC_DMA_MEM_SIZE,
                (spi_control->current_message->length
                        - spi_control->received_len));
        dma_set_rx_para_normal(spi_control, otp_xfer_bytes,fh_spi_rx_only_dma_done);
        SPI_Enable(spi_obj, 0);
        /* this para should ref the dma para..... */
        SPI_WriteRxDmaLevel(spi_obj, 7);
        SPI_ContinueReadNum(spi_obj,otp_xfer_bytes);
        SPI_Enable(spi_obj, 1);
        SPI_EnableDma(spi_obj,SPI_RX_DMA);
        dma_dev->ops->control(dma_dev,
        RT_DEVICE_CTRL_DMA_SINGLE_TRANSFER,
                (void *) &spi_control->dma.rx_trans);
        SPI_WriteData(spi_obj, 0xffffffff);
        ret = rt_completion_wait(&spi_control->transfer_completion,
        RT_TICK_PER_SECOND * 50);
        if (ret)
        {
            rt_kprintf("ERROR: %s, transfer timeout\n", __func__);
            return -RT_ETIMEOUT;
        }
        /* add memcpy to user buff */
        if (spi_control->current_message->recv_buf)
        {
            rt_memcpy((void *) spi_control->current_message->recv_buf,
                    (void *) spi_control->dma.rx_dummy_buff, otp_xfer_bytes);
            spi_control->current_message->recv_buf += otp_xfer_bytes;
        }
        spi_control->received_len += otp_xfer_bytes;
    }

    return 0;
}

rt_uint32_t xfer_data_dma(struct spi_controller *spi_control)
{

    int ret = RT_EOK;
    struct fh_spi_obj *spi_obj;
    spi_obj = &spi_control->obj;
    /* one time xfer bytes... */
#ifdef RT_SFUD_USING_QSPI
    if (get_multi_wire_info(spi_control))
    {

        struct rt_qspi_message *cur_qspi_m;
        /*poll send tx first...*/
        cur_qspi_m = spi_control->cur_qspi_m;
        spi_control->obj.config.transfer_mode = SPI_MODE_TX_ONLY;
        SPI_Enable(spi_obj, 0);
        SPI_SetTransferMode(spi_obj, spi_control->obj.config.transfer_mode);
        SPI_Enable(spi_obj, 1);
        /*first tx only...*/
        poll_pump_tx_only_data_qspi(spi_control);
        /*then rx only...*/
        if (!spi_control->current_message->recv_buf)
        {
            return 0;
        }
        spi_control->obj.config.transfer_mode = SPI_MODE_RX_ONLY;
        /*parse multi wire here..*/
        if (cur_qspi_m->qspi_data_lines == 4)
        {
            if (spi_control->change_to_4_wire)
                spi_control->change_to_4_wire(&spi_control->spi_bus, SPI_DATA_DIR_IN);
        }
        else if (cur_qspi_m->qspi_data_lines == 2)
        {
            if (spi_control->change_to_2_wire)
                spi_control->change_to_2_wire(&spi_control->spi_bus, SPI_DATA_DIR_IN);
        }
        else
        {
            if (spi_control->change_to_1_wire)
                spi_control->change_to_1_wire(&spi_control->spi_bus);
        }
    }
    else
    /*then open dma rx*/
#endif
    {
        spi_ctl_fix_pump_data_mode(spi_control);
        /*if multi open, recheck mode..*/
        fix_multi_xfer_mode(spi_control);
    }

    /* add io ctl to set the rx only to tx_rx mode.*/

    if (spi_control->obj.config.transfer_mode == SPI_MODE_RX_ONLY)
    {
        if (spi_control->spi_bus.dma_xfer_mode != RT_SPI_IO_CTL_DMA_RX_ONLY_MODE)
            spi_control->obj.config.transfer_mode = SPI_MODE_TX_RX;
    }
    SPI_Enable(spi_obj, 0);
    SPI_SetTransferMode(spi_obj, spi_control->obj.config.transfer_mode);
    SPI_Enable(spi_obj, 1);
    if (spi_control->spi_bus.dma_speed == RT_SPI_IO_CTL_DMA_HIGH_SPEED)
    {
        /*rt_kprintf("(! !) %x\n",spi_control->obj.config.transfer_mode);*/
        if (spi_control->obj.config.transfer_mode == SPI_MODE_TX_ONLY)
        {
            ret = dma_pump_tx_only_data(spi_control);
        }
        else if (spi_control->obj.config.transfer_mode == SPI_MODE_RX_ONLY)
        {
            ret = dma_pump_rx_only_data(spi_control);
        }
        else
        {
            ret = dma_pump_tx_rx_data(spi_control);
        }
    }
    else
    {
        /*rt_kprintf("(= =) %x\n",spi_control->obj.config.transfer_mode);*/
        if (spi_control->obj.config.transfer_mode == SPI_MODE_TX_ONLY)
        {
            ret = dma_pump_tx_only_data_normal(spi_control);
        }
        else if (spi_control->obj.config.transfer_mode == SPI_MODE_RX_ONLY)
        {
            ret = dma_pump_rx_only_data_normal(spi_control);
        }
        else
        {
            ret = dma_pump_tx_rx_data_normal(spi_control);
        }
    }
#ifdef RT_SFUD_USING_QSPI
    if (spi_control->change_to_1_wire)
        spi_control->change_to_1_wire(&spi_control->spi_bus);
#endif
    return ret;

}

rt_uint32_t xfer_data_isr(struct spi_controller *control)
{

    int ret;
    struct fh_spi_obj *spi_obj;
    spi_obj = &control->obj;
    spi_ctl_fix_pump_data_mode(control);
    PRINT_SPI_DBG("use isr xfer data...\n");
    /*if multi open ,recheck mode..*/
    fix_multi_xfer_mode(control);
    if (control->obj.config.transfer_mode == SPI_MODE_RX_ONLY)
        control->obj.config.transfer_mode = SPI_MODE_TX_RX;

    SPI_Enable(spi_obj, 0);
    SPI_SetTxLevel(spi_obj, spi_obj->tx_fifo_len - 1);
    SPI_EnableInterrupt(spi_obj, SPI_IRQ_TXEIM);
    SPI_SetTransferMode(spi_obj, control->obj.config.transfer_mode);
    SPI_Enable(spi_obj, 1);
    ret = rt_completion_wait(&control->transfer_completion,
    RT_TICK_PER_SECOND * 50);
    if (ret)
    {
        rt_kprintf("ERROR: %s, transfer timeout\n", __func__);
        return -RT_ETIMEOUT;
    }
    spi_wait_tx_only_done(control);
    return RT_EOK;
}

static inline rt_uint32_t tx_max_tx_only(struct spi_controller *spi_control,
        rt_uint32_t tx_total_len)
{
    struct fh_spi_obj *spi_obj;
    rt_uint32_t hw_tx_level;

    spi_obj = &spi_control->obj;
    hw_tx_level = SPI_ReadTxFifoLevel(spi_obj);
    hw_tx_level = spi_obj->tx_fifo_len - hw_tx_level;
    return MIN(hw_tx_level, tx_total_len);
}

static inline rt_uint32_t tx_max(struct spi_controller *spi_control,
        rt_uint32_t tx_total_len)
{

    struct fh_spi_obj *spi_obj;

    spi_obj = &spi_control->obj;
    rt_uint32_t hw_tx_level, hw_rx_level;
    rt_uint32_t temp_tx_lev;
    temp_tx_lev = SPI_ReadTxFifoLevel(spi_obj);
    hw_rx_level = temp_tx_lev + SPI_ReadRxFifoLevel(spi_obj);
    if (hw_rx_level >= spi_obj->tx_fifo_len)
    {
        return 0;
    }

    hw_rx_level++;
    hw_tx_level = temp_tx_lev;
    hw_tx_level = spi_obj->tx_fifo_len - hw_tx_level;
    hw_rx_level = spi_obj->tx_fifo_len - hw_rx_level;
    return MIN(MIN(hw_tx_level, tx_total_len), hw_rx_level);
}

static inline rt_uint32_t rx_max(struct spi_controller *spi_control)
{
    rt_uint32_t hw_rx_level;
    struct fh_spi_obj *spi_obj;

    spi_obj = &spi_control->obj;

    hw_rx_level = SPI_ReadRxFifoLevel(spi_obj);
    return hw_rx_level;
}


static int poll_rx_only_with_regwidth(struct spi_controller *spi_control,
rt_uint8_t *rxbuf, rt_uint32_t size, rt_uint32_t reg_width)
{
    register rt_uint32_t rx_fifo_capability;
    rt_uint32_t otp_xfer_size;
    rt_uint8_t *rxbuf_8;
    rt_uint16_t *rxbuf_16;
    rt_uint32_t *rxbuf_32;
    struct fh_spi_obj *spi_obj;

    spi_obj = &spi_control->obj;
    rxbuf_8 = (rt_uint8_t *)rxbuf;
    rxbuf_16  = (rt_uint16_t *)rxbuf;
    rxbuf_32  = (rt_uint32_t *)rxbuf;
    if (reg_width == 16)
    {
        Spi_SetSwap(spi_obj, 1);
        Spi_SetWidth(spi_obj, 16);
    }

    if (reg_width == 32)
    {
        Spi_SetSwap(spi_obj, 1);
        Spi_SetWidth(spi_obj, 32);
    }

    /* refix size div reg_width */
    size = size / (reg_width / 8);
start:
    /* or rx fifo error.. */
    if (size == 0)
    {
        if ((reg_width == 16) || (reg_width == 32))
        {
            Spi_SetSwap(spi_obj, 0);
            Spi_SetWidth(spi_obj, 8);
        }
        return 0;
    }

    otp_xfer_size = MIN(spi_obj->rx_fifo_len, size);
    size -= otp_xfer_size;
    SPI_Enable(spi_obj, 0);
    SPI_ContinueReadNum(spi_obj, otp_xfer_size);
    SPI_Enable(spi_obj, 1);
    SPI_WriteData(spi_obj, 0xffffffff);
    do
    {
        rx_fifo_capability = rx_max(spi_control);
        otp_xfer_size -= rx_fifo_capability;
        while (rx_fifo_capability)
        {
            if (reg_width == 32)
                *rxbuf_32++ = SPI_ReadData_32(spi_obj);
            else if (reg_width == 16)
                *rxbuf_16++ = SPI_ReadData_16(spi_obj);
            else
                *rxbuf_8++ = SPI_ReadData(spi_obj);
            rx_fifo_capability--;
        }
    }while (otp_xfer_size);
    goto start;
}

#ifdef RT_SFUD_USING_QSPI
static int poll_pump_rx_only_data_qspi(struct spi_controller *spi_control)
{
    /* struct rt_qspi_message *p_qspi_m;    */
    rt_uint32_t size;
    rt_uint32_t data_width;
    struct rt_spi_message *crt_mes;
    int ret;

    /* p_qspi_m = spi_control->cur_qspi_m;  */
    crt_mes = spi_control->current_message;
    size = crt_mes->length;

    data_width = rx_only_fix_data_width(spi_control, size);
    ret = poll_rx_only_with_regwidth(spi_control, crt_mes->recv_buf, size, data_width);
    return ret;
}
#endif

static int poll_pump_rx_only_data(struct spi_controller *spi_control)
{
    rt_uint32_t size;
    rt_uint32_t data_width;
    rt_uint32_t rx_address;
    int ret;
    struct rt_spi_message *crt_mes;
    crt_mes = spi_control->current_message;
    size = crt_mes->length;
    rx_address = crt_mes->recv_buf;

    if (spi_control->plat_data->swap_support != SWAP_SUPPORT)
        data_width = 8;
    else if (((rx_address % 4) == 0) && ((size % 4) == 0))
        data_width = 32;
    else if (((rx_address % 2) == 0) && ((size % 2) == 0))
        data_width = 16;
    else
        data_width = 8;

    ret = poll_rx_only_with_regwidth(spi_control, crt_mes->recv_buf, size, data_width);
    return ret;

}

static int poll_pump_tx_rx_data(struct spi_controller *control)
{
    struct spi_controller *spi_control;
    struct fh_spi_obj *spi_obj;
    struct rt_spi_message *crt_mes;

    spi_control = control;
    spi_obj = &spi_control->obj;
    register rt_uint32_t rx_fifo_capability, tx_fifo_capability;
    rt_uint8_t *rxp;
    rt_uint8_t *txp;
    rt_uint32_t dummy_data = 0xff;
    rt_uint32_t rx_xfer_len;
    rt_uint32_t tx_xfer_len;

    crt_mes = spi_control->current_message;
    tx_xfer_len = rx_xfer_len = crt_mes->length;
    rxp = crt_mes->recv_buf;
    txp = (rt_uint8_t *) crt_mes->send_buf;

    register rt_uint32_t data_reg;
    rt_uint32_t rx_lev_reg;

    data_reg = spi_obj->base + OFFSET_SPI_DR;
    rx_lev_reg = spi_obj->base + OFFSET_SPI_RXFLR;

    goto first;

start:
    rx_fifo_capability = GET_REG(rx_lev_reg);
    rx_xfer_len -= rx_fifo_capability;
    if (rxp != NULL)
    {
        while (rx_fifo_capability)
        {
            /* *rxp++ = SPI_ReadData(spi_obj); */
            *rxp++ = (rt_uint8_t) GET_REG(data_reg);
            rx_fifo_capability--;
        }
    }
    else
    {
        while (rx_fifo_capability)
        {
            dummy_data = GET_REG(data_reg);
            /* just lie to compile.... */
            dummy_data--;
            rx_fifo_capability--;
        }
    }

    if (rx_xfer_len == 0)
    {
        return RT_EOK;
    }

first:
    tx_fifo_capability = tx_max(spi_control, tx_xfer_len);
    tx_xfer_len -= tx_fifo_capability;
    if (txp != NULL)
    {
        while (tx_fifo_capability)
        {
            SET_REG(data_reg, *txp++);
            tx_fifo_capability--;
        }

    }
    else
    {
        while (tx_fifo_capability)
        {
            SET_REG(data_reg, 0xff);
            tx_fifo_capability--;
        }
    }

    goto start;
}


#ifdef RT_SFUD_USING_QSPI
static int poll_pump_tx_only_data_qspi(struct spi_controller *fh_spi)
{
    struct fh_spi_obj *spi_obj;
    struct s_qspi_send_data *p_send;
    rt_uint32_t tx_xfer_len, tx_fifo_capability, data_reg;
    rt_uint8_t *txp;

    spi_obj = &fh_spi->obj;
    data_reg = spi_obj->base + OFFSET_SPI_DR;
    p_send = &fh_spi->qspi_send_data;
    txp = (rt_uint8_t *) &p_send->buf[0];
    tx_xfer_len = p_send->data_size;
    while (tx_xfer_len)
    {
        tx_fifo_capability = tx_max_tx_only(fh_spi, tx_xfer_len);
        tx_xfer_len -= tx_fifo_capability;
        while (tx_fifo_capability)
        {
            SET_REG(data_reg, *txp++);
            tx_fifo_capability--;
        }
    }
    spi_wait_tx_only_done(fh_spi);
    return 0;
}
#endif

static int poll_pump_tx_only_data(struct spi_controller *fh_spi){
    struct fh_spi_obj *spi_obj;
    struct rt_spi_message *crt_mes;
    rt_uint32_t tx_xfer_len,tx_fifo_capability,data_reg;
    rt_uint8_t *txp;
    spi_obj = &fh_spi->obj;
    data_reg = spi_obj->base + OFFSET_SPI_DR;
    crt_mes = fh_spi->current_message;
    txp = (rt_uint8_t *) crt_mes->send_buf;
    tx_xfer_len = crt_mes->length;
    while (tx_xfer_len)
    {
        tx_fifo_capability = tx_max_tx_only(fh_spi, tx_xfer_len);
        tx_xfer_len -= tx_fifo_capability;
        while (tx_fifo_capability)
        {
            SET_REG(data_reg, *txp++);
            tx_fifo_capability--;
        }
    }
    spi_wait_tx_only_done(fh_spi);
    return 0;
}


rt_uint32_t xfer_data_poll(struct spi_controller *control)
{
    int ret = -1;
    struct fh_spi_obj *spi_obj;
#ifdef RT_SFUD_USING_QSPI
    struct rt_qspi_message *cur_qspi_m;
#endif
    spi_obj = &control->obj;

#ifdef RT_SFUD_USING_QSPI
    if (get_multi_wire_info(control))
    {
        cur_qspi_m = control->cur_qspi_m;

        control->obj.config.transfer_mode = SPI_MODE_TX_ONLY;
        SPI_Enable(spi_obj, 0);
        SPI_SetTransferMode(spi_obj, control->obj.config.transfer_mode);
        SPI_Enable(spi_obj, 1);
        /*first tx only...*/
        poll_pump_tx_only_data_qspi(control);
        /*then rx only...*/
        if (!control->current_message->recv_buf)
        {
            return 0;
        }
        control->obj.config.transfer_mode = SPI_MODE_RX_ONLY;
        SPI_Enable(spi_obj, 0);
        SPI_SetTransferMode(spi_obj, control->obj.config.transfer_mode);
        SPI_Enable(spi_obj, 1);
        /*parse multi wire here..*/
        if (cur_qspi_m->qspi_data_lines == 4)
        {
            if (control->change_to_4_wire)
                control->change_to_4_wire(&control->spi_bus, SPI_DATA_DIR_IN);
        }
        else if (cur_qspi_m->qspi_data_lines == 2)
        {
            if (control->change_to_2_wire)
                control->change_to_2_wire(&control->spi_bus, SPI_DATA_DIR_IN);
        }
        else
        {
            if (control->change_to_1_wire)
                control->change_to_1_wire(&control->spi_bus);
        }
        poll_pump_rx_only_data_qspi(control);
        if (control->change_to_1_wire)
            control->change_to_1_wire(&control->spi_bus);
        return 0;
    }
    else
#endif
    {
        spi_ctl_fix_pump_data_mode(control);
        /*if multi open ,recheck mode..*/
        fix_multi_xfer_mode(control);
        SPI_Enable(spi_obj, 0);
        SPI_SetTransferMode(spi_obj, control->obj.config.transfer_mode);
        SPI_Enable(spi_obj, 1);
        if (control->obj.config.transfer_mode == SPI_MODE_TX_ONLY)
            ret = poll_pump_tx_only_data(control);
        else if (control->obj.config.transfer_mode == SPI_MODE_RX_ONLY)
            ret = poll_pump_rx_only_data(control);
        else
            ret = poll_pump_tx_rx_data(control);
        return ret;
    }


}

void fix_spi_xfer_mode(struct spi_controller *spi_control)
{
    /* switch dma or isr....first check dma ...is error .use isr xfer... */
    /* retry to check if the dma status... */
    unsigned int dma_lev = 0;
    if (spi_control->plat_data->transfer_mode != USE_POLL_TRANSFER)
    {
        if (spi_control->dma.dma_flag == DMA_BIND_OK)
        {
            /* if transfer data too short...use poll.. */
            if(spi_control->plat_data->dma_xfer_threshold == 0)
                dma_lev = DMA_OR_ISR_THRESHOLD;
            else
                dma_lev = spi_control->plat_data->dma_xfer_threshold;

            if (spi_control->current_message->length < dma_lev)
            {
                spi_control->xfer_mode = XFER_USE_POLL;
                return;
            }
            spi_control->xfer_mode = XFER_USE_DMA;
            /* if error use isr mode */
        }
        else
            spi_control->xfer_mode = XFER_USE_POLL;
    }
    else
    {
        spi_control->xfer_mode = XFER_USE_POLL;
    }

}
#if (0)
void fix_fh_spi_xfer_wire_mode(struct rt_spi_device *spi_dev, struct rt_spi_multi_wire_xfer *p_xfer)
{
    struct rt_spi_bus *spi_master;
    struct _spi_advanced_info *p_info;
    spi_master = spi_dev->bus;
    p_info = &spi_master->ctl_multi_wire_info;
    SSI_ASSERT(spi_dev->dev_open_multi_wire_flag <=
    spi_master->ctl_multi_wire_info.ctl_wire_support);
    if (p_xfer->xfer_wire_mode == ONE_WIRE_SUPPORT)
        p_info->change_to_1_wire(spi_master);
    else if (p_xfer->xfer_wire_mode == DUAL_WIRE_SUPPORT)
        p_info->change_to_2_wire(spi_master, p_xfer->xfer_dir);
    else if (p_xfer->xfer_wire_mode == QUAD_WIRE_SUPPORT)
        p_info->change_to_4_wire(spi_master, p_xfer->xfer_dir);
}
#endif

#ifdef RT_SFUD_USING_QSPI
void fh_pre_parse_rtt_qspi_mes_adapt(struct rt_spi_device *device)
{
    /*
     * parse
     * ins(assert 1 line),
     * add(assert 1 line),
     * alternate(assert 1 line),
     * dummy(assert 1 line),
     * sendbuf(assert 1 line)
     */
    struct rt_spi_message *message;
    struct spi_slave_info *spi_slave;
    struct spi_controller *spi_control;
    /* struct fh_spi_obj *spi_obj;  */
    struct rt_qspi_message *p_qspi_m;
    struct s_qspi_send_data *p_send;
    int size = 0;
    int i = 0;

    spi_slave = (struct spi_slave_info *) fh_get_spi_dev_pri_data(device);
    spi_control = spi_slave->control;
    message = spi_control->current_message;

    p_qspi_m = spi_control->cur_qspi_m;
    p_send = &spi_control->qspi_send_data;
    SSI_ASSERT(p_qspi_m->instruction.qspi_lines <= 1);
    SSI_ASSERT(p_qspi_m->address.qspi_lines <= 1);
    /*check total size send..*/
    /*ins = 1;*/
    size++;
    size += p_qspi_m->address.size / 8;
    size += p_qspi_m->dummy_cycles / 8;
    if (!message->recv_buf)
    {
        size += message->length;
    }

    SSI_ASSERT(size <= MAX_QSPI_SEND_BUF_SIZE);
    /*cpy to local buf..*/
    /*cpy ins*/
    rt_memcpy(&p_send->buf[i], &p_qspi_m->instruction.content, 1);
    i += 1;
    /*cpy add*/

    if (p_qspi_m->address.size == 32)
    {
        p_send->buf[i] = (unsigned char)(p_qspi_m->address.content >> 24);
        p_send->buf[i + 1] = (unsigned char)(p_qspi_m->address.content >> 16);
        p_send->buf[i + 2] = (unsigned char)(p_qspi_m->address.content >> 8);
        p_send->buf[i + 3] = (unsigned char)p_qspi_m->address.content;
    }
    else if (p_qspi_m->address.size == 24)
    {
        p_send->buf[i] = (unsigned char)(p_qspi_m->address.content >> 16);
        p_send->buf[i + 1] = (unsigned char)(p_qspi_m->address.content >> 8);
        p_send->buf[i + 2] = (unsigned char)(p_qspi_m->address.content);
    }
    else if (p_qspi_m->address.size == 8)
    {
        p_send->buf[i] = (unsigned char)(p_qspi_m->address.content);
    }
    i += p_qspi_m->address.size / 8;
    /*set dummy*/
    rt_memset(&p_send->buf[i], 0xff, p_qspi_m->dummy_cycles / 8);
    i += p_qspi_m->dummy_cycles / 8;
    if (!message->recv_buf)
    {
        rt_memcpy(&p_send->buf[i], message->send_buf, message->length);
        i += message->length;
    }
    p_send->data_size = i;
    p_send->data_lines = 1;

}
#endif


static rt_uint32_t fh_spi_xfer(struct rt_spi_device *device,
        struct rt_spi_message *message)
{
    /* rt_base_t level; */
    struct spi_slave_info *spi_slave;
    struct spi_controller *spi_control;
    struct fh_spi_obj *spi_obj;
    int ret;
    unsigned int raw_status;
    /* int pts1,pts2; */
    spi_slave = (struct spi_slave_info *) fh_get_spi_dev_pri_data(device);
    spi_control = spi_slave->control;


    rt_sem_take(&spi_control->xfer_lock, RT_WAITING_FOREVER);
    spi_obj = &spi_control->obj;
    spi_control->transfered_len = 0;
    spi_control->received_len = 0;
    spi_control->active_spi_dev = device;
    rt_completion_init(&spi_control->transfer_completion);

    spi_control->current_message = message;
    /* take CS */
    if (message->cs_take)
    {
        if (spi_slave->plat_slave.actice_level == ACTIVE_LOW)
            gpio_direction_output(spi_slave->plat_slave.cs_pin, 0);
        else
            gpio_direction_output(spi_slave->plat_slave.cs_pin, 1);

        /* here will always use the slave_0 because that the cs is gpio... */
        SPI_EnableSlaveen(spi_obj, 0);
    }
#ifdef RT_SFUD_USING_QSPI
    if (get_multi_wire_info(spi_control))
    {
        struct rt_qspi_message *p_qspi_m = (struct rt_qspi_message *)message;

        spi_control->cur_qspi_m = p_qspi_m;
        fh_pre_parse_rtt_qspi_mes_adapt(device);
    }
#endif

    /* fix transfer mode ..... */
    fix_spi_xfer_mode(spi_control);
    if (spi_control->xfer_mode == XFER_USE_DMA)
    {
        if (message->recv_buf)
        {
            if (((unsigned int)message->recv_buf % 32) || (message->length % 32))
            {
                spi_control->xfer_mode = XFER_USE_POLL;
            }
        }
    }

    switch (spi_control->xfer_mode)
    {
    case XFER_USE_DMA:
        ret = xfer_data_dma(spi_control);
        if (ret == RT_EOK)
        {
            break;
        }
        else
        {
            raw_status = SPI_RawInterruptStatus(spi_obj);
            rt_kprintf("%s dma transfer error no:%x, raw status is %x\n", __func__, ret, raw_status);
            SSI_ASSERT(0);
        }
    case XFER_USE_ISR:
#ifdef RT_SFUD_USING_QSPI
        rt_kprintf("%s : %d isr mode only support one wire..\n",__func__, __LINE__);
        SSI_ASSERT(0);
#endif
        ret = xfer_data_isr(spi_control);
        if (ret != RT_EOK)
            rt_kprintf("%s isr transfer error no:%x\n", __func__, ret);
#if (0)
        if (spi_control->plat_data->ctl_wire_support & (DUAL_WIRE_SUPPORT | QUAD_WIRE_SUPPORT))
        {
            #ifdef RT_SFUD_USING_QSPI
            rt_kprintf("%s : %d isr mode only support one wire..\n",__func__, __LINE__);
            SSI_ASSERT(0);
            #endif
        }

        ret = xfer_data_isr(spi_control);
        if (ret != RT_EOK)
            rt_kprintf("%s isr transfer error no:%x\n", __func__, ret);
#endif
        break;

    case XFER_USE_POLL:
        ret = xfer_data_poll(spi_control);
        break;

    default:
        rt_kprintf("%s unknow xfer func...\n", __func__);
        while (1)
            ;
    }

    /* release CS */
    if (message->cs_release)
    {
        if (spi_slave->plat_slave.actice_level == ACTIVE_LOW)
            gpio_direction_output(spi_slave->plat_slave.cs_pin, 1);
        else
            gpio_direction_output(spi_slave->plat_slave.cs_pin, 0);
        SPI_DisableSlaveen(spi_obj, 0);
    }

    rt_sem_release(&spi_control->xfer_lock);
    PRINT_SPI_DBG("%s end\n", __func__);

    return message->length;
}

static struct rt_spi_ops fh_spi_ops = {

.configure = fh_spi_configure, .xfer = fh_spi_xfer,

};

static void fh_spi_interrupt(int irq, void *param)
{
    struct spi_controller *spi_control;
    struct fh_spi_obj *spi_obj;
    rt_uint8_t *p;
    spi_control = (struct spi_controller *) param;
    spi_obj = &spi_control->obj;
    rt_uint32_t rx_fifo_capability, tx_fifo_capability,isr_status;
    isr_status = SPI_InterruptStatus(spi_obj);
    if (isr_status & (SPI_IRQ_TXOIM | SPI_IRQ_RXUIM | SPI_IRQ_RXOIM))
    {
        SPI_ClearInterrupt(spi_obj);
        rt_kprintf("%s isr err:spi isr status:%x\n",__func__,isr_status);
        SSI_ASSERT(0);
    }
    SPI_DisableInterrupt(spi_obj, SPI_IRQ_TXEIM);

    if (spi_control->obj.config.transfer_mode == SPI_MODE_TX_RX)
    {
        tx_fifo_capability = tx_max(spi_control,
                spi_control->current_message->length - spi_control->transfered_len);
        rx_fifo_capability = rx_max(spi_control);
        PRINT_SPI_DBG(" tx cap=%d, rx cap=%d, total len=%d\n", tx_fifo_capability,
                rx_fifo_capability, spi_control->current_message->length);
        spi_control->received_len += rx_fifo_capability;
        while (rx_fifo_capability)
        {
            *(rt_uint8_t *) spi_control->current_message->recv_buf++ =
                    SPI_ReadData(spi_obj);
            rx_fifo_capability--;
        }
        if (spi_control->received_len == spi_control->current_message->length)
        {
            SPI_DisableInterrupt(spi_obj, SPI_IRQ_TXEIM);
            PRINT_SPI_DBG("finished, length=%d, received_len=%d\n",
                    spi_control->current_message->length,
                    spi_control->received_len);
            rt_completion_done(&spi_control->transfer_completion);
            return;
        }

        spi_control->transfered_len += tx_fifo_capability;
        if (spi_control->current_message->send_buf)
        {
            p = (rt_uint8_t *) spi_control->current_message->send_buf;
            while (tx_fifo_capability)
            {
                SPI_WriteData(spi_obj, *p++);
                tx_fifo_capability--;
            }
            spi_control->current_message->send_buf = p;
        }
        else
        {
            while (tx_fifo_capability)
            {
                SPI_WriteData(spi_obj, 0xff);
                tx_fifo_capability--;
            }
        }
    }
    else if (spi_control->obj.config.transfer_mode == SPI_MODE_TX_ONLY)
    {
        tx_fifo_capability = tx_max_tx_only(spi_control,
        spi_control->current_message->length - spi_control->transfered_len);
        spi_control->transfered_len += tx_fifo_capability;
        p = (rt_uint8_t *) spi_control->current_message->send_buf;
        while (tx_fifo_capability)
        {
            SPI_WriteData(spi_obj, *p++);
            tx_fifo_capability--;
        }
        spi_control->current_message->send_buf = p;
        if (spi_control->transfered_len == spi_control->current_message->length)
        {
            SPI_DisableInterrupt(spi_obj, SPI_IRQ_TXEIM);
            rt_completion_done(&spi_control->transfer_completion);
            return;
        }

    }
    SPI_EnableInterrupt(spi_obj, SPI_IRQ_TXEIM);
}

void fh_spi_check_rxfifo_depth(struct spi_controller *spi_control)
{
    rt_uint32_t w_i, r_i, ori;
    struct fh_spi_obj *spi_obj;

    spi_obj = &spi_control->obj;
    ori = SPI_GetRxLevel(spi_obj);
    for (r_i = w_i = RX_FIFO_MIN_LEN; w_i < RX_FIFO_MAX_LEN; r_i++)
    {
        SPI_SetRxLevel(spi_obj, ++w_i);
        if (r_i == SPI_GetRxLevel(spi_obj))
            break;
    }
    SPI_SetRxLevel(spi_obj, ori);
    spi_control->obj.rx_fifo_len = r_i + 1;
}

void fh_spi_check_txfifo_depth(struct spi_controller *spi_control)
{
    rt_uint32_t w_i, r_i, ori;
    struct fh_spi_obj *spi_obj;

    spi_obj = &spi_control->obj;

    ori = SPI_GetTxLevel(spi_obj);
    for (r_i = w_i = TX_FIFO_MIN_LEN; w_i < TX_FIFO_MAX_LEN; r_i++)
    {
        SPI_SetTxLevel(spi_obj, ++w_i);
        if (r_i == SPI_GetTxLevel(spi_obj))
            break;
    }
    SPI_SetTxLevel(spi_obj, ori);
    spi_control->obj.tx_fifo_len = r_i + 1;
}

/*****
 *
 * spi multi wire func.
 *
 *****/
void fh_spic_check_idle(struct fh_spi_obj *spi_obj)
{
    rt_uint32_t status;

    status = SPI_ReadStatus(spi_obj);
    /*ahb rx fifo not empty..*/
    SSI_ASSERT(!(status & 1<<10));
    /*ahb tx fifo empty..*/
    SSI_ASSERT((status & 1<<9) == 1<<9);
    /*apb rx fifo*/
    SSI_ASSERT((status & 1<<3) == 0);
    /*apb tx fifo*/
    SSI_ASSERT((status & 1<<2) == 1<<2);
    /*shift not busy..*/
    SSI_ASSERT((status & 1) == 0);
}

int spic_wire_init(struct rt_spi_bus *p_master)
{
    struct spi_controller *fh_spi;
    struct fh_spi_obj *spi_obj;
    fh_spi = fh_spi_get_private_data(p_master);
    spi_obj = &fh_spi->obj;

    Spi_SetXip(spi_obj, XIP_DISABLE);
    Spi_SetDPI(spi_obj, DPI_DISABLE);
    Spi_SetQPI(spi_obj, QPI_DISABLE);
    Spi_TimingConfigure(spi_obj, 0x0);
    Spi_SetApbReadWireMode(spi_obj, FH_STANDARD_READ);
    Spi_SetClk_Masken(spi_obj, 1);
    SPI_SetRxLevel(spi_obj, fh_spi->obj.rx_fifo_len - 2);

    return 0;
}

void spi_bus_change_1_wire(struct rt_spi_bus *p_master)
{
    struct spi_controller *fh_spi;
    struct fh_spi_obj *spi_obj;

    fh_spi = fh_spi_get_private_data(p_master);
    spi_obj = &fh_spi->obj;

    fh_spi->active_wire_width = ONE_WIRE_SUPPORT;
    fh_spi->dir = SPI_DATA_DIR_DUOLEX;
    fh_spic_check_idle(spi_obj);
    SPI_Enable(spi_obj, 0);
    Spi_SetApbReadWireMode(spi_obj, FH_STANDARD_READ);
    SPI_Enable(spi_obj, 1);
}

void spi_bus_change_2_wire(struct rt_spi_bus *p_master, unsigned int dir)
{
    struct spi_controller *fh_spi;
    struct fh_spi_obj *spi_obj;
    fh_spi = fh_spi_get_private_data(p_master);
    spi_obj = &fh_spi->obj;

    fh_spi->active_wire_width = DUAL_WIRE_SUPPORT;
    fh_spi->dir = dir;
    fh_spic_check_idle(spi_obj);
    SPI_Enable(spi_obj, 0);
    Spi_SetApbReadWireMode(spi_obj, FH_DUAL_OUTPUT);
    SPI_Enable(spi_obj, 1);
}

void spi_bus_change_4_wire(struct rt_spi_bus *p_master, unsigned int dir)
{
    struct spi_controller *fh_spi;
    struct fh_spi_obj *spi_obj;
    fh_spi = fh_spi_get_private_data(p_master);
    spi_obj = &fh_spi->obj;

    fh_spi->active_wire_width = QUAD_WIRE_SUPPORT;
    fh_spi->dir = dir;
    fh_spic_check_idle(spi_obj);
    SPI_Enable(spi_obj, 0);
    Spi_SetApbReadWireMode(spi_obj, FH_QUAD_OUTPUT);
    SPI_Enable(spi_obj, 1);
}

#ifdef RT_USING_PM
int fh_spi_suspend(const struct rt_device *device, rt_uint8_t mode)
{
    struct rt_spi_bus *spi_bus = rt_container_of(device, struct rt_spi_bus, parent);
    struct spi_controller *fh_spi = rt_container_of(spi_bus, struct spi_controller, spi_bus);
    struct spi_control_platform_data *plat_data = fh_spi->plat_data;
    struct clk *clk = (struct clk *)clk_get(NULL, plat_data->clk_name);

    rt_sem_take(&fh_spi->xfer_lock, RT_WAITING_FOREVER);

    return 0;
}

void fh_spi_resume(const struct rt_device *device, rt_uint8_t mode)
{
    struct rt_spi_bus *spi_bus = rt_container_of(device, struct rt_spi_bus, parent);
    struct spi_controller *fh_spi = rt_container_of(spi_bus, struct spi_controller, spi_bus);
    struct spi_config config = fh_spi->obj.config;
    struct spi_control_platform_data *plat_data = fh_spi->plat_data;
    struct clk *clk = (struct clk *)clk_get(NULL, plat_data->clk_name);

    clk_enable(clk);

    SPI_Enable(&fh_spi->obj, 0);
    SPI_SampleDly(&fh_spi->obj, 1);
    SPI_SetParameter(&fh_spi->obj);
    SPI_DisableInterrupt(&fh_spi->obj, SPI_IRQ_ALL);
    if (config.hs_mode == RT_TRUE)
    {
        SPI_SetRxCaptureMode(&fh_spi->obj, 1);
        SPI_SampleDly(&fh_spi->obj, 3);
    }
#ifdef RT_SFUD_USING_QSPI
    if (get_multi_wire_info(fh_spi))
        fh_spi->multi_wire_func_init(&fh_spi->spi_bus);
#endif
    SPI_Enable(&fh_spi->obj, 1);
    rt_sem_release(&fh_spi->xfer_lock);
}

struct rt_device_pm_ops fh_spi_pm_ops = {
    .suspend_prepare = fh_spi_suspend,
    .resume_prepare = fh_spi_resume,
};
#endif

int fh_spi_probe(void *priv_data)
{
    char spi_dev_name[20] = { 0 };
    char spi_bus_name[20] = { 0 };
    char spi_isr_name[20] = { 0 };
    char spi_lock_name[20] = { 0 };

    struct spi_slave_info *spi_slave;
    struct spi_slave_info *next_slave;
    struct spi_slave_info **control_slave;
    struct spi_controller *spi_control;
    struct spi_control_platform_data *plat_data;
    int i, ret;
    struct rt_dma_device *rt_dma_dev;
    struct dma_transfer *tx_trans;
    struct dma_transfer *rx_trans;
#ifdef RT_SFUD_USING_QSPI
    struct fh_spi_obj *spi_obj;
#endif
    struct clk *clk;

    /* check data... */
    plat_data = (struct spi_control_platform_data *) priv_data;

    if (!plat_data)
    {
        rt_kprintf("ERROR:platform data null...\n");
        return -RT_ENOMEM;
    }
    if (plat_data->slave_no > FH_SPI_SLAVE_MAX_NO)
    {
        rt_kprintf("ERROR:spi controller not support %d slave..\n",
                plat_data->slave_no);
        return -RT_ENOMEM;
    }
    clk = (struct clk *)clk_get(NULL, plat_data->clk_name);
    if (clk)
    {
        clk_set_rate(clk, plat_data->clk_in);
        clk_enable(clk);
    }

    /* malloc data */
    spi_control = (struct spi_controller *) rt_malloc(
            sizeof(struct spi_controller));
    if (!spi_control)
    {
        rt_kprintf("ERROR:no mem for malloc the spi controller..\n");
        goto error_malloc_bus;
    }

    rt_memset(spi_control, 0, sizeof(struct spi_controller));
    /* parse platform control data */
    spi_control->base = plat_data->base;
    spi_control->id = plat_data->id;
    spi_control->irq = plat_data->irq;
    spi_control->max_hz = plat_data->max_hz;
    spi_control->slave_no = plat_data->slave_no;
    spi_control->obj.base = plat_data->base;
    spi_control->clk_in = plat_data->clk_in;
    spi_control->plat_data = plat_data;
    spi_control->spi_bus.max_speed_hz = spi_control->max_hz;

    check_spi_ctl_multi_wire_info(spi_control);
    check_spi_ctl_swap_info(spi_control);
    check_spi_ctl_protctl_info(spi_control);
    fh_spi_check_rxfifo_depth(spi_control);
    fh_spi_check_txfifo_depth(spi_control);
#ifdef RT_SFUD_USING_QSPI
    spi_obj = &spi_control->obj;
#endif
    PRINT_SPI_DBG("tx fifo %d, rx fifo %d\n", spi_control->obj.tx_fifo_len,
            spi_control->obj.rx_fifo_len);
    rt_sprintf(spi_lock_name, "%s%d", "spi_lock", spi_control->id);
    rt_sem_init(&spi_control->xfer_lock, spi_lock_name, 1,
    RT_IPC_FLAG_FIFO);

    rt_sprintf(spi_bus_name, "%s%d", "spi_bus", spi_control->id);
    fh_spi_set_private_data(&spi_control->spi_bus, spi_control);
#ifdef RT_SFUD_USING_QSPI
    if (get_multi_wire_info(spi_control))
    {
        spi_control->multi_wire_func_init = spic_wire_init;
        spi_control->change_to_1_wire = spi_bus_change_1_wire;
        spi_control->change_to_2_wire = spi_bus_change_2_wire;
        spi_control->change_to_4_wire = spi_bus_change_4_wire;
        SPI_Enable(spi_obj, 0);
        spi_control->multi_wire_func_init(&spi_control->spi_bus);
        SPI_Enable(spi_obj, 1);
        ret = rt_qspi_bus_register(&spi_control->spi_bus, spi_bus_name, &fh_spi_ops);
    }
    else
#endif
    {
        ret = rt_spi_bus_register(&spi_control->spi_bus, spi_bus_name, &fh_spi_ops);
    }

    PRINT_SPI_DBG("bus name is :%s\n", spi_bus_name);

    /* isr... */
    rt_sprintf(spi_isr_name, "%s%d", "ssi_isr", spi_control->id);
    rt_hw_interrupt_install(spi_control->irq, fh_spi_interrupt,
            (void *) spi_control, spi_isr_name);

    rt_hw_interrupt_umask(spi_control->irq);
    PRINT_SPI_DBG("isr name is :%s\n", spi_isr_name);

    /* check dma .... */
    if (plat_data->transfer_mode == USE_DMA_TRANSFER)
    {
        spi_control->dma.dma_dev = (struct rt_dma_device *) rt_device_find(
                plat_data->dma_name);
        if (spi_control->dma.dma_dev == RT_NULL)
        {
            rt_kprintf("can't find dma dev, name: %s\n", plat_data->dma_name);
            /* goto error_malloc_slave; */
            /* spi_control->dma_xfer_flag = USE_ISR_TRANSFER; */
            /* spi_control->dma.dma_flag = DMA_BIND_ERROR; */
            /* spi_control->xfer_mode = XFER_USE_ISR; */
            goto BIND_DMA_ERROR;
        }
        else
        {
            spi_control->dma.control = spi_control;
            spi_control->dma.rx_hs = plat_data->rx_hs_no;
            spi_control->dma.tx_hs = plat_data->tx_hs_no;
            spi_control->dma.dma_name = plat_data->dma_name;

            spi_control->dma.rx_dummy_buff = fh_dma_mem_malloc_align(
            MALLOC_DMA_MEM_SIZE,32,"spi_dma");
            if (!spi_control->dma.rx_dummy_buff)
            {
                rt_kprintf("malloc rx dma buff failed...\n");
                /* spi_control->xfer_mode = XFER_USE_ISR; */
                goto BIND_DMA_ERROR;
            }

            spi_control->dma.tx_dummy_buff = fh_dma_mem_malloc_align(
            MALLOC_DMA_MEM_SIZE,32,"spi_dma");
            if (!spi_control->dma.tx_dummy_buff)
            {
                rt_kprintf("malloc tx dma buff failed...\n");
                fh_dma_mem_free(spi_control->dma.rx_dummy_buff);
                /* spi_control->xfer_mode = XFER_USE_ISR; */
                goto BIND_DMA_ERROR;
            }

            if (((rt_uint32_t) spi_control->dma.tx_dummy_buff % 32)
                    || ((rt_uint32_t) spi_control->dma.rx_dummy_buff % 32))
            {
                rt_kprintf("dma malloc buff not allign..\n");
                fh_dma_mem_free(spi_control->dma.rx_dummy_buff);
                fh_dma_mem_free(spi_control->dma.tx_dummy_buff);
                goto BIND_DMA_ERROR;
            }
            rt_memset((void *) spi_control->dma.tx_dummy_buff, 0xff, MALLOC_DMA_MEM_SIZE);
            /* open dma dev. */
            spi_control->dma.dma_dev->ops->control(spi_control->dma.dma_dev,
            RT_DEVICE_CTRL_DMA_OPEN, RT_NULL);

            /* request channel */
            rt_dma_dev = spi_control->dma.dma_dev;
            /* first request channel */
            tx_trans = &spi_control->dma.tx_trans;
            rx_trans = &spi_control->dma.rx_trans;
            tx_trans->channel_number = TX_DMA_CHANNEL;
            rx_trans->channel_number = RX_DMA_CHANNEL;

            ret = rt_dma_dev->ops->control(rt_dma_dev,
            RT_DEVICE_CTRL_DMA_REQUEST_CHANNEL, (void *) rx_trans);
            if (ret != RT_EOK)
            {
                goto BIND_DMA_ERROR;
            }

            ret = rt_dma_dev->ops->control(rt_dma_dev,
            RT_DEVICE_CTRL_DMA_REQUEST_CHANNEL, (void *) tx_trans);
            if (ret != RT_EOK)
            {
                /* release tx channel... */
                rt_dma_dev->ops->control(rt_dma_dev,
                RT_DEVICE_CTRL_DMA_RELEASE_CHANNEL, (void *) &rx_trans);
                goto BIND_DMA_ERROR;
            }

            if (rx_trans->channel_number >= tx_trans->channel_number)
            {
                rt_kprintf(
                        "rx channel should be less than tx channel because of the dma process priority..\n");
                SSI_ASSERT(rx_trans->channel_number < tx_trans->channel_number);
            }

            /* spi_control->xfer_mode = XFER_USE_DMA; */
            spi_control->dma.dma_flag = DMA_BIND_OK;
        }
    }
    else
    {
BIND_DMA_ERROR: spi_control->dma.dma_flag = DMA_BIND_ERROR;
        /* spi_control->xfer_mode = XFER_USE_ISR; */
    }

    control_slave = &spi_control->spi_slave;
    for (i = 0; i < plat_data->slave_no; i++)
    {
        spi_slave = (struct spi_slave_info *) rt_malloc(
                sizeof(struct spi_slave_info));
        if (!spi_slave)
        {
            rt_kprintf("ERROR:no mem for malloc the spi_slave%d..\n", i);
            goto error_malloc_slave;
        }
        rt_memset(spi_slave, 0, sizeof(struct spi_slave_info));

        /* parse platform data... */
        spi_slave->id = i;
        /* bind to the spi control....will easy to find all the data... */
        spi_slave->control = spi_control;
        spi_slave->plat_slave.cs_pin = plat_data->plat_slave[i].cs_pin;
        spi_slave->plat_slave.actice_level =
                plat_data->plat_slave[i].actice_level;
        rt_sprintf(spi_dev_name, "%s%d%s%d", "ssi", spi_control->id, "_",
                spi_slave->id);

        *control_slave = spi_slave;
        control_slave = &spi_slave->next;

#ifdef RT_SFUD_USING_QSPI
        spi_slave->qspi_dev.enter_qspi_mode = 0;
        spi_slave->qspi_dev.exit_qspi_mode = 0;
        if (plat_data->ctl_wire_support & QUAD_WIRE_SUPPORT)
            spi_slave->qspi_dev.config.qspi_dl_width = 4;
        else if (plat_data->ctl_wire_support & DUAL_WIRE_SUPPORT)
            spi_slave->qspi_dev.config.qspi_dl_width = 2;
        else
            spi_slave->qspi_dev.config.qspi_dl_width = 1;
        PRINT_SPI_DBG("data width is %d\n",spi_slave->qspi_dev.config.qspi_dl_width);
        ret = rt_spi_bus_attach_device(&spi_slave->qspi_dev.parent, spi_dev_name,
                spi_bus_name, spi_slave);
        if (ret != RT_EOK)
        {
            rt_kprintf("register dev to bus failed...\n");
            goto error_malloc_slave;
        }
#else
        /* register slave dev... */
        ret = rt_spi_bus_attach_device(&spi_slave->spi_device, spi_dev_name,
                spi_bus_name, spi_slave);
        if (ret != RT_EOK)
        {
            rt_kprintf("register dev to bus failed...\n");
            goto error_malloc_slave;
        }
#endif

    }

    /* request gpio... */
    spi_slave = spi_control->spi_slave;
    while (spi_slave != RT_NULL)
    {
        next_slave = spi_slave->next;

        fh_select_gpio(spi_slave->plat_slave.cs_pin);
        ret = gpio_request(spi_slave->plat_slave.cs_pin);
        if (ret != 0)
        {
            rt_kprintf("request gpio_%d failed...\n",
                    spi_slave->plat_slave.cs_pin);
            goto error_malloc_slave;
        }

        PRINT_SPI_DBG("spi_slave info addr:%x,id:%d,cs:%d,active:%d\n",
                (rt_uint32_t)spi_slave, spi_slave->id,
                spi_slave->plat_slave.cs_pin,
                spi_slave->plat_slave.actice_level);
        spi_slave = next_slave;
    }

    /* this will be used in platform exit.. */
    plat_data->control = spi_control;

#ifdef RT_USING_PM
    rt_pm_device_register(&spi_control->spi_bus.parent, &fh_spi_pm_ops);
#endif

    return RT_EOK;

error_malloc_slave:
    /* free the slaveinfo already malloc */
    spi_slave = spi_control->spi_slave;
    while (spi_slave != RT_NULL)
    {
        next_slave = spi_slave->next;
        gpio_release(spi_slave->plat_slave.cs_pin);
        rt_spi_bus_detach_device(&spi_slave->spi_device);
        rt_free(spi_slave);
        spi_slave = next_slave;
    }
    /* mask isr */
    rt_hw_interrupt_mask(spi_control->irq);
    /* release sem .. */
    rt_sem_detach(&spi_control->xfer_lock);
    /* unregister spi bus */
    rt_spi_bus_unregister(&spi_control->spi_bus);
    /* free the control malloc . */
    rt_free(spi_control);

/* fixme:unregister spi_bus... */

error_malloc_bus : return -RT_ENOMEM;
}

int fh_spi_exit(void *priv_data)
{
    struct spi_controller *spi_control;
    struct spi_control_platform_data *plat_data;
    struct spi_slave_info *spi_slave;
    struct spi_slave_info *next_slave;

    plat_data = (struct spi_control_platform_data *) priv_data;
    spi_control = plat_data->control;
    spi_slave = spi_control->spi_slave;

    while (spi_slave != RT_NULL)
    {
        next_slave = spi_slave->next;
        gpio_release(spi_slave->plat_slave.cs_pin);
        rt_free(spi_slave);
        spi_slave = next_slave;
    }
    /* mask isr */
    rt_hw_interrupt_mask(spi_control->irq);
    /* release sem .. */
    rt_sem_detach(&spi_control->xfer_lock);

    /* free the control malloc . */
    rt_free(spi_control);
    /* fixme free all the malloc data ... */

    return 0;
}

struct fh_board_ops spi_driver_ops = {

.probe = fh_spi_probe, .exit = fh_spi_exit,

};

void rt_hw_spi_init(void)
{
    /* rt_kprintf("%s start\n", __func__); */
    PRINT_SPI_DBG("%s start\n", __func__);
    fh_board_driver_register("spi", &spi_driver_ops);
    PRINT_SPI_DBG("%s end\n", __func__);
    /* fixme: never release? */
}


