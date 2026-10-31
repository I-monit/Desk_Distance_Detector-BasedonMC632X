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
#ifndef __NET_ETHERNET_H_
#define __NET_ETHERNET_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "linux/if_ether.h"

#define ETHERTYPE_IP        0x0800      /* IP */
#define ETHERTYPE_ARP       0x0806      /* Address resolution */

struct ether_addr
{
  unsigned char ether_addr_octet[ETH_ALEN];
}__attribute__((__packed__));


/* 10Mb/s ethernet header */
struct ether_header
{
  unsigned char   ether_dhost[ETH_ALEN];  /* destination eth addr */
  unsigned char  ether_shost[ETH_ALEN];  /* source ether addr    */
  unsigned short ether_type;             /* packet type ID field */
}__attribute__((__packed__));

#ifdef __cplusplus
}
#endif

#endif
