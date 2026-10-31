/**
 * @file
 * lwIP network interface abstraction
 * 
 * @defgroup netif Network interface (NETIF)
 * @ingroup callbackstyle_api
 * 
 * @defgroup netif_ip4 IPv4 address handling
 * @ingroup netif
 * 
 * @defgroup netif_ip6 IPv6 address handling
 * @ingroup netif
 * 
 * @defgroup netif_cd Client data handling
 * Store data (void*) on a netif for application usage.
 * @see @ref LWIP_NUM_NETIF_CLIENT_DATA
 * @ingroup netif
 */


#include "lwip/opt.h"

#include <string.h>

#include "lwip/def.h"
#include "lwip/ip_addr.h"
#include "lwip/ip6_addr.h"
#include "lwip/netif.h"
#include "lwip/priv/tcp_priv.h"
#include "lwip/udp.h"
#include "lwip/raw.h"
#include "lwip/snmp.h"
#include "lwip/igmp.h"
#include "lwip/etharp.h"
#include "lwip/stats.h"
#include "lwip/sys.h"
#include "lwip/ip.h"
#include "lwip/inet.h"
#if ENABLE_LOOPBACK
#if LWIP_NETIF_LOOPBACK_MULTITHREADING
#include "lwip/tcpip.h"
#endif /* LWIP_NETIF_LOOPBACK_MULTITHREADING */
#endif /* ENABLE_LOOPBACK */

#include "netif/ethernet.h"

#if LWIP_AUTOIP
#include "lwip/autoip.h"
#endif /* LWIP_AUTOIP */
#if LWIP_DHCP
#include "lwip/dhcp.h"
#endif /* LWIP_DHCP */
#if LWIP_IPV6_DHCP6
#include "lwip/dhcp6.h"
#endif /* LWIP_IPV6_DHCP6 */
#if LWIP_IPV6_MLD
#include "lwip/mld6.h"
#endif /* LWIP_IPV6_MLD */
#if LWIP_IPV6
#include "lwip/nd6.h"
#include "lwip/ethip6.h"
#endif

void* fh_netif_alloc(void)
{
  return rt_calloc(1, sizeof(struct netif));
}

void fh_netif_init(void *netif)
{
  rt_memset(netif, 0, sizeof(struct netif));

#if LWIP_NETIF_HOSTNAME
  /* Initialize interface hostname */
  ((struct netif *)netif)->hostname = "lwip";
#endif/* LWIP_NETIF_HOSTNAME */

  ((struct netif *)netif)->mtu = 1500;
}

void fh_netif_set_common_flags(void *netif)
{
  rt_uint8_t flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_IGMP;

  netif_set_flags((struct netif *)netif, flags);
}

void fh_netif_set_special_flags(void *netif,  unsigned char flags)
{
  ((struct netif *)netif)->flags = flags;
  netif_set_flags((struct netif *)netif, flags);
}

void fh_netif_set_priv(void *netif, void* priv)
{
  ((struct netif *)netif)->state = priv;
}

void* fh_netif_get_priv(void *netif)
{
  return ((struct netif *)netif)->state;
}

void fh_netif_set_hwaddr(void *netif, unsigned char *hwaddr)
{
  rt_memcpy(((struct netif *)netif)->hwaddr, hwaddr, 6);
  ((struct netif *)netif)->hwaddr_len = 6;
}

void fh_netif_set_ifname(void *netif, char *ifname)
{
  ((struct netif *)netif)->name[0] = ifname[0];
  ((struct netif *)netif)->name[1] = ifname[1];
}

void fh_netif_set_tx_callback(void *netif, netif_linkoutput_fn linkoutput)
{
  ((struct netif *)netif)->output = etharp_output;

#if LWIP_IPV6
  ((struct netif *)netif)->output_ip6 = ethip6_output;
#endif

  ((struct netif *)netif)->linkoutput = linkoutput;
}

void fh_netif_get_ipv4_config(void *netif, uint32_t *ip, uint32_t *gw, uint32_t *nmask)
{
	if (ip)
		*ip = *((uint32_t *)&((struct netif *)netif)->ip_addr);

	if (gw)
		*gw = *((uint32_t *)&((struct netif *)netif)->gw);

	if (nmask)
		*nmask = *((uint32_t *)&((struct netif *)netif)->netmask);
}

int fh_netif_is_link_up(char *ifname)
{
  struct netif *net_if;
  rt_enter_critical();
  if(ifname == RT_NULL)
  {
    rt_exit_critical();
    return 0;
  }
  net_if = netif_list;
  while( net_if != RT_NULL )
  {
    if( rt_strncmp(ifname, net_if->name, 2) == 0 )
      break;
    net_if = net_if->next;
  }
  if( net_if == RT_NULL )
  {
    rt_exit_critical();
    return 0;
  }
  rt_exit_critical();

  return netif_is_link_up(net_if);
}

/* return 0 if not link up */
int fh_netif_get_link_status(void *netif)
{
    return ((((struct netif *)netif)->flags) & NETIF_FLAG_LINK_UP);
}

void fh_netif_set_igmp_mac_filter(void *netif, netif_igmp_mac_filter_fn function)
{
  if(netif != NULL)
  {
      ((struct netif *)netif)->igmp_mac_filter = function;
  }
}

netif_input_fn fh_netif_get_input_fn(void *netif)
{
  if (netif != NULL)
  {
    return ((struct netif *)netif)->input;
  }

  return RT_NULL;
}

/*bugfix: #3075 netif_add() set netif->mtu = 0 in LWIP212 */
void fh_netif_set_mtu(void *netif, uint32_t mtu)
{
  if(netif != NULL)
  {
    ((struct netif *)netif)->mtu = mtu;
  }
}
