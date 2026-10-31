#include <rtthread.h>
#include "ethernetif.h"
#include "generalWifi.h"

struct net_device wifi_dev_sta;
struct net_device wifi_dev_ap;

static rt_err_t wifi_device_control(rt_device_t dev, int cmd, void *args)
{
    int ret = 0;
    struct ifreq *rq;
    struct net_device *wifi_dev;

    if (!dev)
    {
        rt_kprintf("%s-%d: dev NULL fail\n", __func__, __LINE__);
        return -1;
    }
    rq = (struct ifreq *)args;
    wifi_dev = (struct net_device *)dev;

    if (rq)
        rt_kprintf("%s cmd 0x%x ifrn_name %s wifi_mode %d\n", __func__, cmd, rq->ifr_ifrn.ifrn_name, wifi_dev->net.wlan.work_mode);
    /* TODO: to customize */

    return ret;
}

#ifdef RT_USING_DEVICE_OPS
const static struct rt_device_ops wifi_dev_ops = {
    .init    = RT_NULL,
    .open    = RT_NULL,
    .close   = RT_NULL,
    .read    = RT_NULL,
    .write   = RT_NULL,
    .control = wifi_device_control
};
#endif

struct netif *wifi_init_net_device(unsigned int wifi_mode, netif_linkoutput_fn linkoutput, void *private)
{
    int ret = -1;
    struct net_device *dev;
    char *dev_name;
    rt_uint8_t flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP;
    rt_uint16_t iff_netcard = 0;

    if (!linkoutput)
    {
        rt_kprintf("%s-%d: linkoutput NULL ERR\n", __func__, __LINE__);
        return NULL;
    }
    switch (wifi_mode)
    {
    case 0:
        dev = &wifi_dev_sta;
        dev_name = IFNAME_STA;
        break;
    case 1:
        dev = &wifi_dev_ap;
        dev_name = IFNAME_AP;
        break;
    default:
        rt_kprintf("%s-%d: wifi_mode(%d) not support!\n", __func__, __LINE__, wifi_mode);
        return NULL;
    }

    if (dev->netif)
    {
        return dev->netif;/* net_device_init exec already */
    }

#ifdef RT_USING_DEVICE_OPS
    dev->parent.ops = &wifi_dev_ops;
#else
    dev->parent.init = RT_NULL;
    dev->parent.open = RT_NULL;
    dev->parent.close = RT_NULL;
    dev->parent.read = RT_NULL;
    dev->parent.write = RT_NULL;
    dev->parent.control = wifi_device_control;
#endif
    dev->parent.type = RT_Device_Class_NetIf;
    dev->net.wlan.linkoutput = linkoutput;
    dev->net.wlan.private = private;
#ifdef LWIP_IGMP
    flags |= NETIF_FLAG_IGMP;
    iff_netcard = IFF_MULTICAST;
#endif
    dev->net.wlan.work_mode = wifi_mode;

    ret = net_device_init(dev, dev_name, flags, NET_DEVICE_WLAN);
    if (ret)
    {
        rt_kprintf("%s-%d: net_device_init(dev_name %s NET_DEVICE_WLAN) ret %d\n", __func__, __LINE__, dev_name, ret);
        return NULL;
    }
    netdev_get_netcard_flags(dev, iff_netcard);

    return dev->netif;
}

void *wifi_dev_get_private(struct net_device *wifi_dev)
{
    if (wifi_dev)
    {
        return wifi_dev->net.wlan.private;
    }
    return NULL;
}