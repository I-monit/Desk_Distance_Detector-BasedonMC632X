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

#ifndef __FH_USBOTG_H__
#define __FH_USBOTG_H__

#include "fh_def.h"

struct fh_usbotg_obj
{
    int id;
    UINT32 irq;
    UINT32 base;
    void *data;
    void (*utmi_rst)(void);
    void (*phy_rst)(void);
    void (*hcd_resume)(void);
    void (*power_on)(void);
    void (*vbus_vldext)(void);
};


#endif /* FH_USBOTG_H_ */
