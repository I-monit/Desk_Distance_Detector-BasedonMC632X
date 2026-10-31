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
#ifndef __LINUX_IF_ARP_H_
#define __LINUX_IF_ARP_H_

#ifdef __cplusplus
extern "C" {
#endif

#define ARPHRD_ETHER    1       /* Ethernet 10Mbps */
#define ARPOP_REQUEST   1       /* ARP request */
#define ARPOP_REPLY 2           /* ARP reply */
#define ARPOP_RREQUEST  3       /* RARP request */

struct arphdr
{
    unsigned short      ar_hrd;     /* format of hardware address   */
    unsigned short      ar_pro;     /* format of protocol address   */
    unsigned char       ar_hln;     /* length of hardware address   */
    unsigned char       ar_pln;     /* length of protocol address   */
    unsigned short      ar_op;      /* ARP opcode (command)     */

    #if 1
     /*
      *     Ethernet looks like this : This bit is variable sized however...
      */
    unsigned char        ar_sha[ETH_ALEN];    /* sender hardware address    */
    unsigned char        ar_sip[4];           /* sender IP address        */
    unsigned char        ar_tha[ETH_ALEN];    /* target hardware address    */
    unsigned char        ar_tip[4];           /* target IP address        */
    #endif
};

#ifdef __cplusplus
}
#endif

#endif
