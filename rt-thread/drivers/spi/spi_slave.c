#include <rtthread.h>
#include <time.h>
#include "board_info.h"
#include "spi_slave.h"
#include "fh_clock.h"
#include "fh_spi.h"
#include "fh_dma.h"
#include "dma.h"
#include "dma_mem.h"
#include "mmu.h"

#define MALLOC_DMA_MEM_SIZE 32
#define SPI_SLAVE_CLK_NAME    "spi2_clk"
/* #define FH_SPI_SLAVE_DEBUG */

#if defined(FH_SPI_SLAVE_DEBUG)/* && defined(RT_DEBUG)*/
#define PRINT_SPI_SLAVE_DBG(fmt, args...)   \
    do                                \
    {                                 \
        rt_kprintf("FH_SPI_SLAVE_DEBUG: "); \
        rt_kprintf(fmt, ##args);      \
    } while (0)
#else
#define PRINT_SPI_SLAVE_DBG(fmt, args...) \
    do                              \
    {                               \
    } while (0)
#endif

static int spi_slave_wait_device_idle(struct fh_spi_obj *spi_slave_obj)
{
    rt_uint32_t tick, timeout;
    //struct fh_spi_obj *spi_slave_obj = spi_slave_drv->priv;

    tick    = rt_tick_get();
    timeout = tick + RT_TICK_PER_SECOND / 2;  /* 500ms */

    while (SPI_ReadStatus(spi_slave_obj) & SPI_STATUS_BUSY)
    {
        tick = rt_tick_get();
        if (tick > timeout)
        {
            rt_kprintf("ERROR: %s, wait device idle timeout\n", __func__);
            return -RT_ETIMEOUT;
        }
    }

    return 0;
}

static void spi_slave_xfer_done(void *arg)
{
    struct spi_slave_driver * spi_slave_drv;
    struct fh_spi_obj *spi_slave_obj;
    spi_slave_drv = (struct spi_slave_driver *)arg;
    spi_slave_obj = &spi_slave_drv->obj;
    spi_slave_drv->dma_complete_count++;
    if (spi_slave_drv->dma_complete_count >= 2)
    {
        spi_slave_drv->dma_complete_count = 0;
        SPI_DisableDma(spi_slave_obj, SPI_TX_DMA | SPI_RX_DMA);
        rt_completion_done(&spi_slave_drv->transfer_completion);
    }
}


static int spi_slave_dma_init(struct spi_slave_driver * spi_slave_drv)
{
    int ret;

    PRINT_SPI_SLAVE_DBG("%s start\n", __func__);
    struct rt_dma_device * dma_dev;
    spi_slave_drv->dma_dev = (struct rt_dma_device *)rt_device_find(spi_slave_drv->plat_data->dma_name);
    if (spi_slave_drv->dma_dev == RT_NULL)
    {
        rt_kprintf("ERROR: %s, can't find dma device\n", __func__);
        return -1;
    }
    dma_dev = spi_slave_drv->dma_dev;
    spi_slave_drv->dma_tx =
        (struct dma_transfer *)rt_malloc(sizeof(struct dma_transfer));
    spi_slave_drv->dma_rx =
        (struct dma_transfer *)rt_malloc(sizeof(struct dma_transfer));
    if(!spi_slave_drv->dma_tx || !spi_slave_drv->dma_rx){
        return -1;
    }
    rt_memset((void *)spi_slave_drv->dma_tx, 0x0, sizeof(struct dma_transfer));
    rt_memset((void *)spi_slave_drv->dma_rx, 0x0, sizeof(struct dma_transfer));
    spi_slave_drv->dma_buff_tx = (rt_uint8_t *)fh_dma_mem_malloc(MALLOC_DMA_MEM_SIZE);
    spi_slave_drv->dma_buff_rx = (rt_uint8_t *)fh_dma_mem_malloc(MALLOC_DMA_MEM_SIZE);
    rt_memset(spi_slave_drv->dma_buff_tx, 0xff, MALLOC_DMA_MEM_SIZE);
    rt_memset(spi_slave_drv->dma_buff_rx, 0xff, MALLOC_DMA_MEM_SIZE);
    dma_dev->ops->control(dma_dev, RT_DEVICE_CTRL_DMA_OPEN,
                                   RT_NULL);
    //request channel...
    spi_slave_drv->dma_tx->dma_number           = 0;
    spi_slave_drv->dma_rx->dma_number           = 0;
    spi_slave_drv->dma_tx->channel_number       = AUTO_FIND_CHANNEL;
    spi_slave_drv->dma_rx->channel_number       = AUTO_FIND_CHANNEL;

    ret = dma_dev->ops->control(dma_dev, RT_DEVICE_CTRL_DMA_REQUEST_CHANNEL,
                                         (void *)spi_slave_drv->dma_tx);
    if (ret)
    {
        rt_kprintf("ERROR: %s, request channel failed, ret: %d\n", __func__,
                   ret);
        return -RT_ENOMEM;
    }
    ret = dma_dev->ops->control(dma_dev, RT_DEVICE_CTRL_DMA_REQUEST_CHANNEL,
                                         (void *)spi_slave_drv->dma_rx);
    if (ret)
    {
        dma_dev->ops->control(dma_dev, RT_DEVICE_CTRL_DMA_RELEASE_CHANNEL,
                                       (void *)&spi_slave_drv->dma_tx);
        rt_kprintf("ERROR: %s, request channel failed, ret: %d\n", __func__,
                   ret);
        return -RT_ENOMEM;
    }
    return 0;

}

static rt_err_t fh_spi_slave_hw_init(rt_device_t dev)
{
    struct spi_slave_driver * spi_slave_drv;
    spi_slave_drv = (struct spi_slave_driver *)(dev->user_data);
    struct fh_spi_obj *spi_slave_obj = &spi_slave_drv->obj;
    spi_slave_obj->config.clk_phase     = SPI_PHASE_RX_FIRST;
    spi_slave_obj->config.clk_polarity  = SPI_POLARITY_LOW;
    spi_slave_obj->config.data_size     = SPI_DATA_SIZE_8BIT;
    spi_slave_obj->config.frame_format  = SPI_FORMAT_MOTOROLA;
    spi_slave_obj->config.transfer_mode = SPI_MODE_TX_RX;
    spi_slave_wait_device_idle(spi_slave_obj);
    SPI_Enable(spi_slave_obj, RT_FALSE);
    SPI_SetParameter(spi_slave_obj);
    SPI_DisableInterrupt(spi_slave_obj, SPI_IRQ_ALL);
    SPI_Enable(spi_slave_obj, RT_TRUE);
    return 0;
}

static rt_err_t fh_spi_slave_open(rt_device_t dev, rt_uint16_t oflag)
{
    return RT_EOK;
}

static rt_err_t fh_spi_slave_close(rt_device_t dev)
{
    return RT_EOK;
}

void spi_slave_xfer_data(struct spi_slave_driver * spi_slave_drv,struct spi_slave_xfer *p_xfer){
    PRINT_SPI_SLAVE_DBG("%s::%d\n",__func__,__LINE__);
    struct rt_dma_device *dma_dev = spi_slave_drv->dma_dev;
    struct fh_spi_obj *spi_slave_obj;
    spi_slave_obj = &spi_slave_drv->obj;
    rt_completion_init(&spi_slave_drv->transfer_completion);
    //tx channel...
    spi_slave_drv->dma_tx->fc_mode              = DMA_M2P;
    spi_slave_drv->dma_tx->dst_add              = (rt_uint32_t)(spi_slave_obj->base + OFFSET_SPI_DR);
    spi_slave_drv->dma_tx->dst_hs               = DMA_HW_HANDSHAKING;
    spi_slave_drv->dma_tx->dst_inc_mode         = DW_DMA_SLAVE_FIX;
    spi_slave_drv->dma_tx->dst_msize            = DW_DMA_SLAVE_MSIZE_1;
    spi_slave_drv->dma_tx->dst_per              = SPI2_TX;
    spi_slave_drv->dma_tx->dst_width            = DW_DMA_SLAVE_WIDTH_8BIT;
    if(!p_xfer->tx_buf_addr){
        spi_slave_drv->dma_tx->src_add              = (rt_uint32_t)spi_slave_drv->dma_buff_tx;
        spi_slave_drv->dma_tx->src_inc_mode         = DW_DMA_SLAVE_FIX;
    }
    else{
        spi_slave_drv->dma_tx->src_add              = (rt_uint32_t)p_xfer->tx_buf_addr;
        spi_slave_drv->dma_tx->src_inc_mode         = DW_DMA_SLAVE_INC;
        //cache 
        mmu_clean_dcache(p_xfer->tx_buf_addr,p_xfer->transfer_length);
    }
    spi_slave_drv->dma_tx->src_msize            = DW_DMA_SLAVE_MSIZE_1;
    spi_slave_drv->dma_tx->src_width            = DW_DMA_SLAVE_WIDTH_8BIT;
    spi_slave_drv->dma_tx->complete_callback    = (void *)spi_slave_xfer_done;
    spi_slave_drv->dma_tx->trans_len            = p_xfer->transfer_length;
    spi_slave_drv->dma_tx->complete_para        = spi_slave_drv;

    //rx channel...
    spi_slave_drv->dma_rx->fc_mode              = DMA_P2M;
    if(!p_xfer->rx_buf_addr){
        spi_slave_drv->dma_rx->dst_add              = (rt_uint32_t)spi_slave_drv->dma_buff_rx;
        spi_slave_drv->dma_rx->dst_inc_mode         = DW_DMA_SLAVE_FIX;
    }
    else{
        spi_slave_drv->dma_rx->dst_add              = (rt_uint32_t)p_xfer->rx_buf_addr;
        spi_slave_drv->dma_rx->dst_inc_mode         = DW_DMA_SLAVE_INC;
        //here first flush src cache...incase cache data to ddr..
        mmu_clean_dcache(p_xfer->rx_buf_addr,p_xfer->transfer_length);
    }
    spi_slave_drv->dma_rx->dst_msize            = DW_DMA_SLAVE_MSIZE_1;
    spi_slave_drv->dma_rx->dst_width            = DW_DMA_SLAVE_WIDTH_8BIT;
    spi_slave_drv->dma_rx->src_add              = (rt_uint32_t)(spi_slave_obj->base + OFFSET_SPI_DR);
    spi_slave_drv->dma_rx->src_inc_mode         = DW_DMA_SLAVE_FIX;
    spi_slave_drv->dma_rx->src_msize            = DW_DMA_SLAVE_MSIZE_1;
    spi_slave_drv->dma_rx->src_width            = DW_DMA_SLAVE_WIDTH_8BIT;
    spi_slave_drv->dma_rx->src_hs               = DMA_HW_HANDSHAKING;
    spi_slave_drv->dma_rx->src_per              = SPI2_RX;
    spi_slave_drv->dma_rx->complete_callback    = (void *)spi_slave_xfer_done;
    spi_slave_drv->dma_rx->trans_len            = p_xfer->transfer_length;
    spi_slave_drv->dma_rx->complete_para        = spi_slave_drv;
    SPI_Enable(spi_slave_obj, RT_FALSE);
    SPI_WriteTxDmaLevel(spi_slave_obj, 1);
    SPI_WriteRxDmaLevel(spi_slave_obj, 0);
    SPI_Enable(spi_slave_obj, RT_TRUE);
    SPI_EnableDma(spi_slave_obj, SPI_TX_DMA | SPI_RX_DMA);
    dma_dev->ops->control(dma_dev, RT_DEVICE_CTRL_DMA_SINGLE_TRANSFER,
                                   (void *)spi_slave_drv->dma_rx);
    dma_dev->ops->control(dma_dev, RT_DEVICE_CTRL_DMA_SINGLE_TRANSFER,
                                   (void *)spi_slave_drv->dma_tx);

}

static void process_slave_io_set_data(struct spi_slave_driver * spi_slave_drv,struct spi_slave_xfer *xfer){
    //check data..
    PRINT_SPI_SLAVE_DBG("%s::%d\n",__func__,__LINE__);
    if(xfer->tx_buf_addr){
        if((xfer->tx_buf_addr % 32) ||
        (xfer->transfer_length % 32)){
            rt_kprintf("%s : %d ; buf add: 0x%x or size : 0x%x should 32B allign..\n"
            ,__func__,__LINE__,xfer->tx_buf_addr,xfer->transfer_length);
        }
    }
    if(xfer->rx_buf_addr){
        if((xfer->rx_buf_addr % 32) ||
        (xfer->transfer_length % 32)){
            rt_kprintf("%s : %d ; buf add: 0x%x or size : 0x%x should 32B allign..\n"
            ,__func__,__LINE__,xfer->rx_buf_addr,xfer->transfer_length);
        }
    }
    spi_slave_drv->xfer.rx_buf_addr = xfer->rx_buf_addr;
    spi_slave_drv->xfer.tx_buf_addr = xfer->tx_buf_addr;
    spi_slave_drv->xfer.transfer_length = xfer->transfer_length;
    spi_slave_drv->xfer.call_back = xfer->call_back;
    spi_slave_drv->xfer.cb_para = xfer->cb_para;
}
static rt_err_t fh_spi_slave_control(rt_device_t dev, int cmd, void *args)
{
    struct spi_slave_xfer *xfer = (struct spi_slave_xfer *)args;
    struct spi_slave_driver * spi_slave_drv;
    spi_slave_drv = (struct spi_slave_driver *)(dev->user_data);
    //lock first..
    rt_sem_take(&spi_slave_drv->xfer_lock, RT_WAITING_FOREVER);
    switch (cmd)
    {
        case SPI_SLAVE_IOCTL_SET_DATA:
            process_slave_io_set_data(spi_slave_drv,xfer);
            break;
        case SPI_SLAVE_IOCTL_XFER_DATA:
            spi_slave_xfer_data(spi_slave_drv,&spi_slave_drv->xfer);
            break;
        case SPI_SLAVE_IOCTL_WAIT_DATA_DONE:
            PRINT_SPI_SLAVE_DBG("%s::%d\n",__func__,__LINE__);
            rt_completion_wait(&spi_slave_drv->transfer_completion,
                                RT_WAITING_FOREVER);
            //if rx data ,just invaild cache data...
            if(spi_slave_drv->xfer.rx_buf_addr){
                mmu_invalidate_dcache(spi_slave_drv->xfer.rx_buf_addr,spi_slave_drv->xfer.transfer_length);
            }
            //need to call back app???? maybe not..
            if(spi_slave_drv->xfer.call_back){
                spi_slave_drv->xfer.call_back(spi_slave_drv->xfer.cb_para);
            }
            break;
    }
    //release..
    rt_sem_release(&spi_slave_drv->xfer_lock);
    PRINT_SPI_SLAVE_DBG("%s end\n", __func__);
    return RT_EOK;
}


int fh_spi_slave_probe(void *priv_data)
{
    // rtt slave handle...
    rt_device_t spi_slave_dev;
    struct clk *clk;
    char spi_lock_name[20] = { 0 };
    char spi_slave_name[20] = { 0 };
    //spi slave driver data..
    struct spi_slave_driver * spi_slave_drv;
    struct spi_control_platform_data *plat_data;
    int ret;

    spi_slave_dev = rt_malloc(sizeof(struct rt_device));
    spi_slave_drv = rt_malloc(sizeof(struct spi_slave_driver));
    if (!spi_slave_dev || !spi_slave_drv)
    {
        return -RT_ENOMEM;
    }
    rt_memset(spi_slave_dev, 0, sizeof(struct rt_device));
    rt_memset(spi_slave_drv, 0, sizeof(struct spi_slave_driver));
    plat_data = (struct spi_control_platform_data *) priv_data;

    //parse plat data...
    spi_slave_drv->obj.base = plat_data->base;
    spi_slave_drv->plat_data = plat_data;
    ret = spi_slave_dma_init(spi_slave_drv);
    if(ret){
        rt_kprintf("%s %d request dma channel error...\n",__func__,__LINE__);
        return -RT_EIO;
    }
    rt_sprintf(spi_slave_name, "%s%d", "spi_slave_", plat_data->id);
    rt_sprintf(spi_lock_name, "%s%d", "spi_slave_lock_", plat_data->id);
    rt_sem_init(&spi_slave_drv->xfer_lock, spi_lock_name, 1,RT_IPC_FLAG_FIFO);
    clk = (struct clk *)clk_get(NULL, SPI_SLAVE_CLK_NAME);
    if (clk)
    {
        clk_set_rate(clk, plat_data->clk_in);
        clk_enable(clk);
    }
    spi_slave_dev->type    = RT_Device_Class_Char;
    spi_slave_dev->init    = fh_spi_slave_hw_init;
    spi_slave_dev->open    = fh_spi_slave_open;
    spi_slave_dev->close   = fh_spi_slave_close;
    spi_slave_dev->read    = NULL;
    spi_slave_dev->write   = NULL;
    spi_slave_dev->control = fh_spi_slave_control;
    //bind to each other..
    spi_slave_dev->user_data = spi_slave_drv;
    spi_slave_drv->priv = spi_slave_dev;
    return rt_device_register(spi_slave_dev, spi_slave_name, RT_DEVICE_FLAG_RDWR);
}

int fh_spi_slave_exit(void *priv_data)
{
    struct spi_control_platform_data *plat_data;
    rt_device_t spi_slave_dev;
    struct spi_slave_driver * spi_slave_drv;
    struct rt_dma_device * dma_dev;
    struct fh_spi_obj *spi_slave_obj;

    plat_data  = (struct spi_control_platform_data *)priv_data;
    char spi_slave_name[20] = { 0 };
    rt_sprintf(spi_slave_name, "%s%d", "spi_slave_", plat_data->id);
    spi_slave_dev = rt_device_find(spi_slave_name);
    if (spi_slave_dev == RT_NULL)
    {
        rt_kprintf("%s find spi slave%d device error..\n",__func__,plat_data->id);
        return -1;
    }
    spi_slave_drv = (struct spi_slave_driver *)(spi_slave_dev->user_data);
    //free dma channel.
    dma_dev = spi_slave_drv->dma_dev;
    dma_dev->ops->control(dma_dev, RT_DEVICE_CTRL_DMA_REQUEST_CHANNEL,
    (void *)spi_slave_drv->dma_rx);
    dma_dev->ops->control(dma_dev, RT_DEVICE_CTRL_DMA_REQUEST_CHANNEL,
    (void *)spi_slave_drv->dma_tx);
    //close hw..
    spi_slave_obj = &spi_slave_drv->obj;
    SPI_Enable(spi_slave_obj, RT_FALSE);
    SPI_DisableDma(spi_slave_obj, SPI_TX_DMA | SPI_RX_DMA);
    //free lock..
    rt_sem_detach(&spi_slave_drv->xfer_lock);
    //free malloc mem..
    rt_free(spi_slave_drv->dma_buff_tx);
    rt_free(spi_slave_drv->dma_buff_rx);
    rt_free(spi_slave_drv);
    return 0;
}

struct fh_board_ops fh_spi_slave_driver_ops = {

    .probe = fh_spi_slave_probe,
    .exit = fh_spi_slave_exit,
};

void rt_hw_spi_slave_init(void)
{
    PRINT_SPI_SLAVE_DBG("%s start\n", __func__);
    fh_board_driver_register("spi_slave", &fh_spi_slave_driver_ops);
    PRINT_SPI_SLAVE_DBG("%s end\n", __func__);
}
