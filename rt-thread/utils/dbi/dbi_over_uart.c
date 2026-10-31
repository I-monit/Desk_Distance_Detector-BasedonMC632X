/*
 * dbi_over_udp.c
 *
 *  Created on: 2016.5.6
 *      Author: gaoyb
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "types/type_def.h"
#include <fcntl.h>
#include "dbi_over_uart.h"
#include <dfs_posix.h>
#include "di/debug_interface.h"
#include <sys/prctl.h>

#define dbg_log_e rt_kprintf
#define FREE rt_free

/* ================== uart ============================ */
struct dbi_over_uart
{
    int index;
    rt_device_t dev;
    struct Debug_Interface *di;
};

static int uart_send(struct dbi_over_uart *uart, unsigned char *buf, int size)
{
    return rt_device_write(uart->dev, 0, buf, size);
}

static int uart_recv(struct dbi_over_uart *uart, unsigned char *buf, int size)
{
    rt_size_t rec_size;

    rec_size = rt_device_read(uart->dev, 0, buf, size);
    return rec_size;
}

static struct dbi_over_uart *uart_dbi_create(int index)
{
    struct dbi_over_uart *uart;
    rt_err_t ret = 0;
    char dev_path[32] = {0};

    uart = rt_malloc(sizeof(struct dbi_over_uart));
    if (!uart)
        return NULL;
    uart->index = index;

    if (index != 4)
        sprintf(dev_path, "uart%d", index);
    else
        strcpy(dev_path, "vcom");

    uart->dev = (rt_device_t)rt_device_find(dev_path);
    if (!uart->dev)
    {
        dbg_log_e("rt_device_find(%s) fail\n", dev_path);
        FREE(uart);
        return NULL;
    }

    ret = rt_device_open(uart->dev, RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_INT_RX | RT_DEVICE_FLAG_INT_TX);

    if (ret != 0)
    {
        dbg_log_e("rt_device->open(%s) fail with(%d)\n", dev_path, ret);
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
        rt_device_close(uart->dev);
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
    rt_device_close(uart->dev);
    DI_destroy(uart->di);
    FREE(uart);
    return 0;
}

int *uart_dbi_thread(void *param)
{
    struct dbi_uart_config *conf = (struct dbi_uart_config *)param;
    int ret;
    int *exit = conf->cancel;
    struct dbi_over_uart *uart = uart_dbi_create(conf->index);

    prctl(PR_SET_NAME, "uart_dbi_thread");
    if (!uart)
    {
        dbg_log_e("uart_dbi_create fail\n");
        return 0;
    }

    while (!*exit)
    {
#ifdef FH_RT_USB_DEVICE_CDC
        int fh_cdc_mode_get(void);
        if (fh_cdc_mode_get() == 1)
        {
            rt_thread_delay(1);
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
    return 0;
}

