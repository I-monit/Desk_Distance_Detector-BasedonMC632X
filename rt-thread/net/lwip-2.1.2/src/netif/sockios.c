/**
 * @file
 * Sockets BSD-Like API module
 *
 * @defgroup socket Socket API
 * @ingroup sequential_api
 * BSD-style socket API.\n
 * Thread-safe, to be called from non-TCPIP threads only.\n
 * Can be activated by defining @ref LWIP_SOCKET to 1.\n
 * Header is in posix/sys/socket.h\b
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
 * Improved by Marc Boucher <marc@mbsi.ca> and David Haas <dhaas@alum.rpi.edu>
 *
 */
#include <rtthread.h>
#include "lwip/opt.h"

#if LWIP_SOCKET /* don't build if not configured for use in lwipopts.h */

#include "lwip/sockets.h"
#include "lwip/api.h"
#include "lwip/sys.h"
#include "lwip/igmp.h"
#include "lwip/inet.h"
#include "lwip/netif.h"
#include "lwip/stats.h"
#include "lwip/tcpip.h"
#include "lwip/ip_addr.h"
#include "lwip/tcp.h"
#include "lwip/raw.h"
#include "lwip/udp.h"
#include "lwip/memp.h"
#include "lwip/pbuf.h"
#include "lwip/priv/tcp_priv.h"
#include "lwip/priv/sockets_priv.h"
#include "lwip/priv/tcpip_priv.h"
#include "lwip/dhcp.h"
#include "lwip/netifapi.h"
#if LWIP_CHECKSUM_ON_COPY
#include "lwip/inet_chksum.h"
#endif
#if LWIP_DHCP
#include "lwip/dhcp.h"
#endif


#ifdef RT_USING_COMPONENTS_LINUX_ADAPTER
#include "netinet/tcp.h"
#include "net/if.h"
#include "linux/sockios.h"
#endif

#include <string.h>

/* If the netconn API is not required publicly, then we include the necessary
   files here to get the implementation */
#if !LWIP_NETCONN
#undef LWIP_NETCONN
#define LWIP_NETCONN 1
#include "api_msg.c"
#include "api_lib.c"
#include "netbuf.c"
#undef LWIP_NETCONN
#define LWIP_NETCONN 0
#endif

extern struct lwip_sock *lwip_tryget_socket(int fd);
extern void done_socket(struct lwip_sock *sock);
/** This is overridable for the rare case where more than 255 threads
 * select on the same socket...
 */

static struct netif *lwip_netif_find(char *name)
{
  struct netif *net_if;
  rt_enter_critical();
  net_if = netif_find(name);
  rt_exit_critical();
  return net_if;
}

int get_netif_gw(char *ifname, struct in_addr *addr)
{
  struct netif *net_if;
  net_if = lwip_netif_find(ifname);
  if( net_if == RT_NULL )     /* interface not found. */
  {
    return -RT_ERROR;
  }
  netifapi_netif_get_addr(net_if, NULL, NULL, (ip4_addr_t *)addr);
  return RT_EOK;
}

int set_netif_gw(char *ifname, struct in_addr *addr)
{
  struct netif *net_if;
  int link_status = 0;

  net_if = lwip_netif_find(ifname);
  if( net_if == RT_NULL )     /* interface not found. */
  {
    return -RT_ERROR;
  }

  link_status = netif_is_link_up(net_if);
  if (link_status)
    netifapi_netif_set_link_down(net_if);

  netifapi_netif_set_addr(net_if, ip_2_ip4(&net_if->ip_addr), ip_2_ip4(&net_if->netmask), (const ip4_addr_t *)addr);

  if (link_status)
    netifapi_netif_set_link_up(net_if);

  return RT_EOK;
}

int get_active_netif(char *name)
{
  netifapi_netif_get_active(name);
  return RT_EOK;
}

#if LWIP_SIOC_TCP_INFO
extern u16_t ipv4_get_ip_id(void);
extern void ipv4_set_ip_id(u16_t arg);

int lwip_tcp_info_ioctl(int s, long cmd, void *argp)
{
    int ret = 0;
    struct lwip_sock *sock = lwip_tryget_socket(s);

#if LWIP_TCPIP_CORE_LOCKING
    LOCK_TCPIP_CORE();
#else
    SYS_ARCH_DECL_PROTECT(lev);
    /* the proper thing to do here would be to get into the tcpip_thread,
        but locking should be OK as well since we only *read* some flags */
    SYS_ARCH_PROTECT(lev);
#endif
    if (!sock)
    {
        return -RT_EFULL;
    }
    switch (cmd)
    {
        case SIOCGIPID:
            *(u16_t*)argp = ipv4_get_ip_id();
            break;

        case SIOCSIPID:
            ipv4_set_ip_id(*(u16_t*)argp);
            break;

        case SIOCGTCPSEQ:
            if ((sock->conn != NULL) && (sock->conn->type == NETCONN_TCP))
            {
                struct tcp_pcb *tcp = sock->conn->pcb.tcp;

                if (tcp != NULL)
                {
                    *(u32_t*)argp = tcp->snd_nxt;
                    break;
                }
            }
            ret = -1;
            break;

        case SIOCSTCPSEQ:
            if ((sock->conn != NULL) && (sock->conn->type == NETCONN_TCP))
            {
                struct tcp_pcb *tcp = sock->conn->pcb.tcp;

                if (tcp != NULL)
                {
                    tcp->snd_nxt = *(u32_t*)argp;
                    break;
                }
            }
            ret = -1;
            break;


        case SIOCGTCPACK:
            if ((sock->conn != NULL) && (sock->conn->type == NETCONN_TCP))
            {
                struct tcp_pcb *tcp = sock->conn->pcb.tcp;

                if (tcp != NULL)
                {
                    *(u32_t*)argp = tcp->rcv_nxt;
                    break;
                }
            }
            ret = -1;
            break;

            case SIOCSTCPACK:
            if ((sock->conn != NULL) && (sock->conn->type == NETCONN_TCP))
            {
                struct tcp_pcb *tcp = sock->conn->pcb.tcp;

                if (tcp != NULL)
                {
                    tcp->rcv_nxt = *(u32_t*)argp;
                    break;
                }
            }
            ret = -1;
            break;

        case SIOCGTCPWIN:
            if ((sock->conn != NULL) && (sock->conn->type == NETCONN_TCP))
            {
                struct tcp_pcb *tcp = sock->conn->pcb.tcp;

                if (tcp != NULL)
                {
                    *(u16_t*)argp = tcp->snd_wnd;
                    break;
                }
            }
            ret = -1;
            break;

        case SIOCSTCPWIN:
            if ((sock->conn != NULL) && (sock->conn->type == NETCONN_TCP))
            {
                struct tcp_pcb *tcp = sock->conn->pcb.tcp;

                if (tcp != NULL)
                {
                    tcp->snd_wnd = *(u16_t*)argp;
                    break;
                }
            }
            ret = -1;
            break;
        default:
            ret = -1;
            break;
    } /* switch (cmd) */

#if LWIP_TCPIP_CORE_LOCKING
    UNLOCK_TCPIP_CORE();
#else
    SYS_ARCH_UNPROTECT(lev);
#endif
    done_socket(sock);
    return ret;
}
#endif /* LWIP_SIOC_TCP_INFO */
/*
    extend lwip_ioctl( s, .... )
*/
extern void mac_set_hwaddr(char *netif_name, u8_t *mac);
#include "ethernetif.h"

int lwip_sock_ioctl(int s, long cmd, void *argp)
{
  struct netif *net_if;
  struct ifreq *net_req = (struct ifreq *)argp;
  char *szifname = net_req->ifr_name;
  int link_status = 0;

  switch (cmd) {
    case (long)SIOCGIFADDR:        /* 0x8915 */
      if( argp == RT_NULL )   return -RT_ERROR;
      net_if = lwip_netif_find(szifname);
      if( net_if == RT_NULL )     /* interface not found. */
      {
        return -RT_ERROR;
      }
      /* copy the address */
      net_req->ifr_addr.sa_family = AF_INET;
      struct sockaddr_in *get_addr = (struct sockaddr_in*)&(net_req->ifr_addr);
      netifapi_netif_get_addr(net_if, (ip4_addr_t *)&(get_addr->sin_addr), NULL, NULL);
      if (get_addr->sin_addr.s_addr == IPADDR_ANY)
      {
        return -RT_ERROR;
      }
      return RT_EOK;
    case (long)SIOCSIFADDR:        /* 0x8916 */
      if( argp == RT_NULL )   return -RT_ERROR;
      net_if = lwip_netif_find(szifname);
      if( net_if == RT_NULL )     /* interface not found. */
      {
        return -RT_ERROR;
      }
      /* set the address */

      link_status = netif_is_link_up(net_if);
      if (link_status)
        netifapi_netif_set_link_down(net_if);

      const struct sockaddr_in *addr = (const struct sockaddr_in*)(const void*)&(net_req->ifr_addr);
      netifapi_netif_set_addr(net_if, (const ip4_addr_t *)&(addr->sin_addr), ip_2_ip4(&net_if->netmask), ip_2_ip4(&net_if->gw));

      if (link_status)
        netifapi_netif_set_link_up(net_if);

      return RT_EOK;
    case (long)SIOCGIFBRDADDR:        /* 0x8919 */
      if( argp == RT_NULL )   return -RT_ERROR;
      net_if = lwip_netif_find(szifname);
      if( net_if == RT_NULL )     /* interface not found. */
      {
        return -RT_ERROR;
      }

      struct sockaddr_in *get_broadaddr = (struct sockaddr_in*)&(net_req->ifr_addr);
      netifapi_netif_get_broadaddr(net_if, (ip4_addr_t *)&(get_broadaddr->sin_addr), NULL, NULL);

      if (get_broadaddr->sin_addr.s_addr == IPADDR_ANY)
      {
        return -RT_ERROR;
      }

      return RT_EOK;

    case (long)SIOCGIFNETMASK:        /* 0x891b */
      if( argp == RT_NULL )   return -RT_ERROR;
      net_if = lwip_netif_find(szifname);
      if( net_if == RT_NULL )     /* interface not found. */
      {
        return -RT_ERROR;
      }
      net_req->ifr_addr.sa_family = AF_INET;
      struct sockaddr_in *get_netmask = (struct sockaddr_in*)&(net_req->ifr_addr);
      netifapi_netif_get_addr(net_if, NULL, (ip4_addr_t *)&(get_netmask->sin_addr), NULL);
      return RT_EOK;
    case (long)SIOCSIFNETMASK:        /* 0x891c */
      if( argp == RT_NULL )   return -RT_ERROR;
      net_if = lwip_netif_find(szifname);
      if( net_if == RT_NULL )     /* interface not found. */
      {
        return -RT_ERROR;
      }
      /* set the netmask */

      link_status = netif_is_link_up(net_if);
      if (link_status)
        netifapi_netif_set_link_down(net_if);

      const struct sockaddr_in *netmask = (const struct sockaddr_in*)(const void*)&(net_req->ifr_addr);
      netifapi_netif_set_addr(net_if, ip_2_ip4(&net_if->ip_addr), (const ip4_addr_t *)&(netmask->sin_addr), ip_2_ip4(&net_if->gw));

      if (link_status)
        netifapi_netif_set_link_up(net_if);

      return RT_EOK;
    case (long)SIOCGIFMTU:        /* 0x8921 */
      if( argp == RT_NULL )   return -RT_ERROR;
      netifapi_netif_get_mtu(szifname, &net_req->ifr_mtu);
      return RT_EOK;
    case (long)SIOCSIFMTU:        /* 0x8922 */
      if( argp == RT_NULL )   return -RT_ERROR;
      netifapi_netif_set_mtu(szifname, net_req->ifr_mtu);
      return RT_EOK;
    case (long)SIOCSIFHWADDR:        /* 0x8924 */
      if( argp == RT_NULL )   return -RT_ERROR;
      netifapi_netif_set_mac_addr(szifname, (u8_t *)&(net_req->ifr_hwaddr.sa_data));
      mac_set_hwaddr(szifname, (u8_t *)&(net_req->ifr_hwaddr.sa_data));
      return RT_EOK;
    case (long)SIOCGIFHWADDR:        /* 0x8927 */
      if( argp == RT_NULL )   return -RT_ERROR;
      netifapi_netif_get_mac_addr(szifname, (u8_t *)&(net_req->ifr_hwaddr.sa_data));
      return RT_EOK;
    case (long)TIOCOUTQ:           /* 0x5411 */
      if( argp == RT_NULL )	return -RT_ERROR;
      int *len = (int *)argp;
      *len = 0;
      struct lwip_sock *sock;
      struct tcp_seg *seg, *usag;

      sock = lwip_tryget_socket(s);
#if LWIP_TCPIP_CORE_LOCKING
      LOCK_TCPIP_CORE();
#else
      SYS_ARCH_DECL_PROTECT(lev);
      /* the proper thing to do here would be to get into the tcpip_thread,
          but locking should be OK as well since we only *read* some flags */
      SYS_ARCH_PROTECT(lev);
#endif
      if(!sock)
      {
#if LWIP_TCPIP_CORE_LOCKING
        UNLOCK_TCPIP_CORE();
#else
        SYS_ARCH_UNPROTECT(lev);
#endif
        return -RT_ERROR;
      }
      if (NETCONNTYPE_GROUP(netconn_type(sock->conn)) != NETCONN_TCP)
      {
#if LWIP_TCPIP_CORE_LOCKING
        UNLOCK_TCPIP_CORE();
#else
        SYS_ARCH_UNPROTECT(lev);
#endif
        done_socket(sock);
        return -RT_ERROR;
      }
      else
      {
        if (sock->conn && sock->conn->pcb.tcp)
        {
          seg = sock->conn->pcb.tcp->unsent;
          for(; seg != RT_NULL; seg = seg->next)
          {
            *len += seg->len;
          }
          usag = sock->conn->pcb.tcp->unacked;
          for(; usag != RT_NULL; usag = usag->next)
          {
            *len += usag->len;
          }
        }
      }
#if LWIP_TCPIP_CORE_LOCKING
      UNLOCK_TCPIP_CORE();
#else
      SYS_ARCH_UNPROTECT(lev);
#endif
      done_socket(sock);
      return RT_EOK;
    case (long)SIOCGIFCONF:        /* 0x8912 */
      if( argp == RT_NULL )	return -RT_ERROR;
      struct ifconf *net_conf = (struct ifconf *)argp;
      netifapi_netif_get_netif_info(net_conf);
      return RT_EOK;
    case (long)SIOCGIFFLAGS:        /* 0x8913 */
      if (net_req == RT_NULL)
        return -RT_ERROR;
      if (netdev_get_flags(net_req->ifr_name, (rt_uint16_t *)&(net_req->ifr_flags)))
      {
        return -RT_ERROR;
      }
      return RT_EOK;
    case (long)SIOCSIFFLAGS:        /* 0x8914 */
      if (net_req == RT_NULL)
        return -RT_ERROR;
      if (netdev_change_flags(net_req->ifr_name, (rt_uint16_t)(net_req->ifr_flags)))
      {
        return -RT_ERROR;
      }
      return RT_EOK;
    case (long)SIOCGIFINDEX:        /* 0x8933 */
      if( argp == RT_NULL )	return -RT_ERROR;
      u8_t index;
      netifapi_netif_name_to_index(szifname, &index);
      net_req->ifr_metric = index;
      return RT_EOK;
    case (long)SIOCADDRT:        /* 0x890b */
      return RT_EOK;
    case (long)SIOCDELRT:        /* 0x890c */
      return RT_EOK;
    default:
    {
      if (cmd >= SIOCDEVPRIVATE && cmd <= (SIOCDEVPRIVATE + 0xf))
          return netdev_do_ioctl(szifname, net_req, cmd);
#if LWIP_SIOC_TCP_INFO
      return lwip_tcp_info_ioctl(s, cmd, argp);
#endif /* LWIP_SIOC_TCP_INFO */
    }
      break;
  } /* switch (cmd) */
  LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_ioctl(%d, UNIMPL: 0x%lx, %p)\n", s, cmd, argp));
  /* sock_set_errno(sock, ENOSYS); *//* not yet implemented */
  return -1;
}

/*
    extend setsockopt( s, IPPROTO_TCP, ... );
*/
u8_t tcp_setsockopt_ext( int s, int optname, const void *optval, socklen_t optlen )
{
  u8_t err;
  struct lwip_sock *sock = lwip_tryget_socket(s);
  switch(optname)
  {
  case TCP_MAXSEG:
    sock->conn->pcb.tcp->mss = (u32_t)(*(const int*)optval);
    err = RT_EOK;
    LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_setsockopt(%d, IPPROTO_TCP, TCP_MAXSEG) -> %"U32_F"\n",
                s, sock->conn->pcb.tcp->mss));
    break;
  default:
    LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_setsockopt(%d, IPPROTO_TCP, UNIMPL: optname=0x%x, ..)\n",
                s, optname));
    err = ENOPROTOOPT;
    break;
  }

  done_socket(sock);
  return err;
}

/*
    extend getsockopt(s, IPPROTO_TCP, ... )
*/
u8_t tcp_getsockopt_ext( int s, int optname, void *optval, socklen_t *optlen )
{
  u8_t err = RT_EOK;
  struct lwip_sock *sock = lwip_tryget_socket(s);

  switch( optname )
  {
  case TCP_MAXSEG:
    *(int*)optval = (int)(sock->conn->pcb.tcp->mss);
    err = RT_EOK;
    LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_getsockopt(%d, IPPROTO_TCP, TCP_KEEPCNT) = %d\n",
                s, *(int *)optval));
    break;

  case TCP_INFO:
  {
    struct tcp_info *tinf = (struct tcp_info *)optval;

    if( tinf == RT_NULL || *optlen < sizeof(struct tcp_info) )
    {
        /* invalid parameter */
    }
    rt_memset( optval, 0, sizeof(struct tcp_info) );
    /* tcp->sa has the srtt * 8 val, and units in TCP_TMR_INTERVAL * 2 */
    tinf->tcpi_rtt = sock->conn->pcb.tcp->sa * TCP_TMR_INTERVAL * 250;
    *optlen = sizeof(struct tcp_info);
    err = RT_EOK;
    break;
  }
  default:
    LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_getsockopt(%d, IPPROTO_TCP, UNIMPL: optname=0x%x, ..)\n",
                s, optname));
    err = ENOPROTOOPT;
  }

  done_socket(sock);
  return err;
}

#endif /* LWIP_SOCKET */
