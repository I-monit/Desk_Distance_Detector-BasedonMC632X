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
#ifndef __NETINET_IP_H
#define __NETINET_IP_H

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>
#include <netinet/in.h>

#define    IPVERSION    4               /* IP version number */
#define    IPDEFTTL    64        /* default ttl, from RFC 1340 */

struct iphdr
{
#if BYTE_ORDER == BIG_ENDIAN
    unsigned char    version:4;
    unsigned char    ihl:4;
#else /* BYTE_ORDER != BIG_ENDIAN */
    unsigned char    ihl:4;
    unsigned char    version:4;
#endif
    unsigned char    tos;
    unsigned short    tot_len;
    unsigned short     id;
    unsigned short     frag_off;
    unsigned char    ttl;
    unsigned char    protocol;
    unsigned short     check;
    unsigned int saddr;
    unsigned int daddr;
    /*The options start here. */
};


/*
 * Structure of an internet header, naked of options.
 */
struct ip
{
#if BYTE_ORDER == BIG_ENDIAN
    u_char ip_v:4;         /* version */
    u_char ip_hl:4;        /* header length */
#else
    u_char ip_hl:4;        /* header length */
    u_char ip_v:4;         /* version */
#endif
    u_char ip_tos;         /* type of service */
    u_short ip_len;        /* total length */
    u_short ip_id;         /* identification */
    u_short ip_off;        /* fragment offset field */
#define    IP_RF 0x8000       /* reserved fragment flag */
#define    IP_DF 0x4000       /* dont fragment flag */
#define    IP_MF 0x2000       /* more fragments flag */
#define    IP_OFFMASK 0x1fff  /* mask for fragmenting bits */
    u_char ip_ttl;         /* time to live */
    u_char ip_p;           /* protocol */
    u_short ip_sum;        /* checksum */
    struct in_addr ip_src, ip_dst;    /* source and dest address */
};

#ifdef __cplusplus
}
#endif

#endif
