/**
 * Copyright (c) 2015-2019 Shanghai Fullhan Microelectronics Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 *
 * Change Logs:
 * Date           Author       Notes
 * 2019-02-24     zhangy       the first version
 */

#include "fh_gmac.h"
#include "fh_gmac_phyt.h"
#include "fh_pmu.h"
#include "pinctrl.h"
#include "gpio.h"

#ifdef GMAC_PHY_DEBUG
#define GMAC_PHY_DEBUG(fmt, args...) rt_kprintf(fmt, ##args);
#else
#define GMAC_PHY_DEBUG(fmt, args...)
#endif
int get_phy_id(fh_gmac_object_t *pGmac, int addr, unsigned int *phy_id);

#ifndef PHY_MAX_ADDR
#define PHY_MAX_ADDR    32
#endif

struct mdio_pin_mux_ref
{
    char *old;
    char *restore;
};

rt_uint32_t phy_support_list[] = {
    FH_GMAC_PHY_IP101G,
    FH_GMAC_PHY_RTL8201,
    FH_GMAC_PHY_TI83848,
    FH_GMAC_PHY_INTERNAL,
    FH_GMAC_PHY_INTERNAL_V2,
    FH_GMAC_PHY_INTERNAL_V3,
    FH_GMAC_PHY_INTERNAL_V4,
};

struct mdio_pin_mux_ref _mdio_pin_ref_obj[] = {
    {.old = "MAC_MDC", .restore = 0},
    {.old = "MAC_MDIO", .restore = 0},
    {.old = "MAC_RMII_CLK", .restore = 0},
    {.old = "EX_PHY_RESET_PIN", .restore = 0},
};

struct phy_setting {
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

static void fh_gmac_set_phy_register(fh_gmac_object_t* p, int reg, int data)
{
    SET_REG(p->f_base_add + REG_GMAC_GMII_DATA, data);

    SET_REG(p->f_base_add + REG_GMAC_GMII_ADDRESS,
            0x1 << 1 | p->phy_addr << 11 | gmac_gmii_clock_100_150 << 2 |
                reg << 6 | 0x1);

    while (GET_REG(p->f_base_add + REG_GMAC_GMII_ADDRESS) & 0x1)
    {
    }
/*
    rt_kprintf("phy write: phy id: %d, reg: 0x%x, data: 0x%x\n",
            p->phy_addr, reg, data);
*/
}

static int fh_gmac_get_phy_register(fh_gmac_object_t* p, int reg)
{
    SET_REG(p->f_base_add + REG_GMAC_GMII_ADDRESS,
            p->phy_addr << 11 | gmac_gmii_clock_100_150 << 2 | reg << 6 | 0x1);
    while (GET_REG(p->f_base_add + REG_GMAC_GMII_ADDRESS) & 0x1)
    {
    }
/*
    rt_kprintf("phy read: phy id: %d, reg: 0x%x, data: 0x%x\n",
            p->phy_addr, reg, GET_REG(p->f_base_add + REG_GMAC_GMII_DATA));
*/
    return GET_REG(p->f_base_add + REG_GMAC_GMII_DATA);
}

static inline int phy_read(struct phy_device *phydev, unsigned int regnum)
{
    fh_gmac_object_t *pGmac = phydev->attached_dev->parent.user_data;
    return fh_gmac_get_phy_register(pGmac, regnum);
}

static inline int phy_write(struct phy_device *phydev, unsigned int regnum, unsigned short val)
{
    fh_gmac_object_t *pGmac = phydev->attached_dev->parent.user_data;
    fh_gmac_set_phy_register(pGmac, regnum, val);
    return 0;
}

int check_white_list(rt_uint32_t phy_id, rt_uint32_t *list, rt_uint32_t size)
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
int scan_valid_phy_id(fh_gmac_object_t *gmac, rt_uint32_t *white_list, rt_uint32_t list_size)
{
    unsigned int phy_id = 0;
    int i;
    int ret;

    for (i = 0; i < PHY_MAX_ADDR; i++)
    {
        ret = get_phy_id(gmac, i, &phy_id);
        if (ret == 0 && (phy_id & 0x1fffffff) != 0x1fffffff)
        {
            if (check_white_list(phy_id, white_list, list_size))
                continue;
            break;
        }
    }
    if (i == PHY_MAX_ADDR)
        return -1;

    return 0;
}


int auto_find_phy(fh_gmac_object_t *gmac)
{
    int i;
    int ret = -1;
    struct fh_gmac_platform_data *p_info;
    struct phy_interface_info *p_phy_info;

    p_info = gmac->p_plat_data;
    for (i = 0, p_phy_info = p_info->phy_info;
            i < ARRAY_SIZE(p_info->phy_info); i++)
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
            /* 4:mac interface set mii or rmii */
            if (p_info->inf_set)
                p_info->inf_set(p_phy_info[i].phy_inf);
            /* find phy id.. */
            ret = scan_valid_phy_id(gmac, phy_support_list,
                        ARRAY_SIZE(phy_support_list));
            if (!ret)
                break;
        }
    }

    if (!ret)
    {
        /* find valid phy...just parse */
        gmac->ac_phy_info = &p_phy_info[i];
        gmac->phy_sel = p_info->phy_sel;
        gmac->inf_set = p_info->inf_set;
        gmac->phy_interface = p_phy_info[i].phy_inf;
        rt_kprintf("auto find phy info :: %s : %s\n",
            p_phy_info[i].phy_sel == EXTERNAL_PHY ?
            "external phy" : "internal phy",
            p_phy_info[i].phy_inf == PHY_INTERFACE_MODE_MII ?
            "mii" : "rmii");
        /* sync with net core later... */
        return 0;
    }

    return -1;
}

int fh_mdio_set_mii(fh_gmac_object_t* pGmac)
{
    struct phy_reg_cfg *p_cfg;
    struct phy_reg_cfg_list *p_cfg_data;

    RT_UNUSED unsigned int ret;

    p_cfg = (struct phy_reg_cfg *)pGmac->p_plat_data->p_cfg_array;

    for (; p_cfg->id != 0; p_cfg++)
    {
        if (p_cfg->id == pGmac->phydev->phy_id)
        {
            /*pr_err("find phy id [%08x] reg para\n",p_cfg->id);*/
            for (p_cfg_data = p_cfg->list;
            p_cfg_data->r_w != 0; p_cfg_data++)
            {
                if ((p_cfg_data->r_w & ACTION_MASK) == W_PHY)
                {
                    fh_gmac_set_phy_register(pGmac,
                            p_cfg_data->reg_add,
                            p_cfg_data->reg_val);
                }
                else if ((p_cfg_data->r_w & ACTION_MASK) == M_PHY)
                {
                    ret = fh_gmac_get_phy_register(pGmac, p_cfg_data->reg_add);
                    ret &= ~(p_cfg_data->reg_mask);
                    ret |= (p_cfg_data->reg_val &
                            p_cfg_data->reg_mask);
                    fh_gmac_set_phy_register(pGmac,
                            p_cfg_data->reg_add, ret);
                }
            }
            /*find one match;return ok*/
            return 0;
        }
    }

    return 0;
}

void phy_print_status(struct phy_device *phydev)
{
    rt_kprintf("PHY: - Auto Negotiation is %s, Link is %s",
            phydev->autoneg ? "ON" : "OFF",
            phydev->link ? "Up" : "Down");
    if (phydev->link)
        rt_kprintf(" - %d/%s", phydev->speed,
                DUPLEX_FULL == phydev->duplex ?
                "Full" : "Half");

    rt_kprintf("\n");
}

static void fh_set_rmii_speed(fh_gmac_object_t *pGmac, int speed)
{
    if (pGmac->ac_phy_info->sync_mac_spd)
        pGmac->ac_phy_info->sync_mac_spd(speed);
}

void GMAC_FlowCtrl(fh_gmac_object_t* pGmac, unsigned int duplex,
                   unsigned int fc, unsigned int pause_time)
{
    unsigned int flow = fc;

    if (duplex)
        flow |= (pause_time << 16);
    SET_REG(pGmac->f_base_add + REG_GMAC_FLOW_CTRL, flow);
}

static void fh_gmac_adjust_link(struct net_device *dev)
{
    fh_gmac_object_t *pGmac = dev->parent.user_data;
    struct phy_device *phydev = pGmac->phydev;

    int new_state = 0;

    if (phydev == NULL)
        return;

    rt_enter_critical();
    if (phydev->link)
    {
        unsigned int ctrl = GET_REG(pGmac->f_base_add + REG_GMAC_CONFIG);

        /* Now we make sure that we can be in full duplex mode.
         * If not, we operate in half-duplex mode. */
        if (phydev->duplex != pGmac->oldduplex)
        {
            new_state = 1;
            if (!(phydev->duplex))
                ctrl &= ~0x800;
            else
                ctrl |= 0x800;
            pGmac->oldduplex = phydev->duplex;
        }
        /* Flow Control operation */
        if (phydev->pause)
        {
        /*
            unsigned int fc = pGmac->flow_ctrl, pause_time = pGmac->pause;
            GMAC_FlowCtrl(pGmac, phydev->duplex, fc, pause_time);
        */
            if (pGmac->old_pause_flag != phydev->pause)
            {
                pGmac->old_pause_flag = phydev->pause;
                unsigned int fc = pGmac->flow_ctrl, pause_time = pGmac->pause;
                GMAC_FlowCtrl(pGmac, phydev->duplex, fc, pause_time);
            }
        }

        if (phydev->speed != pGmac->speed)
        {
            new_state = 1;
            switch (phydev->speed)
            {
            case 100:
                ctrl |= 0x4000;
                fh_set_rmii_speed(pGmac, phydev->speed);
                break;
            case 10:
                ctrl &= ~0x4000;
                fh_set_rmii_speed(pGmac, phydev->speed);
                break;
            default:
                rt_kprintf("%s: Speed (%d) is not 10"
                   " or 100!\n", dev->parent.parent.name, phydev->speed);
                break;
            }

            pGmac->speed = phydev->speed;
        }
        SET_REG(pGmac->f_base_add + REG_GMAC_CONFIG, ctrl);
        if (!pGmac->oldlink)
        {
            new_state = 1;
            pGmac->oldlink = 1;
        }
    }
    else if (pGmac->oldlink)
    {
        new_state = 1;
        pGmac->oldlink = 0;
        pGmac->speed = 0;
        pGmac->oldduplex = -1;
    }

    if (new_state)
    {
        phy_print_status(phydev);
        net_device_linkchange(dev, phydev->link);
    }

    rt_exit_critical();
}


static int genphy_setup_forced(struct phy_device *phydev)
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

    err = phy_write(phydev, MII_BMCR, ctl);

    return err;
}


int get_phy_id(fh_gmac_object_t *pGmac, int addr, unsigned int *phy_id)
{
    int phy_reg;

    /* Grab the bits from PHYIR1, and put them
     * in the upper half */
    phy_reg = fh_gmac_get_phy_register(pGmac, MII_PHYSID1);

    if (phy_reg < 0)
        return -EIO;

    *phy_id = (phy_reg & 0xffff) << 16;

    /* Grab the bits from PHYIR2, and put them in the lower half */
    phy_reg = fh_gmac_get_phy_register(pGmac, MII_PHYSID2);

    if (phy_reg < 0)
        return -EIO;

    *phy_id |= (phy_reg & 0xffff);

    return 0;
}

int genphy_update_link(struct phy_device *phydev)
{
    int status;

    /* Do a fake read */
    status = phy_read(phydev, MII_BMSR);

    if (status < 0)
        return status;

    /* Read link and autonegotiation status */
    status = phy_read(phydev, MII_BMSR);

    if (status < 0)
        return status;

    if ((status & BMSR_LSTATUS) == 0)
        phydev->link = 0;
    else
        phydev->link = 1;

    return 0;
}

int gengenphy_read_status(struct phy_device *phydev)
{
    int adv;
    int err;
    int lpa;
    int lpagb = 0;

    /* Update the link, but return if there
     * was an error */
    err = genphy_update_link(phydev);
    if (err)
        return err;

    if (AUTONEG_ENABLE == phydev->autoneg) {

        lpa = phy_read(phydev, MII_LPA);

        if (lpa < 0)
            return lpa;

        adv = phy_read(phydev, MII_ADVERTISE);

        if (adv < 0)
            return adv;

        lpa &= adv;

        phydev->speed = SPEED_10;
        phydev->duplex = DUPLEX_HALF;
        phydev->pause = phydev->asym_pause = 0;

        if (lpagb & (LPA_1000FULL | LPA_1000HALF)) {
            phydev->speed = SPEED_1000;

            if (lpagb & LPA_1000FULL)
                phydev->duplex = DUPLEX_FULL;
        } else if (lpa & (LPA_100FULL | LPA_100HALF)) {
            phydev->speed = SPEED_100;

            if (lpa & LPA_100FULL)
                phydev->duplex = DUPLEX_FULL;
        } else
            if (lpa & LPA_10FULL)
                phydev->duplex = DUPLEX_FULL;

        if (phydev->duplex == DUPLEX_FULL){
            phydev->pause = lpa & LPA_PAUSE_CAP ? 1 : 0;
            phydev->asym_pause = lpa & LPA_PAUSE_ASYM ? 1 : 0;
        }
    } else {
        int bmcr = phy_read(phydev, MII_BMCR);
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

static inline int phy_aneg_done(struct phy_device *phydev)
{
    int retval;

    retval = phy_read(phydev, MII_BMSR);

    return (retval < 0) ? retval : (retval & BMSR_ANEGCOMPLETE);
}

static inline int phy_find_valid(int idx, unsigned int features)
{
    while (idx < MAX_NUM_SETTINGS && !(settings[idx].setting & features))
        idx++;

    return idx < MAX_NUM_SETTINGS ? idx : MAX_NUM_SETTINGS - 1;
}

int genphy_read_status(struct phy_device *phydev)
{
    int adv;
    int err;
    int lpa;
    int lpagb = 0;

    /* Update the link, but return if there
     * was an error */
    err = genphy_update_link(phydev);
    if (err)
        return err;

    if (AUTONEG_ENABLE == phydev->autoneg) {
        if (phydev->supported & (SUPPORTED_1000baseT_Half
                    | SUPPORTED_1000baseT_Full)) {
            lpagb = phy_read(phydev, MII_STAT1000);

            if (lpagb < 0)
                return lpagb;

            adv = phy_read(phydev, MII_CTRL1000);

            if (adv < 0)
                return adv;

            lpagb &= adv << 2;
        }

        lpa = phy_read(phydev, MII_LPA);

        if (lpa < 0)
            return lpa;

        adv = phy_read(phydev, MII_ADVERTISE);

        if (adv < 0)
            return adv;

        lpa &= adv;

        phydev->speed = SPEED_10;
        phydev->duplex = DUPLEX_HALF;
        phydev->pause = phydev->asym_pause = 0;

        if (lpagb & (LPA_1000FULL | LPA_1000HALF)) {
            phydev->speed = SPEED_1000;

            if (lpagb & LPA_1000FULL)
                phydev->duplex = DUPLEX_FULL;
        } else if (lpa & (LPA_100FULL | LPA_100HALF)) {
            phydev->speed = SPEED_100;

            if (lpa & LPA_100FULL)
                phydev->duplex = DUPLEX_FULL;
        } else
            if (lpa & LPA_10FULL)
                phydev->duplex = DUPLEX_FULL;

        if (phydev->duplex == DUPLEX_FULL){
            phydev->pause = lpa & LPA_PAUSE_CAP ? 1 : 0;
            phydev->asym_pause = lpa & LPA_PAUSE_ASYM ? 1 : 0;
        }
    } else {
        int bmcr = phy_read(phydev, MII_BMCR);
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

static inline int phy_find_setting(int speed, int duplex)
{
    int idx = 0;

    while (idx < ARRAY_SIZE(settings) &&
            (settings[idx].speed != speed ||
            settings[idx].duplex != duplex))
        idx++;

    return idx < MAX_NUM_SETTINGS ? idx : MAX_NUM_SETTINGS - 1;
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

static void phy_error(struct phy_device *phydev)
{
    phydev->state = PHY_HALTED;
}


static int genphy_config_advert(struct phy_device *phydev)
{
    unsigned int advertise;
    int oldadv, adv;
    int err, changed = 0;

    /* Only allow advertising what
     * this PHY supports */
    phydev->advertising &= phydev->supported;
    advertise = phydev->advertising;

    /* Setup standard advertisement */
    oldadv = adv = phy_read(phydev, MII_ADVERTISE);

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

    if (adv != oldadv) {
        err = phy_write(phydev, MII_ADVERTISE, adv);

        if (err < 0)
            return err;
        changed = 1;
    }

    return changed;
}

int genphy_restart_aneg(struct phy_device *phydev)
{
    int ctl;

    ctl = phy_read(phydev, MII_BMCR);

    if (ctl < 0)
        return ctl;

    ctl |= (BMCR_ANENABLE | BMCR_ANRESTART);

    /* Don't isolate the PHY if we're negotiating */
    ctl &= ~(BMCR_ISOLATE);

    ctl = phy_write(phydev, MII_BMCR, ctl);

    return ctl;
}

int genphy_config_aneg(struct phy_device *phydev)
{
    int result;

    if (AUTONEG_ENABLE != phydev->autoneg)
        return genphy_setup_forced(phydev);

    result = genphy_config_advert(phydev);

    if (result < 0) /* error */
        return result;

    if (result == 0) {
        /* Advertisement hasn't changed, but maybe aneg was never on to
         * begin with?  Or maybe phy was isolated? */
        int ctl = phy_read(phydev, MII_BMCR);

        if (ctl < 0)
            return ctl;

        if (!(ctl & BMCR_ANENABLE) || (ctl & BMCR_ISOLATE))
            result = 1; /* do restart aneg */
    }

    /* Only restart aneg if we are advertising something different
     * than we were before.  */
    if (result > 0)
        result = genphy_restart_aneg(phydev);

    return result;
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

int phy_start_aneg(struct phy_device *phydev)
{
    int err;

    if (AUTONEG_DISABLE == phydev->autoneg)
        phy_sanitize_settings(phydev);

    err = genphy_config_aneg(phydev);

    if (err < 0)
        return err;

    if (phydev->state != PHY_HALTED) {
        if (AUTONEG_ENABLE == phydev->autoneg) {
            phydev->state = PHY_AN;
            phydev->link_timeout = PHY_AN_TIMEOUT;
        } else {
            phydev->state = PHY_FORCING;
            phydev->link_timeout = PHY_FORCE_TIMEOUT;
        }
    }

    return err;
}

/**
 * phy_state_machine - Handle the state machine
 * @work: work_struct that describes the work to be done
 */
void phy_state_machine(void* param)
{
    fh_gmac_object_t *gmac = param;
    struct phy_device *phydev = gmac->phydev;
    int needs_aneg = 0;
    int err = 0;

    rt_sem_take(&gmac->mdio_bus_lock, RT_WAITING_FOREVER);

    switch(phydev->state) {
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
            err = genphy_read_status(phydev);

            if (err < 0)
                break;

            /* If the link is down, give up on
             * negotiation for now */
            if (!phydev->link) {
                phydev->state = PHY_NOLINK;
                //netif_carrier_off(phydev->attached_dev);
                fh_gmac_adjust_link(phydev->attached_dev);
                break;
            }

            /* Check if negotiation is done.  Break
             * if there's an error */
            err = phy_aneg_done(phydev);
            if (err < 0)
                break;

            /* If AN is done, we're running */
            if (err > 0) {
                phydev->state = PHY_RUNNING;
                //netif_carrier_on(phydev->attached_dev);
                fh_gmac_adjust_link(phydev->attached_dev);

            } else if (0 == phydev->link_timeout--) {
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
            err = genphy_read_status(phydev);

            if (err)
                break;

            if (phydev->link) {
                phydev->state = PHY_RUNNING;
                //netif_carrier_on(phydev->attached_dev);
                fh_gmac_adjust_link(phydev->attached_dev);
            }
            break;
        case PHY_FORCING:
            err = genphy_update_link(phydev);

            if (err)
                break;

            if (phydev->link) {
                phydev->state = PHY_RUNNING;
                //netif_carrier_on(phydev->attached_dev);
            } else {
                if (0 == phydev->link_timeout--) {
                    phy_force_reduction(phydev);
                    needs_aneg = 1;
                }
            }

            fh_gmac_adjust_link(phydev->attached_dev);
            break;
        case PHY_RUNNING:
            /* Only register a CHANGE if we are
             * polling */
            if (-1 == phydev->irq)
                phydev->state = PHY_CHANGELINK;
            break;
        case PHY_CHANGELINK:
            err = genphy_read_status(phydev);

            if (err)
                break;

            if (phydev->link) {
                phydev->state = PHY_RUNNING;
                //netif_carrier_on(phydev->attached_dev);
            } else {
                phydev->state = PHY_NOLINK;
                //netif_carrier_off(phydev->attached_dev);
            }

            fh_gmac_adjust_link(phydev->attached_dev);

            break;
        case PHY_HALTED:
            if (phydev->link) {
                phydev->link = 0;
                //netif_carrier_off(phydev->attached_dev);
                fh_gmac_adjust_link(phydev->attached_dev);
            }
            break;
        case PHY_RESUMING:
            if (AUTONEG_ENABLE == phydev->autoneg) {
                err = phy_aneg_done(phydev);
                if (err < 0)
                    break;

                /* err > 0 if AN is done.
                 * Otherwise, it's 0, and we're
                 * still waiting for AN */
                if (err > 0) {
                    err = genphy_read_status(phydev);
                    if (err)
                        break;

                    if (phydev->link) {
                        phydev->state = PHY_RUNNING;
                        //netif_carrier_on(phydev->attached_dev);
                    } else
                        phydev->state = PHY_NOLINK;
                    fh_gmac_adjust_link(phydev->attached_dev);
                } else {
                    phydev->state = PHY_AN;
                    phydev->link_timeout = PHY_AN_TIMEOUT;
                }
            } else {
                err = genphy_read_status(phydev);
                if (err)
                    break;

                if (phydev->link) {
                    phydev->state = PHY_RUNNING;
                    //netif_carrier_on(phydev->attached_dev);
                } else
                    phydev->state = PHY_NOLINK;
                fh_gmac_adjust_link(phydev->attached_dev);
            }
            break;
    }

    rt_sem_release(&gmac->mdio_bus_lock);

    if (needs_aneg)
        err = phy_start_aneg(phydev);

    if (err < 0)
        phy_error(phydev);
}

static struct phy_device* phy_device_create(fh_gmac_object_t *pGmac,
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

struct phy_device * get_phy_device(fh_gmac_object_t *pGmac, int addr)
{
    struct phy_device *dev = NULL;
    unsigned int phy_id;
    int r;

    r = get_phy_id(pGmac, addr, &phy_id);
    if (r)
        return RT_NULL;

    /* If the phy_id is mostly Fs, there is no device there */
    if ((phy_id & 0x1fffffff) == 0x1fffffff)
        return NULL;

    dev = phy_device_create(pGmac, addr, phy_id);

    return dev;
}

struct phy_device *mdio_scan(fh_gmac_object_t *pGmac, int addr)
{
    struct phy_device *phydev;

    phydev = get_phy_device(pGmac, addr);
    if (phydev == RT_NULL)
        return phydev;

    return phydev;
}

int fh_mdio_register(struct net_device *dev)
{
    int i;
    fh_gmac_object_t *pGmac = dev->parent.user_data;

    for (i = 0; i < PHY_MAX_ADDR; i++)
    {
        struct phy_device *phydev;

        pGmac->phy_addr = i;
        phydev = mdio_scan(pGmac, i);

        if(phydev)
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


            GMAC_PHY_DEBUG("%s: PHY ID %08x at %d IRQ %d (%s)%s\n",
                dev->parent.parent.name, phydev->phy_id, i,
                phydev->irq, "phy001", " active");

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

static inline unsigned int ethtool_cmd_speed(const struct ethtool_cmd *ep)
{
    return (ep->speed_hi << 16) | ep->speed;
}

int fh_gmac_phy_ethtool_sset(struct phy_device *phydev, struct ethtool_cmd *cmd)
{
    unsigned int speed = ethtool_cmd_speed(cmd);

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
    phy_start_aneg(phydev);

    return 0;
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

