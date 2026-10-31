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
#ifndef __NET_IF_H
#define __NET_IF_H

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/socket.h>

#define IFF_UP          0x1     /* interface is up      */
#define IFF_BROADCAST   0x2     /* broadcast address valid  */
#define IFF_LOOPBACK    0x8     /* is a loopback net        */
#define IFF_POINTOPOINT 0x10    /* interface is has p-p link    */
#define IFF_RUNNING     0x40    /* interface RFC2863 OPER_UP    */
#define IFF_NOARP       0x80    /* no ARP protocol */

#define IFF_PROMISC     0x100   /* receive all packets */
#define IFF_ALLMULTI    0x200   /* receive all multicast packets*/
#define IFF_MULTICAST   0x1000  /* Supports multicast */

#define IFNAMSIZ 16

struct ifreq
{
#define IFHWADDRLEN 6
    union
    {
        char    ifrn_name[IFNAMSIZ];        /* if name, e.g. "en0" */
    } ifr_ifrn;

    union {
        struct  sockaddr ifru_addr;
        struct  sockaddr ifru_dstaddr;
        struct  sockaddr ifru_broadaddr;
        struct  sockaddr ifru_netmask;
        struct  sockaddr ifru_hwaddr;
        short   ifru_flags;
        int ifru_ivalue;
        int ifru_mtu;
        char    ifru_slave[IFNAMSIZ];   /* Just fits the size */
        char    ifru_newname[IFNAMSIZ];
        void    *ifru_data;
    } ifr_ifru;
};

#define ifr_name    ifr_ifrn.ifrn_name  /* interface name   */
#define ifr_hwaddr  ifr_ifru.ifru_hwaddr    /* MAC address      */
#define ifr_addr    ifr_ifru.ifru_addr  /* address      */
#define ifr_dstaddr ifr_ifru.ifru_dstaddr   /* other end of p-p lnk */
#define ifr_broadaddr   ifr_ifru.ifru_broadaddr /* broadcast address    */
#define ifr_netmask ifr_ifru.ifru_netmask   /* interface net mask   */
#define ifr_flags   ifr_ifru.ifru_flags /* flags        */
#define ifr_metric  ifr_ifru.ifru_ivalue    /* metric       */
#define ifr_mtu     ifr_ifru.ifru_mtu   /* mtu          */
#define ifr_slave   ifr_ifru.ifru_slave /* slave device     */
#define ifr_data    ifr_ifru.ifru_data  /* for use by interface */
#define ifr_ifindex ifr_ifru.ifru_ivalue    /* interface index  */
#define ifr_bandwidth   ifr_ifru.ifru_ivalue    /* link bandwidth   */
#define ifr_qlen    ifr_ifru.ifru_ivalue    /* Queue length     */
#define ifr_newname ifr_ifru.ifru_newname   /* New name     */

/*
 * Structure used in SIOCGIFCONF request.
 * Used to retrieve interface configuration
 * for machine (useful for programs which
 * must know all networks accessible).
 */

struct ifconf
{
    int ifc_len;            /* size of buffer   */
    union {
        char *ifcu_buf;
        struct ifreq *ifcu_req;
    } ifc_ifcu;
};
#define ifc_buf ifc_ifcu.ifcu_buf       /* buffer address   */
#define ifc_req ifc_ifcu.ifcu_req       /* array of structures  */

int eth_ioctl(int c, long cmd, void *argp);

#ifdef __cplusplus
}
#endif

#endif
