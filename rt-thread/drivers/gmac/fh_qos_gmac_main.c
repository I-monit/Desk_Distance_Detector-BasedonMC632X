/*****************************************************************************
 *  Include Section
 *  add all #include here
 *****************************************************************************/
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

#ifdef RT_USING_PM
#include <pm.h>
#endif
/*****************************************************************************
 * Define section
 * add all #define here
 *****************************************************************************/
#define TX_TIMEO    5000
#ifndef CACHE_LINE_SIZE
#define CACHE_LINE_SIZE     32
#endif

#define FLOW_CTL_MIN_RX_FIFO    0x1000
#define FLOW_CTL_DEACTIVE_RX_FIFO   0x400
#define FLOW_CTL_MAX_PAUSE_TIME 0xffff
#define FLOW_CTL_PAUSE_TIME     0x500
#define DUMMY_SK_BUFF_FLAG  0xDEADBEEF

#define CHANGE_BIT_LINK     BIT(0)
#define CHANGE_BIT_SPD      BIT(1)
#define CHANGE_BIT_DUP      BIT(2)
#define CHANGE_BIT_PAUSE    BIT(3)
#define DMA_CH_STOPPED   0
#define DMA_TX_CH_SUSPENDED 6

#define ETH_FCS_LEN 4       /* Octets in the FCS */

/* EMAC has BD's in cached memory - so need cache functions */
extern void mmu_invalidate_dcache(rt_uint32_t buffer, rt_uint32_t size);
extern void mmu_clean_dcache(rt_uint32_t buffer, rt_uint32_t size);
#define BD_CACHE_INVALIDATE(addr, size) \
    mmu_invalidate_dcache((rt_uint32_t)addr, size)
#define BD_CACHE_WRITEBACK(addr, size) mmu_clean_dcache((rt_uint32_t)addr, size)

extern void mmu_clean_invalidated_dcache(rt_uint32_t buffer, rt_uint32_t size);
#define BD_CACHE_WRITEBACK_INVALIDATE(addr, size) \
    mmu_clean_invalidated_dcache((rt_uint32_t)addr, size)

static unsigned char gs_mac_addr[6] = {

    0x78, 0x99, 0x6E, 0x33, 0x44, 0x55
};

void fh_gmac_initmac(unsigned char *gmac)
{
    rt_memcpy(gs_mac_addr, gmac, 6);
    rt_kprintf("set Mac address.%02x:%02x:%02x:%02x:%02x:%02x\n", gs_mac_addr[0], gs_mac_addr[1],
                                   gs_mac_addr[2], gs_mac_addr[3], gs_mac_addr[4], gs_mac_addr[5]);
}

#define max_t(type, x, y) ({            \
    type __max1 = (x);            \
    type __max2 = (y);            \
    __max1 > __max2 ? __max1 : __max2; })

/*****************************************************************************
 *  static fun;
 *****************************************************************************/
static int fh_qos_parse_plat_info(struct dw_qos *pGmac, void *priv_data);
static void fh_qos_suspend(struct dw_qos *pGmac);
static void fh_qos_resume(struct dw_qos *pGmac);
static void fh_qos_set_speed(struct dw_qos *pGmac);
static void fh_qos_link_up(struct dw_qos *pGmac);
static void fh_qos_link_down(struct dw_qos *pGmac);
static int fh_qos_txq_malloc_desc(struct dw_qos *pGmac, u32_t q_no, u32_t malloc_size);
/* static u32_t fh_qos_tx_desc_avail(struct dw_qos *pGmac, u32_t q_no);*/
static struct dwcqos_dma_desc *fh_qos_get_tx_desc_cur(struct dw_qos *pGmac, u32_t q_no, u32_t *index);
/* static u32_t get_tx_skbuf_vadd(struct dw_qos *pGmac, u32_t q_no, u32_t index);*/
static u32_t get_tx_skbuf_padd(struct dw_qos *pGmac, u32_t q_no, u32_t index);
/* static void set_tx_skbuf_vadd(struct dw_qos *pGmac, u32_t q_no, u32_t index, u32_t vadd);*/
/* static void set_tx_skbuf_padd(struct dw_qos *pGmac, u32_t q_no, u32_t index, u32_t padd); */
/* static struct dwcqos_dma_desc *__fh_qos_get_tx_desc(struct dw_qos *pGmac, u32_t q_no, u32_t index);*/
static void fh_qos_tx_desc_index_init(struct dw_qos *pGmac, u32_t q_no);
static int  fh_qos_rxq_malloc_desc(struct dw_qos *pGmac, u32_t q_no, u32_t malloc_size);
static struct dwcqos_dma_desc *fh_qos_get_rx_desc_cur(struct dw_qos *pGmac, u32_t q_no, u32_t *index);
/* static struct dwcqos_dma_desc *__fh_qos_get_rx_desc(struct dw_qos *pGmac, u32_t q_no, u32_t index); */
static u32_t get_rx_skbuf_vadd(struct dw_qos *pGmac, u32_t q_no, u32_t index);
/* static u32_t get_rx_skbuf_padd(struct dw_qos *pGmac, u32_t q_no, u32_t index); */
static void set_rx_skbuf_vadd(struct dw_qos *pGmac, u32_t q_no, u32_t index, u32_t vadd);
static void set_rx_skbuf_padd(struct dw_qos *pGmac, u32_t q_no, u32_t index, u32_t padd);
static void debug_rx_desc(void);
static void fh_qos_rx_desc_index_init(struct dw_qos *pGmac, u32_t q_no);
static int dwcqos_reset_dma_hw(struct dw_qos *pGmac);
static void dwcqos_set_dma_mode(struct dw_qos *pGmac, u32_t val);
static void dwcqos_configure_bus(struct dw_qos *pGmac);
static void dwcqos_dma_tx_enable_set(struct dw_qos *pGmac, u32_t dma_no, u32_t enable);
static void dwcqos_dma_rx_enable_set(struct dw_qos *pGmac, u32_t dma_no, u32_t enable);
static void dwcqos_dma_isr_enable_set(struct dw_qos *pGmac, u32_t dma_no, u32_t val);
static u32_t dwcqos_dma_isr_get(struct dw_qos *pGmac, u32_t dma_no);
static void dwcqos_dma_isr_tx_set(struct dw_qos *pGmac, u32_t dma_no, u32_t enable);
static void dwcqos_dma_isr_rx_set(struct dw_qos *pGmac, u32_t dma_no, u32_t enable);
static u32_t dwcqos_dma_isr_status_get(struct dw_qos *pGmac, u32_t dma_no);
static void dwcqos_dma_isr_status_set(struct dw_qos *pGmac, u32_t dma_no, u32_t val);
static void dwcqos_set_hw_mac_addr(struct dw_qos *pGmac, u8_t *mac);
/* static void dwcqos_disable_umac_addr(struct dw_qos *pGmac, unsigned int reg_n); */
static void dwcqos_set_hw_mac_filter(struct dw_qos *pGmac, u32_t filter);
static void dwcqos_set_hw_mac_interrupt(struct dw_qos *pGmac, u32_t isr);
static void dwcqos_set_hw_mac_tx_flowctrl(struct dw_qos *pGmac, u32_t q_no, u32_t data);
static int dwcqos_get_hw_mac_tx_flowctrl(struct dw_qos *pGmac, u32_t q_no);
static int dwcqos_get_mtl_rx_operation(struct dw_qos *pGmac, u32_t q_no);
static void dwcqos_set_mtl_rx_operation(struct dw_qos *pGmac, u32_t q_no, u32_t data);
static void dwcqos_set_hw_parse_pause_frame(struct dw_qos *pGmac, u32_t enable);
static u32_t dwcqos_hw_halt_xmit(struct dw_qos *pGmac);
static void dwcqos_set_hw_tx_auto_flowctrl(struct dw_qos *pGmac, u32_t rxq_no, u32_t txq_no,
u32_t pause_time, u32_t enable);
static void dwcqos_set_hw_mac_config(struct dw_qos *pGmac, u32_t config);
static int dwcqos_get_hw_mac_config(struct dw_qos *pGmac);
static void dwcqos_mac_spd_port_set(struct dw_qos *pGmac, int spd);
static void dwcqos_mac_duplex_set(struct dw_qos *pGmac, int duplex);
static void dwcqos_mac_route_rxqueue_set(struct dw_qos *pGmac);
static void dwcqos_queue_enable_set(struct dw_qos *pGmac, u32_t queue_no, u32_t enable);
static void dwcqos_mac_tx_enable(struct dw_qos *pGmac);
static void dwcqos_mac_tx_disable(struct dw_qos *pGmac);
static void dwcqos_mac_rx_enable(struct dw_qos *pGmac);
static void dwcqos_mac_rx_disable(struct dw_qos *pGmac);
static int dwcqos_get_hw_mtl_txqx_debug(struct dw_qos *pGmac, u32_t q_no);
static int dwcqos_get_hw_mtl_rxqx_debug(struct dw_qos *pGmac, u32_t q_no);
static int dwcqos_get_hw_mtl_isr_status(struct dw_qos *pGmac, u32_t q_no);
static void dwcqos_set_hw_mtl_isr_status(struct dw_qos *pGmac, u32_t q_no, u32_t val);
/* static void dwcqos_dma_chan_set_mss(struct dw_qos *pGmac, u32_t q_no, u32_t val); */
static int dwcqos_dma_get_interrupt_status(struct dw_qos *pGmac);
static int dwcqos_mac_get_interrupt_status(struct dw_qos *pGmac);
static void dwcqos_init_mtl_hw_rxdma_queue_map(struct dw_qos *pGmac);
static void dwcqos_dma_desc_init(struct dw_qos *pGmac);
static void dwcqos_dma_chan_init(struct dw_qos *pGmac);
static void refix_feature(struct dw_qos *pGmac);
static void parse_hw_feature(struct dw_qos *pGmac);
static void fh_qos_mac_add_init(struct dw_qos *pGmac);
static rt_err_t rt_fh_gmac_init(rt_device_t dev);
static rt_err_t rt_fh_gmac_open(rt_device_t dev, rt_uint16_t oflag);
static rt_err_t rt_fh_gmac_close(rt_device_t dev);
static rt_size_t rt_fh_gmac_read(rt_device_t dev, rt_off_t pos, void *buffer,
                                 rt_size_t size);
static rt_size_t rt_fh_gmac_write(rt_device_t dev, rt_off_t pos,
                                  const void *buffer, rt_size_t size);
static rt_err_t rt_fh_gmac_control(rt_device_t dev, int cmd, void *args);
static void dwcqos_desc_init(struct dw_qos *pGmac);
static int  dwcqos_init_sw(struct dw_qos *pGmac);
static int dwcqos_init_dma_hw(struct dw_qos *pGmac);
static void dwcqos_kick_tx_queue(struct dw_qos *pGmac, u32_t q_no);
static void dwcqos_kick_rx_queue(struct dw_qos *pGmac, u32_t q_no);
static void dwcqos_init_mtl_hw(struct dw_qos *pGmac);
static void dwcqos_init_mac_hw(struct dw_qos *pGmac);
static int dwcqos_init_hw(struct dw_qos *pGmac);
static void set_first_desc(struct dwcqos_dma_desc *pdesc);
static void set_last_desc(struct dwcqos_dma_desc *pdesc);
/*
static void fh_qos_prepare_normal_send(struct dwcqos_dma_desc *pdesc,
 u32_t p_buf_add, u32_t len, u32_t total_len, u32_t crc_flag);
*/
static void set_desc_dma_valid(struct dwcqos_dma_desc *pdesc);
static void set_desc_isr_valid(struct dwcqos_dma_desc *pdesc);
/* static u32_t get_desc_data_len(struct dwcqos_dma_desc *pdesc); */
/* static int cal_tx_desc_require(struct sk_buff *skb, struct net_tx_queue *tx_queue); */
/* static int qos_tso_xmit(struct sk_buff *skb, struct net_device *ndev); */
/* static int check_rx_packet(struct dw_qos *pGmac, struct dwcqos_dma_desc *p_desc); */
static void dwcqos_mtl_isr_process(struct dw_qos *pGmac);
static void dwcqos_read_mmc_counters(struct dw_qos *pGmac, u32_t rx_mask,
u32_t tx_mask, u32_t lpc_mask);
static void dwcqos_mac_isr_process(struct dw_qos *pGmac);
static void fh_gmac_common_interrupt(int irq, void *param);
static int fh_qos_gmac_probe(void *priv_data);
static void dump_lli(struct dwcqos_dma_desc *desc, u32_t size);

/*****************************************************************************
 * func below
 *****************************************************************************/

/*****************************************************************************
 * func below
 *****************************************************************************/
static int dwcqos_is_tx_dma_suspended(struct dw_qos *pGmac)
{
    unsigned int ret;

    ret = dw_readl(pGmac, dma.debug_status0);
    ret = ((ret & 0xF000) >> 12);

    return ((ret == DMA_TX_CH_SUSPENDED) || (ret == DMA_CH_STOPPED));
}


static void dwccqos_drain_tx_dma(struct dw_qos *pGmac)
{
    size_t limit = (TX_DESC_NUM * 1250) / 100;

    while (!dwcqos_is_tx_dma_suspended(pGmac) && limit--)
        udelay(200);

    if (!dwcqos_is_tx_dma_suspended(pGmac))
        rt_kprintf("Drain TX DMA Fail REG_DWCEQOS_DMA_DEBUG_ST0 0x%x\n",
        dw_readl(pGmac, dma.debug_status0));

}

static int dwcqos_is_txq_suspended(struct dw_qos *pGmac)
{
    unsigned int reg;

    reg = dwcqos_get_hw_mtl_txqx_debug(pGmac, 0);
    return ((reg & 0x06) != 0x02) && (!(reg & 0x10));
}

static void dwcqos_drain_txq(struct dw_qos *pGmac)
{
    size_t limit = (TX_DESC_NUM * 1250) / 100;

    while (!dwcqos_is_txq_suspended(pGmac) && limit--)
        udelay(200);

    if (!dwcqos_is_txq_suspended(pGmac))
        rt_kprintf("Drain TXQ Fail TXQ0_DEBUG_ST 0x%08x\n",
        dwcqos_get_hw_mtl_txqx_debug(pGmac, 0));
}

static int dwcqos_is_rxq_suspended(struct dw_qos *pGmac)
{
    unsigned int reg;

    reg = dwcqos_get_hw_mtl_rxqx_debug(pGmac, 0);

    return (((reg & 0x30) == 0) && (!(reg & 0x3fff0000)));
}

static void dwcqos_drain_rxq(struct dw_qos *pGmac)
{

    size_t limit = (RX_DESC_NUM * 1250) / 100;

    while (!dwcqos_is_rxq_suspended(pGmac) && limit--)
        udelay(200);

    if (!dwcqos_is_rxq_suspended(pGmac))
        rt_kprintf(" MTL_RXQ0_DEBUG_ST 0x%08x REG_DWCEQOS_MAC_CFG 0x%08x\n",
        dwcqos_get_hw_mtl_rxqx_debug(pGmac, 0),
        dw_readl(pGmac, mac.config));
}

static int fh_qos_parse_plat_info(struct dw_qos *pGmac, void *priv_data)
{
    int ret;
    struct fh_gmac_platform_data *p_plat_data;
    /* int feature2; */

    p_plat_data = (struct fh_gmac_platform_data *)priv_data;
    pGmac->phyreset_gpio = p_plat_data->phy_reset_pin;
    pGmac->regs = (void *)p_plat_data->base_add;

    /* feature2 = __qos_raw_readl(&(((struct dw_qos_regs_map *)pGmac->regs)->mac.hw_feature_2)); */

    pGmac->priv_data = p_plat_data;

    pGmac->clk = clk_get(NULL, "eth_clk");
    if (pGmac->clk != RT_NULL)
    {
        clk_enable(pGmac->clk);
    }

    pGmac->rmii_clk = clk_get(NULL, "eth_rmii_clk");
    if (pGmac->rmii_clk != RT_NULL)
    {
        clk_enable(pGmac->rmii_clk);
    }

    pGmac->hw_fea.feature0 = dw_readl(pGmac, mac.hw_feature_0);
    pGmac->hw_fea.feature1 = dw_readl(pGmac, mac.hw_feature_1);
    pGmac->hw_fea.feature2 = dw_readl(pGmac, mac.hw_feature_2);

    pGmac->active_queue_index = FORCE_ACTIVE_QUEUE_INDEX;
    parse_hw_feature(pGmac);

    if (pGmac->active_queue_index >= pGmac->hw_fea.tx_dma_num)
    {
        rt_kprintf("MAX active queue is %d\n", pGmac->hw_fea.tx_dma_num - 1);
        return -1;
    }

    pGmac->tx_queue = (struct net_tx_queue *) rt_malloc(sizeof(struct net_tx_queue) * pGmac->hw_fea.txq_num);
    if (!pGmac->tx_queue)
    {
        rt_kprintf("malloc tx queue failed.....\n");
        ret = -ENOMEM;
        return ret;
    }
    pGmac->rx_queue = (struct net_rx_queue *) rt_malloc(sizeof(struct net_rx_queue) * pGmac->hw_fea.rxq_num);

    if (!pGmac->rx_queue)
    {
        rt_free(pGmac->tx_queue);
        rt_kprintf("malloc rx queue failed.....\n");
        ret = -ENOMEM;
        return ret;
    }
    rt_memset(pGmac->tx_queue, 0, sizeof(struct net_tx_queue) * pGmac->hw_fea.txq_num);
    rt_memset(pGmac->rx_queue, 0, sizeof(struct net_rx_queue) * pGmac->hw_fea.rxq_num);

    return 0;
}

extern void net_device_stop(char *name);
static void fh_qos_suspend(struct dw_qos *pGmac)
{
    /* struct net_rx_queue *queue; */
    u32_t qno = 0;
    /* close isr. isr will schedule napi..so just close isr */
    DWCQOS_FOR_EACH_QUEUE(pGmac->hw_fea.rxq_num, qno)
    {
        dwcqos_dma_isr_rx_set(pGmac, qno, 0);
    }

    DWCQOS_FOR_EACH_QUEUE(pGmac->hw_fea.txq_num, qno)
    {
        dwcqos_dma_isr_tx_set(pGmac, qno, 0);
    }
    /* close rx mac shift. */
    dwcqos_mac_rx_disable(pGmac);
    dwcqos_drain_rxq(pGmac);
    dwcqos_dma_rx_enable_set(pGmac, 0, 0);
    /* flush rx data... */
    /* neet to confirm all rx desc is idle( bit31 == 1) */
    /* queue = &pGmac->rx_queue[0]; */
    dwccqos_drain_tx_dma(pGmac);
    dwcqos_dma_tx_enable_set(pGmac, 0, 0);
    /* close mtl */
    dwcqos_drain_txq(pGmac);
    dwcqos_mac_tx_disable(pGmac);

    rt_memset(pGmac->tx_queue[0].p_skbuf, 0, GMAC_TX_BUFFER_SIZE * pGmac->tx_queue[0].desc_size);
    rt_memset(pGmac->tx_queue[0].p_descs, 0, pGmac->tx_queue[0].desc_size * sizeof(struct dwcqos_dma_desc));

}

static void fh_qos_resume(struct dw_qos *pGmac)
{
    u32_t qno;
    u32_t i;

    DWCQOS_FOR_EACH_QUEUE(pGmac->hw_fea.txq_num, i)
    {
        fh_qos_tx_desc_index_init(pGmac, i);
    }
    DWCQOS_FOR_EACH_QUEUE(pGmac->hw_fea.rxq_num, i)
    {
        fh_qos_rx_desc_index_init(pGmac, i);
    }

    dwcqos_init_hw(pGmac);
    dwcqos_mac_spd_port_set(pGmac, pGmac->speed);
    dwcqos_mac_duplex_set(pGmac, pGmac->duplex);

    DWCQOS_FOR_EACH_QUEUE(max_t(u32_t, pGmac->hw_fea.rxq_num, pGmac->hw_fea.txq_num), qno)
    {
        dwcqos_dma_isr_enable_set(pGmac, qno,
        DWCQOS_DMA_CH_IE_NIE |
        DWCQOS_DMA_CH_IE_AIE |
        DWCQOS_DMA_CH_IE_FBEE);
    }

    DWCQOS_FOR_EACH_QUEUE(pGmac->hw_fea.txq_num, qno)
    {
        dwcqos_dma_tx_enable_set(pGmac, qno, 1);
        dwcqos_dma_isr_tx_set(pGmac, qno, 1);
    }

    DWCQOS_FOR_EACH_QUEUE(pGmac->hw_fea.rxq_num, qno) {
        dwcqos_dma_rx_enable_set(pGmac, qno, 1);
        dwcqos_dma_isr_rx_set(pGmac, qno, 1);
        dwcqos_kick_rx_queue(pGmac, qno);
    }

#if (0)
    /* mac start later, should sync with phy linkup. */
    dwcqos_mac_rx_enable(pGmac);
    dwcqos_mac_tx_enable(pGmac);
#endif

}

static void fh_qos_set_speed(struct dw_qos *pGmac)
{
    int speed;
    u32_t inf_sup = pGmac->ac_reg_cfg->inf_sup;

    speed = pGmac->speed;

    fh_qos_suspend(pGmac);

    if (pGmac->ac_phy_info->ex_sync_mac_spd)
    {
        pGmac->ac_phy_info->ex_sync_mac_spd(speed,
        pGmac->ac_reg_cfg);
    }

    if (pGmac->inf_set)
    {
        switch (speed)
        {
        case  1000:
            pGmac->inf_set(PHY_INTERFACE_MODE_RGMII);
        break;
        case  100:
            if (inf_sup == PHY_INTERFACE_MODE_RGMII)
            {
                pGmac->inf_set(PHY_INTERFACE_MODE_MII);
            }
            else if (inf_sup == PHY_INTERFACE_MODE_RMII)
            {
                pGmac->inf_set(PHY_INTERFACE_MODE_RMII);
            }
            else
            {
                pGmac->inf_set(PHY_INTERFACE_MODE_MII);
            }
        break;
        case  10:
            if (inf_sup == PHY_INTERFACE_MODE_RGMII)
            {
                pGmac->inf_set(PHY_INTERFACE_MODE_MII);
            }
            else if (inf_sup == PHY_INTERFACE_MODE_RMII)
            {
                pGmac->inf_set(PHY_INTERFACE_MODE_RMII);
            }
            else
            {
                pGmac->inf_set(PHY_INTERFACE_MODE_MII);
            }
        break;

        default:
            rt_kprintf("unknown spd %x\n", speed);
        break;
        }
    }

    fh_qos_resume(pGmac);
}

static void fh_qos_link_up(struct dw_qos *pGmac)
{

    if (pGmac->ac_phy_info->brd_link_up_cb)
        pGmac->ac_phy_info->brd_link_up_cb((void *)pGmac);

    dwcqos_mac_rx_enable(pGmac);
    dwcqos_mac_tx_enable(pGmac);
}

static void fh_qos_link_down(struct dw_qos *pGmac)
{
    if (pGmac->ac_phy_info->brd_link_down_cb)
        pGmac->ac_phy_info->brd_link_down_cb((void *)pGmac);

    dwcqos_mac_rx_disable(pGmac);
    dwcqos_mac_tx_disable(pGmac);
}

void fh_qos_tx_flow_process(struct dw_qos *pGmac)
{
    u32_t ret;
    struct phy_device *phydev;

    phydev = pGmac->phydev;
    ret = dwcqos_hw_halt_xmit(pGmac);
    if (phydev->pause)
    {
        if (pGmac->flow_ctrl & FLOW_TX)
            dwcqos_set_hw_tx_auto_flowctrl(pGmac, 0, 0, FLOW_CTL_PAUSE_TIME, 1);
        else
            dwcqos_set_hw_tx_auto_flowctrl(pGmac, 0, 0, FLOW_CTL_PAUSE_TIME, 0);
    }
    else
        dwcqos_set_hw_tx_auto_flowctrl(pGmac, 0, 0, FLOW_CTL_PAUSE_TIME, 0);
    dwcqos_set_hw_mac_config(pGmac, dwcqos_get_hw_mac_config(pGmac) | ret);
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

void fh_qos_adjust_link(struct dw_qos *pGmac)
{
    struct phy_device *phydev;
    int status_change = 0;
    u32_t ret;

    phydev = pGmac->phydev;
    if (!phydev)
        return;

    if (phydev->link)
    {
        if (pGmac->duplex != phydev->duplex)
        {
            pGmac->duplex = phydev->duplex;
            status_change |= CHANGE_BIT_DUP;
        }
        if (pGmac->speed != phydev->speed)
        {
            pGmac->speed = phydev->speed;
            status_change |= CHANGE_BIT_SPD;
        }
        if (pGmac->pause != phydev->pause)
        {
            pGmac->pause = phydev->pause;
            status_change |= CHANGE_BIT_PAUSE;
        }
    }

    if (phydev->link != pGmac->link)
    {
        pGmac->link = phydev->link;
        status_change |= CHANGE_BIT_LINK;
    }
    if (status_change & CHANGE_BIT_SPD)
    {
        /* here will do hw reset!!!! */
        fh_qos_set_speed(pGmac);
        ret = dwcqos_hw_halt_xmit(pGmac);
        dwcqos_mac_duplex_set(pGmac, pGmac->duplex);
        dwcqos_set_hw_mac_config(pGmac, dwcqos_get_hw_mac_config(pGmac) | ret);
    }

    if (status_change & CHANGE_BIT_DUP)
    {
        ret = dwcqos_hw_halt_xmit(pGmac);
        dwcqos_mac_duplex_set(pGmac, pGmac->duplex);
        dwcqos_set_hw_mac_config(pGmac, dwcqos_get_hw_mac_config(pGmac) | ret);
    }

    /* TBD open rx flow ctl!!!!..maybe halt the local area network */
    if (status_change & CHANGE_BIT_PAUSE)
        fh_qos_tx_flow_process(pGmac);

    if (status_change)
    {
        net_device_linkchange(&(pGmac->parent), phydev->link);

        if (phydev->link)
        {
            fh_qos_link_up(pGmac);
        }
        else
        {
            fh_qos_link_down(pGmac);
        }
        phy_print_status(phydev);
    }

}

extern void *fh_dma_mem_malloc_align(rt_uint32_t size, rt_uint32_t align, char *name);
extern void fh_dma_mem_free(void *ptr);

static int fh_qos_txq_malloc_desc(struct dw_qos *pGmac, u32_t q_no, u32_t malloc_size)
{
    u32_t t2;

    pGmac->tx_queue[q_no].id = q_no;
    pGmac->tx_queue[q_no].desc_size = malloc_size;

    /* malloc skbuff array */
    t2 = (u32_t)rt_malloc_align(GMAC_TX_BUFFER_SIZE * malloc_size, CACHE_LINE_SIZE);
    if (!t2)
    {
        rt_kprintf("[tx alloc] :: no mem for p_skbuf\n");
        return -1;
    }
    /* pGmac->tx_queue[q_no].p_skbuf = rt_malloc(sizeof(u32_t) * malloc_size); */
    pGmac->tx_queue[q_no].p_skbuf = (u32_t *)t2;
    rt_memset(pGmac->tx_queue[q_no].p_skbuf, 0, GMAC_TX_BUFFER_SIZE * malloc_size);

    /* malloc skbuff dma array */
    pGmac->tx_queue[q_no].tx_skbuff_dma = pGmac->tx_queue[q_no].p_skbuf;

    pGmac->tx_queue[q_no].p_raw_desc = fh_dma_mem_malloc_align(sizeof(struct dwcqos_dma_desc) * pGmac->tx_queue[q_no].desc_size, CACHE_LINE_SIZE, "gmac");
    if (!pGmac->tx_queue[q_no].p_raw_desc)
    {
        rt_kprintf("[tx alloc] :: no mem for p_skbuf\n");
        rt_free(pGmac->tx_queue[q_no].p_skbuf);
        return -2;
    }

    pGmac->tx_queue[q_no].p_descs = pGmac->tx_queue[q_no].p_raw_desc;
    pGmac->tx_queue[q_no].descs_phy_base_addr = (u32_t)pGmac->tx_queue[q_no].p_descs;
    pGmac->tx_queue[q_no].descs_phy_tail_addr = pGmac->tx_queue[q_no].descs_phy_base_addr + (pGmac->tx_queue[q_no].desc_size) * sizeof(struct dwcqos_dma_desc);
    pGmac->tx_queue[q_no].hw_queue_size = pGmac->hw_fea.tx_fifo_size / pGmac->hw_fea.txq_num;
    pGmac->tx_queue[q_no].desc_xfer_max_size = MAX_EACH_DESC_XFER_SIZE;
    rt_memset(pGmac->tx_queue[q_no].p_descs, 0, pGmac->tx_queue[q_no].desc_size * sizeof(struct dwcqos_dma_desc));
    return 0;
}

#if 0
static u32_t fh_qos_tx_desc_avail(struct dw_qos *pGmac, u32_t q_no)
{
    return pGmac->tx_queue[q_no].dirty_idx + pGmac->tx_queue[q_no].desc_size - pGmac->tx_queue[q_no].cur_idx;
}
#endif

static struct dwcqos_dma_desc *fh_qos_get_tx_desc_cur(struct dw_qos *pGmac, u32_t q_no, u32_t *index)
{
    struct dwcqos_dma_desc  *p_desc;
    int ret = 0;
    int retry = 10;

    if ((pGmac->phydev->speed == SPEED_100) || (pGmac->phydev->speed == SPEED_1000))
    {
        retry = 10000;
    }

    while (1)
    {
        ret = dw_readl(pGmac, mac.config);
        dw_writel(pGmac, mac.config, ret);
        p_desc = &pGmac->tx_queue[q_no].p_descs[pGmac->tx_queue[q_no].cur_idx];
        BD_CACHE_INVALIDATE((rt_uint32_t)p_desc, sizeof(struct dwcqos_dma_desc));
        /* check hw desc dma own */
        if (p_desc->desc3 & DWCQOS_DMA_TDES3_OWN)
        {
            if (pGmac->phydev->speed == SPEED_10)
                rt_thread_delay(1);
            else
                fh_udelay(1);
            retry--;
            if (!retry)
            {
                /* rt_kprintf("[%s-%d]: pGmac->tx_queue[q_no].cur_idx %d\n", __func__, __LINE__, pGmac->tx_queue[q_no].cur_idx);
                 * dump_lli(pGmac->tx_queue[q_no].p_descs, pGmac->tx_queue[0].desc_size);
                 */
                return NULL;
            }
        }
        else
        {
            break;
        }
    }

    memset(p_desc, 0, sizeof(struct dwcqos_dma_desc));
    *index = pGmac->tx_queue[q_no].cur_idx;
    pGmac->tx_queue[q_no].cur_idx++;
    pGmac->tx_queue[q_no].cur_idx %= pGmac->tx_queue[q_no].desc_size;

    return p_desc;
}

#if 0
static u32_t get_tx_skbuf_vadd(struct dw_qos *pGmac, u32_t q_no, u32_t index)
{
    return (u32_t) pGmac->tx_queue[q_no].p_skbuf[index % pGmac->tx_queue[q_no].desc_size];
}
#endif

static u32_t get_tx_skbuf_padd(struct dw_qos *pGmac, u32_t q_no, u32_t index)
{
    index = index % pGmac->tx_queue[q_no].desc_size;
    return (u32_t) &pGmac->tx_queue[q_no].tx_skbuff_dma[index * (GMAC_TX_BUFFER_SIZE/4)];
}

#if 0
static void set_tx_skbuf_vadd(struct dw_qos *pGmac, u32_t q_no, u32_t index, u32_t vadd)
{
    pGmac->tx_queue[q_no].p_skbuf[index % pGmac->tx_queue[q_no].desc_size] = vadd;
}

static void set_tx_skbuf_padd(struct dw_qos *pGmac, u32_t q_no, u32_t index, u32_t padd)
{
    pGmac->tx_queue[q_no].tx_skbuff_dma[index % pGmac->tx_queue[q_no].desc_size] = padd;
}

static struct dwcqos_dma_desc *__fh_qos_get_tx_desc(struct dw_qos *pGmac, u32_t q_no, u32_t index)
{
    struct dwcqos_dma_desc  *p_desc;

    p_desc = &pGmac->tx_queue[q_no].p_descs[index];
    return p_desc;
}
#endif

static void fh_qos_tx_desc_index_init(struct dw_qos *pGmac, u32_t q_no)
{
    pGmac->tx_queue[q_no].cur_idx = 0;
    pGmac->tx_queue[q_no].dirty_idx = 0;
}

/*******/
static int  fh_qos_rxq_malloc_desc(struct dw_qos *pGmac, u32_t q_no, u32_t malloc_size)
{
    struct pbuf *p;
    struct dwcqos_dma_desc *p_descs;
    int i, j;

    pGmac->rx_queue[q_no].id = q_no;
    pGmac->rx_queue[q_no].desc_size = malloc_size;

    /* malloc skbuff array */
    pGmac->rx_queue[q_no].p_skbuf = rt_malloc(sizeof(u32_t) * malloc_size);
    if (!pGmac->rx_queue[q_no].p_skbuf)
    {
        rt_kprintf("[rx alloc] :: no mem for p_skbuf\n");
        return -1;
    }

    rt_memset(pGmac->rx_queue[q_no].p_skbuf, 0, sizeof(u32_t) * malloc_size);

    pGmac->rx_queue[q_no].rx_skbuff_dma = rt_malloc(sizeof(u32_t) * malloc_size);
    if (!pGmac->rx_queue[q_no].rx_skbuff_dma)
    {
        rt_free(pGmac->rx_queue[q_no].p_skbuf);
        rt_kprintf("[rx alloc] :: no mem for rx_skbuff_dma\n");
        return -1;
    }
    rt_memset(pGmac->rx_queue[q_no].rx_skbuff_dma, 0, sizeof(u32_t) * malloc_size);

    pGmac->rx_queue[q_no].p_raw_desc = (void *)fh_dma_mem_malloc_align(sizeof(struct dwcqos_dma_desc) * pGmac->rx_queue[q_no].desc_size,  CACHE_LINE_SIZE, "gmac");
    if (!pGmac->rx_queue[q_no].p_raw_desc)
    {
        rt_kprintf("[rx alloc] :: no mem for p_skbuf\n");
        rt_free(pGmac->rx_queue[q_no].rx_skbuff_dma);
        rt_free(pGmac->rx_queue[q_no].p_skbuf);
        return -1;
    }

    pGmac->rx_queue[q_no].p_descs = pGmac->rx_queue[q_no].p_raw_desc;
    pGmac->rx_queue[q_no].descs_phy_base_addr = (u32_t)pGmac->rx_queue[q_no].p_descs;
    pGmac->rx_queue[q_no].descs_phy_tail_addr = pGmac->rx_queue[q_no].descs_phy_base_addr + (pGmac->rx_queue[q_no].desc_size) * sizeof(struct dwcqos_dma_desc);
    pGmac->rx_queue[q_no].hw_queue_size = pGmac->hw_fea.rx_fifo_size / pGmac->hw_fea.rxq_num;
    pGmac->rx_queue[q_no].desc_xfer_max_size = MAX_EACH_DESC_REV_SIZE;
    rt_memset(pGmac->rx_queue[q_no].p_descs, 0, pGmac->rx_queue[q_no].desc_size * sizeof(struct dwcqos_dma_desc));
    for (i = 0, p_descs = pGmac->rx_queue[q_no].p_descs; i < malloc_size; i++)
    {
        p = pbuf_alloc(PBUF_RAW, pGmac->rx_queue[q_no].desc_xfer_max_size + CACHE_LINE_SIZE, PBUF_RAM);
        if (p == NULL)
        {
            rt_kprintf("%s: Rx init fails; pbuf is NULL\n", __func__);
            for (j = i; j > 0; j--)
            {
                pbuf_free((struct pbuf *)pGmac->rx_queue[q_no].p_skbuf[j]);
                pGmac->rx_queue[q_no].p_skbuf[j] = 0;
                pGmac->rx_queue[q_no].rx_skbuff_dma[j] = 0;
            }

            rt_free(pGmac->rx_queue[q_no].rx_skbuff_dma);
            rt_free(pGmac->rx_queue[q_no].p_skbuf);
            return -RT_ENOMEM;
        }
        pGmac->rx_queue[q_no].p_skbuf[i] = (u32_t)p;
        pGmac->rx_queue[q_no].rx_skbuff_dma[i] = (((uint32_t)(p->payload) +
                                         (CACHE_LINE_SIZE - 1)) & (~(CACHE_LINE_SIZE - 1)));
        p->payload = (void *)pGmac->rx_queue[q_no].rx_skbuff_dma[i];
        BD_CACHE_INVALIDATE((rt_uint32_t)pGmac->rx_queue[q_no].rx_skbuff_dma[i], pGmac->rx_queue[q_no].desc_xfer_max_size);
        p_descs[i].desc0 = (u32_t) (pGmac->rx_queue[q_no].rx_skbuff_dma[i]);
        p_descs[i].desc1 = 0;
        p_descs[i].desc2 = 0;
        p_descs[i].desc3 = DWCQOS_DMA_RDES3_BUF1V | DWCQOS_DMA_RDES3_OWN | DWCQOS_DMA_RDES3_INTE;
        BD_CACHE_WRITEBACK((rt_uint32_t)&p_descs[i], sizeof(struct dwcqos_dma_desc));

    }
    return 0;

}

static struct dwcqos_dma_desc *fh_qos_get_rx_desc_cur(struct dw_qos *pGmac, u32_t q_no, u32_t *index)
{
    struct dwcqos_dma_desc *p_desc;

    p_desc = &pGmac->rx_queue[q_no].p_descs[pGmac->rx_queue[q_no].cur_idx];
    /* check hw desc dma own */
    BD_CACHE_INVALIDATE((rt_uint32_t)p_desc, sizeof(struct dwcqos_dma_desc));
    if (p_desc->desc3 & DWCQOS_DMA_RDES3_OWN)
    {
        return 0;
    }
    *index = pGmac->rx_queue[q_no].cur_idx;
    pGmac->rx_queue[q_no].cur_idx++;
    pGmac->rx_queue[q_no].cur_idx %= pGmac->rx_queue[q_no].desc_size;

    return p_desc;
}

#if 0
static struct dwcqos_dma_desc *__fh_qos_get_rx_desc(struct dw_qos *pGmac, u32_t q_no, u32_t index)
{
    struct dwcqos_dma_desc  *p_desc;

    p_desc = &pGmac->rx_queue[q_no].p_descs[index];
    return p_desc;
}
#endif

static u32_t get_rx_skbuf_vadd(struct dw_qos *pGmac, u32_t q_no, u32_t index)
{
    return (u32_t) pGmac->rx_queue[q_no].p_skbuf[index % pGmac->rx_queue[q_no].desc_size];
}

#if 0
static u32_t get_rx_skbuf_padd(struct dw_qos *pGmac, u32_t q_no, u32_t index)
{
    return (u32_t) pGmac->rx_queue[q_no].rx_skbuff_dma[index % pGmac->rx_queue[q_no].desc_size];
}
#endif

static void set_rx_skbuf_vadd(struct dw_qos *pGmac, u32_t q_no, u32_t index, u32_t vadd)
{
    pGmac->rx_queue[q_no].p_skbuf[index % pGmac->rx_queue[q_no].desc_size] = vadd;
}

static void set_rx_skbuf_padd(struct dw_qos *pGmac, u32_t q_no, u32_t index, u32_t padd)
{
    pGmac->rx_queue[q_no].rx_skbuff_dma[index % pGmac->rx_queue[q_no].desc_size] = padd;
}

static void debug_rx_desc(void)
{
    struct dwcqos_dma_desc  *p_desc;
    int i;
    struct dwcqos_dma_desc *p_desc_add;
    struct dw_qos *pGmac;
    rt_device_t dev = rt_device_find("e0");

    if (dev == RT_NULL)
        return;

    pGmac = (struct dw_qos *)dev->user_data;
    u32_t q_no = 0;

    p_desc_add = (struct dwcqos_dma_desc *)pGmac->rx_queue[q_no].descs_phy_base_addr;
    rt_kprintf("q_no = %04d : cur_idx = %d : dty_idx = %d\n", q_no, pGmac->rx_queue[q_no].cur_idx, pGmac->rx_queue[q_no].dirty_idx);
    for (i = 0; i < pGmac->rx_queue[q_no].desc_size; i++)
    {
        p_desc = &pGmac->rx_queue[q_no].p_descs[i];
        BD_CACHE_INVALIDATE((rt_uint32_t)p_desc, sizeof(struct dwcqos_dma_desc));
        rt_kprintf("[index : %d] :: desc0 : desc1 : desc2 : desc3 : p_add = %08x : %08x : %08x : %08x %08x\n", i,
        p_desc->desc0, p_desc->desc1, p_desc->desc2, p_desc->desc3, (u32_t)&p_desc_add[i]);
    }

}

static void fh_qos_rx_desc_index_init(struct dw_qos *pGmac, u32_t q_no)
{
    pGmac->rx_queue[q_no].cur_idx = 0;
    pGmac->rx_queue[q_no].dirty_idx = 0;
}

static int dwcqos_reset_dma_hw(struct dw_qos *pGmac)
{
    int ret = -1;
    int i = 5000;
    u32_t reg;

    dw_writel(pGmac, dma.mode, 1 << 0);
    do
    {
        i--;
        reg = dw_readl(pGmac, dma.mode);
    } while ((reg & DWCQOS_DMA_MODE_SWR) && i);
    /* We might experience a timeout if the chip clock mux is broken */
    if (!i)
        rt_kprintf("DMA reset timed out!\n");
    else
        ret = 0;

    return ret;
}

static void dwcqos_set_dma_mode(struct dw_qos *pGmac, u32_t val)
{
    dw_writel(pGmac, dma.mode, val);
}

static void dwcqos_configure_bus(struct dw_qos *pGmac)
{
    /* dw_writel(pGmac, dma.sysbus_mode, 2 << 16 | 7 << 1); */
    dw_writel(pGmac, dma.sysbus_mode, 0x303100c);
}

static void dwcqos_dma_tx_enable_set(struct dw_qos *pGmac, u32_t dma_no, u32_t enable)
{
    u32_t ret;

    ret = dw_readl(pGmac, dma.chan[dma_no].tx_control);
    ret &= ~(1 << 0);
    ret |= (enable << 0);
    dw_writel(pGmac, dma.chan[dma_no].tx_control, ret);
}

static void dwcqos_dma_rx_enable_set(struct dw_qos *pGmac, u32_t dma_no, u32_t enable)
{
    u32_t ret;

    ret = dw_readl(pGmac, dma.chan[dma_no].rx_control);
    ret &= ~(1 << 0);
    ret |= (enable << 0);
    dw_writel(pGmac, dma.chan[dma_no].rx_control, ret);
}

static void dwcqos_dma_isr_enable_set(struct dw_qos *pGmac, u32_t dma_no, u32_t val)
{
    u32_t ret;

    ret = dw_readl(pGmac, dma.chan[dma_no].interrupt_enable);
    ret &= ~(0xffff << 0);
    ret |= (val << 0);
    dw_writel(pGmac, dma.chan[dma_no].interrupt_enable, ret);
}

static u32_t dwcqos_dma_isr_get(struct dw_qos *pGmac, u32_t dma_no)
{
    u32_t ret;

    ret = dw_readl(pGmac, dma.chan[dma_no].interrupt_enable);
    return ret;
}

static void dwcqos_dma_isr_tx_set(struct dw_qos *pGmac, u32_t dma_no, u32_t enable)
{
    u32_t ret;

    ret = dwcqos_dma_isr_get(pGmac, dma_no);
    ret &= ~(DWCQOS_DMA_CH_IE_TIE);
    if (enable)
        ret |= DWCQOS_DMA_CH_IE_TIE;

    dwcqos_dma_isr_enable_set(pGmac, dma_no, ret);
}


static void dwcqos_dma_isr_rx_set(struct dw_qos *pGmac, u32_t dma_no, u32_t enable)
{
    u32_t ret;

    ret = dwcqos_dma_isr_get(pGmac, dma_no);
    ret &= ~(DWCQOS_DMA_CH_IE_RIE);
    if (enable)
        ret |= DWCQOS_DMA_CH_IE_RIE;

    dwcqos_dma_isr_enable_set(pGmac, dma_no, ret);
}

static u32_t dwcqos_dma_isr_status_get(struct dw_qos *pGmac, u32_t dma_no)
{
    u32_t ret;

    ret = dw_readl(pGmac, dma.chan[dma_no].status);
    return ret;
}


static void dwcqos_dma_isr_status_set(struct dw_qos *pGmac, u32_t dma_no, u32_t val)
{
    dw_writel(pGmac, dma.chan[dma_no].status, val);
}

static void dwcqos_set_hw_mac_addr(struct dw_qos *pGmac, u8_t *mac)
{
    u32_t macHigh = 0;
    u32_t macLow = 0;

    macHigh = mac[5] << 8 | mac[4];
    macLow = mac[3] << 24 | mac[2] << 16 | mac[1] << 8 | mac[0];
    dw_writel(pGmac, mac.mac_addr[0].addr_hi, macHigh);
    dw_writel(pGmac, mac.mac_addr[0].addr_lo, macLow);
}

#if 0
static void dwcqos_disable_umac_addr(struct dw_qos *pGmac, unsigned int reg_n)
{
    /* Do not disable MAC address 0 */
    if (reg_n != 0)
        dw_writel(pGmac, mac.mac_addr[reg_n].addr_hi, 0);
}
#endif

static void dwcqos_set_hw_mac_filter(struct dw_qos *pGmac, u32_t filter)
{
    dw_writel(pGmac, mac.packet_filter, filter);
}

static void dwcqos_set_hw_mac_interrupt(struct dw_qos *pGmac, u32_t isr)
{
    dw_writel(pGmac, mac.interrupt_enable, isr);
}

static void dwcqos_set_hw_mac_tx_flowctrl(struct dw_qos *pGmac, u32_t q_no, u32_t data)
{
    dw_writel(pGmac, mac.tx_flow_ctrl[q_no], data);
}

static int dwcqos_get_hw_mac_tx_flowctrl(struct dw_qos *pGmac, u32_t q_no)
{
    return dw_readl(pGmac, mac.tx_flow_ctrl[q_no]);
}

static int dwcqos_get_mtl_rx_operation(struct dw_qos *pGmac, u32_t q_no)
{
    if (q_no == 0)
        return dw_readl(pGmac, mtl.q_0.rxq_operation_mode);
    else
        return dw_readl(pGmac, mtl.q_x[q_no - 1].rxq_operation_mode);
}

static void dwcqos_set_mtl_rx_operation(struct dw_qos *pGmac, u32_t q_no, u32_t data)
{
    if (q_no == 0)
        dw_writel(pGmac, mtl.q_0.rxq_operation_mode, data);
    else
        dw_writel(pGmac, mtl.q_x[q_no - 1].rxq_operation_mode, data);
}

/* mac will parse pause frame,then halt mac transmitter for pause time */
static void dwcqos_set_hw_parse_pause_frame(struct dw_qos *pGmac, u32_t enable)
{
    int ret;

    ret = dw_readl(pGmac, mac.rx_flow_ctrl);
    ret &= ~(1 << 0);
    if (enable)
        ret |= (1 << 0);
    dw_writel(pGmac, mac.rx_flow_ctrl, ret);
}

static u32_t dwcqos_hw_halt_xmit(struct dw_qos *pGmac)
{
    int ret;
    u32_t temp;

    temp = dw_readl(pGmac, mac.config);
    do
    {
        ret = dw_readl(pGmac, mac.config);
        ret &= ~(3);
        dw_writel(pGmac, mac.config, ret);
        ret = dw_readl(pGmac, mac.config);
    } while (ret & 3);
    return temp & 3;
}

/* tx pause, sw could set reg to generate a pause frame. */
/* or when rx fifo exceed lev, auto send pause frame. but need hw cfg support */
static void dwcqos_set_hw_tx_auto_flowctrl(struct dw_qos *pGmac, u32_t rxq_no, u32_t txq_no,
u32_t pause_time, u32_t enable)
{
    u32_t ret;
    u32_t rfa;
    u32_t rfd;
    /* first check rx fifo lev. should >= 4KB */
    ret = (u32_t)dwcqos_get_mtl_rx_operation(pGmac, rxq_no);
    ret = ((ret & 0x3ff00000) >> 20) + 1;
    ret *= 256;
    if (ret < FLOW_CTL_MIN_RX_FIFO)
    {
        rt_kprintf("[rx%d] :: failed to set tx flow, cause min rx 0x%x\n",
        rxq_no, FLOW_CTL_MIN_RX_FIFO);
        return;
    }
    if (pause_time > FLOW_CTL_MAX_PAUSE_TIME)
    {
        rt_kprintf("[tx%d] :: failed to set tx flow, cause max pause time 0x%x\n",
        txq_no, FLOW_CTL_MAX_PAUSE_TIME);
        return;
    }
    /* windonw active,here set full - 1K */
    rfa = 0;
    /* windonw deactive, here set empty + 1K */
    /* 0 means 1K, step is 0.5K */
    rfd = (ret - (FLOW_CTL_DEACTIVE_RX_FIFO + 0x400)) / 0x200;
    /*rt_kprintf("deactive lev [%d]\n",ret - (0x400 + rfd * 0x200));*/
    /*rt_kprintf("active lev [%d]\n",ret - (0x400 + rfa * 0x200));*/

    /* set mtl */
    ret = (u32_t)dwcqos_get_mtl_rx_operation(pGmac, rxq_no);
    /* clear enable | rfa | rfd */
    ret &= ~((0x000fff00) | (1 << 7));
    ret |= (rfa << 8) | (rfd << 14);
    if (enable)
        ret |= 1 << 7;

    dwcqos_set_mtl_rx_operation(pGmac, rxq_no, ret);
    /* mac */
    ret = dwcqos_get_hw_mac_tx_flowctrl(pGmac, txq_no);
    /* clear enable | pause time */
    ret &= ~((1 << 1) | (0xffff << 16));
    ret |= pause_time << 16;
    if (enable)
        ret |= 1 << 1;
    dwcqos_set_hw_mac_tx_flowctrl(pGmac, txq_no, ret);
}


static void dwcqos_set_hw_mac_config(struct dw_qos *pGmac, u32_t config)
{
    dw_writel(pGmac, mac.config, config);
}

static int dwcqos_get_hw_mac_config(struct dw_qos *pGmac)
{
    return dw_readl(pGmac, mac.config);
}

static void dwcqos_mac_spd_port_set(struct dw_qos *pGmac, int spd)
{
    int ret;

    ret = dwcqos_get_hw_mac_config(pGmac);
    ret &= ~(3 << 14);
    /*1000M is 0*/
    if (spd == 100)
        ret |= (1 << 15 | 1 << 14);
    if (spd == 10)
        ret |= (1 << 15);

    dwcqos_set_hw_mac_config(pGmac, ret);
}

static void dwcqos_mac_duplex_set(struct dw_qos *pGmac, int duplex)
{
    int ret;

    ret = dwcqos_get_hw_mac_config(pGmac);
    ret &= ~(1 << 13);
    ret |= (duplex << 13);

    dwcqos_set_hw_mac_config(pGmac, ret);
}

/* maybe could set a map to route diff packet to diff rx queue.. */
/* here set last queue rx mul and broadcast packet */
static void dwcqos_mac_route_rxqueue_set(struct dw_qos *pGmac)
{
    #if (0)
    int val = queue_no << 0 | queue_no << 4 | queue_no << 8
    | queue_no << 12 | queue_no << 16 | 1 << 20;
    dw_writel(pGmac, mac.rxq_ctrl[1], val);
    #endif
    dw_writel(pGmac, mac.rxq_ctrl[1], (1 << 20) | ((pGmac->hw_fea.rxq_num - 1) << 16));
}

static void dwcqos_queue_enable_set(struct dw_qos *pGmac, u32_t queue_no, u32_t enable)
{
    int ret;

    ret = dw_readl(pGmac, mac.rxq_ctrl[0]);
    ret &= ~(3 << (queue_no * 2));
    if (enable)
    {
        ret |= 2 << (queue_no * 2);
    }
    dw_writel(pGmac, mac.rxq_ctrl[0], ret);
}

static void dwcqos_mac_tx_enable(struct dw_qos *pGmac)
{
    int ret;

    ret = dwcqos_get_hw_mac_config(pGmac);
    ret |= DWCQOS_MAC_TX_POS;
    dwcqos_set_hw_mac_config(pGmac, ret);
}

static void dwcqos_mac_tx_disable(struct dw_qos *pGmac)
{
    int ret;

    do
    {
        ret = dw_readl(pGmac, mac.config);
        ret &= ~DWCQOS_MAC_TX_POS;
        dw_writel(pGmac, mac.config, ret);
        ret = dw_readl(pGmac, mac.config);
    } while (ret & DWCQOS_MAC_TX_POS);
}

static void dwcqos_mac_rx_enable(struct dw_qos *pGmac)
{
    int ret;

    ret = dwcqos_get_hw_mac_config(pGmac);
    ret |= DWCQOS_MAC_RX_POS;
    dwcqos_set_hw_mac_config(pGmac, ret);
}

static void dwcqos_mac_rx_disable(struct dw_qos *pGmac)
{
    int ret;

    do
    {
        ret = dw_readl(pGmac, mac.config);
        ret &= ~DWCQOS_MAC_RX_POS;
        dw_writel(pGmac, mac.config, ret);
        ret = dw_readl(pGmac, mac.config);
    } while (ret & DWCQOS_MAC_RX_POS);
}

static int dwcqos_get_hw_mtl_txqx_debug(struct dw_qos *pGmac, u32_t q_no)
{
    int ret;

    if (q_no == 0)
        ret = dw_readl(pGmac, mtl.q_0.txq_debug);
    else
        ret = dw_readl(pGmac, mtl.q_x[q_no - 1].txq_debug);
    return ret;
}


static int dwcqos_get_hw_mtl_rxqx_debug(struct dw_qos *pGmac, u32_t q_no)
{
    int ret;

    if (q_no == 0)
        ret = dw_readl(pGmac, mtl.q_0.rxq_debug);
    else
        ret = dw_readl(pGmac, mtl.q_x[q_no - 1].rxq_debug);
    return ret;
}

static int dwcqos_get_hw_mtl_isr_status(struct dw_qos *pGmac, u32_t q_no)
{
    int ret;

    if (q_no == 0)
        ret = dw_readl(pGmac, mtl.q_0.interrupt_control_status);
    else
        ret = dw_readl(pGmac, mtl.q_x[q_no - 1].interrupt_control_status);
    return ret;
}


static void dwcqos_set_hw_mtl_isr_status(struct dw_qos *pGmac, u32_t q_no, u32_t val)
{

    if (q_no == 0)
        dw_writel(pGmac, mtl.q_0.interrupt_control_status, val);
    else
        dw_writel(pGmac, mtl.q_x[q_no - 1].interrupt_control_status, val);
}

#if 0
static void dwcqos_dma_chan_set_mss(struct dw_qos *pGmac, u32_t q_no, u32_t val)
{
    u32_t ret;

    ret = dw_readl(pGmac, dma.chan[q_no].control);
    ret &= ~(0x3fff);
    ret |= (val & 0x3fff);
    dw_writel(pGmac, dma.chan[q_no].control, ret);
}
#endif

static int dwcqos_dma_get_interrupt_status(struct dw_qos *pGmac)
{
    int ret;

    ret = dw_readl(pGmac, dma.interrupt_status);
    return ret;
}

static int dwcqos_mac_get_interrupt_status(struct dw_qos *pGmac)
{
    int ret;
    ret = dw_readl(pGmac, mac.interrupt_status);
    return ret;
}

static void dwcqos_init_mtl_hw_rxdma_queue_map(struct dw_qos *pGmac)
{
    int q_map_0 = 0;
    int q_map_1 = 0;
    int i;

    DWCQOS_FOR_EACH_QUEUE(pGmac->hw_fea.rxq_num, i) {
        if (i < 4)
        {
            q_map_0 |= i << i * 8;
        }
        else
        {
            q_map_1 |= (i-4) << (i-4) * 8;
        }
    }
    dw_writel(pGmac, mtl.rxq_dma_map0, q_map_0);
    dw_writel(pGmac, mtl.rxq_dma_map1, q_map_1);

}

static void dwcqos_dma_desc_init(struct dw_qos *pGmac)
{
    int i;

    DWCQOS_FOR_EACH_QUEUE(pGmac->hw_fea.txq_num, i) {
        dw_writel(pGmac, dma.chan[i].txdesc_list_laddr, pGmac->tx_queue[i].descs_phy_base_addr);
        /* do not set tail point here...or if you only have one tx desc, the hw will get in suspend mode. */
        /* dw_writel(pGmac, dma.chan[i].txdesc_tail_pointer, pGmac->tx_queue[i].descs_phy_tail_addr); */
        dw_writel(pGmac, dma.chan[i].txdesc_ring_len, pGmac->tx_queue[i].desc_size - 1);
    }

    DWCQOS_FOR_EACH_QUEUE(pGmac->hw_fea.rxq_num, i) {
        dw_writel(pGmac, dma.chan[i].rxdesc_list_laddr, pGmac->rx_queue[i].descs_phy_base_addr);
        dw_writel(pGmac, dma.chan[i].rxdesc_tail_pointer, pGmac->rx_queue[i].descs_phy_tail_addr);
        dw_writel(pGmac, dma.chan[i].rxdesc_ring_len, pGmac->rx_queue[i].desc_size - 1);
    }
}

static void dwcqos_dma_chan_init(struct dw_qos *pGmac)
{
    int i;
    int reg;

    DWCQOS_FOR_EACH_QUEUE(pGmac->hw_fea.tx_dma_num, i) {
        /* dma chan0 ctrl. dma chan0 tx ctrl. */
        reg = 2 << 16;
        dw_writel(pGmac, dma.chan[i].control, 0x110000);
        if (pGmac->hw_fea.tso_flag)
            reg |= DWCQOS_DMA_CH_TX_TSE;
        dw_writel(pGmac, dma.chan[i].tx_control, reg);
        /* dma chan0 interrupt.. */
        dwcqos_dma_isr_enable_set(pGmac, i, 0);
        /* clear isr int status. */
        dwcqos_dma_isr_status_set(pGmac, i, 0xffffffff);
    }

    DWCQOS_FOR_EACH_QUEUE(pGmac->hw_fea.rx_dma_num, i) {
        /* dma rx ctrl.. */
        dw_writel(pGmac, dma.chan[i].rx_control, 2 << 16 | 2048 << 1);
    }


    DWCQOS_FOR_EACH_QUEUE(pGmac->hw_fea.tx_dma_num, i) {
        dwcqos_dma_tx_enable_set(pGmac, i, 0);
    }

    DWCQOS_FOR_EACH_QUEUE(pGmac->hw_fea.rx_dma_num, i) {
        dwcqos_dma_rx_enable_set(pGmac, i, 0);
    }

}

static void refix_feature(struct dw_qos *pGmac)
{
    pGmac->hw_fea.rxq_num = 1;
    pGmac->hw_fea.rx_dma_num = 1;
    pGmac->hw_fea.txq_num = 1;
    pGmac->hw_fea.tx_dma_num = 1;

}

static void parse_hw_feature(struct dw_qos *pGmac)
{
    /* queue num.. */
    unsigned int tx_fifo_size, rx_fifo_size;

    pGmac->hw_fea.rxq_num = (pGmac->hw_fea.feature2 & 0xf) + 1;
    pGmac->hw_fea.txq_num = ((pGmac->hw_fea.feature2 >> 6) & 0xf) + 1;
    pGmac->hw_fea.tx_dma_num = ((pGmac->hw_fea.feature2 >> 18) & 0xf) + 1;
    pGmac->hw_fea.rx_dma_num = ((pGmac->hw_fea.feature2 >> 12) & 0xf) + 1;
    pGmac->hw_fea.tso_flag = (pGmac->hw_fea.feature1  & (1 << 18)) ;

    refix_feature(pGmac);
    /* fifo size.. */
    tx_fifo_size  = (pGmac->hw_fea.feature1 >> 6) & 0x1f;
    rx_fifo_size = (pGmac->hw_fea.feature1 >> 0) & 0x1f;
    pGmac->hw_fea.tx_fifo_size = 128 << tx_fifo_size;
    pGmac->hw_fea.rx_fifo_size = 128 << rx_fifo_size;
    /*
    rt_kprintf("[queue ] : %x : %x; [fifo size] : %x : %x\n",pGmac->hw_fea.rxq_num,pGmac->hw_fea.txq_num,
    pGmac->hw_fea.tx_fifo_size, pGmac->hw_fea.rx_fifo_size);
    rt_kprintf("[dma no] :: tx : rx = %x : %x\n",pGmac->hw_fea.tx_dma_num, pGmac->hw_fea.rx_dma_num);
    */
}

static void fh_qos_mac_add_init(struct dw_qos *pGmac)
{
    /* may be get from uboot.here do nothing */
}

static void dwcqos_desc_init(struct dw_qos *pGmac)
{
    int i;

    DWCQOS_FOR_EACH_QUEUE(pGmac->hw_fea.txq_num, i) {
        fh_qos_txq_malloc_desc(pGmac, i, TX_DESC_NUM);
        fh_qos_tx_desc_index_init(pGmac, i);
        pGmac->tx_queue[i].pGmac = pGmac;
    }

    DWCQOS_FOR_EACH_QUEUE(pGmac->hw_fea.rxq_num, i) {
        /* CAUTION!!!!! here set net_rx_packets for q1-qn..means that only one rx queue could work... */
        fh_qos_rxq_malloc_desc(pGmac, i, RX_DESC_NUM);
        fh_qos_rx_desc_index_init(pGmac, i);
        pGmac->rx_queue[i].pGmac = pGmac;
    }
}

#ifdef RT_USING_DEVICE_OPS
const static struct rt_device_ops fh_gmac_ops = {
    .init    = rt_fh_gmac_init,
    .open    = rt_fh_gmac_open,
    .close   = rt_fh_gmac_close,
    .read    = rt_fh_gmac_read,
    .write   = rt_fh_gmac_write,
    .control = rt_fh_gmac_control
};
#endif

static rt_err_t rt_fh81_gmac_tx(rt_device_t dev, struct pbuf *p)
{
    struct dw_qos *pGmac;
    int qno;
    /* struct net_tx_queue *tx_queue; */
    struct dwcqos_dma_desc *p_desc;
    u32_t index;
    u32_t sk_buf_dma_add = 0;
    rt_uint8_t *sk_buf;
    struct pbuf *temp_pbuf;
    u32_t ret = 0;

    pGmac = (struct dw_qos *)dev->user_data;
    qno = pGmac->active_queue_index;
    /* tx_queue = &pGmac->tx_queue[qno]; */

    if (p)
    {
        rt_sem_take(&pGmac->tx_lock, RT_WAITING_FOREVER);
        p_desc = fh_qos_get_tx_desc_cur(pGmac, qno, &index);
        if (!p_desc)
        {
            rt_kprintf("[%s-%d]: get tx desc failed.\n", __func__, __LINE__);
            rt_sem_release(&pGmac->tx_lock);
            return -1;
        }

        sk_buf_dma_add = get_tx_skbuf_padd(pGmac, qno, index);
        /* sk_buf_dma_add = (u8_t *)((sk_buf_dma_add + CACHE_LINE_SIZE - 1) & (~(CACHE_LINE_SIZE - 1))); */

        sk_buf = ((rt_uint8_t *)sk_buf_dma_add) + 2;
        for (temp_pbuf = p; temp_pbuf != NULL; temp_pbuf = temp_pbuf->next)
        {
            rt_memcpy((void *)sk_buf, temp_pbuf->payload, temp_pbuf->len);
            sk_buf += temp_pbuf->len;
        }

        BD_CACHE_INVALIDATE((rt_uint32_t)sk_buf_dma_add, p->tot_len + 2);
        p_desc->desc0 = sk_buf_dma_add + 2;
        p_desc->desc1 = 0;
        p_desc->desc2 = p->tot_len;
        p_desc->desc3 = p->tot_len;
        set_first_desc(p_desc);
        set_last_desc(p_desc);
        wmb();
        set_desc_isr_valid(p_desc);
        wmb();
        set_desc_dma_valid(p_desc);
        BD_CACHE_WRITEBACK((rt_uint32_t)p_desc, sizeof(struct dwcqos_dma_desc));

        dwcqos_dma_tx_enable_set(pGmac, qno, 1);
        dwcqos_kick_tx_queue(pGmac, qno);

        ret = dw_readl(pGmac, mac.config);
        dw_writel(pGmac, mac.config, ret);

        rt_sem_release(&pGmac->tx_lock);

        /* set_tx_skbuf_vadd(pGmac, qno, index, DUMMY_SK_BUFF_FLAG); */
        /* set_tx_skbuf_padd(pGmac, qno, index, (u32_t)sk_buf_dma_add); */
    }
    else
    {
        rt_kprintf("[%s-%d]: pbuf is NULL\n", __func__, __LINE__);
    }

    return RT_EOK;
}

static struct pbuf *rt_fh81_gmac_rx(rt_device_t dev)
{
    struct dw_qos *pGmac;
    struct dwcqos_dma_desc *p_desc;
    struct pbuf *temp_pbuf = RT_NULL;
    struct pbuf *malloc_pbuf = RT_NULL;
    u32_t sk_buf_dma_add;
    int frame_len;
    /* int status; */
    u32_t index;

    pGmac = (struct dw_qos *)dev->user_data;
    int rx_qno = pGmac->active_queue_index;


    rt_sem_take(&pGmac->rx_lock, RT_WAITING_FOREVER);
    p_desc = fh_qos_get_rx_desc_cur(pGmac, rx_qno, &index);
    if (!p_desc)
    {
        dwcqos_dma_isr_rx_set(pGmac, rx_qno, 1);
        rt_sem_release(&pGmac->rx_lock);
        return NULL;
    }

    /* status = check_rx_packet(pGmac, p_desc); */
    frame_len = p_desc->desc3  & 0x7fff;
    frame_len -= ETH_FCS_LEN;
    temp_pbuf = (struct pbuf *)get_rx_skbuf_vadd(pGmac, rx_qno, index);

    temp_pbuf->len = temp_pbuf->tot_len = frame_len;

    malloc_pbuf = pbuf_alloc(PBUF_RAW, pGmac->rx_queue[rx_qno].desc_xfer_max_size + CACHE_LINE_SIZE, PBUF_RAM);
    if (malloc_pbuf == NULL)
    {
        rt_kprintf("%s: Rx init fails; pbuf is NULL\n", __func__);
        rt_sem_release(&pGmac->rx_lock);
        return temp_pbuf;
    }

    sk_buf_dma_add = (u32_t)(((uint32_t)(malloc_pbuf->payload) +
                                        (CACHE_LINE_SIZE - 1)) & (~(CACHE_LINE_SIZE - 1)));
    malloc_pbuf->payload = (void *)sk_buf_dma_add;
    BD_CACHE_INVALIDATE((rt_uint32_t)sk_buf_dma_add, pGmac->rx_queue[rx_qno].desc_xfer_max_size);
    p_desc->desc0 = (u32_t) sk_buf_dma_add;
    p_desc->desc1 = 0;
    p_desc->desc2 = 0;
    p_desc->desc3 = DWCQOS_DMA_RDES3_BUF1V | DWCQOS_DMA_RDES3_OWN | DWCQOS_DMA_RDES3_INTE;
    BD_CACHE_WRITEBACK((rt_uint32_t)p_desc, sizeof(struct dwcqos_dma_desc));
    set_rx_skbuf_vadd(pGmac, rx_qno, index, (u32_t)malloc_pbuf);
    set_rx_skbuf_padd(pGmac, rx_qno, index, (u32_t)sk_buf_dma_add);

    dwcqos_kick_rx_queue(pGmac, rx_qno);

    rt_sem_release(&pGmac->rx_lock);
    return temp_pbuf;
}

static int dwcqos_init_sw(struct dw_qos *pGmac)
{
    pGmac->duplex = GMAC_DUPLEX_FULL;
    pGmac->speed = GMAC_SPEED_100M;
    pGmac->flow_ctrl = FLOW_RX;
    pGmac->phy_interface = QOS_PHY_MODE;
#ifdef RT_USING_DEVICE_OPS
    pGmac->parent.parent.ops       = &fh_gmac_ops;
#else
    pGmac->parent.parent.init      = rt_fh_gmac_init;
    pGmac->parent.parent.open      = rt_fh_gmac_open;
    pGmac->parent.parent.close     = rt_fh_gmac_close;
    pGmac->parent.parent.read      = rt_fh_gmac_read;
    pGmac->parent.parent.write     = rt_fh_gmac_write;
    pGmac->parent.parent.control   = rt_fh_gmac_control;
#endif
    pGmac->parent.parent.user_data = (void *)pGmac;
    pGmac->parent.net.eth.eth_rx = rt_fh81_gmac_rx;
    pGmac->parent.net.eth.eth_tx = rt_fh81_gmac_tx;
    fh_gmac_set_ethtool_ops(&(pGmac->parent));

    fh_qos_mac_add_init(pGmac);

    return 0;
}


static int dwcqos_init_dma_hw(struct dw_qos *pGmac)
{
    dwcqos_reset_dma_hw(pGmac);
    /* set dma isr mode.. */
    dwcqos_set_dma_mode(pGmac, 3 << 16);
    /* 3:dma_sys_bus_mode... */
    dwcqos_configure_bus(pGmac);
    /* 4:desc.... */
    /* 5:desc ring size... */
    /* 6:desc base add and tail addr... */
    dwcqos_dma_desc_init(pGmac);
    /* 7:dma chan0 ctrl. dma chan0 tx ctrl. dma rx ctrl.. */
    /* 8:dma chan0 interrupt.. */
    /* 9:start dma chan with dma chan0 rx/tx ctrl.. */
    dwcqos_dma_chan_init(pGmac);
    return 0;
}

static void dwcqos_kick_tx_queue(struct dw_qos *pGmac, u32_t q_no)
{
    dw_writel(pGmac, dma.chan[q_no].txdesc_tail_pointer, pGmac->tx_queue[q_no].descs_phy_tail_addr);
}

static void dwcqos_kick_rx_queue(struct dw_qos *pGmac, u32_t q_no)
{
    dw_writel(pGmac, dma.chan[q_no].rxdesc_tail_pointer, pGmac->rx_queue[q_no].descs_phy_tail_addr);
}

static void dwcqos_init_mtl_hw(struct dw_qos *pGmac)
{
    int i;
    int tx_queue_size = 0;
    int rx_queue_size = 0;
    /* program tx schedule */
    /* rev arbitration algo */
    dw_writel(pGmac, mtl.operation_mode, 0x60);
    /* dw_writel(pGmac, mtl.operation_mode, 0); */
    /* pro dma map0 and map1 */
    dwcqos_init_mtl_hw_rxdma_queue_map(pGmac);
    /* tx_queue_size = (pGmac->hw_fea.tx_fifo_size / pGmac->hw_fea.txq_num / 256) - 1; */
    /* tx queue operation... */
        /* 1):TSF, TTC */
        /* 2):TXQEN */
        /* 3):TQS */

    DWCQOS_FOR_EACH_QUEUE(pGmac->hw_fea.txq_num, i) {
        tx_queue_size = (pGmac->tx_queue[i].hw_queue_size / 256) - 1;
        if (i == 0)
        {
            dw_writel(pGmac, mtl.q_0.txq_operation_mode, (tx_queue_size << 16) |
            DWCQOS_MTL_TXQ_TXQEN |
            DWCQOS_MTL_TXQ_TSF | (7 << 4));
            dw_writel(pGmac, mtl.q_0.txq_quantum_weight, i);
        }
        else
        {
            dw_writel(pGmac, mtl.q_x[i - 1].txq_operation_mode, (tx_queue_size << 16) |
            DWCQOS_MTL_TXQ_TXQEN |
            DWCQOS_MTL_TXQ_TSF | (7 << 4));
            dw_writel(pGmac, mtl.q_x[i - 1].txq_quantum_weight, i);
        }
    }

    /* rx queue operation... */
        /* 1):RSF, RTC */
        /* 2):RFA RFD */
        /* 3):FEP FUP */
        /* 4):RQS */
    DWCQOS_FOR_EACH_QUEUE(pGmac->hw_fea.rxq_num, i) {
        rx_queue_size = (pGmac->rx_queue[i].hw_queue_size / 256) - 1;
        if (i == 0)
        {
            dw_writel(pGmac, mtl.q_0.rxq_operation_mode, (rx_queue_size << 20) | DWCQOS_MTL_RXQ_RSF | DWCQOS_MTL_RXQ_FUP | DWCQOS_MTL_RXQ_FEP);
            dw_writel(pGmac, mtl.q_0.rxq_control, 0);
            /* dw_writel(pGmac, mtl.q_0.rxq_control, i); */
        }
        else
        {
            dw_writel(pGmac, mtl.q_x[i - 1].rxq_operation_mode, (rx_queue_size << 20) | DWCQOS_MTL_RXQ_RSF | DWCQOS_MTL_RXQ_FUP | DWCQOS_MTL_RXQ_FEP);
            /* dw_writel(pGmac, mtl.q_x[i - 1].rxq_control, i); */
            dw_writel(pGmac, mtl.q_x[i - 1].rxq_control, 0);
        }
    }
}


static void dwcqos_init_mac_hw(struct dw_qos *pGmac)
{
    int i;
    int ret;
    /* mac addr low and hi.. */
    dwcqos_set_hw_mac_addr(pGmac, pGmac->local_mac_address);
    /* mac packet filter.. */
    /* dwcqos_set_hw_mac_filter(pGmac, 0); */
    /* dwcqos_set_hw_mac_filter(pGmac, 1 << 5 | 1 << 9); */
    dwcqos_set_hw_mac_filter(pGmac, 1 << 31);
    /* parse pause frame. */
    dwcqos_set_hw_parse_pause_frame(pGmac, 1);

    DWCQOS_FOR_EACH_QUEUE(pGmac->hw_fea.txq_num, i) {
        dwcqos_set_hw_mac_tx_flowctrl(pGmac, i, 0);
    }
    /* mac interrupt.. */
    dwcqos_set_hw_mac_interrupt(pGmac, 0);

    DWCQOS_FOR_EACH_QUEUE(pGmac->hw_fea.rxq_num, i) {
        dwcqos_queue_enable_set(pGmac, i, 0);
    }
    /* mac config */
    /* mac start later, should sync with phy linkup. */
    ret = 1 << 27 | 1 << 9;
    dwcqos_set_hw_mac_config(pGmac, ret);
}

static int dwcqos_init_hw(struct dw_qos *pGmac)
{
    int i;
    /* init dma... */
    dwcqos_init_dma_hw(pGmac);
    /* init mtl... */
    dwcqos_init_mtl_hw(pGmac);
    /* init mac... */
    dwcqos_init_mac_hw(pGmac);
    /* TBD */
    /* multi queue enable.. */
    /* set route...maybe mul and broad 0: else 1 */
    dwcqos_mac_route_rxqueue_set(pGmac);
    DWCQOS_FOR_EACH_QUEUE(pGmac->hw_fea.rxq_num, i) {
        dwcqos_queue_enable_set(pGmac, i, 1);
    }
    return 0;
}

static void set_first_desc(struct dwcqos_dma_desc *pdesc)
{
    pdesc->desc3 |= DWCQOS_DMA_TDES3_FD;
}

static void set_last_desc(struct dwcqos_dma_desc *pdesc)
{
    pdesc->desc3 |= DWCQOS_DMA_TDES3_LD;
}

#if 0
static void fh_qos_prepare_normal_send(struct dwcqos_dma_desc *pdesc,
u32_t p_buf_add, u32_t len, u32_t total_len, u32_t crc_flag)
{
    pdesc->desc0 |= p_buf_add;
    pdesc->desc2 |= len;
    pdesc->desc3 |= total_len | (crc_flag << 16);
}
#endif

static void set_desc_dma_valid(struct dwcqos_dma_desc *pdesc)
{
    pdesc->desc3 |= DWCQOS_DMA_TDES3_OWN;
}

static void set_desc_isr_valid(struct dwcqos_dma_desc *pdesc)
{
    pdesc->desc2 |= DWCQOS_DMA_TDES2_IOC;
}

#if 0
static u32_t get_desc_data_len(struct dwcqos_dma_desc *pdesc)
{
    u32_t ret = 0;

    if (pdesc->desc1 != 0)
        ret = ((pdesc->desc2 & 0x3fff0000) >> 16);
    if (pdesc->desc0 != 0)
        ret += pdesc->desc2 & 0x3fff;
    return ret;
}
#endif

static void dump_lli(struct dwcqos_dma_desc *desc, u32_t size)
{
    u32_t i;

    rt_kprintf("dump go...\n");
    for (i = 0; i < size; i++)
    {
        rt_kprintf("[desc add] index %d : %08x\n", i, (u32_t)&desc[i]);
        rt_kprintf("data = %08x : %08x : %08x : %08x\n", desc[i].desc0, desc[i].desc1, desc[i].desc2, desc[i].desc3);
    }
}

#if 0
static int cal_tx_desc_require(struct sk_buff *skb, struct net_tx_queue *tx_queue)
{
    /* head need at least 1 */
    int nfrags, frag_size;
    int i;
    skb_frag_t *frag;

    int ret = 1;
    /* just cal frag */
    nfrags = skb_shinfo(skb)->nr_frags;

    for (i = 0; i < nfrags; i++)
    {
        frag = &skb_shinfo(skb)->frags[i];
        frag_size = skb_frag_size(frag);
        ret += (frag_size / tx_queue->desc_xfer_max_size);
        if (frag_size % tx_queue->desc_xfer_max_size)
            ret++;
    }
    return ret;

}
#endif

#if 0
static int qos_tso_xmit(struct sk_buff *skb, struct net_device *ndev)
{
    struct dwcqos_dma_desc *p_desc;
    struct dwcqos_dma_desc *first_desc = 0;
    struct dw_qos *pGmac;
    struct net_tx_queue *tx_queue;
    u32_t sk_buf_dma_add;
    u32_t index;
    int qno;
    int len;
    u32_t each_desc_xfer_size = 0;
    skb_frag_t *frag;
    int i;
    int csum_insertion = 0;
    u32_t valid_desc_size;
    int nfrags;
    struct netdev_queue *txq;
    /* tcp + ip + mac head */
    u32_t proto_hdr_len;
    u32_t payload_len, mss;
    u32_t consumed_size;
    u32_t frag_size;
    u32_t req_desc;

    proto_hdr_len = skb_transport_offset(skb) + tcp_hdrlen(skb);
    csum_insertion = (skb->ip_summed == CHECKSUM_PARTIAL) ? 3 : 0;
    pGmac = netdev_priv(ndev);
    qno = skb_get_queue_mapping(skb);
    tx_queue = &pGmac->tx_queue[qno];
    txq = netdev_get_tx_queue(ndev, qno);
    mss = skb_shinfo(skb)->gso_size;

    spin_lock(&tx_queue->tx_lock);
    payload_len = skb_headlen(skb) - proto_hdr_len; /* no frags */
    valid_desc_size = fh_qos_tx_desc_avail(pGmac, qno);
    nfrags = skb_shinfo(skb)->nr_frags;

    /* check head payload len; */
    if (payload_len > tx_queue->desc_xfer_max_size)
    {
        rt_kprintf("payload_len %d > max size %d\n", payload_len, tx_queue->desc_xfer_max_size);
        spin_unlock(&tx_queue->tx_lock);
        return NETDEV_TX_BUSY;
    }

    req_desc = cal_tx_desc_require(skb, tx_queue);

    if (unlikely(valid_desc_size < req_desc))
    {
        netif_tx_stop_queue(txq);
        /* This is a hard error, log it. */
        rt_kprintf("%s: BUG! Tx Ring full when queue awake \n",
            __func__);

        rt_kprintf("%x : %x : %x : %x\n", valid_desc_size, req_desc, pGmac->tx_queue[qno].cur_idx, pGmac->tx_queue[qno].dirty_idx);
        spin_unlock(&tx_queue->tx_lock);
        return NETDEV_TX_BUSY;
    }
    if (tx_queue->mss != mss)
    {
        tx_queue->mss = mss;
        /* first set ctxt desc */
        p_desc = fh_qos_get_tx_desc_cur(pGmac, qno, &index);
        /* rec first desc, set dma valid at the end... */
        first_desc = p_desc;
        p_desc->desc0 = 0;
        p_desc->desc1 = 0;
        p_desc->desc2 = mss;
        p_desc->desc3 = DWCQOS_DMA_TDES3_CTXT | DWCQOS_DMA_TDES3_TCMSSV;
        /* set_desc_dma_valid(p_desc); */
        set_tx_skbuf_vadd(pGmac, qno, index, DUMMY_SK_BUFF_FLAG);
        set_tx_skbuf_padd(pGmac, qno, index, 0);
        /*dump_lli(p_desc, 1);*/
    }
    /* set head desc */
    p_desc = fh_qos_get_tx_desc_cur(pGmac, qno, &index);
    /* rec first desc, set dma valid at the end... */
    if (!first_desc)
        first_desc = p_desc;
    else
        set_desc_dma_valid(p_desc);

    len = skb_headlen(skb);
    sk_buf_dma_add = dma_map_single(pGmac->dev, skb->data, len, DMA_TO_DEVICE);
    /* 1: head desc */
    p_desc->desc0 |= sk_buf_dma_add;
    p_desc->desc1 = 0;
    p_desc->desc2 |= proto_hdr_len;
    p_desc->desc3 |= 1 << 18 | ((tcp_hdrlen(skb) / 4) << 19) | (skb->len - proto_hdr_len);
    set_first_desc(p_desc);
    set_tx_skbuf_vadd(pGmac, qno, index, (u32_t)skb);
    set_tx_skbuf_padd(pGmac, qno, index, (u32_t)sk_buf_dma_add);
    /*dump_lli(p_desc, 1);*/

    if ((len - proto_hdr_len) != 0)
    {
        p_desc->desc1 |= sk_buf_dma_add + proto_hdr_len;
        p_desc->desc2 |= (len - proto_hdr_len) << 16;
    }
    /*dump_lli(p_desc, 1);*/
    /* 3: frag data */
    for (i = 0; i < nfrags; i++)
    {
        frag = &skb_shinfo(skb)->frags[i];
        frag_size = skb_frag_size(frag);
        sk_buf_dma_add = skb_frag_dma_map(pGmac->ndev->dev.parent,
                frag, 0, frag_size, DMA_TO_DEVICE);

        consumed_size = 0;
        while (consumed_size < frag_size)
        {
            each_desc_xfer_size = min_t(size_t, frag_size - consumed_size, tx_queue->desc_xfer_max_size);
            /* len -= each_desc_xfer_size; */
            p_desc = fh_qos_get_tx_desc_cur(pGmac, qno, &index);
            fh_qos_prepare_normal_send(p_desc, sk_buf_dma_add + consumed_size, each_desc_xfer_size, skb->len - proto_hdr_len, 0);
            set_desc_dma_valid(p_desc);
            set_tx_skbuf_vadd(pGmac, qno, index, DUMMY_SK_BUFF_FLAG);
            set_tx_skbuf_padd(pGmac, qno, index, sk_buf_dma_add + consumed_size);
            consumed_size += each_desc_xfer_size;
        }
    }
    set_last_desc(p_desc);
    /* last desc make isr go */
    set_desc_isr_valid(p_desc);
    set_desc_dma_valid(first_desc);

    netdev_tx_sent_queue(txq, skb->len);
    dwcqos_kick_tx_queue(pGmac, qno);
    spin_unlock(&tx_queue->tx_lock);
    return NETDEV_TX_OK;
}
#endif

void check_tx_done_desc(struct dwcqos_dma_desc *p_desc)
{
    if (p_desc->desc3 & (1 << 28))
    {
        if (p_desc->desc3 & (1 << 15))
        {
            rt_kprintf("[tx] :: got err %x\n", p_desc->desc3);
        }
    }
}

#if 0
static int check_rx_packet(struct dw_qos *pGmac,
struct dwcqos_dma_desc *p_desc)
{
    if ((p_desc->desc3 & DWCQOS_DMA_RDES3_ES) ||
        (p_desc->desc1 & DWCQOS_DMA_RDES1_IPCE))
    {
        rt_kprintf("rx err desc3 : desc1 = %x : %x\n", p_desc->desc3, p_desc->desc1);
        return discard_frame;
    }
    return csum_none;
}
#endif

static void dwcqos_mtl_isr_process(struct dw_qos *pGmac)
{
    u32_t qno;
    int ret;

    DWCQOS_FOR_EACH_QUEUE(pGmac->hw_fea.rxq_num, qno) {
        ret = dwcqos_get_hw_mtl_isr_status(pGmac, qno);
        rt_kprintf("[mtl isr] :: %x\n", ret);
        dwcqos_set_hw_mtl_isr_status(pGmac, qno, ret);
    }
}

static void dwcqos_read_mmc_counters(struct dw_qos *pGmac, u32_t rx_mask,
                      u32_t tx_mask, u32_t lpc_mask)
{
    struct dw_mac_mmc_regs mmc;
    struct dw_mac_mmc_regs *regs;

    regs = &(((struct dw_qos_regs_map *)pGmac->regs)->mac.mmc);
    /* TBD rec hw status must read hw reg to clear isr status */

    rt_memcpy(&mmc, regs, sizeof(struct dw_mac_mmc_regs));
}

static void dwcqos_mac_isr_process(struct dw_qos *pGmac)
{
    u32_t cause;
    u32_t rx_mask;
    u32_t tx_mask;
    u32_t ipc_mask;

    cause = dwcqos_mac_get_interrupt_status(pGmac);
    if (cause & DWCQOS_MAC_IS_MMC_INT)
    {
        rx_mask = dw_readl(pGmac, mac.mmc.rx_interrupt);
        tx_mask = dw_readl(pGmac, mac.mmc.tx_interrupt);
        ipc_mask = dw_readl(pGmac, mac.mmc.mmc_ipc_rx_interrupt);
        dwcqos_read_mmc_counters(pGmac, rx_mask, tx_mask, ipc_mask);
    }
}

static void fh_gmac_common_interrupt(int irq, void *param)
{
    u32_t qno = 0;
    int ret = 0;
    int cause;
    unsigned long bit_first;
    struct dw_qos *pGmac = (struct dw_qos *)param;
    int ret_eth;

    qno =  pGmac->active_queue_index;
    cause = dwcqos_dma_get_interrupt_status(pGmac);
    bit_first = cause & 0xff;
    /* process all channel isr.. */
    while (bit_first)
    {
        if (bit_first & (1 << qno))
        {
            ret = dwcqos_dma_isr_status_get(pGmac, qno);
            if (ret & DWCQOS_DMA_CH_IS_RI)
            {
                dwcqos_dma_isr_rx_set(pGmac, qno, 0);
                ret_eth = net_device_ready(&(pGmac->parent));
                if (ret_eth != RT_EOK)
                {
                    dwcqos_dma_isr_rx_set(pGmac, qno, 1);
                    rt_kprintf("net_device_ready error\n");
                }
            }

            /* clear all isr status. */
            dwcqos_dma_isr_status_set(pGmac, qno, ret);
            bit_first &= ~(1 << qno);
        }
        qno++;
    }

    /* process mtl */
    if (cause & DWCQOS_DMA_IS_MTLIS)
    {
        dwcqos_mtl_isr_process(pGmac);
    }

    if (cause & DWCQOS_DMA_IS_MACIS)
    {
        dwcqos_mac_isr_process(pGmac);
    }
}


static rt_err_t rt_fh_gmac_init(rt_device_t dev)
{
    struct dw_qos *pGmac;
    u32_t qno;

    pGmac = (struct dw_qos *)dev->user_data;

    DWCQOS_FOR_EACH_QUEUE(max_t(size_t, pGmac->hw_fea.rxq_num, pGmac->hw_fea.txq_num), qno) {
        dwcqos_dma_isr_enable_set(pGmac, qno,
        DWCQOS_DMA_CH_IE_NIE |
        DWCQOS_DMA_CH_IE_AIE |
        DWCQOS_DMA_CH_IE_FBEE);
    }

    DWCQOS_FOR_EACH_QUEUE(pGmac->hw_fea.txq_num, qno) {
        dwcqos_dma_tx_enable_set(pGmac, qno, 1);
    }

    DWCQOS_FOR_EACH_QUEUE(pGmac->hw_fea.rxq_num, qno) {
        dwcqos_dma_rx_enable_set(pGmac, qno, 1);
        dwcqos_dma_isr_rx_set(pGmac, qno, 1);
        dwcqos_kick_rx_queue(pGmac, qno);
    }
#if (0)
    /* mac start later, should sync with phy linkup. */
    dwcqos_mac_rx_enable(pGmac);
    dwcqos_mac_tx_enable(pGmac);
#endif
    return 0;
}

static rt_err_t rt_fh_gmac_open(rt_device_t dev, rt_uint16_t oflag)
{
    return RT_EOK;
}

static rt_err_t rt_fh_gmac_close(rt_device_t dev) { return RT_EOK; }
static rt_size_t rt_fh_gmac_read(rt_device_t dev, rt_off_t pos, void *buffer,
                                 rt_size_t size)
{
    rt_set_errno(RT_ENOSYS);
    return 0;
}

static rt_size_t rt_fh_gmac_write(rt_device_t dev, rt_off_t pos,
                                  const void *buffer, rt_size_t size)
{
    rt_set_errno(RT_ENOSYS);
    return 0;
}

static rt_err_t rt_fh_gmac_control(rt_device_t dev, int cmd, void *args)
{
    struct dw_qos *gmac;
    /* rt_uint16_t flag; */

    gmac = (struct dw_qos *)dev->user_data;

    switch (cmd)
    {
    case NIOCTL_GADDR:
        /* get mac address */
        if (args)
            rt_memcpy(args, gmac->local_mac_address, 6);
        else
            return -RT_ERROR;

        break;
    case NIOCTL_SADDR:
        /* set mac address */
        if (args)
        {
            rt_memcpy(gmac->local_mac_address, args, 6);
            dwcqos_set_hw_mac_addr(gmac, gmac->local_mac_address);
            rt_kprintf("set mac address: %02x:%02x:%02x:%02x:%02x:%02x\n",
                       gmac->local_mac_address[0], gmac->local_mac_address[1], gmac->local_mac_address[2],
                       gmac->local_mac_address[3], gmac->local_mac_address[4], gmac->local_mac_address[5]);
        }
        else
            return -RT_ERROR;

        break;

#if 0
    case NIOCTL_SFLAGS:
        /* set flag */
        if (args)
        {
            rt_memcpy(&flag, args, sizeof(rt_uint16_t));
            if (flag & IFF_PROMISC)
                set_filter(gmac, MAC_FILTER_PROMISCUOUS);
            else if (flag & IFF_ALLMULTI)
                set_filter(gmac, MAC_FILTER_MULTICAST);
            else
            {
                rt_kprintf("unknown flag: 0x%x\n", flag);
                return -RT_ERROR;
            }

        }
        else
            return -RT_ERROR;

        break;
#endif
    default:
        break;
    }
    return RT_EOK;
}

extern void phy_state_machine(void *param);
extern int auto_find_phy(struct dw_qos *pGmac);
extern int fh_qos_mdio_register(struct net_device *dev);



#ifdef RT_USING_PM
static int pm_suspend(const struct rt_device *device, rt_uint8_t mode)
{
    struct dw_qos *pGmac;

    pGmac = (struct dw_qos *)device->user_data;

    net_device_stop("e0");
    fh_qos_suspend(pGmac);
    return RT_EOK;
}

static void pm_resume(const struct rt_device *device, rt_uint8_t mode)
{
    struct dw_qos *pGmac;

    pGmac = (struct dw_qos *)device->user_data;

    if (pGmac->phy_sel)
        pGmac->phy_sel(pGmac->ac_phy_info->phy_sel);

    if (pGmac->ac_phy_info->phy_reset)
        if (pGmac->ac_phy_info->phy_sel == INTERNAL_PHY)
            pGmac->ac_phy_info->phy_reset();

    if (pGmac->inf_set)
        pGmac->inf_set(pGmac->ac_phy_info->phy_inf);

    pGmac->speed = 0xffffffff;
    fh_qos_resume(pGmac);
}

struct rt_device_pm_ops fh_qos_pm_ops = {
    .suspend_prepare = pm_suspend,
    .resume_prepare = pm_resume
};
#endif


static int fh_qos_gmac_probe(void *priv_data)
{
    struct dw_qos *pGmac;
    int ret;
    struct fh_gmac_platform_data *p_plat;
    int flag = 0;
    char gmac_irq_name[16];
    char netif_name[2];
    int i = 0;
    int j = 0;

    rt_memset(gmac_irq_name, 0, 16);
    rt_memset(netif_name, 0, 2);
    if (priv_data == NULL)
    {
        rt_kprintf("fh_eth_initialize: plat data can't be null\n");
        return (-1);
    }

    p_plat = (struct fh_gmac_platform_data *)priv_data;

    pGmac = (struct dw_qos *)rt_malloc(sizeof(struct dw_qos));
    if (pGmac == NULL)
    {
        rt_kprintf("fh_eth_initialize: Cannot allocate Gmac_Object %d\n", 1);
        return (-1);
    }
    memset(pGmac, 0, sizeof(struct dw_qos));

    ret  = fh_qos_parse_plat_info(pGmac, priv_data);
    if (ret < 0)
    {
        rt_free(pGmac);
        pGmac = NULL;
        return -1;
    }

    rt_timer_init(&pGmac->timer, "link_timer", phy_state_machine, (void *)pGmac,
                  RT_TICK_PER_SECOND, RT_TIMER_FLAG_PERIODIC);

    rt_sem_init(&pGmac->tx_desc_lock, "tx_desc_lock", 1, RT_IPC_FLAG_FIFO);
    rt_sem_init(&pGmac->lock, "gmac_lock", 1, RT_IPC_FLAG_FIFO);
    rt_sem_init(&pGmac->tx_lock, "tx_lock", 1, RT_IPC_FLAG_FIFO);
    rt_sem_init(&pGmac->rx_lock, "rx_lock", 1, RT_IPC_FLAG_FIFO);
    rt_sem_init(&pGmac->mdio_bus_lock, "mdio_bus_lock", 1, RT_IPC_FLAG_FIFO);

    dwcqos_init_sw(pGmac);
    dwcqos_desc_init(pGmac);

    if (auto_find_phy(pGmac))
    {
        rt_kprintf("find no phy !!!!!!!");
        ret = -1;
        goto free_desc_info;
    }

    dwcqos_init_hw(pGmac);

    rt_memcpy(pGmac->local_mac_address, gs_mac_addr, 6);

    dwcqos_set_hw_mac_addr(pGmac, pGmac->local_mac_address);

    /* MDIO bus Registration */
    ret = fh_qos_mdio_register(&(pGmac->parent));
    if (ret < 0)
    {
        rt_kprintf("mdio register err..\n");
        ret = -1;
        goto free_desc_info;
    }

#ifndef RT_LWIP_CHECKSUM_GEN
    /* ref desc1 27~28 bit */
    /* pGmac->open_flag = OPEN_IPV4_HEAD_CHECKSUM | OPEN_TCP_UDP_ICMP_CHECKSUM; */
#else
    /* pGmac->open_flag = 0; */
#endif

    rt_kprintf("%s - (IRQ #%d IO base addr: 0x%p)\n",
            __func__, p_plat->irq, pGmac->regs);

    rt_sprintf(gmac_irq_name, "qos_emac_%d",p_plat->id);
    /* instal interrupt */
#ifdef RT_USING_PM
    rt_pm_device_register(&pGmac->parent.parent, &fh_qos_pm_ops);
#endif

    rt_hw_interrupt_install(p_plat->irq, fh_gmac_common_interrupt, (void *)pGmac, gmac_irq_name);
    rt_hw_interrupt_umask(p_plat->irq);
    flag = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP;
#ifdef RT_LWIP_IGMP
    flag |= NETIF_FLAG_IGMP;
#endif
    rt_sprintf(netif_name, "e%d",p_plat->id);
    net_device_init(&(pGmac->parent), netif_name, flag, NET_DEVICE_GMAC);
    phy_state_machine(pGmac);

    return 0;

free_desc_info:
    DWCQOS_FOR_EACH_QUEUE(pGmac->hw_fea.txq_num, i) {
        rt_free(pGmac->tx_queue[i].p_skbuf);
        fh_dma_mem_free(pGmac->tx_queue[i].p_raw_desc);
    }

    DWCQOS_FOR_EACH_QUEUE(pGmac->hw_fea.rxq_num, i) {
        for (j = 0; j < RX_DESC_NUM; j++)
        {
            if (pGmac->rx_queue[i].p_skbuf[j])
                pbuf_free((struct pbuf *)pGmac->rx_queue[i].p_skbuf[j]);
        }
        rt_free(pGmac->rx_queue[i].p_skbuf);
        rt_free(pGmac->rx_queue[i].rx_skbuff_dma);
        fh_dma_mem_free(pGmac->rx_queue[i].p_raw_desc);
    }
    dwcqos_reset_dma_hw(pGmac);

    rt_free(pGmac->tx_queue);
    rt_free(pGmac->rx_queue);

    rt_free(pGmac);
    pGmac = NULL;

    return ret;

}

int fh_qos_gmac_exit(void *priv_data)
{
    return 0;
}

struct fh_board_ops fh_gmac_driver_ops = {

.probe = fh_qos_gmac_probe, .exit = fh_qos_gmac_exit,

};

int rt_app_fh_gmac_init(void)
{
    /* rt_kprintf("[%s-%d]: start\n", __func__, __LINE__); */
    fh_board_driver_register("fh_qos_gmac", &fh_gmac_driver_ops);
    return 0;
    /* fixme: never release? */
}

#ifdef RT_USING_FINSH
#include "finsh.h"
FINSH_FUNCTION_EXPORT(debug_rx_desc, dump e0 rx desc);
#endif
