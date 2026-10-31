#include <usb.h>
#include <rtdef.h>
#include <rtthread.h>
#include <rtdevice.h>
#include <rthw.h>
#include "usb_errno.h"
#include <dma_mem.h>
#include <interrupt.h>
#include <sys/types.h>
#include <sys/time.h>

/*-------------------------------------------------------------------------*/

/* FIXME make these public somewhere; usbdevfs.h? */
struct usbtest_param
{
    /* inputs */
    unsigned int test_num;    /* 0..(TEST_CASES-1) */
    unsigned int iterations;
    unsigned int length;
    unsigned int vary;
    unsigned int sglen;

    /* outputs */
    struct timeval        duration;
};
/* #define USBTEST_REQUEST    _IOWR('U', 100, struct usbtest_param) */

/*-------------------------------------------------------------------------*/

#define    GENERIC        /* let probe() bind using module params */

/* Some devices that can be used for testing will have "real" drivers.
 * Entries for those need to be enabled here by hand, after disabling
 * that "real" driver.
 */
/* #define    IBOT2         */
/* #define    KEYSPAN_19Qi     */

/*-------------------------------------------------------------------------*/

struct usbtest_info
{
    const char        *name;
    rt_uint8_t            ep_in;        /* bulk/intr source */
    rt_uint8_t            ep_out;        /* bulk/intr sink */
    unsigned        autoconf:1;
    unsigned        ctrl_out:1;
    unsigned        iso:1;        /* try iso in/out */
    int            alt;
};

/* this is accessed only through usbfs ioctl calls.
 * one ioctl to issue a test ... one lock per device.
 * tests create other threads if they need them.
 * urbs and buffers are allocated dynamically,
 * and data generated deterministically.
 */
struct usbtest_dev
{
    struct usb_interface    *intf;
    struct usbtest_info    *info;
    int            in_pipe;
    int            out_pipe;
    int            in_iso_pipe;
    int            out_iso_pipe;
    struct usb_endpoint_descriptor    *iso_in, *iso_out;
    struct rt_mutex        lock;

#define TBUF_SIZE    256
    rt_uint8_t            *buf;
};

static struct usb_device *testdev_to_usbdev(struct usbtest_dev *test)
{
        return test->intf->usb_dev;
}

/* set up all urbs so they can be used with either bulk or interrupt */
#define    INTERRUPT_RATE        1    /* msec/transfer */

#define ERROR(tdev, fmt, args...) \
    rt_kprintf(fmt, ## args)
#define WARNING(tdev, fmt, args...) \
    rt_kprintf(fmt, ## args)

#define GUARD_BYTE    0xA5

struct rt_mailbox g_mb_usb_test;
struct urb *g_usb_test_pool[4];
static rt_thread_t usb_test = RT_NULL;

/*-------------------------------------------------------------------------*/

static int
get_endpoints(struct usbtest_dev *dev, struct usb_interface *intf)
{
    int                tmp;
    struct usb_host_interface    *alt;
    struct usb_host_endpoint    *in, *out;
    struct usb_host_endpoint    *iso_in, *iso_out;
    struct usb_device        *udev;

    for (tmp = 0; tmp < intf->num_altsetting; tmp++)
    {
        unsigned int ep;

        in = out = NULL;
        iso_in = iso_out = NULL;
        alt = intf->altsetting + tmp;

        /* take the first altsetting with in-bulk + out-bulk;
         * ignore other endpoints and altsettings.
         */
        for (ep = 0; ep < alt->desc.bNumEndpoints; ep++)
        {
            struct usb_host_endpoint    *e;

            e = alt->endpoint + ep;
            switch (e->desc.bmAttributes)
            {
            case USB_ENDPOINT_XFER_BULK:
                break;
            case USB_ENDPOINT_XFER_ISOC:
                if (dev->info->iso)
                    goto try_iso;
                /* FALLTHROUGH */
            default:
                continue;
            }
            if (usb_endpoint_dir_in(&e->desc))
            {
                if (!in)
                    in = e;
            } else
            {
                if (!out)
                    out = e;
            }
            continue;
try_iso:
            if (usb_endpoint_dir_in(&e->desc))
            {
                if (!iso_in)
                    iso_in = e;
            } else
            {
                if (!iso_out)
                    iso_out = e;
            }
        }
        if ((in && out)  ||  iso_in || iso_out)
            goto found;
    }
    return -EINVAL;

found:
    udev = testdev_to_usbdev(dev);
    if (alt->desc.bAlternateSetting != 0)
    {
        tmp = usb_set_interface(udev,
                alt->desc.bInterfaceNumber,
                alt->desc.bAlternateSetting);
        if (tmp < 0)
            return tmp;
    }

    if (in)
    {
        dev->in_pipe = usb_rcvbulkpipe(udev,
            in->desc.bEndpointAddress & USB_ENDPOINT_NUMBER_MASK);
        dev->out_pipe = usb_sndbulkpipe(udev,
            out->desc.bEndpointAddress & USB_ENDPOINT_NUMBER_MASK);
    }
    if (iso_in)
    {
        dev->iso_in = &iso_in->desc;
        dev->in_iso_pipe = usb_rcvisocpipe(udev,
                iso_in->desc.bEndpointAddress
                    & USB_ENDPOINT_NUMBER_MASK);
    }

    if (iso_out)
    {
        dev->iso_out = &iso_out->desc;
        dev->out_iso_pipe = usb_sndisocpipe(udev,
                iso_out->desc.bEndpointAddress
                    & USB_ENDPOINT_NUMBER_MASK);
    }
    return 0;
}

/*-------------------------------------------------------------------------*/

/* Support for testing basic non-queued I/O streams.
 *
 * These just package urbs as requests that can be easily canceled.
 * Each urb's data buffer is dynamically allocated; callers can fill
 * them with non-zero test data (or test for it) when appropriate.
 */

static void simple_callback(struct urb *urb)
{
    rt_completion_done(urb->context);
}

static struct urb *usbtest_alloc_urb(
    struct usb_device    *udev,
    int            pipe,
    unsigned long        bytes,
    unsigned int transfer_flags,
    unsigned int offset)
{
    struct urb        *urb;

    urb = usb_alloc_urb(0, 0);
    if (!urb)
        return urb;
    usb_fill_bulk_urb(urb, udev, pipe, NULL, bytes, simple_callback, NULL);
    urb->interval = (udev->speed == USB_SPEED_HIGH)
            ? (INTERRUPT_RATE << 3)
            : INTERRUPT_RATE;
    urb->transfer_flags = transfer_flags;
    if (usb_pipein(pipe))
        urb->transfer_flags |= URB_SHORT_NOT_OK;

        urb->transfer_buffer = usb_dma_buffer_alloc(bytes + offset, udev->bus);

    if (!urb->transfer_buffer)
    {
        usb_free_urb(urb);
        return NULL;
    }

    /* To test unaligned transfers add an offset and fill the
        unused memory with a guard value */
    if (offset)
    {
        rt_memset(urb->transfer_buffer, GUARD_BYTE, offset);
        urb->transfer_buffer += offset;
        rt_kprintf("transfer_buffer:%x\n", urb->transfer_buffer);
        /* if (urb->transfer_flags & URB_NO_TRANSFER_DMA_MAP) */
        /* urb->transfer_dma += offset; */
    }

    /* For inbound transfers use guard byte so that test fails if
        data not correctly copied */
    rt_memset(urb->transfer_buffer,
            usb_pipein(urb->pipe) ? GUARD_BYTE : 0,
            bytes);
    return urb;
}

static struct urb *simple_alloc_urb(
    struct usb_device    *udev,
    int            pipe,
    unsigned long        bytes)
{
    return usbtest_alloc_urb(udev, pipe, bytes, URB_NO_TRANSFER_DMA_MAP, 0);
}

static unsigned pattern = 0;
/* static unsigned mod_pattern; */

static inline void simple_fill_buf(struct urb *urb)
{
    unsigned int i;
    rt_uint8_t        *buf = urb->transfer_buffer;
    unsigned int len = urb->transfer_buffer_length;

    switch (pattern)
    {
    default:
        /* FALLTHROUGH */
    case 0:
        rt_memset(buf, 0, len);
        break;
    case 1:            /* mod63 */
        for (i = 0; i < len; i++)
            *buf++ = (rt_uint8_t) (i % 63);
        break;
    }
}

static inline unsigned long buffer_offset(void *buf)
{
    return (unsigned long)buf & (32 - 1);
}

static int check_guard_bytes(struct usbtest_dev *tdev, struct urb *urb)
{
    rt_uint8_t *buf = urb->transfer_buffer;
    rt_uint8_t *guard = buf - buffer_offset(buf);
    unsigned int i;

    for (i = 0; guard < buf; i++, guard++)
    {
        if (*guard != GUARD_BYTE)
        {
            ERROR(tdev, "guard byte[%d] %d (not %d)\n",
                i, *guard, GUARD_BYTE);
            return -EINVAL;
        }
    }
    return 0;
}

static int simple_check_buf(struct usbtest_dev *tdev, struct urb *urb)
{
    unsigned int i;
    rt_uint8_t        expected;
    rt_uint8_t        *buf = urb->transfer_buffer;
    unsigned int len = urb->actual_length;

    int ret = check_guard_bytes(tdev, urb);

    if (ret)
        return ret;

    for (i = 0; i < len; i++, buf++)
    {
        switch (pattern)
        {
        /* all-zeroes has no synchronization issues */
        case 0:
            expected = 0;
            break;
        /* mod63 stays in sync with short-terminated transfers,
         * or otherwise when host and gadget agree on how large
         * each usb transfer request should be.  resync is done
         * with set_interface or set_config.
         */
        case 1:            /* mod63 */
            expected = i % 63;
            break;
        /* always fail unsupported patterns */
        default:
            expected = !*buf;
            break;
        }
        if (*buf == expected)
            continue;
        ERROR(tdev, "buf[%d] = %d (not %d)\n", i, *buf, expected);
        return -EINVAL;
    }
    return 0;
}

static void simple_free_urb(struct urb *urb)
{
    unsigned long offset = buffer_offset(urb->transfer_buffer);
    usb_dma_buffer_free(urb->transfer_buffer - offset, urb->dev->bus);
    usb_free_urb(urb);
}

static int simple_io(
    struct usbtest_dev    *tdev,
    struct urb        *urb,
    int            iterations,
    int            vary,
    int            expected,
    const char        *label
)
{
    struct usb_device    *udev = urb->dev;
    int            max = urb->transfer_buffer_length;
    struct rt_completion    completion;
    int            retval = 0;

    urb->context = &completion;
    while (retval == 0 && iterations-- > 0)
    {
        rt_completion_init(&completion);
        if (usb_pipeout(urb->pipe))
            simple_fill_buf(urb);
        retval = usb_submit_urb(urb, 0);
        if (retval != 0)
            break;

        /* NOTE:  no timeouts; can't be broken out of by interrupt */
        rt_completion_wait(&completion, -1);
        retval = urb->status;
        urb->dev = udev;
        if (retval == 0 && usb_pipein(urb->pipe))
            retval = simple_check_buf(tdev, urb);

        if (vary)
        {
            int    len = urb->transfer_buffer_length;

            len += vary;
            len %= max;
            if (len == 0)
                len = (vary < max) ? vary : max;
            urb->transfer_buffer_length = len;
        }

        /* FIXME if endpoint halted, clear halt (and log) */
    }
    urb->transfer_buffer_length = max;

    if (expected != retval)
        rt_kprintf(
            "%s failed, iterations left %d, status %d (not %d)\n",
                label, iterations, retval, expected);
    return retval;
}


/*-------------------------------------------------------------------------*/

/* We use scatterlist primitives to test queued I/O.
 * Yes, this also tests the scatterlist primitives.
 */
#if 0
static void free_sglist(struct scatterlist *sg, int nents)
{
    unsigned int i;

    if (!sg)
        return;
    for (i = 0; i < nents; i++)
    {
        if (!sg_page(&sg[i]))
            continue;
        rt_free(sg_virt(&sg[i]));
    }
    rt_free(sg);
}

static struct scatterlist *
alloc_sglist(int nents, int max, int vary)
{
    struct scatterlist    *sg;
    unsigned int i;
    unsigned int size = max;

    sg = rt_malloc(nents * sizeof(*sg));
    if (!sg)
        return NULL;
    sg_init_table(sg, nents);

    for (i = 0; i < nents; i++)
    {
        char        *buf;
        unsigned int j;

        buf = rt_malloc(size);
        if (!buf)
        {
            free_sglist(sg, i);
            return NULL;
        }

        /* kmalloc pages are always physically contiguous! */
        sg_set_buf(&sg[i], buf, size);

        switch (pattern)
        {
        case 0:
            /* already zeroed */
            break;
        case 1:
            for (j = 0; j < size; j++)
                *buf++ = (rt_uint8_t) (j % 63);
            break;
        }

        if (vary)
        {
            size += vary;
            size %= max;
            if (size == 0)
                size = (vary < max) ? vary : max;
        }
    }

    return sg;
}

static int perform_sglist(
    struct usbtest_dev    *tdev,
    unsigned int iterations,
    int            pipe,
    struct usb_sg_request    *req,
    struct scatterlist    *sg,
    int            nents
)
{
    struct usb_device    *udev = testdev_to_usbdev(tdev);
    int            retval = 0;

    while (retval == 0 && iterations-- > 0)
    {
        retval = usb_sg_init(req, udev, pipe,
                (udev->speed == USB_SPEED_HIGH)
                    ? (INTERRUPT_RATE << 3)
                    : INTERRUPT_RATE,
                sg, nents, 0, GFP_KERNEL);

        if (retval)
            break;
        usb_sg_wait(req);
        retval = req->status;

        /* FIXME check resulting data pattern */

        /* FIXME if endpoint halted, clear halt (and log) */
    }

    /* FIXME for unlink or fault handling tests, don't report
     * failure if retval is as we expected ...
     */
    if (retval)
        ERROR(tdev, "perform_sglist failed, iterations left %d, status %d\n",
                iterations, retval);
    return retval;
}

#endif
/*-------------------------------------------------------------------------*/

/* unqueued control message testing
 *
 * there's a nice set of device functional requirements in chapter 9 of the
 * usb 2.0 spec, which we can apply to ANY device, even ones that don't use
 * special test firmware.
 *
 * we know the device is configured (or suspended) by the time it's visible
 * through usbfs.  we can't change that, so we won't test enumeration (which
 * worked 'well enough' to get here, this time), power management (ditto),
 * or remote wakeup (which needs human interaction).
 */

static unsigned int realworld = 1;

static int get_altsetting(struct usbtest_dev *dev)
{
    struct usb_interface    *iface = dev->intf;
    struct usb_device    *udev = iface->usb_dev;
    int            retval;

    retval = usb_control_msg(udev, usb_rcvctrlpipe(udev, 0),
            USB_REQ_GET_INTERFACE, USB_DIR_IN|USB_RECIP_INTERFACE,
            0, iface->altsetting[0].desc.bInterfaceNumber,
            dev->buf, 1, USB_CTRL_GET_TIMEOUT);
    switch (retval)
    {
    case 1:
        return dev->buf[0];
    case 0:
        retval = -ERANGE;
        /* FALLTHROUGH */
    default:
        return retval;
    }
}

static int set_altsetting(struct usbtest_dev *dev, int alternate)
{
    struct usb_interface        *iface = dev->intf;
    struct usb_device        *udev;

    if (alternate < 0 || alternate >= 256)
        return -EINVAL;

    udev = iface->usb_dev;
    return usb_set_interface(udev,
            iface->altsetting[0].desc.bInterfaceNumber,
            alternate);
}

static int is_good_config(struct usbtest_dev *tdev, int len)
{
    struct usb_config_descriptor    *config;

    if (len < sizeof(*config))
        return 0;
    config = (struct usb_config_descriptor *) tdev->buf;

    switch (config->bDescriptorType)
    {
    case USB_DT_CONFIG:
    case USB_DT_OTHER_SPEED_CONFIG:
        if (config->bLength != 9)
        {
            ERROR(tdev, "bogus config descriptor length\n");
            return 0;
        }
        /* this bit 'must be 1' but often isn't */
        if (!realworld && !(config->bmAttributes & 0x80))
        {
            ERROR(tdev, "high bit of config attributes not set\n");
            return 0;
        }
        if (config->bmAttributes & 0x1f)    /* reserved == 0 */
        {
            ERROR(tdev, "reserved config bits set\n");
            return 0;
        }
        break;
    default:
        return 0;
    }

    if ((config->wTotalLength) == len)    /* read it all */
        return 1;
    if ((config->wTotalLength) >= TBUF_SIZE)    /* max partial read */
        return 1;
    ERROR(tdev, "bogus config descriptor read size\n");
    return 0;
}

/* sanity test for standard requests working with usb_control_mesg() and some
 * of the utility functions which use it.
 *
 * this doesn't test how endpoint halts behave or data toggles get set, since
 * we won't do I/O to bulk/interrupt endpoints here (which is how to change
 * halt or toggle).  toggle testing is impractical without support from hcds.
 *
 * this avoids failing devices linux would normally work with, by not testing
 * config/altsetting operations for devices that only support their defaults.
 * such devices rarely support those needless operations.
 *
 * NOTE that since this is a sanity test, it's not examining boundary cases
 * to see if usbcore, hcd, and device all behave right.  such testing would
 * involve varied read sizes and other operation sequences.
 */
static int ch9_postconfig(struct usbtest_dev *dev)
{
    struct usb_interface    *iface = dev->intf;
    struct usb_device    *udev = iface->usb_dev;
    int            i, alt, retval;

    /* [9.2.3] if there's more than one altsetting, we need to be able to
     * set and get each one.  mostly trusts the descriptors from usbcore.
     */
    for (i = 0; i < iface->num_altsetting; i++)
    {

        /* 9.2.3 constrains the range here */
        alt = iface->altsetting[i].desc.bAlternateSetting;
        if (alt < 0 || alt >= iface->num_altsetting)
        {
            rt_kprintf(
                    "invalid alt [%d].bAltSetting = %d\n",
                    i, alt);
        }

        /* [real world] get/set unimplemented if there's only one */
        if (realworld && iface->num_altsetting == 1)
            continue;

        /* [9.4.10] set_interface */
        retval = set_altsetting(dev, alt);
        if (retval)
        {
            rt_kprintf("can't set_interface = %d, %d\n",
                    alt, retval);
            return retval;
        }

        /* [9.4.4] get_interface always works */
        retval = get_altsetting(dev);
        if (retval != alt)
        {
            rt_kprintf("get alt should be %d, was %d\n",
                    alt, retval);
            return (retval < 0) ? retval : -EDOM;
        }

    }

    /* [real world] get_config unimplemented if there's only one */
    if (!realworld || udev->descriptor.bNumConfigurations != 1)
    {
        int    expected = udev->actconfig->desc.bConfigurationValue;

        /* [9.4.2] get_configuration always works
         * ... although some cheap devices (like one TI Hub I've got)
         * won't return config descriptors except before set_config.
         */
        retval = usb_control_msg(udev, usb_rcvctrlpipe(udev, 0),
                USB_REQ_GET_CONFIGURATION,
                USB_DIR_IN | USB_RECIP_DEVICE,
                0, 0, dev->buf, 1, USB_CTRL_GET_TIMEOUT);
        if (retval != 1 || dev->buf[0] != expected)
        {
            rt_kprintf("get config --> %d %d (1 %d)\n",
                retval, dev->buf[0], expected);
            return (retval < 0) ? retval : -EDOM;
        }
    }

    /* there's always [9.4.3] a device descriptor [9.6.1] */
    retval = usb_get_descriptor(udev, USB_DT_DEVICE, 0,
            dev->buf, sizeof(udev->descriptor));
    if (retval != sizeof(udev->descriptor))
    {
        rt_kprintf("dev descriptor --> %d\n", retval);
        return (retval < 0) ? retval : -EDOM;
    }

    /* there's always [9.4.3] at least one config descriptor [9.6.3] */
    for (i = 0; i < udev->descriptor.bNumConfigurations; i++)
    {
        retval = usb_get_descriptor(udev, USB_DT_CONFIG, i,
                dev->buf, TBUF_SIZE);
        if (!is_good_config(dev, retval))
        {
            rt_kprintf(
                    "config [%d] descriptor --> %d\n",
                    i, retval);
            return (retval < 0) ? retval : -EDOM;
        }

        /* FIXME cross-checking udev->config[i] to make sure usbcore
         * parsed it right (etc) would be good testing paranoia
         */
    }

    /* and sometimes [9.2.6.6] speed dependent descriptors */
    if ((udev->descriptor.bcdUSB) == 0x0200)
    {
        struct usb_qualifier_descriptor *d = NULL;

        /* device qualifier [9.6.2] */
        retval = usb_get_descriptor(udev,
                USB_DT_DEVICE_QUALIFIER, 0, dev->buf,
                sizeof(struct usb_qualifier_descriptor));
        if (retval == -EPIPE)
        {
            if (udev->speed == USB_SPEED_HIGH)
            {
                rt_kprintf(
                        "hs dev qualifier --> %d\n",
                        retval);
                return (retval < 0) ? retval : -EDOM;
            }
            /* usb2.0 but not high-speed capable; fine */
        } else if (retval != sizeof(struct usb_qualifier_descriptor))
        {
            rt_kprintf("dev qualifier --> %d\n", retval);
            return (retval < 0) ? retval : -EDOM;
        }
        else
            d = (struct usb_qualifier_descriptor *) dev->buf;

        /* might not have [9.6.2] any other-speed configs [9.6.4] */
        if (d)
        {
            unsigned int max = d->bNumConfigurations;

            for (i = 0; i < max; i++)
            {
                retval = usb_get_descriptor(udev,
                    USB_DT_OTHER_SPEED_CONFIG, i,
                    dev->buf, TBUF_SIZE);
                if (!is_good_config(dev, retval))
                {
                    rt_kprintf(
                        "other speed config --> %d\n",
                        retval);
                    return (retval < 0) ? retval : -EDOM;
                }
            }
        }
    }
    /* FIXME fetch strings from at least the device descriptor */

    /* [9.4.5] get_status always works */
    retval = usb_get_status(udev, USB_RECIP_DEVICE, 0, dev->buf);
    if (retval != 2)
    {
        rt_kprintf("get dev status --> %d\n", retval);
        return (retval < 0) ? retval : -EDOM;
    }

    /* FIXME configuration.bmAttributes says if we could try to set/clear
     * the device's remote wakeup feature ... if we can, test that here
     */

    retval = usb_get_status(udev, USB_RECIP_INTERFACE,
            iface->altsetting[0].desc.bInterfaceNumber, dev->buf);
    if (retval != 2)
    {
        rt_kprintf("get interface status --> %d\n", retval);
        return (retval < 0) ? retval : -EDOM;
    }
    /* FIXME get status for each endpoint in the interface */

    return 0;
}

/*-------------------------------------------------------------------------*/

/* use ch9 requests to test whether:
 *   (a) queues work for control, keeping N subtests queued and
 *       active (auto-resubmit) for M loops through the queue.
 *   (b) protocol stalls (control-only) will autorecover.
 *       it's not like bulk/intr; no halt clearing.
 *   (c) short control reads are reported and handled.
 *   (d) queues are always processed in-order
 */

struct ctrl_ctx
{
/* spinlock_t        lock; */
    struct usbtest_dev    *dev;
    struct rt_completion    complete;
    unsigned int count;
    unsigned int pending;
    int            status;
    struct urb        **urb;
    struct usbtest_param    *param;
    int            last;
};

#define NUM_SUBCASES    15        /* how many test subcases here? */

struct subcase
{
    struct usb_ctrlrequest    setup;
    int            number;
    int            expected;
};

static void ctrl_complete(struct urb *urb)
{
    struct ctrl_ctx        *ctx = urb->context;
    struct usb_ctrlrequest    *reqp;
    struct subcase        *subcase;
    int            status = urb->status;

    reqp = (struct usb_ctrlrequest *)urb->setup_packet;
    subcase = container_of(reqp, struct subcase, setup);

    /* spin_lock(&ctx->lock); */
    rt_enter_critical();
    ctx->count--;
    ctx->pending--;

    /* queue must transfer and complete in fifo order, unless
     * usb_unlink_urb() is used to unlink something not at the
     * physical queue head (not tested).
     */
    if (subcase->number > 0)
    {
        if ((subcase->number - ctx->last) != 1)
        {
            ERROR(ctx->dev,
                "subcase %d completed out of order, last %d\n",
                subcase->number, ctx->last);
            status = -EDOM;
            ctx->last = subcase->number;
            goto error;
        }
    }
    ctx->last = subcase->number;

    /* succeed or fault in only one way? */
    if (status == subcase->expected)
        status = 0;

    /* async unlink for cleanup? */
    else if (status != -ECONNRESET)
    {

        /* some faults are allowed, not required */
        if (subcase->expected > 0 && (
              ((status == -subcase->expected    /* happened */
               || status == 0))))            /* didn't */
            status = 0;
        /* sometimes more than one fault is allowed */
        else if (subcase->number == 12 && status == -EPIPE)
            status = 0;
        else
            ERROR(ctx->dev, "subtest %d error, status %d\n",
                    subcase->number, status);
    }

    /* unexpected status codes mean errors; ideally, in hardware */
    if (status)
    {
error:
        if (ctx->status == 0)
        {
            int        i;

            ctx->status = status;
            ERROR(ctx->dev, "control queue %02x.%02x, err %d, %d left, subcase %d, len %d/%d\n",
                    reqp->bRequestType, reqp->bRequest,
                    status, ctx->count, subcase->number,
                    urb->actual_length,
                    urb->transfer_buffer_length);

            /* FIXME this "unlink everything" exit route should
             * be a separate test case.
             */

            /* unlink whatever's still pending */
            for (i = 1; i < ctx->param->sglen; i++)
            {
                struct urb *u = ctx->urb[
                            (i + subcase->number)
                            % ctx->param->sglen];

                if (u == urb || !u->dev)
                    continue;
                /* spin_unlock(&ctx->lock); */
                rt_exit_critical();
                status = usb_unlink_urb(u);
                /* spin_lock(&ctx->lock); */
                rt_enter_critical();
                switch (status)
                {
                case -EINPROGRESS:
                case -EBUSY:
                case -EIDRM:
                    continue;
                default:
                    ERROR(ctx->dev, "urb unlink --> %d\n",
                            status);
                }
            }
            status = ctx->status;
        }
    }

    /* resubmit if we need to, else mark this as done */
    if ((status == 0) && (ctx->pending < ctx->count))
    {
        /* status = usb_submit_urb(urb, 0); */
        rt_mb_send(&g_mb_usb_test, (rt_uint32_t)urb);
        /* if (status != 0) { */
        /* ERROR(ctx->dev, */
        /* "can't resubmit ctrl %02x.%02x, err %d\n", */
        /* reqp->bRequestType, reqp->bRequest, status); */
        /* urb->dev = NULL; */
        /* } else */
        /* ctx->pending++; */
    }
    else
        urb->dev = NULL;

    /* signal completion when nothing's queued */
    if (ctx->pending == 0)
        rt_completion_done(&ctx->complete);
    /* spin_unlock(&ctx->lock); */
    rt_exit_critical();
}

static int
test_ctrl_queue(struct usbtest_dev *dev, struct usbtest_param *param)
{
    struct usb_device    *udev = testdev_to_usbdev(dev);
    struct urb        **urb;
    struct ctrl_ctx        context;
    int            i;
    /* rt_base_t level; */
    /* spin_lock_init(&context.lock); */
    context.dev = dev;
    rt_completion_init(&context.complete);
    context.count = param->sglen * param->iterations;
    context.pending = 0;
    context.status = -ENOMEM;
    context.param = param;
    context.last = -1;

    /* allocate and init the urbs we'll queue.
     * as with bulk/intr sglists, sglen is the queue depth; it also
     * controls which subtests run (more tests than sglen) or rerun.
     */
    urb = rt_malloc(param->sglen*sizeof(struct urb *));
    if (!urb)
        return -ENOMEM;
    for (i = 0; i < param->sglen; i++)
    {
        int            pipe = usb_rcvctrlpipe(udev, 0);
        unsigned int len;
        struct urb        *u;
        struct usb_ctrlrequest    req;
        struct subcase        *reqp;

        /* sign of this variable means:
         *  -: tested code must return this (negative) error code
         *  +: tested code may return this (negative too) error code
         */
        int            expected = 0;

        /* requests here are mostly expected to succeed on any
         * device, but some are chosen to trigger protocol stalls
         * or short reads.
         */
        rt_memset(&req, 0, sizeof(req));
        req.bRequest = USB_REQ_GET_DESCRIPTOR;
        req.bRequestType = USB_DIR_IN|USB_RECIP_DEVICE;

        switch (i % NUM_SUBCASES)
        {
        case 0:        /* get device descriptor */
            req.wValue = (USB_DT_DEVICE << 8);
            len = sizeof(struct usb_device_descriptor);
            break;
        case 1:        /* get first config descriptor (only) */
            req.wValue = ((USB_DT_CONFIG << 8) | 0);
            len = sizeof(struct usb_config_descriptor);
            break;
        case 2:        /* get altsetting (OFTEN STALLS) */
            req.bRequest = USB_REQ_GET_INTERFACE;
            req.bRequestType = USB_DIR_IN|USB_RECIP_INTERFACE;
            /* index = 0 means first interface */
            len = 1;
            expected = EPIPE;
            break;
        case 3:        /* get interface status */
            req.bRequest = USB_REQ_GET_STATUS;
            req.bRequestType = USB_DIR_IN|USB_RECIP_INTERFACE;
            /* interface 0 */
            len = 2;
            break;
        case 4:        /* get device status */
            req.bRequest = USB_REQ_GET_STATUS;
            req.bRequestType = USB_DIR_IN|USB_RECIP_DEVICE;
            len = 2;
            break;
        case 5:        /* get device qualifier (MAY STALL) */
            req.wValue =  (USB_DT_DEVICE_QUALIFIER << 8);
            len = sizeof(struct usb_qualifier_descriptor);
            if (udev->speed != USB_SPEED_HIGH)
                expected = EPIPE;
            break;
        case 6:        /* get first config descriptor, plus interface */
            req.wValue = ((USB_DT_CONFIG << 8) | 0);
            len = sizeof(struct usb_config_descriptor);
            len += sizeof(struct usb_interface_descriptor);
            break;
        case 7:        /* get interface descriptor (ALWAYS STALLS) */
            req.wValue =  (USB_DT_INTERFACE << 8);
            /* interface == 0 */
            len = sizeof(struct usb_interface_descriptor);
            expected = -EPIPE;
            break;
        /* NOTE: two consecutive stalls in the queue here.
         *  that tests fault recovery a bit more aggressively. */
        case 8:        /* clear endpoint halt (MAY STALL) */
            req.bRequest = USB_REQ_CLEAR_FEATURE;
            req.bRequestType = USB_RECIP_ENDPOINT;
            /* wValue 0 == ep halt */
            /* wIndex 0 == ep0 (shouldn't halt!) */
            len = 0;
            pipe = usb_sndctrlpipe(udev, 0);
            expected = EPIPE;
            break;
        case 9:        /* get endpoint status */
            req.bRequest = USB_REQ_GET_STATUS;
            req.bRequestType = USB_DIR_IN|USB_RECIP_ENDPOINT;
            /* endpoint 0 */
            len = 2;
            break;
        case 10:    /* trigger short read (EREMOTEIO) */
            req.wValue = ((USB_DT_CONFIG << 8) | 0);
            len = 1024;
            expected = -EREMOTEIO;
            break;
        /* NOTE: two consecutive _different_ faults in the queue. */
        case 11:    /* get endpoint descriptor (ALWAYS STALLS) */
            req.wValue = (USB_DT_ENDPOINT << 8);
            /* endpoint == 0 */
            len = sizeof(struct usb_interface_descriptor);
            expected = EPIPE;
            break;
        /* NOTE: sometimes even a third fault in the queue! */
        case 12:    /* get string 0 descriptor (MAY STALL) */
            req.wValue = (USB_DT_STRING << 8);
            /* string == 0, for language IDs */
            len = sizeof(struct usb_interface_descriptor);
            /* may succeed when > 4 languages */
            expected = EREMOTEIO;    /* or EPIPE, if no strings */
            break;
        case 13:    /* short read, resembling case 10 */
            req.wValue = ((USB_DT_CONFIG << 8) | 0);
            /* last data packet "should" be DATA1, not DATA0 */
            len = 1024 - udev->descriptor.bMaxPacketSize0;
            expected = -EREMOTEIO;
            break;
        case 14:    /* short read; try to fill the last packet */
            req.wValue = ((USB_DT_DEVICE << 8) | 0);
            /* device descriptor size == 18 bytes */
            len = udev->descriptor.bMaxPacketSize0;
            if (udev->speed == USB_SPEED_SUPER)
                len = 512;
            switch (len)
            {
            case 8:
                len = 24;
                break;
            case 16:
                len = 32;
                break;
            }
            expected = -EREMOTEIO;
            break;
        default:
            ERROR(dev, "bogus number of ctrl queue testcases!\n");
            context.status = -EINVAL;
            goto cleanup;
        }
        req.wLength = (len);
        urb[i] = u = simple_alloc_urb(udev, pipe, len);
        if (!u)
            goto cleanup;

        reqp = usb_dma_buffer_alloc(sizeof(*reqp), udev->bus);
        if (!reqp)
            goto cleanup;
        reqp->setup = req;
        reqp->number = i % NUM_SUBCASES;
        reqp->expected = expected;
        u->setup_packet = (unsigned char *) &reqp->setup;

        u->context = &context;
        u->complete = ctrl_complete;
    }

    /* queue the urbs */
    context.urb = urb;
    /* spin_lock_irq(&context.lock); */
    /* level=rt_hw_interrupt_disable(); */
    /* rt_enter_critical(); */
    for (i = 0; i < param->sglen; i++)
    {
        context.status = usb_submit_urb(urb[i], 0);
        if (context.status != 0)
        {
            ERROR(dev, "can't submit urb[%d], status %d\n",
                    i, context.status);
            context.count = context.pending;
            break;
        }
        context.pending++;
    }
    /* spin_unlock_irq(&context.lock); */
    /* rt_hw_interrupt_enable(level); */
    /* rt_exit_critical(); */
    /* FIXME  set timer and time out; provide a disconnect hook */

    /* wait for the last one to complete */
    if (context.pending > 0)
        rt_completion_wait(&context.complete, -1);

cleanup:
    for (i = 0; i < param->sglen; i++)
    {
        if (!urb[i])
            continue;
        urb[i]->dev = udev;
        usb_dma_buffer_free(urb[i]->setup_packet, udev->bus);
        simple_free_urb(urb[i]);
    }
    rt_free(urb);
    return context.status;
}
#undef NUM_SUBCASES


/*-------------------------------------------------------------------------*/

static void unlink1_callback(struct urb *urb)
{
    int    status = urb->status;

    /* we "know" -EPIPE (stall) never happens */
    if (!status)
    {
        rt_mb_send(&g_mb_usb_test, (rt_uint32_t)urb);
    /* status = usb_submit_urb(urb, 0); */
    }
    else
    {
        urb->status = status;
        rt_completion_done(urb->context);
    }
}

static int unlink1(struct usbtest_dev *dev, int pipe, int size, int async)
{
    struct urb        *urb;
    struct rt_completion    completion;
    int            retval = 0;

    rt_completion_init(&completion);
    urb = simple_alloc_urb(testdev_to_usbdev(dev), pipe, size);
    if (!urb)
        return -ENOMEM;
    urb->context = &completion;
    urb->complete = unlink1_callback;

    /* keep the endpoint busy.  there are lots of hc/hcd-internal
     * states, and testing should get to all of them over time.
     *
     * FIXME want additional tests for when endpoint is STALLing
     * due to errors, or is just NAKing requests.
     */
    retval = usb_submit_urb(urb, 0);
    if (retval != 0)
    {
        rt_kprintf("submit fail %d\n", retval);
        return retval;
    }

    /* unlinking that should always work.  variable delay tests more
     * hcd states and code paths, even with little other system load.
     */
    rt_thread_delay(1);
    if (async)
    {
        while (completion.flag == 0)
        {
            retval = usb_unlink_urb(urb);
            rt_kprintf("usb_unlink_urb\n");

            switch (retval)
            {
            case -EBUSY:
            case -EIDRM:
                /* we can't unlink urbs while they're completing
                 * or if they've completed, and we haven't
                 * resubmitted. "normal" drivers would prevent
                 * resubmission, but since we're testing unlink
                 * paths, we can't.
                 */
                ERROR(dev, "unlink retry\n");
                continue;
            case 0:
            case -EINPROGRESS:
                break;

            default:
                rt_kprintf(
                    "unlink fail %d\n", retval);
                return retval;
            }

        }
    }
    else
        usb_kill_urb(urb);

    rt_completion_wait(&completion, -1);
    retval = urb->status;
    simple_free_urb(urb);

    if (async)
        return (retval == -ECONNRESET) ? 0 : retval - 1000;
    else
        return (retval == -ENOENT || retval == -EPERM) ?
                0 : retval - 2000;
}

static int unlink_simple(struct usbtest_dev *dev, int pipe, int len)
{
    int            retval = 0;

    /* test sync and async paths */
    retval = unlink1(dev, pipe, len, 1);
    if (!retval)
        retval = unlink1(dev, pipe, len, 0);
    return retval;
}

/*-------------------------------------------------------------------------*/

struct queued_ctx
{
    struct rt_completion    complete;
    int        pending;
    unsigned int num;
    int            status;
    struct urb        **urbs;
};

int atomic_dec_and_test(int *v)
{
    v--;
    return v == 0;
}
static void unlink_queued_callback(struct urb *urb)
{
    int            status = urb->status;
    struct queued_ctx    *ctx = urb->context;

    if (ctx->status)
        goto done;
    if (urb == ctx->urbs[ctx->num - 4] || urb == ctx->urbs[ctx->num - 2])
    {
        if (status == -ECONNRESET)
            goto done;
        /* What error should we report if the URB completed normally? */
    }
    if (status != 0)
        ctx->status = status;

 done:
    if (atomic_dec_and_test(&ctx->pending))
        rt_completion_done(&ctx->complete);
}

static int unlink_queued(struct usbtest_dev *dev, int pipe, unsigned int num,
        unsigned int size)
{
    struct queued_ctx    ctx;
    struct usb_device    *udev = testdev_to_usbdev(dev);
    void            *buf;
    /* unsigned int    buf_dma; */
    int            i;
    int            retval = -ENOMEM;

    rt_completion_init(&ctx.complete);
    ctx.pending = 1;    /* One more than the actual value */
    ctx.num = num;
    ctx.status = 0;

    buf = rt_malloc(size);
    if (!buf)
        return retval;
    rt_memset(buf, 0, size);

    /* Allocate and init the urbs we'll queue */
    ctx.urbs = rt_malloc(num*sizeof(struct urb *));
    if (!ctx.urbs)
        goto free_buf;
    rt_memset(ctx.urbs, 0, num*sizeof(struct urb *));
    for (i = 0; i < num; i++)
    {
        ctx.urbs[i] = usb_alloc_urb(0, 0);
        if (!ctx.urbs[i])
            goto free_urbs;
        usb_fill_bulk_urb(ctx.urbs[i], udev, pipe, buf, size,
                unlink_queued_callback, &ctx);
        ctx.urbs[i]->transfer_dma = (unsigned int) buf;
        ctx.urbs[i]->transfer_flags = URB_NO_TRANSFER_DMA_MAP;
    }

    /* Submit all the URBs and then unlink URBs num - 4 and num - 2. */
    for (i = 0; i < num; i++)
    {
        ctx.pending++;
        retval = usb_submit_urb(ctx.urbs[i], 0);
        if (retval != 0)
        {
            rt_kprintf("submit urbs[%d] fail %d\n",
                    i, retval);
            ctx.pending--;
            ctx.status = retval;
            break;
        }
    }
    if (i == num)
    {
        usb_unlink_urb(ctx.urbs[num - 4]);
        usb_unlink_urb(ctx.urbs[num - 2]);
    } else
    {
        while (--i >= 0)
            usb_unlink_urb(ctx.urbs[i]);
    }

    if (atomic_dec_and_test(&ctx.pending))        /* The extra count */
        rt_completion_done(&ctx.complete);
    rt_completion_wait(&ctx.complete, -1);
    retval = ctx.status;

 free_urbs:
    for (i = 0; i < num; i++)
        usb_free_urb(ctx.urbs[i]);
    rt_free(ctx.urbs);
 free_buf:
    rt_free(buf);
    return retval;
}

/*-------------------------------------------------------------------------*/

static int verify_not_halted(struct usbtest_dev *tdev, int ep, struct urb *urb)
{
    int    retval;
    rt_uint16_t    status;

    /* shouldn't look or act halted */
    retval = usb_get_status(urb->dev, USB_RECIP_ENDPOINT, ep, &status);
    if (retval < 0)
    {
        ERROR(tdev, "ep %02x couldn't get no-halt status, %d\n",
                ep, retval);
        return retval;
    }
    if (status != 0)
    {
        ERROR(tdev, "ep %02x bogus status: %04x != 0\n", ep, status);
        return -EINVAL;
    }
    retval = simple_io(tdev, urb, 1, 0, 0, __func__);
    if (retval != 0)
        return -EINVAL;
    return 0;
}

static int verify_halted(struct usbtest_dev *tdev, int ep, struct urb *urb)
{
    int    retval;
    rt_uint16_t    status;

    /* should look and act halted */
    retval = usb_get_status(urb->dev, USB_RECIP_ENDPOINT, ep, &status);
    if (retval < 0)
    {
        ERROR(tdev, "ep %02x couldn't get halt status, %d\n",
                ep, retval);
        return retval;
    }
    /* le16_to_cpus(&status); */
    if (status != 1)
    {
        ERROR(tdev, "ep %02x bogus status: %04x != 1\n", ep, status);
        return -EINVAL;
    }
    retval = simple_io(tdev, urb, 1, 0, -EPIPE, __func__);
    if (retval != -EPIPE)
        return -EINVAL;
    retval = simple_io(tdev, urb, 1, 0, -EPIPE, "verify_still_halted");
    if (retval != -EPIPE)
        return -EINVAL;
    return 0;
}

static int test_halt(struct usbtest_dev *tdev, int ep, struct urb *urb)
{
    int    retval;

    /* shouldn't look or act halted now */
    retval = verify_not_halted(tdev, ep, urb);
    if (retval < 0)
        return retval;

    /* set halt (protocol test only), verify it worked */
    retval = usb_control_msg(urb->dev, usb_sndctrlpipe(urb->dev, 0),
            USB_REQ_SET_FEATURE, USB_RECIP_ENDPOINT,
            USB_ENDPOINT_HALT, ep,
            NULL, 0, USB_CTRL_SET_TIMEOUT);
    if (retval < 0)
    {
        ERROR(tdev, "ep %02x couldn't set halt, %d\n", ep, retval);
        return retval;
    }
    retval = verify_halted(tdev, ep, urb);
    if (retval < 0)
        return retval;

    /* clear halt (tests API + protocol), verify it worked */
    retval = usb_clear_halt(urb->dev, urb->pipe);
    if (retval < 0)
    {
        ERROR(tdev, "ep %02x couldn't clear halt, %d\n", ep, retval);
        return retval;
    }
    retval = verify_not_halted(tdev, ep, urb);
    if (retval < 0)
        return retval;

    /* NOTE:  could also verify SET_INTERFACE clear halts ... */

    return 0;
}

static int halt_simple(struct usbtest_dev *dev)
{
    int        ep;
    int        retval = 0;
    struct urb    *urb;

    urb = simple_alloc_urb(testdev_to_usbdev(dev), 0, 512);
    if (urb == NULL)
        return -ENOMEM;

    if (dev->in_pipe)
    {
        ep = usb_pipeendpoint(dev->in_pipe) | USB_DIR_IN;
        urb->pipe = dev->in_pipe;
        retval = test_halt(dev, ep, urb);
        if (retval < 0)
            goto done;
    }

    if (dev->out_pipe)
    {
        ep = usb_pipeendpoint(dev->out_pipe);
        urb->pipe = dev->out_pipe;
        retval = test_halt(dev, ep, urb);
    }
done:
    simple_free_urb(urb);
    return retval;
}

/*-------------------------------------------------------------------------*/

/* Control OUT tests use the vendor control requests from Intel's
 * USB 2.0 compliance test device:  write a buffer, read it back.
 *
 * Intel's spec only _requires_ that it work for one packet, which
 * is pretty weak.   Some HCDs place limits here; most devices will
 * need to be able to handle more than one OUT data packet.  We'll
 * try whatever we're told to try.
 */
static int ctrl_out(struct usbtest_dev *dev,
        unsigned int count, unsigned int length, unsigned int vary, unsigned int offset)
{
    unsigned int i, j, len;
    int            retval;
    rt_uint8_t            *buf;
    char            *what = "?";
    struct usb_device    *udev;

    if (length < 1 || length > 0xffff || vary >= length)
        return -EINVAL;

    buf = rt_malloc(length + offset);
    if (!buf)
        return -ENOMEM;

    buf += offset;
    udev = testdev_to_usbdev(dev);
    len = length;
    retval = 0;

    /* NOTE:  hardware might well act differently if we pushed it
     * with lots back-to-back queued requests.
     */
    for (i = 0; i < count; i++)
    {
        /* write patterned data */
        for (j = 0; j < len; j++)
            buf[j] = i + j;
        retval = usb_control_msg(udev, usb_sndctrlpipe(udev, 0),
                0x5b, USB_DIR_OUT|USB_TYPE_VENDOR,
                0, 0, buf, len, USB_CTRL_SET_TIMEOUT);
        if (retval != len)
        {
            what = "write";
            if (retval >= 0)
            {
                ERROR(dev, "ctrl_out, wlen %d (expected %d)\n",
                        retval, len);
                retval = -EBADMSG;
            }
            break;
        }

        /* read it back -- assuming nothing intervened!!  */
        retval = usb_control_msg(udev, usb_rcvctrlpipe(udev, 0),
                0x5c, USB_DIR_IN|USB_TYPE_VENDOR,
                0, 0, buf, len, USB_CTRL_GET_TIMEOUT);
        if (retval != len)
        {
            what = "read";
            if (retval >= 0)
            {
                ERROR(dev, "ctrl_out, rlen %d (expected %d)\n",
                        retval, len);
                retval = -EBADMSG;
            }
            break;
        }

        /* fail if we can't verify */
        for (j = 0; j < len; j++)
        {
            if (buf[j] != (rt_uint8_t) (i + j))
            {
                ERROR(dev, "ctrl_out, byte %d is %d not %d\n",
                    j, buf[j], (rt_uint8_t) i + j);
                retval = -EBADMSG;
                break;
            }
        }
        if (retval < 0)
        {
            what = "verify";
            break;
        }

        len += vary;

        /* [real world] the "zero bytes IN" case isn't really used.
         * hardware can easily trip up in this weird case, since its
         * status stage is IN, not OUT like other ep0in transfers.
         */
        if (len > length)
            len = realworld ? 1 : 0;
    }

    if (retval < 0)
        ERROR(dev, "ctrl_out %s failed, code %d, count %d\n",
            what, retval, i);

    rt_free(buf - offset);

    return retval >= 0 ? 0 : retval;

}

/*-------------------------------------------------------------------------*/

/* ISO tests ... mimics common usage
 *  - buffer length is split into N packets (mostly maxpacket sized)
 *  - multi-buffers according to sglen
 */

struct iso_context
{
    unsigned int count;
    unsigned int pending;
    /* spinlock_t        lock; */
    struct rt_completion    done;
    int            submit_error;
    unsigned long        errors;
    unsigned long        packet_count;
    struct usbtest_dev    *dev;
};

#if 0
static void iso_callback(struct urb *urb)
{
    struct iso_context    *ctx = urb->context;

    /* spin_lock(&ctx->lock); */
    rt_enter_critical();
    ctx->count--;

    ctx->packet_count += urb->number_of_packets;
    if (urb->error_count > 0)
        ctx->errors += urb->error_count;
    else if (urb->status != 0)
        ctx->errors += urb->number_of_packets;
    else if (urb->actual_length != urb->transfer_buffer_length)
        ctx->errors++;
    else if (check_guard_bytes(ctx->dev, urb) != 0)
        ctx->errors++;

    if (urb->status == 0 && ctx->count > (ctx->pending - 1)
            && !ctx->submit_error) {
        int status = usb_submit_urb(urb, 0);

        switch (status)
        {
        case 0:
            goto done;
        default:
            rt_kprintf(
                    "iso resubmit err %d\n",
                    status);
            /* FALLTHROUGH */
        case -ENODEV:            /* disconnected */
        case -ESHUTDOWN:        /* endpoint disabled */
            ctx->submit_error = 1;
            break;
        }
    }

    ctx->pending--;
    if (ctx->pending == 0)
    {
        if (ctx->errors)
            rt_kprintf(
                "iso test, %lu errors out of %lu\n",
                ctx->errors, ctx->packet_count);
        rt_completion_done(&ctx->done);
    }
done:
    rt_exit_critical();
}
#endif

#ifndef DIV_ROUND_UP
extern int DIV_ROUND_UP(
    int m,
    /*@sef@*/ int n); /* LINT : This tells lint that  the parameter must be
                         side-effect free. i.e. evaluation does not change any
                         values (since it is being evaulated more than once */
#define DIV_ROUND_UP(m, n) (((m) + (n)-1) / (n))
#endif /* ifndef DIV_ROUND_UP */

#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif
# if 0
static struct urb *iso_alloc_urb(
    struct usb_device    *udev,
    int            pipe,
    struct usb_endpoint_descriptor    *desc,
    long            bytes,
    unsigned offset
)
{
    struct urb        *urb;
    unsigned int i, maxp, packets;

    if (bytes < 0 || !desc)
        return NULL;
    maxp = 0x7ff & (desc->wMaxPacketSize);
    maxp *= 1 + (0x3 & ((desc->wMaxPacketSize) >> 11));
    packets = DIV_ROUND_UP(bytes, maxp);

    urb = usb_alloc_urb(packets, 0);
    if (!urb)
        return urb;
    urb->dev = udev;
    urb->pipe = pipe;

    urb->number_of_packets = packets;
    urb->transfer_buffer_length = bytes;
    urb->transfer_buffer = rt_malloc(bytes + offset);
    urb->transfer_dma = (unsigned int)urb->transfer_buffer;
    if (!urb->transfer_buffer)
    {
        usb_free_urb(urb);
        return NULL;
    }
    if (offset)
    {
        rt_memset(urb->transfer_buffer, GUARD_BYTE, offset);
        urb->transfer_buffer += offset;
        urb->transfer_dma += offset;
    }
    /* For inbound transfers use guard byte so that test fails if
        data not correctly copied */
    rt_memset(urb->transfer_buffer,
            usb_pipein(urb->pipe) ? GUARD_BYTE : 0,
            bytes);

    for (i = 0; i < packets; i++)
    {
        /* here, only the last packet will be short */
        urb->iso_frame_desc[i].length = min((unsigned int) bytes, maxp);
        bytes -= urb->iso_frame_desc[i].length;

        urb->iso_frame_desc[i].offset = maxp * i;
    }

    urb->complete = iso_callback;
    /* urb->context = SET BY CALLER */
    urb->interval = 1 << (desc->bInterval - 1);
    urb->transfer_flags = URB_ISO_ASAP | URB_NO_TRANSFER_DMA_MAP;
    return urb;
}
static int
test_iso_queue(struct usbtest_dev *dev, struct usbtest_param *param,
        int pipe, struct usb_endpoint_descriptor *desc, unsigned int offset)
{
    struct iso_context    context;
    struct usb_device    *udev;
    unsigned int i;
    unsigned long        packets = 0;
    int            status = 0;
    struct urb        *urbs[10];    /* FIXME no limit */
    rt_base_t level;

    if (param->sglen > 10)
        return -EDOM;

    rt_memset(&context, 0, sizeof(context));
    context.count = param->iterations * param->sglen;
    context.dev = dev;
    rt_completion_init(&context.done);
    /* spin_lock_init(&context.lock); */

    rt_memset(urbs, 0, sizeof(urbs));
    udev = testdev_to_usbdev(dev);
    rt_kprintf(
        "... iso period %d %sframes, wMaxPacket %04x\n",
        1 << (desc->bInterval - 1),
        (udev->speed == USB_SPEED_HIGH) ? "micro" : "",
        (desc->wMaxPacketSize));

    for (i = 0; i < param->sglen; i++)
    {
        urbs[i] = iso_alloc_urb(udev, pipe, desc,
                    param->length, offset);
        if (!urbs[i])
        {
            status = -ENOMEM;
            goto fail;
        }
        packets += urbs[i]->number_of_packets;
        urbs[i]->context = &context;
    }
    packets *= param->iterations;
    rt_kprintf(
        "... total %lu msec (%lu packets)\n",
        (packets * (1 << (desc->bInterval - 1)))
            / ((udev->speed == USB_SPEED_HIGH) ? 8 : 1),
        packets);

    /* spin_lock_irq(&context.lock); */
    level = rt_hw_interrupt_disable();
        rt_enter_critical();
    for (i = 0; i < param->sglen; i++)
    {
        ++context.pending;
        status = usb_submit_urb(urbs[i], 0);
        if (status < 0)
        {
            ERROR(dev, "submit iso[%d], error %d\n", i, status);
            if (i == 0)
            {
                rt_hw_interrupt_enable(level);
                rt_exit_critical();
                goto fail;
            }

            simple_free_urb(urbs[i]);
            urbs[i] = NULL;
            context.pending--;
            context.submit_error = 1;
            break;
        }
    }
    /* spin_unlock_irq(&context.lock); */
    rt_hw_interrupt_enable(level);
        rt_exit_critical();

    rt_completion_wait(&context.done, -1);

    for (i = 0; i < param->sglen; i++)
    {
        if (urbs[i])
            simple_free_urb(urbs[i]);
    }
    /*
     * Isochronous transfers are expected to fail sometimes.  As an
     * arbitrary limit, we will report an error if any submissions
     * fail or if the transfer failure rate is > 10%.
     */
    if (status != 0)
        ;
    else if (context.submit_error)
        status = -EACCES;
    else if (context.errors > context.packet_count / 10)
        status = -EIO;
    return status;

fail:
    for (i = 0; i < param->sglen; i++)
    {
        if (urbs[i])
            simple_free_urb(urbs[i]);
    }
    return status;
}
#endif
#if 1
static int test_unaligned_bulk(
    struct usbtest_dev *tdev,
    int pipe,
    unsigned int length,
    int iterations,
    unsigned int transfer_flags,
    const char *label)
{
    int retval;
    struct urb *urb = usbtest_alloc_urb(
        testdev_to_usbdev(tdev), pipe, length, transfer_flags, 1);

    if (!urb)
        return -ENOMEM;

    retval = simple_io(tdev, urb, iterations, 0, 0, label);
    simple_free_urb(urb);
    return retval;
}

#endif

/*-------------------------------------------------------------------------*/

/* We only have this one interface to user space, through usbfs.
 * User mode code can scan usbfs to find N different devices (maybe on
 * different busses) to use when testing, and allocate one thread per
 * test.  So discovery is simplified, and we have no device naming issues.
 *
 * Don't use these only as stress/load tests.  Use them along with with
 * other USB bus activity:  plugging, unplugging, mousing, mp3 playback,
 * video capture, and so on.  Run different tests at different times, in
 * different sequences.  Nothing here should interact with other devices,
 * except indirectly by consuming USB bandwidth and CPU resources for test
 * threads and request completion.  But the only way to know that for sure
 * is to test when HC queues are in use by many devices.
 *
 * WARNING:  Because usbfs grabs udev->dev.sem before calling this ioctl(),
 * it locks out usbcore in certain code paths.  Notably, if you disconnect
 * the device-under-test, khubd will wait block forever waiting for the
 * ioctl to complete ... so that usb_disconnect() can abort the pending
 * urbs and then call usbtest_disconnect().  To abort a test, you're best
 * off just killing the userspace task and waiting for it to exit.
 */

/* No BKL needed */
extern void do_gettimeofday(struct timeval *tv);
extern rt_uint32_t g_dbg_lvl;/* DBG_ANY; */
static rt_err_t
usbtest_ioctl(rt_device_t rt_dev, int code, void *buf)
{
    struct usb_interface *intf = rt_dev->user_data;
    struct usbtest_dev    *dev = usb_get_intfdata(intf);
    struct usb_device    *udev = testdev_to_usbdev(dev);
    struct usbtest_param    *param = buf;
    int            retval = -EOPNOTSUPP;
    struct urb        *urb;
    /* struct scatterlist    *sg; */
/* struct usb_sg_request    req; */
    /* struct timeval        start; */
    unsigned int i;

    /* FIXME USBDEVFS_CONNECTINFO doesn't say how fast the device is. */

    /* pattern = 1; */

/* if (code != USBTEST_REQUEST) */
/* return -EOPNOTSUPP; */

    if (param->iterations <= 0)
        return -EINVAL;

/* if (mutex_lock_interruptible(&dev->lock)) */
/* return -ERESTARTSYS; */
    rt_mutex_take(&dev->lock, -1);
    /* FIXME: What if a system sleep starts while a test is running? */

    /* some devices, like ez-usb default devices, need a non-default
     * altsetting to have any active endpoints.  some tests change
     * altsettings; force a default so most tests don't need to check.
     */
    if (dev->info->alt >= 0)
    {
        int    res;

        if (intf->altsetting->desc.bInterfaceNumber)
        {
            rt_mutex_release(&dev->lock);
            return -ENODEV;
        }
        res = set_altsetting(dev, dev->info->alt);
        if (res)
        {
            rt_kprintf("set altsetting to %d failed, %d\n",
                    dev->info->alt, res);
            rt_mutex_release(&dev->lock);
            return res;
        }
    }

    /*
     * Just a bunch of test cases that every HCD is expected to handle.
     *
     * Some may need specific firmware, though it'd be good to have
     * one firmware image to handle all the test cases.
     *
     * FIXME add more tests!  cancel requests, verify the data, control
     * queueing, concurrent read+write threads, and so on.
     */
    /* do_gettimeofday(&start); */
    switch (param->test_num)
    {

    case 0:
        rt_kprintf("TEST 0:  NOP\n");
        retval = 0;
        break;

    /* Simple non-queued bulk I/O tests */
    case 1:
        if (dev->out_pipe == 0)
            break;
        rt_kprintf("TEST 1:  write %d bytes %u times\n",
                param->length, param->iterations);
        urb = simple_alloc_urb(udev, dev->out_pipe, param->length);
        if (!urb)
        {
            retval = -ENOMEM;
            break;
        }
        /* FIRMWARE:  bulk sink (maybe accepts short writes) */
        retval = simple_io(dev, urb, param->iterations, 0, 0, "test1");
        simple_free_urb(urb);
        break;
    case 2:
        if (dev->in_pipe == 0)
            break;
        rt_kprintf(
                "TEST 2:  read %d bytes %u times\n",
                param->length, param->iterations);
        urb = simple_alloc_urb(udev, dev->in_pipe, param->length);
        if (!urb)
        {
            retval = -ENOMEM;
            break;
        }
        /* FIRMWARE:  bulk source (maybe generates short writes) */
        retval = simple_io(dev, urb, param->iterations, 0, 0, "test2");
        simple_free_urb(urb);
        break;
    case 3:
        if (dev->out_pipe == 0 || param->vary == 0)
            break;
        rt_kprintf(
                "TEST 3:  write/%d 0..%d bytes %u times\n",
                param->vary, param->length, param->iterations);
        urb = simple_alloc_urb(udev, dev->out_pipe, param->length);
        if (!urb)
        {
            retval = -ENOMEM;
            break;
        }
        /* FIRMWARE:  bulk sink (maybe accepts short writes) */
        retval = simple_io(dev, urb, param->iterations, param->vary,
                    0, "test3");
        simple_free_urb(urb);
        break;
    case 4:
        if (dev->in_pipe == 0 || param->vary == 0)
            break;
        rt_kprintf(
                "TEST 4:  read/%d 0..%d bytes %u times\n",
                param->vary, param->length, param->iterations);
        urb = simple_alloc_urb(udev, dev->in_pipe, param->length);
        if (!urb)
        {
            retval = -ENOMEM;
            break;
        }
        /* FIRMWARE:  bulk source (maybe generates short writes) */
        retval = simple_io(dev, urb, param->iterations, param->vary,
                    0, "test4");
        simple_free_urb(urb);
        break;
    default:
        retval = 0;
        break;
#if 0
    /* Queued bulk I/O tests */
    case 5:
        if (dev->out_pipe == 0 || param->sglen == 0)
            break;
        rt_kprintf(
            "TEST 5:  write %d sglists %d entries of %d bytes\n",
                param->iterations,
                param->sglen, param->length);
        sg = alloc_sglist(param->sglen, param->length, 0);
        if (!sg)
        {
            retval = -ENOMEM;
            break;
        }
        /* FIRMWARE:  bulk sink (maybe accepts short writes) */
        retval = perform_sglist(dev, param->iterations, dev->out_pipe,
                &req, sg, param->sglen);
        free_sglist(sg, param->sglen);
        break;

    case 6:
        if (dev->in_pipe == 0 || param->sglen == 0)
            break;
        rt_kprintf(
            "TEST 6:  read %d sglists %d entries of %d bytes\n",
                param->iterations,
                param->sglen, param->length);
        sg = alloc_sglist(param->sglen, param->length, 0);
        if (!sg)
        {
            retval = -ENOMEM;
            break;
        }
        /* FIRMWARE:  bulk source (maybe generates short writes) */
        retval = perform_sglist(dev, param->iterations, dev->in_pipe,
                &req, sg, param->sglen);
        free_sglist(sg, param->sglen);
        break;
    case 7:
        if (dev->out_pipe == 0 || param->sglen == 0 || param->vary == 0)
            break;
        rt_kprintf(
            "TEST 7:  write/%d %d sglists %d entries 0..%d bytes\n",
                param->vary, param->iterations,
                param->sglen, param->length);
        sg = alloc_sglist(param->sglen, param->length, param->vary);
        if (!sg)
        {
            retval = -ENOMEM;
            break;
        }
        /* FIRMWARE:  bulk sink (maybe accepts short writes) */
        retval = perform_sglist(dev, param->iterations, dev->out_pipe,
                &req, sg, param->sglen);
        free_sglist(sg, param->sglen);
        break;
    case 8:
        if (dev->in_pipe == 0 || param->sglen == 0 || param->vary == 0)
            break;
        rt_kprintf(
            "TEST 8:  read/%d %d sglists %d entries 0..%d bytes\n",
                param->vary, param->iterations,
                param->sglen, param->length);
        sg = alloc_sglist(param->sglen, param->length, param->vary);
        if (!sg)
        {
            retval = -ENOMEM;
            break;
        }
        /* FIRMWARE:  bulk source (maybe generates short writes) */
        retval = perform_sglist(dev, param->iterations, dev->in_pipe,
                &req, sg, param->sglen);
        free_sglist(sg, param->sglen);
        break;

#endif
    /* non-queued sanity tests for control (chapter 9 subset) */
    case 9:
        retval = 0;
        rt_kprintf(
            "TEST 9:  ch9 (subset) control tests, %d times\n",
                param->iterations);
        for (i = param->iterations; retval == 0 && i--; /* NOP */)
            retval = ch9_postconfig(dev);
        if (retval)
            rt_kprintf("ch9 subset failed, iterations left %d\n",
                    i);
        break;

    /* queued control messaging */
    case 10:
        if (param->sglen == 0)
            break;
        retval = 0;
        rt_kprintf(
                "TEST 10:  queue %d control calls, %d times\n",
                param->sglen,
                param->iterations);
        retval = test_ctrl_queue(dev, param);
        break;

    /* simple non-queued unlinks (ring with one urb) */
    case 11:
        if (dev->in_pipe == 0 || !param->length)
            break;
        retval = 0;
        rt_kprintf("TEST 11:  unlink %d reads of %d\n",
                param->iterations, param->length);
        for (i = param->iterations; retval == 0 && i--; /* NOP */)
            retval = unlink_simple(dev, dev->in_pipe,
                        param->length);
        if (retval)
            rt_kprintf("unlink reads failed %d, iterations left %d\n",
                retval, i);
        break;
    case 12:
        if (dev->out_pipe == 0 || !param->length)
            break;
        retval = 0;
        rt_kprintf("TEST 12:  unlink %d writes of %d\n",
                param->iterations, param->length);
        for (i = param->iterations; retval == 0 && i--; /* NOP */)
            retval = unlink_simple(dev, dev->out_pipe,
                        param->length);
        if (retval)
            rt_kprintf("unlink writes failed %d, iterations left %d\n",
                retval, i);
        break;

    /* ep halt tests */
    case 13:
        if (dev->out_pipe == 0 && dev->in_pipe == 0)
            break;
        retval = 0;
        rt_kprintf("TEST 13:  set/clear %d halts\n",
                param->iterations);
        /* g_dbg_lvl=0xFF; */
        for (i = param->iterations; retval == 0 && i--; /* NOP */)
            retval = halt_simple(dev);

        if (retval)
            ERROR(dev, "halts failed, iterations left %d\n", i);
        retval = 0;
        break;

    /* control write tests */
    case 14:
        if (!dev->info->ctrl_out)
            break;
        rt_kprintf("TEST 14:  %d ep0out, %d..%d vary %d\n",
                param->iterations,
                realworld ? 1 : 0, param->length,
                param->vary);
        retval = ctrl_out(dev, param->iterations,
                param->length, param->vary, 0);
        rt_kprintf("TEST 14 finish:%d\n", retval);
        break;

    /* iso write tests */
/* case 15: */
/* if (dev->out_iso_pipe == 0 || param->sglen == 0) */
/* break; */
/* rt_kprintf( */
/* "TEST 15:  write %d iso, %d entries of %d bytes\n", */
/* param->iterations, */
/* param->sglen, param->length); */
        /* FIRMWARE:  iso sink */
/* retval = test_iso_queue(dev, param, */
/* dev->out_iso_pipe, dev->iso_out, 0); */
/* break; */

    /* iso read tests */
/* case 16: */
/* if (dev->in_iso_pipe == 0 || param->sglen == 0) */
/* break; */
/* rt_kprintf( */
/* "TEST 16:  read %d iso, %d entries of %d bytes\n", */
/* param->iterations, */
/* param->sglen, param->length); */
        /* FIRMWARE:  iso source */
/* retval = test_iso_queue(dev, param, */
/* dev->in_iso_pipe, dev->iso_in, 0); */
/* break; */

    /* FIXME scatterlist cancel (needs helper thread) */

    /* Tests for bulk I/O using DMA mapping by core and odd address */
    case 17:
        if (dev->out_pipe == 0)
            break;
        rt_kprintf(
            "TEST 17:  write odd addr %d bytes %u times core map\n",
            param->length, param->iterations);
        g_dbg_lvl = 0xFF;
        retval = test_unaligned_bulk(
                dev, dev->out_pipe,
                param->length, param->iterations,
                0, "test17");
        break;

/* case 18: */
/* if (dev->in_pipe == 0) */
/* break; */
/* rt_kprintf( */
/* "TEST 18:  read odd addr %d bytes %u times core map\n", */
/* param->length, param->iterations); */

/* retval = test_unaligned_bulk( */
/* dev, dev->in_pipe, */
/* param->length, param->iterations, */
/* 0, "test18"); */
/* break; */

    /* Tests for bulk I/O using premapped coherent buffer and odd address */
/* case 19: */
/* if (dev->out_pipe == 0) */
/* break; */
/* rt_kprintf( */
/* "TEST 19:  write odd addr %d bytes %u times premapped\n", */
/* param->length, param->iterations); */

/* retval = test_unaligned_bulk( */
/* dev, dev->out_pipe, */
/* param->length, param->iterations, */
/* URB_NO_TRANSFER_DMA_MAP, "test19"); */
/* break; */

/* case 20: */
/* if (dev->in_pipe == 0) */
/* break; */
/* rt_kprintf( */
/* "TEST 20:  read odd addr %d bytes %u times premapped\n", */
/* param->length, param->iterations); */

/* retval = test_unaligned_bulk( */
/* dev, dev->in_pipe, */
/* param->length, param->iterations, */
/* URB_NO_TRANSFER_DMA_MAP, "test20"); */
/* break; */

    /* control write tests with unaligned buffer */
/* case 21: */
/* if (!dev->info->ctrl_out) */
/* break; */
/* rt_kprintf( */
/* "TEST 21:  %d ep0out odd addr, %d..%d vary %d\n", */
/* param->iterations, */
/* realworld ? 1 : 0, param->length, */
/* param->vary); */
/* g_dbg_lvl=0xFF; */
/* retval = ctrl_out(dev, param->iterations, */
/* param->length, param->vary, 1); */
/* break; */

    /* unaligned iso tests */
/* case 22: */
/* if (dev->out_iso_pipe == 0 || param->sglen == 0) */
/* break; */
/* rt_kprintf( */
/* "TEST 22:  write %d iso odd, %d entries of %d bytes\n", */
/* param->iterations, */
/* param->sglen, param->length); */
/* retval = test_iso_queue(dev, param, */
/* dev->out_iso_pipe, dev->iso_out, 1); */
/* break; */

/* case 23: */
/* if (dev->in_iso_pipe == 0 || param->sglen == 0) */
/* break; */
/* rt_kprintf( */
/* "TEST 23:  read %d iso odd, %d entries of %d bytes\n", */
/* param->iterations, */
/* param->sglen, param->length); */
/* retval = test_iso_queue(dev, param, */
/* dev->in_iso_pipe, dev->iso_in, 1); */
/* break; */

    /* unlink URBs from a bulk-OUT queue */
    case 24:
        if (dev->out_pipe == 0 || !param->length || param->sglen < 4)
            break;
        retval = 0;
        rt_kprintf("TEST 24:  unlink from %d queues of %d %d-byte writes\n",
                param->iterations, param->sglen, param->length);
        for (i = param->iterations; retval == 0 && i > 0; --i)
        {
            retval = unlink_queued(dev, dev->out_pipe,
                        param->sglen, param->length);
            if (retval)
            {
                rt_kprintf(
                    "unlink queued writes failed %d, iterations left %d\n",
                    retval, i);
                break;
            }
        }
        break;

    }
    /* do_gettimeofday(&param->duration); */
    /* param->duration.tv_sec -= start.tv_sec; */
    /* param->duration.tv_usec -= start.tv_usec; */
    /* if (param->duration.tv_usec < 0) { */
    /* param->duration.tv_usec += 1000 * 1000; */
    /* param->duration.tv_sec -= 1; */
    /* } */
    rt_mutex_release(&dev->lock);
    return retval;
}

/*-------------------------------------------------------------------------*/
void usb_test_thread(void *para)
{
    int ret = 0;
    struct urb *urb = RT_NULL;

    while (1)
    {
        if (rt_mb_recv(&g_mb_usb_test, (rt_uint32_t *)&urb,
                RT_WAITING_FOREVER) == RT_EOK) {
                ret = usb_submit_urb(urb, 0);
                if (ret)
                {
                    urb->status = ret;
                    rt_completion_done(urb->context);
                }
        }
    }
}
static unsigned force_interrupt = 0;
static int
usbtest_probe(struct usb_interface *intf, const struct usb_device_id *id)
{
    struct usb_device    *udev;
    struct usbtest_dev    *dev;
    struct usbtest_info    *info;
    char            *rtest, *wtest;
    char            *irtest, *iwtest;

    udev = intf->usb_dev;

#ifdef    GENERIC
    /* specify devices by module parameters? */
    if (id->match_flags == 0)
    {
        /* vendor match required, product match optional */
        /* if (!vendor || (udev->descriptor.idVendor) != (u16)vendor) */
        /* return -ENODEV; */
        /* if (product && (udev->descriptor.idProduct) != (u16)product) */
        /* return -ENODEV; */
        rt_kprintf("matched module params, vend=0x%04x prod=0x%04x\n",
                (udev->descriptor.idVendor),
                (udev->descriptor.idProduct));
    }
#endif

    dev = rt_malloc(sizeof(*dev));
    if (!dev)
        return -ENOMEM;
    rt_memset(dev, 0, sizeof(*dev));
    info = (struct usbtest_info *) id->driver_info;
    dev->info = info;
    rt_mutex_init(&dev->lock, "usbtest", RT_IPC_FLAG_FIFO);

    dev->intf = intf;
    intf->user_data = dev;
    intf->dev.user_data = intf;
    /* cacheline-aligned scratch for i/o */
    dev->buf = usb_dma_buffer_alloc(TBUF_SIZE, udev->bus);
    if (dev->buf == NULL)
    {
        rt_free(dev);
        return -ENOMEM;
    }
    rt_memset(dev->buf, 0, TBUF_SIZE);
    /* NOTE this doesn't yet test the handful of difference that are
     * visible with high speed interrupts:  bigger maxpacket (1K) and
     * "high bandwidth" modes (up to 3 packets/uframe).
     */
    rtest = wtest = "";
    irtest = iwtest = "";
    if (force_interrupt || udev->speed == USB_SPEED_LOW)
    {
        if (info->ep_in)
        {
            dev->in_pipe = usb_rcvintpipe(udev, info->ep_in);
            rtest = " intr-in";
        }
        if (info->ep_out)
        {
            dev->out_pipe = usb_sndintpipe(udev, info->ep_out);
            wtest = " intr-out";
        }
    } else
    {
        if (info->autoconf)
        {
            int status;

            status = get_endpoints(dev, intf);
            if (status < 0)
            {
                WARNING(dev, "couldn't get endpoints, %d\n",
                        status);
                return status;
            }
            /* may find bulk or ISO pipes */
        } else
        {
            if (info->ep_in)
                dev->in_pipe = usb_rcvbulkpipe(udev,
                            info->ep_in);
            if (info->ep_out)
                dev->out_pipe = usb_sndbulkpipe(udev,
                            info->ep_out);
        }
        if (dev->in_pipe)
            rtest = " bulk-in";
        if (dev->out_pipe)
            wtest = " bulk-out";
        if (dev->in_iso_pipe)
            irtest = " iso-in";
        if (dev->out_iso_pipe)
            iwtest = " iso-out";
    }

    /* usb_set_intfdata(intf, dev); */
    rt_kprintf("%s\n", info->name);
    rt_kprintf("%s speed {control%s%s%s%s%s} tests%s\n",
            ({ char *tmp;
            switch (udev->speed)
            {
            case USB_SPEED_LOW:
                tmp = "low";
                break;
            case USB_SPEED_FULL:
                tmp = "full";
                break;
            case USB_SPEED_HIGH:
                tmp = "high";
                break;
            case USB_SPEED_SUPER:
                tmp = "super";
                break;
            default:
                tmp = "unknown";
                break;
            }; tmp; }),
            info->ctrl_out ? " in/out" : "",
            rtest, wtest,
            irtest, iwtest,
            info->alt >= 0 ? " (+alt)" : "");
    intf->dev.control = usbtest_ioctl;
    rt_mb_init(&g_mb_usb_test, "mbt", &g_usb_test_pool[0], sizeof(g_usb_test_pool)/4, RT_IPC_FLAG_FIFO);
    usb_test = rt_thread_create("usb_test",
            usb_test_thread, NULL, 1024, 79, 3);

        if (usb_test != RT_NULL)
            rt_thread_startup(usb_test);
     rt_device_register(&(intf->dev), "usbtest", RT_DEVICE_FLAG_RDWR);
    return 0;
}


static void usbtest_disconnect(struct usb_interface *intf)
{
    struct usbtest_dev    *dev = usb_get_intfdata(intf);

    /* usb_set_intfdata(intf, NULL); */

    rt_kprintf("disconnect\n");
    usb_dma_buffer_free(dev->buf, intf->usb_dev->bus);
    rt_device_unregister(&(intf->dev));
    rt_free(dev);
}

/* Basic testing only needs a device that can source or sink bulk traffic.
 * Any device can test control transfers (default with GENERIC binding).
 *
 * Several entries work with the default EP0 implementation that's built
 * into EZ-USB chips.  There's a default vendor ID which can be overridden
 * by (very) small config EEPROMS, but otherwise all these devices act
 * identically until firmware is loaded:  only EP0 works.  It turns out
 * to be easy to make other endpoints work, without modifying that EP0
 * behavior.  For now, we expect that kind of firmware.
 */

/* an21xx or fx versions of ez-usb */
static struct usbtest_info ez1_info = {
    .name        = "EZ-USB device",
    .ep_in        = 2,
    .ep_out        = 2,
    .alt        = 1,
};

/* fx2 version of ez-usb */
static struct usbtest_info ez2_info = {
    .name        = "FX2 device",
    .ep_in        = 6,
    .ep_out        = 2,
    .alt        = 1,
};

/* ezusb family device with dedicated usb test firmware,
 */
static struct usbtest_info fw_info = {
    .name        = "usb test device",
    .ep_in        = 2,
    .ep_out        = 2,
    .alt        = 1,
    .autoconf    = 1,        /* iso and ctrl_out need autoconf */
    .ctrl_out    = 1,
    .iso        = 1,        /* iso_ep's are #8 in/out */
};

/* peripheral running Linux and 'zero.c' test firmware, or
 * its user-mode cousin. different versions of this use
 * different hardware with the same vendor/product codes.
 * host side MUST rely on the endpoint descriptors.
 */
static struct usbtest_info gz_info = {
    .name        = "Linux gadget zero",
    .autoconf    = 1,
    .ctrl_out    = 1,
    .alt        = 0,
};

static struct usbtest_info um_info = {
    .name        = "Linux user mode test driver",
    .autoconf    = 1,
    .alt        = -1,
};

static struct usbtest_info um2_info = {
    .name        = "Linux user mode ISO test driver",
    .autoconf    = 1,
    .iso        = 1,
    .alt        = -1,
};

#ifdef IBOT2
/* this is a nice source of high speed bulk data;
 * uses an FX2, with firmware provided in the device
 */
static struct usbtest_info ibot2_info = {
    .name        = "iBOT2 webcam",
    .ep_in        = 2,
    .alt        = -1,
};
#endif

/* #ifdef GENERIC */
/* we can use any device to test control traffic */
/* static struct usbtest_info generic_info = { */
/* .name        = "Generic USB device", */
/* .alt        = -1, */
/* }; */
/* #endif */


static const struct usb_device_id id_table[] = {

    /*-------------------------------------------------------------*/

    /* EZ-USB devices which download firmware to replace (or in our
     * case augment) the default device implementation.
     */

    /* generic EZ-USB FX controller */
    { USB_DEVICE(0x0547, 0x2235),
        .driver_info = (unsigned long) &ez1_info,
    },

    /* CY3671 development board with EZ-USB FX */
    { USB_DEVICE(0x0547, 0x0080),
        .driver_info = (unsigned long) &ez1_info,
    },

    /* generic EZ-USB FX2 controller (or development board) */
    { USB_DEVICE(0x04b4, 0x8613),
        .driver_info = (unsigned long) &ez2_info,
    },

    /* re-enumerated usb test device firmware */
    { USB_DEVICE(0xfff0, 0xfff0),
        .driver_info = (unsigned long) &fw_info,
    },

    /* "Gadget Zero" firmware runs under Linux */
    { USB_DEVICE(0x0525, 0xa4a0),
        .driver_info = (unsigned long) &gz_info,
    },

    /* so does a user-mode variant */
    { USB_DEVICE(0x0525, 0xa4a4),
        .driver_info = (unsigned long) &um_info,
    },

    /* ... and a user-mode variant that talks iso */
    { USB_DEVICE(0x0525, 0xa4a3),
        .driver_info = (unsigned long) &um2_info,
    },

#ifdef KEYSPAN_19Qi
    /* Keyspan 19qi uses an21xx (original EZ-USB) */
    /* this does not coexist with the real Keyspan 19qi driver! */
    { USB_DEVICE(0x06cd, 0x010b),
        .driver_info = (unsigned long) &ez1_info,
    },
#endif

    /*-------------------------------------------------------------*/

#ifdef IBOT2
    /* iBOT2 makes a nice source of high speed bulk-in data */
    /* this does not coexist with a real iBOT2 driver! */
    { USB_DEVICE(0x0b62, 0x0059),
        .driver_info = (unsigned long) &ibot2_info,
    },
#endif

    /*-------------------------------------------------------------*/

/* #ifdef GENERIC */
    /* module params can specify devices to use for control tests */
/* { .driver_info = (unsigned long) &generic_info, }, */
/* #endif */

    /*-------------------------------------------------------------*/

    { }
};
static struct usb_driver usbtest_driver = {
    .name =        "usbtest",
    .id_table =    id_table,
    .probe =    usbtest_probe,
    .disconnect =    usbtest_disconnect,
};

/*-------------------------------------------------------------------------*/

int  usbtest_init(void)
{
    return usb_register_driver(&usbtest_driver);
}


