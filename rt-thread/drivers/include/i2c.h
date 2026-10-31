/**
 * Copyright (c) 2015-2019 Shanghai Fullhan Microelectronics Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2019-01-24     fullhan      the first version
 *
*/

#ifndef __FH81_I2C_H__
#define __FH81_I2C_H__

#include <rtthread.h>

struct i2c_driver
{
    int cmd_err;
    int msg_err;
    rt_uint32_t status;

    struct rt_i2c_msg *msgs;
    int msgs_num;
    int msg_write_idx;
    rt_uint32_t tx_buf_len;
    rt_uint8_t *tx_buf;
    int msg_read_idx;
    rt_uint32_t rx_buf_len;
    rt_uint8_t *rx_buf;

    struct rt_i2c_bus_device *i2c_bus_dev;
    struct rt_completion transfer_completion;
    rt_mutex_t lock;
    void *priv;
};

void rt_hw_i2c_init(void);

#define I2C_CLK_NAME "I2C%d_clk"
#define I2C_ISR_NAME "i2c_%d"

#endif
