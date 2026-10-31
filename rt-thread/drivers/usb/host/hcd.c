

#include "hcd.h"
#include <rtdef.h>
#include <rtthread.h>
#include <rtdevice.h>
#include <rthw.h>
#include "usb_errno.h"
#include <dma_mem.h>
#include <interrupt.h>
#include <sys/types.h>

/*-------------------------------------------------------------------------*/

/*
 * USB Host Controller Driver framework
 *
 * Plugs into usbcore (usb_bus) and lets HCDs share code, minimizing
 * HCD-specific behaviors/bugs.
 *
 * This does error checks, tracks devices and urbs, and delegates to a
 * "hc_driver" only for code (and data) that really needs to know about
 * hardware differences.  That includes root hub registers, i/o queues,
 * and so on ... but as little else as possible.
 *
 * Shared code includes most of the "root hub" code (these are emulated,
 * though each HC's hardware works differently) and PCI glue, plus request
 * tracking overhead.  The HCD code should only block on spinlocks or on
 * hardware handshaking; blocking on software events (such as other kernel
 * threads releasing resources, or completing actions) is all generic.
 *
 * Happens the USB 2.0 spec says this would be invisible inside the "USBD",
 * and includes mostly a "HCDI" (HCD Interface) along with some APIs used
 * only by the hub driver ... and that neither should be seen or used by
 * usb client device drivers.
 *
 * Contributors of ideas or unattributed patches include: David Brownell,
 * Roman Weissgaerber, Rory Bolt, Greg Kroah-Hartman, ...
 *
 * HISTORY:
 * 2002-02-21    Pull in most of the usb_bus support from usb.c; some
 *        associated cleanup.  "usb_hcd" still != "usb_bus".
 * 2001-12-12    Initial patch version for Linux 2.5.1 kernel.
 */

/*-------------------------------------------------------------------------*/

/* Keep track of which host controller drivers are loaded */

/* used when allocating bus numbers */
#define USB_MAXBUS        64
struct usb_busmap
{
    unsigned long busmap[USB_MAXBUS / (8*sizeof(unsigned long))];
};
static struct usb_busmap busmap;

rt_list_t usb_bus_list;

/* wait queue for synchronous unlinks */
/* DECLARE_WAIT_QUEUE_HEAD(usb_kill_urb_queue); */

static inline int is_root_hub(struct usb_device *udev)
{
    return (udev->parent == RT_NULL);
}

/*-------------------------------------------------------------------------*/

/*
 * Sharable chunks of root hub code.
 */

/*-------------------------------------------------------------------------*/


/* usb 3.0 root hub device descriptor */
static const rt_uint8_t usb3_rh_dev_descriptor[18] = {
    0x12,       /*  __u8  bLength; */
    0x01,       /*  __u8  bDescriptorType; Device */
    0x00, 0x03, /*  __le16 bcdUSB; v3.0 */

    0x09,        /*  __u8  bDeviceClass; HUB_CLASSCODE */
    0x00,        /*  __u8  bDeviceSubClass; */
    0x03,       /*  __u8  bDeviceProtocol; USB 3.0 hub */
    0x09,       /*  __u8  bMaxPacketSize0; 2^9 = 512 Bytes */

    0x6b, 0x1d, /*  __le16 idVendor; Linux Foundation */
    0x03, 0x00, /*  __le16 idProduct; device 0x0003 */
    0xee, 0xee, /*  __le16 bcdDevice */

    0x03,       /*  __u8  iManufacturer; */
    0x02,       /*  __u8  iProduct; */
    0x01,       /*  __u8  iSerialNumber; */
    0x01        /*  __u8  bNumConfigurations; */
};

/* usb 2.0 root hub device descriptor */
static const rt_uint8_t usb2_rh_dev_descriptor[18] = {
    0x12,       /*  __u8  bLength; */
    0x01,       /*  __u8  bDescriptorType; Device */
    0x00, 0x02, /*  __le16 bcdUSB; v2.0 */

    0x09,        /*  __u8  bDeviceClass; HUB_CLASSCODE */
    0x00,        /*  __u8  bDeviceSubClass; */
    0x00,       /*  __u8  bDeviceProtocol; [ usb 2.0 no TT ] */
    0x40,       /*  __u8  bMaxPacketSize0; 64 Bytes */

    0x6b, 0x1d, /*  __le16 idVendor; Linux Foundation */
    0x02, 0x00, /*  __le16 idProduct; device 0x0002 */
    0xee, 0xee, /*  __le16 bcdDevice */

    0x03,       /*  __u8  iManufacturer; */
    0x02,       /*  __u8  iProduct; */
    0x01,       /*  __u8  iSerialNumber; */
    0x01        /*  __u8  bNumConfigurations; */
};

/* no usb 2.0 root hub "device qualifier" descriptor: one speed only */

/* usb 1.1 root hub device descriptor */
static const rt_uint8_t usb11_rh_dev_descriptor[18] = {
    0x12,       /*  __u8  bLength; */
    0x01,       /*  __u8  bDescriptorType; Device */
    0x10, 0x01, /*  __le16 bcdUSB; v1.1 */

    0x09,        /*  __u8  bDeviceClass; HUB_CLASSCODE */
    0x00,        /*  __u8  bDeviceSubClass; */
    0x00,       /*  __u8  bDeviceProtocol; [ low/full speeds only ] */
    0x40,       /*  __u8  bMaxPacketSize0; 64 Bytes */

    0x6b, 0x1d, /*  __le16 idVendor; Linux Foundation */
    0x01, 0x00, /*  __le16 idProduct; device 0x0001 */
    0xee, 0xee, /*  __le16 bcdDevice */

    0x03,       /*  __u8  iManufacturer; */
    0x02,       /*  __u8  iProduct; */
    0x01,       /*  __u8  iSerialNumber; */
    0x01        /*  __u8  bNumConfigurations; */
};


/*-------------------------------------------------------------------------*/

/* Configuration descriptors for our root hubs */

static const rt_uint8_t fs_rh_config_descriptor[] = {

    /* one configuration */
    0x09,       /*  __u8  bLength; */
    0x02,       /*  __u8  bDescriptorType; Configuration */
    0x19, 0x00, /*  __le16 wTotalLength; */
    0x01,       /*  __u8  bNumInterfaces; (1) */
    0x01,       /*  __u8  bConfigurationValue; */
    0x00,       /*  __u8  iConfiguration; */
    0xc0,       /*  __u8  bmAttributes;
                Bit 7: must be set,
                     6: Self-powered,
                     5: Remote wakeup,
                     4..0: resvd */
    0x00,       /*  __u8  MaxPower; */

    /* USB 1.1:
     * USB 2.0, single TT organization (mandatory):
     * one interface, protocol 0
     *
     * USB 2.0, multiple TT organization (optional):
     * two interfaces, protocols 1 (like single TT)
     * and 2 (multiple TT mode) ... config is
     * sometimes settable
     * NOT IMPLEMENTED
     */

    /* one interface */
    0x09,       /*  __u8  if_bLength; */
    0x04,       /*  __u8  if_bDescriptorType; Interface */
    0x00,       /*  __u8  if_bInterfaceNumber; */
    0x00,       /*  __u8  if_bAlternateSetting; */
    0x01,       /*  __u8  if_bNumEndpoints; */
    0x09,       /*  __u8  if_bInterfaceClass; HUB_CLASSCODE */
    0x00,       /*  __u8  if_bInterfaceSubClass; */
    0x00,       /*  __u8  if_bInterfaceProtocol; [usb1.1 or single tt] */
    0x00,       /*  __u8  if_iInterface; */

    /* one endpoint (status change endpoint) */
    0x07,       /*  __u8  ep_bLength; */
    0x05,       /*  __u8  ep_bDescriptorType; Endpoint */
    0x81,       /*  __u8  ep_bEndpointAddress; IN Endpoint 1 */
    0x03,       /*  __u8  ep_bmAttributes; Interrupt */
    0x02, 0x00, /*  __le16 ep_wMaxPacketSize; 1 + (MAX_ROOT_PORTS / 8) */
    0xff        /*  __u8  ep_bInterval; (255ms -- usb 2.0 spec) */
};

static const rt_uint8_t hs_rh_config_descriptor[] = {

    /* one configuration */
    0x09,       /*  __u8  bLength; */
    0x02,       /*  __u8  bDescriptorType; Configuration */
    0x19, 0x00, /*  __le16 wTotalLength; */
    0x01,       /*  __u8  bNumInterfaces; (1) */
    0x01,       /*  __u8  bConfigurationValue; */
    0x00,       /*  __u8  iConfiguration; */
    0xc0,       /*  __u8  bmAttributes;
                 Bit 7: must be set,
                     6: Self-powered,
                     5: Remote wakeup,
                     4..0: resvd */
    0x00,       /*  __u8  MaxPower; */

    /* USB 1.1:
     * USB 2.0, single TT organization (mandatory):
     * one interface, protocol 0
     *
     * USB 2.0, multiple TT organization (optional):
     * two interfaces, protocols 1 (like single TT)
     * and 2 (multiple TT mode) ... config is
     * sometimes settable
     * NOT IMPLEMENTED
     */

    /* one interface */
    0x09,       /*  __u8  if_bLength; */
    0x04,       /*  __u8  if_bDescriptorType; Interface */
    0x00,       /*  __u8  if_bInterfaceNumber; */
    0x00,       /*  __u8  if_bAlternateSetting; */
    0x01,       /*  __u8  if_bNumEndpoints; */
    0x09,       /*  __u8  if_bInterfaceClass; HUB_CLASSCODE */
    0x00,       /*  __u8  if_bInterfaceSubClass; */
    0x00,       /*  __u8  if_bInterfaceProtocol; [usb1.1 or single tt] */
    0x00,       /*  __u8  if_iInterface; */

    /* one endpoint (status change endpoint) */
    0x07,       /*  __u8  ep_bLength; */
    0x05,       /*  __u8  ep_bDescriptorType; Endpoint */
    0x81,       /*  __u8  ep_bEndpointAddress; IN Endpoint 1 */
    0x03,       /*  __u8  ep_bmAttributes; Interrupt */
            /* __le16 ep_wMaxPacketSize; 1 + (MAX_ROOT_PORTS / 8)
             * see hub.c:hub_configure() for details. */
    (USB_MAXCHILDREN + 1 + 7) / 8, 0x00,
    0x0c        /*  __u8  ep_bInterval; (256ms -- usb 2.0 spec) */
};

static const rt_uint8_t ss_rh_config_descriptor[] = {
    /* one configuration */
    0x09,       /*  __u8  bLength; */
    0x02,       /*  __u8  bDescriptorType; Configuration */
    0x1f, 0x00, /*  __le16 wTotalLength; */
    0x01,       /*  __u8  bNumInterfaces; (1) */
    0x01,       /*  __u8  bConfigurationValue; */
    0x00,       /*  __u8  iConfiguration; */
    0xc0,       /*  __u8  bmAttributes;
                 Bit 7: must be set,
                     6: Self-powered,
                     5: Remote wakeup,
                     4..0: resvd */
    0x00,       /*  __u8  MaxPower; */
    /* one interface */
    0x09,       /*  __u8  if_bLength; */
    0x04,       /*  __u8  if_bDescriptorType; Interface */
    0x00,       /*  __u8  if_bInterfaceNumber; */
    0x00,       /*  __u8  if_bAlternateSetting; */
    0x01,       /*  __u8  if_bNumEndpoints; */
    0x09,       /*  __u8  if_bInterfaceClass; HUB_CLASSCODE */
    0x00,       /*  __u8  if_bInterfaceSubClass; */
    0x00,       /*  __u8  if_bInterfaceProtocol; [usb1.1 or single tt] */
    0x00,       /*  __u8  if_iInterface; */

    /* one endpoint (status change endpoint) */
    0x07,       /*  __u8  ep_bLength; */
    0x05,       /*  __u8  ep_bDescriptorType; Endpoint */
    0x81,       /*  __u8  ep_bEndpointAddress; IN Endpoint 1 */
    0x03,       /*  __u8  ep_bmAttributes; Interrupt */
            /* __le16 ep_wMaxPacketSize; 1 + (MAX_ROOT_PORTS / 8)
             * see hub.c:hub_configure() for details. */
    (USB_MAXCHILDREN + 1 + 7) / 8, 0x00,
    0x0c,       /*  __u8  ep_bInterval; (256ms -- usb 2.0 spec) */

    /* one SuperSpeed endpoint companion descriptor */
    0x06,        /* __u8 ss_bLength */
    0x30,        /* __u8 ss_bDescriptorType; SuperSpeed EP Companion */
    0x00,        /* __u8 ss_bMaxBurst; allows 1 TX between ACKs */
    0x00,        /* __u8 ss_bmAttributes; 1 packet per service interval */
    0x02, 0x00   /* __le16 ss_wBytesPerInterval; 15 bits for max 15 ports */
};

/*-------------------------------------------------------------------------*/

/**
 * ascii2desc() - Helper routine for producing UTF-16LE string descriptors
 * @s: Null-terminated ASCII (actually ISO-8859-1) string
 * @buf: Buffer for USB string descriptor (header + UTF-16LE)
 * @len: Length (in bytes; may be odd) of descriptor buffer.
 *
 * The return value is the number of bytes filled in: 2 + 2*strlen(s) or
 * buflen, whichever is less.
 *
 * USB String descriptors can contain at most 126 characters; input
 * strings longer than that are truncated.
 */
static unsigned
ascii2desc(char const *s, rt_uint8_t *buf, unsigned int len)
{
    unsigned int n, t = 2 + 2*rt_strlen(s);

    if (t > 254)
        t = 254;    /* Longest possible UTF string descriptor */
    if (len > t)
        len = t;

    t += USB_DT_STRING << 8;    /* Now t is first 16 bits to store */

    n = len;
    while (n--)
    {
        *buf++ = t;
        if (!n--)
            break;
        *buf++ = t >> 8;
        t = (unsigned char)*s++;
    }
    return len;
}

/**
 * rh_string() - provides string descriptors for root hub
 * @id: the string ID number (0: langids, 1: serial #, 2: product, 3: vendor)
 * @hcd: the host controller for this root hub
 * @data: buffer for output packet
 * @len: length of the provided buffer
 *
 * Produces either a manufacturer, product or serial number string for the
 * virtual root hub device.
 * Returns the number of bytes filled in: the length of the descriptor or
 * of the provided buffer, whichever is less.
 */
static unsigned
rh_string(int id, struct usb_hcd const *hcd, rt_uint8_t *data, unsigned int len)
{
    char buf[100];
    char const *s;
    static char const langids[4] = {4, USB_DT_STRING, 0x09, 0x04};

    /* language ids */
    switch (id)
    {
    case 0:
        /* Array of LANGID codes (0x0409 is MSFT-speak for "en-us") */
        /* See http://www.usb.org/developers/docs/USB_LANGIDs.pdf */
        if (len > 4)
            len = 4;
        rt_memcpy(data, langids, len);
        return len;
    case 1:
        /* Serial number */
        s = hcd->self.bus_name;
        break;
    case 2:
        /* Product name */
        s = hcd->product_desc;
        break;
    case 3:
        /* Manufacturer */
        rt_snprintf(buf, sizeof(buf), "%s", hcd->driver->description);
        s = buf;
        break;
    default:
        /* Can't happen; caller guarantees it */
        return 0;
    }

    return ascii2desc(s, data, len);
}


/* Root hub control transfers execute synchronously */
static int rh_call_control(struct usb_hcd *hcd, struct urb *urb)
{
    struct usb_ctrlrequest *cmd;
     rt_uint16_t typeReq, wValue, wIndex, wLength;
    rt_uint8_t *ubuf = urb->transfer_buffer;
    rt_uint8_t        tbuf[sizeof(struct usb_hub_descriptor)]
        __attribute__((aligned(4)));
    const rt_uint8_t    *bufp = tbuf;
    unsigned int len = 0;
    int        status;
/* rt_uint8_t        patch_wakeup = 0; */
    rt_uint8_t        patch_protocol = 0;
    rt_base_t level;

/* might_sleep(); */

/* spin_lock_irq(&hcd_root_hub_lock); */
    level = rt_hw_interrupt_disable();
    rt_enter_critical();

    status = usb_hcd_link_urb_to_ep(hcd, urb);
    /* spin_unlock_irq(&hcd_root_hub_lock); */
    rt_hw_interrupt_enable(level);
     rt_exit_critical();
    if (status)
        return status;
    urb->hcpriv = hcd;    /* Indicate it's queued */

    cmd = (struct usb_ctrlrequest *) urb->setup_packet;
    typeReq  = (cmd->bRequestType << 8) | cmd->bRequest;
    wValue   = (cmd->wValue);
    wIndex   = (cmd->wIndex);
    wLength  = (cmd->wLength);

    if (wLength > urb->transfer_buffer_length)
        goto error;

    urb->actual_length = 0;
    switch (typeReq)
    {

    /* DEVICE REQUESTS */

    /* The root hub's remote wakeup enable bit is implemented using
     * driver model wakeup flags.  If this system supports wakeup
     * through USB, userspace may change the default "allow wakeup"
     * policy through sysfs or these calls.
     *
     * Most root hubs support wakeup from downstream devices, for
     * runtime power management (disabling USB clocks and reducing
     * VBUS power usage).  However, not all of them do so; silicon,
     * board, and BIOS bugs here are not uncommon, so these can't
     * be treated quite like external hubs.
     *
     * Likewise, not all root hubs will pass wakeup events upstream,
     * to wake up the whole system.  So don't assume root hub and
     * controller capabilities are identical.
     */

    case DeviceRequest | USB_REQ_GET_STATUS:
        tbuf[0] = (1 << USB_DEVICE_SELF_POWERED);
        tbuf[1] = 0;
        len = 2;
        break;
    case DeviceRequest | USB_REQ_GET_CONFIGURATION:
        tbuf[0] = 1;
        len = 1;
            /* FALLTHROUGH */
    case DeviceOutRequest | USB_REQ_SET_CONFIGURATION:
        break;
    case DeviceRequest | USB_REQ_GET_DESCRIPTOR:
        switch (wValue & 0xff00)
        {
        case USB_DT_DEVICE << 8:
            switch (hcd->speed)
            {
            case HCD_USB3:
                bufp = usb3_rh_dev_descriptor;
                break;
            case HCD_USB2:
                bufp = usb2_rh_dev_descriptor;
                break;
            case HCD_USB11:
                bufp = usb11_rh_dev_descriptor;
                break;
            default:
                goto error;
            }
            len = 18;
            if (hcd->has_tt)
                patch_protocol = 1;
            break;
        case USB_DT_CONFIG << 8:
            switch (hcd->speed)
            {
            case HCD_USB3:
                bufp = ss_rh_config_descriptor;
                len = sizeof(ss_rh_config_descriptor);
                break;
            case HCD_USB2:
                bufp = hs_rh_config_descriptor;
                len = sizeof(hs_rh_config_descriptor);
                break;
            case HCD_USB11:
                bufp = fs_rh_config_descriptor;
                len = sizeof(fs_rh_config_descriptor);
                break;
            default:
                goto error;
            }
            break;
        case USB_DT_STRING << 8:
            if ((wValue & 0xff) < 4)
                urb->actual_length = rh_string(wValue & 0xff,
                        hcd, ubuf, wLength);
            else /* unsupported IDs --> "protocol stall" */
                goto error;
            break;
        default:
            goto error;
        }
        break;
    case DeviceRequest | USB_REQ_GET_INTERFACE:
        tbuf[0] = 0;
        len = 1;
            /* FALLTHROUGH */
    case DeviceOutRequest | USB_REQ_SET_INTERFACE:
        break;
    case DeviceOutRequest | USB_REQ_SET_ADDRESS:
        /* wValue == urb->dev->devaddr */
        RT_DEBUG_LOG(rt_debug_usb, ("root hub device address %d\n",
            wValue));
        break;

    /* INTERFACE REQUESTS (no defined feature/status flags) */

    /* ENDPOINT REQUESTS */

    case EndpointRequest | USB_REQ_GET_STATUS:
        /* ENDPOINT_HALT flag */
        tbuf[0] = 0;
        tbuf[1] = 0;
        len = 2;
            /* FALLTHROUGH */
    case EndpointOutRequest | USB_REQ_CLEAR_FEATURE:
    case EndpointOutRequest | USB_REQ_SET_FEATURE:
        RT_DEBUG_LOG(rt_debug_usb, ("no endpoint features yet\n"));
        break;

    /* CLASS REQUESTS (and errors) */

    default:
        /* non-generic request */
        switch (typeReq)
        {
        case GetHubStatus:
        case GetPortStatus:
            len = 4;
            break;
        case GetHubDescriptor:
            len = sizeof(struct usb_hub_descriptor);
            break;
        }
        status = hcd->driver->hub_control(hcd,
            typeReq, wValue, wIndex,
            (char *)tbuf, wLength);
        break;
error:
        /* "protocol stall" on error */
        status = -EPIPE;
    }

    if (status)
    {
        len = 0;
        if (status != -EPIPE)
        {
            RT_DEBUG_LOG(rt_debug_usb,
                ("CTRL: TypeReq=0x%x val=0x%x idx=0x%x len=%d ==> %d\n",
                typeReq, wValue, wIndex,
                wLength, status));
        }
    }
    if (len)
    {
        if (urb->transfer_buffer_length < len)
            len = urb->transfer_buffer_length;
        urb->actual_length = len;
        /* always USB_DIR_IN, toward host */
        rt_memcpy(ubuf, bufp, len);

        /* report whether RH hardware has an integrated TT */
        if (patch_protocol &&
                len > offsetof(struct usb_device_descriptor,
                        bDeviceProtocol))
            ((struct usb_device_descriptor *) ubuf)->
                    bDeviceProtocol = 1;
    }

    /* any errors get returned through the urb completion */
/* spin_lock_irq(&hcd_root_hub_lock); */
    level = rt_hw_interrupt_disable();
        rt_enter_critical();

    usb_hcd_unlink_urb_from_ep(hcd, urb);

    /* This peculiar use of spinlocks echoes what real HC drivers do.
     * Avoiding calls to local_irq_disable/enable makes the code
     * RT-friendly.
     */
    /* spin_unlock(&hcd_root_hub_lock); */
     rt_exit_critical();
    usb_hcd_giveback_urb(hcd, urb, status);
    /* spin_lock(&hcd_root_hub_lock); */
/* rt_enter_critical(); */

    rt_hw_interrupt_enable(level);
/* rt_exit_critical(); */

    /* spin_unlock_irq(&hcd_root_hub_lock); */
    return 0;
}


/*-------------------------------------------------------------------------*/

/*
 * Root Hub interrupt transfers are polled using a timer if the
 * driver requests it; otherwise the driver is responsible for
 * calling usb_hcd_poll_rh_status() when an event occurs.
 *
 * Completions are called in_interrupt(), but they may or may not
 * be in_irq().
 */
void usb_hcd_poll_rh_status(struct usb_hcd *hcd)
{
    struct urb    *urb;
    int        length;
    char        buffer[6];    /* Any root hubs with > 31 ports? */
/* rt_base_t level; */

    if ((!hcd->rh_pollable))
        return;
    if (!hcd->uses_new_polling && !hcd->status_urb)
        return;

    length = hcd->driver->hub_status_data(hcd, buffer);
    if (length > 0)
    {

        /* try to complete the status urb */
        /* spin_lock_irqsave(&hcd_root_hub_lock, flags); */
/* level=rt_hw_interrupt_disable(); */
/* rt_enter_critical(); */
        urb = hcd->status_urb;
        if (urb)
        {
            clear_bit(HCD_FLAG_POLL_PENDING, &hcd->flags);
            hcd->status_urb = RT_NULL;
            urb->actual_length = length;
            rt_memcpy(urb->transfer_buffer, buffer, length);

            usb_hcd_unlink_urb_from_ep(hcd, urb);
            /* spin_unlock(&hcd_root_hub_lock); */
/* rt_exit_critical(); */
            usb_hcd_giveback_urb(hcd, urb, 0);
            /* spin_lock(&hcd_root_hub_lock); */
/* rt_enter_critical(); */
        } else
        {
            length = 0;
            set_bit(HCD_FLAG_POLL_PENDING, &hcd->flags);
        }
/* rt_hw_interrupt_enable(level); */
/* rt_exit_critical(); */
    }

    /* The USB 2.0 spec says 256 ms.  This is close enough and won't
     * exceed that limit if HZ is 100. The math is more clunky than
     * maybe expected, this is to make sure that all timers for USB devices
     * fire at the same time to give the CPU a break in between */
/* if (hcd->uses_new_polling ? HCD_POLL_RH(hcd) : */
/* (length == 0 && hcd->status_urb != RT_NULL)) */
/* rt_timer_start(hcd->rh_timer); */
}
/* timer callback */
static void rh_timer_func(void *_hcd)
{
    usb_hcd_poll_rh_status((struct usb_hcd *) _hcd);
}

/*-------------------------------------------------------------------------*/

static int rh_queue_status(struct usb_hcd *hcd, struct urb *urb)
{
    int        retval;
    rt_base_t level;
    unsigned int len = 1 + (urb->dev->maxchild / 8);

    level = rt_hw_interrupt_disable();
    rt_enter_critical();
    if (hcd->status_urb || urb->transfer_buffer_length < len)
    {
        RT_DEBUG_LOG(rt_debug_usb, ("not queuing rh status urb\n"));
        retval = -EINVAL;
        goto done;
    }

    retval = usb_hcd_link_urb_to_ep(hcd, urb);
    if (retval)
        goto done;

    hcd->status_urb = urb;
    urb->hcpriv = hcd;    /* indicate it's queued */

    /* If a status change has already occurred, report it ASAP */
/* else if (HCD_POLL_PENDING(hcd)) */
/* mod_timer(&hcd->rh_timer, jiffies); */
    retval = 0;
 done:
    rt_hw_interrupt_enable(level);
    rt_exit_critical();
    return retval;
}

static int rh_urb_enqueue(struct usb_hcd *hcd, struct urb *urb)
{
    if (usb_endpoint_xfer_int(&urb->ep->desc))
        return rh_queue_status(hcd, urb);
    if (usb_endpoint_xfer_control(&urb->ep->desc))
        return rh_call_control(hcd, urb);
    return -EINVAL;
}

/*-------------------------------------------------------------------------*/

/* Unlinks of root-hub control URBs are legal, but they don't do anything
 * since these URBs always execute synchronously.
 */
static int usb_rh_urb_dequeue(struct usb_hcd *hcd, struct urb *urb, int status)
{
    rt_base_t level;
    int        rc;

    level = rt_hw_interrupt_disable();
    rt_enter_critical();
    rc = usb_hcd_check_unlink_urb(hcd, urb, status);
    if (rc)
        goto done;

    if (usb_endpoint_num(&urb->ep->desc) == 0)    /* Control URB */
    {
        ;    /* Do nothing */

    } else                /* Status URB */
    {
        if (!hcd->uses_new_polling)
            rt_timer_delete(hcd->rh_timer);
        if (urb == hcd->status_urb)
        {
            hcd->status_urb = RT_NULL;
            usb_hcd_unlink_urb_from_ep(hcd, urb);

            rt_exit_critical();
            usb_hcd_giveback_urb(hcd, urb, status);
            rt_enter_critical();
        }
    }
 done:
    rt_hw_interrupt_enable(level);
     rt_exit_critical();
    return rc;
}


#define MAX_DATA_PACKET_SIZE 2048
#define MAX_CTRL_PACKET_SIZE 8


/*-------------------------------------------------------------------------*/

/**
 * usb_bus_init - shared initialization code
 * @bus: the bus structure being initialized
 *
 * This code is used to initialize a usb_bus structure, memory for which is
 * separately managed.
 */
static void usb_bus_init(struct usb_bus *bus)
{
    rt_memset(&bus->devmap, 0, sizeof(struct usb_devmap));

    bus->devnum_next = 1;

    bus->root_hub = RT_NULL;
    bus->busnum = -1;
    bus->bandwidth_allocated = 0;
    bus->bandwidth_int_reqs  = 0;
    bus->bandwidth_isoc_reqs = 0;

    rt_list_init(&bus->bus_list);

    usb_dma_buffer_create(bus);
}

/*-------------------------------------------------------------------------*/

/**
 * usb_register_bus - registers the USB host controller with the usb core
 * @bus: pointer to the bus to register
 * Context: !in_interrupt()
 *
 * Assigns a bus number, and links the controller into usbcore data
 * structures so that it can be seen by scanning the bus list.
 */
static int usb_register_bus(struct usb_bus *bus)
{
    int result = -E2BIG;
    int busnum;

/* mutex_lock(&usb_bus_list_lock); */
    busnum = find_next_zero_bit((rt_uint32_t *)(busmap.busmap), USB_MAXBUS, 1);
    if (busnum >= USB_MAXBUS)
    {
        rt_kprintf("%s: too many buses\n", usbcore_name);
        goto error_find_busnum;
    }
    set_bit(busnum, busmap.busmap);
    bus->busnum = busnum;

    /* Add it to the local list of buses */
    rt_list_insert_after(&usb_bus_list, &bus->bus_list);
/* mutex_unlock(&usb_bus_list_lock); */

/* usb_notify_add_bus(bus); */

    RT_DEBUG_LOG(rt_debug_usb, ("new USB bus registered, assigned bus number %d\n",
        bus->busnum));
    return 0;

error_find_busnum:
/* mutex_unlock(&usb_bus_list_lock); */
    return result;
}

/**
 * usb_deregister_bus - deregisters the USB host controller
 * @bus: pointer to the bus to deregister
 * Context: !in_interrupt()
 *
 * Recycles the bus number, and unlinks the controller from usbcore data
 * structures so that it won't be seen by scanning the bus list.
 */
#if 1
static void usb_deregister_bus(struct usb_bus *bus)
{
    RT_DEBUG_LOG(rt_debug_usb, ("USB bus %d deregistered\n", bus->busnum));

    /*
     * NOTE: make sure that all the devices are removed by the
     * controller code, as well as having it call this when cleaning
     * itself up
     */
/* mutex_lock(&usb_bus_list_lock); */
    rt_list_remove(&bus->bus_list);
/* mutex_unlock(&usb_bus_list_lock); */

    clear_bit(bus->busnum, busmap.busmap);
}

#endif

/**
 * register_root_hub - called by usb_add_hcd() to register a root hub
 * @hcd: host controller for this root hub
 *
 * This function registers the root hub with the USB subsystem.  It sets up
 * the device properly in the device tree and then calls usb_new_device()
 * to register the usb device.  It also assigns the root hub's USB address
 * (always 1).
 */
static int register_root_hub(struct usb_hcd *hcd)
{
    struct usb_device *usb_dev = hcd->self.root_hub;
    const int devnum = 1;
    int retval;
/* rt_base_t level; */

    usb_dev->devnum = devnum;
    usb_dev->bus->devnum_next = devnum + 1;
    rt_memset(&usb_dev->bus->devmap.devicemap, 0,
            sizeof(usb_dev->bus->devmap.devicemap));
    set_bit(devnum, usb_dev->bus->devmap.devicemap);
    usb_set_device_state(usb_dev, USB_STATE_ADDRESS);

/* mutex_lock(&usb_bus_list_lock); */

    usb_dev->ep0.desc.wMaxPacketSize = (64);
    retval = usb_get_device_descriptor(usb_dev, USB_DT_DEVICE_SIZE);
    if (retval != sizeof(usb_dev->descriptor))
    {
/* mutex_unlock(&usb_bus_list_lock); */
        RT_DEBUG_LOG(rt_debug_usb, ("can't read device descriptor %d\n",
                 retval));
        return (retval < 0) ? retval : -EMSGSIZE;
    }

    retval = usb_new_device(usb_dev);
    if (retval)
    {
        RT_DEBUG_LOG(rt_debug_usb, ("can't register root hub %d\n",
                 retval));
    }

    return retval;
}


/*-------------------------------------------------------------------------*/

/**
 * usb_calc_bus_time - approximate periodic transaction time in nanoseconds
 * @speed: from dev->speed; USB_SPEED_{LOW,FULL,HIGH}
 * @is_input: true iff the transaction sends data to the host
 * @isoc: true for isochronous transactions, false for interrupt ones
 * @bytecount: how many bytes in the transaction.
 *
 * Returns approximate bus time in nanoseconds for a periodic transaction.
 * See USB 2.0 spec section 5.11.3; only periodic transfers need to be
 * scheduled in software, this function is only used for such scheduling.
 */
long usb_calc_bus_time(int speed, int is_input, int isoc, int bytecount)
{
    unsigned long    tmp;

    switch (speed)
    {
    case USB_SPEED_LOW:     /* INTR only */
        if (is_input)
        {
            tmp = (67667L * (31L + 10L * BitTime(bytecount))) / 1000L;
            return (64060L + (2 * BW_HUB_LS_SETUP) + BW_HOST_DELAY + tmp);
        } else
        {
            tmp = (66700L * (31L + 10L * BitTime(bytecount))) / 1000L;
            return (64107L + (2 * BW_HUB_LS_SETUP) + BW_HOST_DELAY + tmp);
        }
    case USB_SPEED_FULL:    /* ISOC or INTR */
        if (isoc)
        {
            tmp = (8354L * (31L + 10L * BitTime(bytecount))) / 1000L;
            return (((is_input) ? 7268L : 6265L) + BW_HOST_DELAY + tmp);
        } else
        {
            tmp = (8354L * (31L + 10L * BitTime(bytecount))) / 1000L;
            return (9107L + BW_HOST_DELAY + tmp);
        }
    case USB_SPEED_HIGH:    /* ISOC or INTR */
        /* FIXME adjust for input vs output */
        if (isoc)
            tmp = HS_NSECS_ISO(bytecount);
        else
            tmp = HS_NSECS(bytecount);
        return tmp;
    default:
        RT_DEBUG_LOG(rt_debug_usb, ("%s: bogus device speed!\n", usbcore_name));
        return -1;
    }
}


/*-------------------------------------------------------------------------*/

/*
 * Generic HC operations.
 */

/*-------------------------------------------------------------------------*/

/**
 * usb_hcd_link_urb_to_ep - add an URB to its endpoint queue
 * @hcd: host controller to which @urb was submitted
 * @urb: URB being submitted
 *
 * Host controller drivers should call this routine in their enqueue()
 * method.  The HCD's private spinlock must be held and interrupts must
 * be disabled.  The actions carried out here are required for URB
 * submission, as well as for endpoint shutdown and for usb_kill_urb.
 *
 * Returns 0 for no error, otherwise a negative error code (in which case
 * the enqueue() method must fail).  If no error occurs but enqueue() fails
 * anyway, it must call usb_hcd_unlink_urb_from_ep() before releasing
 * the private spinlock and returning.
 */
int usb_hcd_link_urb_to_ep(struct usb_hcd *hcd, struct urb *urb)
{
    int        rc = 0;

    rt_enter_critical();

    /* Check that the URB isn't being killed */
    if (urb->reject)
    {
        rc = -EPERM;
        goto done;
    }

    if ((!urb->ep->enabled))
    {
        rc = -ENOENT;
        goto done;
    }

    /*
     * Check the host controller's state and add the URB to the
     * endpoint's queue.
     */
    if (HCD_RH_RUNNING(hcd))
    {
        urb->unlinked = 0;
        rt_list_insert_after(&urb->ep->urb_list, &urb->urb_list);
    } else
    {
        rc = -ESHUTDOWN;
        goto done;
    }
 done:
    rt_exit_critical();
    return rc;
}


/**
 * usb_hcd_check_unlink_urb - check whether an URB may be unlinked
 * @hcd: host controller to which @urb was submitted
 * @urb: URB being checked for unlinkability
 * @status: error code to store in @urb if the unlink succeeds
 *
 * Host controller drivers should call this routine in their dequeue()
 * method.  The HCD's private spinlock must be held and interrupts must
 * be disabled.  The actions carried out here are required for making
 * sure than an unlink is valid.
 *
 * Returns 0 for no error, otherwise a negative error code (in which case
 * the dequeue() method must fail).  The possible error codes are:
 *
 *    -EIDRM: @urb was not submitted or has already completed.
 *        The completion function may not have been called yet.
 *
 *    -EBUSY: @urb has already been unlinked.
 */
int usb_hcd_check_unlink_urb(struct usb_hcd *hcd, struct urb *urb,
        int status)
{
    rt_list_t    *tmp;

    /* insist the urb is still queued */
    for (tmp = urb->ep->urb_list.next; tmp !=  &(urb->ep->urb_list); tmp = tmp->next)
    {
        if (tmp == &urb->urb_list)
            break;
    }
    if (tmp != &urb->urb_list)
        return -EIDRM;

    /* Any status except -EINPROGRESS means something already started to
     * unlink this URB from the hardware.  So there's no more work to do.
     */
    if (urb->unlinked)
        return -EBUSY;
    urb->unlinked = status;

    /* IRQ setup can easily be broken so that USB controllers
     * never get completion IRQs ... maybe even the ones we need to
     * finish unlinking the initial failed usb_set_address()
     * or device descriptor fetch.
     */
    if (!HCD_SAW_IRQ(hcd) && !is_root_hub(urb->dev))
    {
        RT_DEBUG_LOG(rt_debug_usb,("Unlink after no-IRQ?  Controller is probably using the wrong IRQ.\n"
            ));
        set_bit(HCD_FLAG_SAW_IRQ, &hcd->flags);
        if (hcd->shared_hcd)
            set_bit(HCD_FLAG_SAW_IRQ, &hcd->shared_hcd->flags);
    }

    return 0;
}

/**
 * usb_hcd_unlink_urb_from_ep - remove an URB from its endpoint queue
 * @hcd: host controller to which @urb was submitted
 * @urb: URB being unlinked
 *
 * Host controller drivers should call this routine before calling
 * usb_hcd_giveback_urb().  The HCD's private spinlock must be held and
 * interrupts must be disabled.  The actions carried out here are required
 * for URB completion.
 */
void usb_hcd_unlink_urb_from_ep(struct usb_hcd *hcd, struct urb *urb)
{
    /* clear all state linking urb to this dev (and hcd) */
    rt_list_remove(&urb->urb_list);

}




struct usb_hcd *usb_create_shared_hcd(const struct hc_driver *driver,
        struct rt_device *dev, const char *bus_name)
{
    struct usb_hcd *hcd;

    hcd = rt_malloc(sizeof(*hcd) + driver->hcd_priv_size);
    RT_DEBUG_LOG(rt_debug_usb, ("rt_malloc hcd :%d size:%d,addr:%x\n", g_mem_debug++, sizeof(*hcd) + driver->hcd_priv_size, hcd));
    if (!hcd)
    {

        RT_DEBUG_LOG(rt_debug_usb, ("hcd alloc failed\n"));
        return RT_NULL;
    }
    rt_memset(hcd, 0, sizeof(*hcd) + driver->hcd_priv_size);
    hcd->bandwidth_mutex = rt_malloc(sizeof(*hcd->bandwidth_mutex));
        if (!hcd->bandwidth_mutex)
        {
            rt_free(hcd);
            /* RT_DEBUG_LOG(rt_debug_usb,("rt_free hcd:%d\n",--g_mem_debug)); */
            RT_DEBUG_LOG(rt_debug_usb, ("hcd bandwidth mutex alloc failed\n"));
            return RT_NULL;
        }

    rt_mutex_init(hcd->bandwidth_mutex, "bandwidth", RT_IPC_FLAG_FIFO);


    usb_bus_init(&hcd->self);
    hcd->self.controller = dev;
    hcd->self.bus_name = bus_name;
    /* hcd->self.uses_dma = ((dev->flag&(RT_DEVICE_FLAG_DMA_RX|RT_DEVICE_FLAG_DMA_TX) )!=RT_NULL); */

    hcd->rh_timer = rt_timer_create("hcd_timer", rh_timer_func, (void *) hcd, 10, RT_TIMER_FLAG_PERIODIC);

    hcd->driver = driver;
    hcd->speed = driver->flags & HCD_MASK;
    hcd->product_desc = (driver->product_desc) ? driver->product_desc :
            "USB Host Controller";
    return hcd;
}

struct usb_hcd *usb_create_hcd(const struct hc_driver *driver,
        struct rt_device *dev, const char *bus_name)
{
    return usb_create_shared_hcd(driver, dev, bus_name);
}

/*-------------------------------------------------------------------------*/

/* may be called in any context with a valid urb->dev usecount
 * caller surrenders "ownership" of urb
 * expects usb_submit_urb() to have sanity checked and conditioned all
 * inputs in the urb
 */
extern void mmu_clean_invalidated_dcache(rt_uint32_t buffer, rt_uint32_t size);
#define USBNET_CACHE_WRITEBACK_INVALIDATE(addr, size) \
	    mmu_clean_invalidated_dcache((rt_uint32_t)addr, size)
int usb_hcd_submit_urb(struct urb *urb)
{
    int            status;
    struct usb_hcd        *hcd = bus_to_hcd(urb->dev->bus);

    /* increment urb's reference count as part of giving it to the HCD
     * (which will control it).  HCD guarantees that it either returns
     * an error or calls giveback(), but not both.
     */
    usb_get_urb(urb);
    urb->use_count++;
    urb->dev->urbnum++;

    /* NOTE requirements on root-hub callers (usbfs and the hub
     * driver, for now):  URBs' urb->transfer_buffer must be
     * valid and usb_buffer_{sync,unmap}() not be needed, since
     * they could clobber root hub response data.  Also, control
     * URBs must be submitted in process context with interrupts
     * enabled.
     */

    if (is_root_hub(urb->dev))
    {
        urb->transfer_dma = (unsigned int)(urb->transfer_buffer);
        urb->setup_dma = (unsigned int)(urb->setup_packet);
	USBNET_CACHE_WRITEBACK_INVALIDATE( urb->transfer_dma,urb->transfer_buffer_length);
        status = rh_urb_enqueue(hcd, urb);
    }
    else
    {
        urb->transfer_dma = (unsigned int)(urb->transfer_buffer);
        urb->setup_dma = (unsigned int)(urb->setup_packet);
	USBNET_CACHE_WRITEBACK_INVALIDATE( urb->transfer_dma,urb->transfer_buffer_length);
        status = hcd->driver->urb_enqueue(hcd, urb);
    }


    if (status)
    {
        urb->hcpriv = RT_NULL;
        rt_list_init(&urb->urb_list);
        urb->use_count--;
        urb->dev->urbnum--;
        usb_put_urb(urb);
    }
    return status;
}

/*-------------------------------------------------------------------------*/

/* this makes the hcd giveback() the urb more quickly, by kicking it
 * off hardware queues (which may take a while) and returning it as
 * soon as practical.  we've already set up the urb's return status,
 * but we can't know if the callback completed already.
 */
static int unlink1(struct usb_hcd *hcd, struct urb *urb, int status)
{
    int        value;

    if (is_root_hub(urb->dev))
        value = usb_rh_urb_dequeue(hcd, urb, status);
    else
    {

        /* The only reason an HCD might fail this call is if
         * it has not yet fully queued the urb to begin with.
         * Such failures should be harmless. */
        value = hcd->driver->urb_dequeue(hcd, urb, status);
    }
    return value;
}

/*
 * called in any context
 *
 * caller guarantees urb won't be recycled till both unlink()
 * and the urb's completion function return
 */
int usb_hcd_unlink_urb(struct urb *urb, int status)
{
    struct usb_hcd        *hcd;
    int            retval = -EIDRM;
/* unsigned long        flags; */

    /* Prevent the device and bus from going away while
     * the unlink is carried out.  If they are already gone
     * then urb->use_count must be 0, since disconnected
     * devices can't have any active URBs.
     */
/* spin_lock_irqsave(&hcd_urb_unlink_lock, flags); */
    if (urb->use_count > 0)
    {
        retval = 0;
    }
/* spin_unlock_irqrestore(&hcd_urb_unlink_lock, flags); */
    if (retval == 0)
    {
        hcd = bus_to_hcd(urb->dev->bus);
        retval = unlink1(hcd, urb, status);
    }

    if (retval == 0)
        retval = -EINPROGRESS;
    else if (retval != -EIDRM && retval != -EBUSY)
        RT_DEBUG_LOG(rt_debug_usb, ("hcd_unlink_urb %p fail %d\n",
                urb, retval));
    return retval;
}

/*-------------------------------------------------------------------------*/

/**
 * usb_hcd_giveback_urb - return URB from HCD to device driver
 * @hcd: host controller returning the URB
 * @urb: urb being returned to the USB device driver.
 * @status: completion status code for the URB.
 * Context: in_interrupt()
 *
 * This hands the URB from HCD to its USB device driver, using its
 * completion function.  The HCD has freed all per-urb resources
 * (and is done using urb->hcpriv).  It also released all HCD locks;
 * the device driver won't cause problems if it frees, modifies,
 * or resubmits this URB.
 *
 * If @urb was unlinked, the value of @status will be overridden by
 * @urb->unlinked.  Erroneous short transfers are detected in case
 * the HCD hasn't checked for them.
 */
void usb_hcd_giveback_urb(struct usb_hcd *hcd, struct urb *urb, int status)
{
    if (urb == NULL)
    {
        RT_DEBUG_LOG(rt_debug_usb, ("urb is null"));
        return;
    }
    urb->hcpriv = RT_NULL;
    if (urb->unlinked)
        status = urb->unlinked;
    else if (((urb->transfer_flags & URB_SHORT_NOT_OK) &&
            urb->actual_length < urb->transfer_buffer_length &&
            !status))
        status = -EREMOTEIO;


    /* pass ownership to the completion handler */
    if (status == 0) /*invalidate cache again for cache prefetch, I guess. by PeterJiang*/
    {
        USBNET_CACHE_WRITEBACK_INVALIDATE(urb->transfer_dma, urb->actual_length);
    }
    urb->status = status;
    urb->complete(urb);
        urb->use_count--;
    /* usb_put_urb (urb); */
        urb->kref--;
}

/*-------------------------------------------------------------------------*/

/* Cancel all URBs pending on this endpoint and wait for the endpoint's
 * queue to drain completely.  The caller must first insure that no more
 * URBs can be submitted for this endpoint.
 */
void usb_hcd_flush_endpoint(struct usb_device *udev,
        struct usb_host_endpoint *ep)
{
    struct usb_hcd        *hcd;
    struct urb        *urb;

    if (!ep)
        return;
    hcd = bus_to_hcd(udev->bus);

    /* No more submits can occur */
/* spin_lock_irq(&hcd_urb_list_lock); */
rescan:
    list_for_each_entry(urb, &ep->urb_list, urb_list) {
/* int    is_in; */

        if (urb->unlinked)
            continue;
        usb_get_urb(urb);
/* is_in = usb_urb_dir_in(urb); */
/* spin_unlock(&hcd_urb_list_lock); */

        /* kick hcd */
        unlink1(hcd, urb, -ESHUTDOWN);
/* RT_DEBUG_LOG(rt_debug_usb, */
/* ("shutdown urb %p ep%d%s%s\n", */
/* urb, usb_endpoint_num(&ep->desc), */
/* is_in ? "in" : "out", */
/* ({    char *s; */
/*  */
/* switch (usb_endpoint_type(&ep->desc)) { */
/* case USB_ENDPOINT_XFER_CONTROL: */
/* s = ""; break; */
/* case USB_ENDPOINT_XFER_BULK: */
/* s = "-bulk"; break; */
/* case USB_ENDPOINT_XFER_INT: */
/* s = "-intr"; break; */
/* default: */
/* s = "-iso"; break; */
/* }; */
/* s; */
/* }))); */
        usb_put_urb(urb);

        /* list contents may have changed */
/* spin_lock(&hcd_urb_list_lock); */
        goto rescan;
    }
/* spin_unlock_irq(&hcd_urb_list_lock); */

    /* Wait until the endpoint queue is completely empty */
    while (!rt_list_isempty(&ep->urb_list))
    {
/* spin_lock_irq(&hcd_urb_list_lock); */

        /* The list may have changed while we acquired the spinlock */
        urb = RT_NULL;
        if (!rt_list_isempty(&ep->urb_list))
        {
            urb = rt_list_entry(ep->urb_list.prev, struct urb,
                    urb_list);
            usb_get_urb(urb);
        }
/* spin_unlock_irq(&hcd_urb_list_lock); */

        if (urb)
        {
            usb_kill_urb(urb);
            usb_put_urb(urb);
        }
    }
}


/**
 * usb_hcd_alloc_bandwidth - check whether a new bandwidth setting exceeds
 *                the bus bandwidth
 * @udev: target &usb_device
 * @new_config: new configuration to install
 * @cur_alt: the current alternate interface setting
 * @new_alt: alternate interface setting that is being installed
 *
 * To change configurations, pass in the new configuration in new_config,
 * and pass NULL for cur_alt and new_alt.
 *
 * To reset a device's configuration (put the device in the ADDRESSED state),
 * pass in NULL for new_config, cur_alt, and new_alt.
 *
 * To change alternate interface settings, pass in NULL for new_config,
 * pass in the current alternate interface setting in cur_alt,
 * and pass in the new alternate interface setting in new_alt.
 *
 * Returns an error if the requested bandwidth change exceeds the
 * bus bandwidth or host controller internal resources.
 */
int usb_hcd_alloc_bandwidth(struct usb_device *udev,
        struct usb_host_config *new_config,
        struct usb_host_interface *cur_alt,
        struct usb_host_interface *new_alt)
{
    int num_intfs, i, j;
    struct usb_host_interface *alt = RT_NULL;
    int ret = 0;
    struct usb_hcd *hcd;
    struct usb_host_endpoint *ep;

    hcd = bus_to_hcd(udev->bus);
    if (!hcd->driver->check_bandwidth)
        return 0;

    /* Configuration is being removed - set configuration 0 */
    if (!new_config && !cur_alt)
    {
        for (i = 1; i < 16; ++i)
        {
            ep = udev->ep_out[i];
            if (ep)
                hcd->driver->drop_endpoint(hcd, udev, ep);
            ep = udev->ep_in[i];
            if (ep)
                hcd->driver->drop_endpoint(hcd, udev, ep);
        }
        hcd->driver->check_bandwidth(hcd, udev);
        return 0;
    }
    /* Check if the HCD says there's enough bandwidth.  Enable all endpoints
     * each interface's alt setting 0 and ask the HCD to check the bandwidth
     * of the bus.  There will always be bandwidth for endpoint 0, so it's
     * ok to exclude it.
     */
    if (new_config)
    {
        num_intfs = new_config->desc.bNumInterfaces;
        /* Remove endpoints (except endpoint 0, which is always on the
         * schedule) from the old config from the schedule
         */
        for (i = 1; i < 16; ++i)
        {
            ep = udev->ep_out[i];
            if (ep)
            {
                ret = hcd->driver->drop_endpoint(hcd, udev, ep);
                if (ret < 0)
                    goto reset;
            }
            ep = udev->ep_in[i];
            if (ep)
            {
                ret = hcd->driver->drop_endpoint(hcd, udev, ep);
                if (ret < 0)
                    goto reset;
            }
        }
        for (i = 0; i < num_intfs; ++i)
        {
            struct usb_host_interface *first_alt;
            int iface_num;

            first_alt = &new_config->intf_cache[i]->altsetting[0];
            iface_num = first_alt->desc.bInterfaceNumber;
            /* Set up endpoints for alternate interface setting 0 */
            alt = usb_find_alt_setting(new_config, iface_num, 0);
            if (!alt)
                /* No alt setting 0? Pick the first setting. */
                alt = first_alt;

            for (j = 0; j < alt->desc.bNumEndpoints; j++)
            {
                ret = hcd->driver->add_endpoint(hcd, udev, &alt->endpoint[j]);
                if (ret < 0)
                    goto reset;
            }
        }
    }
    if (cur_alt && new_alt)
    {
        struct usb_interface *iface = usb_ifnum_to_if(udev,
                cur_alt->desc.bInterfaceNumber);

        if (iface->resetting_device)
        {
            /*
             * The USB core just reset the device, so the xHCI host
             * and the device will think alt setting 0 is installed.
             * However, the USB core will pass in the alternate
             * setting installed before the reset as cur_alt.  Dig
             * out the alternate setting 0 structure, or the first
             * alternate setting if a broken device doesn't have alt
             * setting 0.
             */
            cur_alt = usb_altnum_to_altsetting(iface, 0);
            if (!cur_alt)
                cur_alt = &iface->altsetting[0];
        }

        /* Drop all the endpoints in the current alt setting */
        for (i = 0; i < cur_alt->desc.bNumEndpoints; i++)
        {
            ret = hcd->driver->drop_endpoint(hcd, udev,
                    &cur_alt->endpoint[i]);
            if (ret < 0)
                goto reset;
        }
        /* Add all the endpoints in the new alt setting */
        for (i = 0; i < new_alt->desc.bNumEndpoints; i++)
        {
            ret = hcd->driver->add_endpoint(hcd, udev,
                    &new_alt->endpoint[i]);
            if (ret < 0)
                goto reset;
        }
    }
    ret = hcd->driver->check_bandwidth(hcd, udev);
reset:
    if (ret < 0)
        hcd->driver->reset_bandwidth(hcd, udev);
    return ret;
}

/* Disables the endpoint: synchronizes with the hcd to make sure all
 * endpoint state is gone from hardware.  usb_hcd_flush_endpoint() must
 * have been called previously.  Use for set_configuration, set_interface,
 * driver removal, physical disconnect.
 *
 * example:  a qh stored in ep->hcpriv, holding state related to endpoint
 * type, maxpacket size, toggle, halt status, and scheduling.
 */
void usb_hcd_disable_endpoint(struct usb_device *udev,
        struct usb_host_endpoint *ep)
{
    struct usb_hcd        *hcd;

    hcd = bus_to_hcd(udev->bus);
    if (hcd->driver->endpoint_disable)
        hcd->driver->endpoint_disable(hcd, ep);
}

/**
 * usb_hcd_reset_endpoint - reset host endpoint state
 * @udev: USB device.
 * @ep:   the endpoint to reset.
 *
 * Resets any host endpoint state such as the toggle bit, sequence
 * number and current window.
 */
void usb_hcd_reset_endpoint(struct usb_device *udev,
                struct usb_host_endpoint *ep)
{
    struct usb_hcd *hcd = bus_to_hcd(udev->bus);

    if (hcd->driver->endpoint_reset)
        hcd->driver->endpoint_reset(hcd, ep);
    else
    {
        int epnum = usb_endpoint_num(&ep->desc);
        int is_out = usb_endpoint_dir_out(&ep->desc);
        int is_control = usb_endpoint_xfer_control(&ep->desc);

        usb_settoggle(udev, epnum, is_out, 0);
        if (is_control)
            usb_settoggle(udev, epnum, !is_out, 0);
    }
}

/**
 * usb_alloc_streams - allocate bulk endpoint stream IDs.
 * @interface:        alternate setting that includes all endpoints.
 * @eps:        array of endpoints that need streams.
 * @num_eps:        number of endpoints in the array.
 * @num_streams:    number of streams to allocate.
 * @mem_flags:        flags hcd should use to allocate memory.
 *
 * Sets up a group of bulk endpoints to have num_streams stream IDs available.
 * Drivers may queue multiple transfers to different stream IDs, which may
 * complete in a different order than they were queued.
 */
int usb_alloc_streams(struct usb_interface *interface,
        struct usb_host_endpoint **eps, unsigned int num_eps,
        unsigned int num_streams)
{
    struct usb_hcd *hcd;
    struct usb_device *dev;
    int i;

    dev = interface->usb_dev;
    hcd = bus_to_hcd(dev->bus);
    if (!hcd||!hcd->driver->alloc_streams || !hcd->driver->free_streams)
        return -EINVAL;
    if (dev->speed != USB_SPEED_SUPER)
        return -EINVAL;

    /* Streams only apply to bulk endpoints. */
    for (i = 0; i < num_eps; i++)
        if (!usb_endpoint_xfer_bulk(&eps[i]->desc))
            return -EINVAL;

    return hcd->driver->alloc_streams(hcd, dev, eps, num_eps,
            num_streams);
}

/**
 * usb_free_streams - free bulk endpoint stream IDs.
 * @interface:    alternate setting that includes all endpoints.
 * @eps:    array of endpoints to remove streams from.
 * @num_eps:    number of endpoints in the array.
 * @mem_flags:    flags hcd should use to allocate memory.
 *
 * Reverts a group of bulk endpoints back to not using stream IDs.
 * Can fail if we are given bad arguments, or HCD is broken.
 */
void usb_free_streams(struct usb_interface *interface,
        struct usb_host_endpoint **eps, unsigned int num_eps)
{
    struct usb_hcd *hcd;
    struct usb_device *dev;
    int i;

    dev = interface->usb_dev;
    hcd = bus_to_hcd(dev->bus);
    if (dev->speed != USB_SPEED_SUPER)
        return;

    /* Streams only apply to bulk endpoints. */
    for (i = 0; i < num_eps; i++)
        if (!eps[i] || !usb_endpoint_xfer_bulk(&eps[i]->desc))
            return;

    hcd->driver->free_streams(hcd, dev, eps, num_eps);
}


/*-------------------------------------------------------------------------*/

/* called in any context */
int usb_hcd_get_frame_number(struct usb_device *udev)
{
    struct usb_hcd    *hcd = bus_to_hcd(udev->bus);

    if (!HCD_RH_RUNNING(hcd))
        return -ESHUTDOWN;
    return hcd->driver->get_frame_number(hcd);
}

/*-------------------------------------------------------------------------*/

/**
 * usb_hcd_irq - hook IRQs to HCD framework (bus glue)
 * @irq: the IRQ being raised
 * @__hcd: pointer to the HCD whose IRQ is being signaled
 *
 * If the controller isn't HALTed, calls the driver's irq handler.
 * Checks whether the controller is now dead.
 */
void usb_hcd_irq(int vector, void *__hcd)
{
    struct usb_hcd        *hcd = __hcd;
/* unsigned long        flags; */
/* rt_uint32_t        rc; */


    if ((HCD_DEAD(hcd) || !HCD_HW_ACCESSIBLE(hcd)))
    {
        /* rc = IRQ_NONE; */
        ;
    } else if (hcd->driver->irq(hcd) == IRQ_NONE)
    {
        /* rc = IRQ_NONE; */
        ;
    } else
    {
        set_bit(HCD_FLAG_SAW_IRQ, &hcd->flags);
        if (hcd->shared_hcd)
            set_bit(HCD_FLAG_SAW_IRQ, &hcd->shared_hcd->flags);
        /* rc = IRQ_HANDLED; */
    }

/* rt_hw_interrupt_mask(vector); */
/* return rc; */
}

/*-------------------------------------------------------------------------*/



int usb_hcd_is_primary_hcd(struct usb_hcd *hcd)
{
    if (!hcd->primary_hcd)
        return 1;
    return hcd == hcd->primary_hcd;
}


static int usb_hcd_request_irqs(struct usb_hcd *hcd,
        unsigned int irqnum)
{
    rt_isr_handler_t retval = RT_NULL;

    if (hcd->driver->irq)
    {

        rt_snprintf(hcd->irq_descr, sizeof(hcd->irq_descr), "%s:usb%d",
                hcd->driver->description, hcd->self.busnum);
        retval = rt_hw_interrupt_install(irqnum, usb_hcd_irq, hcd, hcd->irq_descr);
        if (retval == RT_NULL)
        {
            RT_DEBUG_LOG(rt_debug_usb,
                    ("request interrupt %d failed\n",
                    irqnum));
            return -RT_ERROR;
        }
        rt_hw_interrupt_umask(irqnum);
        hcd->irq = irqnum;
            RT_DEBUG_LOG(rt_debug_usb, ("irq %d\n", irqnum));
    }
    return RT_EOK;
}

/**
 * usb_add_hcd - finish generic HCD structure initialization and register
 * @hcd: the usb_hcd structure to initialize
 * @irqnum: Interrupt line to allocate
 * @irqflags: Interrupt type flags
 *
 * Finish the remaining parts of generic HCD initialization: allocate the
 * buffers of consistent memory, register the bus, request the IRQ line,
 * and call the driver's reset() and start() routines.
 */
int usb_add_hcd(struct usb_hcd *hcd, unsigned int irqnum)
{
    int retval;
    struct usb_device *rhdev;

    RT_DEBUG_LOG(rt_debug_usb, ("%s\n", hcd->product_desc));
    hcd->wireless = 0;
    set_bit(HCD_FLAG_HW_ACCESSIBLE, &hcd->flags);

    /* HC is in reset state, but accessible.  Now do the one-time init,
     * bottom up so that hcds can customize the root hubs before khubd
     * starts talking to them.  (Note, bus id is assigned early too.)
     */
    if ((retval = usb_register_bus(&hcd->self)) < 0)
    {
        RT_DEBUG_LOG(rt_debug_usb, ("usb_register_bus failed:%d\n", retval));
        return retval;
        }
    if ((rhdev = usb_alloc_dev(RT_NULL, &hcd->self, 0)) == RT_NULL)
    {
        RT_DEBUG_LOG(rt_debug_usb, ("unable to allocate root hub:%d\n", retval));
        retval = -ENOMEM;
        return retval;
    }
    hcd->self.root_hub = rhdev;

    switch (hcd->speed)
    {
    case HCD_USB11:
        rhdev->speed = USB_SPEED_FULL;
        break;
    case HCD_USB2:
        rhdev->speed = USB_SPEED_HIGH;
        break;
    case HCD_USB3:
        rhdev->speed = USB_SPEED_SUPER;
        break;
    default:
        retval = -EINVAL;
        RT_DEBUG_LOG(rt_debug_usb, ("hcd speed error\n"));
        return retval;
    }


    /* HCD_FLAG_RH_RUNNING doesn't matter until the root hub is
     * registered.  But since the controller can die at any time,
     * let's initialize the flag before touching the hardware.
     */
    set_bit(HCD_FLAG_RH_RUNNING, &hcd->flags);

    /* "reset" is misnamed; its role is now one-time init. the controller
     * should already have been reset (and boot firmware kicked off etc).
     */
    if (hcd->driver->reset && (retval = hcd->driver->reset(hcd)) < 0)
    {
        RT_DEBUG_LOG(rt_debug_usb, ("can't setup:%d\n", retval));
        return retval;
    }
    hcd->rh_pollable = 1;

    /* enable irqs just before we start the controller */
        retval = usb_hcd_request_irqs(hcd, irqnum);
        if (retval)
        {
            RT_DEBUG_LOG(rt_debug_usb, ("usb_hcd_request_irqs error:%d\n", retval));
            return retval;
        }


    hcd->state = HC_STATE_RUNNING;
    retval = hcd->driver->start(hcd);
    if (retval < 0)
    {
        RT_DEBUG_LOG(rt_debug_usb, ("startup error %d\n", retval));
        return retval;
    }

    /* starting here, usbcore will pay attention to this root hub */

    rhdev->bus_mA = 0;
    /* rhdev->bus_mA = min(500u, hcd->power_budget); */
    if ((retval = register_root_hub(hcd)) != 0)
    {
        RT_DEBUG_LOG(rt_debug_usb, ("register_root_hub error %d\n", retval));
        return retval;
    }

/* if (hcd->uses_new_polling && HCD_POLL_RH(hcd)) */
/* usb_hcd_poll_rh_status(hcd); */
     rt_timer_start(hcd->rh_timer);

    return retval;

}

void usb_remove_hcd(struct usb_hcd *hcd)
{
    struct usb_device *rhdev = hcd->self.root_hub;

    clear_bit(HCD_FLAG_RH_RUNNING, &hcd->flags);
    if (HC_IS_RUNNING (hcd->state))
        hcd->state = HC_STATE_QUIESCING;

    hcd->rh_registered = 0;

    usb_disconnect(&rhdev); 	/* Sets rhdev to NULL */

    hcd->rh_pollable = 0;
    clear_bit(HCD_FLAG_POLL_RH, &hcd->flags);

    hcd->driver->stop(hcd);
    hcd->state = HC_STATE_HALT;

    usb_deregister_bus(&hcd->self);
    /* already free in case: usb_disconnect-->usb_release_dev */
    hcd->self.root_hub = NULL;
    /* already free in case: usb_rh_urb_dequeue-->rt_timer_delete */
    hcd->rh_timer = rt_timer_create("hcd_timer", rh_timer_func,
                    (void *) hcd, 10, RT_TIMER_FLAG_PERIODIC);
}
