/**
 * @file
 * Implementation of rlink protocol PCBs for low-level handling of
 * different types of protocols besides (or overriding) those
 * already available in lwIP.\n
 * See also @ref rlink_raw
 * 
 * @defgroup rlink_raw RAW
 * @ingroup callbackstyle_api
 * Implementation of rlink protocol PCBs for low-level handling of
 * different types of protocols besides (or overriding) those
 * already available in lwIP.\n
 * @see @ref rlink_api
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

#include "lwip/opt.h"

#if LWIP_RAWLINK /* don't build if not configured for use in lwipopts.h */
#include "lwip/def.h"
#include "lwip/memp.h"
#include "lwip/ip_addr.h"
#include "lwip/netif.h"
#include "lwip/rlink.h"
#include "lwip/stats.h"
#include "lwip/ip6.h"
#include "lwip/ip6_addr.h"
#include "lwip/inet_chksum.h"
#ifdef RT_USING_COMPONENTS_LINUX_ADAPTER
#include "linux/if_packet.h"      /* struct sockaddr_ll */
#endif
#include <string.h>

/** The list of RAW PCBs */
static struct rlink_pcb *rlink_pcbs;

/* #define RLINK_DEBUG */
#ifdef  RLINK_DEBUG
#define rlink_debug     rt_kprintf
#else
#define rlink_debug( fmt, arg...)
#endif

extern err_t hwaddr_from_index( int, unsigned char *);
static u8_t
rlink_input_match(struct rlink_pcb *pcb, u8_t *macaddr, u16_t proto)
{
  unsigned char desadr[6];

  if( pcb->protocol == 0x0300 )     /* receive all packets */
    return 1;

  if( pcb->protocol != proto )
  {
    /* not match. */
    rlink_debug( "protocol not match.\n" );
    return 0;
  }


  hwaddr_from_index( pcb->ifindex, desadr );
  if( rt_memcmp( desadr, macaddr, 6 ) == 0 )        /* for us? */
  {
    rlink_debug( "Exact match.\n" );
    return 1;
  }
  rt_memset( desadr, 0xff, 6 );             /* accept broadcast */
  if( rt_memcmp( desadr, macaddr, 6 ) == 0 )
  {
    rlink_debug( "broadcast match.\n" );
    return 1;
  }

  rlink_debug( "not match.\n" );
  return 0;
}

/**
 * Determine if in incoming IP packet is covered by a RAW PCB
 * and if so, pass it to a user-provided receive callback function.
 *
 * Given an incoming IP datagram (as a chain of pbufs) this function
 * finds a corresponding RAW PCB and calls the corresponding receive
 * callback function.
 *
 * @param p pbuf to be demultiplexed to a RAW PCB.
 * @param inp network interface on which the datagram was received.
 * @return - 1 if the packet has been eaten by a RAW PCB receive
 *           callback function. The caller MAY NOT not reference the
 *           packet any longer, and MAY NOT call pbuf_free().
 * @return - 0 if packet is not eaten (pbuf is still referenced by the
 *           caller).
 *
 */
u8_t
rlink_input(struct pbuf *p, struct netif *inp)
{
  struct rlink_pcb *pcb, *prev;
  u16_t proto;
  u8_t eaten = 0;
  u16_t *pload = (u16_t *)p->payload;
  u8_t *dstmac;

  LWIP_UNUSED_ARG(inp);

  proto = pload[6];
  dstmac = (u8_t *)&pload[0];
  prev = NULL;
  pcb = rlink_pcbs;
  /* loop through all rlink pcbs until the packet is eaten by one */
  /* this allows multiple pcbs to match against the packet by design */
  while ((eaten == 0) && (pcb != NULL)) {
    if( proto != 0x608 && proto != 8 )    /* skip IP/ARP */
      rlink_debug( "input protocol: %x/%x\n", proto, pcb->protocol );
    if ( rlink_input_match(pcb, dstmac, proto))
    {
      /* receive callback function available? */
      if (pcb->recv != NULL) {
#ifndef LWIP_NOASSERT
        void* old_payload = p->payload;
#endif
        /* the receive callback function did not eat the packet? */
        eaten = pcb->recv( pcb->recv_arg, pcb, p, 0 );
        rlink_debug( "eaten = %d\n", eaten );
        if (eaten != 0) {
          /* receive function ate the packet */
          p = NULL;
          eaten = 1;
          if (prev != NULL) {
          /* move the pcb to the front of rlink_pcbs so that is
            found faster next time */
            prev->next = pcb->next;
            pcb->next = rlink_pcbs;
            rlink_pcbs = pcb;
          }
        } else {
          /* sanity-check that the receive callback did not alter the pbuf */
          LWIP_ASSERT("rlink pcb recv callback altered pbuf payload pointer without eating packet",
          p->payload == old_payload);
        }
      }
      /* no receive callback function was set for this rlink PCB */
    }
      /* drop the packet */
      prev = pcb;
      pcb = pcb->next;
  }
  return eaten;
}

/**
 * @ingroup rlink_raw
 * Bind a RAW PCB.
 *
 * @param pcb RAW PCB to be bound with a local address ipaddr.
 * @param ipaddr local IP address to bind with. Use IP4_ADDR_ANY to
 * bind to all local interfaces.
 *
 * @return lwIP error code.
 * - ERR_OK. Successful. No error occurred.
 * - ERR_USE. The specified IP address is already bound to by
 * another RAW PCB.
 *
 * @see rlink_disconnect()
 */
err_t
rlink_bind(struct rlink_pcb *pcb, u32_t ifindex, u16_t protocol)
{
  unsigned char macaddr[6];

  if ((pcb == NULL) ) {
    rlink_debug( "NULL pcb.\n" );
    return ERR_VAL;
  }
#if 0
  if( pcb->protocol != protocol )
  {
    rlink_debug("protocol not match.\n");
    return ERR_VAL;
  }
#endif

  if (hwaddr_from_index(ifindex, macaddr) != ERR_OK)
  {
    rlink_debug("invalid ifindex.\n");
    return ERR_VAL;
  }
  pcb->ifindex = ifindex;
  rlink_debug( "rlink bind ok.\n" );
  return ERR_OK;
}

/**
 * @ingroup rlink_raw
 * Connect an RAW PCB. This function is required by upper layers
 * of lwip. Using the rlink api you could use rlink_sendto() instead
 *
 * This will associate the RAW PCB with the remote address.
 *
 * @param pcb RAW PCB to be connected with remote address ipaddr and port.
 * @param ipaddr remote IP address to connect with.
 *
 * @return lwIP error code
 *
 * @see rlink_disconnect() and rlink_sendto()
 */
err_t
rlink_connect(struct rlink_pcb *pcb, u16_t ifindex )
{
  if ((pcb == NULL)) {
    return ERR_VAL;
  }

  pcb->ifindex = ifindex;
  return ERR_OK;
}

/**
 * @ingroup rlink_raw
 * Set the callback function for received packets that match the
 * rlink PCB's protocol and binding.
 *
 * The callback function MUST either
 * - eat the packet by calling pbuf_free() and returning non-zero. The
 *   packet will not be passed to other rlink PCBs or other protocol layers.
 * - not free the packet, and return zero. The packet will be matched
 *   against further PCBs and/or forwarded to another protocol layers.
 */
void
rlink_recv(struct rlink_pcb *pcb, rlink_recv_fn recv, void *recv_arg)
{
  /* remember recv() callback and user data */
  pcb->recv = recv;
  pcb->recv_arg = recv_arg;
}

/**
 * @ingroup rlink_raw
 * Send the rlink IP packet to the address given by rlink_connect()
 *
 * @param pcb the rlink pcb which to send
 * @param p the IP payload to send
 *
 */
err_t
rlink_send(struct rlink_pcb *pcb, struct pbuf *p)
{
  err_t err = RT_EOK;
  struct netif *netif;
  struct pbuf *q;

  if ((pcb == NULL) ) {
    return ERR_VAL;
  }

  netif = netif_get_by_index( pcb->ifindex );

  if (netif == NULL) {
    LWIP_DEBUGF(RAW_DEBUG | LWIP_DBG_LEVEL_WARNING, ("rlink_sendto: No route to "));
    return ERR_RTE;
  }
  LWIP_DEBUGF(RAW_DEBUG | LWIP_DBG_TRACE, ("rlink_sendto\n"));
  if( pcb->type )        /* if SOCK_DGRAM type */
  {
    q = pbuf_alloc( PBUF_LINK, 0, PBUF_RAM );
    if ( q == RT_NULL )
    {
      rlink_debug("malloc qbuf failed.\n" );
      return ERR_MEM;
    }
    /* fill qbuf */
    /* if( pbuf_header( q, PBUF_LINK_HLEN ) != 0 ) */
    if( pbuf_header( q, 14 ) != 0 )
    {
      rlink_debug("fill header failed.\n" );
      pbuf_free(q);
      return ERR_MEM;
    }
    rt_memcpy( q->payload, pcb->destmac, 6 );
    /* get src mac */
    if( hwaddr_from_index( pcb->ifindex, q->payload+6 ) != ERR_OK )
    {
      rlink_debug( "get hwaddr failed.\n" );
      pbuf_free(q);
      return ERR_VAL;
    }
    rt_memcpy( q->payload + 12, &pcb->protocol, 2 );
    if( p->tot_len != 0 ){
      pbuf_chain(q, p);
    }
    netif->linkoutput(netif, q);
  }
  else
  {
    netif->linkoutput(netif, p);
  }

  return err;
}

/**
 * @ingroup rlink_raw
 * Remove an RAW PCB.
 *
 * @param pcb RAW PCB to be removed. The PCB is removed from the list of
 * RAW PCB's and the data structure is freed from memory.
 *
 * @see rlink_new()
 */
void
rlink_remove(struct rlink_pcb *pcb)
{
  struct rlink_pcb *pcb2;
  /* pcb to be removed is first in list? */
  if (rlink_pcbs == pcb) {
    /* make list start at 2nd pcb */
    rlink_pcbs = rlink_pcbs->next;
    /* pcb not 1st in list */
  } else {
    for (pcb2 = rlink_pcbs; pcb2 != NULL; pcb2 = pcb2->next) {
      /* find pcb in rlink_pcbs list */
      if (pcb2->next != NULL && pcb2->next == pcb) {
        /* remove pcb from list */
        pcb2->next = pcb->next;
        break;
      }
    }
  }
  memp_free(MEMP_RAWLINK_PCB, pcb);
}

/**
 * @ingroup rlink_raw
 * Create a RAW PCB.
 *
 * @return The RAW PCB which was created. NULL if the PCB data structure
 * could not be allocated.
 *
 * @param proto the protocol number of the IPs payload (e.g. IP_PROTO_ICMP)
 *
 * @see rlink_remove()
 */
struct rlink_pcb *
rlink_new(u16_t proto, u16_t type)
{
  struct rlink_pcb *pcb;

  LWIP_DEBUGF(RAW_DEBUG | LWIP_DBG_TRACE, ("rlink_new\n"));

  pcb = (struct rlink_pcb *)memp_malloc(MEMP_RAWLINK_PCB);
  /* could allocate RAW PCB? */
  if (pcb != NULL) {
    /* initialize PCB to all zeroes */
    memset(pcb, 0, sizeof(struct rlink_pcb));
    pcb->protocol = proto;
    pcb->type = type;
    /* pcb->ttl = RAW_TTL; */
    pcb->next = rlink_pcbs;
    rlink_pcbs = pcb;
  }
  return pcb;
}

/**
 * @ingroup rlink_raw
 * Create a RAW PCB for specific IP type.
 *
 * @return The RAW PCB which was created. NULL if the PCB data structure
 * could not be allocated.
 *
 * @param type IP address type, see @ref lwip_ip_addr_type definitions.
 * If you want to listen to IPv4 and IPv6 (dual-stack) packets,
 * supply @ref IPADDR_TYPE_ANY as argument and bind to @ref IP_ANY_TYPE.
 * @param proto the protocol number (next header) of the IPv6 packet payload
 *              (e.g. IP6_NEXTH_ICMP6)
 *
 * @see rlink_remove()
 */
#if 0
struct rlink_pcb *
rlink_new_ip_type(u8_t type, u16_t proto)
{
  struct rlink_pcb *pcb;
  pcb = rlink_new(proto);
#if LWIP_IPV4 && LWIP_IPV6
  if (pcb != NULL) {
    IP_SET_TYPE_VAL(pcb->local_ip,  type);           /* shall we add new MAC addr type? */
    IP_SET_TYPE_VAL(pcb->remote_ip, type);
  }
#else /* LWIP_IPV4 && LWIP_IPV6 */
  LWIP_UNUSED_ARG(type);
#endif /* LWIP_IPV4 && LWIP_IPV6 */
  return pcb;
}
#endif

/** This function is called from netif.c when address is changed
 *
 * @param old_addr IP address of the netif before change
 * @param new_addr IP address of the netif after change
 */
void rlink_netif_ip_addr_changed(const ip_addr_t* old_addr, const ip_addr_t* new_addr)
{
  struct rlink_pcb* rpcb;

 /* if (!ip_addr_isany(old_addr) && !ip_addr_isany(new_addr))          // added by sunh@20171123 */
  {
    for (rpcb = rlink_pcbs; rpcb != NULL; rpcb = rpcb->next) {
      /* PCB bound to current local interface address? */
      /* if (ip_addr_cmp(&rpcb->local_ip, old_addr))            // added by sunh@20171123 */
      {
        /* The PCB is bound to the old ipaddr and
         * is set to bound to the new one instead */
        /* ip_addr_copy(rpcb->local_ip, *new_addr);         // added by sunh@20171123 */
      }
    }
  }
}

#endif /* LWIP_RAW */
