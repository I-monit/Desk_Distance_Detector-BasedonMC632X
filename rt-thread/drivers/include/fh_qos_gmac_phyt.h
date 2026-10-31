#ifndef FH_QOS_GMAC_PHYT_H_
#define FH_QOS_GMAC_PHYT_H_

#include "fh_def.h"
#include "mii.h"

#define FH_GMAC_PHY_IP101G  0x02430C54
#define FH_GMAC_PHY_RTL8201 0x001CC816
#define FH_GMAC_PHY_TI83848 0xFFFFFFFF
#define FH_GMAC_PHY_INTERNAL 0x441400
#define FH_GMAC_PHY_DUMMY   0xE3FFE3FF
#define FH_GMAC_PHY_INTERNAL_V2 0x46480000
#define FH_GMAC_PHY_RTL8211F 0x001cc916
#define FH_GMAC_PHY_MAE0621 0x7b744411
#define FH_GMAC_PHY_JL2101 0x937c4032

#define PHY_INIT_TIMEOUT    100000
#define PHY_STATE_TIME      1
#define PHY_FORCE_TIMEOUT   10
#define PHY_AN_TIMEOUT      10

struct phy_interface_info
{
    /*internal or external phy*/
    u32_t phy_sel;
    /*mii phy or rmii phy*/
    u32_t phy_inf;

    void (*phy_reset)(void);
    int (*sync_mac_spd)(unsigned int spd);
#define PHY_TRIGER_OK   0
#define PHY_TRIGER_ERR  1
    int (*phy_triger)(void);
    int (*ex_sync_mac_spd)(unsigned int spd, void *pri);

    void (*brd_link_up_cb)(void *pri);
    void (*brd_link_down_cb)(void *pri);
};

/* Duplex, half or full. */
#define DUPLEX_HALF     0x00
#define DUPLEX_FULL     0x01
#define DUPLEX_UNKNOWN      0xff

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

/* Interface Mode definitions */
typedef enum
{
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

#define W_PHY   0x01
#define M_PHY   0x02
#define ACTION_MASK 0xff
#define MII_BUS_ID_SIZE 61

struct phy_reg_cfg_list
{
    u32_t r_w;
    u32_t reg_add;
    u32_t reg_val;
    u32_t reg_mask;
};
#define MAX_PHY_REG_SIZE    64
#define PHY_ID_FMT "%s:%02x"
struct phy_reg_cfg
{
    u32_t id;
    struct phy_reg_cfg_list list[MAX_PHY_REG_SIZE];
    /*phy interface bind here better, phy support set here*/
    u32_t inf_sup;
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

#endif
