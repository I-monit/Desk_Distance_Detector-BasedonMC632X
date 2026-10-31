/**
 * Copyright (c) 2015-2019 Shanghai Fullhan Microelectronics Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 *
 * Change Logs:
 * Date           Author       Notes
 * 2019-01-29     wangyl307    add license Apache-2.0
 */

#ifndef __FH_I2S_H__
#define __FH_I2S_H__

#include <i2s.h>

struct fh_i2s_platform_data
{
    int dma_capture_channel;
    int dma_playback_channel;
    int dma_master;
    char *i2s_clk;
    char *i2s_pclk;
    char *acodec_mclk;
    char *acodec_pclk;
};

int fh_i2s_init(void);

#endif
