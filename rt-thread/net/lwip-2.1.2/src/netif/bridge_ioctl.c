/**
 * test demo of bridge function as wifi repeater of realtek (br_wifi_init(NOTE: br_name is fixed br in wifi)) or
 * repeater mixed wifi and ethernet(br_ap_eth_init(), NOTE: ethernet should enable Promiscuous mode)
 */
#include <rtthread.h>
#include "netif/bridgeif.h"
#include "netif/ethernetif.h"
#include "lwip/netif.h"
#include "lwip/sys.h"
#include "lwip/etharp.h"
#include "lwip/ethip6.h"
#include "lwip/snmp.h"
#include "lwip/timeouts.h"
#include "lwip/netifapi.h"
#include <string.h>
#include <net/ethernet.h>

/* a bridge MAC address 00-01-02-03-04-05, support 2 bridge ports, 1024 FDB entries + 16 static MAC entries */
static bridgeif_initdata_t mybridge_initdata;
static struct netif *bridge_netif = NULL;
static struct net_device br_net_device;

/* refer to eth_device_init_with_flag() */
struct netif *bridge_netif_init(char *br_name)
{
    int ret = 0;

    if (bridge_netif != NULL)
    {
        rt_kprintf("%s-%d: bridge_netif is not NULL\n", __func__, __LINE__);
        return bridge_netif;
    }

    /* if tcp thread has been started up, we add this netif to the system */
    if (rt_thread_find("tcpip") != RT_NULL)
    {
        bridge_netif = (struct netif *)rt_malloc(sizeof(struct netif));
        if (bridge_netif == RT_NULL)
        {
            rt_kprintf("malloc bridge_netif failed\n");
            return NULL;
        }

        rt_memset(bridge_netif, 0, sizeof(struct netif));
        rt_memset(&br_net_device, 0, sizeof(struct net_device));

        ret = netifapi_netif_add(bridge_netif, NULL, NULL, NULL, &mybridge_initdata, bridgeif_init, tcpip_input);
        if (ret)
        {
            rt_kprintf("%s-%d: netifapi_netif_add() ret %d\n", __func__, __LINE__, ret);
            rt_free(bridge_netif);
            bridge_netif = NULL;
            return NULL;
        }
        netif_set_up(bridge_netif);
    }
    else
    {
        rt_kprintf("%s-%d: Please wait until lwip_init complete\n", __func__, __LINE__, ret);
        return NULL;
    }

    /* set name again after bridgeif_init() */
    bridge_netif->name[0] = br_name[0];
    bridge_netif->name[1] = br_name[1];

    ret = net_device_br_init(&br_net_device, bridge_netif);
    if (ret != 0)
    {
        rt_kprintf("net_device_br_init failed.\n");
        rt_free(bridge_netif);
        bridge_netif = NULL;
        return NULL;
    }

    return bridge_netif;
}

/* demo for br_ap_eth_init and ioctl_wifi */
#include "sys/socket.h"
#include "net/if.h"
#include "linux/sockios.h"
#include "sys/ioctl.h"
#include "dfs_posix.h"

int ioctl_wifi(char *ifname)
{
    struct ifreq ifr;
    char buf[32] = {0};
    int s;
    int ret = -1;

    memset(&ifr, 0, sizeof(struct ifreq));
    sprintf(ifr.ifr_name, "%s", ifname);
    if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {

        printf("%s-%d: socket() fail\n", __func__, __LINE__);
        return -1;
    }

    ifr.ifr_data = buf;
    if ((ret = ioctl(s, SIOCDEVPRIVATE, &ifr)) < 0) {
        printf("%s-%d: cmd=%d fail\n", __func__, __LINE__,  SIOCDEVPRIVATE);
    }

    ifr.ifr_data = buf;
    if ((ret = ioctl(s, SIOCDEVPRIVATE+0xf, &ifr)) < 0) {
        printf("%s-%d: cmd=%d fail\n", __func__, __LINE__,  SIOCDEVPRIVATE+0xf);
    }
    close(s);

    return 0;
}

/* app 1: 应用 通过 socket 设置 以太网混杂模式 */
int promisc_eth(unsigned int enable)
{
    struct ifreq ifr;
    int s;
    int ret = -1;

    memset(&ifr, 0, sizeof(struct ifreq));
    sprintf(ifr.ifr_name, "%s", "e0");

    s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0)
    {
        printf("%s-%d: socket() fail\n", __func__, __LINE__);
        return -1;
    }

    ret = ioctl(s, SIOCGIFFLAGS, &ifr);
    if (ret < 0)
    {
        printf( "%s: cmd=%d fail\n", __func__, SIOCGIFFLAGS);
    }
    printf("%s-%d: ifr.ifr_name %s ifr.ifr_flags 0x%x ret %d\n", __func__, __LINE__, ifr.ifr_name, ifr.ifr_flags, ret);

    if (enable)
    {
        ifr.ifr_flags |= IFF_PROMISC;

        ret = ioctl(s, SIOCSIFFLAGS, &ifr);
        if (ret < 0)
            printf( "%s: cmd=%d fail\n", __func__, SIOCSIFFLAGS);
        /* check ifr.ifr_flags */
        ifr.ifr_flags = 0;
        ret = ioctl(s, SIOCGIFFLAGS, &ifr);
        if (ret < 0)
            printf("%s: cmd=%d fail\n", __func__, SIOCGIFFLAGS);

        printf("%s-%d: ifr.ifr_name %s ifr.ifr_flags 0x%x ret %d\n", __func__, __LINE__, ifr.ifr_name, ifr.ifr_flags, ret);
    } else
    {
        /* disable IFF_PROMISC */
        ifr.ifr_flags &= ~IFF_PROMISC;
        ret = ioctl(s, SIOCSIFFLAGS, &ifr);
        if (ret < 0)
            printf("%s: cmd=%d fail\n", __func__, SIOCGIFFLAGS);

        /* check ifr.ifr_flags */
        ifr.ifr_flags = 0;
        ret = ioctl(s, SIOCGIFFLAGS, &ifr);
        if (ret < 0)
        {
            printf("%s: cmd=%d fail\n", __func__, SIOCGIFFLAGS);
        }
        printf("%s-%d: ifr.ifr_name %s ifr.ifr_flags 0x%x ret %d\n", __func__, __LINE__, ifr.ifr_name, ifr.ifr_flags, ret);
    }
    close(s);

    return 0;
}

int br_add_portif(char *br_name, char *name)
{
    struct netif *netif = NULL;
    int ret = 0;

    if (bridge_netif == NULL)
    {
        rt_kprintf("[%s-%d]: bridge_netif is NULL!!\n", __func__, __LINE__);
        rt_kprintf("Please add bridge netif!!!\n");
        return -1;
    }
    netif = netif_find(name);
    if (netif == NULL)
    {
        rt_kprintf("%s-%d: netif_find(%s) fail\n", __func__, __LINE__, name);
        return -1;
    }
    netif->flags |= NETIF_FLAG_ETHERNET;

    if ((memcmp("e0", name, 2) == 0) || (memcmp("w0", name, 2) == 0))
    {
        bridgeif_set_hwaddr(bridge_netif, netif);
    }

    rt_kprintf("[%s-%d]: %s idx %u\n", __func__, __LINE__, name, netif_name_to_index(name));
    ret = bridgeif_add_port(bridge_netif, netif);
    if (ret)
    {
        rt_kprintf("%s-%d: bridgeif_add_port(bridge_netif, netif) ret %d\n", __func__, __LINE__, ret);
    }

    return ret;
}

int br_del_portif(char *br_name, char *name)
{
    struct netif *netif = NULL;

    if (bridge_netif == NULL)
    {
        rt_kprintf("[%s-%d]: bridge_netif is NULL!!\n", __func__, __LINE__);
        return -1;
    }

    netif = netif_find(name);
    if (netif)
    {
        netifapi_netif_enable_devflag(netif, NETIF_FLAG_ETHARP);
        if (bridge_netif)
            bridgeif_remove_port(bridge_netif, netif);
    }

    return 0;
}

int br_add_if(char *br_name, char *name)
{
    int ret = -1;

    if (bridge_netif == NULL)
    {
        rt_kprintf("[%s-%d]: bridge_netif is NULL!!\n", __func__, __LINE__);
        rt_kprintf("Please add bridge netif!!!\n");
        return -1;
    }

    if ((name == NULL) || br_name == NULL)
    {
        rt_kprintf("Wrong param!!!!\n");
        return -1;
    }

    if (memcmp("e0", name, 2) == 0)
    {
        promisc_eth(1);
    }

    ret = br_add_portif(br_name, name);
    printf("%s-%d: br_add_if ret %d\n", __func__, __LINE__, ret);

    return ret;
}

int br_del_if(char *br_name, char *name)
{
    int ret = 0;

    if (bridge_netif == NULL)
    {
        rt_kprintf("[%s-%d]: bridge_netif is NULL!!\n", __func__, __LINE__);
        rt_kprintf("Please add bridge netif!!!\n");
        return -1;
    }

    if ((name == NULL) || br_name == NULL)
    {
        rt_kprintf("Wrong param!!!!\n");
        return -1;
    }

    if (memcmp("e0", name, 2) == 0)
    {
        promisc_eth(0);
    }

    ret = br_del_portif(br_name, name);

    return ret;
}

int br_netif_init(char *br_name)
{
    mybridge_initdata.max_ports = 2;
    mybridge_initdata.max_fdb_dynamic_entries = 1024;
    mybridge_initdata.max_fdb_static_entries = 16;

    if (bridge_netif != NULL)
    {
        netifapi_netif_set_link_up(bridge_netif);
    }
    else
    {
        bridge_netif = bridge_netif_init(br_name);
        if (bridge_netif == NULL)
        {
            rt_kprintf("%s-%d: bridge_netif_init(%s) failed.\n", __func__, __LINE__, br_name);
            return -1;
        }
    }

    return 0;
}

int br_netif_deinit(char *br_name)
{
    if (bridge_netif == NULL)
    {
        rt_kprintf("[%s-%d]: bridge_netif is NULL!!!\n", __func__, __LINE__);
        return -1;
    }

    br_del_if(bridge_netif->name, "e0");
    br_del_if(bridge_netif->name, "w0");
    br_del_if(bridge_netif->name, "ap");
    netifapi_netif_set_link_down(bridge_netif);

    return 0;
}

extern struct ether_addr *ether_aton_r(const char *asc, struct ether_addr *addr);
int br_set_hwaddr(char *hwaddr)
{
    struct ether_addr br_hwaddr;

    if (hwaddr == NULL)
    {
        rt_kprintf("[%s-%d]: Wrong param!\n", __func__, __LINE__);
        return -1;
    }

    rt_memset(&br_hwaddr, 0, sizeof(br_hwaddr));

    if (ether_aton_r(hwaddr, &br_hwaddr) == NULL)
    {
        rt_kprintf("[%s-%d]: Wrong Hwaddr\n", __func__, __LINE__);
        return -1;
    }

    if (bridge_netif == NULL)
    {
        rt_kprintf("[%s-%d]: bridge_netif is NULL\n", __func__, __LINE__);
        return -1;
    }

    return bridgeif_set_hwaddr_ext(bridge_netif, br_hwaddr.ether_addr_octet);
}


#include <finsh.h>
#include <optparse.h>

typedef enum
{
    CMD_BR_INIT,
    CMD_BR_DEINIT,
    CMD_BR_ADD_IF,
    CMD_BR_DEL_IF,
    CMD_BR_HWADDR,
    CMD_BR_MAX
} BRIDGE_CMD_ID_E;

static void br_wifi_help(void)
{
    printf("Usage:\n"
        "br_wifi --help (show usage manual)\n"
        "br_wifi --addbr <bridge>\n"
        "br_wifi --br <bridge> --addif <netif>\n"
        "br_wifi --br <bridge> --delif <netif>\n"
        "br_wifi --delbr <bridge>\n"
        "br_wifi --hwaddr <xx:xx:xx:xx:xx:xx>\n");
}

int br_wifi(int argc, char **argv)
{
    struct optparse_long longopts[] = {
        {"help", 'h', OPTPARSE_NONE},
        {"addbr", 'a', OPTPARSE_REQUIRED},
        {"addif", 'i', OPTPARSE_REQUIRED},
        {"delif", 'r', OPTPARSE_REQUIRED},
        {"delbr", 'b', OPTPARSE_REQUIRED},
        {"hwaddr", 'w', OPTPARSE_REQUIRED},
        {"br", 'c', OPTPARSE_REQUIRED},
        {0}
    };

    BRIDGE_CMD_ID_E cmd_br_id = CMD_BR_MAX;
    int option;
    struct optparse options;
    char netif_name[16];
    char br_name[16];
    char br_hwaddr[20];

    rt_memset(netif_name, 0, sizeof(netif_name));
    rt_memset(br_name, 0, sizeof(br_name));
    rt_memset(br_hwaddr, 0, sizeof(br_hwaddr));

    if (argc < 2 || argc > 5)
    {
        br_wifi_help();
        return 0;
    }

    optparse_init(&options, argv);
    while ((option = optparse_long(&options, longopts, NULL)) != -1) {
        switch (option) {
        case 'h':
            br_wifi_help();
            return 0;
        case 'c':
            rt_memcpy(br_name, options.optarg, 2);
            break;
        case 'w':
            strcpy(br_hwaddr, options.optarg);
            cmd_br_id = CMD_BR_HWADDR;
            break;
        case 'i':
            rt_memcpy(netif_name, options.optarg, 2);
            cmd_br_id = CMD_BR_ADD_IF;
            break;
        case 'b':
            rt_memcpy(netif_name, options.optarg, 2);
            cmd_br_id = CMD_BR_DEINIT;
            break;
        case 'a':
            rt_memcpy(netif_name, options.optarg, 2);
            cmd_br_id = CMD_BR_INIT;
            break;
        case 'r':
            rt_memcpy(netif_name, options.optarg, 2);
            cmd_br_id = CMD_BR_DEL_IF;
            break;
        case '?':
            printf("%s %s\n", argv[0], options.errmsg);
            return -1;
        default:
            br_wifi_help();
            return -2;
        }
    }

    switch (cmd_br_id)
    {
        case CMD_BR_INIT:
            br_netif_init(netif_name);
            break;
        case CMD_BR_DEINIT:
            br_netif_deinit(netif_name);
            break;
        case CMD_BR_ADD_IF:
            br_add_if(br_name, netif_name);
            break;
        case CMD_BR_DEL_IF:
            br_del_if(br_name, netif_name);
            break;
        case CMD_BR_HWADDR:
            br_set_hwaddr(br_hwaddr);
            break;
        default:
            br_wifi_help();
            return -3;
    }

    return 0;
}
MSH_CMD_EXPORT(br_wifi, br wifi sta and ap or eth and wifi ap);

static void wifi_ioctl_help(void)
{
    printf("Usage:\n"
        "wifi_ioctl --help (show usage manual)\n"
        "wifi_ioctl --ifname x\n"
    );
}

int wifi_ioctl(int argc, char **argv)
{
    struct optparse_long longopts[] = {
        {"help", 'h', OPTPARSE_NONE},
        {"ifname", 'i', OPTPARSE_REQUIRED},
        {0}
    };
    char ifname[RT_NAME_MAX] = {0};
    int option;
    struct optparse options;

    optparse_init(&options, argv);
    while ((option = optparse_long(&options, longopts, NULL)) != -1) {
        switch (option) {
        case 'h':
            wifi_ioctl_help();
            return 0;
        case 'i':
            strncpy(ifname, options.optarg,
                strlen(options.optarg) <= (sizeof(ifname) - 1) ? strlen(options.optarg): (sizeof(ifname) - 1));
            break;
        case '?':
            printf("%s %s\n", argv[0], options.errmsg);
            return -1;
        default:
            wifi_ioctl_help();
            return -2;
        }
    }

    if (ifname[0])
    {
        ioctl_wifi(ifname);
        return 0;
    }

    wifi_ioctl_help();

    return 0;
}
MSH_CMD_EXPORT(wifi_ioctl, wifi device ioctl);
