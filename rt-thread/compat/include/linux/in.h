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
#ifndef __LINUX_IN_H_
#define __LINUX_IN_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>
#define IPPROTO_IP         0
#define IPPROTO_ICMP       1
#define IPPROTO_TCP        6
#define IPPROTO_UDP        17
#define IPPROTO_IPV6       41
#define IPPROTO_UDPLITE    136
#define IPPROTO_RAW        255

#define IP_TOS             1
#define IP_TTL             2
/* #define IP_PKTINFO         8 */

/* Options and types related to multicast membership */
#define IP_ADD_MEMBERSHIP  35
#define IP_DROP_MEMBERSHIP 36

/** 255.255.255.255 */
#define IPADDR_NONE         ((unsigned int)0xffffffffUL)
/** 127.0.0.1 */
#define IPADDR_LOOPBACK     ((unsigned int)0x7f000001UL)
/** 0.0.0.0 */
#define IPADDR_ANY          ((unsigned int)0x00000000UL)
/** 255.255.255.255 */
#define IPADDR_BROADCAST    ((unsigned int)0xffffffffUL)

/** 255.255.255.255 */
#define INADDR_NONE         IPADDR_NONE
/** 127.0.0.1 */
#define INADDR_LOOPBACK     IPADDR_LOOPBACK
/** 0.0.0.0 */
#define INADDR_ANY          IPADDR_ANY
/** 255.255.255.255 */
#define INADDR_BROADCAST    IPADDR_BROADCAST

#define IP_CLASSB_NET       0xffff0000
#define IP_CLASSB_NSHIFT    16
#define IP_CLASSB_HOST      (0xffffffff & ~IP_CLASSB_NET)

#define IN_CLASSB_NET       IP_CLASSB_NET
#define IN_CLASSB_NSHIFT    IP_CLASSB_NSHIFT
#define IN_CLASSB_HOST      IP_CLASSB_HOST

struct in_addr
{
    in_addr_t s_addr;
};

struct sockaddr_in
{
    sa_family_t    sin_family;
    in_port_t      sin_port;
    struct in_addr sin_addr;
#define SIN_ZERO_LEN 8
    char            sin_zero[SIN_ZERO_LEN];
};

typedef struct ip_mreq
{
    struct in_addr imr_multiaddr; /* IP multicast address of group */
    struct in_addr imr_interface; /* local IP address of interface */
} ip_mreq;

struct ip_mreqn
{
    struct in_addr  imr_multiaddr;      /* IP multicast address of group */
    struct in_addr  imr_address;        /* local IP address of interface */
    int     imr_ifindex;        /* Interface index */
};

#ifdef __cplusplus
}
#endif

#endif
