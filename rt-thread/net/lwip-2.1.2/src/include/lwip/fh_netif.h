/**
 * @file
 * netif API (to be used from non-TCPIP threads)
 */

/*
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
 */
#ifndef LWIP_HDR_FH_NETIF_H
#define LWIP_HDR_FH_NETIF_H

#include "lwip/opt.h"

#include "lwip/sys.h"
#include "lwip/netif.h"
#include "lwip/dhcp.h"
#include "lwip/autoip.h"
#include "lwip/priv/tcpip_priv.h"

extern void* fh_netif_alloc(void);
extern void  fh_netif_init(void *netif);
extern void  fh_netif_set_common_flags(void *netif);
extern void  fh_netif_set_special_flags(void *netif,  unsigned char flags);
extern void  fh_netif_set_priv(void *netif, void* priv);
extern void* fh_netif_get_priv(void *netif);
extern void  fh_netif_set_hwaddr(void *netif, unsigned char *hwaddr);
extern void  fh_netif_set_ifname(void *netif, char *ifname);
extern void  fh_netif_set_tx_callback(void *netif, netif_linkoutput_fn linkoutput);
extern void  fh_netif_get_ipv4_config(void *netif, uint32_t *ip, uint32_t *gw, uint32_t *nmask);
/* return 0 if not link up */
extern int   fh_netif_is_link_up(char *ifname);
extern int fh_netif_get_link_status(void *netif);
extern void fh_netif_set_igmp_mac_filter(void *netif, netif_igmp_mac_filter_fn function);
extern netif_input_fn fh_netif_get_input_fn(void *netif);
extern void fh_netif_set_mtu(void *netif, uint32_t mtu);
#endif /* LWIP_HDR_FH_NETIF_H */
