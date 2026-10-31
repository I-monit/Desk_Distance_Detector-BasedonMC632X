/*
 *  * Copyright (c) 2015-2019 Shanghai Fullhan Microelectronics Co., Ltd.
 *  *
 *  * SPDX-License-Identifier: Apache-2.0
 *  *
 *  * Change Logs:
 *  * Date           Author       Notes
 *  * 2019-01-25      {fullhan}   the first version
 *  *
 */

#include <rthw.h>
#include <rtthread.h>
#include <rtdevice.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "../core/usbdevice.h"
#include "udc_dwc.h"
#include "udc_otg_dwc.h"
#include "../class/video.h"

// #define USB_EP0_ERR_PROC
#ifdef USB_EP0_ERR_PROC
static volatile uint8_t g_uvc_stream_on_once = 1;
static volatile int g_ep0_nak_int_cnt = 0;
#endif
#define DESC_DMA_ENABLE 1
extern void mmu_clean_invalidated_dcache(rt_uint32_t buffer, rt_uint32_t size);
extern void mmu_invalidate_dcache(rt_uint32_t buffer, rt_uint32_t size);
extern uint64_t read_pts(void);
static void dwc_otg_flush_tx_fifo(dwc_handle *dwc, uint8_t epnum);

/*#define DWC_FORCE_SPEED_FULL*/
/*#define DWC_DEBUG*/
#ifdef DWC_DEBUG
#define DWC_DBG(fmt, args...)   rt_kprintf(fmt, ##args)
#else
#define DWC_DBG(fmt, args...)
#endif

/* #define MY_DWC_DEBUG */
#ifdef MY_DWC_DEBUG
#define MY_DWC_DBG(fmt, args...)   rt_kprintf(fmt, ##args)
#else
#define MY_DWC_DBG(fmt, args...)
#endif


#define UdcID (('U' << 24) | ('D' << 16) | ('C' << 16) | (':' << 16))
#define IS_SLAVE_MODE   0
#define IS_INTERN_DMA   2
#define IS_EXTERN_DMA   1

const char *ep0_state_string[] = {
    "EP_SETUP",
    "EP_DATA",
    "EP_STATUS",
    "EP_SETUP_PHASEDONE",
};

#ifdef DWC_FORCE_SPEED_FULL
#define DEP_EP_MAXPKT(n)        \
        ({                      \
            int v = 0;          \
            if (n)              \
                v = 64;         \
            else                \
                v = 64;         \
            v;                  \
        })
#else
#define DEP_EP_MAXPKT(n)        \
        ({                      \
            int v = 0;          \
            if (n)              \
                v = 512;        \
            else                \
                v = 64;         \
            v;                  \
        })
#endif

#define MAX_PKT_CNT     1023

ALIGN(32)
static int sleep_flag = 0;

/*
 * static functions
 */
static void dwc_otg_device_init(dwc_handle *dwc);
static void dwc_otg_core_reset(dwc_handle *dwc);
static void dwc_otg_core_init(dwc_handle *dwc, uint8_t dma_enable);

#if 0
static void udelay(uint32_t x)
{
    volatile uint32_t n = 1000;

    while (x--)
    {
        for (n = 0; n < 1000; ++n)
            ;
    }
}
#else
#define udelay rt_udelay
#endif
static void mdelay(uint32_t x)
{
    while (x--)
        udelay(1000);
}

static void dwc_otg_select_phy_width(dwc_handle *dwc)
{
    REG_GUSB_CFG &= ~USBCFG_TRDTIME_MASK;
    REG_GUSB_CFG |= (1 << 3);
    REG_GUSB_CFG |= USBCFG_TRDTIME_9;
}

static void dwc_otg_write_packet(dwc_handle *dwc, uint8_t epnum)
{
    int i;
    uint32_t dwords;
    uint32_t byte_count;
    dwc_ep *pep;

    epnum &= DWC_EPNO_MASK;
    pep = dwc->dep[DWC_EP_IN_OFS + epnum];

    byte_count = pep->xfer_len - pep->xfer_count;

    if (byte_count > DEP_EP_MAXPKT(epnum))
        byte_count = DEP_EP_MAXPKT(epnum);

    dwords = (byte_count + 3) / 4;

    for (i = 0; i < dwords; i++)
    {
        REG_EP_FIFO(epnum) = REG32((uint32_t *)(pep->xfer_buff) + i);
    }

    pep->xfer_count += byte_count;
    pep->xfer_buff += byte_count;
}

void dwc_read_ep_packet(dwc_handle *dwc, uint8_t epnum, uint32_t count)
{
    int i;
    int dwords = (count + 3) / 4;
    dwc_ep *pep;

    epnum &= DWC_EPNO_MASK;
    pep = dwc->dep[DWC_EP_OUT_OFS + epnum];

    for (i = 0; i < dwords; i++)
        REG32((uint32_t *)(pep->xfer_buff + pep->xfer_count / 4) + i) = REG_EP_FIFO(epnum);

    pep->xfer_count += count;
}

void dwc_write_ep_packet(dwc_handle *dwc, uint8_t epnum)
{
    uint32_t xfersize, finish, insize;
    uint32_t dwords;
    uint32_t txstatus = REG_DIEP_TXFSTS(epnum & 0x0F);
    dwc_ep *pep;

    epnum &= DWC_EPNO_MASK;
    pep = dwc->dep[DWC_EP_IN_OFS + epnum];

    insize = pep->xfer_len;
    if (pep->xfer_len > DEP_EP_MAXPKT(epnum))
        xfersize = DEP_EP_MAXPKT(epnum);
    else
        xfersize = pep->xfer_len;

    dwords = (xfersize + 3) / 4;
    DWC_DBG("txstatus(%x) dwords(%x) length(%x) xfer_count(%x)\n",
             txstatus, dwords, pep->xfer_len, pep->xfer_count);

    while ((txstatus > dwords) && (pep->xfer_len > 0) && (pep->xfer_count < pep->xfer_len))
    {
        dwc_otg_write_packet(dwc, epnum);
        xfersize = pep->xfer_len - pep->xfer_count;
        if (xfersize > DEP_EP_MAXPKT(epnum))
            xfersize = DEP_EP_MAXPKT(epnum);
        dwords = (xfersize + 3) / 4;
        txstatus = REG_DIEP_TXFSTS(epnum);
    }
    finish = pep->xfer_count;

    if (insize > finish)
    {
        uint32_t intr = REG_DIEP_INT(epnum);

        while (!(intr & DEP_TXFIFO_EMPTY))
        {
            intr = REG_DIEP_INT(epnum);
        }
        HW_SendPKT(dwc, epnum, pep->xfer_buff, insize - finish);
    }
    return;
}

void dwc_handle_ep_data_in_phase(dwc_handle *dwc, uint8_t epnum)
{
    uint32_t pktcnt, xfersize;
    dwc_ep *pep;

    epnum &= DWC_EPNO_MASK;
    pep = dwc->dep[DWC_EP_IN_OFS + epnum];

    xfersize = pep->xfer_len;
    pktcnt = (xfersize + pep->maxpacket - 1) / pep->maxpacket;
    if (pktcnt > 1023)
    {
        rt_kprintf("%s--%d WARNING...\n", __func__, __LINE__);
        while (1)
            ;
    }

    if (epnum == 0)
    {
        REG_DIEP_SIZE(epnum) &= ~(0x1fffff);
        REG_DIEP_SIZE(epnum) |= (pktcnt << 19) | xfersize;
    }
    else
    {

        REG_DIEP_SIZE(epnum) &= ~(0x7fffffff);
        if (pep->type == DWC_OTG_EP_TYPE_ISOC)
        {
            REG_DIEP_SIZE(epnum) |= (pktcnt << 29) | (pktcnt << 19) | xfersize;

        }
        else
            REG_DIEP_SIZE(epnum) |= (pktcnt << 19) | xfersize;
    }

    if (dwc->is_dma != 0)
    {
        // DWC_DBG("IN PACKET: 0x%p xfersize %d pktcnt %d\n", pep->data_buff, xfersize, pktcnt);

        REG_DIEP_DMA(epnum) = (uint32_t)(pep->data_buff);
        REG_DIEP_CTL(epnum) |= (DEP_ENA_BIT | DEP_CLEAR_NAK);
    }
    else
    {
        REG_DIEP_CTL(epnum) |= (DEP_ENA_BIT | DEP_CLEAR_NAK);
        REG_DIEP_EMPMSK |= (1 << epnum);
    }

    return;
}


void dwc_handle_ep_status_in_phase(dwc_handle *dwc, uint8_t epnum)
{
    dwc_ep *pep;

    DWC_DBG("%s %d\n", __func__, __LINE__);
    DWC_DBG("epnum = %d\n", epnum);
    epnum &= DWC_EPNO_MASK;
    pep = dwc->dep[DWC_EP_IN_OFS + epnum];

    pep->xfer_len = 0;
    pep->xfer_count = 0;

    if (epnum == 0)
    {
        REG_DIEP_SIZE(epnum) &= ~(0x1fffff);
        REG_DIEP_SIZE(epnum) |= DOEPSIZE0_PKTCNT_BIT | (pep->xfer_len);
    }
    else
    {
        REG_DIEP_SIZE(epnum) &= ~(0x1FFFFFFF);
        REG_DIEP_SIZE(epnum) |= DOEPSIZE0_PKTCNT_BIT | (pep->xfer_len);
    }

    if (dwc->is_dma == IS_INTERN_DMA)
    {
        REG_DIEP_DMA(epnum) = (uint32_t)(0xFFFFFFFF);
        REG_DIEP_CTL(epnum) |= DEP_ENA_BIT | DEP_CLEAR_NAK;
    }
    else
    {
        REG_DIEP_CTL(epnum) |= DEP_ENA_BIT | DEP_CLEAR_NAK;
    }
    return;
}

void dwc_handle_ep_data_out_phase(dwc_handle *dwc, uint8_t epnum)
{
    uint32_t dma_addr, dma_len;
    uint32_t pktcnt;
    dwc_ep *pep;

    DWC_DBG("%s %d\n", __func__, __LINE__);
    DWC_DBG("epnum = %d\n", epnum);

    epnum &= DWC_EPNO_MASK;
    pep = dwc->dep[DWC_EP_OUT_OFS + epnum];

    if (epnum == 0)
    {
        if (dwc->is_dma == IS_INTERN_DMA)
        {
            dma_len     = pep->maxpacket;
            dma_addr    = (uint32_t) (pep->xfer_buff);
            mmu_clean_invalidated_dcache(dma_addr, dma_len);
            if (pep->ep_state != EP_STATUS)
            	REG_DOEP_DMA(epnum) = (uint32_t)(pep->xfer_buff);
        }
        if (pep->ep_state == EP_SETUP)
        {
            REG_DOEP_SIZE(epnum) = DOEPSIZE0_PKTCNT_BIT | (8);
            REG_DOEP_CTL(epnum) |= DEP_ENA_BIT;
        }
        else
        {
            REG_DOEP_SIZE(epnum) = DOEPSIZE0_SUPCNT_1 | DOEPSIZE0_PKTCNT_BIT | (pep->maxpacket);
        REG_DOEP_CTL(epnum) |= DEP_ENA_BIT | DEP_CLEAR_NAK;
        }
    }
    else
    {
        if (pep->xfer_len > 0)
        {
            if (pep->xfer_len > MAX_PKT_CNT * DEP_EP_MAXPKT(epnum))
                pep->xfer_len = MAX_PKT_CNT * DEP_EP_MAXPKT(epnum);
            pktcnt = (pep->xfer_len + DEP_EP_MAXPKT(epnum) - 1) / DEP_EP_MAXPKT(epnum);
            if (pktcnt > 1023)
            {
                DWC_DBG("WARNING...\n");
                while (1)
                    ;
            }

            REG_DOEP_SIZE(epnum) &= ~(0x1fffffff);
            REG_DOEP_SIZE(epnum) |= (pktcnt << 19) | (pep->xfer_len);
        }

        if (dwc->is_dma == IS_INTERN_DMA)
        {
            dma_len = (((pep->xfer_len + 7) >> 3) << 3);
            dma_addr = (uint32_t)(pep->xfer_buff);
            mmu_clean_invalidated_dcache(dma_addr, dma_len);
            REG_DOEP_DMA(epnum) = (uint32_t)(pep->xfer_buff);
        }
        /* Program the DOEPCTLn Register with endpoint charateristics,
         * and set the Endpoint Enable and Clear NAK bit
         */
        REG_DOEP_CTL(epnum) |= DEP_ENA_BIT | DEP_CLEAR_NAK;
    }
}

int HW_SendPKT(dwc_handle *dwc, uint8_t epnum, const uint8_t *buf, int size)
{
    dwc_ep *pep;

    // DWC_DBG("HW_SendPKT addr = %02x, size = %d\n", epnum, size);

    epnum &= DWC_EPNO_MASK;
    pep = dwc->dep[DWC_EP_IN_OFS + epnum];

    /* write uvc srcSourceClock */
    if ((epnum == UVC_STREAM1_EP1 ||
        epnum == UVC_STREAM2_EP1) &&
        buf[1] & UVC_STREAM_SCR)
    {
        dsts_data_t dsts;
        uint32_t clock_pts = 0;
        uint32_t clock_sof = 0;
        uint8_t *clock_buf = 0;

        clock_buf = (uint8_t *)&buf[6];
        dsts.d32 = REG_OTG_DSTS;
        switch (dwc->speed)
        {
        case USB_SPEED_FULL:
        case USB_SPEED_LOW:
            clock_sof = ((dsts.b.soffn)) & 0x7ff;
            break;
        case USB_SPEED_HIGH:
            clock_sof = ((dsts.b.soffn) >> 3) & 0x7ff;
            break;
        }
        clock_pts = (uint32_t)(read_pts() & 0xffffffff);

        clock_buf[0] = (uint8_t)(clock_pts & 0xff);
        clock_buf[1] = (uint8_t)(clock_pts >> 8 & 0xff);
        clock_buf[2] = (uint8_t)(clock_pts >> 16 & 0xff);
        clock_buf[3] = (uint8_t)(clock_pts >> 24 & 0xff);
        clock_buf[4] = (uint8_t)(clock_sof & 0xff);
        clock_buf[5] = (uint8_t)(clock_sof >> 8 & 0x7);
    }

    pep->xfer_len   = size;                 /* number of bytes to transfer */
    pep->total_len   = size;                 /* number of bytes to transfer */
    pep->xfer_count = 0;                    /* number of bytes transferred */

    if (size > 0)
    {
        if (epnum == 0 && size < 4096)
        {
            rt_memset(pep->data_buff_align, 0, pep->maxpacket);
            rt_memcpy(pep->data_buff_align, buf, size);
            pep->data_buff = pep->data_buff_align;
            mmu_clean_invalidated_dcache((rt_uint32_t)pep->data_buff, size);
        } else
        {
        pep->data_buff = (void *)buf;
        mmu_clean_invalidated_dcache((rt_uint32_t)buf, size);
        }
    }

    if (pep->xfer_len > MAX_PKT_CNT * DEP_EP_MAXPKT(epnum))
        pep->xfer_len = MAX_PKT_CNT * DEP_EP_MAXPKT(epnum);

    pep->xfer_count = 0;

    // DWC_DBG("HW_SendPKT type :%d, state:%d\n", pep->type, pep->ep_state);
    switch (pep->type)
    {
    case DWC_OTG_EP_TYPE_CONTROL:
        pep->xfer_len   = MIN(pep->maxpacket, size);    /* number of bytes to transfer */
        if (pep->xfer_len > 0)
            pep->ep_state = EP_DATA;
        else
            pep->ep_state = EP_STATUS;
        DWC_DBG("HW_SendPKT type :%d, state:%d pep->xfer_len %d\n", pep->type, pep->ep_state, pep->xfer_len);
        /* 2 Stage */
        if (pep->ep_state == EP_STATUS && pep->xfer_len == 0) /*EP_SETUP 0   EP_DATA 1  EP_STATUS   2*/
        {
            DWC_DBG("%s %d ep_state = %s\n", __func__, __LINE__, ep0_state_string[pep->ep_state]);

            dwc_handle_ep_status_in_phase(dwc, 0);

            return 0;
        }

        /* 3 Stage */
        if (pep->ep_state == EP_DATA)
        {
            DWC_DBG("%s %d ep_state = %s pep->xfer_len %d\n", __func__, __LINE__, ep0_state_string[pep->ep_state], pep->xfer_len);
            /* enable in data phase */
            dwc_handle_ep_data_in_phase(dwc, epnum);
        }
        break;
    case DWC_OTG_EP_TYPE_INTR:
    case DWC_OTG_EP_TYPE_BULK:
    case DWC_OTG_EP_TYPE_ISOC:
        if (pep->ep_state == EP_IDLE || pep->ep_state == EP_TRANSFERRED)
        {
            pep->ep_state = EP_TRANSFERRING;
            if (pep->xfer_len == 0)
            {
                dwc_handle_ep_status_in_phase(dwc, epnum);
                return 0;
            }
            dwc_handle_ep_data_in_phase(dwc, epnum);
        }
        break;
    }

    return pep->xfer_len;
}

int HW_GetPKT(dwc_handle *dwc, uint8_t epnum, uint8_t *buf, int size)
{
    dwc_ep *pep;

    DWC_DBG("HW_GetPKT:%d %d\n", epnum, dwc->is_dma);

    epnum &= DWC_EPNO_MASK;
    pep = dwc->dep[DWC_EP_OUT_OFS + epnum];

    if ((size == 0) || (size > pep->xfer_count))
        size = pep->xfer_count;
    if (size < 0 || size > pep->xfer_len)
        size = 0;
    if (dwc->is_dma == IS_INTERN_DMA)
    {
        rt_memcpy((uint8_t *) buf, (uint8_t *)(pep->xfer_buff), size);
    }
    else
    {
        rt_memcpy((uint8_t *) buf, (uint8_t *) (pep->xfer_buff), size);
    }

    return size;
}

static void dwc_otg_flush_rx_fifo(dwc_handle *dwc)
{
    uint32_t grstctl;
    int  timeout = 100000;
    while (!(REG_GRST_CTL & RSTCTL_AHB_IDLE) && --timeout > 0)
        udelay(1);
    if (timeout < 2)
    {
        DWC_DBG("dwc dwc_otg_flush_rx_fifo RSTCTL_AHB_IDLE timeout!\n");
    }
    grstctl = REG_GRST_CTL;
    if (!(grstctl & RSTCTL_RXFIFO_FLUSH))
    {
        REG_GRST_CTL |= RSTCTL_RXFIFO_FLUSH;
    }
    timeout = 100000;
    while ((REG_GRST_CTL & RSTCTL_RXFIFO_FLUSH) &&  --timeout > 0)
    {
        udelay(1);
    }
    if (timeout < 2)
    {
        DWC_DBG("dwc dwc_otg_flush_rx_fifo RSTCTL_RXFIFO_FLUSH timeout!\n");
    }
}

static void dwc_otg_flush_tx_fifo(dwc_handle *dwc, uint8_t epnum)
{
    uint32_t gintsts;
    uint32_t grstctl;

    gintsts = REG_GINT_STS;
    /* Step1: Check that GINTSTS.GinNakEff=0 if this
     * bit is cleared then set Dctl.SGNPInNak = 1.
     * Nak effective interrupt = H indicating the core
     * is not reading from fifo
     */
    if (!(gintsts & GINTSTS_GINNAK_EFF))
    {
        REG_OTG_DCTL |= DCTL_SGNPINNAK;

        /* Step2: wait for GINTSTS.GINNakEff=1,which indicates
         * the NAK setting has taken effect to all IN endpoints
         */
        while (!(REG_GINT_STS & GINTSTS_GINNAK_EFF))
            udelay(1);
    }

    /* Step3: wait for ahb master idle state */
    while (!(REG_GRST_CTL & RSTCTL_AHB_IDLE))
        udelay(1);

    /* Step4: Check that GrstCtl.TxFFlsh=0, if it is 0, then write
     * the TxFIFO number you want to flush to GrstCTL.TxFNum
     */
    grstctl = REG_GRST_CTL;
    if (!(grstctl & RSTCTL_TXFIFO_FLUSH))
    {
        REG_GRST_CTL |= ((epnum & 0x0F) << 6);
    }

    /* Step5: Set GRSTCTL.TxFFlsh=1 and wait for it to clear */
    REG_GRST_CTL |= RSTCTL_TXFIFO_FLUSH;

    while (REG_GRST_CTL & RSTCTL_TXFIFO_FLUSH)
    {
        udelay(1);
    }

    /* Step6: Set the DCTL.GCNPinNak bit */
    REG_OTG_DCTL |= DCTL_CLR_GNPINNAK;
}

static void dwc_set_in_nak(dwc_handle *dwc, int epnum)
{
    int  timeout = 5000;

    epnum &= DWC_EPNO_MASK;

    REG_DIEP_CTL(epnum) |= DEP_SET_NAK;
    do
    {
        udelay(1);
        if (timeout < 2)
        {
            DWC_DBG("dwc set in nak timeout epnum %d\n", epnum);
        }
    } while ((!(REG_DIEP_INT(epnum) & DEP_INEP_NAKEFF)) && (--timeout > 0));
}

/* static void dwc_set_out_nak(dwc_handle *dwc, int epnum)
{
    epnum &= DWC_EPNO_MASK;
    REG_DOEP_CTL(epnum) |= DEP_SET_NAK;
} */

void dwc_disable_in_ep(dwc_handle *dwc, int epnum)
{
    int  timeout = 100000;
    dwc_ep  *pep = RT_NULL;

    epnum &= DWC_EPNO_MASK;
    pep = dwc->dep[DWC_EP_IN_OFS + epnum];

    if (!(REG_DIEP_CTL(epnum) & DEP_ENA_BIT))
        return;

    REG_DIEP_CTL(epnum) &= ~DEP_ENA_BIT;
    REG_DIEP_CTL(epnum) &= ~USB_ACTIVE_EP;
    /*step 1 : set nak*/
    dwc_set_in_nak(dwc, epnum);

    /*step 2: disable endpoint*/
    REG_DIEP_CTL(epnum) |= DEP_DISENA_BIT;

    do
    {
        udelay(1);
        if (timeout < 2)
        {
            DWC_DBG("dwc disable in ep timeout epnum : %d\n", epnum);
        }
    } while ((!(REG_DIEP_INT(epnum) & DEP_EPDIS_INT)) && (--timeout > 0));

    REG_DIEP_INT(epnum) = DEP_EPDIS_INT;

    /*step 3: flush tx fifo*/
    dwc_otg_flush_tx_fifo(dwc, epnum);

    REG_DIEP_SIZE(epnum) = 0x0;

    pep->ep_state = EP_IDLE;

}

int dwc_enable_in_ep(dwc_handle *dwc, uint8_t epnum)
{
    dwc_ep  *pep = RT_NULL;

    /* rt_kprintf("dwc_enable_in_ep :%x\n", epnum); */
    epnum &= DWC_EPNO_MASK;
    pep = dwc->dep[DWC_EP_IN_OFS + epnum];

    /* Program the endpoint register to configure them with the characteristics of valid endpoints */
    REG_DIEP_CTL(epnum) &= ~DEP_PKTSIZE_MASK;
    REG_DIEP_CTL(epnum) &= ~DEP_TYPE_MASK;

    switch (dwc->speed)
    {
    case USB_SPEED_FULL:
    case USB_SPEED_LOW:
        REG_DIEP_CTL(epnum) |= pep->maxpacket;
        break;
    case USB_SPEED_HIGH:
        REG_DIEP_CTL(epnum) |= pep->maxpacket;
        break;
    }

    REG_DIEP_CTL(epnum) |= (epnum << 22);

    switch (pep->type)
    {
    case DWC_OTG_EP_TYPE_CONTROL:
        REG_DIEP_CTL(epnum) |= USB_ACTIVE_EP | DEP_TYPE_CNTL;
        break;
    case DWC_OTG_EP_TYPE_ISOC:
        REG_DIEP_CTL(epnum) |= USB_ACTIVE_EP | DEP_TYPE_ISO;
        break;
    case DWC_OTG_EP_TYPE_BULK:
        REG_DIEP_CTL(epnum) |= USB_ACTIVE_EP | DEP_TYPE_BULK;
        break;
    case DWC_OTG_EP_TYPE_INTR:
        REG_DIEP_CTL(epnum) |= USB_ACTIVE_EP | DEP_TYPE_INTR;
        break;
    }

    /* DATA0 */
    REG_DIEP_CTL(epnum) |= (1 << 28);

    /* Enable EP INT */
    REG_DAINT_MASK |= (0x01 << (DWC_EP_IN_OFS + epnum));
    /* rt_kprintf("dwc_enable_in_ep :%x, done\n", epnum); */
    if (epnum == 0)
        pep->ep_state = EP_IDLE;
    return 0;
}

int dwc_enable_out_ep(dwc_handle *dwc, uint8_t epnum)
{
    uint32_t xfersize;
    uint32_t dma_addr, dma_len, pktcnt;
    dwc_ep  *pep = RT_NULL;

    /* rt_kprintf("dwc_enable_out_ep :%x\n", epnum); */
    epnum &= DWC_EPNO_MASK;
    pep = dwc->dep[DWC_EP_OUT_OFS + epnum];
    /* rt_kprintf("setting\n"); */

    /* Program the endpoint register to configure them with the characteristics of valid endpoints */
    REG_DOEP_CTL(epnum) &= ~DEP_PKTSIZE_MASK;
    REG_DOEP_CTL(epnum) &= ~DEP_TYPE_MASK;

    switch (dwc->speed)
    {
    case USB_SPEED_FULL:
    case USB_SPEED_LOW:
        REG_DOEP_CTL(epnum) |= pep->maxpacket;
        break;
    case USB_SPEED_HIGH:
        REG_DOEP_CTL(epnum) |= pep->maxpacket;
        break;
    }

    switch (pep->type)
    {
    case DWC_OTG_EP_TYPE_CONTROL:
        REG_DOEP_CTL(epnum) |= USB_ACTIVE_EP | DEP_TYPE_CNTL;
        break;
    case DWC_OTG_EP_TYPE_ISOC:
        REG_DOEP_CTL(epnum) |= USB_ACTIVE_EP | DEP_TYPE_ISO;
        break;
    case DWC_OTG_EP_TYPE_BULK:
        REG_DOEP_CTL(epnum) |= USB_ACTIVE_EP | DEP_TYPE_BULK;
        break;
    case DWC_OTG_EP_TYPE_INTR:
        REG_DOEP_CTL(epnum) |= USB_ACTIVE_EP | DEP_TYPE_INTR;
        break;
    }

    /* DATA0 */
    REG_DOEP_CTL(epnum) |= (1 << 28);

    /* Enable EP INT */
    REG_DAINT_MASK |= (0x01 << (DWC_EP_OUT_OFS + epnum));

    /* OUT-EP must init xfer buffer */
    xfersize    = pep->maxpacket * 2;
    pktcnt      = xfersize / DEP_EP_MAXPKT(epnum);

    pep->xfer_len   = xfersize;
    pep->xfer_count = 0;
    /* xfer_buffer has been initialized by up-layer */

    DWC_DBG("%s %d xfer_buff: %x %x\n", __func__, __LINE__, pep->xfer_buff, (void *)(pep->xfer_buff));

    /* Program the DOEPSIZn register for the transfer size and  corresponding packet count */
    REG_DOEP_SIZE(epnum) &= ~(0x1fffffff);
    REG_DOEP_SIZE(epnum) = (pktcnt << 19) | xfersize;
    if (dwc->is_dma == IS_INTERN_DMA)
    {
        dma_addr = (uint32_t) (pep->xfer_buff);
        dma_len  = (((xfersize + 7) >> 3) << 3);    /*pep->xfer_len;*/
        /* Additionally, in DMA mode, program the DOEPDMAn register */
        mmu_clean_invalidated_dcache(dma_addr, dma_len);
        REG_DOEP_DMA(epnum) = (uint32_t)(pep->xfer_buff);
    }

    /* Program the DOEPCTLn Register with endpoint charateristics,
     * and set the Endpoint Enable and Clear NAK bit
     */
    REG_DOEP_CTL(epnum) |= DEP_ENA_BIT | DEP_CLEAR_NAK;
    /* rt_kprintf("dwc_enable_out_ep :%x, done\n", epnum); */
    if (epnum == 0)
        pep->ep_state = EP_IDLE;
    return 0;
}

void dwc_set_address(dwc_handle *dwc, uint8_t address)
{
    sleep_flag = 1;
    REG_OTG_DCFG &= ~DCFG_DEV_ADDR_MASK;
    REG_OTG_DCFG |= address << DCFG_DEV_ADDR_BIT;
}


void dwc_otg_ep0_out_start(dwc_handle *dwc)
{
    dwc_ep *pep = dwc->dep[DWC_EP_OUT_OFS + 0];

    DWC_DBG("%s %d\n", __func__, __LINE__);
    pep->xfer_len   = 64;
    pep->xfer_count = 0;
    pep->maxpacket  = 8 * 8; /* 8 * 3 change by oxh */
    /*pep->xfer_buff = pep->xfer_buff;*/

    if (dwc->is_dma == IS_INTERN_DMA)
    {
        REG_DOEP_SIZE(0)    = DOEPSIZE0_SUPCNT_3 | DOEPSIZE0_PKTCNT_BIT | (pep->maxpacket);
        REG_DOEP_DMA(0)     = (uint32_t)(pep->xfer_buff);
    }
    else
    {
        REG_DOEP_SIZE(0)    = DOEPSIZE0_SUPCNT_3 | DOEPSIZE0_PKTCNT_BIT | (pep->maxpacket);
    }

    REG_DOEP_CTL(0) |= DEP_ENA_BIT | /* DEP_CLEAR_NAK | */ USB_ACTIVE_EP;
}

static void dwc_calculate_fifo_size(dwc_handle *dwc)
{
    /*
     * TODO: we are use "Dedicated FIFO Mode with No Thresholding"
     *  if need thresholding, the calculation algorithm may need change
     */

    /**
     * 3.2.1.1 FIFO SPRAM(Single-Port RAM) mapping:
     *
     * 1. One common RxFIFO, used in Host and Device modes
     * 2. One common Periodic TxFIFO, used in Host mode
     * 3. Separate IN endpoint transmit FIFO for each Device mode IN endpoints in Dedicated Transmit FIFO
     *    operation (OTG_EN_DED_TX_FIFO = 1)
     * 4. The FIFO SPRAM is also used for storing some register values to save gates. In Scatter/Gather DMA
     *    mode, four SPRAM locations (four 35-bit words) are reserved for this. In DMA and Slave modes
     *    (non-Scatter/Gather mode), one SPRAM location (one 35-bit word) is used for storing the DMA epnum.
     *
     * NOTE: when the device is operating in Scatter/Gather mode, then the last
     *       locations of the SPRAM store the Base Descriptor epnum, Current
     *       Descriptor epnum, Current Buffer epnum and status quadlet
     *       information for each endpoint direction (4 locations per Endpoint).
     *       If an endpoint is bidirectional, then 4 locations will be used for IN,
     *       and another 4 for OUT
     * 3.2.4.4 Endpoint Information Controller
     *       The last locations in the SPRAM are used to hold register values.
     *    Device Buffer DMA Mode:
     *       one location per endpoint direction is used in SPRAM to store the
     *       DIEPDMA and DOEPDMA value. The application writes data and then reads
     *       it from the same location
     *       For example, if there are ten bidirectional endpoints, then the last
     *       20 SPRAM locations are reserved for storing the DMA epnum for IN
     *       and OUT endpoints
     *   Scatter/Gather DMA Mode:
     *       Four locations per endpoint direction are used in SPRAM to store the
     *       Base Descriptor epnum, Current Descriptor epnum, Current Buffer
     *       Pointer and the Status Quadlet.
     *       The application writes data to the base descriptor epnum.
     *       When the application reads the location where it wrote the base
     *       descriptor epnum, it receives the current descriptor epnum.
     *       For example, if there are ten bidirectional endpoints, then the last 80
     *      locations are reserved for storing these values.
     *
     * Figure 3-13
     *  ________________________
     *  |                       |
     *  | DI/OEPDMAn Register   | Depends on the value of OTG_NUM_EPS
     *  | and Descriptor Status | and OTG_EP_DIRn, see not above
     *  |      values           |
     *  -------------------------
     *  |   TxFIFO #n Packets   |  DIEPTXFn
     *  -------------------------
     *  |                       |
     *  |   ................    |
     *  |                       |
     *  -------------------------
     *  |  TxFIFO #1 Packets    | DIEPTXF1
     *  -------------------------
     *  |  TxFIFO #0 Packets    |
     *  |( up to3 SETUP Packets)| GNPTXFSIZ
     *  ------------------------
     *  |                       |
     *  |     Rx Packets        |  GRXFSIZ
     *  |                       |
     *  -------------------------  epnum = 0, Rx starting epnum fixed to 0
     *
     */

    /**
     * Rx FIFO Allocation (rx_fifo_size)
     *
     * RAM for SETUP Packets: 4 * n + 6 locations must be Reserved in the receive FIFO to receive up to
     * n SETUP packets on control endpoints, where n is the number of control endpoints the device
     * core supports.
     *
     * One location for Global OUT NAK
     *
     * Status information is written to the FIFO along with each received packet. Therefore, a minimum
     * space of (Largest Packet Size / 4) + 1 must be allotted to receive packets. If a high-bandwidth
     * endpoint is enabled, or multiple isochronous endpoints are enabled, then at least two (Largest
     * Packet Size / 4) + 1 spaces must be allotted to receive back-to-back packets. Typically, two
     * (Largest Packet Size / 4) + 1 spaces are recommended so that when the previous packet is being
     * transferred to AHB, the USB can receive the subsequent packet. If AHB latency is high, you must
     * allocate enough space to receive multiple packets. This is critical to prevent dropping of any
     * isochronous packets.
     *
     * Typically, one location for each OUT endpoint is recommended.
     *
     * one location for eatch endpoint for EPDisable is required
     */

    /**
     * Tx FIFO Allocation (tx_fifo_size[n])
     *
     * The minimum RAM space required for each IN Endpoint Transmit FIFO is the maximum packet size
     * for that particular IN endpoint.
     *
     * More space allocated in the transmit IN Endpoint FIFO results in a better performance on the USB
     *and can hide latencies on the AHB.
     */
    uint32_t rx_fifo_size, i;
    uint32_t tx_fifo_size = 0;
    uint16_t startaddr = 0;
    uint16_t fifodepth = 0;

    /* Step1: Recevice FIFO Size Register (GRXFSIZ) */
    /* rx_fifo_size = (4 * 1 + 6) + (2) * (1024 / 4 + 1) + (2 * dwc->hwcfg2.b.num_dev_ep) + 1; */
    /**
     *   Buffer DMA Mode:Method 2
     *   Use this method if you are using the recommended minimum FIFO depth allocation with support for
     *   highbandwidth endpoints. This FIFO allocation enables the core to transfer a packet on the USB while the
     *   previous (next) packet is simultaneously transferred to the AHB. This FIFO allocation improves the core�s
     *   performance.
     *   Slave or Buffer DMA Mode:
     *   (5 * number of control endpoints + 8) + 2 * ((largest USB packet used / 4) + 1) +(2 * number of OUT
     *   endpoints) + 1
     */
    rx_fifo_size = (5 * 1 + 8) + 2 * (1024 / 4 + 1) + (2 * 16/* dwc->hwcfg2.b.num_dev_ep */) + 1;
    REG_GRXFIFO_SIZE = rx_fifo_size;

    /**
     *Step2: Program device in ep transmit fifo0 size register (GNPTXFSIZ)
     *Device IN Endpoint-Specific TxFIFOs (a separate FIFO is allocated to each endpoint) =
     *2 * (max_pkt_size for the endpoint) / 4.
     */
    tx_fifo_size = 2 * (64 / 4);
    REG_GNPTXFIFO_SIZE = (tx_fifo_size << 16) | rx_fifo_size;

    startaddr = tx_fifo_size + rx_fifo_size;
    for (i = 1; i < dwc->hwcfg4.b.num_in_eps-1; i++)
    {
        if (i == UVC_STREAM1_EP1 || i == UVC_STREAM2_EP1)
        {
            if (i == UVC_STREAM2_EP1)
                tx_fifo_size = (2 * (512 / 4));
            else
#ifdef ISOC_HIGH_BANDWIDTH_EP
                tx_fifo_size = (2 * (1536 / 4));
#else
                tx_fifo_size = (2 * (512 / 4));
#endif
        }
        else if (i == USB_CDC_VCOM_EP || i == USB_MSTORAGE_EP || i == USB_RNDIS_EP)
        {
            tx_fifo_size = (2 * (256 / 4));
        }
        else
        {
            tx_fifo_size = (2 * (128 / 4));
        }

        REG_GDIEP_TXF(i) = (tx_fifo_size << 16) | startaddr;
        startaddr += tx_fifo_size;
    }

    /* Configure fifo start addr and depth for endpoint information controller */
    fifodepth = REG_GHW_CFG3 >> 16;
    REG_GDFIFO_CFG = (startaddr << 16) | fifodepth;

    if (startaddr >= fifodepth)
    {
        rt_kprintf("error:fifo size(%d) > fifo depth =%d\n", startaddr, fifodepth);
    }

    /* flush tx and rx fifo */
    dwc_otg_flush_rx_fifo(dwc);

    dwc_otg_flush_tx_fifo(dwc, 0x10);

}


void dump_global_dwcreg(void)
{
    DWC_DBG("REG_OTG_DCTL: 0x%x\n", REG_OTG_DCTL);
    DWC_DBG("REG_OTG_DCFG: 0x%x\n", REG_OTG_DCFG);
    DWC_DBG("REG_GUSB_CFG: 0x%x\n", REG_GUSB_CFG);
    DWC_DBG("REG_GAHB_CFG: 0x%x\n", REG_GAHB_CFG);
    DWC_DBG("REG_GHW_CFG4: 0x%x\n", REG_GHW_CFG4);
}

static void dwc_handle_enum_done_intr(dwc_handle *dwc)
{
    dwc_ep *pep = dwc->dep[0];

    DWC_DBG("usbd enum start.\n");
    /* Step1: Read the DSTS register to determine the enumeration speed */
    uint32_t dsts       = REG_OTG_DSTS;
    uint32_t diep0ctl   = REG_DIEP_CTL(0);

    diep0ctl &= ~(0x3);

    switch (dsts & DSTS_ENUM_SPEED_MASK)
    {
    case DSTS_ENUM_SPEED_HIGH:
        DWC_DBG("High speed.\n");
        dwc->speed = USB_SPEED_HIGH;
        pep->maxpacket = 64;
        diep0ctl |= DEP_EP0_MPS_64;
        REG_OTG_DCFG &= ~1;
        break;
    case DSTS_ENUM_SPEED_FULL_30OR60:
    case DSTS_ENUM_SPEED_FULL_48:
        DWC_DBG("Full speed.\n");
        dwc->speed = USB_SPEED_FULL;
        pep->maxpacket = 64;
        diep0ctl |= DEP_EP0_MPS_64;
        REG_OTG_DCFG |= 1;
        break;
    case DSTS_ENUM_SPEED_LOW:
        DWC_DBG("Low speed.\n");
        dwc->speed = USB_SPEED_LOW;
        pep->maxpacket = 8;
        diep0ctl |= DEP_EP0_MPS_8;
        break;
    default:
        DWC_DBG("Fault speed enumration\n");
        break;
    }

    dwc->speed = USB_SPEED_HIGH;
    pep->maxpacket = 64;
    diep0ctl |= DEP_EP0_MPS_64;
    REG_OTG_DCFG &= ~1;

    REG_OTG_DCTL |= DCTL_CLR_GNPINNAK;

    /* Step2: Program the DIEPCTL0.MPS to set the maximum packet size */
    REG_DIEP_CTL(0) = diep0ctl;

    /* Step3: In Dma mode program the DOEPCTL0 register
     * to enable control ouctrl_req_addrt endpoint0 to receive setup
     * packet .
     */
    dwc_otg_ep0_out_start(dwc);

    /* Step4: unmask the SOF interrupt */
    /* REG_GINT_MASK |= GINTMSK_START_FRAM; */

    REG_GINT_STS = GINTSTS_ENUM_DONE;
    /*dump_global_dwcreg();*/
    return;
}

static void dwc_handle_early_suspend_intr(dwc_handle *dwc)
{
    DWC_DBG("Handle early suspend intr.\n");

    REG_GINT_STS = GINTSTS_USB_EARLYSUSPEND;

    if (REG_OTG_DSTS & DSTS_ERRATIC_ERROR)
    {
        REG_OTG_DCTL |= DCTL_SOFT_DISCONN;
        mdelay(100);
        dwc_otg_core_reset(dwc);
        dwc_otg_core_init(dwc, 1);
        dwc_otg_device_init(dwc);
        dwc_calculate_fifo_size(dwc);
    }
}

static void dwc_handle_suspend_intr(dwc_handle *dwc)
{
    DWC_DBG("Handle  suspend intr.\n");
    REG_GINT_STS = GINTSTS_USB_SUSPEND;
    DWC_DBG("==>%s, sleep_flag = %d\n", __func__, sleep_flag);
}

static void dwc_handle_start_frame_intr(dwc_handle *dwc)
{
    REG_GINT_STS = GINTSTS_START_FRAM;
}

static void dwc_handle_reset_intr(dwc_handle *dwc)
{
    int i;

    DWC_DBG("Handle reset intr.\n");
    /* Step1: NAK OUT ep */
    for (i = 0; i < dwc->hwcfg2.b.num_dev_ep; i++)
    {
        REG_DOEP_CTL(i) |= DEP_SET_NAK;
    }

    /* Step2: unmask the following interrupt bits */
    REG_DAINT_MASK = 0;
    REG_DOEP_MASK = 0;
    REG_DIEP_MASK = 0;

    REG_DAINT_MASK |=  (1 << 0) | (1 << 16);
    REG_DOEP_MASK |= DEP_XFER_COMP | DEP_SETUP_PHASE_DONE | DEP_AHB_ERR;
    REG_DOEP_MASK &= ~DEP_OUTTOKEN_RECV_EPDIS;
    REG_DIEP_MASK |= DEP_XFER_COMP | DEP_TIME_OUT   | DEP_AHB_ERR;

    dwc->dep[0]->ep_state = EP_SETUP;

    /* Step3: Device initalization */
    dwc_otg_device_init(dwc);

    /* Step4: Set up the data fifo ram for each of the fifo */
    /*dwc_calculate_fifo_size();*/

    /* Step5: Reset Device Address */
    REG_OTG_DCFG &= (~DCFG_DEV_ADDR_MASK);

    /* Step6: setup EP0 to receive SETUP packets */

    REG_DOEP_CTL(0) |= DEP_ENA_BIT | DEP_CLEAR_NAK;

    REG_GINT_STS = GINTSTS_USB_RESET;

    return;
}

void dwc_handle_rxfifo_nempty(dwc_handle *dwc)
{
    uint32_t count;
    uint32_t rxsts_pop = REG_GRXSTS_POP;
    uint8_t epnum = (rxsts_pop & 0xf);

    switch (rxsts_pop & GRXSTSP_PKSTS_MASK)
    {
    case GRXSTSP_PKSTS_GOUT_NAK:
        DWC_DBG("GRXSTSP_PKSTS_GOUT_NAK.\n");
        break;
    case GRXSTSP_PKSTS_GOUT_RECV:
        DWC_DBG("GRXSTSP_PKSTS_GOUT_RECV. - ");

        count = (rxsts_pop & GRXSTSP_BYTE_CNT_MASK) >> GRXSTSP_BYTE_CNT_BIT;
        if (count)
        {
            DWC_DBG("count:%d\n", count);
            dwc_read_ep_packet(dwc, epnum, count);
        }

        break;
    case GRXSTSP_PKSTS_TX_COMP:
        DWC_DBG("GRXSTSP_PKSTS_TX_COMP.\n");
        break;
    case GRXSTSP_PKSTS_SETUP_COMP:
        DWC_DBG("GRXSTSP_PKSTS_SETUP_COMP.\n");
        break;
    case GRXSTSP_PKSTS_SETUP_RECV:
        DWC_DBG("GRXSTSP_PKSTS_SETUP_RECV. - ");
        ((uint8_t *)dwc->dep[0]->xfer_buff)[0] = REG_EP_FIFO(0);
        ((uint8_t *)dwc->dep[0]->xfer_buff)[1] = REG_EP_FIFO(1);
        DWC_DBG("%x %x\n", ((uint8_t *)dwc->dep[0]->xfer_buff)[0], ((uint8_t *)dwc->dep[0]->xfer_buff)[1]);
        break;
    default:
        break;
    }
    REG_GINT_STS = GINTSTS_RXFIFO_NEMPTY;
}


void dwc_ep0_in_intr(dwc_handle *dwc, uint8_t epnum)
{
    uint32_t updated_size;
    uint32_t byte_count;
    uint32_t intr = REG_DIEP_INT(epnum & 0x0F);
    dwc_ep *pep;

    // REG_DIEP_INT(epnum & 0x0F) = intr;
    epnum &= DWC_EPNO_MASK;
    pep = dwc->dep[DWC_EP_IN_OFS + epnum];
    /*udelay(1);*/

    /* When the transfer size if 0 and the packet count is 0,
     * the transfer complete interrupt for the endpoint is generated
     * and the endpoint enable is cleared
     */
    if (intr & DEP_XFER_COMP)
    {
#ifdef USB_EP0_ERR_PROC
        g_ep0_nak_int_cnt = 0;
#endif
        REG_DIEP_INT(epnum) = DEP_XFER_COMP;
        if (dwc->is_dma == IS_SLAVE_MODE)
            REG_DIEP_EMPMSK &= ~(1 << epnum);

        updated_size = (REG_DIEP_SIZE(epnum) & 0x7f);
        byte_count = pep->xfer_len - updated_size;
        pep->xfer_count += byte_count;

        switch (pep->ep_state)
        {
        case EP_DATA:
            /* 3 Stage */
            if (pep->xfer_count < pep->total_len && pep->xfer_count != 0)
            {
                pep->data_buff = (uint8_t *)pep->data_buff + byte_count;
                pep->xfer_len = MIN((pep->total_len - pep->xfer_count), pep->maxpacket);
                dwc_handle_ep_data_in_phase(dwc, epnum);
            }
            else
            {
                pep->ep_state = EP_STATUS;
                dwc_handle_ep_data_out_phase(dwc, 0);
            }
            break;
        case EP_STATUS:
            pep->ep_state = EP_SETUP;
            dwc_handle_ep_data_out_phase(dwc, 0);
            if (dwc->test_mode)
            {
                int dctl = REG_OTG_DCTL;
                dctl &= ~(0x7 << 4);
                switch (dwc->test_mode)
                {
                case TEST_J:
				case TEST_K:
				case TEST_SE0_NAK:
				case TEST_PACKET:
				case TEST_FORCE_EN:
                    dctl |= dwc->test_mode << 4;
                    REG_OTG_DCTL = dctl;
                    break;
                default:
                    rt_kprintf("USB_FEATURE_TEST_MODE request, but test_mode is illegal\n");
                    break;
                }
            }
            break;
        }
    }

    if (dwc->is_dma == IS_SLAVE_MODE)
    {
        if ((intr & DEP_TXFIFO_EMPTY) && (REG_DIEP_EMPMSK & (1 << epnum)))
        {
            if (pep->xfer_len)
            {
                MY_DWC_DBG("dwc_write_ep_packet\n");
                dwc_write_ep_packet(dwc, epnum);
            }
            REG_DIEP_INT(epnum) = DEP_TXFIFO_EMPTY;

        }
    }

    if (intr & DEP_AHB_ERR)
    {
        DWC_DBG("1 AHB ERR\n");
        REG_DIEP_INT(epnum) = DEP_AHB_ERR;
    }


    if (intr & DEP_TIME_OUT)
    {
        DWC_DBG("IN TIME_OUT.\n");
        REG_DIEP_INT(epnum) = DEP_TIME_OUT;
    }

    if (intr & DEP_INTOKEN_RECV_TXFIFO_EMPTY)
    {
        // DWC_DBG("DEP_INTOKEN_RECV_TXFIFO_EMPTY.\n");
        REG_DIEP_INT(epnum) = DEP_INTOKEN_RECV_TXFIFO_EMPTY;
    }

    /** IN Endpoint NAK Effective */
    if (intr & DEP_INEP_NAKEFF)
    {
        REG_DIEP_INT(epnum) = DEP_INEP_NAKEFF;
    }

    /** IN EP Tx FIFO Empty Intr */
    if (intr & DEP_TXFIFO_EMPTY)
    {
        REG_DIEP_INT(epnum) = DEP_TXFIFO_EMPTY;
    }

    /* NAK Interrupt */
    if (intr & DEP_NAK_INT)
    {
#ifdef USB_EP0_ERR_PROC
        g_ep0_nak_int_cnt++;
        if (g_ep0_nak_int_cnt > 100000 && g_uvc_stream_on_once)
        {
            g_ep0_nak_int_cnt = 0;

            rt_kprintf("ep NAK status=%d\n", pep->ep_state);
            if (epnum == 0)
            {
                depctl_data_t depctl;
                depctl.d32 = REG_DIEP_CTL(0);
                if (depctl.b.epena)
                {
                    depctl.b.epdis = 1;
                }
                depctl.b.stall = 1;
                REG_DIEP_CTL(0) = depctl.d32;
                depctl.d32 = REG_DOEP_CTL(epnum);
                depctl.b.stall = 1;
                REG_DOEP_CTL(epnum) = depctl.d32;
                pep->ep_state = EP_SETUP;
            }
            if (REG_DOEP_SIZE(0) & 0x7fff)
            {
                dwc_otg_flush_rx_fifo(dwc);
            }
            if (REG_DIEP_SIZE(0) & 0x7fff)
            {
                dwc_disable_in_ep(dwc,0);
                dwc_enable_in_ep(dwc,0);
            }
            dwc_otg_ep0_out_start(dwc);
        }
#endif
        REG_DIEP_INT(epnum) = DEP_NAK_INT;
    }

}

void dwc_epn_in_intr(dwc_handle *dwc, uint8_t epnum)
{
    uint32_t intr = REG_DIEP_INT(epnum & 0x0F);
    uint32_t updated_size;

    /* When the transfer size if 0 and the packet count is 0,
     * the transfer complete interrupt for the endpoint is generated
     * and the endpoint enable is cleared
     */
    dwc_ep *pep;

    epnum &= DWC_EPNO_MASK;
    pep = dwc->dep[DWC_EP_IN_OFS + epnum];
    if (intr & DEP_XFER_COMP)
    {
        // DWC_DBG("1 IN XFER_COMP. %x\n", REG_DIEP_SIZE(epnum));
        REG_DIEP_INT(epnum) = DEP_XFER_COMP;
        REG_DIEP_CTL(epnum) |= DEP_SET_NAK;
        if (pep->ep_state == EP_TRANSFERRING)
        {
            if (dwc->is_dma == IS_SLAVE_MODE)
                REG_DIEP_EMPMSK &= ~(1 << epnum);
            updated_size = (REG_DIEP_SIZE(epnum) & 0x7ffff);
            pep->xfer_count = pep->xfer_len - updated_size;
            pep->ep_state = EP_TRANSFERRED;
            udc_usbd_event_cb(epnum, USB_EVT_IN, 0);
        }
    }
    if (dwc->is_dma == IS_SLAVE_MODE)
    {
        if ((intr & DEP_TXFIFO_EMPTY) && (REG_DIEP_EMPMSK & (1 << epnum)))
        {
            REG_DIEP_EMPMSK &= ~(1 << epnum);
            if (pep->xfer_len)
            {
                dwc_write_ep_packet(dwc, epnum);
            }
            REG_DIEP_INT(epnum) = DEP_TXFIFO_EMPTY;
        }
    }
    if (intr & DEP_AHB_ERR)
    {
        rt_kprintf("AHB ERR %d\n", epnum);
        REG_DIEP_INT(epnum) = DEP_AHB_ERR;
    }

    if (intr & DEP_TIME_OUT)
    {
        DWC_DBG("IN TIME_OUT.\n");
        REG_DIEP_INT(epnum) = DEP_TIME_OUT;
    }

    /* NAK Interrupt */
    if (intr & DEP_NAK_INT)
    {
        REG_DIEP_INT(epnum) = DEP_NAK_INT;
    }
}

/*
 * ep0 control transfer:
 *          3 Stage:
 *          SetupPhase-------->IN DataPhase ---------> OUT StatusPhase
 *      Or  2 Stage:
 *          SetupPhase-------->IN StatusPhase
 */

typedef struct {
    u8 bmRequestType;
    u8 bRequest;
    u16 wValue;
    u16 wIndex;
    u16 wLength;
}__packed dwc_DeviceRequest;
/*__attribute__ ((packed)) dwc_DeviceRequest;*/

int dwc_ep0_out_intr(dwc_handle *dwc, uint8_t epnum)
{
    uint32_t intr;

    dwc_ep *pep = RT_NULL;

    epnum &= DWC_EPNO_MASK;
    pep = dwc->dep[DWC_EP_OUT_OFS + epnum];

    intr = REG_DOEP_INT(epnum);
    // REG_DOEP_INT(epnum) = intr;
    /*udelay(1);*/
    /* comp intrerrupt indeicates completion of the status out phase */
    if (intr & DEP_XFER_COMP)
    {
        REG_DOEP_INT(epnum) = DEP_XFER_COMP;
        DWC_DBG("pep->ep_state = %s\n",ep0_state_string[pep->ep_state]);

        if (pep->ep_state == EP_STATUS)
        {
            pep->ep_state = EP_SETUP;
            dwc_handle_ep_data_out_phase(dwc, 0);
        }
        else if (pep->ep_state == EP_DATA)
        {
            DWC_DBG("*** EP0 DATA *** %d %d %d\n",pep->maxpacket,epnum,(REG_DOEP_SIZE(epnum) & 0x7ffff));
            pep->xfer_count = pep->maxpacket - (REG_DOEP_SIZE(epnum) & 0x7ffff);
            DWC_DBG("pep->xfer_count = %d\n", pep->xfer_count);
            udc_usbd_event_cb(0, USB_EVT_OUT, 0);
        }
        else if (!(intr & (DEP_SETUP_PHASE_DONE | (1 << 15))))
        {
            DWC_DBG("error\n");
            rt_kprintf("%s  %d error+++++++++++++++++++++intr %x\n", __func__, __LINE__, intr);
            pep->ep_state = EP_SETUP;
            dwc_handle_ep_data_out_phase(dwc, epnum);
        }
/*         else if (!(intr & (DEP_SETUP_PHASE_DONE)) && (intr & (1 << 15)))
        {
            DWC_DBG("error\n");
            rt_kprintf("%d error\n", __LINE__);
            pep->ep_state = EP_SETUP;
            dwc_handle_ep_data_out_phase(dwc, epnum);
        } */
        else if (pep->ep_state != EP_SETUP)
        {
            DWC_DBG("ep0 state mismatch\n");
            rt_kprintf("ep0 state mismatch\n");
        }
    }

    if (intr & DEP_STATUS_PHASE_RECV)
    {
        if (pep->ep_state == EP_DATA)
        {
            pep->ep_state = EP_STATUS;
            dwc_handle_ep_status_in_phase(dwc, epnum);
        }
        REG_DOEP_INT(epnum) = DEP_INTOKEN_EPMISATCH;
    }

    if (intr & DEP_AHB_ERR)
    {
        DWC_DBG("AHB ERR\n");
        REG_DOEP_INT(0) = DEP_AHB_ERR;
    }

    if (intr & DEP_OUTTOKEN_RECV_EPDIS)
    {
        REG_DOEP_INT(0) = DEP_OUTTOKEN_RECV_EPDIS;
    }

    if (intr & DEP_NAK_INT)
    {
        REG_DOEP_INT(0) = DEP_NAK_INT;
    }

    if (intr & (DEP_SETUP_PHASE_DONE/*  | (1 << 15) */))
    {
        DWC_DBG("SETUP_PHASE_DONE.\n");
        /* read the DOEPTSIZn to determine the number of setup packets
         * recevied and process the last recevied setup packet
         */
        REG_DOEP_INT(epnum) = DEP_SETUP_PHASE_DONE | (1 << 15);
        if (intr & DEP_B2B_SETUP_RECV)
        {
            DWC_DBG("back to back setup recevie++++++++++++++++++++++++++++\n");
        }
        else
        {
            /* Read out the last packet from the rxfifo */
            // mmu_clean_invalidated_dcache((uint32_t)(pep->xfer_buff), sizeof(dwc_DeviceRequest));
            mmu_invalidate_dcache((uint32_t)(pep->xfer_buff), sizeof(dwc_DeviceRequest));
            /* At the end of the Setup stage, the appliaction must reporgram the
             * DOEPTSIZn.SUPCnt field to 3 receive the next SETUP packet
             */
            if (pep->ep_state == EP_SETUP)
            {
                if (dwc->is_dma == IS_INTERN_DMA)
                {
                    // REG_DOEP_SIZE(epnum) = DOEPSIZE0_SUPCNT_3 | DOEPSIZE0_PKTCNT_BIT | (pep->maxpacket);
                    // REG_DOEP_DMA(epnum) = (uint32_t)(pep->xfer_buff);
                }
            }

            /* Setup Finish */
            pep->xfer_count = sizeof(dwc_DeviceRequest);

            udc_usbd_event_cb(0, USB_EVT_SETUP, pep->xfer_buff);

            /* REG_DOEP_CTL(epnum) |= DEP_DISENA_BIT; */
        }
    }
    return 0;
}

int dwc_epn_out_intr(dwc_handle *dwc, uint8_t epnum)
{
    uint32_t intr, updated_size;
    dwc_ep *pep;

    epnum &= DWC_EPNO_MASK;
    pep = dwc->dep[DWC_EP_OUT_OFS + epnum];
    /*udelay(1);*/
    DWC_DBG("ep%d out_intr\n",epnum);

    intr = REG_DOEP_INT(epnum);
    // REG_DOEP_INT(epnum) = intr;
    if (intr & DEP_XFER_COMP)
    {
        REG_DOEP_INT(epnum) = DEP_XFER_COMP;
        if (pep->ep_state != EP_IDLE)
        {
        updated_size = REG_DOEP_SIZE(epnum) & 0x7ffff;
        pep->xfer_count = pep->xfer_len - updated_size;

        // rt_kprintf("xfer_count = %d\n", pep->xfer_count);
        udc_usbd_event_cb(epnum, USB_EVT_OUT, 0);
        }

#if 0
        pep->xfer_len = pep->maxpacket; /* number of bytes to transfer */
        pep->xfer_count = 0;  /* number of bytes transferred */
        dwc_handle_ep_data_out_phase(dwc, epnum);
        DWC_DBG("REG_DOEP_SIZE: %x\n", REG_DOEP_SIZE(epnum));
#endif
    }

    if (intr & (DEP_AHB_ERR | 1<<12 | 1<<8))
    {
        DWC_DBG("1 AHB ERR\n");
        rt_kprintf("%s-%d epnum %d INT %x\n", __func__, __LINE__, epnum, intr);
        REG_DOEP_INT(epnum) = DEP_AHB_ERR |  1<<12 | 1<<8;
    }
    return 0;
}

static void dwc_handle_inep_intr(dwc_handle *dwc)
{
    uint32_t ep_intr, ep_in_intr;
    uint8_t epnum = 0;

    ep_intr = (REG_OTG_DAINT);
    ep_in_intr = (ep_intr & 0x0000ffff);
    epnum = 0;
    while (ep_in_intr)
    {
        if (ep_in_intr & 0x01)
        {
            epnum &= 0x0f;
            MY_DWC_DBG("Ep IN %d %x\n", epnum, DIEP_CTL(epnum));
            if (epnum == 0)
            {
                dwc_ep0_in_intr(dwc, epnum);
            }
            else
            {
                dwc_epn_in_intr(dwc, epnum);
            }
        }
        epnum++;
        ep_in_intr >>= 1;
    }
    REG_GINT_STS = GINTSTS_IEP_INTR;
    return;
}

static void dwc_handle_outep_intr(dwc_handle *dwc)
{
    uint32_t ep_intr, ep_out_intr;
    uint8_t epnum = 0;

    ep_intr = (REG_OTG_DAINT & 0xffff0000);
    ep_out_intr = ep_intr >> 16;
    epnum = 0;
    while (ep_out_intr)
    {
        if (ep_out_intr & 0x01)
        {
            if (epnum == 0)
            {
                dwc_ep0_out_intr(dwc, 0);
            }
            else
            {
                dwc_epn_out_intr(dwc, epnum);
            }
        }
        epnum++;
        ep_out_intr >>= 1;
    }
    REG_GINT_STS = GINTSTS_OEP_INTR;
}

static void dwc_otg_intr(dwc_handle *dwc)
{
    REG_GINT_STS = GINTSTS_OTG_INTR;
}

void dwc_common_intr(dwc_handle *dwc, uint32_t intsts)
{
    if (intsts & GINTSTS_USB_EARLYSUSPEND)
    {
        dwc_handle_early_suspend_intr(dwc);
    }

    if (intsts & GINTSTS_USB_SUSPEND)
    {
        dwc_handle_suspend_intr(dwc);
    }

    if (intsts & GINTSTS_USB_RESET)
    {
        dwc_handle_reset_intr(dwc);
    }

    if (intsts & GINTSTS_ENUM_DONE)
    {
        dwc_handle_enum_done_intr(dwc);
    }

    if (intsts & GINTSTS_START_FRAM)
    {
        dwc_handle_start_frame_intr(dwc);
    }
}

void dwc_handle_resume_intr(dwc_handle *dwc)
{
    DWC_DBG("Handle resume intr.\n");
    REG_GINT_STS = GINTSTS_RSUME_DETE;
}
static void udc_usbd_isr_service(void *param);

static void dwc_irq_handler(int vector, void *arg)
{
    dwc_handle *dwc = (dwc_handle *)arg;

    RT_ASSERT(dwc != RT_NULL);

    rt_hw_interrupt_mask(IRQ_OTG);
    udc_usbd_isr_service(dwc);
    rt_hw_interrupt_umask(IRQ_OTG);
}

static void dwc_otg_core_reset(dwc_handle *dwc)
{
    uint32_t greset = 0;
    uint32_t cnt = 0;

    REG_GRST_CTL |= RSTCTL_CORE_RST;
    do
    {
        greset = REG_GRST_CTL;
        if (cnt++ > 100000)
        {
            DWC_DBG("GRESET wait reset timeout.\n");
            return;
        }
        udelay(10);
    } while (greset & RSTCTL_CORE_RST);

    cnt = 0;

    do
    {
        udelay(10);
        greset = REG_GRST_CTL;
        if (cnt++ > 100000)
        {
            DWC_DBG("GRESET wait IDLE timeout.\n");
            return;
        }
    } while ((greset & RSTCTL_AHB_IDLE) == 0);

    /* wait for 3 phy clocks */
    udelay(100);
}

static void dwc_otg_device_init(dwc_handle *dwc)
{
    uint32_t dcfg = 0;

    /* Restart the phy clock */
    if (REG_PCGC_CTL & 0x1)
    {
        DWC_DBG("<<<<<< pcgcctl %x >>>>>\n", REG_PCGC_CTL);
        REG_PCGC_CTL &= ~(0x1 | (1 << 2) | (1 << 3));
    }

    /* In dma mode GINTMSK_NPTXFIFO_EMPTY , GINTMSK_RXFIFO_NEMPTY must be masked*/
    if (dwc->is_dma == IS_INTERN_DMA)
    {
        if (REG_GINT_MASK & (GINTMSK_NPTXFIFO_EMPTY | GINTMSK_RXFIFO_NEMPTY))
        {
            REG_GINT_MASK &= ~(GINTMSK_NPTXFIFO_EMPTY | GINTMSK_RXFIFO_NEMPTY);
        }
    }
    else
    {
        REG_GINT_MASK |= (GINTMSK_NPTXFIFO_EMPTY | GINTMSK_RXFIFO_NEMPTY);
    }

    /* Program the DCFG register */
    if (dwc->hwcfg4.b.desc_dma)
    {
        dcfg |= DCFG_DEV_DESC_DMA;
    }

#ifdef DWC_FORCE_SPEED_FULL
    REG_OTG_DCFG |= 1;
#else
    REG_OTG_DCFG &= ~3;
#endif

    /* Clear the DCTL.SftDiscon bit the core issues aconnect after ths bit is cleared */
    REG_OTG_DCTL &= ~DCTL_SOFT_DISCONN;

    REG_OTG_DCTL |= DCTL_IGNRFRMNUM;

    REG_GINT_STS = 0xffffffff;
    /* Program the GINTMSK */
    REG_GINT_MASK  = GINTMSK_MODE_MISMATCH | GINTMSK_OTG_INTR | GINTMSK_USB_EARLYSUSPEND |
                     GINTMSK_USB_SUSPEND | /* GINTSTS_ISOINCMP_INTR | */GINTMSK_USB_RESET |
                     GINTMSK_ENUM_DONE | GINTMSK_IEP_INTR | GINTMSK_OEP_INTR |
                     GINTMSK_CONID_STSCHG | GINTMSK_SESS_REQ | GINTMSK_WKUP;
    REG_DIEP_MASK = DIEPMSK_NAK | DIEPMSK_TIMEOUT | DIEPMSK_AHBERR | DIEPMSK_EPDISBLD | DIEPMSK_XFERCMP; /* 0x200f */
}

static void dwc_otg_core_init(dwc_handle *dwc, uint8_t dma_enable)
{
    uint32_t ahbcfg = 0, gusbcfg = 0;
    uint8_t arch;

    DWC_DBG("Core Init...\n");
    /* Step1: Read the GHWCFG1,2,3,4 to find the configuration parameters selected for DWC_otg core */
    dwc->hwcfg1.d32 = REG_GHW_CFG1;
    dwc->hwcfg2.d32 = REG_GHW_CFG2;
    dwc->hwcfg3.d32 = REG_GHW_CFG3;
    dwc->hwcfg4.d32 = REG_GHW_CFG4;
    DWC_DBG("cfg1:%x 2:%x 3:%x 4:%x\n", dwc->hwcfg1, dwc->hwcfg2, dwc->hwcfg3, dwc->hwcfg4);
    DWC_DBG("cfg2->arch %x\n", dwc->hwcfg2.b.architecture);
    arch = dwc->hwcfg2.b.architecture;
    switch (arch)
    {
    case IS_SLAVE_MODE:
        dwc->is_dma = IS_SLAVE_MODE;
        break;
    case IS_EXTERN_DMA:
        dwc->is_dma = IS_EXTERN_DMA;
        break;
    case IS_INTERN_DMA:
        dwc->is_dma = IS_INTERN_DMA;
        break;
    }
    /* Step2: Program the GAHBCFG register */

    /* DMA Mode bit and Burst Length */
    if (dwc->is_dma == IS_EXTERN_DMA)
    {
        DWC_DBG("DWC IS_EXTERN_DMA\n");
        ahbcfg |= AHBCFG_DMA_ENA;
    }
    else if (dwc->is_dma == IS_INTERN_DMA)
    {
        if (dma_enable)
        {
            DWC_DBG("DWC IS_INTERN_DMA\n");
            ahbcfg |= AHBCFG_DMA_ENA | (DWC_GAHBCFG_INT_DMA_BURST_INCR16 << 1);
        }
        else
        {
            ahbcfg |= AHBCFG_TXFE_LVL;
            dwc->is_dma = 0;
        }
    }
    else
    {
        DWC_DBG("DWC IS_SLAVE_MODE\n");
    }

    /* Step3: Program the GINTMSK register */
    REG_GINT_MASK = 0;

    /* Step4: Program the GUSBCFG register */
    gusbcfg = REG_GUSB_CFG;

    gusbcfg &= ~((1 << 4) | (1 << 6) | (1 << 8) | (1 << 9));
    REG_GUSB_CFG = gusbcfg;
    dwc_otg_select_phy_width(dwc);

    dwc_otg_core_reset(dwc);

    /* Global Interrupt Mask bit = 1 */
    ahbcfg |= AHBCFG_GLOBLE_INTRMASK;
    REG_GAHB_CFG = ahbcfg;

    /* Step5: The software must unmask OTG Interrupt Mask bit ,
     * MOde mismatch interrupt Mask bit in the GINTMSK
     */
    /* REG_GINT_MASK |= (GINTMSK_MODE_MISMATCH | GINTMSK_OTG_INTR); */
    /* Force to device mode */
    gusbcfg &= ~(1 << 29);
    gusbcfg |= (1 << 30);
    REG_GUSB_CFG = gusbcfg;
    mdelay(1);
}


int dwc_set_config(dwc_handle *dwc)
{
    return 0;
}

int dwc_set_ep_stall(dwc_handle *dwc, uint8_t epnum)
{
    depctl_data_t        depctl;
    dwc_ep  *pep = RT_NULL;

    if (epnum & USB_DIR_IN)
    {
        pep = dwc->dep[(epnum & 0x0F) + DWC_EP_IN_OFS];
    }
    else
    {
        pep = dwc->dep[(epnum & 0x0F) + DWC_EP_OUT_OFS];
    }
    epnum &= DWC_EPNO_MASK;

    depctl.d32 = REG_DIEP_CTL(epnum);

    if (depctl.b.epena)
    {
        depctl.b.epdis = 1;
    }
    depctl.b.stall = 1;
    REG_DIEP_CTL(epnum) = depctl.d32;

    depctl.d32 = REG_DOEP_CTL(epnum);
    depctl.b.stall = 1;
    REG_DOEP_CTL(epnum) = depctl.d32;

    pep->ep_state = EP_SETUP;
    dwc_otg_ep0_out_start(dwc);

    return 0;
}

int dwc_clr_ep_stall(dwc_handle *dwc, uint8_t epnum)
{
    depctl_data_t        depctl;
    dwc_ep  *pep = RT_NULL;

    if (epnum & USB_DIR_IN)
    {
        pep = dwc->dep[(epnum & 0x0F) + DWC_EP_IN_OFS];
    }
    else
    {
        pep = dwc->dep[(epnum & 0x0F) + DWC_EP_OUT_OFS];
    }
    epnum &= DWC_EPNO_MASK;

    if (pep->is_in)
    {
        depctl.d32 = REG_DIEP_CTL(epnum);
        depctl.b.stall = 0;
        REG_DIEP_CTL(epnum) = depctl.d32;
    }
    else
    {
        depctl.d32 = REG_DOEP_CTL(epnum);
        depctl.b.stall = 0;
        REG_DOEP_CTL(epnum) = depctl.d32;
    }

    return 0;
}

int dwc_ep_disable(dwc_handle *dwc, uint8_t epnum)
{
    depctl_data_t        depctl;
    daint_data_t         daintmsk;
    dwc_ep  *pep = RT_NULL;

    DWC_DBG("%s epnum = %02x\n", epnum);

    if (epnum & USB_DIR_IN)
    {
        pep = dwc->dep[(epnum & 0x0F) + DWC_EP_IN_OFS];
    }
    else
    {
        pep = dwc->dep[(epnum & 0x0F) + DWC_EP_OUT_OFS];
    }
    epnum &= DWC_EPNO_MASK;

    /* EP0 can not deactivate! */
    if (epnum == 0)
        return -1;

    daintmsk.d32 = REG_DAINT_MASK;
    if (pep->is_in)
    {
        depctl.d32 = REG_DIEP_CTL(epnum);
        daintmsk.ep.in &= ~(1 << epnum);
    }
    else
    {
        depctl.d32 = REG_DOEP_CTL(epnum);
        daintmsk.ep.out &= ~(1 << epnum);
    }
    if (!depctl.b.usbactep)
    {
        DWC_DBG("EP %d already deactivated\n", pep->num);
        return 0;
    }

    depctl.b.usbactep = 0;
    if (pep->is_in)
    {
        REG_DIEP_CTL(epnum) = depctl.d32;
    }
    else
    {
        REG_DOEP_CTL(epnum) = depctl.d32;
    }

    /* mask EP interrupts */
    REG_DAINT_MASK = daintmsk.d32;

    if (pep->is_in)
    {/* Disable IN-EP */

    }
    else
    {
        /* Disable IN-EP */
    }

    DWC_DBG("EP %d deactivated\n", pep->num);
    return 0;
}

static void udc_usb_set_device_only_mode(dwc_handle *dwc)
{
}

static void udc_usb_phy_init(dwc_handle *dwc)
{
    REG_GUSB_CFG &= ~USBCFG_ULPI_UTMI_SEL;
    REG_GUSB_CFG |= USBCFG_PHYIF;
    REG_GUSB_CFG &= ~USBCFG_PHYSEL;
    /* speed initialize */
    REG_OTG_DCFG &= ~DCFG_DEV_SPDMODE;
#ifdef DWC_FORCE_SPEED_FULL
    REG_OTG_DCFG |= DCFG_DEV_SPD;
#else
    REG_OTG_DCFG &= ~DCFG_DEV_SPD;
#endif
}

/* usb device init */
static void dwc_gadget_init(dwc_handle *dwc)
{
    uint32_t curmod;

    rt_hw_interrupt_mask(IRQ_OTG);

    /* force usb device mode */
    udc_usb_set_device_only_mode(dwc);

    udc_usb_phy_init(dwc);

    /* soft disconnect and soft reset */
    REG_OTG_DCTL |= DCTL_SOFT_DISCONN;
    udelay(3000);

    /* reset dwc register */
    dwc_otg_core_reset(dwc);

    /* DWC OTG Core init */
    dwc_otg_core_init(dwc, 1);

    /* Read Gintsts confirm the device or host mode */
    curmod = REG_GINT_STS;
    if (curmod & 0x1)
    {
        DWC_DBG("Curmod: Host Mode\n");
    }
    else
    {
        DWC_DBG("Curmod: Device Mode\n");

        /* DWC OTG Device init */
        dwc_otg_device_init(dwc);

        /* DWC OTG Fifo init */
        dwc_calculate_fifo_size(dwc);
    }

    /* End-point has been inited */
}

static void udc_usbd_isr_service(void *param)
{
    dwc_handle *dwc = (dwc_handle *)param;
    uint32_t    intsts;

    RT_ASSERT(dwc != RT_NULL);


    {
        if (REG_GOTG_INTR & BIT2)
            REG_GOTG_INTR |= BIT2;

        intsts = REG_GINT_STS & REG_GINT_MASK;

        MY_DWC_DBG("usbd isr %x %x\n",intsts, REG_GINT_MASK);

        if (intsts & GINTSTS_OTG_INTR)
        {
            MY_DWC_DBG("dwc_otg_intr!!!\n");
            dwc_otg_intr(dwc);
        }

        if ((intsts & GINTSTS_USB_EARLYSUSPEND)
                        || (intsts & GINTSTS_USB_SUSPEND)
                        || (intsts & GINTSTS_START_FRAM)
                        || (intsts & GINTSTS_USB_RESET)
                        || (intsts & GINTSTS_ENUM_DONE))
        {
            MY_DWC_DBG("dwc_common_intr!!!\n");
            dwc_common_intr(dwc, intsts);
        }

        /* dwc in pio mode not dma mode */
        if (intsts & GINTSTS_RXFIFO_NEMPTY)
        {
            MY_DWC_DBG("GINTSTS_RXFIFO_NEMPTY!!!\n");
            if (dwc->is_dma == IS_SLAVE_MODE)
                dwc_handle_rxfifo_nempty(dwc);

            REG_GINT_STS = GINTSTS_RXFIFO_NEMPTY;
        }

        if (intsts & GINTSTS_IEP_INTR)
        {
            // DWC_DBG("IEP_INTR!!!\n");
            dwc_handle_inep_intr(dwc);
        }

        if (intsts & GINTSTS_OEP_INTR)
        {
            // DWC_DBG("OEP_INTR!!!\n");
            dwc_handle_outep_intr(dwc);
        }

        if (intsts & GINTSTS_RSUME_DETE)
        {
            MY_DWC_DBG("RESUME_INTR\n");
            dwc_handle_resume_intr(dwc);
        }

        if (intsts & GINTSTS_ISOINCMP_INTR)
        {
            uint32_t intr = REG_DIEP_INT(3 & 0x0F);
            uint32_t size = (REG_DIEP_SIZE(3) & 0x7ffff);

            if ((intr & DEP_INTOKEN_RECV_TXFIFO_EMPTY) && !(intr & DEP_XFER_COMP))
            {
                if (size)
                    rt_kprintf("DEP_INTOKEN_RECV_TXFIFO_EMPTY %d size %x\n", 3, size);
                REG_DIEP_INT(3) = DEP_INTOKEN_RECV_TXFIFO_EMPTY;
            }
            REG_GINT_STS = GINTSTS_ISOINCMP_INTR;
        }

        if (intsts & (1 << 5))
        {
            MY_DWC_DBG("REG_GINT_STS 5\n");
            REG_GINT_STS = 1 << 5;
        }

        if (intsts & (1 << 15))
        {
            MY_DWC_DBG("REG_GINT_STS 15\n");
            REG_GINT_STS = 1 << 15;
        }

        if (intsts & (1 << 18))
        {
            MY_DWC_DBG("REG_GINT_STS 18\n");
            REG_GINT_STS = 1 << 18;
        }

        if (intsts & (1 << 26))
        {
            MY_DWC_DBG("REG_GINT_STS 26\n");
            REG_GINT_STS = 1 << 26;
        }

        if (intsts & (1 << 31))
        {
            REG_GINT_STS = 1 << 31;
        }

    }
}

void udc_usbd_init(dwc_handle *dwc)
{
    if (dwc->isr_sem == RT_NULL)
    {
        dwc->isr_sem = rt_sem_create("dwcSem", 0, RT_IPC_FLAG_FIFO);
        if (!dwc->isr_sem)
        {
            DWC_DBG("%s %d sem create err\n", __func__, __LINE__);
            while (1)
                ;
        }

        dwc->status.b.state = USB_CABLE_DISCONNECT;
        dwc->status.b.event = 0;
    }

    dwc_gadget_init(dwc);

    /* request irq */
    rt_hw_interrupt_install(IRQ_OTG, dwc_irq_handler, (void *)dwc, "otgISR");
    rt_hw_interrupt_umask(IRQ_OTG);
    DWC_DBG("[DWC] DWC request IRQ success %x\n", REG_GINT_MASK);
}
