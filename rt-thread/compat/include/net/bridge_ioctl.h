/*
 *  * Copyright (c) 2015-2019 Shanghai Fullhan Microelectronics Co., Ltd.
 *  *
 *  * SPDX-License-Identifier: Apache-2.0
 *  *
 *  * Change Logs:
 *  * Date           Author       Notes
 *  * 2019-01-18      {fullhan}   the first version
 *  *
 */
#ifndef __BRIDGE_IOCTL_H_
#define __BRIDGE_IOCTL_H_

#ifdef __cplusplus
extern "C" {
#endif

int br_netif_init(char *br_name);
int br_netif_deinit(char *br_name);

int br_add_if(char *br_name, char *name);
int br_del_if(char *br_name, char *name);

int br_set_hwaddr(char *hwaddr);

#ifdef __cplusplus
}
#endif

#endif