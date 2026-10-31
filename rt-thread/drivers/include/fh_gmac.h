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

#ifndef __FH_GMAC_H__
#define __FH_GMAC_H__

#include <netif/ethernetif.h>
#include "linux/ethtool.h"
#include "fh_chip.h"
#include "fh_def.h"
#include "fh_pmu.h"
#include "fh_gmac_dma.h"
#include "fh_clock.h"
#include "fh_gmac_phyt.h"
#define REG_GMAC_CONFIG (0x0000)
#define REG_GMAC_FRAME_FILTER (0x0004)
#define REG_GMAC_HASH_HIGH (0x0008)
#define REG_GMAC_HASH_LOW (0x000C)
#define REG_GMAC_GMII_ADDRESS (0x0010)
#define REG_GMAC_GMII_DATA (0x0014)
#define REG_GMAC_FLOW_CTRL (0x0018)
#define REG_GMAC_DEBUG (0x0024)
#define REG_GMAC_MAC_HIGH (0x0040)
#define REG_GMAC_MAC_LOW (0x0044)

/* GMAC-DMA */
#define REG_GMAC_BUS_MODE (0x1000)
#define REG_GMAC_TX_POLL_DEMAND (0x1004)
#define REG_GMAC_RX_POLL_DEMAND (0x1008)
#define REG_GMAC_RX_DESC_ADDR (0x100C)
#define REG_GMAC_TX_DESC_ADDR (0x1010)
#define REG_GMAC_STATUS (0x1014)
#define REG_GMAC_OP_MODE (0x1018)
#define REG_GMAC_INTR_EN (0x101C)
#define REG_GMAC_ERROR_COUNT (0x1020)
#define REG_GMAC_AXI_BUS_MODE (0x1028)
#define REG_GMAC_AXI_STATUS (0x102C)
#define REG_GMAC_CURR_TX_DESC (0x1048)
#define REG_GMAC_CURR_RX_DESC (0x104C)


#ifndef GMAC_TX_RING_SIZE
    #define GMAC_TX_RING_SIZE       32
#endif
#ifndef GMAC_TX_BUFFER_SIZE
    #define GMAC_TX_BUFFER_SIZE     2048
#endif
#ifndef GMAC_RX_BUFFER_SIZE
    #define GMAC_RX_BUFFER_SIZE     2048
#endif
#ifndef GMAC_RX_RING_SIZE
    #define GMAC_RX_RING_SIZE       120
#endif
#ifndef PBUF_MALLOC_LEN
    #define PBUF_MALLOC_LEN         1600
#endif

#define FLOW_OFF 0
#define FLOW_RX     4
#define FLOW_TX     2
#define FLOW_AUTO   (FLOW_TX | FLOW_RX)
#define PAUSE_TIME 0x200

#define INTERNAL_PHY    0x55
#define EXTERNAL_PHY    0xaa
enum tx_dma_irq_status
{
    tx_hard_error         = 1,
    tx_hard_error_bump_tc = 2,
    handle_tx_rx          = 3,
};

enum rx_frame_status
{
    good_frame    = 0,
    discard_frame = 1,
    csum_none     = 2,
    llc_snap      = 4,
};

enum
{
    gmac_gmii_clock_60_100,
    gmac_gmii_clock_100_150,
    gmac_gmii_clock_20_35,
    gmac_gmii_clock_35_60,
    gmac_gmii_clock_150_250,
    gmac_gmii_clock_250_300
};

enum
{
    gmac_interrupt_all  = 0x0001ffff,
    gmac_interrupt_none = 0x0
};

enum
{
    gmac_mii = PHY_INTERFACE_MODE_MII,
    gmac_rmii = PHY_INTERFACE_MODE_RMII,
};


/****************************************************************************
 * ADT section
 *  add definition of user defined Data Type that only be used in this file here
 ***************************************************************************/
typedef struct fh_gmac_stats
{
    /* Transmit errors */
    unsigned long tx_underflow;
    unsigned long tx_carrier;
    unsigned long tx_losscarrier;
    unsigned long tx_heartbeat;
    unsigned long tx_deferred;
    unsigned long tx_vlan;
    unsigned long tx_jabber;
    unsigned long tx_frame_flushed;
    unsigned long tx_payload_error;
    unsigned long tx_ip_header_error;
    /* Receive errors */
    unsigned long rx_desc;
    unsigned long rx_partial;
    unsigned long rx_runt;
    unsigned long rx_toolong;
    unsigned long rx_collision;
    unsigned long rx_crc;
    unsigned long rx_length;
    unsigned long rx_mii;
    unsigned long rx_multicast;
    unsigned long rx_gmac_overflow;
    unsigned long rx_watchdog;
    unsigned long da_rx_filter_fail;
    unsigned long sa_rx_filter_fail;
    unsigned long rx_missed_cntr;
    unsigned long rx_overflow_cntr;
    /* Tx/Rx IRQ errors */
    unsigned long tx_undeflow_irq;
    unsigned long tx_process_stopped_irq;
    unsigned long tx_jabber_irq;
    unsigned long rx_overflow_irq;
    unsigned long rx_buf_unav_irq;
    unsigned long rx_process_stopped_irq;
    unsigned long rx_watchdog_irq;
    unsigned long tx_early_irq;
    unsigned long fatal_bus_error_irq;
    /* Extra info */
    unsigned long threshold;
    unsigned long tx_pkt_n;
    unsigned long rx_pkt_n;
    unsigned long poll_n;
    unsigned long sched_timer_n;
    unsigned long normal_irq_n;
    unsigned long tx_errors;
} fh_gmac_stats_t;

struct fh_gmac_platform_data
{
    unsigned int id;
    unsigned int irq;
    unsigned int base_add;
    unsigned int phy_reset_pin;
    void (*gmac_reset)(void);
    /* phy need to sel func */
    int (*phy_sel)(unsigned int sel);
    /* mac need set mii or rmii to adjust phy, here just bind to phy driver */
    int (*inf_set)(unsigned int inf);
    struct phy_interface_info phy_info[2];
    void *p_cfg_array;
};

#define MAX_ADDR_LEN 6
typedef struct fh_gmac_object
{
    /* inherit from ethernet device */
    struct net_device parent;

    void *p_base_add;
    UINT32 f_base_add;
    UINT8 local_mac_address[MAX_ADDR_LEN];
    unsigned short phy_addr;
    int full_duplex;  /* read only */
    int speed_100m;   /* read only */

    /* Added by PeterJiang */
    UINT8 *rx_ring_original;
    UINT8 *tx_ring_original;
    UINT8 *rx_buffer_original;
    UINT8 *tx_buffer_original;

    UINT8 *rx_buffer;
    UINT8 *tx_buffer;

    Gmac_Rx_DMA_Descriptors *rx_ring;
    Gmac_Tx_DMA_Descriptors *tx_ring;

    unsigned long rx_buffer_dma;
    unsigned long tx_buffer_dma;
    unsigned long rx_ring_dma;
    unsigned long tx_ring_dma;

    unsigned int tx_stop;
    struct pbuf *rx_array[GMAC_RX_RING_SIZE];
    struct rt_semaphore tx_lock;
    struct rt_semaphore rx_lock;
    struct rt_semaphore lock;
    struct rt_semaphore mdio_bus_lock;

    int oldlink;
    int speed;
    int oldduplex;
    unsigned int flow_ctrl;
    unsigned int pause;
    int old_pause_flag;

    int phy_interface;
    struct rt_timer timer;
    fh_gmac_stats_t stats;
    unsigned int rx_cur_desc;
    unsigned int tx_cur_desc;
    unsigned int rx_pbuf_index;
    unsigned int get_frame_no;

    struct phy_device *phydev;

    #define OPEN_IPV4_HEAD_CHECKSUM   (1<<0)
    #define OPEN_TCP_UDP_ICMP_CHECKSUM   (1<<1)
    unsigned int open_flag;
    struct fh_gmac_platform_data *p_plat_data;

    /* cpy from platform */
    int (*phy_sel)(unsigned int sel);
    int (*inf_set)(unsigned int inf);
    struct phy_interface_info *ac_phy_info;
    int phyreset_gpio;
#define FH_GMAC_PM_STAT_NORMAL    0
#define FH_GMAC_PM_STAT_SUSPNED    0x55aaaa55
    rt_uint32_t pm_stat;

} fh_gmac_object_t;

struct fh_gmac_error_status
{
    UINT32 rx_buff_unavail_isr;
    UINT32 rx_buff_unavail_nor;
    UINT32 rx_over_flow;
    UINT32 rx_process_stop;
    UINT32 tx_buff_unavail;
    UINT32 tx_under_flow;
    UINT32 tx_over_write_desc;
    UINT32 other_error;
    UINT32 rtt_malloc_failed;
    UINT32 tx_get_desc_timeout;
};



void fh_gmac_initmac(unsigned char *gmac);
int rt_app_fh_gmac_init(void);

#endif /* FH_GMAC_H_ */
