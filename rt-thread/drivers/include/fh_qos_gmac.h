#ifndef FH_QOS_GMAC_H_
#define FH_QOS_GMAC_H_
/*****************************************************************************
 *  Include Section
 *  add all #include here
 *****************************************************************************/
#include <netif/ethernetif.h>
#include "linux/ethtool.h"
#include "fh_chip.h"
#include "fh_def.h"
#include "fh_pmu.h"
#include "fh_clock.h"
#include "fh_qos_gmac_phyt.h"

/*****************************************************************************
 * Define section
 * add all #define here
 *****************************************************************************/
#define __qos_raw_writeb(v, a) (*(volatile unsigned char *)(a) = (v))
#define __qos_raw_writew(v, a) (*(volatile unsigned short *)(a) = (v))
#define __qos_raw_writel(v, a) (*(volatile unsigned int *)(a) = (v))
#define __qos_raw_readb(a) (*(volatile unsigned char *)(a))
#define __qos_raw_readw(a) (*(volatile unsigned short *)(a))
#define __qos_raw_readl(a) (*(volatile unsigned int *)(a))

#define dw_readl(dw, name) \
    __qos_raw_readl(&(((struct dw_qos_regs_map *)dw->regs)->name))
#define dw_writel(dw, name, val) \
    __qos_raw_writel((val), &(((struct dw_qos_regs_map *)dw->regs)->name))
#define dw_readw(dw, name) \
    __qos_raw_readw(&(((struct dw_qos_regs_map *)dw->regs)->name))
#define dw_writew(dw, name, val) \
    __qos_raw_writew((val), &(((struct dw_qos_regs_map *)dw->regs)->name))

#define DWCQOS_FOR_EACH_QUEUE(max_queues, queue_num)            \
        for (queue_num = 0; queue_num < max_queues; queue_num++)

#ifndef BIT
#define BIT(x)  (1 << (x))
#endif

#ifndef CACHE_LINE_SIZE
#define CACHE_LINE_SIZE     32
#endif

#define DWCQOS_MAC_RX_POS       BIT(0)
#define DWCQOS_MAC_TX_POS       BIT(1)

#define DWCQOS_DMA_MODE_SWR      BIT(0)
#define DWCQOS_DMA_RDES3_OWN     BIT(31)
#define DWCQOS_DMA_RDES3_INTE    BIT(30)
#define DWCQOS_DMA_RDES3_BUF2V   BIT(25)
#define DWCQOS_DMA_RDES3_BUF1V   BIT(24)

#define DWCQOS_DMA_CH_CTRL_PBLX8       BIT(16)
#define DWCQOS_DMA_CH_TX_TSE           BIT(12)

#define DWCQOS_MTL_TXQ_TXQEN            BIT(3)
#define DWCQOS_MTL_TXQ_TSF              BIT(1)
#define DWCQOS_MTL_TXQ_TTC512           0x00000070
#define DWCQOS_MTL_RXQ_FUP              BIT(3)
#define DWCQOS_MTL_RXQ_FEP              BIT(4)
#define DWCQOS_MTL_RXQ_RSF              BIT(5)

#define DWCQOS_MTL_TXQ_DEBUG_TRCSTS_READ_STATE  BIT(1)
#define DWCQOS_MTL_TXQ_DEBUG_TRCSTS_MASK    (0x3 << 1)
/* txq empty */
#define DWCQOS_MTL_TXQ_DEBUG_TXQSTS     BIT(4)


#define DWCQOS_MTL_RXQ_DEBUG_ACTIVE_PACKET_POS_MASK     (0x3fff < 16)
#define DWCQOS_MTL_RXQ_DEBUG_RXQSTS_MASK        (0x3 << 4)
#define DWCQOS_MTL_RXQ_DEBUG_RXQSTS_EMPTY   (0)


#define DWCQOS_DMA_TDES2_IOC     BIT(31)
#define DWCQOS_DMA_TDES3_OWN     BIT(31)
#define DWCQOS_DMA_TDES3_FD      BIT(29)
#define DWCQOS_DMA_TDES3_LD      BIT(28)
#define GMAC_TIMEOUT_SEND       (1000)      /* 10ms */

#define DWCQOS_DMA_CH_IE_NIE           BIT(15)
#define DWCQOS_DMA_CH_IE_AIE           BIT(14)
#define DWCQOS_DMA_CH_IE_RIE           BIT(6)
#define DWCQOS_DMA_CH_IE_TIE           BIT(0)
#define DWCQOS_DMA_CH_IE_FBEE          BIT(12)
#define DWCQOS_DMA_CH_IE_RBUE          BIT(7)

#define DWCQOS_DMA_IS_MTLIS             BIT(16)
#define DWCQOS_DMA_IS_MACIS             BIT(17)
#define DWCQOS_DMA_CH_IS_TI            BIT(0)
#define DWCQOS_DMA_CH_IS_RI            BIT(6)


#define DWCQOS_MAC_HW_FEATURE1_TSOEN    BIT(18)
#define DWCQOS_MAC_HW_FEATURE0_RXCOESEL BIT(16)
#define DWCQOS_MAC_HW_FEATURE0_TXCOESEL BIT(14)


#define DWCQOS_HASH_TABLE_SIZE 64
#define DWCQOS_MAC_IS_LPI_INT           BIT(5)
#define DWCQOS_MAC_IS_MMC_INT           BIT(8)
#define DWCQOS_MAC_IS_RXIPIS_INT        BIT(11)
#define DWCQOS_MAC_RXQ_EN               BIT(1)
#define DWCQOS_MAC_MAC_ADDR_HI_EN       BIT(31)
#define DWCQOS_MAC_PKT_FILT_RA          BIT(31)
#define DWCQOS_MAC_PKT_FILT_HPF         BIT(10)
#define DWCQOS_MAC_PKT_FILT_SAF         BIT(9)
#define DWCQOS_MAC_PKT_FILT_SAIF        BIT(8)
#define DWCQOS_MAC_PKT_FILT_DBF         BIT(5)
#define DWCQOS_MAC_PKT_FILT_PM          BIT(4)
#define DWCQOS_MAC_PKT_FILT_DAIF        BIT(3)
#define DWCQOS_MAC_PKT_FILT_HMC         BIT(2)
#define DWCQOS_MAC_PKT_FILT_HUC         BIT(1)
#define DWCQOS_MAC_PKT_FILT_PR          BIT(0)


#define DWCQOS_DMA_RDES1_IPCE    BIT(7)
#define DWCQOS_DMA_RDES3_ES      BIT(15)

#define DWCQOS_DMA_TDES3_CTXT    BIT(30)
#define DWCQOS_DMA_TDES3_TCMSSV    BIT(26)


#define DWCQOS_DMA_RDES1_PT      0x00000007
#define DWCQOS_DMA_RDES1_PT_UDP  BIT(0)
#define DWCQOS_DMA_RDES1_PT_TCP  BIT(1)
#define DWCQOS_DMA_RDES1_PT_ICMP 0x00000003

#define BUFFER_SIZE_2K  2048
#define BUFFER_SIZE_4K  4096
#define BUFFER_SIZE_8K  8192
#define BUFFER_SIZE_16K 16384

#define MAX_EACH_DESC_XFER_SIZE 16376
#define MAX_EACH_DESC_REV_SIZE  BUFFER_SIZE_2K
#define MAX_TSO_SEGMENT_SIZE    0x20000
#define FORCE_ACTIVE_QUEUE_INDEX    0

#define QOS_PHY_MODE    PHY_INTERFACE_MODE_RMII

#define QOS_GMAC_DEBUG  (NETIF_MSG_DRV | \
            NETIF_MSG_PROBE | \
            NETIF_MSG_LINK | \
            NETIF_MSG_TIMER | \
            NETIF_MSG_IFDOWN | \
            NETIF_MSG_IFUP | \
            NETIF_MSG_RX_ERR | \
            NETIF_MSG_TX_ERR | \
            NETIF_MSG_TX_QUEUED | \
            NETIF_MSG_INTR | \
            NETIF_MSG_TX_DONE | \
            NETIF_MSG_RX_STATUS | \
            NETIF_MSG_PKTDATA | \
            NETIF_MSG_HW | \
            NETIF_MSG_WOL)

/* #define RX_DESC_NUM NAPI_POLL_WEIGHT */
#define RX_DESC_NUM 256
#define TX_DESC_NUM 256

#ifndef PBUF_MALLOC_LEN
    #define PBUF_MALLOC_LEN         1600
#endif

#ifndef GMAC_TX_BUFFER_SIZE
    #define GMAC_TX_BUFFER_SIZE     2048
#endif

/* Flow Control defines */
#define FLOW_OFF    0
#define FLOW_RX     1
#define FLOW_TX     2
#define FLOW_AUTO   (FLOW_TX | FLOW_RX)

#define INTERNAL_PHY    0x55
#define EXTERNAL_PHY    0xaa
/****************************************************************************
 * ADT section
 *  add definition of user defined Data Type that only be used in this file  here
 ***************************************************************************/
enum
{
    GMAC_SPEED_10M,
    GMAC_SPEED_100M,
};

enum
{
    GMAC_DUPLEX_HALF,
    GMAC_DUPLEX_FULL,
};

enum
{
    gmac_mii = PHY_INTERFACE_MODE_MII,
    gmac_rmii = PHY_INTERFACE_MODE_RMII,
};

struct dwcqos_dma_desc
{
    u32_t desc0;
    u32_t desc1;
    u32_t desc2;
    u32_t desc3;
    u32_t rev[4];
}__attribute__((__aligned__(32)));


enum tx_dma_irq_status
{
    tx_hard_error = 1,
    tx_hard_error_bump_tc = 2,
    handle_tx_rx = 3,
};

enum rx_frame_status
{
    good_frame = 0,
    discard_frame = 1,
    csum_none = 2,
    llc_snap = 4,
};


struct dw_qos_hw_feature
{
    u32_t feature0;
    u32_t feature1;
    u32_t feature2;

    u32_t tx_fifo_size;
    u32_t rx_fifo_size;
    /* txq should == tx dma num.(ip required) */
    u32_t txq_num;
    u32_t rxq_num;
    u32_t tx_dma_num;
    u32_t rx_dma_num;
    u32_t tso_flag;
};



struct net_tx_queue
{

    struct dw_qos *pGmac;
    unsigned int irq_num;
    char irq_name[10];
    unsigned int id;

    void *p_raw_desc;
    struct dwcqos_dma_desc *p_descs;
    /* rec skbuf point add */
    u32_t *p_skbuf;
    /* rec skbuf point dma add */
    u32_t *tx_skbuff_dma;
    /* DMA Mapped Descriptor areas*/
    u32_t descs_phy_base_addr;
    u32_t descs_phy_tail_addr;
    u32_t desc_size;
    u32_t hw_queue_size;
    u32_t desc_idx;
    u32_t desc_xfer_max_size;
    u32_t mss;
    u32_t cur_idx;
    u32_t dirty_idx;

};


struct net_rx_queue
{

    /* bind to driver core point. */
    struct dw_qos *pGmac;
    unsigned int irq_num;
    char irq_name[10];
    unsigned int id;

    void *p_raw_desc;
    struct dwcqos_dma_desc *p_descs;
    /* rec each skbuf vadd, here just use u32_t array to rec */
    u32_t *p_skbuf;
    /* rec each skbuf padd, here just use u32_t array to rec */
    u32_t *rx_skbuff_dma;
    /* DMA Mapped Descriptor areas*/
    u32_t descs_phy_base_addr;
    u32_t descs_phy_tail_addr;
    u32_t desc_size;
    u32_t hw_queue_size;
    u32_t desc_xfer_max_size;
    u32_t cur_idx;
    u32_t dirty_idx;
    u32_t desc_idx;

};

struct fh_gmac_platform_data
{
    UINT32 id;
    UINT32 irq;
    UINT32 base_add;
    UINT32 phy_reset_pin;
    void (*gmac_reset)(void);
    /* phy need to sel func */
    int (*phy_sel)(unsigned int sel);
    /* mac need set mii or rmii to adjust phy, here just bind to phy driver */
    int (*inf_set)(unsigned int inf);
    struct phy_interface_info phy_info[2];
    void *p_cfg_array;

};

struct dw_qos
{
    struct net_device parent;
    void *regs;
    struct phy_device *phydev;
    unsigned char local_mac_address[6];
    struct fh_gmac_platform_data *priv_data;
    struct clk *clk;
    struct clk *rmii_clk;
    int phyreset_gpio;
    int phy_id;
    int phy_addr;
    int duplex;
    int speed;
    int oldduplex;
    int oldlink;
    int link;
    int pause;
    struct dw_qos_hw_feature hw_fea;
    struct net_tx_queue *tx_queue;
    struct net_rx_queue *rx_queue;
    int phy_interface;
    unsigned int active_queue_index;
    u32_t msg_enable;
    int (*phy_sel)(unsigned int sel);
    int (*inf_set)(unsigned int inf);
    struct phy_interface_info *ac_phy_info;
    struct phy_reg_cfg *ac_reg_cfg;
    u32_t flow_ctrl;
    int wolopts;
    struct rt_timer timer;
    struct rt_semaphore tx_desc_lock;
    struct rt_semaphore tx_lock;
    struct rt_semaphore rx_lock;
    struct rt_semaphore lock;
    struct rt_semaphore mdio_bus_lock;
};

/* Hardware register definitions. */
struct dw_qos_dma_channel_regs
{
    u32_t control;
    u32_t tx_control;
    u32_t rx_control;
    u32_t reserved_0;
    u32_t txdesc_list_haddr;
    u32_t txdesc_list_laddr;
    u32_t rxdesc_list_haddr;
    u32_t rxdesc_list_laddr;
    u32_t txdesc_tail_pointer;
    u32_t reserved_1;
    u32_t rxdesc_tail_pointer;
    u32_t txdesc_ring_len;
    u32_t rxdesc_ring_len;
    u32_t interrupt_enable;
    u32_t rx_interrupt_wdt_timer;
    u32_t slot_func_control_status;
    u32_t reserved_2;
    u32_t current_app_txdesc;
    u32_t reserved_3;
    u32_t current_app_rxdesc;
    u32_t current_app_txbuf_haddr;
    u32_t current_app_txbuf_laddr;
    u32_t current_app_rxbuf_haddr;
    u32_t current_app_rxbuf_laddr;
    u32_t status;
    u32_t reserved_4[2];
    u32_t miss_frame_cnt;
    u32_t reserved_5[4];
};

struct dw_qos_dma_regs
{
    u32_t mode;
    u32_t sysbus_mode;
    u32_t interrupt_status;
    u32_t debug_status0;
    u32_t debug_status1;
    u32_t debug_status2;
    u32_t reserved_0[2];
    u32_t tx_ar_ace_control;
    u32_t rx_aw_ace_control;
    u32_t txrx_awar_ace_control;
    u32_t reserved_1[53];
    struct dw_qos_dma_channel_regs chan[8];
};


/* mtl register */
struct dw_qos_mtl_queue_0_regs
{
    u32_t txq_operation_mode;
    u32_t txq_underflow;
    u32_t txq_debug;
    u32_t reserved_0[2];
    u32_t txq_ets_status;
    u32_t txq_quantum_weight;
    u32_t reserved_1[4];
    u32_t interrupt_control_status;
    u32_t rxq_operation_mode;
    u32_t rxq_missed_packet_overflow_cnt;
    u32_t rxq_debug;
    u32_t rxq_control;
};

struct dw_qos_mtl_queue_1_regs
{
    u32_t txq_operation_mode;
    u32_t txq_underflow;
    u32_t txq_debug;
    u32_t reserved_0;
    u32_t txq_ets_control;
    u32_t txq_ets_status;
    u32_t txq_quantum_weight;
    u32_t txq_send_slope_credit;
    u32_t txq_hi_credit;
    u32_t txq_lo_credit;
    u32_t reserved_1;
    u32_t interrupt_control_status;
    u32_t rxq_operation_mode;
    u32_t rxq_missed_packet_overflow_cnt;
    u32_t rxq_debug;
    u32_t rxq_control;
};

struct dw_qos_mtl_regs
{
    u32_t operation_mode;
    u32_t reserved_0;
    u32_t debug_control;
    u32_t debug_status;
    u32_t fifo_debug_data;
    /* doc bug... */
    u32_t reserved_1[3];

    u32_t interrupt_status;
    u32_t reserved_2[3];

    u32_t rxq_dma_map0;
    u32_t rxq_dma_map1;
    u32_t reserved_3[50];

    struct dw_qos_mtl_queue_0_regs q_0;
    struct dw_qos_mtl_queue_1_regs q_x[7];
    u32_t reserved_4[64];
};


/* mac register */
struct dw_mac_addr
{
    u32_t addr_hi;
    u32_t addr_lo;
};

struct dw_mac_mmc_regs
{
    u32_t control;
    u32_t rx_interrupt;
    u32_t tx_interrupt;
    u32_t rx_interrupt_mask;
    u32_t tx_interrupt_mask;
    u32_t tx_octet_cnt_good_bad;
    u32_t tx_packet_cnt_good_bad;
    u32_t tx_broadcast_packet_good;
    u32_t tx_multi_packet_good;
    u32_t tx_64_octet_packet_good_bad;
    u32_t tx_65_127_octet_packet_good_bad;
    u32_t tx_128_255_octet_packet_good_bad;
    u32_t tx_256_511_octet_packet_good_bad;
    u32_t tx_512_1023_octet_packet_good_bad;
    u32_t tx_1024_max_octet_packet_good_bad;
    u32_t tx_unicast_packet_good_bad;
    u32_t tx_multicast_packet_good_bad;
    u32_t tx_broadcast_packet_good_bad;
    u32_t tx_underflow_err_packet;
    u32_t tx_single_collision_packet;
    u32_t tx_multi_collision_packet;
    u32_t tx_deferred_packet;
    u32_t tx_late_collision_packet;
    u32_t tx_excessive_collision_packet;
    u32_t tx_carrier_err_packet;
    u32_t tx_octet_cnt_good;
    u32_t tx_packet_cnt_good;
    u32_t tx_excessive_deferral_err;
    u32_t tx_pause_packet;
    u32_t tx_vlan_packet_good;
    u32_t tx_osize_packet_good;
    u32_t reserved_0;
    u32_t rx_packet_cnt_good_bad;
    u32_t rx_octet_cnt_good_bad;
    u32_t rx_octet_cnt_good;
    u32_t rx_broadcast_packet_good;
    u32_t rx_multi_packet_good;
    u32_t rx_crc_err_packet;
    u32_t rx_align_err_packet;
    u32_t rx_runt_err_packet;
    u32_t rx_jabber_err_packet;
    u32_t rx_undersize_packet_good;
    u32_t rx_oversize_packet_good;
    u32_t rx_64_octet_packet_good_bad;
    u32_t rx_65_127_octet_packet_good_bad;
    u32_t rx_128_255_octet_packet_good_bad;
    u32_t rx_256_511_octet_packet_good_bad;
    u32_t rx_512_1023_octet_packet_good_bad;
    u32_t rx_1024_max_octet_packet_good_bad;
    u32_t rx_unicase_packet_good;
    u32_t rx_len_err_packet;
    u32_t rx_out_of_range_type_packet;
    u32_t rx_pause_packet;
    u32_t rx_fifo_overflow_packet;
    u32_t rx_vlan_packet_good_bad;
    u32_t rx_wdt_err_packet;
    u32_t rx_rev_err_packet;
    u32_t rx_control_packet_good;
    u32_t reserved_1;
    u32_t tx_lpi_micro_sec_timer;
    u32_t tx_lpi_trans_cnt;
    u32_t rx_lpi_micro_sec_timer;
    u32_t rx_lpi_trans_cnt;
    u32_t reserved_2;

    u32_t mmc_ipc_rx_interrupt_mask;
    u32_t reserved_3;
    u32_t mmc_ipc_rx_interrupt;
    u32_t reserved_4;
    /* 0x810 TBD........ */
    u32_t reserved_5[60];
};

struct dw_qos_mac_regs
{
    u32_t config;
    u32_t ext_config;
    u32_t packet_filter;
    u32_t wdt_timeout;
    u32_t hash_table_reg[8];
    u32_t reserved_0[8];
    u32_t vlan_tag;
    u32_t reserved_1;
    u32_t vlan_hash_table;
    u32_t reserved_2;
    u32_t vlan_incl;
    u32_t inner_vlan_incl;
    u32_t reserved_3[2];
    u32_t tx_flow_ctrl[8];
    u32_t rx_flow_ctrl;
    u32_t reserved_4;
    u32_t txq_prty_map0;
    u32_t txq_prty_map1;
    u32_t rxq_ctrl[4];
    u32_t interrupt_status;
    u32_t interrupt_enable;
    u32_t rx_tx_status;
    u32_t reserved_5;
    u32_t pmt_control_status;
    u32_t rwk_packet_filter;
    u32_t reserved_6[2];
    u32_t lpi_control_status;
    u32_t lpi_timers_control;
    u32_t lpi_entry_timer;
    u32_t us_tic_cnt;
    u32_t an_control;
    u32_t an_status;
    u32_t an_advertisement;
    u32_t an_link_partner_ability;
    u32_t an_expansion;
    u32_t tbi_extended_status;
    u32_t phyif_control_status;
    u32_t reserved_7[5];
    u32_t version;
    u32_t debug;
    u32_t reserved_8;
    u32_t hw_feature_0;
    u32_t hw_feature_1;
    u32_t hw_feature_2;
    u32_t reserved_9[54];
    u32_t mdio_addr;
    u32_t mdio_data;
    u32_t gpio_control;
    u32_t gpio_status;
    u32_t arp_addr;
    u32_t reserved_10[59];
    struct dw_mac_addr mac_addr[128];
    struct dw_mac_mmc_regs mmc;
    /* TBD 0x900... */
    u32_t reserved_11[192];
};

struct dw_qos_regs_map
{
    struct dw_qos_mac_regs mac;
    struct dw_qos_mtl_regs mtl;
    struct dw_qos_dma_regs dma;
};


/*****************************************************************************
 *  fun below;
 *****************************************************************************/
void fh_qos_adjust_link(struct dw_qos *pGmac);
void fh_gmac_set_ethtool_ops(struct net_device *netdev);
void fh_qos_tx_flow_process(struct dw_qos *pGmac);
#endif
