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
/*****************************************************************************
 *  Include Section
 *  add all #include here
 *****************************************************************************/
#include <rtthread.h>
#include <rthw.h>
#include <rtdevice.h>
#include <netif/ethernetif.h>
#include "lwipopts.h"
#include "fh_gmac_phyt.h"
#include "mmu.h"
#include "fh_gmac.h"
#include "mii.h"
#include "board_info.h"
#include "delay.h"
#ifndef NIOCTL_SADDR
#define NIOCTL_SADDR 0x02
#endif
#ifdef RT_USING_PM
#include <pm.h>
#endif
/*****************************************************************************
 * Define section
 * add all #define here
 *****************************************************************************/
/*#define GMAC_DEBUG_PRINT*/
#if defined(GMAC_DEBUG_PRINT) && defined(RT_DEBUG)
#define GMAC_DEBUG(fmt, args...) rt_kprintf(fmt, ##args);
#else
#define GMAC_DEBUG(fmt, args...)
#endif

#define EMAC_CACHE_INVALIDATE(addr, size) \
    mmu_invalidate_dcache((rt_uint32_t)addr, size)
#define EMAC_CACHE_WRITEBACK(addr, size) \
    mmu_clean_dcache((rt_uint32_t)addr, size)
#define EMAC_CACHE_WRITEBACK_INVALIDATE(addr, size) \
    mmu_clean_invalidated_dcache((rt_uint32_t)addr, size)

/* EMAC has BD's in cached memory - so need cache functions */
#define BD_CACHE_INVALIDATE(addr, size) \
    mmu_invalidate_dcache((rt_uint32_t)addr, size)
#define BD_CACHE_WRITEBACK(addr, size) mmu_clean_dcache((rt_uint32_t)addr, size)

extern void mmu_clean_invalidated_dcache(rt_uint32_t buffer, rt_uint32_t size);
#define BD_CACHE_WRITEBACK_INVALIDATE(addr, size) \
    mmu_clean_invalidated_dcache((rt_uint32_t)addr, size)

#define DUPLEX_HALF 0x00
#define DUPLEX_FULL 0x01

#define GMAC_TIMEOUT_AUTODONE (200000)  /* 2s */
#define GMAC_TIMEOUT_PHYLINK (200000)   /* 2s */
#define GMAC_TIMEOUT_RECV (100000)      /* 1s */
#define GMAC_TIMEOUT_SEND (100000)      /* 1s */

#define GMAC_EACH_DESC_MAX_TX_SIZE (2048)
#define GMAC_EACH_DESC_MAX_RX_SIZE (2048)

#define NORMAL_INTERRUPT_ENABLE (1 << 16)
#define ABNORMAL_INTERRUPT_ENABLE (1 << 15)
#define ERE_ISR (1 << 14)
#define FBE_ISR (1 << 13)
#define ETE_ISR (1 << 10)
#define RWE_ISR (1 << 9)
#define RSE_ISR (1 << 8)
#define RUE_ISR (1 << 7)
#define RIE_ISR (1 << 6)
#define UNE_ISR (1 << 5)
#define OVE_ISR (1 << 4)
#define TJE_ISR (1 << 3)
#define TUE_ISR (1 << 2)
#define TSE_ISR (1 << 1)
#define TIE_ISR (1 << 0)

#define MAC_RX_ENABLE_POS (1 << 2)
#define MAC_TX_ENABLE_POS (1 << 3)

#define MAC_FILTER_PROMISCUOUS      0x01
#define MAC_FILTER_MULTICAST        0x10

struct fh_gmac_error_status g_error_rec = {0};

static int flow_ctrl = FLOW_AUTO;
static int pause = PAUSE_TIME;

void tx_desc_init(fh_gmac_object_t *gmac);
void rx_desc_init(fh_gmac_object_t *gmac);
extern int auto_find_phy(fh_gmac_object_t *gmac);
void dump_rx_desc(void);
/******************************************************************************
 * Function prototype section
 * add prototypes for all functions called by this file,execepting those
 * declared in header file
 *****************************************************************************/
/* extern void mmu_clean_dcache(rt_uint32_t buffer, rt_uint32_t size); */
/* extern void mmu_invalidate_dcache(rt_uint32_t buffer, rt_uint32_t size); */

/*****************************************************************************
 * Global variables section - Exported
 * add declaration of global variables that will be exported here
 * e.g.
 *  int8_t foo;
 ****************************************************************************/

/*****************************************************************************
 * Global variables section - Local
 * define global variables(will be refered only in this file) here,
 * static keyword should be used to limit scope of local variable to this file
 * e.g.
 *  static uint8_t ufoo;
 *****************************************************************************/

/* function body */
/*****************************************************************************
 * Description:
 *      add funtion description here
 * Parameters:
 *      description for each argument, new argument starts at new line
 * Return:
 *      what does this function returned?
 *****************************************************************************/
static Gmac_Rx_DMA_Descriptors *fh_gmac_get_rx_desc(fh_gmac_object_t *gmac);
static int fh_gmac_get_tx_desc_index(fh_gmac_object_t *gmac,
rt_uint32_t frame_len);
/*****************************************************************************
 * Description:
 *      add funtion description here
 * Parameters:
 *      description for each argument, new argument starts at new line
 * Return:
 *      what does this function returned?
 *****************************************************************************/
static rt_uint32_t fh_gmac_pm_stat_get(fh_gmac_object_t *gmac)
{
    rt_uint32_t ret;

    ret = gmac->pm_stat;

    return ret;
}

static void fh_gmac_pm_stat_set(fh_gmac_object_t *gmac, rt_uint32_t pm_stat)
{
    gmac->pm_stat = pm_stat;
}


void fh_gmac_isr_set(fh_gmac_object_t *gmac, rt_uint32_t bit)
{
    rt_uint32_t isr_ret;

    isr_ret = GET_REG(gmac->f_base_add + REG_GMAC_INTR_EN);
    isr_ret |= bit;
    SET_REG(gmac->f_base_add + REG_GMAC_INTR_EN, isr_ret);
}

void fh_gmac_isr_clear(fh_gmac_object_t *gmac, rt_uint32_t bit)
{
    rt_uint32_t isr_ret;

    isr_ret = GET_REG(gmac->f_base_add + REG_GMAC_INTR_EN);
    isr_ret &= ~bit;
    SET_REG(gmac->f_base_add + REG_GMAC_INTR_EN, isr_ret);
}

void fh_gmac_clear_status(fh_gmac_object_t *gmac, rt_uint32_t bit)
{
    SET_REG(gmac->f_base_add + REG_GMAC_STATUS, bit);
}

void fh_gmac_dma_op_set(fh_gmac_object_t *gmac, UINT32 bit)
{
    UINT32 ret;

    ret = GET_REG(gmac->f_base_add + REG_GMAC_OP_MODE);
    ret |= bit;
    SET_REG(gmac->f_base_add + REG_GMAC_OP_MODE, ret);
}

void fh_gmac_dma_op_clear(fh_gmac_object_t *gmac, UINT32 bit)
{
    UINT32 ret;

    ret = GET_REG(gmac->f_base_add + REG_GMAC_OP_MODE);
    ret &= ~bit;
    SET_REG(gmac->f_base_add + REG_GMAC_OP_MODE, ret);
}

void fh_gmac_dma_tx_stop(fh_gmac_object_t *gmac)
{
    fh_gmac_dma_op_clear(gmac, 1 << 13);
}

void fh_gmac_dma_tx_start(fh_gmac_object_t *gmac)
{
    fh_gmac_dma_op_set(gmac, 1 << 21);
    fh_gmac_dma_op_set(gmac, 1 << 13);
}

void fh_gmac_dma_rx_stop(fh_gmac_object_t *gmac)
{
    fh_gmac_dma_op_clear(gmac, 1 << 1);
}

void fh_gmac_dma_rx_start(fh_gmac_object_t *gmac)
{
    fh_gmac_dma_op_set(gmac, 1 << 1);
}

void fh_set_mac_config_reg(fh_gmac_object_t *gmac, UINT32 bit)
{
    UINT32 config_ret;
    config_ret = GET_REG(gmac->f_base_add + REG_GMAC_CONFIG);
    config_ret |= bit;
    SET_REG(gmac->f_base_add + REG_GMAC_CONFIG, config_ret);
}

void fh_clear_mac_config_reg(fh_gmac_object_t *gmac, UINT32 bit)
{
    UINT32 config_ret;
    config_ret = GET_REG(gmac->f_base_add + REG_GMAC_CONFIG);
    config_ret &= ~bit;
    SET_REG(gmac->f_base_add + REG_GMAC_CONFIG, config_ret);
}

void fh_gmac_mac_tx_stop(fh_gmac_object_t *gmac)
{
       fh_clear_mac_config_reg(gmac, MAC_TX_ENABLE_POS);
}

void fh_gmac_mac_tx_start(fh_gmac_object_t *gmac)
{
       fh_set_mac_config_reg(gmac, MAC_TX_ENABLE_POS);
}

void fh_gmac_mac_rx_stop(fh_gmac_object_t *gmac)
{
       fh_clear_mac_config_reg(gmac, MAC_RX_ENABLE_POS);
}

void fh_gmac_mac_rx_start(fh_gmac_object_t *gmac)
{
       fh_set_mac_config_reg(gmac, MAC_RX_ENABLE_POS);
}

rt_inline unsigned long emac_virt_to_phys(unsigned long addr) { return addr; }
static void fh_gmac_set_mac_address(fh_gmac_object_t *p, UINT8 *mac)
{
    UINT32 macHigh = mac[5] << 8 | mac[4];
    UINT32 macLow  = mac[3] << 24 | mac[2] << 16 | mac[1] << 8 | mac[0];

    SET_REG(p->f_base_add + REG_GMAC_MAC_HIGH, macHigh);
    SET_REG(p->f_base_add + REG_GMAC_MAC_LOW, macLow);
}

void fh_gmac_dma_reset(fh_gmac_object_t *gmac)
{
    fh_gmac_dma_rx_stop(gmac);
    fh_gmac_dma_tx_stop(gmac);

    SET_REG(gmac->f_base_add + REG_GMAC_BUS_MODE, 1 << 0);
    while (GET_REG(gmac->f_base_add + REG_GMAC_BUS_MODE) & 0x01)
        ;

    while (GET_REG(gmac->f_base_add + REG_GMAC_AXI_STATUS) & 0x3)
        ;
}

void set_filter(fh_gmac_object_t *gmac, unsigned int filter_mode)
{
    UINT32 base_addr;
    base_addr = gmac->f_base_add;
    /* enable boardcase frame.. */
    SET_REG(base_addr+REG_GMAC_FRAME_FILTER, filter_mode);
}



int fh_gmac_dma_int(fh_gmac_object_t *gmac)
{
    /* soft reset */
    fh_gmac_dma_reset(gmac);
    /*SET_REG(gmac->f_base_add + REG_GMAC_BUS_MODE, 32 << 8);*/
    SET_REG(gmac->f_base_add + REG_GMAC_BUS_MODE, 0x00012000);


    SET_REG(gmac->f_base_add + REG_GMAC_RX_DESC_ADDR,
            (UINT32)gmac->rx_ring_dma);

    SET_REG(gmac->f_base_add + REG_GMAC_TX_DESC_ADDR,
            (UINT32)gmac->tx_ring_dma);
    fh_gmac_clear_status(gmac, 0xffffffff);
    /* enable isr */
    fh_gmac_isr_set(gmac, NORMAL_INTERRUPT_ENABLE | RIE_ISR);
    return 0;
}


int fh_gmac_mac_int(fh_gmac_object_t *gmac)
{
    rt_uint16_t iff_netcard = IFF_MULTICAST | IFF_ALLMULTI;

    netdev_get_netcard_flags(&(gmac->parent), iff_netcard);
    set_filter(gmac, MAC_FILTER_MULTICAST);
    fh_gmac_set_mac_address(gmac, gmac->local_mac_address);

    if (flow_ctrl)
        gmac->flow_ctrl = FLOW_AUTO;    /* RX/TX pause on */
    gmac->pause = pause;

    fh_clear_mac_config_reg(gmac, 0xffffffff);
    fh_set_mac_config_reg(gmac, 1 << 16 | 1 << 15 | 1 << 7 |
                                    MAC_RX_ENABLE_POS | MAC_TX_ENABLE_POS);

    return 0;
}

int fh_gmac_init(rt_device_t dev)
{
    fh_gmac_object_t *gmac;
    /*dma and mac reg set...*/
    gmac = (fh_gmac_object_t *)dev->user_data;

    if (gmac->p_plat_data->gmac_reset)
        gmac->p_plat_data->gmac_reset();

    if (fh_gmac_dma_int(gmac) < 0 || fh_gmac_mac_int(gmac) < 0)
    {
        return -1;
    }

    return 0;
}

void fh_gmac_halt(rt_device_t dev)
{

}


/*********************
 *
 * up level use interface
 *
 *********************/
static rt_err_t rt_fh_gmac_init(rt_device_t dev)
{
    int ret;
    fh_gmac_object_t *gmac;

    gmac = (fh_gmac_object_t *)dev->user_data;
    ret = fh_gmac_init(dev);
    if (ret < 0)
    {
        rt_kprintf("GMAC init failed\n");
    }
    /* dma go...rx working. */
    fh_gmac_dma_rx_start(gmac);
    fh_gmac_dma_tx_start(gmac);
    return RT_EOK;
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
    fh_gmac_object_t *gmac;
    rt_uint16_t flag;

    gmac = (fh_gmac_object_t *)dev->user_data;

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
            fh_gmac_set_mac_address(gmac, gmac->local_mac_address);
            rt_kprintf("set mac address: %02x:%02x:%02x:%02x:%02x:%02x\n",
                       gmac->local_mac_address[0], gmac->local_mac_address[1], gmac->local_mac_address[2],
                       gmac->local_mac_address[3], gmac->local_mac_address[4], gmac->local_mac_address[5]);
        }
        else
            return -RT_ERROR;

        break;

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
    default:
        break;
    }

    return RT_EOK;
}

static rt_err_t rt_fh81_gmac_tx(rt_device_t dev, struct pbuf *p)
{
    Gmac_Tx_DMA_Descriptors *desc;
    rt_uint32_t pm_state;
    fh_gmac_object_t *gmac;
    struct pbuf *temp_pbuf;
    rt_uint8_t *bufptr;
    rt_uint32_t cur_tx_desc;
    gmac = (fh_gmac_object_t *)dev->user_data;

    pm_state = fh_gmac_pm_stat_get(gmac);
    if (pm_state == FH_GMAC_PM_STAT_SUSPNED)
        return -RT_EBUSY;
    rt_sem_take(&gmac->tx_lock, RT_WAITING_FOREVER);
    cur_tx_desc = fh_gmac_get_tx_desc_index(gmac, p->tot_len);
    if (cur_tx_desc == -1)
    {
        rt_kprintf("get wrong tx index\n");
        rt_sem_release(&gmac->tx_lock);
        return -RT_EBUSY;
    }
    desc = &gmac->tx_ring[cur_tx_desc];
    bufptr = (rt_uint8_t *)&gmac->tx_buffer[cur_tx_desc * GMAC_TX_BUFFER_SIZE];
    bufptr += 2;
    for (temp_pbuf = p; temp_pbuf != NULL; temp_pbuf = temp_pbuf->next)
    {
        rt_memcpy(bufptr, temp_pbuf->payload, temp_pbuf->len);
        bufptr += temp_pbuf->len;
    }
    /* clean cache to mem */
    BD_CACHE_WRITEBACK_INVALIDATE(
        (rt_uint32_t)&gmac->tx_buffer[cur_tx_desc * GMAC_TX_BUFFER_SIZE],
        p->tot_len + 2);

    desc->desc1.bit.buffer1_size = p->tot_len;
    desc->desc2.bit.buffer_address_pointer =
      (rt_uint32_t)&gmac->tx_buffer[cur_tx_desc * GMAC_TX_BUFFER_SIZE] + 2 ;
    desc->desc1.bit.first_segment = 1;
    desc->desc1.bit.last_segment  = 1;
    desc->desc1.bit.checksum_insertion_ctrl  = gmac->open_flag;
    /* desc->desc1.bit.intr_on_completion =1; */
    desc->desc0.bit.own = 1;
    BD_CACHE_WRITEBACK_INVALIDATE((unsigned int)desc, sizeof(Gmac_Tx_DMA_Descriptors));
    SET_REG(gmac->f_base_add + REG_GMAC_TX_POLL_DEMAND, 0);
    rt_sem_release(&gmac->tx_lock);

    return RT_EOK;
}

void fh81_gmac_rx_error_handle(fh_gmac_object_t *gmac)
{
    fh_gmac_clear_status(gmac, 1 << 7);
    fh_gmac_dma_rx_stop(gmac);
    fh_gmac_mac_rx_stop(gmac);
    rx_desc_init(gmac);
    fh_gmac_isr_set(gmac, RIE_ISR);
    fh_gmac_mac_rx_start(gmac);
    fh_gmac_dma_rx_start(gmac);
    SET_REG(gmac->f_base_add + REG_GMAC_RX_POLL_DEMAND, 1);
}
static struct pbuf *get_cur_rx_pbuf(fh_gmac_object_t *gmac)
{
    return gmac->rx_array[gmac->rx_pbuf_index];
}

static void set_cur_rx_pbuf(fh_gmac_object_t *gmac, struct pbuf *p_buf)
{
    gmac->rx_array[gmac->rx_pbuf_index] = p_buf;
    gmac->rx_pbuf_index++;
    gmac->rx_pbuf_index %= GMAC_RX_RING_SIZE;
}

/* reception packet. */
static struct pbuf *rt_fh81_gmac_rx(rt_device_t dev)
{

    Gmac_Rx_DMA_Descriptors *desc;
    rt_uint32_t pm_state;
    fh_gmac_object_t *gmac;
    struct pbuf *temp_pbuf = RT_NULL;
    struct pbuf *malloc_pbuf = RT_NULL;
    rt_uint32_t each_len = 0;
    rt_uint32_t ret;
    gmac = (fh_gmac_object_t *)dev->user_data;

    pm_state = fh_gmac_pm_stat_get(gmac);
    if (pm_state == FH_GMAC_PM_STAT_SUSPNED)
        return 0;
    rt_sem_take(&gmac->rx_lock, RT_WAITING_FOREVER);
    ret = GET_REG(gmac->f_base_add + REG_GMAC_STATUS);
    if (ret & 1 << 7)
    {
        fh_gmac_clear_status(gmac, 1 << 7);
        g_error_rec.rx_buff_unavail_nor++;
        GMAC_DEBUG("rx buff unavailable when process.\n");
        #if (0)
        fh81_gmac_rx_error_handle(gmac);
        /* fh_gmac_isr_set(gmac,RIE_ISR); */
        rt_sem_release(&gmac->rx_lock);
        return RT_NULL;
        #endif
    }

    desc = fh_gmac_get_rx_desc(gmac);
    if (desc)
    {
        each_len  = desc->desc0.bit.frame_length;

        BD_CACHE_WRITEBACK_INVALIDATE(
            (rt_uint32_t)(desc->desc2.bit.buffer_address_pointer), each_len);

        /*set payload actual len..*/
        temp_pbuf = get_cur_rx_pbuf(gmac);
        /* gmac write crc to the data buf. so here should subtract 4B*/
        temp_pbuf->len = temp_pbuf->tot_len = each_len - 4;
        malloc_pbuf = pbuf_alloc(PBUF_RAW, PBUF_MALLOC_LEN + CACHE_LINE_SIZE, PBUF_RAM);
        if (!malloc_pbuf)
        {
            /* set null to the rx array.. and don't set dma to go anymore*/
            /* may be I should call the core to set GMAC is dead... */
            set_cur_rx_pbuf(gmac, malloc_pbuf);
#ifdef GMAC_DEBUG_PRINT
            extern long list_thread(void);
            extern void list_mem(void);
            list_thread();
            list_mem();
#endif
            rt_kprintf("%s : %d alloc pbuf failed..\n", __func__, __LINE__);
            rt_sem_release(&gmac->rx_lock);
            /* return success pbuf.. */
            return temp_pbuf;
        }
        malloc_pbuf->payload = (void *)(((uint32_t)(malloc_pbuf->payload) +
                                         (CACHE_LINE_SIZE - 1)) & (~(CACHE_LINE_SIZE - 1)));
        BD_CACHE_WRITEBACK_INVALIDATE((uint32_t)(malloc_pbuf->payload), PBUF_MALLOC_LEN);
        desc->desc2.bit.buffer_address_pointer = (rt_uint32_t)malloc_pbuf->payload;
        desc->desc0.bit.own = 1;
        set_cur_rx_pbuf(gmac, malloc_pbuf);
        BD_CACHE_WRITEBACK_INVALIDATE(desc, sizeof(Gmac_Rx_DMA_Descriptors));
    }
    else
    {
        /* read all the desc done...reopen the rx isr.. */
        fh_gmac_isr_set(gmac, RIE_ISR);
    }

    rt_sem_release(&gmac->rx_lock);
    return temp_pbuf;
}

static Gmac_Rx_DMA_Descriptors *fh_gmac_get_rx_desc(fh_gmac_object_t *gmac)
{
    Gmac_Rx_DMA_Descriptors *desc = &gmac->rx_ring[gmac->rx_cur_desc];

    BD_CACHE_WRITEBACK_INVALIDATE(desc, sizeof(Gmac_Rx_DMA_Descriptors));
    if (desc->desc0.bit.own)
    {
        return RT_NULL;
    }
    gmac->rx_cur_desc++;
    gmac->rx_cur_desc %= GMAC_RX_RING_SIZE;
    return desc;
}

void fh81_gmac_tx_error_handle(fh_gmac_object_t *gmac)
{
    fh_gmac_dma_tx_stop(gmac);
    tx_desc_init(gmac);
    fh_gmac_dma_tx_start(gmac);
    SET_REG(gmac->f_base_add + REG_GMAC_TX_POLL_DEMAND, 0);
}

static int fh_gmac_get_tx_desc_index(fh_gmac_object_t *gmac,
rt_uint32_t frame_len)
{
    int ret_index = -1;
    int retry = 10;
    Gmac_Tx_DMA_Descriptors *desc;
    if (frame_len > GMAC_TX_BUFFER_SIZE)
    {
        rt_kprintf("payload len too long.....\n");
        RT_ASSERT(frame_len < GMAC_EACH_DESC_MAX_TX_SIZE);
    }

    if (gmac->phydev->speed == 100)
    {
        retry = 1000;
    }
    while (1)
    {
        SET_REG(gmac->f_base_add + REG_GMAC_TX_POLL_DEMAND, 0);
        desc = &gmac->tx_ring[gmac->tx_cur_desc];
        BD_CACHE_WRITEBACK_INVALIDATE((unsigned int)desc, sizeof(Gmac_Tx_DMA_Descriptors));
        if (desc->desc0.bit.own)
        {
            if (gmac->phydev->speed == 10)
                rt_thread_delay(1);
            else
                fh_udelay(1);
            retry--;
            if (!retry)
                return -1;
        }
        else
            break;
    }

    ret_index = gmac->tx_cur_desc;
    gmac->tx_cur_desc++;
    gmac->tx_cur_desc %= GMAC_TX_RING_SIZE;
    return ret_index;

}

void tx_desc_init(fh_gmac_object_t *gmac)
{
    int i;

    for (i = 0; i < GMAC_TX_RING_SIZE; i++)
    {
        Gmac_Tx_DMA_Descriptors *desc = &gmac->tx_ring[i];

        desc->desc0.dw                = 0;
        desc->desc1.dw                = 0;
        desc->desc2.dw                = 0;
        desc->desc3.dw                = 0;
        /* desc->desc3.dw = (rt_uint32_t)&gmac->tx_ring[0]; */
        desc->desc1.bit.first_segment          = 1;
        desc->desc1.bit.last_segment           = 1;
        desc->desc1.bit.end_of_ring            = 0;
        desc->desc1.bit.second_address_chained = 1;
        desc->desc3.bit.buffer_address_pointer =
            (rt_uint32_t)&gmac->tx_ring[i + 1];
    }
    /* gmac->tx_ring[GMAC_TX_RING_SIZE -1].desc1.bit.last_segment =1; */
    gmac->tx_ring[GMAC_TX_RING_SIZE - 1].desc3.bit.buffer_address_pointer =
        (UINT32)&gmac->tx_ring[0];
    /* gmac->tx_ring[GMAC_TX_RING_SIZE -1].desc1.bit.end_of_ring = 1; */
    gmac->tx_cur_desc = 0;
    BD_CACHE_WRITEBACK_INVALIDATE(
        gmac->tx_ring, sizeof(Gmac_Tx_DMA_Descriptors) * GMAC_TX_RING_SIZE);
}

void rx_desc_init(fh_gmac_object_t *gmac)
{
    int i;

    for (i = 0; i < GMAC_RX_RING_SIZE; i++)
    {
        Gmac_Rx_DMA_Descriptors *desc          = &gmac->rx_ring[i];

        desc->desc0.dw                         = 0;
        desc->desc1.dw                         = 0;
        desc->desc2.dw                         = 0;
        desc->desc3.dw                         = 0;
        desc->desc1.bit.buffer1_size           = PBUF_MALLOC_LEN;
        desc->desc1.bit.end_of_ring            = 0;
        desc->desc1.bit.second_address_chained = 1;
        desc->desc2.bit.buffer_address_pointer = (rt_uint32_t)gmac->rx_array[i]->payload;
        desc->desc3.bit.buffer_address_pointer = (rt_uint32_t)&gmac->rx_ring[i + 1];
        desc->desc0.bit.own = 1;
    }
    gmac->rx_ring[GMAC_RX_RING_SIZE - 1].desc3.bit.buffer_address_pointer =
        (rt_uint32_t)&gmac->rx_ring[0];
    /* gmac->rx_ring[GMAC_RX_RING_SIZE -1].desc1.bit.end_of_ring =1; */
    gmac->rx_cur_desc = 0;
    gmac->rx_pbuf_index = 0;
    BD_CACHE_WRITEBACK_INVALIDATE(
        gmac->rx_ring, sizeof(Gmac_Rx_DMA_Descriptors) * GMAC_RX_RING_SIZE);
}

static rt_err_t fh_gmac_initialize(fh_gmac_object_t *gmac)
{
    UINT32 t1;
    UINT32 t2;
    UINT32 t3;

    UINT32 i, j;
    t1 = (UINT32)rt_malloc(GMAC_TX_RING_SIZE * sizeof(Gmac_Tx_DMA_Descriptors) +
                           CACHE_LINE_SIZE);
    if (!t1)
        goto err1;

    t2 = (UINT32)rt_malloc(GMAC_TX_RING_SIZE * GMAC_TX_BUFFER_SIZE +
                           CACHE_LINE_SIZE);
    if (!t2)
        goto err2;

    t3 = (UINT32)rt_malloc(GMAC_RX_RING_SIZE * sizeof(Gmac_Rx_DMA_Descriptors) +
                           CACHE_LINE_SIZE);
    if (!t3)
        goto err3;

    for (i = 0; i < GMAC_RX_RING_SIZE; i++)
    {
        j = i;
        gmac->rx_array[i] = pbuf_alloc(PBUF_RAW, PBUF_MALLOC_LEN + CACHE_LINE_SIZE, PBUF_RAM);
        if (!gmac->rx_array[i])
        {
            rt_kprintf("%s : %d malloc pbuf error..\n",__func__, __LINE__);
            for (; j > 0; j--)
            {
                pbuf_free(gmac->rx_array[j]);
                gmac->rx_array[j] = 0;
            }
            goto err4;
        }
        gmac->rx_array[i]->payload = (void *)(((uint32_t)(gmac->rx_array[i]->payload) +
        (CACHE_LINE_SIZE - 1)) & (~(CACHE_LINE_SIZE - 1)));
    }
    gmac->tx_ring_original = (UINT8 *)t1;
    gmac->tx_ring = (Gmac_Tx_DMA_Descriptors *)((t1 + CACHE_LINE_SIZE - 1) &
                                               (~(CACHE_LINE_SIZE - 1)));
    rt_memset(gmac->tx_ring, 0,
              GMAC_TX_RING_SIZE * sizeof(Gmac_Tx_DMA_Descriptors));
    BD_CACHE_WRITEBACK_INVALIDATE(
        gmac->tx_ring, GMAC_TX_RING_SIZE * sizeof(Gmac_Tx_DMA_Descriptors));
    gmac->tx_ring_dma = emac_virt_to_phys((unsigned long)gmac->tx_ring);

    gmac->tx_buffer_original = (UINT8 *)t2;
    gmac->tx_buffer =
        (UINT8 *)((t2 + CACHE_LINE_SIZE - 1) & (~(CACHE_LINE_SIZE - 1)));
    gmac->tx_buffer_dma = emac_virt_to_phys((unsigned long)gmac->tx_buffer);

    gmac->rx_ring_original = (UINT8 *)t3;
    gmac->rx_ring = (Gmac_Rx_DMA_Descriptors *)((t3 + CACHE_LINE_SIZE - 1) &
                                               (~(CACHE_LINE_SIZE - 1)));
    rt_memset(gmac->rx_ring, 0,
              GMAC_RX_RING_SIZE * sizeof(Gmac_Rx_DMA_Descriptors));
    BD_CACHE_WRITEBACK_INVALIDATE(
        gmac->rx_ring, GMAC_RX_RING_SIZE * sizeof(Gmac_Rx_DMA_Descriptors));
    gmac->rx_ring_dma = emac_virt_to_phys((unsigned long)gmac->rx_ring);

    /* init tx desc */
    tx_desc_init(gmac);
    /* init rx desc */
    rx_desc_init(gmac);
    gmac->phy_addr = 0;
    gmac->phy_interface = gmac_rmii;
    GMAC_DEBUG("gmac use rmii..\n");

    return RT_EOK;

err4:
    rt_free((void *)t3);
err3:
    rt_free((void *)t2);
err2:
    rt_free((void *)t1);
err1:
    return -RT_ENOMEM;
}

static void rt_fh_gmac_isr(int irq, void *param)
{
    fh_gmac_object_t *gmac;
    gmac = (fh_gmac_object_t *)param;
    int ret_eth;
    unsigned int ret;
    ret = GET_REG(gmac->f_base_add + REG_GMAC_STATUS);
    fh_gmac_clear_status(gmac, 0xffffffff);

    if (ret & 1 << 4)
    {
        GMAC_DEBUG("gmac rx overflow..\n\n");
        g_error_rec.rx_over_flow++;
    }

    if (ret & 1 << 5)
    {
        GMAC_DEBUG("gmac tx underflow..\n\n");
        g_error_rec.tx_under_flow++;
    }

    if (ret & 1 << 2)
    {
        g_error_rec.tx_buff_unavail++;
    }

    if (ret & 1 << 7)
    {
        GMAC_DEBUG("gmac rx buff isr unavailable\n");
        g_error_rec.rx_buff_unavail_isr++;
        /* rx_desc_init(gmac); */
        SET_REG(gmac->f_base_add + REG_GMAC_RX_POLL_DEMAND, 1);
        SET_REG_M(gmac->f_base_add + REG_GMAC_OP_MODE, 1 << 1, 1 << 1);
    }

    if (ret & 1 << 8)
    {
        GMAC_DEBUG("gmac rx process stop..\n");
        g_error_rec.rx_process_stop++;
    }

    if (ret & 3 << 23)
    {
        GMAC_DEBUG("error status:%x\n", ret);
        g_error_rec.other_error++;
    }

    if (ret & 1 << 6)
    {
        fh_gmac_isr_clear(gmac, RIE_ISR);
        /* wake up rx.. */
        ret_eth = net_device_ready(&(gmac->parent));
        if (ret_eth != RT_EOK)
        {
            fh_gmac_isr_set(gmac, RIE_ISR);
            rt_kprintf("net_device_ready error\n");
        }

    }
}

static unsigned char gs_mac_addr[6] = {

    0x78, 0x99, 0x6E, 0x11, 0x29, 0x3b
};

void fh_gmac_initmac(unsigned char *gmac)
{
    rt_memcpy(gs_mac_addr, gmac, 6);
    GMAC_DEBUG("set Mac address.%02x:%02x:%02x:%02x:%02x:%02x\n", gs_mac_addr[0], gs_mac_addr[1], \
                                   gs_mac_addr[2], gs_mac_addr[3], gs_mac_addr[4], gs_mac_addr[5]);
}

/* static struct ethtool_ops fh_gmac_ethtool_ops = { */
/* .get_link = ethtool_op_get_link, */
unsigned int ethtool_op_get_link(struct net_device *dev)
{
    return dev->netif->flags & NETIF_FLAG_LINK_UP ? 1 : 0;
}

static int fh_gmac_ethtool_getsettings(struct net_device *dev, struct ethtool_cmd *cmd)
{
    fh_gmac_object_t *pGmac = dev->parent.user_data;
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
    fh_gmac_object_t *pGmac = dev->parent.user_data;
    struct phy_device *phy = pGmac->phydev;
    int rc;

    rt_sem_take(&pGmac->lock, RT_WAITING_FOREVER);
    rc = fh_gmac_phy_ethtool_sset(phy, cmd);
    rt_sem_release(&pGmac->lock);

    return rc;
}

static struct ethtool_ops fh_gmac_ethtool_ops = {
    .get_link = ethtool_op_get_link,
    .get_settings = fh_gmac_ethtool_getsettings,
    .set_settings = fh_gmac_ethtool_setsettings,
};

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

#ifdef RT_USING_PM
extern void net_device_stop(char *name);
extern void net_device_start(char *name);
static void fh_mac_suspend(fh_gmac_object_t *pGmac)
{
    net_device_stop("e0");
    fh_gmac_mac_rx_stop(pGmac);
    fh_gmac_dma_rx_stop(pGmac);
    fh_gmac_dma_tx_stop(pGmac);
    fh_gmac_mac_tx_stop(pGmac);
    SET_REG(pGmac->f_base_add + REG_GMAC_BUS_MODE, 1 << 0);
    while (GET_REG(pGmac->f_base_add + REG_GMAC_BUS_MODE) & 0x01)
        ;
    if (pGmac->rx_cur_desc != pGmac->rx_pbuf_index)
        rt_kprintf("%s :: ERROR.....rx_cur_desc[%d] != rx_pbuf_index[%d]\n",
        __func__, pGmac->rx_cur_desc, pGmac->rx_pbuf_index);
    tx_desc_init(pGmac);
    pGmac->speed = 0xffffffff;
}

static void fh_mac_resume(fh_gmac_object_t *gmac)
{
    Gmac_Rx_DMA_Descriptors *desc;

    desc = &gmac->rx_ring[gmac->rx_cur_desc];
    BD_CACHE_INVALIDATE(desc, sizeof(Gmac_Rx_DMA_Descriptors));
    desc->desc0.bit.own = 0;
    EMAC_CACHE_WRITEBACK(desc, sizeof(Gmac_Rx_DMA_Descriptors));

    SET_REG(gmac->f_base_add + REG_GMAC_BUS_MODE, 0x00012000);
    SET_REG(gmac->f_base_add + REG_GMAC_RX_DESC_ADDR, (UINT32)desc);
    SET_REG(gmac->f_base_add + REG_GMAC_TX_DESC_ADDR,
            (UINT32)gmac->tx_ring_dma);
    fh_gmac_clear_status(gmac, 0xffffffff);
    /* enable isr */
    fh_gmac_isr_set(gmac, NORMAL_INTERRUPT_ENABLE | RIE_ISR);

    fh_gmac_mac_int(gmac);
    fh_gmac_dma_rx_start(gmac);
    fh_gmac_dma_tx_start(gmac);

    BD_CACHE_INVALIDATE(desc, sizeof(Gmac_Rx_DMA_Descriptors));
    desc->desc0.bit.own = 1;
    EMAC_CACHE_WRITEBACK(desc, sizeof(Gmac_Rx_DMA_Descriptors));
    SET_REG(gmac->f_base_add + REG_GMAC_RX_POLL_DEMAND, 1);
}

static int pm_suspend(const struct rt_device *device, rt_uint8_t mode)
{
    fh_gmac_object_t *gmac;

    gmac = (fh_gmac_object_t *)device->user_data;
    fh_gmac_pm_stat_set(gmac, FH_GMAC_PM_STAT_SUSPNED);
    fh_mac_suspend(gmac);
    return RT_EOK;
}

static void pm_resume(const struct rt_device *device, rt_uint8_t mode)
{
    fh_gmac_object_t *gmac;

    gmac = (fh_gmac_object_t *)device->user_data;

    if (gmac->ac_phy_info->phy_reset)
        if (gmac->ac_phy_info->phy_sel == INTERNAL_PHY)
            gmac->ac_phy_info->phy_reset();
    fh_mac_resume(gmac);
    fh_gmac_pm_stat_set(gmac, FH_GMAC_PM_STAT_NORMAL);
    net_device_start("e0");
}

static struct rt_device_pm_ops fh_pm_ops = {
    .suspend_prepare = pm_suspend,
    .resume_prepare = pm_resume
};
#endif

int fh_gmac_probe(void *priv_data)
{
    fh_gmac_object_t *gmac;
    struct fh_gmac_platform_data *p_plat;
    int ret;
    rt_uint8_t flag = 0;

    if (priv_data == NULL)
    {
        rt_kprintf("fh_eth_initialize: plat data can't be null\n");
        return (-1);
    }
    p_plat = (struct fh_gmac_platform_data *)priv_data;
    gmac = (fh_gmac_object_t *)rt_malloc(sizeof(*gmac));
    if (gmac == NULL)
    {
        rt_kprintf("fh_eth_initialize: Cannot allocate Gmac_Object %d\n", 1);
        return (-1);
    }
    memset(gmac, 0, sizeof(*gmac));
    gmac->p_base_add = (void *)p_plat->base_add;
    gmac->f_base_add = p_plat->base_add;
    /* bind plat data */
    gmac->p_plat_data = p_plat;
    gmac->phyreset_gpio =  p_plat->phy_reset_pin;
    fh_gmac_initialize(gmac);
    rt_sem_init(&gmac->tx_lock, "tx_lock", 1, RT_IPC_FLAG_FIFO);
    rt_sem_init(&gmac->rx_lock, "rx_lock", 1, RT_IPC_FLAG_FIFO);
    rt_sem_init(&gmac->lock, "gmac_lock", 1, RT_IPC_FLAG_FIFO);
    rt_sem_init(&gmac->mdio_bus_lock, "mdio_bus_lock", 1, RT_IPC_FLAG_FIFO);
    rt_timer_init(&gmac->timer, "link_timer", phy_state_machine, (void *)gmac,
                  RT_TICK_PER_SECOND, RT_TIMER_FLAG_PERIODIC);
    rt_memcpy(gmac->local_mac_address, gs_mac_addr, 6);
#ifdef RT_USING_DEVICE_OPS
    gmac->parent.parent.ops       = &fh_gmac_ops;
#else
    gmac->parent.parent.init      = rt_fh_gmac_init;
    gmac->parent.parent.open      = rt_fh_gmac_open;
    gmac->parent.parent.close     = rt_fh_gmac_close;
    gmac->parent.parent.read      = rt_fh_gmac_read;
    gmac->parent.parent.write     = rt_fh_gmac_write;
    gmac->parent.parent.control   = rt_fh_gmac_control;
#endif
    gmac->parent.parent.user_data = (void *)gmac;
    gmac->parent.net.eth.eth_rx = rt_fh81_gmac_rx;
    gmac->parent.net.eth.eth_tx = rt_fh81_gmac_tx;
    gmac->parent.net.eth.ethtool_ops = &fh_gmac_ethtool_ops;
    if (auto_find_phy(gmac))
        rt_kprintf("find no phy !!!!!!!");

    ret = fh_mdio_register(&(gmac->parent));
    if (ret)
    {
        rt_kprintf("fh_mdio_register err,ret:%x\n", ret);
        return (-1);
    }


#ifndef RT_LWIP_CHECKSUM_GEN
    /* ref desc1 27~28 bit */
    gmac->open_flag = OPEN_IPV4_HEAD_CHECKSUM | OPEN_TCP_UDP_ICMP_CHECKSUM;
#else
    gmac->open_flag = 0;
#endif
    fh_gmac_pm_stat_set(gmac, FH_GMAC_PM_STAT_NORMAL);
#ifdef RT_USING_PM
    rt_pm_device_register(&gmac->parent.parent, &fh_pm_ops);
#endif

    /* instal interrupt */
    rt_hw_interrupt_install(p_plat->irq, rt_fh_gmac_isr, (void *)gmac, "emac_isr");
    rt_hw_interrupt_umask(p_plat->irq);
    flag = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP;
#ifdef RT_LWIP_IGMP
    flag |= NETIF_FLAG_IGMP;
#endif
    net_device_init(&(gmac->parent), "e0", flag, NET_DEVICE_GMAC);
    phy_state_machine(gmac);
    return 0;
}

int fh_gmac_exit(void *priv_data)
{
    return 0;
}

#ifdef RT_USING_FINSH
#include "finsh.h"

void dump_rx_desc(void)
{
    int i;
    fh_gmac_object_t *gmac;
    rt_device_t dev = rt_device_find("e0");

    if (dev == RT_NULL)
        return;

    gmac = (fh_gmac_object_t *)dev->user_data;

    for (i = 0; i < GMAC_RX_RING_SIZE; i++)
    {
        Gmac_Rx_DMA_Descriptors *desc = &gmac->rx_ring[i];

        rt_kprintf("desc%d\tadd:%08x\n", i, (unsigned int)desc);
        rt_kprintf("%08x  %08x  %08x  %08x\n", desc->desc0.dw, desc->desc1.dw,
                   desc->desc2.dw, desc->desc3.dw);
        rt_kprintf("\n");
    }
    rt_kprintf("soft current desc is:%d\n", gmac->rx_cur_desc);
}

void dump_tx_desc(void)
{
    int i;
    fh_gmac_object_t *gmac;
    rt_device_t dev = rt_device_find("e0");

    if (dev == RT_NULL)
        return;

    gmac = (fh_gmac_object_t *)dev->user_data;

    for (i = 0; i < GMAC_TX_RING_SIZE; i++)
    {
        Gmac_Tx_DMA_Descriptors *desc = &gmac->tx_ring[i];

        rt_kprintf("desc%d\tadd:%08x\n", i, (unsigned int)desc);
        rt_kprintf("%08x  %08x  %08x  %08x\n", desc->desc0.dw, desc->desc1.dw,
                   desc->desc2.dw, desc->desc3.dw);
        rt_kprintf("\n");
    }
    rt_kprintf("soft current desc is:%d\n", gmac->tx_cur_desc);
}

int dump_mac_err(void)
{
    rt_kprintf("rx buff unavail_isr:\t%08d\n", g_error_rec.rx_buff_unavail_isr);
    rt_kprintf("rx buff unavail_nor:\t%08d\n", g_error_rec.rx_buff_unavail_nor);
    rt_kprintf("rtt malloc failed:\t%08d\n", g_error_rec.rtt_malloc_failed);
    rt_kprintf("rx over flow:\t\t%08d\n", g_error_rec.rx_over_flow);
    rt_kprintf("rx process stop:\t%08d\n", g_error_rec.rx_process_stop);
    rt_kprintf("tx under flow:\t\t%08d\n", g_error_rec.tx_under_flow);
    rt_kprintf("other error:\t\t%08d\n", g_error_rec.other_error);
    rt_kprintf("tx over write desc error:\t\t%08d\n",
               g_error_rec.tx_over_write_desc);

    return 0;
}

FINSH_FUNCTION_EXPORT(dump_rx_desc, dump e0 rx desc);
FINSH_FUNCTION_EXPORT(dump_tx_desc, dump e0 tx desc);
FINSH_FUNCTION_EXPORT(dump_mac_err, dump e0 error status);

#endif


struct fh_board_ops fh_gmac_driver_ops = {

.probe = fh_gmac_probe, .exit = fh_gmac_exit,

};

int rt_app_fh_gmac_init(void)
{
    /* rt_kprintf("%s start\n", __func__); */
    fh_board_driver_register("fh_gmac", &fh_gmac_driver_ops);
    return 0;
    /* fixme: never release? */
}

