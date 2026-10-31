#include <usb.h>
#include <rtdef.h>
#include <rtthread.h>
#include <rtdevice.h>
#include <rthw.h>
#include "usb_errno.h"
#include <dma_mem.h>
#include <interrupt.h>
#include <sys/types.h>
#include <cdc.h>
#include <usbnet.h>
#include "rndis_host.h"
#include <mii.h>

#define DRIVER_VERSION "11-Dec-2019"

struct rndis_rx_fixup_info
{
    struct pbuf *rndis_pbuf;
    struct rndis_data_hdr header;
    rt_uint32_t pbuf_offset;
    rt_uint32_t remaining;
    rt_uint32_t split_head;
};

/*
 * RNDIS indicate messages.
 */
static void rndis_msg_indicate(struct usbnet *dev, struct rndis_indicate *msg,
                int buflen)
{
    if (dev->driver_info->indication)
    {
        dev->driver_info->indication(dev, msg, buflen);
    } else
    {
        __u32 status = le32_to_cpu(msg->status);

        switch (status)
        {
        case RNDIS_STATUS_MEDIA_CONNECT:
            rt_kprintf("rndis media connect\n");
            break;
        case RNDIS_STATUS_MEDIA_DISCONNECT:
            rt_kprintf("rndis media disconnect\n");
            break;
        default:
            rt_kprintf("rndis indication: 0x%08x\n", status);
        }
    }
}

/*
 * RPC done RNDIS-style.  Caller guarantees:
 * - message is properly byteswapped
 * - there's no other request pending
 * - buf can hold up to 1KB response (required by RNDIS spec)
 * On return, the first few entries are already byteswapped.
 *
 * Call context is likely probe(), before interface name is known,
 * which is why we won't try to use it in the diagnostics.
 */
int rndis_command(struct usbnet *dev, struct rndis_msg_hdr *buf, int buflen)
{
    struct cdc_state    *info = (void *) &dev->data;
    struct usb_cdc_notification notification;
    int            master_ifnum;
    int            retval;
    int            partial;
    unsigned int count;
    __u32            xid = 0, msg_len, request_id, msg_type, rsp,
                status;

    /* REVISIT when this gets called from contexts other than probe() or
     * disconnect(): either serialize, or dispatch responses on xid
     */

    msg_type = le32_to_cpu(buf->msg_type);

    /* Issue the request; xid is unique, don't bother byteswapping it */
    if (msg_type != RNDIS_MSG_HALT && msg_type != RNDIS_MSG_RESET)
    {
        xid = dev->xid++;
        if (!xid)
            xid = dev->xid++;
        buf->request_id = xid;
    }
    master_ifnum = info->control->cur_altsetting->desc.bInterfaceNumber;
    retval = usb_control_msg(dev->udev,
        usb_sndctrlpipe(dev->udev, 0),
        USB_CDC_SEND_ENCAPSULATED_COMMAND,
        USB_TYPE_CLASS | USB_RECIP_INTERFACE,
        0, master_ifnum,
        buf, le32_to_cpu(buf->msg_len),
        RNDIS_CONTROL_TIMEOUT_MS);
    if (retval < 0 || xid == 0)
        return retval;
/**
 * Some devices don't respond on the control channel until
 * polled on the status channel, so do that first.
 */
    if (dev->driver_info->data & RNDIS_DRIVER_DATA_POLL_STATUS)
    {
        retval = usb_interrupt_msg(
            dev->udev,
            usb_rcvintpipe(dev->udev,
                       dev->status->desc.bEndpointAddress),
            &notification, sizeof(notification), &partial,
            RNDIS_CONTROL_TIMEOUT_MS);
        if (retval < 0)
            return retval;
    }

    /* Poll the control channel; the request probably completed immediately */
    rsp = le32_to_cpu(buf->msg_type) | RNDIS_MSG_COMPLETION;
    for (count = 0; count < 10; count++)
    {
        memset(buf, 0, CONTROL_BUFFER_SIZE);
        retval = usb_control_msg(dev->udev,
            usb_rcvctrlpipe(dev->udev, 0),
            USB_CDC_GET_ENCAPSULATED_RESPONSE,
            USB_DIR_IN | USB_TYPE_CLASS | USB_RECIP_INTERFACE,
            0, master_ifnum,
            buf, buflen,
            RNDIS_CONTROL_TIMEOUT_MS);
        if (retval >= 8)
        {
            msg_type = le32_to_cpu(buf->msg_type);
            msg_len = le32_to_cpu(buf->msg_len);
            status = le32_to_cpu(buf->status);
            request_id = (__u32) buf->request_id;
            if (msg_type == rsp)
            {
                if (request_id == xid)
                {
                    if (rsp == RNDIS_MSG_RESET_C)
                        return 0;
                    if (RNDIS_STATUS_SUCCESS ==
                            status)
                        return 0;
                    rt_kprintf("rndis reply status %08x\n",
                        status);
                    return -EL3RST;
                }
                rt_kprintf("rndis reply id %d expected %d\n",
                    request_id, xid);
                /* then likely retry */
            } else
                switch (msg_type)
                {
                case RNDIS_MSG_INDICATE: /* fault/event */
                    rndis_msg_indicate(dev, (void *)buf, buflen);
                    break;
                case RNDIS_MSG_KEEPALIVE: { /* ping */
                    struct rndis_keepalive_c *msg = (void *)buf;

                    msg->msg_type = cpu_to_le32(RNDIS_MSG_KEEPALIVE_C);
                    msg->msg_len = cpu_to_le32(sizeof(*msg));
                    msg->status = cpu_to_le32(RNDIS_STATUS_SUCCESS);
                    retval = usb_control_msg(dev->udev,
                        usb_sndctrlpipe(dev->udev, 0),
                        USB_CDC_SEND_ENCAPSULATED_COMMAND,
                        USB_TYPE_CLASS | USB_RECIP_INTERFACE,
                        0, master_ifnum,
                        msg, sizeof(*msg),
                        RNDIS_CONTROL_TIMEOUT_MS);
                    if (retval < 0)
                        rt_kprintf("rndis keepalive err %d\n",
                            retval);
                    }
                    break;
                default:
                    rt_kprintf("unexpected rndis msg %08x len %d\n",
                        le32_to_cpu(buf->msg_type), msg_len);
                }
        } else
        {
            /* device probably issued a protocol stall; ignore */
            dbg("rndis response error, code %d\n", retval);
        }
        rt_thread_delay(2);
    }
    rt_kprintf("rndis response timeout\n");
    return -ETIMEDOUT;
}

/*
 * RNDIS notifications from device: command completion; "reverse"
 * keepalives; etc
 */
void rndis_status(struct usbnet *dev, struct urb *urb)
{
    /* FIXME for keepalives, respond immediately (asynchronously) */
    /* if not an RNDIS status, do like cdc_status(dev,urb) does */
    if (urb->status == 0)
        net_device_linkchange(&dev->rtt_eth_interface, 1);


}
/*
 * rndis_query:
 *
 * Performs a query for @oid along with 0 or more bytes of payload as
 * specified by @in_len. If @reply_len is not set to -1 then the reply
 * length is checked against this value, resulting in an error if it
 * doesn't match.
 *
 * NOTE: Adding a payload exactly or greater than the size of the expected
 * response payload is an evident requirement MSFT added for ActiveSync.
 *
 * The only exception is for OIDs that return a variably sized response,
 * in which case no payload should be added.  This undocumented (and
 * nonsensical!) issue was found by sniffing protocol requests from the
 * ActiveSync 4.1 Windows driver.
 */
static int rndis_query(struct usbnet *dev, struct usb_interface *intf,
        void *buf, __u32 oid, __u32 in_len,
        void **reply, int *reply_len)
{
    int retval;
    union {
        void            *buf;
        struct rndis_msg_hdr    *header;
        struct rndis_query    *get;
        struct rndis_query_c    *get_c;
    } u;
    __u32 off, len;

    u.buf = buf;

    memset(u.get, 0, sizeof(*u.get) + in_len);
    u.get->msg_type = RNDIS_MSG_QUERY;
    u.get->msg_len = cpu_to_le32(sizeof(*u.get) + in_len);
    u.get->oid = oid;
    u.get->len = cpu_to_le32(in_len);
    u.get->offset = cpu_to_le32(20);

    retval = rndis_command(dev, u.header, CONTROL_BUFFER_SIZE);
    if (retval < 0)
    {
        rt_kprintf("RNDIS_MSG_QUERY(0x%08x) failed, %d\n",
                oid, retval);
        return retval;
    }

    off = le32_to_cpu(u.get_c->offset);
    len = le32_to_cpu(u.get_c->len);
    if ((8 + off + len) > CONTROL_BUFFER_SIZE)
        goto response_error;

    if (*reply_len != -1 && len != *reply_len)
        goto response_error;

    *reply = (unsigned char *) &u.get_c->request_id + off;
    *reply_len = len;

    return retval;

response_error:
    rt_kprintf("RNDIS_MSG_QUERY(0x%08x) "
            "invalid response - off %d len %d\n",
        oid, off, len);
    return -EDOM;
}

/*
 * DATA -- host must not write zlps
 */
unsigned int rndis_rx_fixup(struct usbnet *dev, unsigned char *rx_buf,
                unsigned int rx_buf_size, struct usbnet_rx_node *rx_node)
{
    /* dump_stack(); */
    /* peripheral may have batched packets to us... */
    unsigned char *temp_buf = RT_NULL;
    int offset = 0;
    struct rndis_rx_fixup_info *rx;
    struct rndis_data_hdr *hdr;

    temp_buf = rx_buf;
    rx = dev->driver_info->driver_priv;
    /* sizeof(rndis_data_hdr) = 44 */

    while ((offset + 4 <= rx_buf_size))
    {
        rt_uint16_t copy_length;

        if (!rx->remaining)
        {
            hdr = (struct rndis_data_hdr *)(temp_buf + offset);
            if (offset + 44 >= rx_buf_size && hdr->msg_type == RNDIS_MSG_PACKET)
            {
                copy_length = rx_buf_size - offset;
                rt_memcpy(&rx->header, (temp_buf + offset), copy_length);
                rx->split_head = copy_length;
                offset += copy_length;
                break;
            }
            if (rx->split_head)
            {
                if (rx->split_head < 44)
                    rt_memcpy(((rt_uint8_t)(&rx->header)) + rx->split_head, (temp_buf + offset), 44 - rx->split_head);
                rx->split_head = 0;
                offset += 44 - rx->split_head;
                hdr = &rx->header;
            } else
            {
                offset += hdr->data_offset + 8;
            }
            if (hdr->msg_type != RNDIS_MSG_PACKET ||
                    rx_buf_size < hdr->msg_len ||
                    (hdr->data_offset + hdr->data_len + 8) > hdr->msg_len) {
                rt_kprintf("bad rndis message %d/%d/%d/%d, len %d\n",
                    hdr->msg_type,
                    hdr->msg_len, hdr->data_offset, hdr->data_len, rx_buf_size);
                rt_kprintf("rx_buf_size %d/ offset %d\n",
                    rx_buf_size, offset);
                return 0;
            }
            rx->rndis_pbuf = pbuf_alloc(PBUF_RAW, USBNET_PBUF_MALLOC_LEN, PBUF_RAM);
            rx->remaining = hdr->data_len;
        }
        if (rx->remaining > rx_buf_size - offset)
        {
            copy_length = rx_buf_size - offset;
            rx->remaining -= copy_length;
        }
        else
        {
            copy_length = rx->remaining;
            rx->remaining = 0;
        }

        if (rx->rndis_pbuf)
        {
            rt_memcpy((rx->rndis_pbuf->payload + rx->pbuf_offset), temp_buf + offset, copy_length);
            rx->pbuf_offset += copy_length;
            if (rx->remaining == 0)
            {
                rx->rndis_pbuf->len = rx->rndis_pbuf->tot_len = rx->pbuf_offset;
                usbnet_put_one_pbuf(dev, rx->rndis_pbuf, &rx_node->list_done_pbuf);
                rx->pbuf_offset = 0;
                rx->rndis_pbuf = RT_NULL;
            }
            offset += copy_length;
        }
    }

    if (rx_buf_size != offset)
    {
        rt_kprintf("rndis_rx_fixup() Bad SKB Length %d, %d\n", rx_buf_size, offset);
        return -1;
    }
    rx_node->rx_state_mac = GET_ACTIVE_NODE_PBUFF;
    return 0;
}

unsigned char *rndis_tx_fixup(struct usbnet *dev,
        struct pbuf *p, unsigned char *tx_buf, unsigned int tx_buf_size, unsigned int *status, unsigned int *size)
{
    struct rndis_data_hdr    *hdr;
    unsigned char *temp_buf;
    struct pbuf *temp_pbuf;
    /* check tx buff size.. */
    *status = 0;
    if ((p->tot_len + 44) > tx_buf_size)
    {
        rt_kprintf("tx need 44 byte space to tell the rndis how many bytes to send...\n");
        *status = -1;
        goto out;
    }

    hdr = rt_malloc(sizeof(*hdr));
    memset(hdr, 0, sizeof(*hdr));
    hdr->msg_type = RNDIS_MSG_PACKET;
    hdr->msg_len = p->tot_len + 44;
    hdr->data_offset = sizeof(*hdr) - 8;
    hdr->data_len = p->tot_len;
    rt_memcpy(tx_buf, hdr, sizeof(*hdr));
    /* cpy data to the align buf.. */
    for (temp_pbuf = p, temp_buf = tx_buf + sizeof(*hdr); temp_pbuf != NULL; temp_pbuf = temp_pbuf->next)
    {
        rt_memcpy(temp_buf, temp_pbuf->payload, temp_pbuf->len);
        temp_buf += temp_pbuf->len;
    }
    *size = p->tot_len + 44;
    /* rt_kprintf("transfer size is %d\n",*size); */
    rt_free(hdr);
out:
    return tx_buf;

}
/* static const struct net_device_ops rndis_netdev_ops = { */
/* .ndo_open        = usbnet_open, */
/* .ndo_stop        = usbnet_stop, */
/* .ndo_start_xmit        = usbnet_start_xmit, */
/* .ndo_tx_timeout        = usbnet_tx_timeout, */
/* .ndo_set_mac_address     = eth_mac_addr, */
/* .ndo_validate_addr    = eth_validate_addr, */
/* }; */

extern int usbnet_generic_cdc_bind(struct usbnet *dev, struct usb_interface *intf);

int
generic_rndis_bind(struct usbnet *dev, struct usb_interface *intf, int flags)
{
    int            retval;
    struct cdc_state    *info = (void *) &dev->data;
    union {
        void            *buf;
        struct rndis_msg_hdr    *header;
        struct rndis_init    *init;
        struct rndis_init_c    *init_c;
        struct rndis_query    *get;
        struct rndis_query_c    *get_c;
        struct rndis_set    *set;
        struct rndis_set_c    *set_c;
        struct rndis_halt    *halt;
    } u;
    __u32            tmp, hard_header_len;
    __u32            phym_unspec, *phym;
    int            reply_len;
    unsigned char        *bp;
    /* we can't rely on i/o from stack working, or stack allocation */
    u.buf = kmalloc(CONTROL_BUFFER_SIZE, GFP_KERNEL);
    if (!u.buf)
        return -ENOMEM;
    retval = usbnet_generic_cdc_bind(dev, intf);
    if (retval < 0)
        goto fail;

    u.init->msg_type = RNDIS_MSG_INIT;
    u.init->msg_len = cpu_to_le32(sizeof(*u.init));
    u.init->major_version = cpu_to_le32(1);
    u.init->minor_version = cpu_to_le32(0);

    if (!dev->driver_info->driver_priv)
    {
        dev->driver_info->driver_priv = rt_malloc(sizeof(struct rndis_rx_fixup_info));
        if (!dev->driver_info->driver_priv)
        {
            retval = -ENOMEM;
            rt_kprintf("%s : %d malloc failed..\n", __func__, __LINE__);
            goto fail;
        }
        rt_memset(dev->driver_info->driver_priv, 0, sizeof(struct rndis_rx_fixup_info));
    }
    /* max transfer (in spec) is 0x4000 at full speed, but for
     * TX we'll stick to one Ethernet packet plus RNDIS framing.
     * For RX we handle drivers that zero-pad to end-of-packet.
     * Don't let userspace change these settings.
     *
     * NOTE: there still seems to be wierdness here, as if we need
     * to do some more things to make sure WinCE targets accept this.
     * They default to jumbograms of 8KB or 16KB, which is absurd
     * for such low data rates and which is also more than Linux
     * can usually expect to allocate for SKB data...
     */
    hard_header_len = 14 + sizeof(struct rndis_data_hdr);  /* TODO:net->hard_header_len = 14 */
    dev->hard_mtu = /* net->netif->mtu +  */hard_header_len;
    dev->maxpacket = usb_maxpacket(dev->udev, dev->out, 1);
    if (dev->maxpacket == 0)
    {
        rt_kprintf("%s dev->maxpacket can't be 0\n", __func__);
        retval = -EINVAL;
        goto fail_and_release;
    }

    dev->rx_urb_size = dev->hard_mtu + (dev->maxpacket + 1);
    dev->rx_urb_size &= ~(dev->maxpacket - 1);
    u.init->max_transfer_size = cpu_to_le32(dev->rx_urb_size);
    /* TODO: */
    /* net->netdev_ops = &rndis_netdev_ops; */
    retval = rndis_command(dev, u.header, CONTROL_BUFFER_SIZE);
    if (retval < 0)
    {
        /* it might not even be an RNDIS device!! */
        rt_kprintf("RNDIS init failed, %d~\n", retval);
        goto fail_and_release;
    }
    tmp = le32_to_cpu(u.init_c->max_transfer_size);
    if (tmp < dev->hard_mtu)
    {
        if (tmp <= hard_header_len)
        {
            rt_kprintf("dev can't take %u byte packets(max %u)\n",
                dev->hard_mtu, tmp);
            retval = -EINVAL;
            goto halt_fail_and_release;
        }
        rt_kprintf("dev can't take %u byte packets(max %u),adjusting MTU to %u\n",
             dev->hard_mtu, tmp, tmp - hard_header_len);
        dev->hard_mtu = tmp;
        /* net->netif->mtu = dev->hard_mtu - hard_header_len; */
    }
    /* REVISIT:  peripheral "alignment" request is ignored ... */
        rt_kprintf("hard mtu %u(%u from dev), rx buflen %u, align %d\n",
            dev->hard_mtu, tmp, dev->rx_urb_size,
            1 << le32_to_cpu(u.init_c->packet_alignment));

/**
 * module has some device initialization code needs to be done right
 * after RNDIS_INIT
 */
    if (dev->driver_info->early_init &&
            dev->driver_info->early_init(dev) != 0)
        goto halt_fail_and_release;

    /* Check physical medium */
    phym = NULL;
    reply_len = sizeof(*phym);
    retval = rndis_query(dev, intf, u.buf, OID_GEN_PHYSICAL_MEDIUM,
            0, (void **) &phym, &reply_len);
    if (retval != 0 || !phym)
    {
        /* OID is optional so don't fail here. */
        phym_unspec = RNDIS_PHYSICAL_MEDIUM_UNSPECIFIED;
        phym = &phym_unspec;
    }
    if ((flags & FLAG_RNDIS_PHYM_WIRELESS) &&
            *phym != RNDIS_PHYSICAL_MEDIUM_WIRELESS_LAN) {
        rt_kprintf("driver requires wireless physical medium, but device is not\n");
        retval = -ENODEV;
        goto halt_fail_and_release;
    }
    if ((flags & FLAG_RNDIS_PHYM_NOT_WIRELESS) &&
            *phym == RNDIS_PHYSICAL_MEDIUM_WIRELESS_LAN) {
        rt_kprintf("driver requires non-wireless physical medium, but device is wireless.\n");
        retval = -ENODEV;
        goto halt_fail_and_release;
    }

    /* Get designated host ethernet address */
    reply_len = ETH_ALEN;
    retval = rndis_query(dev, intf, u.buf, OID_802_3_PERMANENT_ADDRESS,
            48, (void **) &bp, &reply_len);
    if (retval < 0)
    {
        rt_kprintf("rndis get ethaddr, %d\n", retval);
        goto halt_fail_and_release;
    }
    /* memcpy(net->dev_addr, bp, ETH_ALEN); */
    /* memcpy(net->perm_addr, bp, ETH_ALEN);   //todo */
    memcpy(dev->usbnet_mac_add, bp, ETH_ALEN);

    /* set a nonzero filter to enable data transfers */
    memset(u.set, 0, sizeof(*u.set));
    u.set->msg_type = RNDIS_MSG_SET;
    u.set->msg_len = cpu_to_le32(4 + sizeof(*u.set));
    u.set->oid = OID_GEN_CURRENT_PACKET_FILTER;
    u.set->len = cpu_to_le32(4);
    u.set->offset = cpu_to_le32((sizeof(*u.set)) - 8);
    *(__u32 *)(u.buf + sizeof(*u.set)) = RNDIS_DEFAULT_FILTER;
    retval = rndis_command(dev, u.header, CONTROL_BUFFER_SIZE);
    if (retval < 0)
    {
        rt_kprintf("rndis set packet filter, %d\n", retval);
        goto halt_fail_and_release;
    }

    retval = 0;

    kfree(u.buf);
    return retval;

halt_fail_and_release:
    memset(u.halt, 0, sizeof(*u.halt));
    u.halt->msg_type = RNDIS_MSG_HALT;
    u.halt->msg_len = cpu_to_le32(sizeof(*u.halt));
    (void) rndis_command(dev, (void *)u.halt, CONTROL_BUFFER_SIZE);
fail_and_release:
    usb_set_intfdata(info->data, NULL);

    /* usb_driver_release_interface(driver_of(intf), info->data);  //todo */
    info->data = NULL;
fail:
    kfree(u.buf);
    return retval;
}

static int rndis_bind(struct usbnet *dev, struct usb_interface *intf)
{
    return generic_rndis_bind(dev, intf, FLAG_RNDIS_PHYM_NOT_WIRELESS);
}

static const struct driver_info    rndis_info = {
    .description =    "RNDIS device",
    .flags =    FLAG_ETHER | FLAG_POINTTOPOINT | FLAG_FRAMING_RN | FLAG_NO_SETINT,
    .bind =        rndis_bind,
    /* .unbind =    rndis_unbind, */
    .status =    rndis_status,
    .rx_fixup =    rndis_rx_fixup,
    .tx_fixup =    rndis_tx_fixup,
};

static const struct driver_info    rndis_poll_status_info = {
    .description =    "RNDIS device(poll status before control)",
    .flags =    FLAG_ETHER | FLAG_POINTTOPOINT | FLAG_FRAMING_RN | FLAG_NO_SETINT,
    .data =        RNDIS_DRIVER_DATA_POLL_STATUS,
    .bind =        rndis_bind,
    /* .unbind =    rndis_unbind, */
    .status =    rndis_status,
    .rx_fixup =    rndis_rx_fixup,
    .tx_fixup =    rndis_tx_fixup,
};

static const struct usb_device_id    products[] = {
{
    /* 2Wire HomePortal 1000SW */
    USB_DEVICE_AND_INTERFACE_INFO(0x1630, 0x0042,
                      USB_CLASS_COMM, 2 /* ACM */, 0x0ff),
    .driver_info = (unsigned long) &rndis_poll_status_info,
}, {
    /* RNDIS is MSFT's un-official variant of CDC ACM */
    USB_INTERFACE_INFO(USB_CLASS_COMM, 2 /* ACM */, 0x0ff),
    .driver_info = (unsigned long) &rndis_info,
}, {
    /* "ActiveSync" is an undocumented variant of RNDIS, used in WM5 */
    USB_INTERFACE_INFO(USB_CLASS_MISC, 1, 1),
    .driver_info = (unsigned long) &rndis_poll_status_info,
}, {
    /* RNDIS for tethering */
    USB_INTERFACE_INFO(USB_CLASS_WIRELESS_CONTROLLER, 1, 3),
    .driver_info = (unsigned long) &rndis_info,
},
    { },        /* END */
};

static struct usb_driver rndis_driver = {
    .name =        "rndis_host",
    .id_table =    products,
    .probe =    usbnet_probe,
    .disconnect =    usbnet_disconnect,
    /* .suspend =    usbnet_suspend, */
    /* .resume =    usbnet_resume, */
};

int usb_rndis_init(void)
{
    int retval;

    rt_kprintf("usb rndis init go...\n");
    /* register the driver, return usb_register return code if error */
    retval = usb_register_driver(&rndis_driver);
    if (retval == 0)
    {
        rt_kprintf("usb rndis registered done..\n");
    }
    return retval;
}
