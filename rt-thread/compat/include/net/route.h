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
#ifndef __ADAPTER_ROUTE_H__
#define __ADAPTER_ROUTE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "sys/socket.h"
#include "netinet/in.h"

#define RTF_UP      0x0001      /* Route usable.  */
#define RTF_GATEWAY 0x0002      /* Destination is a gateway.  */
#define RTF_HOST    0x0004      /* host entry (net otherwise)   */

/* This structure gets passed by the SIOCADDRT and SIOCDELRT calls. */
struct rtentry
{
    unsigned long   rt_pad1;
    struct sockaddr rt_dst;     /* target address       */
    struct sockaddr rt_gateway; /* gateway addr (RTF_GATEWAY)   */
    struct sockaddr rt_genmask; /* target network mask (IP) */
    unsigned short  rt_flags;
    short       rt_pad2;
    unsigned long   rt_pad3;
    void        *rt_pad4;
    short       rt_metric;  /* +1 for binary compatibility! */
    char *rt_dev;   /* forcing the device at add    */
    unsigned long   rt_mtu;     /* per route MTU/Window     */
#define rt_mss  rt_mtu          /* Compatibility :-(            */
    unsigned long   rt_window;  /* Window clamping      */
    unsigned short  rt_irtt;    /* Initial RTT          */
};

#ifdef __cplusplus
}
#endif

#endif
