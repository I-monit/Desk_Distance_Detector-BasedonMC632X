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

#ifndef __FH81_GMAC_PHYT_H__
#define __FH81_GMAC_PHYT_H__

#include "fh_def.h"
#include "mii.h"

#define FH_GMAC_PHY_IP101G  0x02430C54
#define FH_GMAC_PHY_RTL8201 0x001CC816
#define FH_GMAC_PHY_TI83848 0xFFFFFFFF
#define FH_GMAC_PHY_DUMMY   0xE3FFE3FF
#define FH_GMAC_PHY_INTERNAL 0x441400
#define FH_GMAC_PHY_INTERNAL_V2 0x46480000
#define FH_GMAC_PHY_INTERNAL_V3 0x00568101
#define FH_GMAC_PHY_INTERNAL_V4 0x0058a002

struct phy_interface_info
{
    /*internal or external phy*/
    unsigned int phy_sel;
    /*mii phy or rmii phy*/
    unsigned int phy_inf;

    void (*phy_reset)(void);
    int (*sync_mac_spd)(unsigned int spd);
#define PHY_TRIGER_OK   0
#define PHY_TRIGER_ERR  1
    int (*phy_triger)(void);
    int (*ex_sync_mac_spd)(unsigned int spd, void *pri);
    void (*brd_link_up_cb)(void *pri);
    void (*brd_link_down_cb)(void *pri);
    void (*dll_set)(unsigned int tx_val, unsigned int rx_val);
    void (*polarity_set)(unsigned int tx_pol, unsigned int rx_pol);
    void (*power_down_cb)(void *pri);
};

#define W_PHY   0x01
#define M_PHY   0x02
#define ACTION_MASK 0xff
struct phy_reg_cfg_list
{
    unsigned int r_w;
    unsigned int reg_add;
    unsigned int reg_val;
    unsigned int reg_mask;
};
#define MAX_PHY_REG_SIZE    64
#define DLL_SETTING_ENABLE  0x55aaaa55
struct phy_reg_cfg
{
    unsigned int id;
    struct phy_reg_cfg_list list[MAX_PHY_REG_SIZE];
    /*phy interface bind here better, phy support set here*/
    unsigned int inf_sup;
};

struct s_train_val
{
    char *name;
    unsigned int src_add;
    unsigned int src_mask;
    unsigned int src_valid_index;
    unsigned char *dst_base_name;
    unsigned int dst_add;
    unsigned int dst_valid_index;
    unsigned int *bind_train_array;
    unsigned int bind_train_size;
    int usr_train_offset;
};

#define PHY_INIT_TIMEOUT    100000
#define PHY_STATE_TIME      1
#define PHY_FORCE_TIMEOUT   10
#define PHY_AN_TIMEOUT      10

#define PHY_MAX_ADDR    32

/* Interface Mode definitions */
typedef enum {
    PHY_INTERFACE_MODE_MII,
    PHY_INTERFACE_MODE_GMII,
    PHY_INTERFACE_MODE_SGMII,
    PHY_INTERFACE_MODE_TBI,
    PHY_INTERFACE_MODE_RMII,
    PHY_INTERFACE_MODE_RGMII,
    PHY_INTERFACE_MODE_RGMII_ID,
    PHY_INTERFACE_MODE_RGMII_RXID,
    PHY_INTERFACE_MODE_RGMII_TXID,
    PHY_INTERFACE_MODE_RTBI
} phy_interface_t;

enum phy_state
{
    PHY_DOWN=0,
    PHY_STARTING,
    PHY_READY,
    PHY_PENDING,
    PHY_UP,
    PHY_AN,
    PHY_RUNNING,
    PHY_NOLINK,
    PHY_FORCING,
    PHY_CHANGELINK,
    PHY_HALTED,
    PHY_RESUMING
};


typedef union {
    struct
    {
        UINT32 reserved_6_0 : 7;
        UINT32 collision_test : 1;
        UINT32 duplex_mode : 1;
        UINT32 restart_auto_negotiate : 1;
        UINT32 isolate : 1;
        UINT32 power_down : 1;
        UINT32 auto_negotiate_enable : 1;
        UINT32 speed_select : 1;
        UINT32 loopback : 1;
        UINT32 reset : 1;
        UINT32 reserved_31_16 : 16;
    } bit;
    UINT32 dw;
} Reg_Phyt_Basic_Ctrl;

typedef union {
    struct
    {
        UINT32 extended_capabilities : 1;
        UINT32 jabber_detect : 1;
        UINT32 link_status : 1;
        UINT32 auto_negotiate_ability : 1;
        UINT32 remote_fault : 1;
        UINT32 auto_negotiate_complete : 1;
        UINT32 reserved_10_6 : 5;
        UINT32 base_t_half_duplex_10 : 1;
        UINT32 base_t_full_duplex_10 : 1;
        UINT32 base_tx_half_duplex_100 : 1;
        UINT32 base_tx_full_duplex_100 : 1;
        UINT32 base_t_4 : 1;
        UINT32 reserved_31_16 : 16;
    } bit;
    UINT32 dw;
} Reg_Phyt_Basic_Status;

typedef union {
    struct
    {
        UINT32 scramble_disable : 1;
        UINT32 reserved_1 : 1;
        UINT32 speed_indication : 3;
        UINT32 reserved_5 : 1;
        UINT32 enable_4b5b : 1;
        UINT32 gpo : 3;
        UINT32 reserved_11_10 : 2;
        UINT32 auto_done : 1;
        UINT32 reserved_31_13 : 19;
    } bit;
    UINT32 dw;
} Reg_Phyt_Special_Status;


struct phy_device
{

    unsigned int phy_id;

    enum phy_state state;

    unsigned int dev_flags;

    phy_interface_t interface;

    /* Bus address of the PHY (0-31) */
    int addr;

    /*
     * forced speed & duplex (no autoneg)
     * partner speed & duplex & pause (autoneg)
     */
    int speed;
    int duplex;
    int pause;
    int asym_pause;

    /* The most recently read link state */
    int link;

    /* Enabled Interrupts */
    unsigned int interrupts;

    /* Union of PHY and Attached devices' supported modes */
    /* See mii.h for more info */
    unsigned int supported;
    unsigned int advertising;

    int autoneg;

    int link_timeout;

    /*
     * Interrupt number for this PHY
     * -1 means no interrupt
     */
    int irq;

    /* private data pointer */
    /* For use by PHYs to maintain extra state */
    void *priv;

    struct rt_semaphore lock;
    struct rt_timer timer;

    struct net_device *attached_dev;
};

void phy_state_machine(void *param);
int fh_mdio_register(struct net_device *dev);
int fh_gmac_phy_ethtool_sset(struct phy_device *phydev, struct ethtool_cmd *cmd);
int fh_gmac_phy_ethtool_gset(struct phy_device *phydev, struct ethtool_cmd *cmd);

#endif /* FH81_GMAC_PHYT_H_ */
