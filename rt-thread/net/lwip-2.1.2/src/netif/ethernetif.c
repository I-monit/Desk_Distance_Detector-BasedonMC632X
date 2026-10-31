/*
 * File      : ethernetif.c
 * This file is part of RT-Thread RTOS
 * COPYRIGHT (C) 2006 - 2010, RT-Thread Development Team
 *
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution or at
 * http://www.rt-thread.org/license/LICENSE
 *
 * Change Logs:
 * Date           Author       Notes
 * 2010-07-07     Bernard      fix send mail to mailbox issue.
 * 2011-07-30     mbbill       port lwIP 1.4.0 to RT-Thread
 * 2012-04-10     Bernard      add more compatible with RT-Thread.
 * 2012-11-12     Bernard      The network interface can be initialized
 *                             after lwIP initialization.
 * 2013-02-28     aozima       fixed list_tcps bug: ipaddr_ntoa isn't reentrant.
 * 2016-08-18     Bernard      port to lwIP 2.0.0
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

/*
 * This file is a skeleton for developing Ethernet network interface
 * drivers for lwIP. Add code to the low_level functions and do a
 * search-and-replace for the word "ethernetif" to replace it with
 * something that better describes your network interface.
 */

#include "lwip/opt.h"

#include "lwip/opt.h"
#include "lwip/debug.h"
#include "lwip/def.h"
#include "lwip/mem.h"
#include "lwip/pbuf.h"
#include "lwip/sys.h"
#include "lwip/netif.h"
#include "lwip/stats.h"
#include "lwip/tcpip.h"
#include "lwip/dhcp.h"
#include "lwip/netifapi.h"
#include "lwip/ip_addr.h"
#include "lwip/inet.h"

#if LWIP_IPV6
#include "lwip/ethip6.h"
#include "lwip/mld6.h"
#endif

#include "netif/etharp.h"
#include "netif/ethernetif.h"
#include "rtdevice.h"

#define netifapi_netif_set_link_up(n)      netifapi_netif_common(n, netif_set_link_up, NULL)
#define netifapi_netif_set_link_down(n)    netifapi_netif_common(n, netif_set_link_down, NULL)

#ifndef RT_LWIP_ETHTHREAD_PRIORITY
#define RT_ETHERNETIF_THREAD_PREORITY   0x90
#else
#define RT_ETHERNETIF_THREAD_PREORITY   RT_LWIP_ETHTHREAD_PRIORITY
#endif

#ifndef LWIP_NO_TX_THREAD
/**
 * Tx message structure for Ethernet interface
 */
struct eth_tx_msg
{
  struct netif    *netif;
  struct pbuf     *buf;
  struct rt_completion complete;
};

static struct rt_mailbox eth_tx_thread_mb;
static struct rt_thread eth_tx_thread;
#ifndef RT_LWIP_ETHTHREAD_MBOX_SIZE
static char eth_tx_thread_mb_pool[32 * 4];
static char eth_tx_thread_stack[512];
#else
static char eth_tx_thread_mb_pool[RT_LWIP_ETHTHREAD_MBOX_SIZE * 4];
static char eth_tx_thread_stack[RT_LWIP_ETHTHREAD_STACKSIZE];
#endif
#endif

#ifndef LWIP_NO_RX_THREAD
static struct rt_mailbox eth_rx_thread_mb;
static struct rt_thread eth_rx_thread;
#ifndef RT_LWIP_ETHTHREAD_MBOX_SIZE
static char eth_rx_thread_mb_pool[48 * 4];
static char eth_rx_thread_stack[1024];
#else
static char eth_rx_thread_mb_pool[RT_LWIP_ETHTHREAD_MBOX_SIZE * 4];
static char eth_rx_thread_stack[RT_LWIP_ETHTHREAD_STACKSIZE];
#endif
#endif

static err_t ethernetif_linkoutput(struct netif *netif, struct pbuf *p)
{
#ifndef LWIP_NO_TX_THREAD
  struct eth_tx_msg msg;

  RT_ASSERT(netif != RT_NULL);
  /* send a message to eth tx thread */
  msg.netif = netif;
  msg.buf   = p;
  rt_completion_init(&msg.complete);
  if (rt_mb_send(&eth_tx_thread_mb, (rt_uint32_t) &msg) == RT_EOK)
  {
    /* waiting for ack */
    rt_completion_wait(&msg.complete, RT_WAITING_FOREVER);
  }
#else
  struct net_device* enetif;

  RT_ASSERT(netif != RT_NULL);
  enetif = (struct net_device*)netif->state;

  if (enetif->net.eth.eth_tx(&(enetif->parent), p) != RT_EOK)
  {
    return ERR_IF;
  }
#endif
  return ERR_OK;
}

static err_t net_netif_device_init(struct netif *netif)
{
  struct net_device *ethif;

  ethif = (struct net_device*)netif->state;
  if (ethif != RT_NULL)
  {
    rt_device_t device;

    /* get device object */
    device = (rt_device_t) ethif;
    if (rt_device_init(device) != RT_EOK)
    {
      return ERR_IF;
    }

    /* copy device flags to netif flags */
    netif_set_flags(netif, (ethif->init_flags & 0xff));

#if LWIP_IPV6
        netif->output_ip6 = ethip6_output;
#if LWIP_IPV6_AUTOCONFIG
        netif->ip6_autoconfig_enabled = 1;
#endif
        netif_create_ip6_linklocal_address(netif, 1);

#if LWIP_IPV6_MLD
        netif_set_flags(netif, NETIF_FLAG_MLD6);

        /*
        * For hardware/netifs that implement MAC filtering.
        * All-nodes link-local is handled by default, so we must let the hardware know
        * to allow multicast packets in.
        * Should set mld_mac_filter previously. */
        if (netif->mld_mac_filter != NULL)
        {
            ip6_addr_t ip6_allnodes_ll;
            ip6_addr_set_allnodes_linklocal(&ip6_allnodes_ll);
            netif->mld_mac_filter(netif, &ip6_allnodes_ll, NETIF_ADD_MAC_FILTER);
        }
#endif /* LWIP_IPV6_MLD */

#endif /* LWIP_IPV6 */

        /* set default netif */
        if (netif_default == RT_NULL)
            netif_set_default(ethif->netif);

    netif_set_up(ethif->netif);

#if 0
#if LWIP_DHCP
    /* set interface up */
    netif_set_up(ethif->netif);
    /* if this interface uses DHCP, start the DHCP client */
    dhcp_start(ethif->netif);
#else
    /* set interface up */
    netif_set_up(ethif->netif);
#endif
#endif

#if 0
    if (!(ethif->flags & ETHIF_LINK_PHYUP))
    {
      /* set link_up for this netif */
      netif_set_link_up(ethif->netif);
    }
#endif

    return ERR_OK;
  }

  return ERR_IF;
}

err_t net_device_br_init(struct net_device *dev, struct netif *netif)
{
    rt_device_t device;
    char br_name[3];

    if ((!dev) || (!netif))
    {
      rt_kprintf("[%s-%d]: dev is NULL or netif is NULL!!!\n");
      return ERR_VAL;
    }

    rt_memset(br_name, 0, sizeof(br_name));

    dev->netif = netif;
    dev->type = NET_DEVICE_BR;
    dev->parent.type = RT_Device_Class_NetIf;

    rt_memcpy(br_name, netif->name, 2);

    rt_device_register(&(dev->parent), br_name, RT_DEVICE_FLAG_RDWR);

    /* get device object */
    device = (rt_device_t) dev;
    if (rt_device_init(device) != RT_EOK)
    {
      return ERR_IF;
    }

    return ERR_OK;
}

rt_err_t set_net_device_flag(struct net_device *dev, rt_uint8_t netif_flags)
{
  if (netif_flags & NETIF_FLAG_UP)
  {
    dev->flags |= IFF_UP;
  }
  else
  {
    dev->flags &= ~IFF_UP;
  }
  if (netif_flags & NETIF_FLAG_LINK_UP)
  {
    dev->flags |= IFF_RUNNING;
  }
  else
  {
    dev->flags &= ~IFF_RUNNING;
  }
  if (netif_flags & NETIF_FLAG_BROADCAST)
  {
    dev->flags |= IFF_BROADCAST;
  }
  else
  {
    dev->flags &= ~IFF_BROADCAST;
  }
  if (netif_flags & NETIF_FLAG_ETHARP)
  {
    dev->flags &= ~IFF_NOARP;
  }
  else
  {
    dev->flags |= IFF_NOARP;
  }

  return RT_EOK;
}

static unsigned int lwip_init_finished = 0;

static void net_wait_lwip_init(void)
{
  while (!lwip_init_finished)
    rt_thread_delay(1);
}
/* Keep old drivers compatible in RT-Thread */
rt_err_t net_device_init(struct net_device *dev, char *name, rt_uint8_t flag, rt_uint8_t type)
{
  struct netif* netif;

  net_wait_lwip_init();
  netif = (struct netif*) rt_malloc (sizeof(struct netif));
  if (netif == RT_NULL)
  {
    rt_kprintf("malloc netif failed\n");
    return -RT_ERROR;
  }
  rt_memset(netif, 0, sizeof(struct netif));
  /* set netif */
  dev->netif = netif;
  /* device flags, which will be set to netif flags when initializing */
  dev->init_flags = flag & 0xff;
  dev->type = type;
  if (type != NET_DEVICE_WLAN)
  {
      /* link changed status of device */
      dev->net.eth.link_changed = 0x00;
  }
  dev->parent.type = RT_Device_Class_NetIf;
  /* register to RT-Thread device manager */
  rt_device_register(&(dev->parent), name, RT_DEVICE_FLAG_RDWR);
  /* set name */
  netif->name[0] = name[0];
  netif->name[1] = name[1];

  /* set hw address to 6 */
  netif->hwaddr_len   = 6;

  /* get hardware MAC address */
  rt_device_control(&(dev->parent), NIOCTL_GADDR, netif->hwaddr);
#if LWIP_NETIF_HOSTNAME
  /* Initialize interface hostname */
  netif->hostname = "rtthread";
#endif /* LWIP_NETIF_HOSTNAME */

    /* if tcp thread has been started up, we add this netif to the system */
    if (rt_thread_find("tcpip") != RT_NULL)
    {
        ip4_addr_t ipaddr, netmask, gw;

#if !LWIP_DHCP
        ipaddr.addr = inet_addr(RT_LWIP_IPADDR);
        gw.addr = inet_addr(RT_LWIP_GWADDR);
        netmask.addr = inet_addr(RT_LWIP_MSKADDR);
#else
        IP4_ADDR(&ipaddr, 0, 0, 0, 0);
        IP4_ADDR(&gw, 0, 0, 0, 0);
        IP4_ADDR(&netmask, 0, 0, 0, 0);
#endif
        netifapi_netif_add(netif, &ipaddr, &netmask, &gw, dev, net_netif_device_init, tcpip_input);
        /* set output */
        netif->output       = etharp_output;
#if LWIP_IPV6
        netif->output_ip6   = ethip6_output;
#endif /* LWIP_IPV6 */
        if (type == NET_DEVICE_WLAN)
        {
            netif->linkoutput = dev->net.wlan.linkoutput;
        } else
        {
            netif->linkoutput = ethernetif_linkoutput;
        }
        netif->mtu = ETHERNET_MTU;
#if LWIP_IPV6
        netif->mtu6 = netif->mtu;
#endif /* LWIP_IPV6 */
    }

  return RT_EOK;
}

void net_device_stop(char *name)
{
  struct netif *net_if;
  rt_uint32_t level;

  if (name == NULL)
    return;

  level = rt_hw_interrupt_disable();

  net_if = netif_find(name);
  if (net_if)
    netif_clear_flags(net_if, NETIF_FLAG_LINK_UP);
  rt_hw_interrupt_enable(level);
}


void net_device_start(char *name)
{
  struct netif *net_if;
  rt_uint32_t level;

  if (name == NULL)
    return;

  level = rt_hw_interrupt_disable();

  net_if = netif_find(name);
  if (net_if)
    netif_set_flags(net_if, NETIF_FLAG_LINK_UP);
  rt_hw_interrupt_enable(level);
}

#if 0
rt_err_t net_device_init(struct net_device * dev, char *name, rt_uint8_t type)
{
  rt_uint8_t flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP;

#if LWIP_IGMP
  /* IGMP support */
  flags |= NETIF_FLAG_IGMP;
#endif

  dev->type = type;

  return net_device_init_with_flag(dev, name, flags, type);
}
#endif

#ifndef LWIP_NO_RX_THREAD
rt_err_t net_device_ready(struct net_device* dev)
{
  if (dev->netif)
    /* post message to Ethernet thread */
    return rt_mb_send(&eth_rx_thread_mb, (rt_uint32_t)dev);
  else
    return ERR_OK; /* netif is not initialized yet, just return. */
}

rt_err_t net_device_linkchange(struct net_device* dev, rt_bool_t up)
{
  rt_uint32_t level;

  RT_ASSERT(dev != RT_NULL);

  level = rt_hw_interrupt_disable();
  dev->net.eth.link_changed = 0x01;
  if (up == RT_TRUE)
    dev->net.eth.link_status = 0x01;
  else
    dev->net.eth.link_status = 0x00;
  rt_hw_interrupt_enable(level);

  /* post message to ethernet thread */
  return rt_mb_send(&eth_rx_thread_mb, (rt_uint32_t)dev);
}
#else
/* NOTE: please not use it in interrupt when no RxThread exist */
rt_err_t net_device_linkchange(struct net_device* dev, rt_bool_t up)
{
  if (up == RT_TRUE)
    netifapi_netif_set_link_up(dev->netif);
  else
    netifapi_netif_set_link_down(dev->netif);

  return RT_EOK;
}
#endif

#ifndef LWIP_NO_TX_THREAD
/* Ethernet Tx Thread */
static void eth_tx_thread_entry(void* parameter)
{
  struct eth_tx_msg* msg;

  while (1)
  {
    if (rt_mb_recv(&eth_tx_thread_mb, (rt_uint32_t*)&msg, RT_WAITING_FOREVER) == RT_EOK)
    {
      struct net_device* enetif;

      RT_ASSERT(msg->netif != RT_NULL);
      RT_ASSERT(msg->buf   != RT_NULL);

      enetif = (struct net_device*)msg->netif->state;
      if (enetif != RT_NULL)
      {
        /* call driver's interface */
        if (enetif->net.eth.eth_tx(&(enetif->parent), msg->buf) != RT_EOK)
        {
          /* transmit eth packet failed */
        }
      }

      /* send ACK */
      rt_completion_done(&(msg->complete));
    }
  }
}
#endif

#ifndef LWIP_NO_RX_THREAD
/* Ethernet Rx Thread */
static void eth_rx_thread_entry(void* parameter)
{
  struct net_device* device;

  while (1)
  {
    if (rt_mb_recv(&eth_rx_thread_mb, (rt_uint32_t*)&device, RT_WAITING_FOREVER) == RT_EOK)
    {
      struct pbuf *p;

      /* check link status */
      if (device->net.eth.link_changed)
      {
        int status;
        rt_uint32_t level;

        level = rt_hw_interrupt_disable();
        status = device->net.eth.link_status;
        device->net.eth.link_changed = 0x00;
        rt_hw_interrupt_enable(level);

        if (status)
          netifapi_netif_set_link_up(device->netif);
        else
          netifapi_netif_set_link_down(device->netif);
      }

      /* receive all of buffer */
      while (1)
      {
        if(device->net.eth.eth_rx == RT_NULL) break;

        p = device->net.eth.eth_rx(&(device->parent));
        if (p != RT_NULL)
        {
          /* notify to upper layer */
          if( device->netif->input(p, device->netif) != ERR_OK )
          {
            LWIP_DEBUGF(NETIF_DEBUG, ("ethernetif_input: Input error\n"));
            pbuf_free(p);
            p = NULL;
          }
        }
        else break;
      }
    }
    else
    {
      LWIP_ASSERT("Should not happen!\n",0);
    }
  }
}
#endif

int eth_system_device_init(void)
{
    lwip_init_finished = 1;
    return 0;
}

int eth_system_device_init_private(void)
{
    rt_err_t result = RT_EOK;

  /* initialize Rx thread. */
#ifndef LWIP_NO_RX_THREAD
  /* initialize mailbox and create Ethernet Rx thread */
  result = rt_mb_init(&eth_rx_thread_mb, "erxmb",
                      &eth_rx_thread_mb_pool[0], sizeof(eth_rx_thread_mb_pool)/4,
                      RT_IPC_FLAG_FIFO);
  RT_ASSERT(result == RT_EOK);

  result = rt_thread_init(&eth_rx_thread, "erx", eth_rx_thread_entry, RT_NULL,
                          &eth_rx_thread_stack[0], sizeof(eth_rx_thread_stack),
                          RT_ETHERNETIF_THREAD_PREORITY, 16);
  RT_ASSERT(result == RT_EOK);
  result = rt_thread_startup(&eth_rx_thread);
  RT_ASSERT(result == RT_EOK);
#endif

  /* initialize Tx thread */
#ifndef LWIP_NO_TX_THREAD
  /* initialize mailbox and create Ethernet Tx thread */
  result = rt_mb_init(&eth_tx_thread_mb, "etxmb",
                      &eth_tx_thread_mb_pool[0], sizeof(eth_tx_thread_mb_pool)/4,
                      RT_IPC_FLAG_FIFO);
  RT_ASSERT(result == RT_EOK);

  result = rt_thread_init(&eth_tx_thread, "etx", eth_tx_thread_entry, RT_NULL,
                          &eth_tx_thread_stack[0], sizeof(eth_tx_thread_stack),
                          RT_ETHERNETIF_THREAD_PREORITY, 16);
  RT_ASSERT(result == RT_EOK);

  result = rt_thread_startup(&eth_tx_thread);
  RT_ASSERT(result == RT_EOK);
#endif

  return (int)result;
}
/* INIT_DEVICE_EXPORT(eth_system_device_init); */

#if 1
#ifdef RT_USING_ETHTOOL

static int ethtool_get_settings(struct net_device *dev, void *useraddr)
{
  struct ethtool_cmd cmd = { .cmd = ETHTOOL_GSET };
  int err;

  if (!dev->net.eth.ethtool_ops->get_settings)
    return -RT_ENOSYS;

  err = dev->net.eth.ethtool_ops->get_settings(dev, &cmd);
  if (err < 0)
    return err;

  rt_memcpy(useraddr, &cmd, sizeof(cmd));

  return 0;
}

static int ethtool_set_settings(struct net_device *dev, void *useraddr)
{
  struct ethtool_cmd cmd;

  if (!dev->net.eth.ethtool_ops->set_settings)
    return -RT_ENOSYS;

  rt_memcpy(&cmd, useraddr, sizeof(cmd));

  return dev->net.eth.ethtool_ops->set_settings(dev, &cmd);
}

static int ethtool_get_link(struct net_device *dev, void *useraddr)
{
  struct ethtool_value *edata = useraddr;

  if (!dev->net.eth.ethtool_ops->get_link)
    return -RT_ENOSYS;

  edata->data = (dev->netif->flags & NETIF_FLAG_UP) && dev->net.eth.ethtool_ops->get_link(dev);

  return 0;
}

int fh_ethtool(int c, struct ifreq *ifr)
{
  struct net_device *ethtool_dev;
  void *useraddr = ifr->ifr_data;
  unsigned int ethcmd;
  int rc;

  ethtool_dev = (struct net_device *)rt_device_find(ifr->ifr_name);

  if (!ethtool_dev) {
    rt_kprintf("ERROR: %s, cannot find device %s\n",
            __func__, ifr->ifr_name);
    return -RT_EIO;
  }

  rt_memcpy(&ethcmd, useraddr, sizeof(ethcmd));

  if (!ethtool_dev->net.eth.ethtool_ops) {
    return -RT_ENOSYS;
  }
  switch (ethcmd) {
  case ETHTOOL_GSET:
    rc = ethtool_get_settings(ethtool_dev, useraddr);
    break;
  case ETHTOOL_SSET:
    rc = ethtool_set_settings(ethtool_dev, useraddr);
    break;
  case ETHTOOL_GLINK:
    rc = ethtool_get_link(ethtool_dev, useraddr);
    break;
  default:
    rc = -RT_ENOSYS;
  }

  return rc;
}
#endif

err_t hwaddr_from_index( int index, unsigned char *macbuf )
{
  struct netif *net_if;

  net_if = netif_get_by_index(index);
  if (net_if == NULL)
  {
    return ERR_VAL;
  }

  rt_memcpy( macbuf, net_if->hwaddr, 6 );
  return ERR_OK;
}

extern int lwip_sock_ioctl(int s, long cmd, void *argp);
int eth_ioctl(int c, long cmd, void *argp)
{
  switch (cmd) {
#ifdef RT_USING_ETHTOOL
  case SIOCETHTOOL:
    return fh_ethtool(c, (struct ifreq *)argp);
#endif
  default:
    return lwip_sock_ioctl(c, cmd, argp);
  } /* switch (cmd) */
  return -1;
}

void netdev_get_netcard_flags(struct net_device *dev, rt_uint16_t iff_netcard)
{
    dev->flags = (dev->flags & 0xff) | (iff_netcard & 0xff00);
}

rt_uint16_t netdev_flags_from_neitf_flags(rt_uint16_t *flags, rt_uint8_t neitf_flags)
{
    rt_uint8_t flags_low8 = 0;

    if (neitf_flags & NETIF_FLAG_UP)
    {
        flags_low8 |= IFF_UP;
    }
    if (neitf_flags & NETIF_FLAG_BROADCAST)
    {
        flags_low8 |= IFF_BROADCAST;
    }
    if (neitf_flags & NETIF_FLAG_LINK_UP)
    {
        flags_low8 |= IFF_RUNNING;
    }
    if ((neitf_flags & NETIF_FLAG_ETHARP) == 0)
    {
        flags_low8 |= IFF_NOARP;
    }
    *flags &= 0xff00;
    *flags |= flags_low8;

    return *flags;
}

rt_uint8_t netdev_set_netif_change_flags(struct netif *netif, rt_uint16_t devflag)
{
  if (devflag & IFF_UP)
  {
    netifapi_netif_enable_devflag(netif, NETIF_FLAG_UP);
  } else
  {
    netifapi_netif_disable_devflag(netif, NETIF_FLAG_UP);
  }

  if (devflag & IFF_BROADCAST)
  {
    netifapi_netif_enable_devflag(netif, NETIF_FLAG_BROADCAST);
  } else
  {
    netifapi_netif_disable_devflag(netif, NETIF_FLAG_BROADCAST);
  }

  if (devflag & IFF_RUNNING)
  {
    netifapi_netif_enable_devflag(netif, NETIF_FLAG_LINK_UP);
  } else
  {
    netifapi_netif_disable_devflag(netif, NETIF_FLAG_LINK_UP);
  }

  if (devflag & IFF_NOARP)
  {
    netifapi_netif_disable_devflag(netif, NETIF_FLAG_ETHARP);
  } else
  {
    netifapi_netif_enable_devflag(netif, NETIF_FLAG_ETHARP);
  }

  return 0;
}

int netdev_get_flags(char *netif_name, rt_uint16_t *flags)
{
    rt_device_t rt_dev;

    memset(flags, 0, sizeof(rt_uint16_t));
    rt_dev = rt_device_find(netif_name);
    if (!rt_dev)
    {
        rt_kprintf("%s-%d rt_device_find(%s) fail\n", __func__, __LINE__, netif_name);
        return -1;
    }

    *flags = ((struct net_device *)rt_dev)->flags;
    return 0;
}

int netdev_change_flags(char *netif_name, rt_uint16_t flags)
{
    struct netif *netif;
    rt_uint16_t flags_devset, *dev_flags;
    rt_device_t rt_dev;

    rt_dev = rt_device_find(netif_name);
    if (!rt_dev)
    {
        rt_kprintf("%s-%d rt_device_find(%s) fail\n", __func__, __LINE__, netif_name);
        return -1;
    }
    dev_flags = &(((struct net_device *)rt_dev)->flags);
    netif = ((struct net_device *)rt_dev)->netif;
    if (!netif)
    {
        rt_kprintf("%s-%d (%s) netif==NULL fail\n", __func__, __LINE__, netif_name);
        return -2;
    }
    if (flags == *dev_flags)
        goto exit_;

    netdev_set_netif_change_flags(netif, flags);
    flags_devset = flags & 0xff00;
    if (flags_devset == ((*dev_flags) & 0xff00))
        goto exit_;

    if (rt_device_control(rt_dev, NIOCTL_SFLAGS, &flags_devset))
    {
        rt_kprintf("%s-%d rt_device_control(NIOCTL_SFLAGS, flags=0x%x) fail\n", __func__, __LINE__, flags);
        return -2;
    } else
    {
        *dev_flags = flags_devset | (flags & 0xff);
        if ((*dev_flags) & IFF_PROMISC)
        {
            rt_kprintf("%s netif(%s) entered promiscuous mode\n", __func__, netif_name);
        }
    }

exit_:
    return 0;
}

/* NIOCTL_DEVPRIV~(NIOCTL_DEVPRIV + 0xf) are for netdev_do_ioctl() */
int netdev_do_ioctl(char *netif_name, struct ifreq *ifr, long cmd)
{
    rt_device_t rt_dev;
    int ret = -1;
    unsigned offset = 0;

    rt_dev = rt_device_find(netif_name);
    if (!rt_dev)
    {
        rt_kprintf("%s-%d rt_device_find(%s) fail\n", __func__, __LINE__, netif_name);
        return -1;
    }
    if (cmd < SIOCDEVPRIVATE || cmd > (SIOCDEVPRIVATE + 0xf))
    {
        rt_kprintf("%s-%d cmd %d not supported!\n", __func__, __LINE__, cmd);
        return -2;
    }
    offset = cmd - SIOCDEVPRIVATE;

    ret = rt_device_control(rt_dev, NIOCTL_DEVPRIV + offset, (void *)ifr);
    if (ret)
    {
        rt_kprintf("%s-%d rt_device_control(NIOCTL_DEVPRIV) ret %d!\n", __func__, __LINE__, ret);
        return ret;
    }

    return ret;
}

void mac_set_hwaddr(char *netif_name, u8_t *mac)
{
  struct net_device *dev;

  dev = (struct net_device *)rt_device_find(netif_name);
  if (!dev)
  {
    return;
  }

  rt_device_control(&(dev->parent), NIOCTL_SADDR, mac);
}

extern err_t netifapi_netif_get_netif_infolog(struct ifconf *net_conf);
int get_netif_infolog()
{
  struct ifconf net_conf;

  return netifapi_netif_get_netif_infolog(&net_conf);
}

#endif
