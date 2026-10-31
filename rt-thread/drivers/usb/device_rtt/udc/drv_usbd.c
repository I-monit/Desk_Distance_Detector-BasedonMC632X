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

#include <rtthread.h>
#include "drivers/usb_device.h"
#include "fh_clock.h"
#include "board_info.h"
#include "fh_usbotg.h"

#include "udc_dwc.h"

static struct udcd __udc_usbd;
static dwc_handle __dwc_hdl;
rt_uint32_t otg_base = 0, irq_otg = 0;

/*#define DWC_FORCE_SPEED_FULL*/
/*#define USBD_DEBUG*/
#ifdef USBD_DEBUG
#define USBD_DBG(fmt, args...)   rt_kprintf(fmt, ##args)
#else
#define USBD_DBG(fmt, args...)
#endif

static struct ep_id __ep_pool[] = {
    {0x00, USB_EP_ATTR_CONTROL,     USB_DIR_INOUT,  64, ID_ASSIGNED},
#ifdef DWC_FORCE_SPEED_FULL
    {0x01, USB_EP_ATTR_INT,         USB_DIR_OUT,    64, ID_UNASSIGNED},
    {0x01, USB_EP_ATTR_INT,         USB_DIR_IN,     64, ID_UNASSIGNED},
    {0x02, USB_EP_ATTR_BULK,        USB_DIR_OUT,    64, ID_UNASSIGNED},
    {0x02, USB_EP_ATTR_BULK,        USB_DIR_IN,     64, ID_UNASSIGNED},
    {0x04, USB_EP_ATTR_BULK,        USB_DIR_OUT,    64, ID_UNASSIGNED},
    {0x04, USB_EP_ATTR_BULK,        USB_DIR_IN,     64, ID_UNASSIGNED},
#else
    {0x01, USB_EP_ATTR_INT,         USB_DIR_IN,    512, ID_UNASSIGNED},
    {0x01, USB_EP_ATTR_INT,         USB_DIR_OUT,   512, ID_UNASSIGNED},

    {0x02, USB_EP_ATTR_BULK,        USB_DIR_OUT,   512, ID_UNASSIGNED},
    {0x02, USB_EP_ATTR_BULK,        USB_DIR_IN,    512, ID_UNASSIGNED},
    {0x04, USB_EP_ATTR_BULK,        USB_DIR_OUT,   512, ID_UNASSIGNED},
    {0x04, USB_EP_ATTR_BULK,        USB_DIR_IN,    512, ID_UNASSIGNED},
    /*{0x06, USB_EP_ATTR_BULK,        USB_DIR_OUT,   512, ID_UNASSIGNED},*/
    /*{0x06, USB_EP_ATTR_BULK,        USB_DIR_IN,    512, ID_UNASSIGNED},*/
#endif
    {0xFF, USB_EP_ATTR_TYPE_MASK,   USB_DIR_MASK,   0,  ID_ASSIGNED}
};

#define __is_print(ch) ((unsigned int)((ch) - ' ') < 127u - ' ')
#if 0
static void dump_hex(const rt_uint8_t *ptr, rt_size_t buflen)
{
    unsigned char *buf = (unsigned char *)ptr;
    int i, j;

    for (i = 0; i < buflen; i += 16)
    {
        rt_kprintf("%06x: ", i);
        for (j = 0; j < 16; j++)
            if (i + j < buflen)
                rt_kprintf("%02x ", buf[i + j]);
            else
                rt_kprintf(" ");
        rt_kprintf(" ");
        for (j = 0; j < 16; j++)
            if (i + j < buflen)
                rt_kprintf("%c", __is_print(buf[i + j]) ? buf[i + j] : '.');
        rt_kprintf("\n");
    }
}
#endif

static rt_err_t __ep_set_stall(rt_uint8_t address)
{

    RT_DEBUG_LOG(RT_DEBUG_USB, ("ep set_stall, address 0x%x\n", address));
    rt_kprintf("ep set_stall, address 0x%x\n", address);
    dwc_set_ep_stall(&__dwc_hdl, address);

    return RT_EOK;
}

static rt_err_t __ep_clear_stall(rt_uint8_t address)
{
    RT_DEBUG_LOG(RT_DEBUG_USB, ("ep clear_stall, address 0x%x\n", address));

    dwc_clr_ep_stall(&__dwc_hdl, address);

    return RT_EOK;
}

static rt_err_t __set_address(rt_uint8_t address)
{
    RT_DEBUG_LOG(RT_DEBUG_USB,("set address, 0x%x\n", address));

    dwc_set_address(&__dwc_hdl, address);
    return RT_EOK;
}

static rt_err_t __set_config(rt_uint8_t address)
{
    RT_DEBUG_LOG(RT_DEBUG_USB,("%s, 0x%x\n", __func__,address));

    __dwc_hdl.status.b.state = USB_CONFIGURED;

    return RT_EOK;
}

static rt_err_t __ep_enable(uep_t ep)
{
    RT_ASSERT(ep != RT_NULL);
    RT_ASSERT(ep->ep_desc != RT_NULL);
    RT_DEBUG_LOG(RT_DEBUG_USB, ("%s, address = %02x\n", __func__, EP_ADDRESS(ep)));

    if (ep->id->dir == USB_DIR_IN)
        dwc_enable_in_ep(&__dwc_hdl, ep->id->addr);
    else
        dwc_enable_out_ep(&__dwc_hdl, ep->id->addr);

    return RT_EOK;
}

static rt_err_t __ep_disable(uep_t ep)
{
    RT_ASSERT(ep != RT_NULL);
    RT_ASSERT(ep->ep_desc != RT_NULL);
    RT_DEBUG_LOG(RT_DEBUG_USB,("%s\n", __func__));

    return RT_EOK;
}

static  rt_size_t __ep_read_prepare(rt_uint8_t address, void *buffer, rt_size_t size)
{
    dwc_ep *pep;

    RT_DEBUG_LOG(RT_DEBUG_USB,("%s address = %02x, size = %d\n", __func__, address, size));

    pep = __dwc_hdl.dep[(address & 0x0F) + DWC_EP_OUT_OFS];
    pep->ep_state   = EP_DATA;
    pep->xfer_len   = size;
    pep->xfer_count = 0;
    dwc_handle_ep_data_out_phase(&__dwc_hdl, address);
    return size;
}

static rt_size_t __ep_read(rt_uint8_t address, void *buffer)
{
    rt_size_t size = 0;

    RT_ASSERT(buffer != RT_NULL);
    RT_DEBUG_LOG(RT_DEBUG_USB,("%s\n", __func__));

    size = HW_GetPKT(&__dwc_hdl, address, (uint8_t *)buffer, 0);

    return size;
}

static rt_size_t __ep_write(rt_uint8_t address, void *buffer, rt_size_t size)
{
    RT_DEBUG_LOG(RT_DEBUG_USB, ("%s address = %02x, buffer = %08x, size = %d\n",
                  __func__, address, (uint32_t)buffer, size));

    size = HW_SendPKT(&__dwc_hdl, address, (const uint8_t *)buffer, size);
    return size;
}

static rt_err_t __ep0_send_status(void)
{
    RT_DEBUG_LOG(RT_DEBUG_USB,("%s\n", __func__));

    HW_SendPKT(&__dwc_hdl, 0, 0, 0);
    return RT_EOK;
}

static rt_err_t __suspend(void)
{
    RT_DEBUG_LOG(RT_DEBUG_USB, ("%s\n", __func__));

    return RT_EOK;
}

static rt_err_t __wakeup(void)
{
    RT_DEBUG_LOG(RT_DEBUG_USB, ("%s\n", __func__));

    return RT_EOK;
}

static rt_err_t __set_test_mode(rt_uint8_t test_mode)
{
    RT_DEBUG_LOG(RT_DEBUG_USB, ("%s, test_mode = %d\n", __func__, test_mode));

    __dwc_hdl.test_mode = test_mode;

    return RT_EOK;
}
static void fh_board_usbotg_prepare(void *info_data)
{
    struct fh_usbotg_obj *usbotg_obj = info_data;
    struct clk *usb_clk = RT_NULL;

    usb_clk = clk_get(NULL, "usb_clk");
    if (usb_clk == RT_NULL)
        rt_kprintf("warning: can not find usb_clk\n");
    else
        clk_enable(usb_clk);

    if (usbotg_obj == RT_NULL)
    {
        rt_kprintf("warning: can not find usbotg_obj\n");
    }
    else
    {
        if (usbotg_obj->power_on == RT_NULL)
            rt_kprintf("warning: can not find usbotg_obj->power_on\n");
    }

    if (usbotg_obj != RT_NULL
        && usbotg_obj->power_on != RT_NULL)
        usbotg_obj->power_on();

    if (usbotg_obj != RT_NULL
        && usbotg_obj->utmi_rst != RT_NULL)
        usbotg_obj->utmi_rst();

    if (usbotg_obj != RT_NULL
        && usbotg_obj->phy_rst != RT_NULL)
        usbotg_obj->phy_rst();

    if (usbotg_obj != RT_NULL
        && usbotg_obj->vbus_vldext != RT_NULL)
        usbotg_obj->vbus_vldext();

    if (usbotg_obj != RT_NULL
        && usbotg_obj->base != 0)
        otg_base = usbotg_obj->base;

    if (usbotg_obj != RT_NULL
        && usbotg_obj->irq != 0)
        irq_otg = usbotg_obj->irq;
}

static rt_err_t __init(rt_device_t device)
{
    int epidx = 0, epnum = 0;

    __dwc_hdl.test_mode = 0;
    __dwc_hdl.status.b.state = USB_CABLE_DISCONNECT;
    /* clear all dep */
    for (epidx = 0; epidx < 32; epidx++)
    {
        __dwc_hdl.dep[epidx] = RT_NULL;
    }

    for (epidx = 0; __ep_pool[epidx].addr != 0xFF; ++epidx)
    {
        dwc_ep      *pep = RT_NULL;
        rt_uint8_t  *pXfer = RT_NULL;

        if (epidx == 0)
        { /* EP0 is IN-OUT */
            pep = (dwc_ep *) rt_malloc(sizeof(dwc_ep));
            if (!pep)
            {
                rt_kprintf("ERROR: no memory for pep\n");
                while (1)
                    ;
            }
            /* malloc memory for EP */
            pXfer = rt_malloc_align(__ep_pool[epidx].maxpacket * 2, 32);
            rt_kprintf("pep memory addr for ep0:0x%x\n", pXfer);
            if (!pXfer)
            {
                rt_kprintf("ERROR: no memory for pXfer\n");
                while (1)
                    ;
            }
            /* init pep */
            if (pep)
            {
                pep->num        = 0;
                pep->ep_state   = EP_SETUP;
                pep->is_in      = 0;
                pep->active     = 0;
                pep->type       = DWC_OTG_EP_TYPE_CONTROL;
                pep->maxpacket  = __ep_pool[epidx].maxpacket;
                /*pep->xfer_buff  = (void *)UNCACHED(pXfer);*/
                pep->xfer_buff  = (void *)(pXfer);
                pep->xfer_len   = 0;
                pep->xfer_count = 0;
            }

            __dwc_hdl.dep[0 + DWC_EP_IN_OFS]    = pep;
            __dwc_hdl.dep[0 + DWC_EP_OUT_OFS]   = pep;
        }
        else
        {
            pep = (dwc_ep *) rt_malloc(sizeof(dwc_ep));
            if (!pep)
            {
                rt_kprintf("ERROR: no memory for pep\n");
                while (1)
                    ;
            }

            /* malloc memory for EP */
            pXfer = rt_malloc_align(__ep_pool[epidx].maxpacket * 2, 32);
            if (!pXfer)
            {
                rt_kprintf("ERROR: no memory for pXfer\n");
                while (1)
                    ;
            }

            /* init pep */
            {
                pep->num        = __ep_pool[epidx].addr;
                pep->ep_state   = EP_IDLE;
                pep->is_in      = (__ep_pool[epidx].dir == USB_DIR_IN) ? 1 : 0;
                pep->active     = 0;
                pep->type       = __ep_pool[epidx].type;
                pep->maxpacket  = __ep_pool[epidx].maxpacket;
                pep->xfer_buff  = (void *)(pXfer);
                pep->xfer_len   = 0;
                pep->xfer_count = 0;
            }
            if (__ep_pool[epidx].dir == USB_DIR_OUT)
                epnum = __ep_pool[epidx].addr + DWC_EP_OUT_OFS;
            else
                epnum = __ep_pool[epidx].addr + DWC_EP_IN_OFS;
            __dwc_hdl.dep[epnum] = pep;
        }
    }
    fh_board_usbotg_prepare(device->user_data);
    udc_usbd_init(&__dwc_hdl);

    return RT_EOK;
}


static struct udcd_ops __udc_usbd_ops = {
    __set_address,
    __set_config,
    __ep_set_stall,
    __ep_clear_stall,
    __ep_enable,
    __ep_disable,
    __ep_read_prepare,
    __ep_read,
    __ep_write,
    __ep0_send_status,
    __suspend,
    __wakeup,
    __set_test_mode,
};


void udc_usbd_event_cb(uint8_t address, uint32_t event, void *arg)
{
    switch (event)
    {
    case USB_EVT_SETUP:
        USBD_DBG("USB_EVT_SETUP\n");
        if (address == 0)
        {
            rt_usbd_ep0_setup_handler(&__udc_usbd, (struct urequest *)arg);
        }
        break;
    case USB_EVT_OUT:
        USBD_DBG("USB_EVT_OUT\n");
        if (address == 0)
            rt_usbd_ep0_out_handler(&__udc_usbd, (rt_size_t)arg);
        else
            rt_usbd_ep_out_handler(&__udc_usbd, USB_DIR_OUT | address, 0);
        break;
    case USB_EVT_IN:
        USBD_DBG("USB_EVT_IN\n");
        if (address == 0)
            rt_usbd_ep0_in_handler(&__udc_usbd);
        else
            rt_usbd_ep_in_handler(&__udc_usbd, USB_DIR_IN | address,
                      __dwc_hdl.dep[DWC_EP_IN_OFS + address]->xfer_count);
        break;
    case USB_EVT_SOF:
        rt_usbd_sof_handler(&__udc_usbd);
        break;
    default:
        break;
    }
}

int udc_usbd_register(void)
{
    rt_memset((void *)&__udc_usbd, 0, sizeof(struct udcd));

    __udc_usbd.parent.type    = RT_Device_Class_USBDevice;
    __udc_usbd.parent.init    = __init;
    __udc_usbd.parent.user_data = fh_get_board_info_data("fh_otg");

    __udc_usbd.ops            = &__udc_usbd_ops;

    /* Register endpoint information */
    __udc_usbd.ep_pool        = __ep_pool;
    __udc_usbd.ep0.id         = &__ep_pool[0];

#ifdef DWC_FORCE_SPEED_FULL
    __udc_usbd.device_is_hs = 0;
#else
    __udc_usbd.device_is_hs = 1;
#endif

    /*rt_device_register(&__udc_usbd.parent, "usbd", 0);*/
    rt_device_register((rt_device_t)&__udc_usbd, "usbd", 0);

    return RT_EOK;
}
INIT_ENV_EXPORT(udc_usbd_register);
