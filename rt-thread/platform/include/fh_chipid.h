/**
 * Copyright (c) 2015-2019 Shanghai Fullhan Microelectronics Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 *
 * Change Logs:
 * Date           Author       Notes
 * 2019-08-20     wangyl       add license Apache-2.0
 * 2020-05-13     wangyl       add FH8852V200/FH8856V200 support
 */

#ifndef __FH_CHIPID_H__
#define __FH_CHIPID_H__

#define FH_CHIP_FH8626V100      0x8626A100
#define FH_CHIP_FH8852V200      0x8852A200
#define FH_CHIP_FH8856V200      0x8856A200
#define FH_CHIP_FH8858V200      0x8858A200
#define FH_CHIP_FH8856V201      0x8856A201
#define FH_CHIP_FH8858V201      0x8858A201
#define FH_CHIP_FH8852V210      0x8852A210
#define FH_CHIP_FH8856V210      0x8856A210
#define FH_CHIP_FH8858V210      0x8858A210
#define FH_CHIP_FH8652          0x8652A100
#define FH_CHIP_FH8656          0x8656A100
#define FH_CHIP_FH8658          0x8658A100
#define FH_CHIP_FH8852V201      0x8852A201
#define FH_CHIP_FH8852V301      0x8852A301
#define FH_CHIP_FH8662V100      0x8662A100
#define FH_CHIP_FH8626V200      0x8626A200
#define FH_CHIP_FH8852V202      0x8852A202
#define FH_CHIP_FH8636          0x8636A100
#define FH_CHIP_FH8858V300      0x8858A300
#define FH_CHIP_FH8858V310      0x8858A310
#define FH_CHIP_FH8856V300      0x8856A300
#define FH_CHIP_FH8856V310      0x8856A310
#define FH_CHIP_FH8852V310      0x8852A310
#define FH_CHIP_FH8862          0x8862A100
#define FH_CHIP_MC6322          0x6322A100
#define FH_CHIP_MC6326          0x6326A100
#define FH_CHIP_MC6322L         0x6322B100
#define FH_CHIP_MC6326L         0x6326B100
#define FH_CHIP_MC6321          0x6321A100
#define FH_CHIP_MC6321L         0x6321B100
#define FH_CHIP_MC3303          0x3303A100
#define FH_CHIP_FH8856V500      0x8856A500
#define FH_CHIP_FH8857V500      0x8857A500

struct fh_chip_info
{
    int _plat_id; /* 芯片寄存器中的plat_id */
    int _chip_id; /* 芯片寄存器中的chip_id */
    int _chip_mask; /* 芯片寄存器中的chip_id */
    int chip_id; /* 芯片chip_id，详见上述定义 */
    int ddr_size; /* 芯片DDR大小，单位Mbit */
    char chip_name[32]; /* 芯片名称 */
};

unsigned int fh_is_8626v100(void);
unsigned int fh_is_8852v200(void);
unsigned int fh_is_8856v200(void);
unsigned int fh_is_8858v200(void);
unsigned int fh_is_8856v201(void);
unsigned int fh_is_8858v201(void);
unsigned int fh_is_8852v210(void);
unsigned int fh_is_8856v210(void);
unsigned int fh_is_8858v210(void);
unsigned int fh_is_8652(void);
unsigned int fh_is_8656(void);
unsigned int fh_is_8658(void);
unsigned int fh_is_8852v201(void);
unsigned int fh_is_8852v301(void);
unsigned int fh_is_8662v100(void);
unsigned int fh_is_8626v200(void);
unsigned int fh_is_8852v202(void);
unsigned int fh_is_8636(void);
unsigned int fh_is_8856v300(void);
unsigned int fh_is_8856v310(void);
unsigned int fh_is_8852v310(void);
unsigned int fh_is_8858v300(void);
unsigned int fh_is_8858v310(void);
unsigned int fh_is_8862(void);
unsigned int fh_is_6322(void);
unsigned int fh_is_6326(void);
unsigned int fh_is_6322l(void);
unsigned int fh_is_6326l(void);
unsigned int fh_is_6321(void);
unsigned int fh_is_6321l(void);
unsigned int fh_is_3303(void);

#endif /* __FH_CHIPID_H__ */
