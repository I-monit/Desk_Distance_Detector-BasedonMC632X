/**
 * @file
 * raw API (to be used from TCPIP thread)\n
 * See also @ref link_raw
 */

/*
 * Copyright (c) 2001-2004 Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 * Author: Adam Dunkels <adam@sics.se>
 *
 */
#ifndef LWIP_HDR_RAWLINK_H
#define LWIP_HDR_RAWLINK_H

#include "lwip/opt.h"

#if LWIP_RAWLINK /* don't build if not configured for use in lwipopts.h */

#include "lwip/pbuf.h"
#include "lwip/def.h"
#include "lwip/sockets.h"
// #include "lwip/ip.h"
// #include "lwip/ip_addr.h"
// #include "lwip/ip6_addr.h"

#ifdef __cplusplus
extern "C" {
#endif

struct rlink_pcb;

/** Function prototype for raw pcb receive callback functions.
 * @param arg user supplied argument (raw_pcb.recv_arg)
 * @param pcb the raw_pcb which received data
 * @param p the packet buffer that was received
 * @param addr the remote IP address from which the packet was received
 * @return 1 if the packet was 'eaten' (aka. deleted),
 *         0 if the packet lives on
 * If returning 1, the callback is responsible for freeing the pbuf
 * if it's not used any more.
 */
typedef u8_t (*rlink_recv_fn)(void *arg, struct rlink_pcb *pcb, struct pbuf *p,
    const ip_addr_t *addr);

/** the RAW protocol control block */
struct rlink_pcb {
  /* Common members of all PCB types */
  // IP_PCB;        // link-layer has nothing to do with IP-layer definitions
  u32_t ifindex;

  struct rlink_pcb *next;

  u16_t protocol;
  u16_t type;
  unsigned char destmac[6];

  /** receive callback function */
  rlink_recv_fn recv;
  /* user-supplied argument for the recv callback */
  void *recv_arg;
};

/* The following functions is the application layer interface to the
   RAW code. */
struct rlink_pcb * rlink_new        (u16_t proto, u16_t type);
void             rlink_remove     (struct rlink_pcb *pcb);
err_t            rlink_bind       (struct rlink_pcb *pcb, u32_t ifindex, u16_t protocol);
err_t            rlink_connect    (struct rlink_pcb *pcb, u16_t ifindex);

err_t            rlink_send       (struct rlink_pcb *pcb, struct pbuf *p);

void             rlink_recv       (struct rlink_pcb *pcb, rlink_recv_fn recv, void *recv_arg);

/* The following functions are the lower layer interface to RAW. */
u8_t             rlink_input      (struct pbuf *p, struct netif *inp);
#define rlink_init() /* Compatibility define, no init needed. */

// void raw_netif_ip_addr_changed(const ip_addr_t* old_addr, const ip_addr_t* new_addr);

/* for compatibility with older implementation */
// #define raw_new_ip6(proto) raw_new_ip_type(IPADDR_TYPE_V6, proto)

#ifdef __cplusplus
}
#endif

#endif /* LWIP_RAWLINK */

#endif /* LWIP_HDR_RAWLINK_H */
