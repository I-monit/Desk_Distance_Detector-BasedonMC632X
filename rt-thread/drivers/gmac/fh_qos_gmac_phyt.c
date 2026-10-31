/*****************************************************************************
 *  Include Section
 *  add all #include here
 *****************************************************************************/
#include "fh_qos_gmac.h"
#include "fh_qos_gmac_phyt.h"
#include "fh_pmu.h"
#include "pinctrl.h"
#include "gpio.h"

/*****************************************************************************
 * Define section
 * add all #define here
 *****************************************************************************/


/****************************************************************************
 * ADT section
 *  add definition of user defined Data Type that only be used in this file  here
 ***************************************************************************/


/*****************************************************************************
 *  static fun;
 *****************************************************************************/


/*****************************************************************************
 *  extern fun;
 *****************************************************************************/


/*****************************************************************************
 * Global variables section - Local
 * define global variables(will be refered only in this file) here,
 * static keyword should be used to limit scope of local variable to this file
 * e.g.
 *  static uint8_t ufoo;
 *****************************************************************************/
static int __fh_mdio_read(struct dw_qos *pGmac, int phyaddr, int phyreg);
static int __fh_mdio_write(struct dw_qos *pGmac, int phyaddr, int phyreg,
             u16_t phydata);
static int get_phy_id(struct dw_qos *gmac, int addr, u32_t *phy_id);
static int check_white_list(u32_t phy_id, u32_t *list, u32_t size);
static int scan_valid_phy_id(struct dw_qos *gmac,
u32_t *white_list, u32_t list_size, u32_t *ret_phy_id);
static int fh_mdio_set_mii(struct dw_qos *pGmac);

/*****************************************************************************
 * func below
 *****************************************************************************/
#ifndef PHY_MAX_ADDR
#define PHY_MAX_ADDR    32
#endif

unsigned int phy_support_list[] = {
    FH_GMAC_PHY_IP101G,
    FH_GMAC_PHY_RTL8201,
    FH_GMAC_PHY_TI83848,
    FH_GMAC_PHY_INTERNAL,
    FH_GMAC_PHY_DUMMY,
    FH_GMAC_PHY_INTERNAL_V2,
    FH_GMAC_PHY_RTL8211F,
    FH_GMAC_PHY_MAE0621,
    FH_GMAC_PHY_JL2101,
};

struct mdio_pin_mux_ref
{
    char *old;
    char *restore;
};

struct mdio_pin_mux_ref _mdio_pin_ref_obj[] = {
    {.old = "MAC_MDC", .restore = 0},
    {.old = "MAC_MDIO", .restore = 0},
    {.old = "MAC_RMII_CLK", .restore = 0},
    {.old = "EX_PHY_RESET_PIN", .restore = 0},
};

struct phy_setting
{
    int speed;
    int duplex;
    unsigned int setting;
};

/* A mapping of all SUPPORTED settings to speed/duplex */
static const struct phy_setting settings[] = {
    {
        .speed = 10000,
        .duplex = DUPLEX_FULL,
        .setting = SUPPORTED_10000baseT_Full,
    },
    {
        .speed = SPEED_1000,
        .duplex = DUPLEX_FULL,
        .setting = SUPPORTED_1000baseT_Full,
    },
    {
        .speed = SPEED_1000,
        .duplex = DUPLEX_HALF,
        .setting = SUPPORTED_1000baseT_Half,
    },
    {
        .speed = SPEED_100,
        .duplex = DUPLEX_FULL,
        .setting = SUPPORTED_100baseT_Full,
    },
    {
        .speed = SPEED_100,
        .duplex = DUPLEX_HALF,
        .setting = SUPPORTED_100baseT_Half,
    },
    {
        .speed = SPEED_10,
        .duplex = DUPLEX_FULL,
        .setting = SUPPORTED_10baseT_Full,
    },
    {
        .speed = SPEED_10,
        .duplex = DUPLEX_HALF,
        .setting = SUPPORTED_10baseT_Half,
    },
};

#define MAX_NUM_SETTINGS ARRAY_SIZE(settings)

static int __fh_mdio_read(struct dw_qos *pGmac, int phyaddr, int phyreg)
{
#ifdef CONFIG_EMULATION
    return 0xe3ff;
#else
    u32_t ret;
    int i = 10000;

    dw_writel(pGmac, mac.mdio_addr,
            0x3 << 2 | phyaddr << 21 | 1 << 8 | phyreg << 16 | 0x1);
    while ((dw_readl(pGmac, mac.mdio_addr) & 0x1) && i)
    {
        i--;
    }
    if (!i)
    {
        rt_kprintf("ERROR: %s, timeout, phyaddr: 0x%x, phyreg: 0x%x\n",
        __func__, phyaddr, phyreg);
        return 0;
    }
    ret  = dw_readl(pGmac, mac.mdio_data);
    /* rt_kprintf("[read] :: reg:%08x = %08x\n",phyreg, ret); */
    return ret;
#endif

}

static int __fh_mdio_write(struct dw_qos *pGmac, int phyaddr, int phyreg,
             u16_t phydata)
{

#ifdef CONFIG_EMULATION
    return 0;
#else
    /* rt_kprintf("[write] :: reg:%08x = %08x\n",phyreg, phydata); */
    int i = 10000;

    dw_writel(pGmac, mac.mdio_data, phydata);
    dw_writel(pGmac, mac.mdio_addr, 0x1 << 2 | phyaddr << 21 | 1 << 8
                    | phyreg << 16 | 0x1);
    while ((dw_readl(pGmac, mac.mdio_addr) & 0x1) && i)
    {
        i--;
    }
    if (!i)
    {
        rt_kprintf("ERROR: %s, timeout, phyaddr: %d, phyreg: 0x%x, phydata: 0x%x\n",
                __func__, phyaddr, phyreg, phydata);
    }
    return 0;
#endif

}

static int fh_mdio_read(struct dw_qos *pGmac, int phyaddr, int phyreg)
{
    return __fh_mdio_read(pGmac, phyaddr, phyreg);
}

static int fh_mdio_write(struct dw_qos *pGmac, int phyaddr, int phyreg,
             u16_t phydata)
{
    return __fh_mdio_write(pGmac, phyaddr, phyreg, phydata);
}


static int get_phy_id(struct dw_qos *gmac, int addr, u32_t *phy_id)
{
    u16_t phy_reg;

    phy_reg = __fh_mdio_read(gmac, addr, 2);
    *phy_id = (phy_reg & 0xffff) << 16;
    phy_reg = __fh_mdio_read(gmac, addr, 3);
    *phy_id |= (phy_reg & 0xffff);

    return 0;
}


static int check_white_list(u32_t phy_id, u32_t *list, u32_t size)
{
    int i;
    int ret = -1;

    for (i = 0; i < size; i++)
    {
        if (phy_id == list[i])
        {
            ret = 0;
            break;
        }
    }
    return ret;
}

/* 0 find valid, others failed*/
static int scan_valid_phy_id(struct dw_qos *gmac,
u32_t *white_list, u32_t list_size, u32_t *ret_phy_id)
{
    u32_t phy_id = 0;
    int i;
    int ret;
    *ret_phy_id = 0xffffffff;
    /* add 2 dummy read.... */
    get_phy_id(gmac, 0, (u32_t *)&phy_id);
    get_phy_id(gmac, 0, (u32_t *)&phy_id);

    for (i = 0; i < PHY_MAX_ADDR; i++)
    {
        ret = get_phy_id(gmac, i, (u32_t *)&phy_id);
        if (ret == 0 && (phy_id & 0x1fffffff) != 0x1fffffff)
        {
            if (check_white_list(phy_id, white_list, list_size))
                continue;
            break;
        }
    }
    if (i == PHY_MAX_ADDR)
        return -1;
    *ret_phy_id = phy_id;
    return 0;
}

#if 0
int fh_phy_reset_api(struct phy_device *phydev)
{
    struct net_device *ndev;
    struct dw_qos *pGmac;

    rt_kprintf("reset api get in....\n");
    ndev = phydev->mdio.bus->priv;
    pGmac = netdev_priv(ndev);

    if (pGmac->phy_sel)
        pGmac->phy_sel(pGmac->ac_phy_info->phy_sel);
    if (pGmac->inf_set)
        pGmac->inf_set(pGmac->ac_phy_info->phy_inf);
    if (pGmac->ac_phy_info->phy_reset)
        pGmac->ac_phy_info->phy_reset();

    fh_mdio_set_mii(pGmac->mii);
    return 0;
}
EXPORT_SYMBOL(fh_phy_reset_api);


int fh_phy_triger_api(struct phy_device *phydev)
{
    struct net_device *ndev;
    int ret;
    struct dw_qos *pGmac;

    ndev = phydev->mdio.bus->priv;
    pGmac = netdev_priv(ndev);
    if (pGmac->ac_phy_info->phy_triger)
    {
        ret = pGmac->ac_phy_info->phy_triger();
        if (ret != PHY_TRIGER_OK)
        {
            rt_kprintf("got triger evet [%x]\n", ret);
            if (pGmac->phy_sel)
                pGmac->phy_sel(pGmac->ac_phy_info->phy_sel);
            if (pGmac->inf_set)
                pGmac->inf_set(pGmac->ac_phy_info->phy_inf);
            if (pGmac->ac_phy_info->phy_reset)
                pGmac->ac_phy_info->phy_reset();

            fh_mdio_set_mii(pGmac->mii);
        }
    }
    return 0;
}
EXPORT_SYMBOL(fh_phy_triger_api);
#endif

void bind_phy_reg_cfg(struct dw_qos *pGmac, int phy_id)
{
    struct phy_reg_cfg *p_cfg;

    p_cfg = (struct phy_reg_cfg *)pGmac->priv_data->p_cfg_array;

    for (; p_cfg->id != 0; p_cfg++)
    {
        if (p_cfg->id == phy_id)
        {
            /*rt_kprintf("find phy id [%08x] reg para\n",p_cfg->id);*/
            /* bind phy set list here.. */
            pGmac->ac_reg_cfg = p_cfg;
            /* set phy ref work clk. */
            if (pGmac->ac_phy_info->ex_sync_mac_spd)
            {
                pGmac->ac_phy_info->ex_sync_mac_spd(100,
                pGmac->ac_reg_cfg);
            }
        }
    }
}

int auto_find_phy(struct dw_qos *pGmac)
{
    int i;
    int ret = -1;
    int ret_phy_id;
    struct fh_gmac_platform_data *p_info;
    struct phy_interface_info *p_phy_info;

    p_info = (struct fh_gmac_platform_data *)pGmac->priv_data;
    if (!p_info)
    {
        rt_kprintf("NULL plat info ....\n");
        return -1;
    }
    for (i = 0, p_phy_info = p_info->phy_info; i < ARRAY_SIZE(p_info->phy_info); i++)
    {
        /* only parse usr set info.if not set ,just continue. */
        if (!!p_phy_info[i].phy_sel)
        {
            /* 2:phy sel */
            if (p_info->phy_sel)
                p_info->phy_sel(p_phy_info[i].phy_sel);
            /* 3:phy reset */
            if (p_phy_info[i].phy_reset)
                p_phy_info[i].phy_reset();
            /* 4:mac interface set mii or rmii...sw set later,when phy auto done... */
#if (0)
            if (p_info->inf_set)
                p_info->inf_set(p_phy_info[i].phy_inf);
#endif
            /* find phy id.. */
            ret = scan_valid_phy_id(pGmac, (u32_t *)phy_support_list,
            ARRAY_SIZE(phy_support_list), (u32_t *)&ret_phy_id);
            if (!ret)
                break;
        }
    }
    if (!ret)
    {
        /* find valid phy...just parse */

        pGmac->ac_phy_info = &p_phy_info[i];
        bind_phy_reg_cfg(pGmac, ret_phy_id);
        pGmac->phy_sel = p_info->phy_sel;
        pGmac->inf_set = p_info->inf_set;
        pGmac->phy_interface = p_phy_info[i].phy_inf;
        rt_kprintf("auto find phy info :: %s : ",
        p_phy_info[i].phy_sel == EXTERNAL_PHY ?
        "external phy" : "internal phy");

        if (p_phy_info[i].phy_inf == PHY_INTERFACE_MODE_MII)
            rt_kprintf(" mii\n");
        if (p_phy_info[i].phy_inf == PHY_INTERFACE_MODE_RMII)
            rt_kprintf(" rmii\n");
        if (p_phy_info[i].phy_inf == PHY_INTERFACE_MODE_RGMII)
            rt_kprintf(" rgmii\n");

        /* sync with net core later... */
        return 0;
    }
    return -1;
}

int genphy_update_link(struct dw_qos *gmac, struct phy_device *phydev)
{
    int status;

    /* Do a fake read */
    status = fh_mdio_read(gmac, gmac->phy_addr, MII_BMSR);

    if (status < 0)
        return status;

    /* Read link and autonegotiation status */
    status = fh_mdio_read(gmac, gmac->phy_addr, MII_BMSR);

    if (status < 0)
        return status;

    if ((status & BMSR_LSTATUS) == 0)
        phydev->link = 0;
    else
        phydev->link = 1;

    return 0;
}

int genphy_read_status(struct dw_qos *gmac, struct phy_device *phydev)
{
    int adv;
    int err;
    int lpa;
    int lpagb = 0;

    /*
     *Update the link, but return if there
     * was an error
     */
    err = genphy_update_link(gmac, phydev);
    if (err)
        return err;

    if (AUTONEG_ENABLE == phydev->autoneg)
    {
        if (phydev->supported & (SUPPORTED_1000baseT_Half
                    | SUPPORTED_1000baseT_Full))
        {
            lpagb = fh_mdio_read(gmac, gmac->phy_addr, MII_STAT1000);

            if (lpagb < 0)
                return lpagb;

            adv = fh_mdio_read(gmac, gmac->phy_addr, MII_CTRL1000);

            if (adv < 0)
                return adv;

            lpagb &= adv << 2;
        }

        lpa = fh_mdio_read(gmac, gmac->phy_addr, MII_LPA);

        if (lpa < 0)
            return lpa;

        adv = fh_mdio_read(gmac, gmac->phy_addr, MII_ADVERTISE);

        if (adv < 0)
            return adv;

        lpa &= adv;

        phydev->speed = SPEED_10;
        phydev->duplex = DUPLEX_HALF;
        phydev->pause = phydev->asym_pause = 0;

        if (lpagb & (LPA_1000FULL | LPA_1000HALF))
        {
            phydev->speed = SPEED_1000;

            if (lpagb & LPA_1000FULL)
                phydev->duplex = DUPLEX_FULL;
        } else if (lpa & (LPA_100FULL | LPA_100HALF))
        {
            phydev->speed = SPEED_100;

            if (lpa & LPA_100FULL)
                phydev->duplex = DUPLEX_FULL;
        }
        else if (lpa & LPA_10FULL)
            phydev->duplex = DUPLEX_FULL;

        if (phydev->duplex == DUPLEX_FULL)
        {
            phydev->pause = lpa & LPA_PAUSE_CAP ? 1 : 0;
            phydev->asym_pause = lpa & LPA_PAUSE_ASYM ? 1 : 0;
        }
    }
    else
    {
        int bmcr = fh_mdio_read(gmac, gmac->phy_addr, MII_BMCR);
        if (bmcr < 0)
            return bmcr;

        if (bmcr & BMCR_FULLDPLX)
            phydev->duplex = DUPLEX_FULL;
        else
            phydev->duplex = DUPLEX_HALF;

        if (bmcr & BMCR_SPEED1000)
            phydev->speed = SPEED_1000;
        else if (bmcr & BMCR_SPEED100)
            phydev->speed = SPEED_100;
        else
            phydev->speed = SPEED_10;

        phydev->pause = phydev->asym_pause = 0;
    }

    return 0;
}

static void phy_error(struct phy_device *phydev)
{
    phydev->state = PHY_HALTED;
}

static inline int phy_find_valid(int idx, unsigned int features)
{
    while (idx < MAX_NUM_SETTINGS && !(settings[idx].setting & features))
        idx++;

    return idx < MAX_NUM_SETTINGS ? idx : MAX_NUM_SETTINGS - 1;
}

static inline int phy_find_setting(int speed, int duplex)
{
    int idx = 0;

    while (idx < ARRAY_SIZE(settings) &&
            (settings[idx].speed != speed ||
            settings[idx].duplex != duplex))
        idx++;

    return idx < MAX_NUM_SETTINGS ? idx : MAX_NUM_SETTINGS - 1;
}

static void phy_sanitize_settings(struct phy_device *phydev)
{
    unsigned int features = phydev->supported;
    int idx;

    /* Sanitize settings based on PHY capabilities */
    if ((features & SUPPORTED_Autoneg) == 0)
        phydev->autoneg = AUTONEG_DISABLE;

    idx = phy_find_valid(phy_find_setting(phydev->speed, phydev->duplex),
            features);

    phydev->speed = settings[idx].speed;
    phydev->duplex = settings[idx].duplex;
}

static int genphy_setup_forced(struct dw_qos *gmac, struct phy_device *phydev)
{
    int err;
    int ctl = 0;

    phydev->pause = phydev->asym_pause = 0;

    if (SPEED_1000 == phydev->speed)
        ctl |= BMCR_SPEED1000;
    else if (SPEED_100 == phydev->speed)
        ctl |= BMCR_SPEED100;

    if (DUPLEX_FULL == phydev->duplex)
        ctl |= BMCR_FULLDPLX;

    err = fh_mdio_write(gmac, gmac->phy_addr, MII_BMCR, ctl);

    return err;
}

static int genphy_config_advert(struct dw_qos *gmac, struct phy_device *phydev)
{
    unsigned int advertise;
    int oldadv, adv, bmsr;
    int err, changed = 0;

    /* Only allow advertising what
     * this PHY supports */
    phydev->advertising &= phydev->supported;
    advertise = phydev->advertising;

    /* Setup standard advertisement */
    oldadv = adv = fh_mdio_read(gmac, gmac->phy_addr, MII_ADVERTISE);

    if (adv < 0)
        return adv;

    adv &= ~(ADVERTISE_ALL | ADVERTISE_100BASE4 | ADVERTISE_PAUSE_CAP |
         ADVERTISE_PAUSE_ASYM);
    if (advertise & ADVERTISED_10baseT_Half)
        adv |= ADVERTISE_10HALF;
    if (advertise & ADVERTISED_10baseT_Full)
        adv |= ADVERTISE_10FULL;
    if (advertise & ADVERTISED_100baseT_Half)
        adv |= ADVERTISE_100HALF;
    if (advertise & ADVERTISED_100baseT_Full)
        adv |= ADVERTISE_100FULL;
    if (advertise & ADVERTISED_Pause)
        adv |= ADVERTISE_PAUSE_CAP;
    if (advertise & ADVERTISED_Asym_Pause)
        adv |= ADVERTISE_PAUSE_ASYM;

    if (adv != oldadv)
    {
        err = fh_mdio_write(gmac, gmac->phy_addr, MII_ADVERTISE, adv);

        if (err < 0)
            return err;
        changed = 1;
    }

    bmsr = fh_mdio_read(gmac, gmac->phy_addr, MII_BMSR);
    if (bmsr < 0)
        return bmsr;

    /* Per 802.3-2008, Section 22.2.4.2.16 Extended status all
    * 1000Mbits/sec capable PHYs shall have the BMSR_ESTATEN bit set to a
    * logical 1.
    */
    if (!(bmsr & BMSR_ESTATEN))
        return changed;

    /* Configure gigabit if it's supported */
    adv = fh_mdio_read(gmac, gmac->phy_addr, MII_CTRL1000);
    if (adv < 0)
        return adv;

    oldadv = adv;
    adv &= ~(ADVERTISE_1000FULL | ADVERTISE_1000HALF);

    if (phydev->supported & (SUPPORTED_1000baseT_Half |
                    SUPPORTED_1000baseT_Full)) {
        if (advertise & ADVERTISED_1000baseT_Half)
            adv |= ADVERTISE_1000HALF;
        if (advertise & ADVERTISED_1000baseT_Full)
            adv |= ADVERTISE_1000FULL;
    }

    if (adv != oldadv)
        changed = 1;

    err = fh_mdio_write(gmac, gmac->phy_addr, MII_CTRL1000, adv);
    if (err < 0)
        return err;

    return changed;
}

int genphy_restart_aneg(struct dw_qos *gmac, struct phy_device *phydev)
{
    int ctl;

    ctl = fh_mdio_read(gmac, gmac->phy_addr, MII_BMCR);

    if (ctl < 0)
        return ctl;

    ctl |= (BMCR_ANENABLE | BMCR_ANRESTART);

    /* Don't isolate the PHY if we're negotiating */
    ctl &= ~(BMCR_ISOLATE);

    ctl = fh_mdio_write(gmac, gmac->phy_addr, MII_BMCR, ctl);

    return ctl;
}

int genphy_config_aneg(struct dw_qos *gmac, struct phy_device *phydev)
{
    int result;

    if (AUTONEG_ENABLE != phydev->autoneg)
    {
        return genphy_setup_forced(gmac, phydev);
    }

    result = genphy_config_advert(gmac, phydev);

    if (result < 0) /* error */
        return result;

    if (result == 0)
    {
        /* Advertisement hasn't changed, but maybe aneg was never on to
         * begin with?  Or maybe phy was isolated? */
        int ctl = fh_mdio_read(gmac, gmac->phy_addr, MII_BMCR);

        if (ctl < 0)
            return ctl;

        if (!(ctl & BMCR_ANENABLE) || (ctl & BMCR_ISOLATE))
            result = 1; /* do restart aneg */
    }

    /* Only restart aneg if we are advertising something different
     * than we were before.  */
    if (result > 0)
    {
        result = genphy_restart_aneg(gmac, phydev);
    }

    return result;
}

int phy_start_aneg(struct dw_qos *gmac, struct phy_device *phydev)
{
    int err;

    if (AUTONEG_DISABLE == phydev->autoneg)
        phy_sanitize_settings(phydev);

    err = genphy_config_aneg(gmac, phydev);

    if (err < 0)
        return err;

    if (phydev->state != PHY_HALTED)
    {
        if (AUTONEG_ENABLE == phydev->autoneg)
        {
            phydev->state = PHY_AN;
            phydev->link_timeout = PHY_AN_TIMEOUT;
        }
        else
        {
            phydev->state = PHY_FORCING;
            phydev->link_timeout = PHY_FORCE_TIMEOUT;
        }
    }

    return err;
}

static void phy_force_reduction(struct phy_device *phydev)
{
    int idx;

    idx = phy_find_setting(phydev->speed, phydev->duplex);

    idx++;

    idx = phy_find_valid(idx, phydev->supported);

    phydev->speed = settings[idx].speed;
    phydev->duplex = settings[idx].duplex;

    rt_kprintf("Trying %d/%s\n", phydev->speed,
            DUPLEX_FULL == phydev->duplex ?
            "FULL" : "HALF");
}

static inline int phy_aneg_done(struct dw_qos *gmac, struct phy_device *phydev)
{
    int retval;

    retval = fh_mdio_read(gmac, gmac->phy_addr, MII_BMSR);

    return (retval < 0) ? retval : (retval & BMSR_ANEGCOMPLETE);
}

void phy_state_machine(void *param)
{
    struct dw_qos *gmac = param;
    struct phy_device *phydev = gmac->phydev;
    int needs_aneg = 0;
    int err = 0;

    rt_sem_take(&gmac->mdio_bus_lock, RT_WAITING_FOREVER);

    switch (phydev->state)
    {
    case PHY_DOWN:
    case PHY_STARTING:
    case PHY_READY:
    case PHY_PENDING:
        break;
    case PHY_UP:
        needs_aneg = 1;

        phydev->link_timeout = PHY_AN_TIMEOUT;

        break;
    case PHY_AN:
        err = genphy_read_status(gmac, phydev);

        if (err < 0)
            break;

        /* If the link is down, give up on
            * negotiation for now */
        if (!phydev->link)
        {
            phydev->state = PHY_NOLINK;
            fh_qos_adjust_link(gmac);
            break;
        }

        /* Check if negotiation is done.  Break
            * if there's an error */
        err = phy_aneg_done(gmac, phydev);
        if (err < 0)
            break;

        /* If AN is done, we're running */
        if (err > 0)
        {
            phydev->state = PHY_RUNNING;
            fh_qos_adjust_link(gmac);

        }
        else if (0 == phydev->link_timeout--)
        {
            int idx;

            needs_aneg = 1;

            /* The timer expired, and we still
                * don't have a setting, so we try
                * forcing it until we find one that
                * works, starting from the fastest speed,
                * and working our way down */
            idx = phy_find_valid(0, phydev->supported);

            phydev->speed = settings[idx].speed;
            phydev->duplex = settings[idx].duplex;

            phydev->autoneg = AUTONEG_DISABLE;

            rt_kprintf("Trying %d/%s\n", phydev->speed,
                    DUPLEX_FULL ==
                    phydev->duplex ?
                    "FULL" : "HALF");
        }
        break;
    case PHY_NOLINK:
        err = genphy_read_status(gmac, phydev);

        if (err)
            break;

        if (phydev->link)
        {
            phydev->state = PHY_RUNNING;
            fh_qos_adjust_link(gmac);
        }
        break;
    case PHY_FORCING:
        err = genphy_update_link(gmac, phydev);

        if (err)
            break;

        if (phydev->link)
        {
            phydev->state = PHY_RUNNING;
        }
        else
        {
            if (0 == phydev->link_timeout--)
            {
                phy_force_reduction(phydev);
                needs_aneg = 1;
            }
        }

        fh_qos_adjust_link(gmac);
        break;
    case PHY_RUNNING:
        /* Only register a CHANGE if we are
            * polling */
        if (-1 == phydev->irq)
            phydev->state = PHY_CHANGELINK;
        break;
    case PHY_CHANGELINK:
        err = genphy_read_status(gmac, phydev);

        if (err)
            break;

        if (phydev->link)
        {
            phydev->state = PHY_RUNNING;
        } else
        {
            phydev->state = PHY_NOLINK;
        }

        fh_qos_adjust_link(gmac);

        break;
    case PHY_HALTED:
        if (phydev->link)
        {
            phydev->link = 0;
            fh_qos_adjust_link(gmac);
        }
        break;
    case PHY_RESUMING:
        if (AUTONEG_ENABLE == phydev->autoneg)
        {
            err = phy_aneg_done(gmac, phydev);
            if (err < 0)
                break;

            /* err > 0 if AN is done.
                * Otherwise, it's 0, and we're
                * still waiting for AN */
            if (err > 0)
            {
                err = genphy_read_status(gmac, phydev);
                if (err)
                    break;

                if (phydev->link)
                {
                    phydev->state = PHY_RUNNING;
                }
                else
                    phydev->state = PHY_NOLINK;
                fh_qos_adjust_link(gmac);
            }
            else
            {
                phydev->state = PHY_AN;
                phydev->link_timeout = PHY_AN_TIMEOUT;
            }
        }
        else
        {
            err = genphy_read_status(gmac, phydev);
            if (err)
                break;

            if (phydev->link)
            {
                phydev->state = PHY_RUNNING;
            }
            else
            {
                phydev->state = PHY_NOLINK;
            }
            fh_qos_adjust_link(gmac);
        }
        break;
    }

    rt_sem_release(&gmac->mdio_bus_lock);

    if (needs_aneg)
        err = phy_start_aneg(gmac, phydev);

    if (err < 0)
        phy_error(phydev);
}

#if 0
int fh_mdio_reset(struct mii_bus *bus)
{

    return 0;
}
#endif

static int fh_mdio_set_mii(struct dw_qos *pGmac)
{
    u32_t ret;
    struct phy_reg_cfg *p_cfg;
    struct phy_reg_cfg_list *p_cfg_data;

    int phyid = pGmac->phy_id;

    if (pGmac->phydev == NULL)
        return -ENODEV;

#ifdef CONFIG_EMULATION
        return 0;
#endif

    p_cfg = (struct phy_reg_cfg *)pGmac->priv_data->p_cfg_array;
    for (; p_cfg->id != 0; p_cfg++)
    {
        if (p_cfg->id == pGmac->phydev->phy_id)
        {
            for (p_cfg_data = p_cfg->list;
                p_cfg_data->r_w != 0; p_cfg_data++)
            {
                if ((p_cfg_data->r_w & ACTION_MASK) == W_PHY)
                    fh_mdio_write(pGmac, phyid,
                        p_cfg_data->reg_add,
                        p_cfg_data->reg_val);
                else if ((p_cfg_data->r_w & ACTION_MASK) == M_PHY)
                {
                    ret = fh_mdio_read(pGmac, phyid,
                    p_cfg_data->reg_add);
                    ret &= ~(p_cfg_data->reg_mask);
                    ret |= (p_cfg_data->reg_val &
                    p_cfg_data->reg_mask);
                    fh_mdio_write(pGmac, phyid,
                    p_cfg_data->reg_add, ret);
                }
            }
            /*find one match;return ok*/
            return 0;
        }
    }
    return -ENODEV;
}

#if 0
void fh_qos_find_valid_phy_busid(struct net_device *ndev)
{
    struct dw_qos *pGmac = netdev_priv(ndev);
    /* here set bus id is broadcast id... */
    /* but mdio suppoty multi phyid...maybe use phyid array is better. */
    pGmac->phy_id = 0;
}

int fh_gmac_phy_reset(struct dw_qos *pGmac, unsigned int phy_sel)
{
    return 0;
}
#endif

static struct phy_device *phy_device_create(struct dw_qos *pGmac,
                        int addr, int phy_id)
{
    struct phy_device *dev;

    /* We allocate the device, and initialize the
     * default values */
    dev = rt_malloc(sizeof(*dev));

    if (NULL == dev)
        return RT_NULL;

#ifdef FH_GMAC_DISABLE_NG
    dev->speed = 100;
    dev->duplex = 1;
    dev->pause = dev->asym_pause = 0;
    dev->link = 1;
    dev->interface = PHY_INTERFACE_MODE_RMII;

    dev->autoneg = AUTONEG_DISABLE;

    dev->supported = 0xffffffff;
    dev->advertising = 0xffffffff;

    dev->addr = addr;
    dev->phy_id = phy_id;
    dev->state = PHY_RUNNING;
#else
    dev->speed = 0;
    dev->duplex = -1;
    dev->pause = dev->asym_pause = 0;
    dev->link = 1;
    dev->interface = PHY_INTERFACE_MODE_RMII;

    dev->autoneg = AUTONEG_ENABLE;

    dev->supported = 0xffffffff;
    dev->advertising = 0xffffffff;

    dev->addr = addr;
    dev->phy_id = phy_id;
    dev->state = PHY_DOWN;
#endif

    rt_timer_start(&pGmac->timer);

    return dev;
}

struct phy_device *get_phy_device(struct dw_qos *pGmac, int addr)
{
    struct phy_device *dev = NULL;
    unsigned int phy_id;
    int r;

    r = get_phy_id(pGmac, addr, (u32_t *)&phy_id);
    if (r)
        return RT_NULL;

    /* If the phy_id is mostly Fs, there is no device there */
    if ((phy_id & 0x1fffffff) == 0x1fffffff)
        return NULL;

    dev = phy_device_create(pGmac, addr, phy_id);

    return dev;
}

struct phy_device *mdio_scan(struct dw_qos *pGmac, int addr)
{
    struct phy_device *phydev;

    phydev = get_phy_device(pGmac, addr);
    if (phydev == RT_NULL)
        return phydev;

    return phydev;
}
int fh_qos_mdio_register(struct net_device *dev)
{
    int i;
    struct dw_qos *pGmac = dev->parent.user_data;
#if 0
    fh_gmac_phy_reset(pGmac, pGmac->phy_sel);
#endif

    for (i = 0; i < PHY_MAX_ADDR; i++)
    {
        struct phy_device *phydev;

        pGmac->phy_addr = i;
        phydev = mdio_scan(pGmac, i);

        if (phydev)
        {
            phydev->attached_dev = dev;
            phydev->dev_flags = 0;
            pGmac->phy_interface = phydev->interface = PHY_INTERFACE_MODE_RMII;
#ifdef FH_GMAC_DISABLE_NG
            phydev->state = PHY_CHANGELINK;
#else
            phydev->state = PHY_UP;
#endif
            phydev->irq = -1;

            pGmac->phydev = phydev;


            /* GMAC_PHY_DEBUG("%s: PHY ID %08x at %d IRQ %d (%s)%s\n",
                 dev->parent.parent.name, phydev->phy_id, i,
                 phydev->irq, "phy001", " active"); */

            break;
        }
    }

    if (pGmac->phydev == RT_NULL)
    {
        rt_kprintf("ERROR: cannot find emac phy\n");
        return -RT_ERROR;
    }

    fh_mdio_set_mii(pGmac);

    return 0;
}


static inline void ethtool_cmd_speed_set(struct ethtool_cmd *ep,
                     unsigned int speed)
{

    ep->speed = (unsigned short)speed;
    ep->speed_hi = (unsigned short)(speed >> 16);
}


int fh_gmac_phy_ethtool_gset(struct phy_device *phydev, struct ethtool_cmd *cmd)
{
    cmd->supported = phydev->supported;

    cmd->advertising = phydev->advertising;

    ethtool_cmd_speed_set(cmd, phydev->speed);
    cmd->duplex = phydev->duplex;
    cmd->port = PORT_MII;
    cmd->phy_address = phydev->addr;
    cmd->transceiver = XCVR_EXTERNAL;
    cmd->autoneg = phydev->autoneg;

    return 0;
}

static inline unsigned int ethtool_cmd_speed(const struct ethtool_cmd *ep)
{
    return (ep->speed_hi << 16) | ep->speed;
}

int fh_gmac_phy_ethtool_sset(struct phy_device *phydev, struct ethtool_cmd *cmd)
{
    struct net_device *dev;
    struct dw_qos *pGmac;
    unsigned int speed = ethtool_cmd_speed(cmd);

    dev = phydev->attached_dev;
    pGmac = dev->parent.user_data;

    if (cmd->phy_address != phydev->addr)
        return -RT_ERROR;

    /* We make sure that we don't pass unsupported
     * values in to the PHY */
    cmd->advertising &= phydev->supported;

    /* Verify the settings we care about. */
    if (cmd->autoneg != AUTONEG_ENABLE && cmd->autoneg != AUTONEG_DISABLE)
        return -EINVAL;

    if (cmd->autoneg == AUTONEG_ENABLE && cmd->advertising == 0)
        return -EINVAL;

    if (cmd->autoneg == AUTONEG_DISABLE &&
        ((speed != SPEED_1000 &&
          speed != SPEED_100 &&
          speed != SPEED_10) ||
         (cmd->duplex != DUPLEX_HALF &&
          cmd->duplex != DUPLEX_FULL)))
        return -EINVAL;

    phydev->autoneg = cmd->autoneg;

    phydev->speed = speed;

    phydev->advertising = cmd->advertising;

    if (AUTONEG_ENABLE == cmd->autoneg)
        phydev->advertising |= ADVERTISED_Autoneg;
    else
        phydev->advertising &= ~ADVERTISED_Autoneg;

    phydev->duplex = cmd->duplex;

    /* Restart the PHY */
    phy_start_aneg(pGmac, phydev);

    return 0;
}
