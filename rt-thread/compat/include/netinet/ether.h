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

#ifndef __ETHERNET_ETHER_H_
#define __ETHERNET_ETHER_H_

#ifdef __cplusplus
extern "C" {
#endif
#include <netinet/if_ether.h>

extern char *ether_ntoa (const struct ether_addr *addr);
extern char *ether_ntoa_r(const struct ether_addr *addr, char *buf);
extern struct ether_addr *ether_aton (const char *asc);
extern struct ether_addr *ether_aton_r(const char *asc, struct ether_addr *addr);


#ifdef __cplusplus
}
#endif

#endif