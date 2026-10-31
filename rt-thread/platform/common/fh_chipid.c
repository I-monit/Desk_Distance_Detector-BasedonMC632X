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

#include "rtthread.h"
#include "rtdebug.h"
#include "fh_chip.h"
#include "fh_pmu.h"
#include "fh_chipid.h"

#define CHIP_INFO(__plat_id, __chip_id, __chip_mask, chip, size) \
    { \
        ._plat_id = __plat_id, \
        ._chip_id = __chip_id, \
        ._chip_mask = __chip_mask, \
        .chip_id = FH_CHIP_##chip, \
        .ddr_size = size, \
        .chip_name = #chip, \
    },

#define RD_REG      0xffffffff

static struct fh_chip_info chip_infos[] = {
    CHIP_INFO(0x18112301, 0x0, 0x0, FH8626V100, 512)
    CHIP_INFO(0x19112201, 0x00000001, 0x00FFFFFF, FH8852V200, RD_REG)
    CHIP_INFO(0x19112201, 0x00100001, 0x00FFFFFF, FH8856V200, RD_REG)
    CHIP_INFO(0x19112201, 0x00410001, 0x00FFFFFF, FH8858V200, RD_REG)
    CHIP_INFO(0x19112201, 0x00200001, 0x00FFFFFF, FH8856V201, RD_REG)
    CHIP_INFO(0x19112201, 0x00300001, 0x00FFFFFF, FH8858V201, RD_REG)
    CHIP_INFO(0x19112201, 0x00000002, 0x00FFFFFF, FH8852V210, RD_REG)
    CHIP_INFO(0x19112201, 0x00100002, 0x00FFFFFF, FH8856V210, RD_REG)
    CHIP_INFO(0x19112201, 0x00410002, 0x00FFFFFF, FH8858V210, RD_REG)
    CHIP_INFO(0x20031601, 0x00410005, 0x00FFFFFF, FH8858V300, RD_REG)
    CHIP_INFO(0x22071801, 0x00410007, 0x00FFFFFF, FH8858V310, RD_REG)
    CHIP_INFO(0x20031601, 0x00100005, 0x00FFFFFF, FH8856V300, RD_REG)
    CHIP_INFO(0x22071801, 0x00100007, 0x00FFFFFF, FH8856V310, RD_REG)
    CHIP_INFO(0x22071801, 0x00000007, 0x00FFFFFF, FH8852V310, RD_REG)
    CHIP_INFO(0x23073101, 0x0000000b, 0x00FFFFFF, FH8852V301, 512)
    CHIP_INFO(0x23073101, 0x0001000b, 0x00FFFFFF, FH8662V100, 512)
    /*FH885XV300 must in front of fh865x*/
    CHIP_INFO(0x20031601, 0xc, 0xc, FH8652, RD_REG)
    CHIP_INFO(0x20031601, 0x8, 0xc, FH8656, RD_REG)
    CHIP_INFO(0x20031601, 0x4, 0xc, FH8658, RD_REG)
    CHIP_INFO(0x20102801, 0x00000003, 0x00FFFFFF, FH8852V201, 512)
    CHIP_INFO(0x20102801, 0x00020003, 0x00FFFFFF, FH8626V200, 512)
    CHIP_INFO(0x20102801, 0x00200003, 0x00FFFFFF, FH8852V202, 1024)
    CHIP_INFO(0x20102801, 0x00010003, 0x00FFFFFF, FH8636, 512)
    CHIP_INFO(0x21101901, 0x00400006, 0x00FFFFFF, FH8862, RD_REG)
    CHIP_INFO(0x20122801, 0x0000000a, 0x00FFFFFF, MC6322, RD_REG)
    CHIP_INFO(0x20122801, 0x0001000a, 0x00FFFFFF, MC6322L, RD_REG)
    CHIP_INFO(0x20122801, 0x0002000a, 0x00FFFFFF, MC6326L, RD_REG)
    CHIP_INFO(0x20122801, 0x0003000a, 0x00FFFFFF, MC6326, RD_REG)
    CHIP_INFO(0x20122801, 0x0000020a, 0x00FFFFFF, MC6322, RD_REG)
    CHIP_INFO(0x20122801, 0x0000170a, 0x00FFFFFF, MC6321, RD_REG)
    CHIP_INFO(0x20122801, 0x0000140a, 0x00FFFFFF, MC6321L, RD_REG)
    CHIP_INFO(0x20122801, 0x0000060a, 0x00FFFFFF, MC3303, RD_REG)
    CHIP_INFO(0x21101902, 0x00400007, 0x00FFFFFF, FH8856V500, RD_REG)
    CHIP_INFO(0x21101903, 0x00400008, 0x00FFFFFF, FH8857V500, RD_REG)
};

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#endif

#define FH_GET_CHIP_ID(plat_id, chip_id) \
    unsigned int plat_id = 0;\
    unsigned int chip_id = 0;\
    fh_get_chipid(&plat_id, &chip_id)


struct fh_chip_info *fh_get_chip_info(void)
{
    static struct fh_chip_info *chip_info = RT_NULL;
    struct fh_chip_info *info = RT_NULL;
    unsigned int plat_id = 0;
    unsigned int chip_id = 0;
    int i = 0;

    if (chip_info != RT_NULL)
        return chip_info;

    fh_get_chipid(&plat_id, &chip_id);
#ifdef REG_PMU_CHIP_INFO
    chip_id = fh_pmu_get_reg(REG_PMU_CHIP_INFO);
#endif

    for (i = 0; i < ARRAY_SIZE(chip_infos); i++)
    {
        info = &chip_infos[i];
        if (plat_id == info->_plat_id && (chip_id & info->_chip_mask) == info->_chip_id)
        {
            chip_info = info;
            if (chip_info->ddr_size == RD_REG)
                chip_info->ddr_size = fh_pmu_get_ddrsize();
            return info;
        }
    }
    rt_kprintf("Error: unknown chip\n");
    return RT_NULL;
}

unsigned int fh_get_ddrsize_mbit(void)
{
    struct fh_chip_info *info = fh_get_chip_info();

    if (info)
        return info->ddr_size;
    return 0;
}

char *fh_get_chipname(void)
{
    struct fh_chip_info *info = fh_get_chip_info();

    if (info)
        return info->chip_name;
    return "UNKNOWN";
}

#define DEFINE_FUNC_FH_IS(name, chip) \
unsigned int fh_is_##name(void) \
{ \
    struct fh_chip_info *info = fh_get_chip_info(); \
 \
    if (info) \
        return info->chip_id == FH_CHIP_##chip; \
    return 0; \
}

DEFINE_FUNC_FH_IS(8626v100, FH8626V100);
DEFINE_FUNC_FH_IS(8852v200, FH8852V200);
DEFINE_FUNC_FH_IS(8856v200, FH8856V200);
DEFINE_FUNC_FH_IS(8858v200, FH8858V200);
DEFINE_FUNC_FH_IS(8856v201, FH8856V201);
DEFINE_FUNC_FH_IS(8858v201, FH8858V201);
DEFINE_FUNC_FH_IS(8852v210, FH8852V210);
DEFINE_FUNC_FH_IS(8856v210, FH8856V210);
DEFINE_FUNC_FH_IS(8858v210, FH8858V210);
DEFINE_FUNC_FH_IS(8652, FH8652);
DEFINE_FUNC_FH_IS(8656, FH8656);
DEFINE_FUNC_FH_IS(8658, FH8658);
DEFINE_FUNC_FH_IS(8852v201, FH8852V201);
DEFINE_FUNC_FH_IS(8852v301, FH8852V301);
DEFINE_FUNC_FH_IS(8662v100, FH8662V100);
DEFINE_FUNC_FH_IS(8626v200, FH8626V200);
DEFINE_FUNC_FH_IS(8852v202, FH8852V202);
DEFINE_FUNC_FH_IS(8636, FH8636);
DEFINE_FUNC_FH_IS(8856v300, FH8856V300);
DEFINE_FUNC_FH_IS(8856v310, FH8856V310);
DEFINE_FUNC_FH_IS(8852v310, FH8852V310);
DEFINE_FUNC_FH_IS(8858v300, FH8858V300);
DEFINE_FUNC_FH_IS(8858v310, FH8858V310);
DEFINE_FUNC_FH_IS(8862, FH8862);
DEFINE_FUNC_FH_IS(6322, MC6322);
DEFINE_FUNC_FH_IS(6326, MC6326);
DEFINE_FUNC_FH_IS(6322L, MC6322L);
DEFINE_FUNC_FH_IS(6326L, MC6326L);
DEFINE_FUNC_FH_IS(6321, MC6321);
DEFINE_FUNC_FH_IS(6321L, MC6321L);
DEFINE_FUNC_FH_IS(3303, MC3303);
DEFINE_FUNC_FH_IS(8856v500, FH8856V500);
DEFINE_FUNC_FH_IS(8857v500, FH8857V500);

void fh_print_chip_info(void)
{
    FH_GET_CHIP_ID(plat_id, chip_id);
    rt_kprintf("chip_name\t: %s\n", fh_get_chipname());
    rt_kprintf("ddr_size\t: %dMbit\n", fh_get_ddrsize_mbit());
    rt_kprintf("plat_id\t\t: 0x%x\npkg_id\t\t: 0x%x\n",
            plat_id, chip_id);
}
#ifdef FINSH_USING_MSH
MSH_CMD_EXPORT_ALIAS(fh_print_chip_info,
        fh_chip_info, print chip infomation);
#else
FINSH_FUNCTION_EXPORT_ALIAS(fh_print_chip_info,
        fh_chip_info, print chip infomation);
#endif
