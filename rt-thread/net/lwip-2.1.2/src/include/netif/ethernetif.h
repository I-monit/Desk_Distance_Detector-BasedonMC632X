#ifndef __NETIF_ETHERNETIF_H__
#define __NETIF_ETHERNETIF_H__

#include "lwip/netif.h"
#include <rtthread.h>
#ifdef RT_USING_COMPONENTS_LINUX_ADAPTER
#include "linux/ethtool.h"
#include "net/if.h"
#include "linux/sockios.h"
#endif

#define NIOCTL_GADDR        0x01
#define NIOCTL_SADDR        0x02
#define NIOCTL_GFLAGS       0x03
#define NIOCTL_SFLAGS       0x04
/* NIOCTL_DEVPRIV~(NIOCTL_DEVPRIV + 0xf) are for netdev_do_ioctl() */
#define NIOCTL_DEVPRIV      0x05

#ifndef RT_LWIP_ETH_MTU
#define ETHERNET_MTU        1500
#else
#define ETHERNET_MTU        RT_LWIP_ETH_MTU
#endif

/* eth flag with auto_linkup or phy_linkup */
#define ETHIF_LINK_AUTOUP    0x0000
#define ETHIF_LINK_PHYUP     0x0100

#ifndef RT_USING_ETHTOOL
#define RT_USING_ETHTOOL
#endif

#if 0
struct eth_device
{
    /* inherit from rt_device */
    struct rt_device parent;

    /* network interface for lwip */
    struct netif *netif;
    struct rt_semaphore tx_ack;

    rt_uint16_t flags;
    rt_uint8_t  link_changed;
    rt_uint8_t  link_status;

    /* eth device interface */
    struct pbuf* (*eth_rx)(rt_device_t dev);
    rt_err_t (*eth_tx)(rt_device_t dev, struct pbuf *p);

#ifdef RT_USING_ETHTOOL
    struct ethtool_ops *ethtool_ops;
#endif
};
#endif

enum net_device_type
{
    NET_DEVICE_GMAC = 0,
    NET_DEVICE_USBNET,
    NET_DEVICE_WLAN,
    NET_DEVICE_BR,
    NET_DEVICE_DM9051,
    NET_DEVICE_ENC28J60
};

struct net_device
{
    /* inherit from rt_device */
    struct rt_device parent;

    /* network interface for lwip */
    struct netif *netif;
    rt_uint16_t flags;
    rt_uint8_t init_flags;
    rt_uint8_t type;
    union
    {
        struct
        {
            rt_uint8_t link_changed;
            rt_uint8_t link_status;
            rt_uint16_t reserve;
            /* eth device interface */
            struct pbuf* (*eth_rx)(rt_device_t dev);
            rt_err_t (*eth_tx)(rt_device_t dev, struct pbuf *p);
        #ifdef RT_USING_ETHTOOL
            struct ethtool_ops *ethtool_ops;
        #endif
        } eth;

        struct
        {
            rt_uint32_t reserve;
            rt_uint32_t work_mode;
            err_t (*linkoutput)(struct netif *netif, struct pbuf *p);/* VS ethernetif_linkoutput */
            void *private;
        } wlan;
    } net;
};

rt_err_t net_device_ready(struct net_device* dev);
rt_err_t net_device_init(struct net_device *dev, char *name, rt_uint8_t flag, rt_uint8_t type);
rt_err_t set_net_device_flag(struct net_device *dev, rt_uint8_t netif_flags);
rt_err_t net_device_linkchange(struct net_device* dev, rt_bool_t up);
err_t net_device_br_init(struct net_device *dev, struct netif *netif);

int eth_system_device_init(void);

extern int netdev_get_flags(char *netif_name, rt_uint16_t *flags);
extern int netdev_change_flags(char *netif_name, rt_uint16_t flags);
extern int netdev_do_ioctl(char *netif_name, struct ifreq *ifr, long cmd);
extern void netdev_get_netcard_flags(struct net_device *dev, rt_uint16_t iff_netcard);
#endif /* __NETIF_ETHERNETIF_H__ */
