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

/*****************************************************************************
 *  Include Section
 *  add all #include here
 *****************************************************************************/

#include "rtconfig.h"
#include <rthw.h>
#include "fh_uart.h"
#include <rtconfig.h>
/*****************************************************************************
 * Define section
 * add all #define here
 *****************************************************************************/

/****************************************************************************
 * ADT section
 *  add definition of user defined Data Type that only be used in this file here
 ***************************************************************************/

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
 * Global variables section - Local
 * define global variables(will be refered only in this file) here,
 * static keyword should be used to limit scope of local variable to this file
 * e.g.
 *  static uint8_t ufoo;
 *****************************************************************************/

/* function body */

/*****************************************************************************
 * Description:
 *      add funtion description here
 * Parameters:
 *      description for each argument, new argument starts at new line
 * Return:
 *      what does this function returned?
 *****************************************************************************/

int uart_init(uart *port)
{
    port->UART_IER = 0;
    port->UART_LCR = 0;

    return 0;
}

UINT32 uart_get_status(uart *port) { return port->UART_USR; }

void uart_reset_fifo(uart *port)
{
    port->UART_FCR = UART_FCR_RFIFOR | UART_FCR_XFIFOR;
}

void uart_configure(uart *port, enum data_bits data_bit,
                    enum stop_bits stop_bit, enum parity parity,
                    UINT32 buard_rate, UINT32 uart_clk, int use_dma)
{
    UINT32 baud_div;
    UINT32 lcr_reg = 0;
    UINT32 ret;
    struct fh_uart *fh_uart;

    fh_uart = rt_container_of(port, struct fh_uart, uart_port);
    do
    {
        port->UART_FCR = UART_FCR_RFIFOR | UART_FCR_XFIFOR;
        port->UART_FCR = UART_FCR_FIFOE;
        /* read status.. */
        ret = uart_get_status(port);
    } while (ret & UART_USR_BUSY);
    switch (data_bit)
    {
    case UART_DATA_BIT5:
        lcr_reg |= UART_LCR_DLS5;
        break;
    case UART_DATA_BIT6:
        lcr_reg |= UART_LCR_DLS6;
        break;
    case UART_DATA_BIT7:
        lcr_reg |= UART_LCR_DLS7;
        break;
    case UART_DATA_BIT8:
        lcr_reg |= UART_LCR_DLS8;
        break;
    default:
        lcr_reg |= UART_LCR_DLS8;
        break;
    }

    switch (stop_bit)
    {
    case UART_STOP_BIT1:
        lcr_reg |= UART_LCR_STOP1;
        break;
    case UART_STOP_BIT2:
        lcr_reg |= UART_LCR_STOP2;
        break;
    default:
        lcr_reg |= UART_LCR_STOP1;
        break;
    }

    switch (parity)
    {
    case UART_PARITY_EVEN:
        lcr_reg |= UART_LCR_EVEN | UART_LCR_PEN;
        break;
    case UART_PARITY_ODD:
        lcr_reg |= UART_LCR_PEN;
        break;
    case UART_PARITY_ST:
        lcr_reg |= UART_LCR_SP;
        break;
    case UART_PARITY_NONE:
    default:
        break;
    }

    switch (buard_rate)
    {
    case 115200:
        baud_div = BAUDRATE_115200;
        break;
    case 57600:
        baud_div = BAUDRATE_57600;
        break;
    case 38400:
        baud_div = BAUDRATE_38400;
        break;
    case 19200:
        baud_div = BAUDRATE_19200;
        break;
    case 9600:
        baud_div = BAUDRATE_9600;
        break;
    case 2400:
        baud_div = BAUDRATE_2400;
        break;
    default:
        baud_div = BAUDRATE_115200;
        break;
    }

    port->UART_FCR = UART_FCR_RFIFOR | UART_FCR_XFIFOR;

    /* div */
    ret = port->UART_LCR;
    ret |= UART_LCR_DLAB;
    port->UART_LCR  = ret;
    port->RBRTHRDLL = baud_div & 0x00ff;
    port->DLHIER    = (baud_div & 0xff00) >> 8;
    /* clear DLAB */
    ret            = ret & 0x7f;
    port->UART_LCR = ret;

    /* line control */
    port->UART_LCR = lcr_reg;
    /* fifo control */
    if (use_dma)
    {
        port->UART_FCR = UART_FCR_FIFOE | UART_FCR_RFIFOR | UART_FCR_XFIFOR |
                UART_FCR_TET_EMPTY | UART_FCR_RT_ONE | UART_FCR_DMAM;
        fh_uart->pm_fcr_dma = UART_FCR_FIFOE | UART_FCR_RFIFOR | UART_FCR_XFIFOR |
                UART_FCR_TET_EMPTY | UART_FCR_RT_ONE | UART_FCR_DMAM;
    }
    else
    {
        port->UART_FCR = UART_FCR_FIFOE | UART_FCR_RFIFOR | UART_FCR_XFIFOR |
                        UART_FCR_TET_1_4 | UART_FCR_RT_ONE;
        fh_uart->pm_fcr = UART_FCR_FIFOE | UART_FCR_RFIFOR | UART_FCR_XFIFOR |
                        UART_FCR_TET_1_4 | UART_FCR_RT_ONE;
    }
}

int uart_enable_irq(uart *port, UINT32 mode)
{
    unsigned int ret;

    ret = port->UART_IER;
    ret |= mode;
    port->UART_IER = ret;
    return 0;
}

int uart_disable_irq(uart *port, UINT32 mode)
{
    unsigned int ret;

    ret = port->UART_IER;
    ret &= ~mode;

    port->UART_IER = ret;
    return 0;
}

UINT32 uart_get_iir_status(uart *port) { return port->UART_IIR; }
UINT32 uart_get_line_status(uart *port) { return port->UART_LSR; }
UINT32 uart_is_rx_ready(uart *port) { return port->UART_LSR & UART_LSR_DR; }
UINT8 uart_getc(uart *port) { return port->UART_RBR & 0xFF; }
void uart_putc(uart *port, UINT8 c)
{
    port->UART_THR = c;
}

void uart_set_fifo_mode(uart *port, UINT32 fifo_mode)
{
    port->UART_FCR = fifo_mode;
}
