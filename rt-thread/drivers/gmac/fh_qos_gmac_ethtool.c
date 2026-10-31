#include <rtthread.h>
#include <rthw.h>
#include <rtdevice.h>
#include <netif/ethernetif.h>
#include "lwipopts.h"
#include "fh_qos_gmac_phyt.h"
#include "mmu.h"
#include "fh_qos_gmac.h"
#include "mii.h"
#include "board_info.h"
#include "delay.h"

#define REG_SPACE_SIZE  0x1054
#define GMAC_ETHTOOL_NAME   "fh_qos_gmac"

int fh_gmac_phy_ethtool_gset(struct phy_device *phydev, struct ethtool_cmd *cmd);
int fh_gmac_phy_ethtool_sset(struct phy_device *phydev, struct ethtool_cmd *cmd);



static int fh_gmac_ethtool_getsettings(struct net_device *dev, struct ethtool_cmd *cmd)
{
    struct dw_qos *pGmac = dev->parent.user_data;
    struct phy_device *phy = pGmac->phydev;
    int rc;

    if (phy == RT_NULL)
    {
        rt_kprintf("%s: %s: PHY is not registered\n",
               __func__, dev->parent.parent.name);
        return -RT_ENOSYS;
    }
    if (!(dev->netif->flags & NETIF_FLAG_UP))
    {
        rt_kprintf("%s: interface is disabled: we cannot track "
        "link speed / duplex setting\n", dev->parent.parent.name);
        return -RT_EBUSY;
    }
    cmd->transceiver = XCVR_INTERNAL;
    rt_sem_take(&pGmac->lock, RT_WAITING_FOREVER);
    rc = fh_gmac_phy_ethtool_gset(phy, cmd);
    rt_sem_release(&pGmac->lock);

    return rc;
}

static int fh_gmac_ethtool_setsettings(struct net_device *dev, struct ethtool_cmd *cmd)
{
    struct dw_qos *pGmac = dev->parent.user_data;
    struct phy_device *phy = pGmac->phydev;
    int rc;

    rt_sem_take(&pGmac->lock, RT_WAITING_FOREVER);
    rc = fh_gmac_phy_ethtool_sset(phy, cmd);
    rt_sem_release(&pGmac->lock);

    return rc;
}

unsigned int ethtool_op_get_link(struct net_device *dev)
{
    return dev->netif->flags & NETIF_FLAG_LINK_UP ? 1 : 0;
}

static struct ethtool_ops fh_gmac_ethtool_ops = {
    .get_settings = fh_gmac_ethtool_getsettings,
    .set_settings = fh_gmac_ethtool_setsettings,
    .get_link = ethtool_op_get_link,
};

void fh_gmac_set_ethtool_ops(struct net_device *netdev)
{
    netdev->net.eth.ethtool_ops = &fh_gmac_ethtool_ops;
}
