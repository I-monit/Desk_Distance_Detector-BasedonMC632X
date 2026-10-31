/**
 * Copyright (c) 2015-2019 Shanghai Fullhan Microelectronics Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 *
 * Change Logs:
 * Date           Author       Notes
 * 2019-01-24     tangyh    the first version
 *
 */

#ifndef __SADC_H__
#define __SADC_H__

typedef struct
{
    unsigned int channel;
    unsigned int sadc_data;
} SADC_INFO;

#define SADC_CMD_READ_RAW_DATA          (0x22)
#define SADC_CMD_READ_VOLT              (0x33)
#define SADC_CMD_DISABLE                (0x44)
/*FH8856V200*/
#define SADC_CMD_READ_CONTINUE_DATA       (0x55)
#define IOCTL_GET_HIT_DATA              (0x66)

#endif /* SADC_H_ */
