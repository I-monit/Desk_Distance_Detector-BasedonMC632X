/**
 * Copyright (c) 2015-2019 Shanghai Fullhan Microelectronics Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 *
 * Change Logs:
 * Date           Author       Notes
 * 2019-04-12     songyh         add license Apache-2.0
 */

#include <rthw.h>
#include "fh_arch.h"
#include "fh_uart.h"
#include "uart.h"
#include "dma.h"
#include "fh_dma.h"
#include "fh_clock.h"
#include "drivers/serial.h"
#include "dma_mem.h"
#include <delay.h>
#ifdef RT_USING_PM
#include <pm.h>
#endif

#if defined(FH_UART_DEBUG) && defined(RT_DEBUG)
#define PRINT_UART_DBG(fmt, args...)   \
    do                                \
    {                                 \
        rt_kprintf("FH_UART_DEBUG: "); \
        rt_kprintf(fmt, ##args);      \
    } while (0)
#else
#define PRINT_UART_DBG(fmt, args...) \
    do                              \
    {                               \
    } while (0)
#endif

#define UART0_CLK_NAME    "uart0_clk"
#define UART1_CLK_NAME    "uart1_clk"
#define UART2_CLK_NAME    "uart2_clk"
#define UART3_CLK_NAME    "uart3_clk"

#if defined(FH_USING_UART0)
static struct rt_serial_device serial0;
struct fh_uart uart0 = {
    .uart_port        = (uart *)UART0_REG_BASE,
    .irq              = UART0_IRQn,
    .use_dma          = 0,
    .dma_rx_handshake = UART0_RX,
    .dma_tx_handshake = UART0_TX,
    .need_pm          = 1,
    .pm_fcr           = UART_FCR_FIFOE | UART_FCR_RFIFOR | UART_FCR_XFIFOR |
                        UART_FCR_TET_1_4 | UART_FCR_RT_ONE,
    .pm_fcr_dma       = UART_FCR_FIFOE | UART_FCR_RFIFOR | UART_FCR_XFIFOR |
                UART_FCR_TET_EMPTY | UART_FCR_RT_ONE | UART_FCR_DMAM,
};
#endif

#if defined(FH_USING_UART1)
static struct rt_serial_device serial1;
struct fh_uart uart1 = {
    .uart_port        = (uart *)UART1_REG_BASE,
    .irq              = UART1_IRQn,
    .use_dma          = 0,
    .dma_rx_handshake = UART1_RX,
    .dma_tx_handshake = UART1_TX,
    .need_pm          = 1,
    .pm_fcr           = UART_FCR_FIFOE | UART_FCR_RFIFOR | UART_FCR_XFIFOR |
                        UART_FCR_TET_1_4 | UART_FCR_RT_ONE,
    .pm_fcr_dma       = UART_FCR_FIFOE | UART_FCR_RFIFOR | UART_FCR_XFIFOR |
                UART_FCR_TET_EMPTY | UART_FCR_RT_ONE | UART_FCR_DMAM,
};
#endif

#if defined(FH_USING_UART2)
static struct rt_serial_device serial2;
struct fh_uart uart2 = {
    .uart_port        = (uart *)UART2_REG_BASE,
    .irq              = UART2_IRQn,
    .use_dma          = 0,
    .dma_rx_handshake = UART2_RX,
    .dma_tx_handshake = UART2_TX,
    .need_pm          = 1,
    .pm_fcr           = UART_FCR_FIFOE | UART_FCR_RFIFOR | UART_FCR_XFIFOR |
                        UART_FCR_TET_1_4 | UART_FCR_RT_ONE,
    .pm_fcr_dma       = UART_FCR_FIFOE | UART_FCR_RFIFOR | UART_FCR_XFIFOR |
                UART_FCR_TET_EMPTY | UART_FCR_RT_ONE | UART_FCR_DMAM,
};
#endif

#if defined(FH_USING_UART3)
static struct rt_serial_device serial3;
struct fh_uart uart3 = {
    .uart_port        = (uart *)UART3_REG_BASE,
    .irq              = UART3_IRQn,
    .use_dma          = 0,
    .dma_rx_handshake = UART3_RX,
    .dma_tx_handshake = UART3_TX,
    .need_pm          = 1,
    .pm_fcr           = UART_FCR_FIFOE | UART_FCR_RFIFOR | UART_FCR_XFIFOR |
                        UART_FCR_TET_1_4 | UART_FCR_RT_ONE,
    .pm_fcr_dma       = UART_FCR_FIFOE | UART_FCR_RFIFOR | UART_FCR_XFIFOR |
                UART_FCR_TET_EMPTY | UART_FCR_RT_ONE | UART_FCR_DMAM,
};
#endif

#if defined(FH_USING_UART_DMA)
#define UART_DMA_RXMEM_SIZE 32
#define UART_DMA_TXMEM_SIZE 256
#ifndef UART_DMA_DEV
#define UART_DMA_DEV        uart1
#endif

struct uart_dma_obj
{
    struct dma_transfer *dma_tx;
    struct dma_transfer *dma_rx;
    rt_uint32_t rx_buf_read_ptr;
    int ptr_on_diff_page;
    rt_uint8_t *dma_buff_tx;
    rt_uint8_t *dma_buff_rx;
    struct rt_dma_device *dma_dev;
    struct rt_completion transfer_completion;
    struct rt_completion rx_transfer_completion;
    rt_uint32_t transfer_length;
    rt_bool_t is_used;
    rt_bool_t is_waiting_for_completion;
    struct rt_mutex lock;
    void *priv;
};

struct uart_dma_obj *uart_dma;

void uart_dma_tx(rt_uint32_t mem_addr, rt_uint16_t data_size)
{
    int ret;

    rt_completion_init(&uart_dma->transfer_completion);

    uart_dma->transfer_length = data_size;

    rt_memset((void *)uart_dma->dma_buff_tx, 0, UART_DMA_TXMEM_SIZE);
    rt_memcpy(uart_dma->dma_buff_tx, (void *)mem_addr, data_size);

    uart_dma->dma_tx->trans_len = data_size;

    uart_dma->dma_dev->ops->control(uart_dma->dma_dev,
                                    RT_DEVICE_CTRL_DMA_SINGLE_TRANSFER,
                                   (void *)uart_dma->dma_tx);

    ret = rt_completion_wait(&uart_dma->transfer_completion,
                            RT_TICK_PER_SECOND * 5);

    if (ret)
    {
        rt_kprintf("ERROR: %s, transfer timeout\n", __func__);
        uart_dma->dma_dev->ops->control(uart_dma->dma_dev,
                                        RT_DEVICE_CTRL_DMA_PAUSE,
                                        (void *)uart_dma->dma_tx);
        return;
    }

}


void uart_dma_rx_internal(void)
{
    uart_dma->dma_rx->trans_len = UART_DMA_RXMEM_SIZE;

    uart_dma->dma_dev->ops->control(uart_dma->dma_dev,
                                    RT_DEVICE_CTRL_DMA_SINGLE_TRANSFER,
                                    (void *)uart_dma->dma_rx);
}

int uart_dma_rx(rt_uint32_t mem_addr, rt_uint16_t data_size)
{
    uart_dma->is_waiting_for_completion = 1;
    rt_completion_wait(&uart_dma->rx_transfer_completion, RT_WAITING_FOREVER);
    uart_dma->is_waiting_for_completion = 0;
    rt_memcpy((void *)mem_addr,
                    (void *)uart_dma->dma_buff_rx,
                    UART_DMA_RXMEM_SIZE);

    return UART_DMA_RXMEM_SIZE;
}

static void uart_dma_rx_done(void *arg)
{
    rt_completion_done(&uart_dma->rx_transfer_completion);
    PRINT_UART_DBG("kick\n");
}

static void uart_dma_tx_done(void *arg)
{
    rt_completion_done(&uart_dma->transfer_completion);
}

static void uart_dma_open(void)
{
    int ret = 0;

    uart_dma->dma_tx->dma_number        = 0;
    uart_dma->dma_tx->dst_add           = (rt_uint32_t)(&UART_DMA_DEV.uart_port->UART_THR);
    uart_dma->dma_tx->dst_hs            = DMA_HW_HANDSHAKING;
    uart_dma->dma_tx->dst_inc_mode      = DW_DMA_SLAVE_FIX;
    uart_dma->dma_tx->dst_msize         = DW_DMA_SLAVE_MSIZE_1;
    uart_dma->dma_tx->dst_per           = UART_DMA_DEV.dma_tx_handshake;
    uart_dma->dma_tx->dst_width         = DW_DMA_SLAVE_WIDTH_8BIT;
    uart_dma->dma_tx->fc_mode           = DMA_M2P;
    uart_dma->dma_tx->src_add           = (rt_uint32_t)(uart_dma->dma_buff_tx);
    uart_dma->dma_tx->src_inc_mode      = DW_DMA_SLAVE_INC;
    uart_dma->dma_tx->src_msize         = DW_DMA_SLAVE_MSIZE_1;
    uart_dma->dma_tx->src_width         = DW_DMA_SLAVE_WIDTH_8BIT;
    uart_dma->dma_tx->complete_callback = (void *)uart_dma_tx_done;
    uart_dma->dma_tx->channel_number    = AUTO_FIND_CHANNEL;

    uart_dma->dma_rx->dma_number        = 0;
    uart_dma->dma_rx->fc_mode           = DMA_P2M;
    uart_dma->dma_rx->dst_add           = (rt_uint32_t)(uart_dma->dma_buff_rx);
    uart_dma->dma_rx->dst_inc_mode      = DW_DMA_SLAVE_INC;
    uart_dma->dma_rx->dst_msize         = DW_DMA_SLAVE_MSIZE_1;
    uart_dma->dma_rx->dst_width         = DW_DMA_SLAVE_WIDTH_8BIT;
    uart_dma->dma_rx->src_add           = (rt_uint32_t)(&UART_DMA_DEV.uart_port->UART_THR);
    uart_dma->dma_rx->src_inc_mode      = DW_DMA_SLAVE_FIX;
    uart_dma->dma_rx->src_msize         = DW_DMA_SLAVE_MSIZE_1;
    uart_dma->dma_rx->src_width         = DW_DMA_SLAVE_WIDTH_8BIT;
    uart_dma->dma_rx->src_hs            = DMA_HW_HANDSHAKING;
    uart_dma->dma_rx->src_per           = UART_DMA_DEV.dma_rx_handshake;
    uart_dma->dma_rx->complete_callback = (void *)uart_dma_rx_done;
    uart_dma->dma_rx->channel_number    = AUTO_FIND_CHANNEL;

    uart_dma->ptr_on_diff_page = 0;
    uart_dma->rx_buf_read_ptr = 0;

    uart_configure(UART_DMA_DEV.uart_port, UART_DATA_BIT8, UART_STOP_BIT1, UART_PARITY_NONE,
            115200, UART_CLOCK_FREQ, UART_DMA_DEV.use_dma);

    ret = uart_dma->dma_dev->ops->control(uart_dma->dma_dev,
                                        RT_DEVICE_CTRL_DMA_REQUEST_CHANNEL,
                                        (void *)uart_dma->dma_rx);
    if (ret)
    {
        rt_kprintf("ERROR: %s, request channel failed, ret: %d\n", __func__,
                    ret);
        return;
    }

    ret = uart_dma->dma_dev->ops->control(uart_dma->dma_dev,
                                        RT_DEVICE_CTRL_DMA_REQUEST_CHANNEL,
                                        (void *)uart_dma->dma_tx);
    if (ret)
    {
        rt_kprintf("ERROR: %s, request channel failed, ret: %d\n", __func__,
                    ret);
        uart_dma->dma_dev->ops->control(uart_dma->dma_dev,
                                        RT_DEVICE_CTRL_DMA_RELEASE_CHANNEL,
                                        (void *)uart_dma->dma_rx);
    }

    rt_completion_init(&uart_dma->rx_transfer_completion);
    uart_dma_rx_internal();

    return;
}

static void uart_dma_close(void)
{
    uart_dma->dma_dev->ops->control(uart_dma->dma_dev,
                                    RT_DEVICE_CTRL_DMA_RELEASE_CHANNEL,
                                    (void *)uart_dma->dma_rx);
    uart_dma->dma_dev->ops->control(uart_dma->dma_dev,
                                    RT_DEVICE_CTRL_DMA_RELEASE_CHANNEL,
                                    (void *)uart_dma->dma_tx);

    rt_memset((void *)uart_dma->dma_tx, 0x0, sizeof(struct dma_transfer));
    rt_memset((void *)uart_dma->dma_rx, 0x0, sizeof(struct dma_transfer));

}

void uart_dma_reset(void)
{
    rt_mutex_take(&uart_dma->lock, RT_WAITING_FOREVER);
    uart_dma_close();
    uart_dma_open();
    rt_mutex_release(&uart_dma->lock);
}

static void uart_dma_init(void)
{
    int errno;

    uart_dma = (struct uart_dma_obj *)rt_malloc(sizeof(struct uart_dma_obj));
    rt_memset(uart_dma, 0, sizeof(struct uart_dma_obj));

    uart_dma->dma_dev = (struct rt_dma_device *)rt_device_find("fh_dma0");

    uart_dma->dma_tx =
        (struct dma_transfer *)rt_malloc(sizeof(struct dma_transfer));
    uart_dma->dma_rx =
        (struct dma_transfer *)rt_malloc(sizeof(struct dma_transfer));

    rt_memset((void *)uart_dma->dma_tx, 0x0, sizeof(struct dma_transfer));
    rt_memset((void *)uart_dma->dma_rx, 0x0, sizeof(struct dma_transfer));

    if (uart_dma->dma_dev == RT_NULL)
    {
        rt_kprintf("ERROR: %s, can't find dma device\n", __func__);
        return;
    }

    uart_dma->dma_buff_tx = (rt_uint8_t *)fh_dma_mem_malloc(UART_DMA_TXMEM_SIZE);

    if(uart_dma->dma_buff_tx == RT_NULL)
    {
        rt_kprintf("ERROR: %s, tx malloced dma buffer failed\n", __func__);
        return;
    }

    uart_dma->dma_buff_rx = (rt_uint8_t *)fh_dma_mem_malloc(UART_DMA_RXMEM_SIZE);

    if(uart_dma->dma_buff_rx == RT_NULL)
    {
        rt_kprintf("ERROR: %s, rx malloced dma buffer failed\n", __func__);
        return;
    }

    if (((rt_uint32_t)uart_dma->dma_buff_tx % 4) ||
        ((rt_uint32_t)uart_dma->dma_buff_rx % 4))
    {
        rt_kprintf("ERROR: %s, malloced dma buffer is not aligned\n", __func__);
        fh_dma_mem_free(uart_dma->dma_buff_rx);
        fh_dma_mem_free(uart_dma->dma_buff_tx);
        return;
    }

    uart_dma->is_used = 0;
    errno = rt_mutex_init(&uart_dma->lock, "Uart_Dma_Lock", RT_IPC_FLAG_FIFO);

}

static rt_err_t fh_uart_dma_init(rt_device_t dev)
{
    uart_dma_init();
    return 0;
}

static rt_err_t fh_uart_dma_open(rt_device_t dev, rt_uint16_t oflag)
{
    if (!uart_dma->is_used)
    {
        rt_mutex_take(&uart_dma->lock, RT_WAITING_FOREVER);
        uart_dma_open();
        uart_dma->is_used = 1;
        rt_mutex_release(&uart_dma->lock);
        return 0;
    }
    else
    {
        rt_kprintf("Uart DMA Device Busy!\n");
        return -RT_EBUSY;
    }

}

static rt_err_t fh_uart_dma_close(rt_device_t dev)
{
    if (uart_dma->is_used)
    {
        rt_mutex_take(&uart_dma->lock, RT_WAITING_FOREVER);
        uart_dma_close();
        uart_dma->is_used = 0;
        rt_mutex_release(&uart_dma->lock);
        return 0;
    }
    else
    {
        rt_kprintf("Uart DMA Device not open!\n");
        return -RT_EIO;
    }

}

static rt_size_t fh_uart_dma_read(rt_device_t dev, rt_off_t pos, void *buffer, rt_size_t size)
{
    int len;

    rt_mutex_take(&uart_dma->lock, RT_WAITING_FOREVER);
    len = uart_dma_rx((rt_uint32_t)buffer, size);
    rt_mutex_release(&uart_dma->lock);
    return len;
}

static rt_size_t fh_uart_dma_write(rt_device_t dev, rt_off_t pos, const void *buffer, rt_size_t size)
{
    rt_mutex_take(&uart_dma->lock, RT_WAITING_FOREVER);
    uart_dma_tx((rt_uint32_t)buffer, size);
    rt_mutex_release(&uart_dma->lock);
    return size;
}

static rt_err_t fh_uart_dma_ioctl(rt_device_t dev, int cmd, void *args)
{
    switch (cmd)
    {
    case FH_UART_FIFO_RESET:
        uart_reset_fifo((uart *)(&UART_DMA_DEV));
        break;
    case FH_DMA_RESET:
        uart_dma_reset();
        break;
    }

    return 0;
}

void uartd_init(void)
{
    uart_dma_init();
}

void uartd_open(void)
{
    uart_dma_open();
}

void uartd_close(void)
{
    uart_dma_close();
}

void uartd_reset(void)
{
    uart_dma_close();
    uart_dma_open();
}

void uartd_rx(void)
{
    char input[20] = {0};
    char tx[20] = "uart tx test";
    int size = 0;

    uart_dma_open();

    uart_dma_tx((rt_uint32_t)tx, 12);

    do
    {
        size = uart_dma_rx((rt_uint32_t)input, 16);
        rt_thread_delay(1);
    }while(size < 16);

    uart_dma_close();

    rt_kprintf("read %d, %s\n", size, input);
    rt_kprintf("readptr: %d\n", uart_dma->rx_buf_read_ptr);


}

void uartd_tx(void)
{
    int size = 0;
    char input[20] = {0};

    uart_dma_open();
    while (1)
    {
        do
        {
            size = uart_dma_rx((rt_uint32_t)input, 1);
            rt_thread_delay(1);
        } while (size < 1);
        uart_dma_tx((rt_uint32_t)input, 1);
    }
}

#ifdef RT_USING_FINSH
FINSH_FUNCTION_EXPORT(uartd_init, uartd_init);
FINSH_FUNCTION_EXPORT(uartd_open, uartd_open);
FINSH_FUNCTION_EXPORT(uartd_close, uartd_close);
FINSH_FUNCTION_EXPORT(uartd_rx, uartd_rx);
FINSH_FUNCTION_EXPORT(uartd_tx, uartd_tx);
FINSH_FUNCTION_EXPORT(uartd_reset, uartd_reset);
#endif
#endif

void rt_fh_uart_handler(int vector, void *param)
{
    int status;

    struct fh_uart *uart;
    int ch = -1;
    unsigned int reg_status;
    struct rt_serial_device *t_serial;
    rt_device_t dev = (rt_device_t)param;

    uart            = (struct fh_uart *)dev->user_data;
    status          = uart_get_iir_status(uart->uart_port);
    t_serial        = (struct rt_serial_device *)dev;
    if (status & UART_IIR_NOINT)
        return;

    if (status & UART_IIR_THREMPTY)
    {
        /* first close tx isr */
        uart_disable_irq(uart->uart_port, UART_IER_ETBEI);

        rt_hw_serial_isr((struct rt_serial_device *)dev,
                        RT_SERIAL_EVENT_TX_DONE);
    }
    else if ((status & UART_IIR_CHRTOUT) == UART_IIR_CHRTOUT)
    {
        /* bug.... */
        /* if no data in rx fifo */
        reg_status = uart_get_status(uart->uart_port);
        if ((reg_status & 1 << 3) == 0)
            uart_getc(uart->uart_port);
    }
    else
    {
        if (t_serial->serial_rx == RT_NULL)
        {
            while (1)
            {
                ch = t_serial->ops->getc(t_serial);
                if (ch == -1)
                    break;
            }
        }
        else
        {
            rt_hw_serial_isr((struct rt_serial_device *)dev,
                            RT_SERIAL_EVENT_RX_IND);
        }

    }
}

/**
* UART device in RT-Thread
*/
static rt_err_t fh_uart_configure(struct rt_serial_device *serial,
                                struct serial_configure *cfg)
{
/* int div; */
    enum data_bits data_mode;
    enum stop_bits stop_mode;
    enum parity parity_mode;
    struct fh_uart *uart;

    RT_ASSERT(serial != RT_NULL);
    RT_ASSERT(cfg != RT_NULL);
    uart = (struct fh_uart *)serial->parent.user_data;

    switch (cfg->data_bits)
    {
    case DATA_BITS_8:
        data_mode = UART_DATA_BIT8;
        break;
    case DATA_BITS_7:
        data_mode = UART_DATA_BIT7;
        break;
    case DATA_BITS_6:
        data_mode = UART_DATA_BIT6;
        break;
    case DATA_BITS_5:
        data_mode = UART_DATA_BIT5;
        break;
    default:
        data_mode = UART_DATA_BIT8;
        break;
    }

    switch (cfg->stop_bits)
    {
    case STOP_BITS_2:
        stop_mode = UART_STOP_BIT2;
        break;
    case STOP_BITS_1:
    default:
        stop_mode = UART_STOP_BIT1;
        break;
    }

    switch (cfg->parity)
    {
    case PARITY_ODD:
        parity_mode = UART_PARITY_ODD;
        break;
    case PARITY_EVEN:
        parity_mode = UART_PARITY_EVEN;
        break;
    case PARITY_NONE:
    default:
        parity_mode = UART_PARITY_NONE;
        break;
    }

    uart_disable_irq(uart->uart_port, UART_IER_ERBFI);

    uart_configure(uart->uart_port, data_mode, stop_mode, parity_mode,
                    cfg->baud_rate, UART_CLOCK_FREQ, uart->use_dma);

    uart_enable_irq(uart->uart_port, UART_IER_ERBFI);

    return RT_EOK;
}

static rt_err_t fh_uart_control(struct rt_serial_device *serial, int cmd,
                                void *arg)
{
    struct fh_uart *uart;

    RT_ASSERT(serial != RT_NULL);
    uart = (struct fh_uart *)serial->parent.user_data;

    switch (cmd)
    {
    case RT_DEVICE_CTRL_CLR_INT:
        /* disable rx irq */
        rt_hw_interrupt_mask(uart->irq);
        uart_disable_irq(uart->uart_port, UART_IER_ERBFI);
        break;
    case RT_DEVICE_CTRL_SET_INT:
        /* enable rx irq */
        rt_hw_interrupt_umask(uart->irq);
        uart_enable_irq(uart->uart_port, UART_IER_ERBFI);
        break;
    }

    return RT_EOK;
}

static int fh_uart_putc(struct rt_serial_device *serial, char c)
{
    struct fh_uart *uart = serial->parent.user_data;
    unsigned int ret;

    ret = uart_get_status(uart->uart_port);
    if (serial->parent.open_flag & RT_DEVICE_FLAG_INT_TX)
    {
        /* RT_DEVICE_FLAG_INT_TX */
        if (ret &UART_USR_TFNF)
        {
            uart_putc(uart->uart_port, c);
            return 1;
        }
        /* open tx isr here.. */
        uart_enable_irq(uart->uart_port, UART_IER_ETBEI);
        return - 1;
    }
    /* poll mode */
    else
    {
        while (!(uart_get_status(uart->uart_port) & UART_USR_TFNF))
            ;
        uart_putc(uart->uart_port, c);
        return 1;
    }
}

#define     PRINTBUF_SIZE_BITS (12)
#define     PRINTBUF_SIZE      (1<<PRINTBUF_SIZE_BITS)
#define     PRINTBUF_NEXT(idx) (((idx) + 1) & (PRINTBUF_SIZE - 1))
static int  g_printbuf_rd;
static int  g_printbuf_wr;
static char g_printbuf[PRINTBUF_SIZE];
static uart *g_log_uart;

int trigger_uart_output_nolock(void)
{
    int rd = g_printbuf_rd;
    int wr = g_printbuf_wr;

    if (rd == wr)
        return 0;

    while (rd != wr)
    {
        if ((g_log_uart->UART_USR & UART_USR_TFNF) == 0)
            break;

        g_log_uart->UART_THR = (UINT8)g_printbuf[rd];
        rd = PRINTBUF_NEXT(rd);
    }
    g_printbuf_rd = rd;

    return 1;
}

static void write_into_buffer(char *ptr, int length)
{
#define CRNUM_MAX (32)
    rt_base_t level;
    int rd;
    int wr;
    int room;
    int crnum = 0;

    if (length > (int)(PRINTBUF_SIZE - 1 - CRNUM_MAX))
    {
        return; /*too long  print message, just skip!!!*/
    }

    while (length > 0)
    {
        level = rt_hw_interrupt_disable();

        rd = g_printbuf_rd;
        wr = g_printbuf_wr;
        if (rd <= wr)
        {
            room = PRINTBUF_SIZE - 1 - (wr - rd);
        }
        else
        {
            room = rd - wr - 1;
        }
        if (room >= length + CRNUM_MAX)
        {
            do
            {
                if (*ptr == 0x0a && crnum < CRNUM_MAX)
                {
                    g_printbuf[wr] = 0x0d;
                    wr = PRINTBUF_NEXT(wr);
                    crnum++;
                }
                g_printbuf[wr] = *(ptr++);
                wr = PRINTBUF_NEXT(wr);
            }while (--length > 0);
            g_printbuf_wr = wr;
        }

        trigger_uart_output_nolock();
        rt_hw_interrupt_enable(level);
    }
}

static int fh_uart_putmulti(struct rt_serial_device *serial, const rt_uint8_t *data, int length)
{
    g_log_uart = ((struct fh_uart *)serial->parent.user_data)->uart_port;
    write_into_buffer((char *)data, length);
    return length;
}

static int fh_uart_getc(struct rt_serial_device *serial)
{
    int result;
    struct fh_uart *uart = serial->parent.user_data;

    if (uart_is_rx_ready(uart->uart_port))
    {
        result = uart_getc(uart->uart_port);
    }
    else
    {
        result = -1;
    }

    return result;
}

static const struct rt_uart_ops fh_uart_ops = {
    fh_uart_configure, fh_uart_control, fh_uart_putc, fh_uart_getc, RT_NULL, fh_uart_putmulti,
};


#ifdef RT_USING_PM
static int fh_serial_runtime_suspend(const struct rt_device *device, rt_uint8_t mode)
{
    struct fh_uart *uart = (struct fh_uart *)device->user_data;
    unsigned int ret;
    struct uart_reg *port;

    port = uart->uart_port;
    if (uart->need_pm)
    {
        do
        {
            port->IIRFCR = UART_FCR_RFIFOR | UART_FCR_XFIFOR;
            port->IIRFCR = UART_FCR_FIFOE;
            /* read status.. */
            ret = uart_get_status(port);
        } while (ret & UART_USR_BUSY);

        ret = port->UART_LCR;
        /* if DLAB not set */
        if (!(ret & UART_LCR_DLAB))
        {
            ret |= UART_LCR_DLAB;
            port->UART_LCR = ret;
        }

        uart->pm_dll = port->RBRTHRDLL;
        uart->pm_dlh = port->DLHIER;

        /* clear DLAB */
        ret = ret & 0x7f;
        port->UART_LCR = ret;

        ret = port->UART_LCR;
        /* if DLAB set */
        if (ret & UART_LCR_DLAB)
        {
            ret &= !UART_LCR_DLAB;
            port->UART_LCR = ret;
        }
        uart->pm_ier = port->DLHIER;

        uart->pm_lcr = port->UART_LCR;

        uart->pm_mcr = port->UART_MCR;

#if defined(FH_USING_UART_DMA)
        if ((uart_dma) && (uart_dma->is_waiting_for_completion))
        rt_completion_done(&uart_dma->rx_transfer_completion);
#endif
        PRINT_UART_DBG("dll:0x%x dlh:0x%x ier:0x%x lcr:0x%x mcr:0x%x fcr:0x%x\n",
                    uart->pm_dll, uart->pm_dlh, uart->pm_ier,
                    uart->pm_lcr, uart->pm_mcr, uart->pm_fcr);
    }

    return 0;
}

static void fh_serial_runtime_resume(const struct rt_device *device, rt_uint8_t mode)
{
    struct fh_uart *uart = (struct fh_uart *)device->user_data;
    unsigned int ret;
    struct uart_reg *port;

    port = uart->uart_port;
    if (uart->need_pm)
    {
        port->UART_SRR = 1;
        udelay(10);
        /*port->IIRFCR = UART_FCR_RFIFOR | UART_FCR_XFIFOR;*/
        ret = port->UART_LCR;
        /* if DLAB not set */
        if (!(ret & UART_LCR_DLAB))
        {
            ret |= UART_LCR_DLAB;
            port->UART_LCR = ret;
        }

        port->RBRTHRDLL = uart->pm_dll;
        port->DLHIER = uart->pm_dlh;
        /* clear DLAB */
        ret = ret & 0x7f;
        port->UART_LCR = ret;

        port->UART_LCR = uart->pm_lcr;

        if (uart->use_dma)
            port->IIRFCR = uart->pm_fcr_dma;
        else
            port->IIRFCR = uart->pm_fcr;

        port->UART_MCR = uart->pm_mcr;

        uart_disable_irq(port, 0xf);

        ret = port->UART_LCR;
        /* if DLAB set */
        if (ret & UART_LCR_DLAB)
        {
            ret &= !UART_LCR_DLAB;
            port->UART_LCR = ret;
        }
        port->DLHIER = uart->pm_ier;
    }

    return ;
}

struct rt_device_pm_ops uart_pm_ops = {
    .suspend_late = fh_serial_runtime_suspend,
    .resume_late = fh_serial_runtime_resume
};

#endif


/**
 * This function will handle init uart
 */
void rt_hw_uart_init(void)
{
    struct serial_configure config = RT_SERIAL_CONFIG_DEFAULT;
#if defined(FH_USING_UART0)
    struct clk *clk_rate_serial0 = RT_NULL;
    serial0.ops    = &fh_uart_ops;
    serial0.config = config;

    clk_rate_serial0 = clk_get(NULL, UART0_CLK_NAME);
    if (clk_rate_serial0 == RT_NULL)
        rt_kprintf("ERROR: uart0 get clk failed\n");
    else
        clk_enable(clk_rate_serial0);

    /* register vcom device */
    rt_hw_serial_register(
        &serial0, "uart0",
        RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_INT_RX | RT_DEVICE_FLAG_STREAM,
        &uart0);
    rt_hw_interrupt_install(uart0.irq, rt_fh_uart_handler,
                            (void *)&(serial0.parent), "UART0");
    rt_hw_interrupt_umask(uart0.irq);
    /* rt_hw_enable_irq_wake(uart0.irq);*/

#ifdef RT_USING_PM
    rt_pm_device_register(&(serial0.parent), &uart_pm_ops);
#endif

#endif

#if defined(FH_USING_UART1)
    struct clk *clk_rate_serial1 = RT_NULL;
    serial1.ops    = &fh_uart_ops;
    serial1.config = config;

    clk_rate_serial1 = clk_get(NULL, UART1_CLK_NAME);
    if (clk_rate_serial1 == RT_NULL)
        rt_kprintf("ERROR: uart1 get clk failed\n");
    else
        clk_enable(clk_rate_serial1);

    /* register vcom device */
    rt_hw_serial_register(
        &serial1, "uart1",
        RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_INT_RX | RT_DEVICE_FLAG_STREAM,
        &uart1);
    rt_hw_interrupt_install(uart1.irq, rt_fh_uart_handler,
                            (void *)&(serial1.parent), "UART1");
    rt_hw_interrupt_umask(uart1.irq);

#ifdef RT_USING_PM
    /* rt_hw_enable_irq_wake(uart1.irq); */
    rt_pm_device_register(&(serial1.parent), &uart_pm_ops);
#endif

#endif

#if defined(FH_USING_UART2)
    struct clk *clk_rate_serial2 = RT_NULL;
    serial2.ops    = &fh_uart_ops;
    serial2.config = config;

    clk_rate_serial2 = clk_get(NULL, UART2_CLK_NAME);
    if (clk_rate_serial2 == RT_NULL)
        rt_kprintf("ERROR: uart2 get clk failed\n");
    else
        clk_enable(clk_rate_serial2);

    /* register vcom device */
    rt_hw_serial_register(
        &serial2, "uart2",
        RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_INT_RX | RT_DEVICE_FLAG_STREAM,
        &uart2);
    rt_hw_interrupt_install(uart2.irq, rt_fh_uart_handler,
                            (void *)&(serial2.parent), "UART2");
    rt_hw_interrupt_umask(uart2.irq);

#ifdef RT_USING_PM
    rt_pm_device_register(&(serial2.parent), &uart_pm_ops);
#endif

#endif

#if defined(FH_USING_UART3)
    struct clk *clk_rate_serial3 = RT_NULL;
    serial3.ops    = &fh_uart_ops;
    serial3.config = config;

    clk_rate_serial3 = clk_get(NULL, UART3_CLK_NAME);
    if (clk_rate_serial3 == RT_NULL)
        rt_kprintf("ERROR: uart3 get clk failed\n");
    else
        clk_enable(clk_rate_serial3);

    /* register vcom device */
    rt_hw_serial_register(
        &serial3, "uart3",
        RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_INT_RX | RT_DEVICE_FLAG_STREAM,
        &uart3);
    rt_hw_interrupt_install(uart3.irq, rt_fh_uart_handler,
                            (void *)&(serial3.parent), "UART3");
    rt_hw_interrupt_umask(uart3.irq);

#ifdef RT_USING_PM
    rt_pm_device_register(&(serial3.parent), &uart_pm_ops);
#endif

#endif

#if defined(FH_USING_UART_DMA)
    rt_device_t uart_dma_dev;

    uart_dma_dev = rt_malloc(sizeof(struct rt_device));
    if (uart_dma_dev == RT_NULL)
    {
        rt_kprintf("ERROR: %s rt_device malloc failed\n", __func__);
        return;
    }
    rt_memset(uart_dma_dev, 0, sizeof(struct rt_device));

    uart_dma_dev->user_data = &UART_DMA_DEV;
    uart_dma_dev->init      = fh_uart_dma_init;
    uart_dma_dev->open      = fh_uart_dma_open;
    uart_dma_dev->close     = fh_uart_dma_close;
    uart_dma_dev->control   = fh_uart_dma_ioctl;
    uart_dma_dev->read      = fh_uart_dma_read;
    uart_dma_dev->write     = fh_uart_dma_write;
    uart_dma_dev->type      = RT_Device_Class_Miscellaneous;

    rt_device_register(uart_dma_dev, "uart_dma", RT_DEVICE_FLAG_RDWR);
#endif
}
