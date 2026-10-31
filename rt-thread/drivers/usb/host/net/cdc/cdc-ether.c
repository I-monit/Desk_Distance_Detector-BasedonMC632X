#include <rtdef.h>
#include <rtthread.h>
#include <rtdevice.h>
#include <rthw.h>
#include <dma_mem.h>
#include <interrupt.h>
#include <sys/types.h>
#include <cdc.h>
#include "usbnet.h"
#include "rndis_host.h"
#include <mii.h>

static const __u8 mbm_guid[16] = {
    0xa3, 0x17, 0xa8, 0x8b, 0x04, 0x5e, 0x4f, 0x01,
    0xa6, 0x07, 0xc0, 0xff, 0xcb, 0x7e, 0x39, 0x2a,
};

#if defined(RT_USING_USB_RNDIS)

static int is_rndis(struct usb_interface_descriptor *desc)
{
    return (desc->bInterfaceClass == USB_CLASS_COMM &&
        desc->bInterfaceSubClass == 2 &&
        desc->bInterfaceProtocol == 0xff);
}

static int is_activesync(struct usb_interface_descriptor *desc)
{
    return (desc->bInterfaceClass == USB_CLASS_MISC &&
        desc->bInterfaceSubClass == 1 &&
        desc->bInterfaceProtocol == 1);
}

static int is_wireless_rndis(struct usb_interface_descriptor *desc)
{
    return (desc->bInterfaceClass == USB_CLASS_WIRELESS_CONTROLLER &&
        desc->bInterfaceSubClass == 1 &&
        desc->bInterfaceProtocol == 3);
}

#else

#define is_rndis(desc)          0
#define is_activesync(desc)     0
#define is_wireless_rndis(desc) 0

#endif
int cdc_parse_cdc_header(struct usb_cdc_parsed_header *hdr,
                struct usb_interface *intf,
                __u8 *buffer,
                int buflen)
{
    /* duplicates are ignored */
    struct usb_cdc_union_desc *union_header = NULL;

    /* duplicates are not tolerated */
    struct usb_cdc_header_desc *header = NULL;
    struct usb_cdc_ether_desc *ether = NULL;
    struct usb_cdc_mdlm_detail_desc *detail = NULL;
    struct usb_cdc_mdlm_desc *desc = NULL;

    unsigned int elength;
    int cnt = 0;

    memset(hdr, 0x00, sizeof(struct usb_cdc_parsed_header));
    while (buflen > 0)
    {
        elength = buffer[0];
        if (!elength)
        {
            rt_kprintf("skipping garbage byte\n");
            elength = 1;
            goto next_desc;
        }
        if ((buflen < elength) || (elength < 3))
        {
            rt_kprintf("invalid descriptor buffer length\n");
            break;
        }
        if (buffer[1] != USB_DT_CS_INTERFACE)
        {
            rt_kprintf("skipping garbage\n");
            goto next_desc;
        }

        switch (buffer[2])
        {
        case USB_CDC_UNION_TYPE: /* we've found it */
            if (elength < sizeof(struct usb_cdc_union_desc))
                goto next_desc;
            if (union_header)
            {
                rt_kprintf("More than one union descriptor, skipping ...\n");
                goto next_desc;
            }
            union_header = (struct usb_cdc_union_desc *)buffer;
            break;
        case USB_CDC_COUNTRY_TYPE:
            if (elength < sizeof(struct usb_cdc_country_functional_desc))
                goto next_desc;
            hdr->usb_cdc_country_functional_desc =
                (struct usb_cdc_country_functional_desc *)buffer;
            break;
        case USB_CDC_HEADER_TYPE:
            if (elength != sizeof(struct usb_cdc_header_desc))
                goto next_desc;
            if (header)
                return -EINVAL;
            header = (struct usb_cdc_header_desc *)buffer;
            break;
        case USB_CDC_ACM_TYPE:
            if (elength < sizeof(struct usb_cdc_acm_descriptor))
                goto next_desc;
            hdr->usb_cdc_acm_descriptor =
                (struct usb_cdc_acm_descriptor *)buffer;
            break;
        case USB_CDC_ETHERNET_TYPE:
            if (elength != sizeof(struct usb_cdc_ether_desc))
                goto next_desc;
            if (ether)
                return -EINVAL;
            ether = (struct usb_cdc_ether_desc *)buffer;
            break;
        case USB_CDC_CALL_MANAGEMENT_TYPE:
            if (elength < sizeof(struct usb_cdc_call_mgmt_descriptor))
                goto next_desc;
            hdr->usb_cdc_call_mgmt_descriptor =
                (struct usb_cdc_call_mgmt_descriptor *)buffer;
            break;
        case USB_CDC_DMM_TYPE:
            if (elength < sizeof(struct usb_cdc_dmm_desc))
                goto next_desc;
            hdr->usb_cdc_dmm_desc =
                (struct usb_cdc_dmm_desc *)buffer;
            break;
        case USB_CDC_MDLM_TYPE:
            if (elength < sizeof(struct usb_cdc_mdlm_desc *))
                goto next_desc;
            if (desc)
                return -EINVAL;
            desc = (struct usb_cdc_mdlm_desc *)buffer;
            break;
        case USB_CDC_MDLM_DETAIL_TYPE:
            if (elength < sizeof(struct usb_cdc_mdlm_detail_desc *))
                goto next_desc;
            if (detail)
                return -EINVAL;
            detail = (struct usb_cdc_mdlm_detail_desc *)buffer;
            break;
        case USB_CDC_NCM_TYPE:
            if (elength < sizeof(struct usb_cdc_ncm_desc))
                goto next_desc;
            hdr->usb_cdc_ncm_desc = (struct usb_cdc_ncm_desc *)buffer;
            break;
        case USB_CDC_MBIM_TYPE:
            if (elength < sizeof(struct usb_cdc_mbim_desc))
                goto next_desc;

            hdr->usb_cdc_mbim_desc = (struct usb_cdc_mbim_desc *)buffer;
            break;
        case USB_CDC_MBIM_EXTENDED_TYPE:
            if (elength < sizeof(struct usb_cdc_mbim_extended_desc))
                break;
            hdr->usb_cdc_mbim_extended_desc =
                (struct usb_cdc_mbim_extended_desc *)buffer;
            break;
        default:
            /*
             * there are LOTS more CDC descriptors that
             * could legitimately be found here.
             */
            rt_kprintf("Ignoring descriptor: type %02x, length %ud\n",
                    buffer[2], elength);
            goto next_desc;
        }
        cnt++;
next_desc:
        buflen -= elength;
        buffer += elength;
    }
    hdr->usb_cdc_union_desc = union_header;
    hdr->usb_cdc_header_desc = header;
    hdr->usb_cdc_mdlm_detail_desc = detail;
    hdr->usb_cdc_mdlm_desc = desc;
    hdr->usb_cdc_ether_desc = ether;
    return cnt;
}

/*
 * probes control interface, claims data interface, collects the bulk
 * endpoints, activates data interface (if needed), maybe sets MTU.
 * all pure cdc, except for certain firmware workarounds, and knowing
 * that rndis uses one different rule.
 */
int usbnet_generic_cdc_bind(struct usbnet *dev, struct usb_interface *intf)
{
    __u8            *buf = intf->cur_altsetting->extra;
    int             len = intf->cur_altsetting->extralen;
    struct usb_interface_descriptor *d;
    struct cdc_state       *info = (void *) &dev->data;
    int             status;
    int     rndis;
    struct usb_cdc_parsed_header header;

    if (sizeof(dev->data) < sizeof(*info))
        return -EDOM;

    /* expect strict spec conformance for the descriptors, but
     * cope with firmware which stores them in the wrong place
     */
    if (len == 0 && dev->udev->actconfig->extralen)
    {
        /* Motorola SB4100 (and others: Brad Hards says it's
         * from a Broadcom design) put CDC descriptors here
         */
        buf = dev->udev->actconfig->extra;
        len = dev->udev->actconfig->extralen;
        rt_kprintf("CDC descriptors on config\n");
    }

    /* Maybe CDC descriptors are after the endpoint?  This bug has
     * been seen on some 2Wire Inc RNDIS-ish products.
     */
    if (len == 0)
    {
        struct usb_host_endpoint    *hep;

        hep = intf->cur_altsetting->endpoint;
        if (hep)
        {
            buf = hep->extra;
            len = hep->extralen;
        }
        if (len)
            rt_kprintf("CDC descriptors on endpoint\n");
    }

    /* this assumes that if there's a non-RNDIS vendor variant
     * of cdc-acm, it'll fail RNDIS requests cleanly.
     */
    rndis = (is_rndis(&intf->cur_altsetting->desc) ||
         is_activesync(&intf->cur_altsetting->desc) ||
         is_wireless_rndis(&intf->cur_altsetting->desc));

    memset(info, 0, sizeof(*info));
    info->control = intf;


    cdc_parse_cdc_header(&header, intf, buf, len);

    info->u = header.usb_cdc_union_desc;
    info->header = header.usb_cdc_header_desc;
    info->ether = header.usb_cdc_ether_desc;
    if (!info->u)
    {
        if (rndis)
            goto skip;
        else /* in that case a quirk is mandatory */
            goto bad_desc;
    }
    /* we need a master/control interface (what we're
     * probed with) and a slave/data interface; union
     * descriptors sort this all out.
     */
    info->control = usb_ifnum_to_if(dev->udev,
    info->u->bMasterInterface0);
    info->data = usb_ifnum_to_if(dev->udev,
        info->u->bSlaveInterface0);
    if (!info->control || !info->data)
    {
        rt_kprintf("master #%u/%p slave #%u/%p\n",
            info->u->bMasterInterface0,
            info->control,
            info->u->bSlaveInterface0,
            info->data);
        /* fall back to hard-wiring for RNDIS */
        if (rndis)
        {
            goto skip;
        }
        goto bad_desc;
    }
    if (info->control != intf)
    {
        rt_kprintf("bogus CDC Union\n");
        /* Ambit USB Cable Modem (and maybe others)
         * interchanges master and slave interface.
         */
        if (info->data == intf)
        {
            info->data = info->control;
            info->control = intf;
        } else
            goto bad_desc;
    }

    /* some devices merge these - skip class check */
    if (info->control == info->data)
        goto skip;

    /* a data interface altsetting does the real i/o */
    d = &info->data->cur_altsetting->desc;
    if (d->bInterfaceClass != USB_CLASS_CDC_DATA)
    {
        rt_kprintf("slave class %u\n",
            d->bInterfaceClass);
        goto bad_desc;
    }
skip:
    if (rndis &&
        header.usb_cdc_acm_descriptor &&
        header.usb_cdc_acm_descriptor->bmCapabilities)
    {
        rt_kprintf("ACM capabilities %02x, not really RNDIS?\n",
            header.usb_cdc_acm_descriptor->bmCapabilities);
        goto bad_desc;
    }

    if (header.usb_cdc_ether_desc && info->ether->wMaxSegmentSize)
    {
        dev->hard_mtu = le16_to_cpu(info->ether->wMaxSegmentSize);
        /* because of Zaurus, we may be ignoring the host
         * side link address we were given.
         */
    }

    if (header.usb_cdc_mdlm_desc &&
        memcmp(header.usb_cdc_mdlm_desc->bGUID, mbm_guid, 16))
        {
        rt_kprintf("GUID doesn't match\n");
        goto bad_desc;
    }

    if (header.usb_cdc_mdlm_detail_desc &&
        header.usb_cdc_mdlm_detail_desc->bLength <
            (sizeof(struct usb_cdc_mdlm_detail_desc) + 1))
            {
        rt_kprintf("Descriptor too short\n");
        goto bad_desc;
    }



    /* Microsoft ActiveSync based and some regular RNDIS devices lack the
     * CDC descriptors, so we'll hard-wire the interfaces and not check
     * for descriptors.
     */
    if (rndis && !info->u)
    {
        info->control = usb_ifnum_to_if(dev->udev, 0);
        info->data = usb_ifnum_to_if(dev->udev, 1);
        if (!info->control || !info->data || info->control != intf)
        {
            rt_kprintf("rndis: master #0/%p slave #1/%p\n",
                info->control,
                info->data);
            goto bad_desc;
        }

    }
    else if (!info->header || (!rndis && !info->ether))
    {
        rt_kprintf("missing cdc %s%s%sdescriptor\n",
        info->header ? "" : "header ",
        info->u ? "" : "union ",
        info->ether ? "" : "ether ");
        goto bad_desc;
    }

    /* claim data interface and set it up ... with side effects.
     * network traffic can't flow until an altsetting is enabled.
     */

    if (info->data != info->control)
    {
        usb_set_intfdata(info->data, dev);
        intf->needs_binding = 0;
        intf->condition = USB_INTERFACE_BOUND;
    }
    status = usbnet_get_endpoints(dev, info->data);
    if (status < 0)
    {
        /* ensure immediate exit from usbnet_disconnect */
        usb_set_intfdata(info->data, NULL);
        if (info->data != info->control)
        {
            intf->condition = USB_INTERFACE_UNBINDING;
        }

        return status;
    }

    /* status endpoint: optional for CDC Ethernet, not RNDIS (or ACM) */
    if (info->data != info->control)
        dev->status = NULL;
    if (info->control->cur_altsetting->desc.bNumEndpoints == 1)
    {
        struct usb_endpoint_descriptor  *desc;

        dev->status = &info->control->cur_altsetting->endpoint [0];
        desc = &dev->status->desc;
        if (!usb_endpoint_is_int_in(desc) ||
            (le16_to_cpu(desc->wMaxPacketSize)
             < sizeof(struct usb_cdc_notification)) ||
            !desc->bInterval) {
            rt_kprintf("bad notification endpoint\n");
            dev->status = NULL;
        }
    }
    if (rndis && !dev->status)
    {
        rt_kprintf("missing RNDIS status endpoint\n");
        usb_set_intfdata(info->data, NULL);
        intf->condition = USB_INTERFACE_UNBINDING;
        return -ENODEV;
    }

    /* Some devices don't initialise properly. In particular
     * the packet filter is not reset. There are devices that
     * don't do reset all the way. So the packet filter should
     * be set to a sane initial value.
     */

    return 0;

bad_desc:
    rt_kprintf("bad CDC descriptors\n");
    return -ENODEV;
}

#ifdef RT_USING_USB_ECM

void usbnet_cdc_unbind(struct usbnet *dev, struct usb_interface *intf)
{
    struct cdc_state		*info = (void *) &dev->data;

    /* combined interface - nothing  to do */
    if (info->data == info->control)
        return;

    /* disconnect master --> disconnect slave */
    if (intf == info->control && info->data) {
        /* ensure immediate exit from usbnet_disconnect */
        usb_set_intfdata(info->data, NULL);
        info->data = NULL;
    }

    /* and vice versa (just in case) */
    else if (intf == info->data && info->control) {
        /* ensure immediate exit from usbnet_disconnect */
        usb_set_intfdata(info->control, NULL);
        info->control = NULL;
    }
}

int usbnet_cdc_bind(struct usbnet *dev, struct usb_interface *intf)
{
    int				status;
    struct cdc_state		*info = (void *) &dev->data;

    status = usbnet_generic_cdc_bind(dev, intf);
    if (status < 0)
        return status;

    status = usbnet_get_ethernet_addr(dev, info->ether->iMACAddress);
    if (status < 0) {
        usb_set_intfdata(info->data, NULL);
        return status;
    }

    return 0;
}

void usbnet_cdc_status(struct usbnet *dev, struct urb *urb)
{
    struct usb_cdc_notification	*event;
    unsigned int *speeds;
    int link;
    unsigned int old_link;

    if (urb->actual_length < sizeof(*event))
        return;

    /* SPEED_CHANGE can get split into two 8-byte packets */
    if (test_and_clear_bit(EVENT_STS_SPLIT, &dev->flags)) {
        speeds = (unsigned int *)urb->transfer_buffer;
        rt_kprintf("[ECM] link speeds: %u kbps up, %u kbps down\n",
           (speeds[0]) / 1000, (speeds[1]) / 1000);
        return;
    }

    event = urb->transfer_buffer;
    switch (event->bNotificationType) {
    case USB_CDC_NOTIFY_NETWORK_CONNECTION:
        rt_kprintf("[ECM] CDC: carrier %s\n", event->wValue ? "on" : "off");
        link = event->wValue & 0x01;
        old_link = dev->link_state;
        if (link != old_link)
        {
            rt_kprintf("%s Link Status : %s\n",
            dev->rtt_eth_interface.parent.parent.name,
            (link == 1) ? "UP" : "DOWN");
            dev->link_state = link;
            net_device_linkchange(&dev->rtt_eth_interface, dev->link_state);
        }
        break;
    case USB_CDC_NOTIFY_SPEED_CHANGE:	/* tx/rx rates */
        rt_kprintf("[ECM] CDC: speed change (len %d)\n",
              urb->actual_length);
        if (urb->actual_length != (sizeof(*event) + 8))
            set_bit(EVENT_STS_SPLIT, &dev->flags);
        else
        {
            speeds = (unsigned int *)&event[1];
            rt_kprintf("[ECM] link speeds: %u kbps up, %u kbps down\n",
           (speeds[0]) / 1000, (speeds[1]) / 1000);
        }
        break;
    /* USB_CDC_NOTIFY_RESPONSE_AVAILABLE can happen too (e.g. RNDIS),
     * but there are no standard formats for the response data.
     */
    default:
        rt_kprintf("[ECM] CDC: unexpected notification %02x!\n",
               event->bNotificationType);
        break;
    }
}
static unsigned int ecm_rx_fixup(struct usbnet *dev, unsigned char *rx_buf,
                unsigned int rx_buf_size, struct usbnet_rx_node *rx_node)
{
    struct pbuf *pbuf = pbuf_alloc(PBUF_RAW, USBNET_PBUF_MALLOC_LEN, PBUF_RAM);
    if (!pbuf)
    {
        rt_kprintf("malloc pbuf error...\n");
        return -1;
    }
    rt_memcpy(pbuf->payload, rx_buf, rx_buf_size);
    pbuf->len = pbuf->tot_len = rx_buf_size;
    usbnet_put_one_pbuf(dev, pbuf, &rx_node->list_done_pbuf);
    rx_node->rx_state_mac = GET_ACTIVE_NODE_PBUFF;

    return 0;
}

static unsigned char *ecm_tx_fixup(struct usbnet *dev,
        struct pbuf *p, unsigned char *tx_buf,
        unsigned int tx_buf_size, unsigned int *status, unsigned int *size)
{
    struct pbuf *temp_pbuf;
    unsigned char *temp_buf;

    *status = 0;
    if (p->tot_len > tx_buf_size)
    {
        rt_kprintf("[ECM] error: pbuf tot_len > tx_buf_size!\n");
        *status = -1;
        goto out;
    }

    for (temp_pbuf = p, temp_buf = tx_buf; temp_pbuf != NULL; temp_pbuf = temp_pbuf->next)
    {
        rt_memcpy(temp_buf, temp_pbuf->payload, temp_pbuf->len);
        temp_buf += temp_pbuf->len;
    }
    *size = p->tot_len;

out:
    return tx_buf;
}

static const struct driver_info	cdc_info = {
    .description =	"CDC Ethernet Device",
    .flags =    FLAG_ETHER | FLAG_POINTTOPOINT,
    .bind =     usbnet_cdc_bind,
    .unbind =	usbnet_cdc_unbind,
    .status =	usbnet_cdc_status,
    .rx_fixup = ecm_rx_fixup,
    .tx_fixup = ecm_tx_fixup,
};

static const struct driver_info wwan_info = {
    .description =	"Mobile Broadband Network Device",
    .flags =	FLAG_WWAN,
    .bind =		usbnet_cdc_bind,
    .unbind =	usbnet_cdc_unbind,
    .status =	usbnet_cdc_status,
    .rx_fixup = ecm_rx_fixup,
    .tx_fixup = ecm_tx_fixup,
};

#define HUAWEI_VENDOR_ID	0x12D1

static const struct usb_device_id   products[] = {
/*
 * BLACKLIST !!
 *
 * First blacklist any products that are egregiously nonconformant
 * with the CDC Ethernet specs.  Minor braindamage we cope with; when
 * they're not even trying, needing a separate driver is only the first
 * of the differences to show up.
 */

#define	ZAURUS_MASTER_INTERFACE \
    .bInterfaceClass	= USB_CLASS_COMM, \
    .bInterfaceSubClass	= USB_CDC_SUBCLASS_ETHERNET, \
    .bInterfaceProtocol	= USB_CDC_PROTO_NONE

/* SA-1100 based Sharp Zaurus ("collie"), or compatible;
 * wire-incompatible with true CDC Ethernet implementations.
 * (And, it seems, needlessly so...)
 */
{
    .match_flags	=   USB_DEVICE_ID_MATCH_INT_INFO
              | USB_DEVICE_ID_MATCH_DEVICE,
    .idVendor		= 0x04DD,
    .idProduct		= 0x8004,
    ZAURUS_MASTER_INTERFACE,
    .driver_info		= 0,
},

/* PXA-25x based Sharp Zaurii.  Note that it seems some of these
 * (later models especially) may have shipped only with firmware
 * advertising false "CDC MDLM" compatibility ... but we're not
 * clear which models did that, so for now let's assume the worst.
 */
{
    .match_flags	=   USB_DEVICE_ID_MATCH_INT_INFO
              | USB_DEVICE_ID_MATCH_DEVICE,
    .idVendor		= 0x04DD,
    .idProduct		= 0x8005,	/* A-300 */
    ZAURUS_MASTER_INTERFACE,
    .driver_info		= 0,
}, {
    .match_flags	=   USB_DEVICE_ID_MATCH_INT_INFO
              | USB_DEVICE_ID_MATCH_DEVICE,
    .idVendor		= 0x04DD,
    .idProduct		= 0x8006,	/* B-500/SL-5600 */
    ZAURUS_MASTER_INTERFACE,
    .driver_info		= 0,
}, {
    .match_flags    =   USB_DEVICE_ID_MATCH_INT_INFO
              | USB_DEVICE_ID_MATCH_DEVICE,
    .idVendor		= 0x04DD,
    .idProduct		= 0x8007,	/* C-700 */
    ZAURUS_MASTER_INTERFACE,
    .driver_info		= 0,
}, {
    .match_flags    =   USB_DEVICE_ID_MATCH_INT_INFO
         | USB_DEVICE_ID_MATCH_DEVICE,
    .idVendor               = 0x04DD,
    .idProduct              = 0x9031,	/* C-750 C-760 */
    ZAURUS_MASTER_INTERFACE,
    .driver_info		= 0,
}, {
    .match_flags    =   USB_DEVICE_ID_MATCH_INT_INFO
         | USB_DEVICE_ID_MATCH_DEVICE,
    .idVendor               = 0x04DD,
    .idProduct              = 0x9032,	/* SL-6000 */
    ZAURUS_MASTER_INTERFACE,
    .driver_info		= 0,
}, {
    .match_flags    =   USB_DEVICE_ID_MATCH_INT_INFO
         | USB_DEVICE_ID_MATCH_DEVICE,
    .idVendor               = 0x04DD,
    /* reported with some C860 units */
    .idProduct              = 0x9050,	/* C-860 */
    ZAURUS_MASTER_INTERFACE,
    .driver_info		= 0,
},

/* Olympus has some models with a Zaurus-compatible option.
 * R-1000 uses a FreeScale i.MXL cpu (ARMv4T)
 */
{
    .match_flags    =   USB_DEVICE_ID_MATCH_INT_INFO
         | USB_DEVICE_ID_MATCH_DEVICE,
    .idVendor               = 0x07B4,
    .idProduct              = 0x0F02,	/* R-1000 */
    ZAURUS_MASTER_INTERFACE,
    .driver_info		= 0,
},

/* LG Electronics VL600 wants additional headers on every frame */
{
    USB_DEVICE_AND_INTERFACE_INFO(0x1004, 0x61aa, USB_CLASS_COMM,
            USB_CDC_SUBCLASS_ETHERNET, USB_CDC_PROTO_NONE),
    .driver_info = (unsigned long)&wwan_info,
},
/*
 * WHITELIST!!!
 *
 * CDC Ether uses two interfaces, not necessarily consecutive.
 * We match the main interface, ignoring the optional device
 * class so we could handle devices that aren't exclusively
 * CDC ether.
 *
 * NOTE:  this match must come AFTER entries blacklisting devices
 * because of bugs/quirks in a given product (like Zaurus, above).
 */
{
    USB_INTERFACE_INFO(USB_CLASS_COMM, USB_CDC_SUBCLASS_ETHERNET,
            USB_CDC_PROTO_NONE),
    .driver_info = (unsigned long) &cdc_info,
}, {
    USB_INTERFACE_INFO(USB_CLASS_COMM, USB_CDC_SUBCLASS_MDLM,
            USB_CDC_PROTO_NONE),
    .driver_info = (unsigned long)&wwan_info,

}, {
    /* Various Huawei modems with a network port like the UMG1831 */
    .match_flags    =   USB_DEVICE_ID_MATCH_VENDOR
         | USB_DEVICE_ID_MATCH_INT_INFO,
    .idVendor               = HUAWEI_VENDOR_ID,
    .bInterfaceClass	= USB_CLASS_COMM,
    .bInterfaceSubClass	= USB_CDC_SUBCLASS_ETHERNET,
    .bInterfaceProtocol	= 255,
    .driver_info = (unsigned long)&wwan_info,
},
    { },		// END
};

static struct usb_driver ecm_driver = {
    .name =        "cdc-ether",
    .id_table =    products,
    .probe =    usbnet_probe,
    .disconnect =    usbnet_disconnect,
    /* .suspend =    usbnet_suspend, */
    /* .resume =    usbnet_resume, */
};

int usb_ecm_init(void)
{
    int retval;

    rt_kprintf("usb ecm init go...\n");
    /* register the driver, return usb_register return code if error */
    retval = usb_register_driver(&ecm_driver);
    if (retval == 0)
    {
        rt_kprintf("usb ecm registered done..\n");
    }
    return retval;
}
#endif
