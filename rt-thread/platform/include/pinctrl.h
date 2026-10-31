/**
 * Copyright (c) 2015-2019 Shanghai Fullhan Microelectronics Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 *
 * Change Logs:
 * Date           Author       Notes
 * 2019-01-08     wangyl307    zy&zy2 compatible support
 * 2019-01-14     wangyl307    remove pad fix funcs and defs
 */

#ifndef __PINCTRL_H__
#define __PINCTRL_H__
#include "pinctrl_osdep.h"

#if defined(CONFIG_CHIP_FH8862) || defined(CONFIG_ARCH_FH885xV500)
#define MUX_NUM             (9)
#else
#define MUX_NUM             (6)
#endif

#define PINCTRL_UNUSED      (-1)

#define PUPD_NONE           (0)
#define PUPD_UP             (1)
#define PUPD_DOWN           (2)

#define INPUT_DISABLE       (0)
#define INPUT_ENABLE        (1)
#define OUTPUT_DISABLE      (0)
#define OUTPUT_ENABLE       (1)
#if defined(CONFIG_CHIP_FH8626V100)
#define PUPD_DISABLE        (1)
#define PUPD_ENABLE         (0)
#define PUPD_ZERO           (0)
#else
#define PUPD_DISABLE        (0)
#define PUPD_ENABLE         (1)
#define PUPD_ZERO           (0)
#endif

#define FUNC0               (0)
#define FUNC1               (1)
#define FUNC2               (2)
#define FUNC3               (3)
#define FUNC4               (4)
#define FUNC5               (5)
#define FUNC6               (6)
#define FUNC7               (7)
#define FUNC8               (8)
#define FUNC9               (9)

#define NEED_CHECK_PINLIST  (1)
#define PIN_BACKUP          (1<<1)
#define PIN_RESTORE         (1<<2)
#define NO_PRINT            (1<<3)

#if defined(CONFIG_ARCH_MC632X)
#define PINCTRL_FUNC(name, id, sel, pupd, ds, sys, reg_off)  \
PinCtrl_Pin PAD##id##_##name =      \
{                                   \
    .pad_id                = id,        \
    .func_name            = #name,    \
    .reg_offset            = (id * 4),    \
    .func_sel            = sel,        \
    .pullup_pulldown    = pupd,        \
    .driving_curr        = ds,        \
    .output_enable        = 1,        \
    .reg    = (PinCtrl_Register *)((sys##_SYS_PIN_REG_BASE)+(reg_off)),\
}

typedef union {
    struct {
        unsigned int pus    : 1;    /*0*/
        unsigned int pun    : 1;    /*1*/
        unsigned int pdn    : 1;    /*2*/
        unsigned int st        : 1;    /*3*/
        unsigned int sr        : 1;    /*4*/
        unsigned int ds        : 3;    /*5~7*/
        unsigned int        : 24;    /*8~31*/
    } bit;
    unsigned int dw;
} PinCtrl_Register;
#else

typedef union {
    struct {
        unsigned int sl     : 1;    /*0*/
        unsigned int        : 3;    /*1~3*/
        unsigned int ds     : 3;    /*4~6*/
        unsigned int msc    : 1;    /*7*/
        unsigned int st     : 1;    /*8*/
        unsigned int        : 3;    /*9~11*/
        unsigned int ie     : 1;    /*12*/
        unsigned int        : 3;    /*13~15*/
        unsigned int pdn    : 1;    /*16*/
        unsigned int        : 3;    /*17~19*/
        unsigned int pun    : 1;    /*20*/
        unsigned int        : 3;    /*21~23*/
        unsigned int mfs    : 4;    /*24~27*/
        unsigned int oe     : 1;    /*28*/
        unsigned int        : 3;    /*29~31*/
    } bit;
    unsigned int dw;
} PinCtrl_Register;

#define PINCTRL_FUNC(name, id, sel, pupd, ds)       \
PinCtrl_Pin PAD##id##_##name =                      \
{                                                   \
    .pad_id         = id,                           \
    .func_name      = #name,                        \
    .reg_offset     = (id * 4),                     \
    .func_sel       = sel,                          \
    .pullup_pulldown = pupd,                        \
    .driving_curr = ds,                             \
    .output_enable  = 1,                            \
}
#endif

#define PINCTRL_MUX(pname, sel, ...)                \
PinCtrl_Mux MUX_##pname =                           \
{                                                   \
    .mux_pin = { __VA_ARGS__ },                     \
    .cur_pin = sel,                                 \
}

#define PINCTRL_DEVICE(name, count, ...)            \
typedef struct                                      \
{                                                   \
    char *dev_name;                                 \
    int mux_count;                                  \
    OS_LIST list;                                   \
    PinCtrl_Mux *mux[count];                        \
} PinCtrl_Device_##name;                            \
PinCtrl_Device_##name pinctrl_dev_##name =          \
{                                                   \
     .dev_name = #name,                             \
     .mux_count = count,                            \
     .mux = { __VA_ARGS__ },                        \
}

typedef struct
{
    char *func_name;
    PinCtrl_Register *reg;
    unsigned int pad_id             : 8;
    unsigned int reg_offset         : 12;
    unsigned int func_sel           : 4;
    unsigned int input_enable       : 1;
    unsigned int output_enable      : 1;
    unsigned int pullup_pulldown    : 2;
    unsigned int volt_mode          : 1;
    unsigned int driving_curr       : 3;
}PinCtrl_Pin;

typedef struct
{
    /* todo: int lock; */
    int cur_pin;
    PinCtrl_Pin *mux_pin[MUX_NUM];
} PinCtrl_Mux;

typedef struct
{
    char *dev_name;
    int mux_count;
    OS_LIST list;
    void *mux;
}PinCtrl_Device;

void fh_pinctrl_init(unsigned int base);
void fh_pinctrl_prt(void);
int fh_pinctrl_smux(char *devname, char *muxname, int muxsel, unsigned int flag);
int fh_pinctrl_sdev(char *devname, unsigned int flag);
void fh_pinctrl_init_devicelist(OS_LIST *list);
int fh_select_gpio(int gpio);
int fh_pinctrl_ds(char *pin_name, unsigned int ds);
int fh_pinctrl_pupd(char *pin_name, char *pupd);
int fh_pinctrl_set_oe(char *pin_name, unsigned int oe);
char *fh_pinctrl_smux_backup(char *devname, char *muxname, int muxsel);
char *fh_pinctrl_smux_restore(char *devname, char *muxname, int muxsel);
#endif /* PINCTRL_H_ */
