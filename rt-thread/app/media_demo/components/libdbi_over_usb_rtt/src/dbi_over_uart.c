/*
 * dbi_over_udp.c
 *
 *  Created on: 2016.5.6
 *      Author: gaoyb
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/prctl.h>
#include "libdbi_over_usb_rtt/include/dbi_over_uart.h"
#include "isp_ext/debug_interface.h"

#define dbg_log_e printf
#define FREE free

/* ================== uart ============================ */
struct dbi_over_uart
{
    int index;
    int fd;
    struct Debug_Interface *di;
};

static int uart_send(struct dbi_over_uart *uart, unsigned char *buf, int size)
{
    return write(uart->fd, buf, size);
}

static int uart_recv(struct dbi_over_uart *uart, unsigned char *buf, int size)
{
    return read(uart->fd, buf, size);
}

static struct dbi_over_uart *uart_dbi_create(int index)
{
    struct dbi_over_uart *uart;
    char dev_path[32] = {0};

    uart = malloc(sizeof(struct dbi_over_uart));
    if (!uart)
        return NULL;
    uart->index = index;

    if (index != 4)
        sprintf(dev_path, "uart%d", index);
    else
        strcpy(dev_path, "/dev/vcom");

    uart->fd = open(dev_path, O_RDWR);
    if (uart->fd < 0)
    {
        dbg_log_e("open(%s) fail with(%d)\n", dev_path, uart->fd);
        FREE(uart);
        return NULL;
    }

    struct DI_config di_cfg;

    di_cfg.obj = uart;
    di_cfg.send = (dbi_send)uart_send;
    di_cfg.recv = (dbi_recv)uart_recv;

    uart->di = DI_create(&di_cfg);
    if (!uart->di)
    {
        dbg_log_e("DI_create fail\n");
        close(uart->fd);
        FREE(uart);
        return NULL;
    }

    return uart;
}

struct dbi_uart_config g_uart_cfg = {

    0,
    NULL,
};

static int uart_dbi_destroy(struct dbi_over_uart *uart)
{
    close(uart->fd);
    DI_destroy(uart->di);
    FREE(uart);
    printf("uart_dbi_destroy\n");
    return 0;
}

int fh_cdc_mode_get(void);
int *uart_dbi_thread(void *param)
{
    struct dbi_uart_config *conf = (struct dbi_uart_config *)param;
    int ret;
    volatile int *exit = conf->cancel;
    int *is_running = (int *)conf->is_running;
    struct dbi_over_uart *uart = uart_dbi_create(conf->index);

    if (!uart)
    {
        dbg_log_e("uart_dbi_create fail\n");
        return 0;
    }

    prctl(PR_SET_NAME, "demo_coolview");

    *is_running = 1;
    while (!*exit)
    {
#ifdef USB_CDC_ENABLE
        if (fh_cdc_mode_get() != FH_CDC_COOLVIEW_MODE)
        {
            sleep(1);
            continue;
        }
#endif
        ret = DI_handle(uart->di);
        if (ret == -1)
        {
            continue;
        }
    }

    uart_dbi_destroy(uart);
    *exit = 0;

    return 0;
}
