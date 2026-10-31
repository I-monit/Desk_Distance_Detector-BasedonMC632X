/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2015-02-17     Bernard      First version
 */

#ifndef IN_H__
#define IN_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/socket.h>
#include <sys/types.h>

typedef uint32_t in_addr_t;
/* typedef unsigned short in_port_t;    */
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

struct in6_addr
{
    union
    {
        unsigned int u32_addr[4];
        unsigned char u8_addr[16];
    } un;
#define s6_addr  un.u8_addr
};

typedef struct ipv6_mreq
{
    struct in6_addr ipv6mr_multiaddr;
    unsigned int ipv6mr_interface;
}ipv6_mreq;

struct sockaddr_in6
{
    sa_family_t     sin6_family;   /* AF_INET6                    */
    in_port_t       sin6_port;     /* Transport layer port #      */
    unsigned int           sin6_flowinfo; /* IPv6 flow information       */
    struct in6_addr sin6_addr;     /* IPv6 address                */
    unsigned int           sin6_scope_id; /* Set of interfaces for scope */
};

typedef struct ip_mreq
{
    struct in_addr imr_multiaddr; /* IP multicast address of group */
    struct in_addr imr_interface; /* local IP address of interface */
} ip_mreq;

#define IPPROTO_IP      0
#define IPPROTO_ICMP    1
#define IPPROTO_TCP     6
#define IPPROTO_UDP     17
#define IPPROTO_IPV6    41
#define IPPROTO_ICMPV6  58
#define IPPROTO_UDPLITE 136
#define IPPROTO_RAW     255

#define IP_TOS             1
#define IP_TTL             2
/* #define IP_PKTINFO         8 */

/* Options and types related to multicast membership */
#define IP_ADD_MEMBERSHIP  35
#define IP_DROP_MEMBERSHIP 36

#define IP_MULTICAST_IF    32
#define IP_MULTICAST_TTL   33
#define IP_MULTICAST_LOOP  34

unsigned int htonl(unsigned int x);
#define ntohl(x) htonl(x)
unsigned short htons(unsigned short x);
#define ntohs(x) htons(x)

#if 0
#define PP_HTONL(x) ((((x) & 0x000000ffUL) << 24) | \
                     (((x) & 0x0000ff00UL) <<  8) | \
                     (((x) & 0x00ff0000UL) >>  8) | \
                     (((x) & 0xff000000UL) >> 24))

#define IN6ADDR_LOOPBACK_INIT {{{0,0,0,PP_HTONL(1)}}}
#endif
#define IN6ADDR_LOOPBACK_INIT { { { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 } } }

#define IP_CLASSB_NET       0xffff0000
#define IP_CLASSB_NSHIFT    16
#define IP_CLASSB_HOST      (0xffffffff & ~IP_CLASSB_NET)

#define IN_CLASSB_NET       IP_CLASSB_NET
#define IN_CLASSB_NSHIFT    IP_CLASSB_NSHIFT
#define IN_CLASSB_HOST      IP_CLASSB_HOST

#ifndef INET_ADDRSTRLEN
#define INET_ADDRSTRLEN     16
#endif

#ifndef INET6_ADDRSTRLEN
#define INET6_ADDRSTRLEN    46
#endif

#ifndef IN6_IS_ADDR_MULTICAST
#define IN6_IS_ADDR_MULTICAST(a) (((uint8_t *) (a))[0] == 0xff)
#endif

#ifndef IN6_IS_ADDR_LINKLOCAL
#define IN6_IS_ADDR_LINKLOCAL(a) \
    ((((uint32_t *) (a))[0] & htonl (0xffc00000)) == htonl (0xfe800000))
#endif
extern const struct in6_addr in6addr_any;

#ifdef __cplusplus
}
#endif

#endif
