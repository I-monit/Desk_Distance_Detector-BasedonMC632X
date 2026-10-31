#include <rtthread.h>
#include <string.h>
#include <errno.h>

#include <lwip/inet.h>
#include <sys/socket.h>
#include <lwip/netif.h>
#include <lwip/sockets.h>
#include <linux/ethtool.h>
#include <net/if.h>
#include <linux/sockios.h>

int gethostname(char *name, size_t len)
{
    if (len < 0)
    {
        errno = EINVAL;
        return -1;
    }

#if LWIP_NETIF_HOSTNAME
    const char *hostname;
    struct netif *netif;

    if (netif_default != RT_NULL)
        netif = netif_default;
    else
    {
        netif = netif_list;
        while (netif != NULL)
        {
            if (ip4_addr_get_u32(netif_ip4_addr(netif)) != 0)
                break;
            netif = netif->next;
        }
    }

    hostname = netif_get_hostname(netif);
    if (strlen(hostname) + 1 > len)
    {
        errno = ENAMETOOLONG;
        return -1;
    }
    strcpy(name, hostname);
    return 0;
#else
    errno = EFAULT;
    return -1;
#endif
}

int ethtool_glink(char *eth_name)
{
    int sock_fd;
    struct ifreq ifr;
    struct ethtool_value edata;
    if (!eth_name)
    {
        rt_kprintf("please input eth device name...\n");
        return -EINVAL;
    }
    sock_fd = lwip_socket(AF_INET, SOCK_DGRAM, 0);

    if (sock_fd < 0)
        return -1;

    edata.cmd = ETHTOOL_GLINK;

    memset(&ifr, 0, sizeof(ifr));

    strncpy(ifr.ifr_name, eth_name, sizeof(ifr.ifr_name) - 1);

    ifr.ifr_data = (char *)&edata;

    if (lwip_ioctl(sock_fd, SIOCETHTOOL, &ifr))
    {
        rt_kprintf("ETHTOOL_GLINK fail\n");
        lwip_close(sock_fd);
        return -1;
    }
    lwip_close(sock_fd);

    rt_kprintf("edata.data: 0x%x\n", edata.data);

    return 0;
}

int ethtool_gset(char *eth_name)
{
    int sock_fd;
    struct ifreq ifr;
    struct ethtool_cmd ecmd;

    if (!eth_name)
    {
        rt_kprintf("please input eth device name...\n");
        return -EINVAL;
    }

    sock_fd = lwip_socket(AF_INET, SOCK_DGRAM, 0);

    if (sock_fd < 0)
        return -1;

    ecmd.cmd = ETHTOOL_GSET;

    memset(&ifr, 0, sizeof(ifr));

    strncpy(ifr.ifr_name, eth_name, sizeof(ifr.ifr_name) - 1);

    ifr.ifr_data = (char *)&ecmd;

    if (lwip_ioctl(sock_fd, SIOCETHTOOL, &ifr))
    {
        rt_kprintf("ETHTOOL_GSET fail\n");
        lwip_close(sock_fd);
        return -1;
    }
    lwip_close(sock_fd);

    rt_kprintf("ecmd.cmd: 0x%x\n", ecmd.cmd);
    rt_kprintf("ecmd.supported: 0x%x\n", ecmd.supported);
    rt_kprintf("ecmd.advertising: 0x%x\n", ecmd.advertising);
    rt_kprintf("ecmd.speed: 0x%x\n", ecmd.speed);
    rt_kprintf("ecmd.speed_hi: 0x%x\n", ecmd.speed_hi);
    rt_kprintf("ecmd.duplex: 0x%x\n", ecmd.duplex);
    rt_kprintf("ecmd.phy_address: 0x%x\n", ecmd.phy_address);
    rt_kprintf("ecmd.autoneg: 0x%x\n", ecmd.autoneg);

    return 0;
}

int ethtool_sset(char *eth_name, int an, int speed, int duplex)
{
    int sock_fd;
    struct ifreq ifr;
    struct ethtool_cmd ecmd;

    if (!eth_name)
    {
        rt_kprintf("please input eth device name...\n");
        return -EINVAL;
    }

    sock_fd = lwip_socket(AF_INET, SOCK_DGRAM, 0);

    if (sock_fd < 0)
        return -1;

    ecmd.cmd = ETHTOOL_SSET;
    ecmd.phy_address = 0;
    ecmd.autoneg = an;
    ecmd.speed_hi = 0;
    ecmd.port = PORT_MII;
    ecmd.transceiver = XCVR_INTERNAL;
    ecmd.speed = speed;
    ecmd.duplex = duplex;
    ecmd.advertising = 0xffffffff;

    if (an)
    {
        ecmd.advertising |= ADVERTISED_Autoneg;
    }
    else
    {
        ecmd.advertising &= ~ADVERTISED_Autoneg;
    }

    if (duplex)
    {
        ecmd.advertising |= (ADVERTISED_10baseT_Full | ADVERTISED_100baseT_Full | ADVERTISED_1000baseT_Full);
    }
    else
    {
        ecmd.advertising &= ~(ADVERTISED_1000baseT_Full | ADVERTISED_100baseT_Full | ADVERTISED_10baseT_Full);
    }

    if (speed == SPEED_100)
    {
        ecmd.advertising &= ~(ADVERTISED_1000baseT_Full | ADVERTISED_1000baseT_Half);
    }

    if (speed == SPEED_10)
    {
        ecmd.advertising &= ~(ADVERTISED_100baseT_Full | ADVERTISED_100baseT_Half | ADVERTISED_1000baseT_Full | ADVERTISED_1000baseT_Half);
    }

    memset(&ifr, 0, sizeof(ifr));

    strncpy(ifr.ifr_name, eth_name, sizeof(ifr.ifr_name) - 1);

    ifr.ifr_data = (char *)&ecmd;

    if (lwip_ioctl(sock_fd, SIOCETHTOOL, &ifr))
    {
        rt_kprintf("ETHTOOL_SSET fail\n");
        lwip_close(sock_fd);
        return -1;
    }
    lwip_close(sock_fd);

    return 0;
}

static int ethtool_sset_adv(char *eth_name, int an, int speed, int duplex, unsigned int advertising)
{
    int sock_fd;
    struct ifreq ifr;
    struct ethtool_cmd ecmd;

    if (!eth_name)
    {
        rt_kprintf("please input eth device name...\n");
        return -EINVAL;
    }

    sock_fd = lwip_socket(AF_INET, SOCK_DGRAM, 0);

    if (sock_fd < 0)
        return -1;

    ecmd.cmd = ETHTOOL_SSET;
    ecmd.phy_address = 0;
    ecmd.autoneg = an;
    ecmd.speed_hi = 0;
    ecmd.port = PORT_MII;
    ecmd.transceiver = XCVR_INTERNAL;
    ecmd.speed = speed;
    ecmd.duplex = duplex;
    ecmd.advertising = advertising;

    memset(&ifr, 0, sizeof(ifr));

    strncpy(ifr.ifr_name, eth_name, sizeof(ifr.ifr_name) - 1);

    ifr.ifr_data = (char *)&ecmd;

    if (lwip_ioctl(sock_fd, SIOCETHTOOL, &ifr))
    {
        rt_kprintf("ETHTOOL_SSET fail\n");
        lwip_close(sock_fd);
        return -1;
    }
    lwip_close(sock_fd);

    return 0;
}

void ethtool_help(void)
{
    rt_kprintf("Usage:\n");
    rt_kprintf("ethtool <interface>\n");
    rt_kprintf("ethtool <-s> <interface> [speed 10|100|1000] [duplex half|full] [autoneg on|off]\n");
    rt_kprintf("e.g: ethtool e0\n");
    rt_kprintf("e.g: ethtool -s e0 autoneg on speed 100 duplex full\n");
}

int ecmd_init(char *eth_name, struct ethtool_cmd *ecmd)
{
    int sock_fd;
    struct ifreq ifr;

    if (!eth_name)
    {
        rt_kprintf("please input eth device name...\n");
        return -1;
    }

    sock_fd = lwip_socket(AF_INET, SOCK_DGRAM, 0);

    if (sock_fd < 0)
        return -1;

    ecmd->cmd = ETHTOOL_GSET;

    memset(&ifr, 0, sizeof(ifr));

    strncpy(ifr.ifr_name, eth_name, sizeof(ifr.ifr_name) - 1);

    ifr.ifr_data = (char *)ecmd;

    if (lwip_ioctl(sock_fd, SIOCETHTOOL, &ifr))
    {
        rt_kprintf("ETHTOOL_GSET fail\n");
        lwip_close(sock_fd);
        return -1;
    }
    lwip_close(sock_fd);

    return 0;
}

int ethtool(int argc, char **argv)
{
    struct ethtool_cmd ecmd;
    int i = 1;
    int ret = 0;

    if (argc == 2)
    {
        if (argv[1][0] != '-')
        {
            ethtool_gset(argv[1]);
        }
        else
        {
            ethtool_help();
        }
    }
    else if (argc > 2)
    {
        if (strcmp(argv[1], "-s") == 0)
        {
            i = 3;
            ret = ecmd_init(argv[2], &ecmd);
            if (ret < 0)
            {
                rt_kprintf("bad parameter!\n");
                return -1;
            }
            ecmd.advertising = 0xffffffff;
            ecmd.cmd = ETHTOOL_SSET;
            while (argv[i])
            {
                if (!argv[i+1])
                {
                    rt_kprintf("bad parameter! e.g: ethtool -s e0 autoneg on speed 100 duplex full\n");
                    return -1;
                }
                if (strcmp(argv[i], "autoneg") == 0)
                {
                    if (strcmp(argv[i+1], "on") == 0)
                    {
                        ecmd.autoneg = 1;
                        ecmd.advertising |= ADVERTISED_Autoneg;
                    }
                    else if (strcmp(argv[i+1], "off") == 0)
                    {
                        ecmd.autoneg = 0;
                         ecmd.advertising &= ~ADVERTISED_Autoneg;
                    }
                    else
                    {
                        rt_kprintf("bad parameter! e.g: ethtool -s e0 autoneg on/off\n");
                        return -1;
                    }
                }
                else if (strcmp(argv[i], "speed") == 0)
                {
                    ecmd.speed = atoi(argv[i+1]);

                    if (ecmd.speed == SPEED_100)
                    {
                        ecmd.advertising &= ~(ADVERTISED_1000baseT_Full | ADVERTISED_1000baseT_Half);
                    }

                    if (ecmd.speed == SPEED_10)
                    {
                        ecmd.advertising &= ~(ADVERTISED_100baseT_Full | ADVERTISED_100baseT_Half | ADVERTISED_1000baseT_Full | ADVERTISED_1000baseT_Half);
                    }

                }
                else if (strcmp(argv[i], "duplex") == 0)
                {
                    if (strcmp(argv[i+1], "full") == 0)
                    {
                        ecmd.duplex = 1;
                        if (ecmd.advertising & (ADVERTISED_1000baseT_Full | ADVERTISED_1000baseT_Half))
                            ecmd.advertising |= ADVERTISED_1000baseT_Full;
                        if (ecmd.advertising & (ADVERTISED_100baseT_Full | ADVERTISED_100baseT_Half))
                            ecmd.advertising |= ADVERTISED_100baseT_Full;
                        if (ecmd.advertising & (ADVERTISED_10baseT_Full | ADVERTISED_10baseT_Half))
                            ecmd.advertising |= ADVERTISED_10baseT_Full;
                    }
                    else if (strcmp(argv[i+1], "half") == 0)
                    {
                        ecmd.duplex = 0;
                        ecmd.advertising &= ~(ADVERTISED_1000baseT_Full | ADVERTISED_100baseT_Full | ADVERTISED_10baseT_Full);
                    }
                    else
                    {
                        rt_kprintf("bad parameter! e.g: ethtool -s e0 duplex full/half\n");
                        return -1;
                    }
                }
                else
                {
                    ethtool_help();
                    return -1;
                }
                i = i + 2;
            }

            if (ecmd.cmd == ETHTOOL_SSET)
            {
                ethtool_sset_adv(argv[2], ecmd.autoneg, ecmd.speed, ecmd.duplex, ecmd.advertising);
            }
        }
        else
        {
            ethtool_help();
        }
    }
    else
    {
         ethtool_help();
    }
    return 0;
}
#ifdef RT_USING_FINSH
#include <finsh.h>
FINSH_FUNCTION_EXPORT(ethtool_glink, ethtool_glink);
FINSH_FUNCTION_EXPORT(ethtool_gset, ethtool_gset);
FINSH_FUNCTION_EXPORT(ethtool_sset, ethtool_sset);
#endif
#ifdef FINSH_USING_MSH
MSH_CMD_EXPORT(ethtool, query or control network driver and hardware settings);
/* FINSH_FUNCTION_EXPORT_ALIAS(cmd_ethtool, __cmd_ethtool, ethtool);    */
#endif
