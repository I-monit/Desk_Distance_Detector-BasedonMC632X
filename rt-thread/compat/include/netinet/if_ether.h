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
#ifndef __ETHERNET_IF_ETHER_H_
#define __ETHERNET_IF_ETHER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "linux/if_ether.h"
#include "net/if_arp.h"
#include <net/ethernet.h>

struct ether_arp
{
    struct  arphdr ea_hdr;      /* fixed-size header */
    unsigned char arp_sha[ETH_ALEN]; /* sender hardware address */
    unsigned char arp_spa[4];        /* sender protocol address */
    unsigned char arp_tha[ETH_ALEN]; /* target hardware address */
    unsigned char arp_tpa[4];        /* target protocol address */
};

#define arp_hrd ea_hdr.ar_hrd
#define arp_pro ea_hdr.ar_pro
#define arp_hln ea_hdr.ar_hln
#define arp_pln ea_hdr.ar_pln
#define arp_op  ea_hdr.ar_op

#ifdef __cplusplus
}
#endif

#endif
