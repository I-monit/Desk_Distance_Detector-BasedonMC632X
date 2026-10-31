#include <rtthread.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <finsh.h>

#ifdef RT_USING_LWIP

#ifdef RT_LWIP_DNS
#include <lwip/dns.h>
void get_dns(void)
{
  ip_addr_t ip_addr[DNS_MAX_SERVERS];
  int index = 0;


  rt_enter_critical();
  for(index = 0; index < DNS_MAX_SERVERS; index++)
  {
      memcpy(&(ip_addr[index]), dns_getserver(index), sizeof(ip_addr_t));
  }
  rt_exit_critical();

  for (index = 0; index < DNS_MAX_SERVERS; index++)
  {
    rt_kprintf("dns server #%d: %s\n", index, ipaddr_ntoa(&(ip_addr[index])));
  }
}
void set_dns(int index, char* dns_server)
{
  ip_addr_t addr;

  rt_enter_critical();
  if ((index < DNS_MAX_SERVERS)&&(index >= 0)&&(dns_server != RT_NULL) && ipaddr_aton(dns_server, &addr))
  {
    dns_setserver(index, &addr);
  }
  rt_exit_critical();
}
FINSH_FUNCTION_EXPORT(set_dns, set DNS server index(0 or 1) and address);

static void dns_usage(void)
{
  rt_kprintf("Usage:\n");
  rt_kprintf("  dns [<address1>] [<address2>]\n");
  rt_kprintf("  e.g: dns 192.168.0.6 192.168.0.254 \n");
}
int cmd_dns(int argc, char **argv)
{
    if (argc == 1)
    {
        get_dns();
    }
    else if (argc == 2)
    {
        rt_kprintf("dns : %s\n", argv[1]);
        if (strcmp(argv[1], "-h") == 0)
        {
          dns_usage();
        }
        else if (inet_addr(argv[1]) != IPADDR_NONE)
        {
          set_dns(0, argv[1]);
        }
        else
        {
          rt_kprintf("bad parameter! Wrong ipaddr!\n");
        }
    }
    else if (argc == 3)
    {
        rt_kprintf("dns : %s\n", argv[1]);
        rt_kprintf("dns : %s\n", argv[2]);
        if ((inet_addr(argv[1]) != IPADDR_NONE) && (inet_addr(argv[2]) != IPADDR_NONE))
        {
          set_dns(0, argv[1]);
          set_dns(1, argv[2]);
        }
        else
        {
          rt_kprintf("bad parameter! Wrong ipaddr!\n");
        }
    }
    else
    {
        dns_usage();
    }
    return 0;
}

#ifdef FINSH_USING_MSH
FINSH_FUNCTION_EXPORT_ALIAS(cmd_dns, __cmd_dns, list the information of dns);
#endif
#endif

#ifdef RT_LWIP_TCP
#include <lwip/tcp.h>
#include <lwip/priv/tcp_priv.h>

void list_tcps(void)
{
  rt_uint32_t num = 0;
  struct tcp_pcb *pcb;
  char local_ip_str[16];
  char remote_ip_str[16];

  extern struct tcp_pcb *tcp_active_pcbs;
  extern union tcp_listen_pcbs_t tcp_listen_pcbs;
  extern struct tcp_pcb *tcp_tw_pcbs;

  rt_enter_critical();
  rt_kprintf("Active PCB states:\n");
  for(pcb = tcp_active_pcbs; pcb != NULL; pcb = pcb->next)
  {
    strcpy(local_ip_str, ipaddr_ntoa(&(pcb->local_ip)));
    strcpy(remote_ip_str, ipaddr_ntoa(&(pcb->remote_ip)));

    rt_kprintf("#%d %s:%d <==> %s:%d snd_nxt 0x%08X rcv_nxt 0x%08X ",
                num++,
                local_ip_str,
                pcb->local_port,
                remote_ip_str,
                pcb->remote_port,
                pcb->snd_nxt,
                pcb->rcv_nxt);
    rt_kprintf("state: %s\n", tcp_debug_state_str(pcb->state));
  }

  rt_kprintf("Listen PCB states:\n");
  num = 0;
  for(pcb = (struct tcp_pcb *)tcp_listen_pcbs.pcbs; pcb != NULL; pcb = pcb->next)
  {
    rt_kprintf("#%d local port %d ", num++, pcb->local_port);
    rt_kprintf("state: %s\n", tcp_debug_state_str(pcb->state));
  }

  rt_kprintf("TIME-WAIT PCB states:\n");
  num = 0;
  for(pcb = tcp_tw_pcbs; pcb != NULL; pcb = pcb->next)
  {
    strcpy(local_ip_str, ipaddr_ntoa(&(pcb->local_ip)));
    strcpy(remote_ip_str, ipaddr_ntoa(&(pcb->remote_ip)));

    rt_kprintf("#%d %s:%d <==> %s:%d snd_nxt 0x%08X rcv_nxt 0x%08X ",
                num++,
                local_ip_str,
                pcb->local_port,
                remote_ip_str,
                pcb->remote_port,
                pcb->snd_nxt,
                pcb->rcv_nxt);
    rt_kprintf("state: %s\n", tcp_debug_state_str(pcb->state));
  }
  rt_exit_critical();
}
FINSH_FUNCTION_EXPORT(list_tcps, list all of tcp connections);
#endif

#ifdef RT_LWIP_UDP
#include "lwip/udp.h"
void list_udps(void)
{
    struct udp_pcb *pcb;
    rt_uint32_t num = 0;
    char local_ip_str[16];
    char remote_ip_str[16];

    rt_enter_critical();
    rt_kprintf("Active UDP PCB states:\n");
    for (pcb = udp_pcbs; pcb != NULL; pcb = pcb->next)
    {
        strcpy(local_ip_str, ipaddr_ntoa(&(pcb->local_ip)));
        strcpy(remote_ip_str, ipaddr_ntoa(&(pcb->remote_ip)));

        rt_kprintf("#%d %d %s:%d <==> %s:%d \n",
                   num, (int)pcb->flags,
                   local_ip_str,
                   pcb->local_port,
                   remote_ip_str,
                   pcb->remote_port);

        num++;
    }
    rt_exit_critical();
}
FINSH_FUNCTION_EXPORT(list_udps, list all of udp connections);
#endif /* LWIP_UDP */

int cmd_netstat(int argc, char **argv)
{
#ifdef RT_LWIP_TCP
    list_tcps();
#endif
#ifdef RT_LWIP_UDP
    list_udps();
#endif

    return 0;
}

#ifdef FINSH_USING_MSH
FINSH_FUNCTION_EXPORT_ALIAS(cmd_netstat, __cmd_netstat, list the information of TCP / IP);
#endif

extern struct ether_addr *ether_aton_r(const char *asc, struct ether_addr *addr);
void set_mac(char *netif_name, char *mac_add)
{
  struct ifreq net_req;
  struct sockaddr *set_hwaddr;
  int sock = -1;

  memset(&net_req, 0, sizeof(struct ifreq));
  memcpy(net_req.ifr_name, netif_name, 2);

  set_hwaddr = (struct sockaddr*)&net_req.ifr_hwaddr;
  if(ether_aton_r(mac_add, (struct ether_addr *)set_hwaddr->sa_data) == NULL)
  {
      rt_kprintf("Error: ether_aton_r failed. \n");
      goto SET_MAC_END;
  }

  sock = socket(AF_INET, SOCK_DGRAM, 0);
  if (sock < 0)
  {
    rt_kprintf("Error: Create socket failed. \n");
    goto SET_MAC_END;
  }

  if(ioctl(sock, SIOCSIFHWADDR, &net_req) < 0)
  {
      rt_kprintf("Error: ioctl SIOCSIFHWADDR failed\n");
      goto SET_MAC_END;
  }

SET_MAC_END:
  if (sock > 0)
  {
    close(sock);
  }
  return;
}

void set_netif_flag(char *netif_name, char *flag)
{
  struct ifreq net_req;
  int sock = -1;

  memset(&net_req, 0, sizeof(net_req));
  memcpy(net_req.ifr_name, netif_name, 2);

  sock = socket(AF_INET, SOCK_DGRAM, 0);
  if (sock < 0)
  {
      rt_kprintf("Error: create socket failed. \n");
      goto SET_FLAG_END;
  }

  if(ioctl(sock, SIOCGIFFLAGS, &net_req) < 0)
  {
    printf("Error: ioctl SIOCGIFFLAGS failed\n");
    goto SET_FLAG_END;
  }
  if (strncmp(strupr(flag), "UP", 2) == 0)
  {
     net_req.ifr_flags = net_req.ifr_flags | IFF_UP;
  }
  else if (strncmp(strupr(flag), "DOWN", 4) == 0)
  {
    net_req.ifr_flags = net_req.ifr_flags & (~IFF_UP);
  }
  else
  {
    rt_kprintf("network interface FLAG error.(only up/down)\n");
    goto SET_FLAG_END;
  }
  if(ioctl(sock, SIOCSIFFLAGS, &net_req) < 0)
  {
    printf("Error: ioctl SIOCSIFFLAGS failed\n");
    goto SET_FLAG_END;
  }

SET_FLAG_END:
  if (sock > 0)
  {
      close(sock);
  }
  return;
}

extern int set_netif_gw(char *ifname, struct in_addr *addr);
void set_if(char *netif_name, char *ip_addr, char *gw_addr, char *nm_addr)
{
  struct ifreq req_addr;
  struct ifreq req_gw;
  struct ifreq req_nm;
  struct sockaddr_in *set_addr;
  int sock = -1;

  memset(&req_addr, 0, sizeof(struct ifreq));
  memset(&req_gw, 0, sizeof(struct ifreq));
  memset(&req_nm, 0, sizeof(struct ifreq));

  set_addr = (struct sockaddr_in *)(void *)&(req_addr.ifr_addr);
  memcpy(req_addr.ifr_name, netif_name, 2);
  if (!inet_aton(ip_addr, &(set_addr->sin_addr)))
  {
    rt_kprintf("Error: inet_aton failed. \n");
    goto SET_ADDR_END;
  }

  set_addr = (struct sockaddr_in *)(void *)&(req_nm.ifr_addr);
  memcpy(req_nm.ifr_name, netif_name, 2);
  if (!inet_aton(nm_addr, &(set_addr->sin_addr)))
  {
    rt_kprintf("Error: inet_aton failed. \n");
    goto SET_ADDR_END;
  }

  set_addr = (struct sockaddr_in *)(void *)&(req_gw.ifr_addr);
  memcpy(req_gw.ifr_name, netif_name, 2);
  if (!inet_aton(gw_addr, &(set_addr->sin_addr)))
  {
    rt_kprintf("Error: inet_aton failed. \n");
    goto SET_ADDR_END;
  }

  sock = socket(AF_INET, SOCK_DGRAM, 0);
  if (sock < 0)
  {
    rt_kprintf("Error: Create socket failed. \n");
    goto SET_ADDR_END;
  }

  if (ioctl(sock, SIOCSIFADDR, &req_addr) < 0)
  {
      rt_kprintf("Error: ioctl SIOCSIFADDR failed\n");
      goto SET_ADDR_END;
  }

  if (ioctl(sock, SIOCSIFNETMASK, &req_nm) < 0)
  {
      rt_kprintf("Error: ioctl SIOCSIFNETMASK failed\n");
      goto SET_ADDR_END;;
  }

  if (set_netif_gw(req_gw.ifr_name, &(set_addr->sin_addr)) < 0)
  {
    rt_kprintf("Error: set_netif_gw failed\n");
    goto SET_ADDR_END;
  }
SET_ADDR_END:
  if (sock > 0)
  {
    close(sock);
  }
  return;

}

#if 0
#if LWIP_IPV6

void set_ip6_if(char* netif_name, char* ip_addr)
{
  ip_addr_t addr;
  ip_addr_t *ip;
  struct netif * netif = netif_list;

  if(strlen(netif_name) > sizeof(netif->name))
  {
    rt_kprintf("network interface name too long!\r\n");
    return;
  }

  while(netif != RT_NULL)
  {
    if(strncmp(netif_name, netif->name, sizeof(netif->name)) == 0)
      break;

    netif = netif->next;
    if( netif == RT_NULL )
    {
      rt_kprintf("network interface: %s not found!\r\n", netif_name);
      return;
    }
  }

  ip = (ip_addr_t *)&addr;
  ip_addr_t groupaddr;
  ip_addr_set_group6(&groupaddr, &netif->ip6_addr[0]);
  mld6_leavegroup(&netif->ip6_addr[0].u_addr.ip6, &groupaddr.u_addr.ip6);
  if ((ip_addr != RT_NULL) && ipaddr_aton(ip_addr, &addr))
  {
    ip6_addr_set(ip_2_ip6(&netif->ip6_addr[0]),&ip->u_addr.ip6);
  }
  ip_addr_set_group6(&groupaddr, &netif->ip6_addr[0]);
  mld6_joingroup(&netif->ip6_addr[0].u_addr.ip6, &groupaddr.u_addr.ip6);
}
FINSH_FUNCTION_EXPORT(set_ip6_if, set network interface address);

#endif
#endif

extern int get_netif_infolog();
void list_if(void)
{
  get_netif_infolog();
#ifdef RT_LWIP_DNS
  get_dns();
#endif /**< #if LWIP_DNS */
  return;

}
#ifdef RT_USING_FINSH
#include <finsh.h>
FINSH_FUNCTION_EXPORT(set_mac, set network mac address);
FINSH_FUNCTION_EXPORT(set_netif_flag, set network interface flag);
FINSH_FUNCTION_EXPORT(set_if, set network interface address);
FINSH_FUNCTION_EXPORT(list_if, list network interface information);
#endif

extern char *inet_ntoa_r(struct in_addr addr, char *buf, int buflen);
static void ifconfig_usage(void)
{
  rt_kprintf("Usage:\n");
  rt_kprintf("  ifconfig <interface> [<address>]\n");
  rt_kprintf("  [netmask <address>] [hw <ether> <address>]\n");
}
int cmd_ifconfig(int argc, char **argv)
{
    struct in_addr ip_addr;
    struct in_addr gw_addr;
    struct in_addr netmask;
    char ip[16];
    char gw[16];
    char nm[16];

    if (argc == 1)
    {
        list_if();
    }
    else if (argc == 2)
    {
        ifconfig_usage();
    }
    else if (argc == 3)
    {
      ip_addr.s_addr = inet_addr(argv[2]);
      if (ip_addr.s_addr == IPADDR_NONE)
      {
        rt_kprintf("bad parameter! Wrong ipaddr!\n");
        return -1;
      }
      if (ip_addr.s_addr == 0)
      {
        set_if(argv[1], "0.0.0.0", "0.0.0.0", "0.0.0.0");
        return 0;
      }
      if ((ip_addr.s_addr & 0xff) < 127)
      {
        netmask.s_addr = 0xff;
      }
      else if ((ip_addr.s_addr & 0xff) < 192)
      {
        netmask.s_addr = 0xffff;
      }
      else if ((ip_addr.s_addr & 0xff) < 223)
      {
        netmask.s_addr = 0xffffff;
      }
      else
      {
        rt_kprintf("bad parameter! e.g: ifconfig e0 192.168.1.30\n");
        return -1;
      }
      gw_addr.s_addr = (ip_addr.s_addr & 0xffffff) | (1 << 24);

      inet_ntoa_r(ip_addr, ip, 16);
      inet_ntoa_r(gw_addr, gw, 16);
      inet_ntoa_r(netmask, nm, 16);

      rt_kprintf("config : %s\n", argv[1]);
      rt_kprintf("IP addr: %s\n", ip);
      rt_kprintf("Gateway: %s\n", gw);
      rt_kprintf("netmask: %s\n", nm);
      set_if(argv[1], ip, gw, nm);
    }
    else if (argc == 5)
    {
        if ((strcmp(argv[1], "e0") == 0) && (strcmp(argv[2], "hw") == 0))
        {
          if (strcmp(argv[3], "ether") == 0)
          {
            set_mac(argv[1], argv[4]);
          }
          else
          {
            rt_kprintf("bad parameter! e.g: ifconfig e0 hw ether 86:30:33:44:55:66\n");
            return -1;
          }
        }
        else if (strcmp(argv[3], "netmask") == 0)
        {
          ip_addr.s_addr = inet_addr(argv[2]);
          netmask.s_addr = inet_addr(argv[4]);
          if ((ip_addr.s_addr == IPADDR_NONE) || netmask.s_addr == IPADDR_NONE)
          {
            rt_kprintf("bad parameter! Wrong ipaddr!\n");
            return -1;
          }
          gw_addr.s_addr = (ip_addr.s_addr & 0xffffff) | (1 << 24);
          inet_ntoa_r(ip_addr, ip, 16);
          inet_ntoa_r(gw_addr, gw, 16);
          inet_ntoa_r(netmask, nm, 16);
          rt_kprintf("config : %s\n", argv[1]);
          rt_kprintf("IP addr: %s\n", ip);
          rt_kprintf("Gateway: %s\n", gw);
          rt_kprintf("netmask: %s\n", nm);
          set_if(argv[1], ip, gw, nm);
        }
        else
        {
          ifconfig_usage();
        }
    }
    else
    {
        ifconfig_usage();
    }

    return 0;
}

#ifdef FINSH_USING_MSH
FINSH_FUNCTION_EXPORT_ALIAS(cmd_ifconfig, __cmd_ifconfig, list the information of network interfaces);
#endif

#endif /* RT_USING_LWIP */