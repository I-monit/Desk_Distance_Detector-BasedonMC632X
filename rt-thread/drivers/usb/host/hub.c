
/* #include "usb.h" */
#include "rtdef.h"
#include "rtthread.h"
#include <rthw.h>
#include "hcd.h"
#include "ch11.h"
#include "quirks.h"
#include <dma_mem.h>
#include "usb_errno.h"

/* if we are in debug mode, always announce new devices */
struct usb_hub
{
    struct usb_device    *hdev;
    rt_int8_t        kref;
    struct urb        *urb;        /* for interrupt polling pipe */

    /* buffer for urb ... with extra space in case of babble */
    char            (*buffer)[8];
    union {
        struct usb_hub_status    hub;
        struct usb_port_status    port;
    } *status;    /* buffer for status reports */
    int            error;        /* last reported error */
    int            nerrors;    /* track consecutive errors */

    rt_list_t event_list;    /* hubs w/data or errs ready */
    unsigned long        event_bits[1];    /* status change bitmask */
    unsigned long        change_bits[1];    /* ports with logical connect
                            status change */
    unsigned long        removed_bits[1]; /* ports with a "removed"
                            device present */

    struct usb_hub_descriptor *descriptor;    /* class descriptor */
/* struct usb_tt        tt;         Transaction Translator  */

    unsigned int mA_per_port;    /* current for each child */

    unsigned        limited_power:1;
    unsigned        quiescing:1;
    unsigned        disconnected:1;

    unsigned        has_indicators:1;
    rt_uint8_t            indicator[USB_MAXCHILDREN];
    rt_timer_t        leds;
    void            **port_owners;
};

static inline int hub_is_superspeed(struct usb_device *hdev)
{
    return (hdev->descriptor.bDeviceProtocol == 3);
}

/* Protect struct usb_device->state and ->children members
 * Note: Both are also protected by ->dev.sem, except that ->state can
 * change to USB_STATE_NOTATTACHED even when the semaphore isn't held. */
/* static DEFINE_SPINLOCK(device_state_lock); */

/* khubd's worklist and its lock */
/* static DEFINE_SPINLOCK(hub_event_lock); */
rt_list_t hub_event_list;    /* List of hubs needing servicing */


/* Wakes up khubd */
/* struct rt_event event_khubd_wait; */
rt_sem_t sem_khubd_wait = RT_NULL;


rt_thread_t khubd_task = RT_NULL;
rt_timer_t hub_timer = RT_NULL;


#define USB_THREAD_STACK_SIZE    (16*1024 - 128)

#if 0
static inline void set_bit(int nr, volatile unsigned long *addr)
{
    unsigned long mask = 1<<(nr%32);
    unsigned long *p = ((unsigned long *)addr + nr/32);
    *p  |= mask;

}

static inline int test_bit(int nr, const volatile unsigned long *addr)
{
    return 1UL&(addr[nr/32] >> nr&0x1f);
}


static inline int test_and_clear_bit(int nr, volatile unsigned long *addr)
{
    unsigned long mask = 1<<(nr%32);
    unsigned long *p = ((unsigned long *)addr + nr/32);
    unsigned long old;

    old = *p;
    *p = old & ~mask;


    return (old & mask) != 0;
}

inline void clear_bit(int nr, volatile unsigned long *addr)
{
    unsigned long mask =  1<<(nr%32);
    unsigned long *p = ((unsigned long *)addr + nr/32);
    *p &= ~mask;

}
#endif

char *kstrdup(const char *s)
{
    rt_uint32_t len;
    char *buf;

    if (!s)
        return RT_NULL;

    len = rt_strlen(s) + 1;
    buf = rt_malloc(len);
    RT_DEBUG_LOG(rt_debug_usb, ("rt_malloc kstrdup buf:%d size:%d,addr:%x\n", g_mem_debug++, len, buf));
    if (buf)
        rt_memcpy(buf, s, len);
    return buf;
}

int find_next_zero_bit(rt_uint32_t *p, rt_uint32_t size, rt_uint32_t offset)
{
    rt_uint32_t bitmap;
    rt_uint32_t a, b;
    rt_uint32_t i, ret = RT_EOK;

    for (i = 0; i < size; i++)
        {
           if ((i+offset) >= size)
               ret = i+offset-size;
            else
                ret = i+offset;

            a = ret/32;
            b = ret%32;
            bitmap =  *(p+a);
            if (!(bitmap&(1<<b)))
                break;
        }
    return ret;

}


/* cycle leds on hubs that aren't blinking for attention */
static int blinkenlights;
/*
 * Device SATA8000 FW1.0 from DATAST0R Technology Corp requires about
 * 10 seconds to send reply for the initial 64-byte descriptor request.
 */
/* define initial 64-byte descriptor request timeout in milliseconds */
static int initial_descriptor_timeout = USB_CTRL_GET_TIMEOUT;
/*
 * As of 2.6.10 we introduce a new USB device initialization scheme which
 * closely resembles the way Windows works.  Hopefully it will be compatible
 * with a wider range of devices than the old scheme.  However some previously
 * working devices may start giving rise to "device not accepting address"
 * errors; if that happens the user can try the old scheme by adjusting the
 * following module parameters.
 *
 * For maximum flexibility there are two boolean parameters to control the
 * hub driver's behavior.  On the first initialization attempt, if the
 * "old_scheme_first" parameter is set then the old scheme will be used,
 * otherwise the new scheme is used.  If that fails and "use_both_schemes"
 * is set, then the driver will make another attempt, using the other scheme.
 */
static int old_scheme_first;

static int use_both_schemes = 1;

/* Mutual exclusion for EHCI CF initialization.  This interferes with
 * port reset on some companion controllers.
 */

/**/
/* DECLARE_RWSEM(ehci_cf_port_reset_rwsem); */
/* EXPORT_SYMBOL_GPL(ehci_cf_port_reset_rwsem); */

#define HUB_DEBOUNCE_TIMEOUT    1500
#define HUB_DEBOUNCE_STEP      25
#define HUB_DEBOUNCE_STABLE     100


static inline char *portspeed(struct usb_hub *hub, int portstatus)
{
    if (hub_is_superspeed(hub->hdev))
        return "5.0 Gb/s";
    if (portstatus & USB_PORT_STAT_HIGH_SPEED)
            return "480 Mb/s";
    else if (portstatus & USB_PORT_STAT_LOW_SPEED)
        return "1.5 Mb/s";
    else
        return "12 Mb/s";
}

/* Note that hdev or one of its children must be locked! */
static struct usb_hub *hdev_to_hub(struct usb_device *hdev)
{
    if (!hdev || !hdev->actconfig)
        return RT_NULL;
    return usb_get_intfdata(hdev->actconfig->interface[0]);
}

/* USB 2.0 spec Section 11.24.4.5 */
static int get_hub_descriptor(struct usb_device *hdev, void *data)
{
    int i, ret, size;
    unsigned int dtype;


    if (hub_is_superspeed(hdev))
    {
        dtype = USB_DT_SS_HUB;
        size = USB_DT_SS_HUB_SIZE;
    } else
    {
        dtype = USB_DT_HUB;
        size = sizeof(struct usb_hub_descriptor);
    }

    for (i = 0; i < 3; i++)
    {
        ret = usb_control_msg(hdev, usb_rcvctrlpipe(hdev, 0),
            USB_REQ_GET_DESCRIPTOR, USB_DIR_IN | USB_RT_HUB,
            dtype << 8, 0, data, size,
            USB_CTRL_GET_TIMEOUT);
        if (ret >= (USB_DT_HUB_NONVAR_SIZE + 2))
        {
            return ret;
        }
    }
    return -EINVAL;
}

/*
 * USB 2.0 spec Section 11.24.2.1
 */
static int clear_hub_feature(struct usb_device *hdev, int feature)
{
    return usb_control_msg(hdev, usb_sndctrlpipe(hdev, 0),
        USB_REQ_CLEAR_FEATURE, USB_RT_HUB, feature, 0, RT_NULL, 0, 1000);
}

/*
 * USB 2.0 spec Section 11.24.2.2
 */
static int clear_port_feature(struct usb_device *hdev, int port1, int feature)
{
    return usb_control_msg(hdev, usb_sndctrlpipe(hdev, 0),
        USB_REQ_CLEAR_FEATURE, USB_RT_PORT, (rt_uint16_t)feature, (rt_uint16_t)port1,
        RT_NULL, 0, 1000);
}

/*
 * USB 2.0 spec Section 11.24.2.13
 */
static int set_port_feature(struct usb_device *hdev, int port1, int feature)
{
    return usb_control_msg(hdev, usb_sndctrlpipe(hdev, 0),
        USB_REQ_SET_FEATURE, USB_RT_PORT, (rt_uint16_t)feature, (rt_uint16_t)port1,
        RT_NULL, 0, 1000);
}

/*
 * USB 2.0 spec Section 11.24.2.7.1.10 and table 11-7
 * for info about using port indicators
 */
static void set_port_led(
    struct usb_hub *hub,
    int port1,
    int selector
)
{
    int status = set_port_feature(hub->hdev, (selector << 8) | port1,
            USB_PORT_FEAT_INDICATOR);
    if (status < 0)
        RT_DEBUG_LOG(rt_debug_usb,
            ("port %d status %d\n",
            port1, status));
}

#define    LED_CYCLE_PERIOD    ((2*RT_TICK_PER_SECOND)/3)

static void led_work(void *dev)
{
    struct usb_hub   *hub = dev;
    struct usb_device    *hdev = hub->hdev;
    unsigned int i;
    unsigned int changed = 0;
    int            cursor = -1;

    if (hdev->state != USB_STATE_CONFIGURED || hub->quiescing)
        return;

    for (i = 0; i < hub->descriptor->bNbrPorts; i++)
    {
        unsigned int selector, mode;

        /* 30%-50% duty cycle */

        switch (hub->indicator[i])
        {
        /* cycle marker */
        case INDICATOR_CYCLE:
            cursor = i;
            selector = HUB_LED_AUTO;
            mode = INDICATOR_AUTO;
            break;
        /* blinking green = sw attention */
        case INDICATOR_GREEN_BLINK:
            selector = HUB_LED_GREEN;
            mode = INDICATOR_GREEN_BLINK_OFF;
            break;
        case INDICATOR_GREEN_BLINK_OFF:
            selector = HUB_LED_OFF;
            mode = INDICATOR_GREEN_BLINK;
            break;
        /* blinking amber = hw attention */
        case INDICATOR_AMBER_BLINK:
            selector = HUB_LED_AMBER;
            mode = INDICATOR_AMBER_BLINK_OFF;
            break;
        case INDICATOR_AMBER_BLINK_OFF:
            selector = HUB_LED_OFF;
            mode = INDICATOR_AMBER_BLINK;
            break;
        /* blink green/amber = reserved */
        case INDICATOR_ALT_BLINK:
            selector = HUB_LED_GREEN;
            mode = INDICATOR_ALT_BLINK_OFF;
            break;
        case INDICATOR_ALT_BLINK_OFF:
            selector = HUB_LED_AMBER;
            mode = INDICATOR_ALT_BLINK;
            break;
        default:
            continue;
        }
        if (selector != HUB_LED_AUTO)
            changed = 1;
        set_port_led(hub, i + 1, selector);
        hub->indicator[i] = mode;
    }
    if (!changed && blinkenlights)
    {
        cursor++;
        cursor %= hub->descriptor->bNbrPorts;
        set_port_led(hub, cursor + 1, HUB_LED_GREEN);
        hub->indicator[cursor] = INDICATOR_CYCLE;
        changed++;
    }
/* if (changed) */
/* schedule_delayed_work(&hub->leds, LED_CYCLE_PERIOD); */
}

/* use a short timeout for hub/port status fetches */
#define    USB_STS_TIMEOUT        1000
#define    USB_STS_RETRIES        5

/*
 * USB 2.0 spec Section 11.24.2.6
 */
static int get_hub_status(struct usb_device *hdev,
        struct usb_hub_status *data)
{
    int i, status = -ETIMEDOUT;

    for (i = 0; i < USB_STS_RETRIES &&
            (status == -ETIMEDOUT || status == -EPIPE); i++) {
        status = usb_control_msg(hdev, usb_rcvctrlpipe(hdev, 0),
            USB_REQ_GET_STATUS, USB_DIR_IN | USB_RT_HUB, 0, 0,
            data, sizeof(*data), USB_STS_TIMEOUT);
    }
    return status;
}

/*
 * USB 2.0 spec Section 11.24.2.7
 */
static int get_port_status(struct usb_device *hdev, int port1,
        struct usb_port_status *data)
{
    int i, status = -ETIMEDOUT;

    for (i = 0; i < USB_STS_RETRIES &&
            (status == -ETIMEDOUT || status == -EPIPE); i++) {
        status = usb_control_msg(hdev, usb_rcvctrlpipe(hdev, 0),
            USB_REQ_GET_STATUS, USB_DIR_IN | USB_RT_PORT, 0, port1,
            data, sizeof(*data), USB_STS_TIMEOUT);
    }
    return status;
}

static int hub_port_status(struct usb_hub *hub, int port1,
        rt_uint16_t *status, rt_uint16_t *change)
{
    int ret;
    void *dma_addr = RT_NULL;
    /* mutex_lock(&hub->status_mutex); */
    dma_addr = usb_dma_buffer_alloc(sizeof(hub->status->port), hub->hdev->bus);
    ret = get_port_status(hub->hdev, port1, dma_addr);
    if (ret < 4)
    {
        RT_DEBUG_LOG(rt_debug_usb,
            ("%s failed (err = %d)\n", __func__, ret));
        if (ret >= 0)
            ret = -EIO;
    } else
    {
        rt_memcpy(&hub->status->port, dma_addr, sizeof(hub->status->port));
        *status = hub->status->port.wPortStatus;
        *change = hub->status->port.wPortChange;
        /* usb_dma_buffer_free(dma_addr,hub->hdev->bus); */
        ret = 0;
    }
    /* mutex_unlock(&hub->status_mutex); */
    usb_dma_buffer_free(dma_addr, hub->hdev->bus);
    return ret;
}

static void kick_khubd(struct usb_hub *hub)
{
    rt_base_t level;
    /* <-tangyh*/

       /* spin_lock_irqsave(&hub_event_lock, flags); */
       /* tangyh ->*/
    if (!hub->disconnected && rt_list_isempty(&hub->event_list))
    {
        level = rt_hw_interrupt_disable();
        rt_enter_critical();
        rt_list_insert_after(&hub_event_list, &hub->event_list);
        rt_hw_interrupt_enable(level);
        rt_exit_critical();
        /* rt_kprintf("rt_sem_release:%d,hub:%x\n",sem_khubd_wait->value,hub); */
        rt_sem_release(sem_khubd_wait);
    }
    /* <-tangyh*/
        /* rt_hw_interrupt_enable(level); */
        /* rt_exit_critical(); */
    /* spin_unlock_irqrestore(&hub_event_lock, flags); */
/* tangyh ->*/
}

void usb_kick_khubd(struct usb_device *hdev)
{
    struct usb_hub *hub = hdev_to_hub(hdev);

    if (hub)
        kick_khubd(hub);
}


/* completion function, fires on port status changes and various faults */
static void hub_irq(struct urb *urb)
{
    struct usb_hub *hub = urb->context;
    int status = urb->status;
    unsigned int i;
    unsigned long bits;

    switch (status)
    {
    case -ENOENT:        /* synchronous unlink */
    case -ECONNRESET:    /* async unlink */
    case -ESHUTDOWN:    /* hardware going away */
        return;

    default:        /* presumably an error */
        /* Cause a hub reset after 10 consecutive errors */
        RT_DEBUG_LOG(rt_debug_usb, ("transfer --> %d\n", status));
        if ((++hub->nerrors < 10) || hub->error)
            goto resubmit;
        hub->error = status;
        /* FALL THROUGH */

    /* let khubd handle things */
    case 0:            /* we got data:  port status changed */
        bits = 0;
        for (i = 0; i < urb->actual_length; ++i)
            bits |= ((unsigned long) ((*hub->buffer)[i]))
                    << (i*8);
        hub->event_bits[0] = bits;
        break;
    }

    hub->nerrors = 0;

    /* Something happened, let khubd figure it out */
    kick_khubd(hub);

resubmit:
    if (hub->quiescing)
        return;

    if ((status = usb_submit_urb(hub->urb, 0) != 0)
            && status != -ENODEV && status != -EPERM)
        RT_DEBUG_LOG(rt_debug_usb, ("resubmit --> %d\n", status));
}


/* USB 2.0 spec Section 11.24.2.3 */
static inline int
hub_clear_tt_buffer(struct usb_device *hdev, rt_uint16_t devinfo, rt_uint16_t tt)
{
    return usb_control_msg(hdev, usb_sndctrlpipe(hdev, 0),
                   HUB_CLEAR_TT_BUFFER, USB_RT_PORT, devinfo,
                   tt, RT_NULL, 0, 1000);
}

/*
 * enumeration blocks khubd for a long time. we use keventd instead, since
 * long blocking there is the exception, not the rule.  accordingly, HCDs
 * talking to TTs must queue control transfers (not just bulk and iso), so
 * both can talk to the same hub concurrently.
 */
#if 0
static void hub_tt_work(struct work_struct *work)
{
    struct usb_hub        *hub =
        container_of(work, struct usb_hub, tt.clear_work);
    unsigned long        flags;
    int            limit = 100;

    spin_lock_irqsave(&hub->tt.lock, flags);
    while (--limit && !list_empty(&hub->tt.clear_list))
    {
        struct list_head    *next;
        struct usb_tt_clear    *clear;
        struct usb_device    *hdev = hub->hdev;
        const struct hc_driver    *drv;
        int            status;

        next = hub->tt.clear_list.next;
        clear = list_entry(next, struct usb_tt_clear, clear_list);
        list_del(&clear->clear_list);

        /* drop lock so HCD can concurrently report other TT errors */
        spin_unlock_irqrestore(&hub->tt.lock, flags);
        status = hub_clear_tt_buffer(hdev, clear->devinfo, clear->tt);
        if (status)
            dev_err(&hdev->dev,
                "clear tt %d (%04x) error %d\n",
                clear->tt, clear->devinfo, status);

        /* Tell the HCD, even if the operation failed */
        drv = clear->hcd->driver;
        if (drv->clear_tt_buffer_complete)
            (drv->clear_tt_buffer_complete)(clear->hcd, clear->ep);

        kfree(clear);
        spin_lock_irqsave(&hub->tt.lock, flags);
    }
    spin_unlock_irqrestore(&hub->tt.lock, flags);
}

#endif
/**
 * usb_hub_clear_tt_buffer - clear control/bulk TT state in high speed hub
 * @urb: an URB associated with the failed or incomplete split transaction
 *
 * High speed HCDs use this to tell the hub driver that some split control or
 * bulk transaction failed in a way that requires clearing internal state of
 * a transaction translator.  This is normally detected (and reported) from
 * interrupt context.
 *
 * It may not be possible for that hub to handle additional full (or low)
 * speed transactions until that state is fully cleared out.
 */

#if 0
int usb_hub_clear_tt_buffer(struct urb *urb)
{
    struct usb_device    *udev = urb->dev;
    int            pipe = urb->pipe;
    struct usb_tt        *tt = udev->tt;
    unsigned long        flags;
    struct usb_tt_clear    *clear;

    /* we've got to cope with an arbitrary number of pending TT clears,
     * since each TT has "at least two" buffers that can need it (and
     * there can be many TTs per hub).  even if they're uncommon.
     */
    if ((clear = kmalloc(sizeof(*clear), GFP_ATOMIC)) == NULL)
    {
        dev_err(&udev->dev, "can't save CLEAR_TT_BUFFER state\n");
        /* FIXME recover somehow ... RESET_TT? */
        return -ENOMEM;
    }

    /* info that CLEAR_TT_BUFFER needs */
    clear->tt = tt->multi ? udev->ttport : 1;
    clear->devinfo = usb_pipeendpoint (pipe);
    clear->devinfo |= udev->devnum << 4;
    clear->devinfo |= usb_pipecontrol(pipe)
            ? (USB_ENDPOINT_XFER_CONTROL << 11)
            : (USB_ENDPOINT_XFER_BULK << 11);
    if (usb_pipein(pipe))
        clear->devinfo |= 1 << 15;

    /* info for completion callback */
    clear->hcd = bus_to_hcd(udev->bus);
    clear->ep = urb->ep;

    /* tell keventd to clear state for this TT */
    spin_lock_irqsave(&tt->lock, flags);
    list_add_tail(&clear->clear_list, &tt->clear_list);
    schedule_work(&tt->clear_work);
    spin_unlock_irqrestore(&tt->lock, flags);
    return 0;
}
#endif

/* If do_delay is false, return the number of milliseconds the caller
 * needs to delay.
 */
static unsigned int hub_power_on(struct usb_hub *hub, rt_bool_t do_delay)
{
    int port1;
    unsigned int pgood_delay = hub->descriptor->bPwrOn2PwrGood * 2;
    unsigned int delay;
    rt_uint16_t wHubCharacteristics = hub->descriptor->wHubCharacteristics;
/* le16_to_cpu(hub->descriptor->wHubCharacteristics); */

    /* Enable power on each port.  Some hubs have reserved values
     * of LPSM (> 2) in their descriptors, even though they are
     * USB 2.0 hubs.  Some hubs do not implement port-power switching
     * but only emulate it.  In all cases, the ports won't work
     * unless we send these messages to the hub.
     */
    if ((wHubCharacteristics & HUB_CHAR_LPSM) < 2)
        RT_DEBUG_LOG(rt_debug_usb, ("enabling power on all ports\n"));
    else
        RT_DEBUG_LOG(rt_debug_usb, ("trying to enable port power on non-switchable hub\n"
                ));
    for (port1 = 1; port1 <= hub->descriptor->bNbrPorts; port1++)
        set_port_feature(hub->hdev, port1, USB_PORT_FEAT_POWER);

    /* Wait at least 100 msec for power to become stable */
	if (pgood_delay <= (unsigned) 100)
    delay = 100;
    else
    delay = pgood_delay;

    if (do_delay)
        rt_thread_delay(delay/(1000/RT_TICK_PER_SECOND));
    return delay;
}

static int hub_hub_status(struct usb_hub *hub,
        rt_uint16_t *status, rt_uint16_t *change)
{
    int ret;

/* mutex_lock(&hub->status_mutex); */
    ret = get_hub_status(hub->hdev, &hub->status->hub);
    if (ret < 0)
        RT_DEBUG_LOG(rt_debug_usb,
            ("%s failed (err = %d)\n", __func__, ret));
    else
    {
        *status = (hub->status->hub.wHubStatus);
        *change = (hub->status->hub.wHubChange);
        ret = 0;
    }
/* mutex_unlock(&hub->status_mutex); */
    return ret;
}

static int hub_port_disable(struct usb_hub *hub, int port1, int set_state)
{
    struct usb_device *hdev = hub->hdev;
    int ret = 0;

    if (hdev->children[port1-1] && set_state)
        usb_set_device_state(hdev->children[port1-1],
                USB_STATE_NOTATTACHED);
    if (!hub->error && !hub_is_superspeed(hub->hdev))
        ret = clear_port_feature(hdev, port1, USB_PORT_FEAT_ENABLE);
    if (ret)
        RT_DEBUG_LOG(rt_debug_usb, ("cannot disable port %d (err = %d)\n",
                port1, ret));
    return ret;
}

static void hub_activate(struct usb_hub *hub)
{
    struct usb_device *hdev = hub->hdev;
    int port1;
    int status;
    rt_bool_t need_debounce_delay = RT_FALSE;


    hub_power_on(hub, RT_TRUE);

    /* Check each port and set hub->change_bits to let khubd know
     * which ports need attention.
     */
    for (port1 = 1; port1 <= hdev->maxchild; ++port1)
    {
        struct usb_device *udev = hdev->children[port1-1];
        rt_uint16_t portstatus, portchange;

        portstatus = portchange = 0;
        status = hub_port_status(hub, port1, &portstatus, &portchange);
        if (udev || (portstatus & USB_PORT_STAT_CONNECTION))
            RT_DEBUG_LOG(rt_debug_usb,
                    ("port %d: status %04x change %04x\n",
                    port1, portstatus, portchange));

        /* After anything other than HUB_RESUME (i.e., initialization
         * or any sort of reset), every port should be disabled.
         * Unconnected ports should likewise be disabled (paranoia),
         * and so should ports for which we have no usb_device.
         */
        if ((portstatus & USB_PORT_STAT_ENABLE) && (
                !(portstatus & USB_PORT_STAT_CONNECTION) ||
                !udev ||
                udev->state == USB_STATE_NOTATTACHED)) {
            /*
             * USB3 protocol ports will automatically transition
             * to Enabled state when detect an USB3.0 device attach.
             * Do not disable USB3 protocol ports.
             */
            if (!hub_is_superspeed(hdev))
            {
                clear_port_feature(hdev, port1,
                           USB_PORT_FEAT_ENABLE);
                portstatus &= ~USB_PORT_STAT_ENABLE;
            } else
            {
                /* Pretend that power was lost for USB3 devs */
                portstatus &= ~USB_PORT_STAT_ENABLE;
            }
        }

        /* Clear status-change flags; we'll debounce later */
        if (portchange & USB_PORT_STAT_C_CONNECTION)
        {
            need_debounce_delay = RT_TRUE;
            clear_port_feature(hub->hdev, port1,
                    USB_PORT_FEAT_C_CONNECTION);
        }
        if (portchange & USB_PORT_STAT_C_ENABLE)
        {
            need_debounce_delay = RT_TRUE;
            clear_port_feature(hub->hdev, port1,
                    USB_PORT_FEAT_C_ENABLE);
        }
        if (portchange & USB_PORT_STAT_C_LINK_STATE)
        {
            need_debounce_delay = RT_TRUE;
            clear_port_feature(hub->hdev, port1,
                    USB_PORT_FEAT_C_PORT_LINK_STATE);
        }

        /* We can forget about a "removed" device when there's a
         * physical disconnect or the connect status changes.
         */
        if (!(portstatus & USB_PORT_STAT_CONNECTION) ||
                (portchange & USB_PORT_STAT_C_CONNECTION))
            {
            clear_bit(port1, hub->removed_bits);
            }

        if (!udev)
        {
            /* Tell khubd to disconnect the device or
             * check for a new connection
             */
            if (portstatus & USB_PORT_STAT_CONNECTION)
                {
                    set_bit(port1, hub->change_bits);
                }


        }
    }

    /* If no port-status-change flags were set, we don't need any
     * debouncing.  If flags were set we can try to debounce the
     * ports all at once right now, instead of letting khubd do them
     * one at a time later on.
     *
     * If any port-status changes do occur during this delay, khubd
     * will see them later and handle them normally.
     */
    if (need_debounce_delay)
    {
        rt_thread_delay(HUB_DEBOUNCE_STABLE/(1000/RT_TICK_PER_SECOND));

    }

    hub->quiescing = 0;

    status = usb_submit_urb(hub->urb, 0);
    if (status < 0)
        RT_DEBUG_LOG(rt_debug_usb, ("activate --> %d\n", status));
    if (hub->has_indicators && blinkenlights)
        rt_timer_start(hub->leds);

    /* Scan all ports that need attention */
    kick_khubd(hub);

}


static void hub_quiesce(struct usb_hub *hub)
{
    struct usb_device *hdev = hub->hdev;
    int i;


    /* khubd and related activity won't re-trigger */
    hub->quiescing = 1;

        /* Disconnect all the children */
        for (i = 0; i < hdev->maxchild; ++i)
        {
            if (hdev->children[i])
                usb_disconnect(&hdev->children[i]);
        }

    /* Stop khubd and related activity */
    usb_kill_urb(hub->urb);
    if (hub->leds)
        rt_timer_delete(hub->leds);
/* if (hub->tt.hub) */
/* cancel_work_sync(&hub->tt.clear_work); */
}

static int hub_configure(struct usb_hub *hub,
    struct usb_endpoint_descriptor *endpoint)
{
    struct usb_hcd *hcd;
    struct usb_device *hdev = hub->hdev;
    rt_uint16_t hubstatus, hubchange;
    rt_uint16_t wHubCharacteristics;
    unsigned int pipe;
    int maxp, ret;
    char *message = "out of memory";

    hub->buffer = rt_malloc(sizeof(*hub->buffer));
    RT_DEBUG_LOG(rt_debug_usb, ("rt_malloc hub buffer:%d size:%d,addr:%x\n", g_mem_debug++, sizeof(*hub->buffer), hub->buffer));
    if (!hub->buffer)
    {
        ret = -ENOMEM;
        goto fail;
    }
    rt_memset(hub->buffer, 0, sizeof(*hub->buffer));

    hub->status = rt_malloc(sizeof(*hub->status));
    RT_DEBUG_LOG(rt_debug_usb, ("rt_malloc hub status:%d size:%d,addr :%x\n", g_mem_debug++, sizeof(*hub->status), hub->status));

    if (!hub->status)
    {
        ret = -ENOMEM;
        goto fail;
    }
    rt_memset(hub->status, 0, sizeof(*hub->status));

    hub->descriptor = rt_malloc(sizeof(*hub->descriptor));
    RT_DEBUG_LOG(rt_debug_usb, ("rt_malloc hub descriptor:%d size:%d,addr :%x\n", g_mem_debug++, sizeof(*hub->descriptor), hub->descriptor));
    if (!hub->descriptor)
    {
        ret = -ENOMEM;
        goto fail;
    }
    rt_memset(hub->descriptor, 0, sizeof(*hub->descriptor));

    if (hub_is_superspeed(hdev) && (hdev->parent != RT_NULL))
    {
        ret = usb_control_msg(hdev, usb_sndctrlpipe(hdev, 0),
                HUB_SET_DEPTH, USB_RT_HUB,
                hdev->level - 1, 0, RT_NULL, 0,
                USB_CTRL_SET_TIMEOUT);

        if (ret < 0)
        {
            message = "can't set hub depth";
            goto fail;
        }
    }

    /* Request the entire hub descriptor.
     * hub->descriptor can handle USB_MAXCHILDREN ports,
     * but the hub can/will return fewer bytes here.
     */
    void *dma_addr = RT_NULL;
    dma_addr = usb_dma_buffer_alloc(sizeof(*hub->descriptor), hdev->bus);
    ret = get_hub_descriptor(hdev, dma_addr);
    if (ret < 0)
    {
        message = "can't read hub descriptor";
        usb_dma_buffer_free(dma_addr, hdev->bus);
        goto fail;
    } else if (hub->descriptor->bNbrPorts > USB_MAXCHILDREN)
    {
        message = "hub has too many ports!";
        ret = -ENODEV;
        usb_dma_buffer_free(dma_addr, hdev->bus);
        goto fail;
    }
    rt_memcpy(hub->descriptor, dma_addr, sizeof(*hub->descriptor));
    usb_dma_buffer_free(dma_addr, hdev->bus);
    hdev->maxchild = hub->descriptor->bNbrPorts;
    RT_DEBUG_LOG(rt_debug_usb, ("%d port%s detected\n", hdev->maxchild,
        (hdev->maxchild == 1) ? "" : "s"));

    hub->port_owners = rt_malloc(hdev->maxchild * sizeof(void *));
    RT_DEBUG_LOG(rt_debug_usb, ("rt_malloc port_owners:%d size:%d addr:%x\n", g_mem_debug++, hdev->maxchild * sizeof(void *), hub->port_owners));
    if (!hub->port_owners)
    {
        ret = -ENOMEM;
        goto fail;
    }
    rt_memset(hub->port_owners, 0, hdev->maxchild * sizeof(void *));
    wHubCharacteristics = hub->descriptor->wHubCharacteristics;

    /* FIXME for USB 3.0, skip for now */
/* if ((wHubCharacteristics & HUB_CHAR_COMPOUND) && */
/* !(hub_is_superspeed(hdev))) { */
/* int    i; */
/* char    portstr [USB_MAXCHILDREN + 1]; */

/* for (i = 0; i < hdev->maxchild; i++) */
/* portstr[i] = hub->descriptor->u.hs.DeviceRemovable */
/* [((i + 1) / 8)] & (1 << ((i + 1) % 8)) */
/* ? 'F' : 'R'; */
/* portstr[hdev->maxchild] = 0; */
/* RT_DEBUG_LOG(rt_debug_usb, ("compound device; port removable status: %s\n", portstr)); */
/* } else */
/* RT_DEBUG_LOG(rt_debug_usb, ("standalone hub\n")); */

    switch (wHubCharacteristics & HUB_CHAR_LPSM)
    {
    case 0x00:
            RT_DEBUG_LOG(rt_debug_usb, ("ganged power switching\n"));
            break;
    case 0x01:
            RT_DEBUG_LOG(rt_debug_usb, ("individual port power switching\n"));
            break;
    case 0x02:
    case 0x03:
            RT_DEBUG_LOG(rt_debug_usb, ("no power switching (usb 1.0)\n"));
            break;
    }

    switch (wHubCharacteristics & HUB_CHAR_OCPM)
    {
    case 0x00:
            RT_DEBUG_LOG(rt_debug_usb, ("global over-current protection\n"));
            break;
    case 0x08:
            RT_DEBUG_LOG(rt_debug_usb, ("individual port over-current protection\n"));
            break;
    case 0x10:
    case 0x18:
            RT_DEBUG_LOG(rt_debug_usb, ("no over-current protection\n"));
                        break;
    }

/* spin_lock_init (&hub->tt.lock); */
/* INIT_LIST_HEAD (&hub->tt.clear_list); */
/* INIT_WORK(&hub->tt.clear_work, hub_tt_work); */
    switch (hdev->descriptor.bDeviceProtocol)
    {
    case 0:
            break;
    case 1:
            RT_DEBUG_LOG(rt_debug_usb, ("Single TT\n"));
            /* hub->tt.hub = hdev; */
            break;
    case 2:
            ret = usb_set_interface(hdev, 0, 1);
            if (ret == 0)
            {
                RT_DEBUG_LOG(rt_debug_usb, ("TT per port\n"));
                /* hub->tt.multi = 1; */
            }
                        else
                RT_DEBUG_LOG(rt_debug_usb, ("Using single TT (err %d)\n",
                    ret));
            /* hub->tt.hub = hdev; */
            break;
    case 3:
            /* USB 3.0 hubs don't have a TT */
            break;
        default:
            RT_DEBUG_LOG(rt_debug_usb, ("Unrecognized hub protocol %d\n",
                hdev->descriptor.bDeviceProtocol));
            break;
    }

#if 0
    /* Note 8 FS bit times == (8 bits / 12000000 bps) ~= 666ns */
    switch (wHubCharacteristics & HUB_CHAR_TTTT)
    {
        case HUB_TTTT_8_BITS:
            if (hdev->descriptor.bDeviceProtocol != 0)
            {
                hub->tt.think_time = 666;
                RT_DEBUG_LOG(rt_debug_usb, ("TT requires at most %d FS bit times (%d ns)\n",
                    8, hub->tt.think_time));
            }
            break;
        case HUB_TTTT_16_BITS:
            hub->tt.think_time = 666 * 2;
            RT_DEBUG_LOG(rt_debug_usb, ("TT requires at most %d FS bit times (%d ns)\n",
                16, hub->tt.think_time));
            break;
        case HUB_TTTT_24_BITS:
            hub->tt.think_time = 666 * 3;
            RT_DEBUG_LOG(rt_debug_usb, ("TT requires at most %d FS bit times (%d ns)\n",
                24, hub->tt.think_time));
            break;
        case HUB_TTTT_32_BITS:
            hub->tt.think_time = 666 * 4;
            RT_DEBUG_LOG(rt_debug_usb, ("TT requires at most %d FS bit times (%d ns)\n",
                32, hub->tt.think_time));
            break;
    }
#endif
    /* probe() zeroes hub->indicator[] */
    if (wHubCharacteristics & HUB_CHAR_PORTIND)
    {
        hub->has_indicators = 1;
        RT_DEBUG_LOG(rt_debug_usb, ("Port indicators are supported\n"));
    }

    RT_DEBUG_LOG(rt_debug_usb, ("power on to power good time: %dms\n",
        hub->descriptor->bPwrOn2PwrGood * 2));

    /* power budgeting mostly matters with bus-powered hubs,
     * and battery-powered root hubs (may provide just 8 mA).
     */
    ret = usb_get_status(hdev, USB_RECIP_DEVICE, 0, &hubstatus);
    if (ret < 2)
    {
        message = "can't get hub status";
        goto fail;
    }
/* le16_to_cpus(&hubstatus); */
    if (hdev == hdev->bus->root_hub)
    {
        if (hdev->bus_mA == 0 || hdev->bus_mA >= 500)
            hub->mA_per_port = 500;
        else
        {
            hub->mA_per_port = hdev->bus_mA;
            hub->limited_power = 1;
        }
    } else if ((hubstatus & (1 << USB_DEVICE_SELF_POWERED)) == 0)
    {
        RT_DEBUG_LOG(rt_debug_usb, ("hub controller current requirement: %dmA\n",
            hub->descriptor->bHubContrCurrent));
        hub->limited_power = 1;
        if (hdev->maxchild > 0)
        {
            int remaining = hdev->bus_mA -
                    hub->descriptor->bHubContrCurrent;

            if (remaining < hdev->maxchild * 100)
                RT_DEBUG_LOG(rt_debug_usb,
                    ("insufficient power available to use all downstream ports\n"
                    ));
            hub->mA_per_port = 100;        /* 7.2.1.1 */
        }
    } else    /* Self-powered external hub */
    {
        /* FIXME: What about battery-powered external hubs that
         * provide less current per port? */
        hub->mA_per_port = 500;
    }
    if (hub->mA_per_port < 500)
        RT_DEBUG_LOG(rt_debug_usb, ("%umA bus power budget for each child\n",
                hub->mA_per_port));

    /* Update the HCD's internal representation of this hub before khubd
     * starts getting port status changes for devices under the hub.
     */
    hcd = bus_to_hcd(hdev->bus);
    if (hcd->driver->update_hub_device)
    {
        ret = hcd->driver->update_hub_device(hcd, hdev);
        if (ret < 0)
        {
            message = "can't update HCD hub info";
            goto fail;
        }
    }

    ret = hub_hub_status(hub, &hubstatus, &hubchange);
    if (ret < 0)
    {
        message = "can't get hub status";
        goto fail;
    }

    /* local power status reports aren't always correct */
    if (hdev->actconfig->desc.bmAttributes & USB_CONFIG_ATT_SELFPOWER)
        RT_DEBUG_LOG(rt_debug_usb, ("local power source is %s\n",
            (hubstatus & HUB_STATUS_LOCAL_POWER)
            ? "lost (inactive)" : "good"));

    if ((wHubCharacteristics & HUB_CHAR_OCPM) == 0)
        RT_DEBUG_LOG(rt_debug_usb, ("%sover-current condition exists\n",
            (hubstatus & HUB_STATUS_OVERCURRENT) ? "" : "no "));

    /* set up the interrupt endpoint
     * We use the EP's maxpacket size instead of (PORTS+1+7)/8
     * bytes as USB2.0[11.12.3] says because some hubs are known
     * to send more data (and thus cause overflow). For root hubs,
     * maxpktsize is defined in hcd.c's fake endpoint descriptors
     * to be big enough for at least USB_MAXCHILDREN ports. */
    pipe = usb_rcvintpipe(hdev, endpoint->bEndpointAddress);
    maxp = usb_maxpacket(hdev, pipe, usb_pipeout(pipe));

    if (maxp > sizeof(*hub->buffer))
        maxp = sizeof(*hub->buffer);

    hub->urb = usb_alloc_urb(0, 0);
    if (!hub->urb)
    {
        ret = -ENOMEM;
        goto fail;
    }

    usb_fill_int_urb(hub->urb, hdev, pipe, *hub->buffer, maxp, hub_irq,
        hub, endpoint->bInterval);

    /* maybe cycle the hub leds */
    if (hub->has_indicators && blinkenlights)
        hub->indicator[0] = INDICATOR_CYCLE;



    hub_activate(hub);
    return 0;

fail:
    rt_kprintf("config failed, %s (err %d)\n",
            message, ret);
    /* hub_disconnect() frees urb and descriptor */
    return ret;
}

static void hub_release(struct usb_hub *hub)
{
    /* struct usb_hub *hub = container_of(kref, struct usb_hub, kref); */
    hub->kref--;
    /* usb_put_intf(to_usb_interface(hub->intfdev)); */
    /* RT_DEBUG_LOG(rt_debug_usb,("rt_free hub:%d,addr:%x\n",--g_mem_debug,hub)); */
    if (hub->kref <= 0)
    {
        RT_DEBUG_LOG(rt_debug_usb, ("rt_free hub:%d,addr:%x\n", --g_mem_debug, hub));
        rt_free(hub);
}
}


static unsigned int highspeed_hubs;

static void hub_disconnect(struct usb_interface *intf)
{
    struct usb_hub *hub = intf->user_data;
    rt_base_t level;
    /* Take the hub off the event list and don't let it be added again */

    if (hub == RT_NULL)
        return;
    /* <-tangyh*/
    level = rt_hw_interrupt_disable();
    rt_enter_critical();
    /* spin_lock_irq(&hub_event_lock); */
    /* tangyh ->*/

    if (!rt_list_isempty(&hub->event_list))
    {
        rt_list_remove(&hub->event_list);
    }
    hub->disconnected = 1;

     /* <-tangyh*/
     rt_hw_interrupt_enable(level);
     rt_exit_critical();
    /* spin_unlock_irq(&hub_event_lock);; */
    /* tangyh ->*/


    /* Disconnect all children and quiesce the hub */
    hub->error = 0;
    hub_quiesce(hub);

    /* usb_set_intfdata (intf, NULL); */
    intf->user_data = RT_NULL;
    hub->hdev->maxchild = 0;

    if (hub->hdev->speed == USB_SPEED_HIGH)
        highspeed_hubs--;

    if (hub->urb)
    usb_free_urb(hub->urb);

    RT_DEBUG_LOG(rt_debug_usb, ("rt_free hub port_owners:%d,addr:%x\n", --g_mem_debug, hub->port_owners));
    RT_DEBUG_LOG(rt_debug_usb, ("rt_free hub descriptor:%d,addr:%x\n", --g_mem_debug, hub->descriptor));
    RT_DEBUG_LOG(rt_debug_usb, ("rt_free hub status:%d,addr:%x\n", --g_mem_debug, hub->status));
    RT_DEBUG_LOG(rt_debug_usb, ("rt_free hub buffer:%d,addr:%x\n", --g_mem_debug, hub->buffer));
    rt_free(hub->port_owners);
    rt_free(hub->descriptor);
    rt_free(hub->status);
    rt_free(hub->buffer);

    hub_release(hub);

    if (rt_device_find(intf->dev.parent.name))
    rt_device_unregister(&intf->dev);
}

static int hub_probe(struct usb_interface *intf, const struct usb_device_id *id)
{
    struct usb_host_interface *desc = RT_NULL;
    struct usb_endpoint_descriptor *endpoint = RT_NULL;
    struct usb_device *hdev = RT_NULL;
    struct usb_hub *hub = RT_NULL;

    desc = intf->cur_altsetting;
    hdev = intf->usb_dev;

    if (hdev->level == MAX_TOPO_LEVEL)
    {
        RT_DEBUG_LOG(rt_debug_usb,
            ("Unsupported bus topology: hub nested too deep\n"));
        return -E2BIG;
    }

#ifdef    CONFIG_USB_OTG_BLACKLIST_HUB
    if (hdev->parent)
    {
        RT_DEBUG_LOG(rt_debug_usb, ("ignoring external hub\n"));
        return -ENODEV;
    }
#endif

    /* Some hubs have a subclass of 1, which AFAICT according to the */
    /*  specs is not defined, but it works */
    if ((desc->desc.bInterfaceSubClass != 0) &&
        (desc->desc.bInterfaceSubClass != 1)) {
descriptor_error:
        RT_DEBUG_LOG(rt_debug_usb, ("bad descriptor, ignoring hub\n"));
        return -EIO;
    }

    /* Multiple endpoints? What kind of mutant ninja-hub is this? */
    if (desc->desc.bNumEndpoints != 1)
        goto descriptor_error;

    endpoint = &desc->endpoint[0].desc;

    /* If it's not an interrupt in endpoint, we'd better punt! */
    if (!usb_endpoint_is_int_in(endpoint))
        goto descriptor_error;

    /* We found a hub */
    RT_DEBUG_LOG(rt_debug_usb, ("USB hub found\n"));

    hub = rt_malloc(sizeof(*hub));
    RT_DEBUG_LOG(rt_debug_usb, ("rt_malloc hub :%d size:%d\n", g_mem_debug++, sizeof(*hub), hub));
    if (!hub)
    {
        RT_DEBUG_LOG(rt_debug_usb, ("couldn't kmalloc hub struct\n"));
        return -ENOMEM;
    }
    rt_memset(hub, 0, sizeof(*hub));

    hub->kref = 1;
    rt_list_init(&hub->event_list);
    intf->user_data = hub;
    hub->hdev = hdev;

    hub->leds = rt_timer_create(intf->dev.parent.name, led_work, (void *)hub, LED_CYCLE_PERIOD, RT_TIMER_FLAG_PERIODIC|RT_TIMER_FLAG_SOFT_TIMER);

/* intf->needs_remote_wakeup = 1; */

    if (hdev->speed == USB_SPEED_HIGH)
        highspeed_hubs++;

    if (hub_configure(hub, endpoint) >= 0)
    {
        rt_device_register(&intf->dev, intf->dev.parent.name, RT_DEVICE_FLAG_REMOVABLE);
        return 0;
    }

    hub_disconnect(intf);
    return -ENODEV;
}


/*
 * Allow user programs to claim ports on a hub.  When a device is attached
 * to one of these "claimed" ports, the program will "own" the device.
 */
static int find_port_owner(struct usb_device *hdev, unsigned int port1,
        void ***ppowner)
{
    if (hdev->state == USB_STATE_NOTATTACHED)
        return -ENODEV;
    if (port1 == 0 || port1 > hdev->maxchild)
        return -EINVAL;

    /* This assumes that devices not managed by the hub driver
     * will always have maxchild equal to 0.
     */
    *ppowner = &(hdev_to_hub(hdev)->port_owners[port1 - 1]);
    return 0;
}

/* In the following three functions, the caller must hold hdev's lock */
int usb_hub_claim_port(struct usb_device *hdev, unsigned int port1, void *owner)
{
    int rc;
    void **powner;

    rc = find_port_owner(hdev, port1, &powner);
    if (rc)
        return rc;
    if (*powner)
        return -EBUSY;
    *powner = owner;
    return rc;
}

int usb_hub_release_port(struct usb_device *hdev, unsigned int port1, void *owner)
{
    int rc;
    void **powner;

    rc = find_port_owner(hdev, port1, &powner);
    if (rc)
        return rc;
    if (*powner != owner)
        return -ENOENT;
    *powner = RT_NULL;
    return rc;
}

void usb_hub_release_all_ports(struct usb_device *hdev, void *owner)
{
    int n;
    void **powner;

    n = find_port_owner(hdev, 1, &powner);
    if (n == 0)
    {
        for (; n < hdev->maxchild; (++n, ++powner))
        {
            if (*powner == owner)
                *powner = RT_NULL;
        }
    }
}

/* The caller must hold udev's lock */
rt_bool_t usb_device_is_owned(struct usb_device *udev)
{
    struct usb_hub *hub;

    if (udev->state == USB_STATE_NOTATTACHED || !udev->parent)
        return RT_FALSE;
    hub = hdev_to_hub(udev->parent);
    return !!hub->port_owners[udev->portnum - 1];
}


static void recursively_mark_NOTATTACHED(struct usb_device *udev)
{
    int i;

    for (i = 0; i < udev->maxchild; ++i)
    {
        if (udev->children[i])
            recursively_mark_NOTATTACHED(udev->children[i]);
    }
/* if (udev->state == USB_STATE_SUSPENDED) */
/* udev->active_duration -= jiffies; */
    udev->state = USB_STATE_NOTATTACHED;
}

/**
 * usb_set_device_state - change a device's current state (usbcore, hcds)
 * @udev: pointer to device whose state should be changed
 * @new_state: new state value to be stored
 *
 * udev->state is _not_ fully protected by the device lock.  Although
 * most transitions are made only while holding the lock, the state can
 * can change to USB_STATE_NOTATTACHED at almost any time.  This
 * is so that devices can be marked as disconnected as soon as possible,
 * without having to wait for any semaphores to be released.  As a result,
 * all changes to any device's state must be protected by the
 * device_state_lock spinlock.
 *
 * Once a device has been added to the device tree, all changes to its state
 * should be made using this routine.  The state should _not_ be set directly.
 *
 * If udev->state is already USB_STATE_NOTATTACHED then no change is made.
 * Otherwise udev->state is set to new_state, and if new_state is
 * USB_STATE_NOTATTACHED then all of udev's descendants' states are also set
 * to USB_STATE_NOTATTACHED.
 */
void usb_set_device_state(struct usb_device *udev,
        enum usb_device_state new_state)
{
    rt_base_t level;
/* int wakeup = -1; */

   /* <-tangyh*/
    level = rt_hw_interrupt_disable();
    rt_enter_critical();
    /* spin_lock_irqsave(&device_state_lock, flags); */
    /* tangyh ->*/
    if (udev->state == USB_STATE_NOTATTACHED)
        ;    /* do nothing */
    else if (new_state != USB_STATE_NOTATTACHED)
    {

        udev->state = new_state;
    }
        else
        recursively_mark_NOTATTACHED(udev);
    /* <-tangyh*/
        rt_hw_interrupt_enable(level);
        rt_exit_critical();
    /* spin_unlock_irqrestore(&device_state_lock, flags); */
    /* tangyh ->*/
/* if (wakeup >= 0) */
/* device_set_wakeup_capable(&udev->dev, wakeup); */
}


/*
 * Choose a device number.
 *
 * Device numbers are used as filenames in usbfs.  On USB-1.1 and
 * USB-2.0 buses they are also used as device addresses, however on
 * USB-3.0 buses the address is assigned by the controller hardware
 * and it usually is not the same as the device number.
 *
 * WUSB devices are simple: they have no hubs behind, so the mapping
 * device <-> virtual port number becomes 1:1. Why? to simplify the
 * life of the device connection logic in
 * drivers/usb/wusbcore/devconnect.c. When we do the initial secret
 * handshake we need to assign a temporary address in the unauthorized
 * space. For simplicity we use the first virtual port number found to
 * be free [drivers/usb/wusbcore/devconnect.c:wusbhc_devconnect_ack()]
 * and that becomes it's address [X < 128] or its unauthorized address
 * [X | 0x80].
 *
 * We add 1 as an offset to the one-based USB-stack port number
 * (zero-based wusb virtual port index) for two reasons: (a) dev addr
 * 0 is reserved by USB for default address; (b) Linux's USB stack
 * uses always #1 for the root hub of the controller. So USB stack's
 * port #1, which is wusb virtual-port #0 has address #2.
 *
 * Devices connected under xHCI are not as simple.  The host controller
 * supports virtualization, so the hardware assigns device addresses and
 * the HCD must setup data structures before issuing a set address
 * command to the hardware.
 */
static void choose_devnum(struct usb_device *udev)
{
    int        devnum;
    struct usb_bus    *bus = udev->bus;

    /* If khubd ever becomes multithreaded, this will need a lock */
    if (udev->wusb)
    {
        devnum = udev->portnum + 1;
        RT_ASSERT(!test_bit(devnum, bus->devmap.devicemap));
    } else
    {
        /* Try to allocate the next devnum beginning at
         * bus->devnum_next. */
        devnum = find_next_zero_bit(bus->devmap.devicemap, 128,
                        bus->devnum_next);
/* if (devnum >= 128) */
/* devnum = find_next_zero_bit(bus->devmap.devicemap, */
/* 128, 1); */
        bus->devnum_next = (devnum >= 127 ? 1 : devnum + 1);
    }
    if (devnum < 128)
    {
        set_bit(devnum, bus->devmap.devicemap);
        udev->devnum = devnum;
    }
}

static void release_devnum(struct usb_device *udev)
{
    if (udev->devnum > 0)
    {
        clear_bit(udev->devnum, udev->bus->devmap.devicemap);
        udev->devnum = -1;
    }
}

static void update_devnum(struct usb_device *udev, int devnum)
{
    /* The address for a WUSB device is managed by wusbcore. */
    if (!udev->wusb)
        udev->devnum = devnum;
}

static void hub_free_dev(struct usb_device *udev)
{
    struct usb_hcd *hcd = bus_to_hcd(udev->bus);

    /* Root hubs aren't real devices, so don't free HCD resources */
    if (hcd->driver->free_dev && udev->parent)
        hcd->driver->free_dev(hcd, udev);
}

/**
 * usb_disconnect - disconnect a device (usbcore-internal)
 * @pdev: pointer to device being disconnected
 * Context: !in_interrupt ()
 *
 * Something got disconnected. Get rid of it and all of its children.
 *
 * If *pdev is a normal device then the parent hub must already be locked.
 * If *pdev is a root hub then this routine will acquire the
 * usb_bus_list_lock on behalf of the caller.
 *
 * Only hub drivers (including virtual root hub drivers for host
 * controllers) should ever call this.
 *
 * This call is synchronous, and may not be used in an interrupt context.
 */
void usb_disconnect(struct usb_device **pdev)
{
    struct usb_device    *udev = *pdev;
    int            i;
    struct usb_hcd        *hcd = RT_NULL;
    rt_base_t level;

    if (!udev)
    {
        RT_DEBUG_LOG(rt_debug_usb, ("%s nodev\n"));
        return;
    }
    hcd = bus_to_hcd(udev->bus);
    /* mark the device as inactive, so any further urb submissions for
     * this device (and any of its children) will fail immediately.
     * this quiesces everything except pending urbs.
     */
    usb_set_device_state(udev, USB_STATE_NOTATTACHED);
    RT_DEBUG_LOG(rt_debug_usb, ("USB disconnect, device number %d\n",
            udev->devnum));


    /* Free up all the children before we remove this device */
    for (i = 0; i < USB_MAXCHILDREN; i++)
    {
        if (udev->children[i])
            usb_disconnect(&udev->children[i]);
    }

    /* deallocate hcd/hardware state ... nuking all pending urbs and
     * cleaning up all state associated with the current configuration
     * so that the hardware is now fully quiesced.
     */
    RT_DEBUG_LOG(rt_debug_usb, ("unregistering device\n"));
    rt_mutex_take(hcd->bandwidth_mutex, RT_WAITING_FOREVER);
    usb_disable_device(udev, 0);
    rt_mutex_release(hcd->bandwidth_mutex);
/* usb_hcd_synchronize_unlinks(udev); */

    /* Unregister the device.  The device driver is responsible
     * for de-configuring the device and invoking the remove-device
     * notifier chain (used by usbfs and possibly others).
     */
/* rt_device_unregister(&udev->dev); */

    /* Free the device number and delete the parent's children[]
     * (or root_hub) pointer.
     */
    release_devnum(udev);

    /* Avoid races with recursively_mark_NOTATTACHED() */
    /* <-tangyh*/
    level = rt_hw_interrupt_disable();
    rt_enter_critical();
    /* spin_lock_irq(&device_state_lock); */
    /* tangyh->*/
    *pdev = RT_NULL;

    /* <-tangyh*/
        rt_hw_interrupt_enable(level);
        rt_exit_critical();
    /* spin_unlock_irq(&device_state_lock); */
    /* tangyh ->*/


    hub_free_dev(udev);

    usb_release_dev(udev);

/* put_device(&udev->dev); */
}

#if 0
static void show_string(struct usb_device *udev, char *id, char *string)
{
    if (!string)
        return;
    rt_kprintf("%s: %s\n", id, string);
}
#endif

static void announce_device(struct usb_device *udev)
{
    RT_DEBUG_LOG(rt_debug_usb, ("New USB device found, idVendor=%04x, idProduct=%04x\n",
        (udev->descriptor.idVendor),
        (udev->descriptor.idProduct)));
    RT_DEBUG_LOG(rt_debug_usb,
        ("New USB device strings: Mfr=%d, Product=%d, SerialNumber=%d\n",
        udev->descriptor.iManufacturer,
        udev->descriptor.iProduct,
        udev->descriptor.iSerialNumber));
/* show_string(udev, "Product", udev->product); */
/* show_string(udev, "Manufacturer", udev->manufacturer); */
/* show_string(udev, "SerialNumber", udev->serial); */
}

#ifdef    CONFIG_USB_OTG
#include "otg_whitelist.h"
#endif

/**
 * usb_enumerate_device_otg - FIXME (usbcore-internal)
 * @udev: newly addressed device (in ADDRESS state)
 *
 * Finish enumeration for On-The-Go devices
 */
static int usb_enumerate_device_otg(struct usb_device *udev)
{
    int err = 0;

#ifdef    CONFIG_USB_OTG
    /*
     * OTG-aware devices on OTG-capable root hubs may be able to use SRP,
     * to wake us after we've powered off VBUS; and HNP, switching roles
     * "host" to "peripheral".  The OTG descriptor helps figure this out.
     */
    if (!udev->bus->is_b_host
            && udev->config
            && udev->parent == udev->bus->root_hub) {
        struct usb_otg_descriptor    *desc = NULL;
        struct usb_bus            *bus = udev->bus;

        /* descriptor may appear anywhere in config */
        if (__usb_get_extra_descriptor(udev->rawdescriptors[0],
                    (udev->config[0].desc.wTotalLength),
                    USB_DT_OTG, (void **) &desc) == 0) {
            if (desc->bmAttributes & USB_OTG_HNP)
            {
                unsigned int port1 = udev->portnum;

                RT_DEBUG_LOG(rt_debug_usb,
                    ("Dual-Role OTG device on %sHNP port\n",
                    (port1 == bus->otg_port)
                        ? "" : "non-"));

                /* enable HNP before suspend, it's simpler */
                if (port1 == bus->otg_port)
                    bus->b_hnp_enable = 1;
                err = usb_control_msg(udev,
                    usb_sndctrlpipe(udev, 0),
                    USB_REQ_SET_FEATURE, 0,
                    bus->b_hnp_enable
                        ? USB_DEVICE_B_HNP_ENABLE
                        : USB_DEVICE_A_ALT_HNP_SUPPORT,
                    0, NULL, 0, USB_CTRL_SET_TIMEOUT);
                if (err < 0)
                {
                    /* OTG MESSAGE: report errors here,
                     * customize to match your product.
                     */
                    RT_DEBUG_LOG(rt_debug_usb,
                        ("can't set HNP mode: %d\n",
                        err));
                    bus->b_hnp_enable = 0;
                }
            }
        }
    }

    if (!is_targeted(udev))
    {

        err = -ENOTSUPP;
        goto fail;
    }
fail:
#endif
    return err;
}


/**
 * usb_enumerate_device - Read device configs/intfs/otg (usbcore-internal)
 * @udev: newly addressed device (in ADDRESS state)
 *
 * This is only called by usb_new_device() and usb_authorize_device()
 * and FIXME -- all comments that apply to them apply here wrt to
 * environment.
 *
 * If the device is WUSB and not authorized, we don't attempt to read
 * the string descriptors, as they will be errored out by the device
 * until it has been authorized.
 */
static int usb_enumerate_device(struct usb_device *udev)
{
    int err;

    if (udev->config == RT_NULL)
    {
        err = usb_get_configuration(udev);
        if (err < 0)
        {
            RT_DEBUG_LOG(rt_debug_usb, ("can't read configurations, error %d\n",
                err));
            goto fail;
        }
    }
    err = usb_enumerate_device_otg(udev);
fail:
    return err;
}


/**
 * usb_new_device - perform initial device setup (usbcore-internal)
 * @udev: newly addressed device (in ADDRESS state)
 *
 * This is called with devices which have been detected but not fully
 * enumerated.  The device descriptor is available, but not descriptors
 * for any device configuration.  The caller must have locked either
 * the parent hub (if udev is a normal device) or else the
 * usb_bus_list_lock (if udev is a root hub).  The parent's pointer to
 * udev has already been installed, but udev is not yet visible through
 * sysfs or other filesystem code.
 *
 * It will return if the device is configured properly or not.  Zero if
 * the interface was registered with the driver core; else a negative
 * errno value.
 *
 * This call is synchronous, and may not be used in an interrupt context.
 *
 * Only the hub driver or root-hub registrar should ever call this.
 */
int usb_new_device(struct usb_device *udev)
{
    int err;

    err = usb_enumerate_device(udev);    /* Read descriptors */
    if (err < 0)
        goto fail;
    RT_DEBUG_LOG(rt_debug_usb, ("udev %d, busnum %d, minor = %d\n",
            udev->devnum, udev->bus->busnum,
            (((udev->bus->busnum-1) * 128) + (udev->devnum-1))));

    /* Tell the world! */
    announce_device(udev);

    /* Register the device.  The device driver is responsible
     * for configuring the device and invoking the add-device
     * notifier chain (used by usbfs and possibly others).
     */
    err = generic_probe(udev);
    if (err)
    {
        RT_DEBUG_LOG(rt_debug_usb, ("can't device_add, error %d\n", err));
        goto fail;
    }

    return err;

fail:
    usb_set_device_state(udev, USB_STATE_NOTATTACHED);
    return err;
}


/* Returns 1 if @hub is a WUSB root hub, 0 otherwise */
static unsigned int hub_is_wusb(struct usb_hub *hub)
{
    //struct usb_hcd *hcd;

    //if (hub->hdev->parent != RT_NULL)  /* not a root hub? */
        return 0;
   // hcd = (struct usb_hcd *)hub;
    //return hcd->wireless;
}


#define PORT_RESET_TRIES    5
#define SET_ADDRESS_TRIES    2
#define GET_DESCRIPTOR_TRIES    2
#define SET_CONFIG_TRIES    (2 * (use_both_schemes + 1))
#define USE_NEW_SCHEME(i)    ((i) / 2 == old_scheme_first)

#define HUB_ROOT_RESET_TIME    50    /* times are in msec */
#define HUB_SHORT_RESET_TIME    10
#define HUB_LONG_RESET_TIME    200
#define HUB_RESET_TIMEOUT    500

static int hub_port_wait_reset(struct usb_hub *hub, int port1,
                struct usb_device *udev, unsigned int delay)
{
    int delay_time, ret;
    rt_uint16_t portstatus = 0;
    rt_uint16_t portchange = 0;

    for (delay_time = 0;
            delay_time < HUB_RESET_TIMEOUT;
            delay_time += delay) {
        /* wait to give the device a chance to reset */
        rt_thread_delay(delay/10);

        /* read and decode port status */
        ret = hub_port_status(hub, port1, &portstatus, &portchange);
        if (ret < 0)
            return ret;

        /* Device went away? */
        if (!(portstatus & USB_PORT_STAT_CONNECTION))
            return -ENOTCONN;

        /* bomb out completely if the connection bounced */
        if ((portchange & USB_PORT_STAT_C_CONNECTION))
            return -ENOTCONN;

        /* if we`ve finished resetting, then break out of the loop */
        if (!(portstatus & USB_PORT_STAT_RESET) &&
            (portstatus & USB_PORT_STAT_ENABLE)) {
            if (hub_is_wusb(hub))
                udev->speed = USB_SPEED_WIRELESS;
            else if (hub_is_superspeed(hub->hdev))
                udev->speed = USB_SPEED_SUPER;
            else if (portstatus & USB_PORT_STAT_HIGH_SPEED)
                udev->speed = USB_SPEED_HIGH;
            else if (portstatus & USB_PORT_STAT_LOW_SPEED)
                udev->speed = USB_SPEED_LOW;
            else
                udev->speed = USB_SPEED_FULL;
            return 0;
        }

        /* switch to the long delay after two short delay failures */
        if (delay_time >= 2 * HUB_SHORT_RESET_TIME)
            delay = HUB_LONG_RESET_TIME;

        RT_DEBUG_LOG(rt_debug_usb,
            ("port %d not reset yet, waiting %dms\n",
            port1, delay));
    }

    return -EBUSY;
}


static int hub_port_reset(struct usb_hub *hub, int port1,
                struct usb_device *udev, unsigned int delay)
{
    int i, status;
    struct usb_hcd *hcd;

    hcd = bus_to_hcd(udev->bus);
    /* Block EHCI CF initialization during the port reset.
     * Some companion controllers don't like it when they mix.
     */
/* down_read(&ehci_cf_port_reset_rwsem); */

    /* Reset the port */
    for (i = 0; i < PORT_RESET_TRIES; i++)
    {
        status = set_port_feature(hub->hdev,
                port1, USB_PORT_FEAT_RESET);
        if (status)
            RT_DEBUG_LOG(rt_debug_usb,
                    ("cannot reset port %d (err = %d)\n",
                    port1, status));
        else
        {
            status = hub_port_wait_reset(hub, port1, udev, delay);
            if (status && status != -ENOTCONN)
                RT_DEBUG_LOG(rt_debug_usb,
                        ("port_wait_reset: err = %d\n",
                        status));
        }

        /* return on disconnect or reset */
        switch (status)
        {
        case 0:
            /* TRSTRCY = 10 ms; plus some extra */
            rt_thread_delay(5);
            update_devnum(udev, 0);
            if (hcd->driver->reset_device)
            {
                status = hcd->driver->reset_device(hcd, udev);
                if (status < 0)
                {
                    RT_DEBUG_LOG(rt_debug_usb, ("Cannot reset HCD device state\n"
                            ));
                    break;
                }
            }
            /* FALL THROUGH */
        case -ENOTCONN:
        case -ENODEV:
            clear_port_feature(hub->hdev,
                port1, USB_PORT_FEAT_C_RESET);
            /* FIXME need disconnect() for NOTATTACHED device */
            usb_set_device_state(udev, status
                    ? USB_STATE_NOTATTACHED
                    : USB_STATE_DEFAULT);
            goto done;
        }

        RT_DEBUG_LOG(rt_debug_usb,
            ("port %d not enabled, trying reset again...\n",
            port1));
        delay = HUB_LONG_RESET_TIME;
    }

    RT_DEBUG_LOG(rt_debug_usb,
        ("Cannot enable port %i.  Maybe the USB cable is bad?\n",
        port1));

 done:
/* up_read(&ehci_cf_port_reset_rwsem); */
    return status;
}

/* Warm reset a USB3 protocol port */
static int hub_port_warm_reset(struct usb_hub *hub, int port)
{
    int ret;
    rt_uint16_t portstatus, portchange;

    if (!hub_is_superspeed(hub->hdev))
    {
        RT_DEBUG_LOG(rt_debug_usb, ("only USB3 hub support warm reset\n"));
        return -EINVAL;
    }

    /* Warm reset the port */
    ret = set_port_feature(hub->hdev,
                port, USB_PORT_FEAT_BH_PORT_RESET);
    if (ret)
    {
        RT_DEBUG_LOG(rt_debug_usb, ("cannot warm reset port %d\n", port));
        return ret;
    }

    rt_thread_delay(2);
    ret = hub_port_status(hub, port, &portstatus, &portchange);

    if (portchange & USB_PORT_STAT_C_RESET)
        clear_port_feature(hub->hdev, port, USB_PORT_FEAT_C_RESET);

    if (portchange & USB_PORT_STAT_C_BH_RESET)
        clear_port_feature(hub->hdev, port,
                    USB_PORT_FEAT_C_BH_PORT_RESET);

    if (portchange & USB_PORT_STAT_C_LINK_STATE)
        clear_port_feature(hub->hdev, port,
                    USB_PORT_FEAT_C_PORT_LINK_STATE);

    return ret;
}

/* Check if a port is power on */
static int port_is_power_on(struct usb_hub *hub, unsigned int portstatus)
{
    int ret = 0;

    if (hub_is_superspeed(hub->hdev))
    {
        if (portstatus & USB_SS_PORT_STAT_POWER)
            ret = 1;
    } else
    {
        if (portstatus & USB_PORT_STAT_POWER)
            ret = 1;
    }

    return ret;
}


/* USB 2.0 spec, 7.1.7.3 / fig 7-29:
 *
 * Between connect detection and reset signaling there must be a delay
 * of 100ms at least for debounce and power-settling.  The corresponding
 * timer shall restart whenever the downstream port detects a disconnect.
 *
 * Apparently there are some bluetooth and irda-dongles and a number of
 * low-speed devices for which this debounce period may last over a second.
 * Not covered by the spec - but easy to deal with.
 *
 * This implementation uses a 1500ms total debounce timeout; if the
 * connection isn't stable by then it returns -ETIMEDOUT.  It checks
 * every 25ms for transient disconnects.  When the port status has been
 * unchanged for 100ms it returns the port status.
 */
static int hub_port_debounce(struct usb_hub *hub, int port1)
{
    int ret;
    int total_time, stable_time = 0;
    rt_uint16_t portchange, portstatus;
    unsigned int connection = 0xffff;

    for (total_time = 0; ; total_time += HUB_DEBOUNCE_STEP)
    {
        ret = hub_port_status(hub, port1, &portstatus, &portchange);
        if (ret < 0)
            return ret;

        if (!(portchange & USB_PORT_STAT_C_CONNECTION) &&
             (portstatus & USB_PORT_STAT_CONNECTION) == connection) {
            stable_time += HUB_DEBOUNCE_STEP;
            if (stable_time >= HUB_DEBOUNCE_STABLE)
                break;
        } else
        {
            stable_time = 0;
            connection = portstatus & USB_PORT_STAT_CONNECTION;
        }

        if (portchange & USB_PORT_STAT_C_CONNECTION)
        {
            clear_port_feature(hub->hdev, port1,
                    USB_PORT_FEAT_C_CONNECTION);
        }

        if (total_time >= HUB_DEBOUNCE_TIMEOUT)
            break;
        rt_thread_delay(3);
    }

    RT_DEBUG_LOG(rt_debug_usb,
        ("debounce: port %d: total %dms stable %dms status 0x%x\n",
        port1, total_time, stable_time, portstatus));

    if (stable_time < HUB_DEBOUNCE_STABLE)
        return -ETIMEDOUT;
    return portstatus;
}

void usb_ep0_reinit(struct usb_device *udev)
{
    usb_disable_endpoint(udev, 0 + USB_DIR_IN, RT_TRUE);
    usb_disable_endpoint(udev, 0 + USB_DIR_OUT, RT_TRUE);
    usb_enable_endpoint(udev, &udev->ep0, RT_TRUE);
}

#define usb_sndaddr0pipe()    (PIPE_CONTROL << 30)
#define usb_rcvaddr0pipe()    ((PIPE_CONTROL << 30) | USB_DIR_IN)

static int hub_set_address(struct usb_device *udev, int devnum)
{
    int retval;
    struct usb_hcd *hcd = bus_to_hcd(udev->bus);

    /*
     * The host controller will choose the device address,
     * instead of the core having chosen it earlier
     */
    if (!hcd->driver->address_device && devnum <= 1)
        return -EINVAL;
    if (udev->state == USB_STATE_ADDRESS)
        return 0;
    if (udev->state != USB_STATE_DEFAULT)
        return -EINVAL;
    if (hcd->driver->address_device)
        retval = hcd->driver->address_device(hcd, udev);
    else
        retval = usb_control_msg(udev, usb_sndaddr0pipe(),
                USB_REQ_SET_ADDRESS, 0, devnum, 0,
                RT_NULL, 0, USB_CTRL_SET_TIMEOUT);
    if (retval == 0)
    {
        update_devnum(udev, devnum);
        /* Device now using proper address. */
        usb_set_device_state(udev, USB_STATE_ADDRESS);
        usb_ep0_reinit(udev);
    }
    return retval;
}

/* Reset device, (re)assign address, get device descriptor.
 * Device connection must be stable, no more debouncing needed.
 * Returns device in USB_STATE_ADDRESS, except on error.
 *
 * If this is called for an already-existing device (as part of
 * usb_reset_and_verify_device), the caller must own the device lock.  For a
 * newly detected device that is not accessible through any global
 * pointers, it's not necessary to lock the device.
 */
static int
hub_port_init(struct usb_hub *hub, struct usb_device *udev, int port1,
        int retry_counter)
{
/* static DEFINE_MUTEX(usb_address0_mutex); */

    struct usb_device    *hdev = hub->hdev;
    struct usb_hcd        *hcd = bus_to_hcd(hdev->bus);
    int            i, j, retval;
    unsigned int delay = HUB_SHORT_RESET_TIME;
    enum usb_device_speed    oldspeed = udev->speed;
    char             *speed, *type;
    int            devnum = udev->devnum;

    /* root hub ports have a slightly longer reset period
     * (from USB 2.0 spec, section 7.1.7.5)
     */
    if (!hdev->parent)
    {
        delay = HUB_ROOT_RESET_TIME;
        if (port1 == hdev->bus->otg_port)
            hdev->bus->b_hnp_enable = 0;
    }

    /* Some low speed devices have problems with the quick delay, so */
    /*  be a bit pessimistic with those devices. RHbug #23670 */
    if (oldspeed == USB_SPEED_LOW)
        delay = HUB_LONG_RESET_TIME;

/* mutex_lock(&usb_address0_mutex); */

    /* Reset the device; full speed may morph to high speed */
    /* FIXME a USB 2.0 device may morph into SuperSpeed on reset. */
    retval = hub_port_reset(hub, port1, udev, delay);
    if (retval < 0)        /* error or disconnect */
        goto fail;
    /* success, speed is known */

    retval = -ENODEV;

    if (oldspeed != USB_SPEED_UNKNOWN && oldspeed != udev->speed)
    {
        RT_DEBUG_LOG(rt_debug_usb, ("device reset changed speed!\n"));
        goto fail;
    }
    oldspeed = udev->speed;

    /* USB 2.0 section 5.5.3 talks about ep0 maxpacket ...
     * it's fixed size except for full speed devices.
     * For Wireless USB devices, ep0 max packet is always 512 (tho
     * reported as 0xff in the device descriptor). WUSB1.0[4.8.1].
     */
    switch (udev->speed)
    {
    case USB_SPEED_SUPER:
    case USB_SPEED_WIRELESS:    /* fixed at 512 */
        udev->ep0.desc.wMaxPacketSize = 512;
        break;
    case USB_SPEED_HIGH:        /* fixed at 64 */
        udev->ep0.desc.wMaxPacketSize = 64;
        break;
    case USB_SPEED_FULL:        /* 8, 16, 32, or 64 */
        /* to determine the ep0 maxpacket size, try to read
         * the device descriptor to get bMaxPacketSize0 and
         * then correct our initial guess.
         */
        udev->ep0.desc.wMaxPacketSize = 64;
        break;
    case USB_SPEED_LOW:        /* fixed at 8 */
        udev->ep0.desc.wMaxPacketSize = 8;
        break;
    default:
        goto fail;
    }

    type = "";
    switch (udev->speed)
    {
    case USB_SPEED_LOW:    speed = "low";    break;
    case USB_SPEED_FULL:    speed = "full";    break;
    case USB_SPEED_HIGH:    speed = "high";    break;
    case USB_SPEED_SUPER:
                speed = "super";
                break;
    case USB_SPEED_WIRELESS:
                speed = "variable";
                type = "Wireless ";
                break;
    default:         speed = "?";    break;
    }
    if (udev->speed != USB_SPEED_SUPER)
        rt_kprintf
                ("%s %s speed %sUSB device number %d using\n",
                (udev->config) ? "reset" : "new", speed, type,
                devnum);

    /* Set up TT records, if needed  */
/* if (hdev->tt) { */
/* udev->tt = hdev->tt; */
/* udev->ttport = hdev->ttport; */
/* } else if (udev->speed != USB_SPEED_HIGH */
/* && hdev->speed == USB_SPEED_HIGH) { */
/* if (!hub->tt.hub) { */
/* RT_DEBUG_LOG(rt_debug_usb,("parent hub has no TT\n")); */
/* retval = -EINVAL; */
/* goto fail; */
/* } */
/* udev->tt = &hub->tt; */
/* udev->ttport = port1; */
/* } */

    /* Why interleave GET_DESCRIPTOR and SET_ADDRESS this way?
     * Because device hardware and firmware is sometimes buggy in
     * this area, and this is how Linux has done it for ages.
     * Change it cautiously.
     *
     * NOTE:  If USE_NEW_SCHEME() is true we will start by issuing
     * a 64-byte GET_DESCRIPTOR request.  This is what Windows does,
     * so it may help with some non-standards-compliant devices.
     * Otherwise we start with SET_ADDRESS and then try to read the
     * first 8 bytes of the device descriptor to get the ep0 maxpacket
     * value.
     */
    for (i = 0; i < GET_DESCRIPTOR_TRIES; (++i, rt_thread_delay(10)))
    {
        if (USE_NEW_SCHEME(retry_counter) && !(hcd->driver->flags & HCD_USB3))
        {
            struct usb_device_descriptor *buf;
            int r = 0;

#define GET_DESCRIPTOR_BUFSIZE    64
            /* buf = rt_malloc(GET_DESCRIPTOR_BUFSIZE); */
            /* RT_DEBUG_LOG(rt_debug_usb,("rt_malloc GET_DESCRIPTOR_BUFSIZE:%d size:%d\n",g_mem_debug++,GET_DESCRIPTOR_BUFSIZE)); */
            buf = usb_dma_buffer_alloc(GET_DESCRIPTOR_BUFSIZE, udev->bus);
            if (!buf)
            {
                retval = -ENOMEM;
                continue;
            }
            rt_memset(buf, 0, GET_DESCRIPTOR_BUFSIZE);

            /* Retry on all errors; some devices are flakey.
             * 255 is for WUSB devices, we actually need to use
             * 512 (WUSB1.0[4.8.1]).
             */
            for (j = 0; j < 3; ++j)
            {
                buf->bMaxPacketSize0 = 0;
                r = usb_control_msg(udev, usb_rcvaddr0pipe(),
                    USB_REQ_GET_DESCRIPTOR, USB_DIR_IN,
                    USB_DT_DEVICE << 8, 0,
                    buf, GET_DESCRIPTOR_BUFSIZE,
                    initial_descriptor_timeout);


                switch (buf->bMaxPacketSize0)
                {
                case 8: case 16: case 32: case 64: case 255:
                    if (buf->bDescriptorType ==
                            USB_DT_DEVICE) {
                        r = 0;
                        break;
                    }
                    /* FALL THROUGH */
                default:
                    if (r == 0)
                        r = -EPROTO;
                    break;
                }
                if (r == 0)
                    break;
            }
            udev->descriptor.bMaxPacketSize0 =
                    buf->bMaxPacketSize0;
            usb_dma_buffer_free(buf, udev->bus);
            /* rt_free(buf); */
            /* RT_DEBUG_LOG(rt_debug_usb,("rt_free hub GET_DESCRIPTOR_BUFSIZE:%d\n",--g_mem_debug)); */

            retval = hub_port_reset(hub, port1, udev, delay);
            if (retval < 0)        /* error or disconnect */
                goto fail;
            if (oldspeed != udev->speed)
            {
                RT_DEBUG_LOG(rt_debug_usb,
                    ("device reset changed speed!\n"));
                retval = -ENODEV;
                goto fail;
            }
            if (r)
            {
                RT_DEBUG_LOG(rt_debug_usb,
                    ("device descriptor read/64, error %d\n",
                    r));
                retval = -EMSGSIZE;
                continue;
            }
#undef GET_DESCRIPTOR_BUFSIZE
        }

         /*
          * If device is WUSB, we already assigned an
          * unauthorized address in the Connect Ack sequence;
          * authorization will assign the final address.
          */
        if (udev->wusb == 0)
        {
            for (j = 0; j < SET_ADDRESS_TRIES; ++j)
            {
                retval = hub_set_address(udev, devnum);
                if (retval >= 0)
                    break;
                rt_thread_delay(20);
            }
            if (retval < 0)
            {
                RT_DEBUG_LOG(rt_debug_usb,
                    ("device not accepting address %d, error %d\n",
                    devnum, retval));
                goto fail;
            }
            if (udev->speed == USB_SPEED_SUPER)
            {
                devnum = udev->devnum;
                RT_DEBUG_LOG(rt_debug_usb,
                        ("%s SuperSpeed USB device number %d using\n",
                        (udev->config) ? "reset" : "new",
                        devnum));
            }

            /* cope with hardware quirkiness:
             *  - let SET_ADDRESS settle, some device hardware wants it
             *  - read ep0 maxpacket even for high and low speed,
             */
            rt_thread_delay(1);
            if (USE_NEW_SCHEME(retry_counter) && !(hcd->driver->flags & HCD_USB3))
                break;
          }

        retval = usb_get_device_descriptor(udev, 8);
        if (retval < 8)
        {
            RT_DEBUG_LOG(rt_debug_usb,
                    ("device descriptor read/8, error %d\n",
                    retval));
            if (retval >= 0)
                retval = -EMSGSIZE;
        } else
        {
            retval = 0;
            break;
        }
    }
    if (retval)
        goto fail;

    if (udev->descriptor.bMaxPacketSize0 == 0xff ||
            udev->speed == USB_SPEED_SUPER)
        i = 512;
    else
        i = udev->descriptor.bMaxPacketSize0;
    if ((udev->ep0.desc.wMaxPacketSize) != i)
    {
        if (udev->speed == USB_SPEED_LOW ||
                !(i == 8 || i == 16 || i == 32 || i == 64)) {
            RT_DEBUG_LOG(rt_debug_usb, ("Invalid ep0 maxpacket: %d\n", i));
            retval = -EMSGSIZE;
            goto fail;
        }
        if (udev->speed == USB_SPEED_FULL)
            RT_DEBUG_LOG(rt_debug_usb, ("ep0 maxpacket = %d\n", i));
        else
            RT_DEBUG_LOG(rt_debug_usb, ("Using ep0 maxpacket: %d\n", i));
        udev->ep0.desc.wMaxPacketSize = (i);
        usb_ep0_reinit(udev);
    }

    retval = usb_get_device_descriptor(udev, USB_DT_DEVICE_SIZE);
    if (retval < (signed int)sizeof(udev->descriptor))
    {
        RT_DEBUG_LOG(rt_debug_usb, ("device descriptor read/all, error %d\n",
            retval));
        if (retval >= 0)
            retval = -ENOMSG;
        goto fail;
    }

    retval = 0;
    /* notify HCD that we have a device connected and addressed */
    if (hcd->driver->update_device)
        hcd->driver->update_device(hcd, udev);
fail:
    if (retval)
    {
        hub_port_disable(hub, port1, 0);
        update_devnum(udev, devnum);    /* for disconnect processing */
    }
/* mutex_unlock(&usb_address0_mutex); */
    return retval;
}

static void
check_highspeed(struct usb_hub *hub, struct usb_device *udev, int port1)
{
    struct usb_qualifier_descriptor    *qual;
    int                status;
    rt_base_t level;

    qual = rt_malloc(sizeof(*qual));
    RT_DEBUG_LOG(rt_debug_usb, ("rt_malloc qual:%d size:%d,addr:%x\n", g_mem_debug++, (sizeof(*qual)), qual));
    if (qual == RT_NULL)
        return;

    status = usb_get_descriptor(udev, USB_DT_DEVICE_QUALIFIER, 0,
            qual, sizeof(*qual));
    if (status == sizeof(*qual))
    {
        RT_DEBUG_LOG(rt_debug_usb, ("not running at top speed; "
            "connect to a high speed hub\n"));
        /* hub LEDs are probably harder to miss than syslog */
        level = rt_hw_interrupt_disable();
         rt_enter_critical();
        if (hub->has_indicators)
        {
            hub->indicator[port1-1] = INDICATOR_GREEN_BLINK;
/* schedule_delayed_work (&hub->leds, 0); */
        }
        rt_hw_interrupt_enable(level);
        rt_exit_critical();
    }
    RT_DEBUG_LOG(rt_debug_usb, ("rt_free hub qual:%d,addr:%x\n", --g_mem_debug, qual));
    rt_free(qual);
}

static unsigned
hub_power_remaining(struct usb_hub *hub)
{
    struct usb_device *hdev = hub->hdev;
    int remaining;
    int port1;

    if (!hub->limited_power)
        return 0;

    remaining = hdev->bus_mA - hub->descriptor->bHubContrCurrent;
    for (port1 = 1; port1 <= hdev->maxchild; ++port1)
    {
        struct usb_device    *udev = hdev->children[port1 - 1];
        int            delta;

        if (!udev)
            continue;

        /* Unconfigured devices may not use more than 100mA,
         * or 8mA for OTG ports */
        if (udev->actconfig)
            delta = udev->actconfig->desc.bMaxPower * 2;
        else if (port1 != udev->bus->otg_port || hdev->parent)
            delta = 100;
        else
            delta = 8;
        if (delta > hub->mA_per_port)
            RT_DEBUG_LOG(rt_debug_usb,
                 ("%dmA is over %umA budget for port %d!\n",
                 delta, hub->mA_per_port, port1));
        remaining -= delta;
    }
    if (remaining < 0)
    {
        RT_DEBUG_LOG(rt_debug_usb, ("%dmA over power budget!\n",
            -remaining));
        remaining = 0;
    }
    return remaining;
}

/* Handle physical or logical connection change events.
 * This routine is called when:
 *     a port connection-change occurs;
 *    a port enable-change occurs (often caused by EMI);
 *    usb_reset_and_verify_device() encounters changed descriptors (as from
 *        a firmware download)
 * caller already locked the hub
 */
static void hub_port_connect_change(struct usb_hub *hub, int port1,
                    rt_uint16_t portstatus, rt_uint16_t portchange)
{
    struct usb_device *hdev = hub->hdev;
    struct usb_hcd *hcd = bus_to_hcd(hdev->bus);
    unsigned wHubCharacteristics = hub->descriptor->wHubCharacteristics;
    struct usb_device *udev = RT_NULL;
    int status, i;
    rt_base_t level;

    RT_DEBUG_LOG(rt_debug_usb,
        ("port %d, status %04x, change %04x, %s\n",
        port1, portstatus, portchange, portspeed(hub, portstatus)));

    if (hub->has_indicators)
    {
        set_port_led(hub, port1, HUB_LED_AUTO);
        hub->indicator[port1-1] = INDICATOR_AUTO;
    }

#ifdef    CONFIG_USB_OTG
    /* during HNP, don't repeat the debounce */
    if (hdev->bus->is_b_host)
        portchange &= ~(USB_PORT_STAT_C_CONNECTION |
                USB_PORT_STAT_C_ENABLE);
#endif

    /* Try to resuscitate an existing device */
    udev = hdev->children[port1-1];
    if ((portstatus & USB_PORT_STAT_CONNECTION) && udev &&
            udev->state != USB_STATE_NOTATTACHED) {
    /* usb_lock_device(udev); */
        if (portstatus & USB_PORT_STAT_ENABLE)
        {
            status = 0;        /* Nothing to do */

        } else
        {
            status = -ENODEV;    /* Don't resuscitate */
        }
/* usb_unlock_device(udev); */

        if (status == 0)
        {
            clear_bit(port1, hub->change_bits);
            return;
        }
    }

    /* Disconnect any existing devices under this port */
    if (udev)
        usb_disconnect(&hdev->children[port1-1]);
    clear_bit(port1, hub->change_bits);

    /* We can forget about a "removed" device when there's a physical
     * disconnect or the connect status changes.
     */
    if (!(portstatus & USB_PORT_STAT_CONNECTION) ||
            (portchange & USB_PORT_STAT_C_CONNECTION))
        clear_bit(port1, hub->removed_bits);

    if (portchange & (USB_PORT_STAT_C_CONNECTION |
                USB_PORT_STAT_C_ENABLE)) {
        status = hub_port_debounce(hub, port1);
        if (status < 0)
        {
                RT_DEBUG_LOG(rt_debug_usb, ("connect-debounce failed, port %d disabled\n",
                        port1));
            portstatus &= ~USB_PORT_STAT_CONNECTION;
        } else
        {
            portstatus = status;
        }
    }

    /* Return now if debouncing failed or nothing is connected or
     * the device was "removed".
     */
    if (!(portstatus & USB_PORT_STAT_CONNECTION) ||
            test_bit(port1, hub->removed_bits)) {

        /* maybe switch power back on (e.g. root hub was reset) */
        if ((wHubCharacteristics & HUB_CHAR_LPSM) < 2
                && !port_is_power_on(hub, portstatus))
            set_port_feature(hdev, port1, USB_PORT_FEAT_POWER);

        if (portstatus & USB_PORT_STAT_ENABLE)
              goto done;
        return;
    }

    for (i = 0; i < SET_CONFIG_TRIES; i++)
    {

        /* reallocate for each attempt, since references
         * to the previous one can escape in various ways
         */
        udev = usb_alloc_dev(hdev, hdev->bus, port1);
        if (!udev)
        {
            RT_DEBUG_LOG(rt_debug_usb,
                ("couldn't allocate port %d usb_device\n",
                port1));
            goto done;
        }

        usb_set_device_state(udev, USB_STATE_POWERED);
         udev->bus_mA = hub->mA_per_port;
        udev->level = hdev->level + 1;
        udev->wusb = hub_is_wusb(hub);

        /* Only USB 3.0 devices are connected to SuperSpeed hubs. */
        if (hub_is_superspeed(hub->hdev))
            udev->speed = USB_SPEED_SUPER;
        else
            udev->speed = USB_SPEED_UNKNOWN;

        choose_devnum(udev);
        if (udev->devnum <= 0)
        {
            status = -ENOTCONN;    /* Don't retry */
            goto loop;
        }

        /* reset (non-USB 3.0 devices) and get descriptor */
        status = hub_port_init(hub, udev, port1, i);
        if (status < 0)
            goto loop;

        usb_detect_quirks(udev);
        if (udev->quirks & USB_QUIRK_DELAY_INIT)
            rt_thread_delay(100);

        /* consecutive bus-powered hubs aren't reliable; they can
         * violate the voltage drop budget.  if the new child has
         * a "powered" LED, users should notice we didn't enable it
         * (without reading syslog), even without per-port LEDs
         * on the parent.
         */
        if (udev->descriptor.bDeviceClass == USB_CLASS_HUB
                && udev->bus_mA <= 100) {
            rt_uint32_t devstat;

            status = usb_get_status(udev, USB_RECIP_DEVICE, 0,
                    &devstat);
            if (status < 2)
            {
                RT_DEBUG_LOG(rt_debug_usb, ("get status %d ?\n", status));
                goto loop_disable;
            }
/* le16_to_cpus(&devstat); */
            if ((devstat & (1 << USB_DEVICE_SELF_POWERED)) == 0)
            {
                RT_DEBUG_LOG(rt_debug_usb,
                    ("can't connect bus-powered hub to this port\n"
                    ));
                level = rt_hw_interrupt_disable();
                rt_enter_critical();
                if (hub->has_indicators)
                {
                    hub->indicator[port1-1] =
                        INDICATOR_AMBER_BLINK;
/* schedule_delayed_work (&hub->leds, 0); */
                }
                rt_hw_interrupt_enable(level);
                rt_exit_critical();
                status = -ENOTCONN;    /* Don't retry */
                goto loop_disable;
            }
        }

        /* check for devices running slower than they could */
        if ((udev->descriptor.bcdUSB) >= 0x0200
                && udev->speed == USB_SPEED_FULL
                && highspeed_hubs != 0)
            check_highspeed(hub, udev, port1);

        /* Store the parent's children[] pointer.  At this point
         * udev becomes globally accessible, although presumably
         * no one will look at it until hdev is unlocked.
         */
        status = 0;

        /* We mustn't add new devices if the parent hub has
         * been disconnected; we would race with the
         * recursively_mark_NOTATTACHED() routine.
         */
        /* <-tangyh*/
    level = rt_hw_interrupt_disable();
    rt_enter_critical();
    /* spin_lock_irq(&device_state_lock); */
    /* tangyh->*/
        if (hdev->state == USB_STATE_NOTATTACHED)
            status = -ENOTCONN;
        else
            hdev->children[port1-1] = udev;

    /* <-tangyh*/
        rt_hw_interrupt_enable(level);
        rt_exit_critical();
    /* spin_unlock_irq(&device_state_lock); */
    /* tangyh ->*/

        /* Run it through the hoops (find a driver, etc) */
        if (!status)
        {
            status = usb_new_device(udev);
            if (status)
            {
                    /* <-tangyh*/
    level = rt_hw_interrupt_disable();
    rt_enter_critical();
    /* spin_lock_irq(&device_state_lock); */
    /* tangyh->*/
                hdev->children[port1-1] = RT_NULL;
    /* <-tangyh*/
        rt_hw_interrupt_enable(level);
        rt_exit_critical();
    /* spin_unlock_irq(&device_state_lock); */
    /* tangyh ->*/
            }
        }

        if (status)
            goto loop_disable;

        status = hub_power_remaining(hub);
        if (status)
            RT_DEBUG_LOG(rt_debug_usb, ("%dmA power budget left\n", status));

        return;

loop_disable:
        hub_port_disable(hub, port1, 1);
loop:
        usb_ep0_reinit(udev);
        release_devnum(udev);
        hub_free_dev(udev);
        rt_free(udev);

        if ((status == -ENOTCONN) || (status == -ENOTSUPP))
            break;
    }
    if (hub->hdev->parent ||
            !hcd->driver->port_handed_over ||
            !(hcd->driver->port_handed_over)(hcd, port1))
        RT_DEBUG_LOG(rt_debug_usb, ("unable to enumerate USB device on port %d\n",
                port1));

done:
    hub_port_disable(hub, port1, 1);
    if (hcd->driver->relinquish_port && !hub->hdev->parent)
        hcd->driver->relinquish_port(hcd, port1);
}

int usb_remote_wakeup(struct usb_device *dev)
{
    return 0;
}

static struct usb_device *g_hdev;
static void hub_events(void)
{
     rt_list_t *tmp;
    struct usb_device *hdev;
/* struct usb_interface *intf; */
    struct usb_hub *hub;
    rt_uint16_t hubstatus;
    rt_uint16_t hubchange;
    rt_uint16_t portstatus;
    rt_uint16_t portchange;
    int i, ret;
    int connect_change;
    rt_base_t level;

    /*
     *  We restart the list every time to avoid a deadlock with
     * deleting hubs downstream from this one. This should be
     * safe since we delete the hub from the event list.
     * Not the most efficient, but avoids deadlocks.
     */
    do
    {

        /* Grab the first entry at the beginning of the list */

    level = rt_hw_interrupt_disable();
    rt_enter_critical();
        if (rt_list_isempty(&hub_event_list))
        {

     rt_exit_critical();
     rt_hw_interrupt_enable(level);
            break;
        }

        tmp = hub_event_list.next;
        rt_list_remove(tmp);

        hub = rt_list_entry(tmp, struct usb_hub, event_list);
        hub->kref++;

        rt_exit_critical();
        rt_hw_interrupt_enable(level);

        hdev = hub->hdev;
        g_hdev = hdev;
        RT_DEBUG_LOG(rt_debug_usb, ("state %d ports %d chg %04x evt %04x\n",
                hdev->state, hub->descriptor
                    ? hub->descriptor->bNbrPorts
                    : 0,
                /* NOTE: expects max 15 ports... */
                (rt_uint16_t) hub->change_bits[0],
                (rt_uint16_t) hub->event_bits[0]));


        if (hub->error)
        {
            RT_DEBUG_LOG(rt_debug_usb, ("resetting for error %d\n",
                hub->error));

            hub->nerrors = 0;
            hub->error = 0;
        }

        /* deal with port status changes */
        for (i = 1; i <= hub->descriptor->bNbrPorts; i++)
        {
            /* if (test_bit(i, hub->busy_bits)) */
            /* continue; */
            connect_change = test_bit(i, hub->change_bits);
            if (!test_and_clear_bit(i, hub->event_bits) &&
                    !connect_change)
                continue;

            ret = hub_port_status(hub, i,
                    &portstatus, &portchange);
            if (ret < 0)
                continue;
            if (portchange & USB_PORT_STAT_C_CONNECTION)
            {
                clear_port_feature(hdev, i,
                    USB_PORT_FEAT_C_CONNECTION);
                connect_change = 1;
            }

            if (portchange & USB_PORT_STAT_C_ENABLE)
            {
                if (!connect_change)
                    RT_DEBUG_LOG(rt_debug_usb,
                        ("port %d enable change, status %08x\n",
                        i, portstatus));
                clear_port_feature(hdev, i,
                    USB_PORT_FEAT_C_ENABLE);

                /*
                 * EM interference sometimes causes badly
                 * shielded USB devices to be shutdown by
                 * the hub, this hack enables them again.
                 * Works at least with mouse driver.
                 */
                if (!(portstatus & USB_PORT_STAT_ENABLE)
                    && !connect_change
                    && hdev->children[i-1]) {
                    RT_DEBUG_LOG(rt_debug_usb,
					    ("port %i disabled by hub (EMI?), "
                        "re-enabling...\n",
                        i));
                    connect_change = 1;
                }
            }

            if (portchange & USB_PORT_STAT_C_SUSPEND)
            {
                struct usb_device *udev;

                clear_port_feature(hdev, i,
                    USB_PORT_FEAT_C_SUSPEND);
                udev = hdev->children[i-1];
                if (udev)
                {
                    /* TRSMRCY = 10 msec */
                    rt_thread_delay(1);

                    /* usb_lock_device(udev); */
                    ret = usb_remote_wakeup(hdev->
                            children[i-1]);
                    /* usb_unlock_device(udev); */
                    if (ret < 0)
                        connect_change = 1;
                } else
                {
                    ret = -ENODEV;
                    hub_port_disable(hub, i, 1);
                }
                RT_DEBUG_LOG(rt_debug_usb,
                    ("resume on port %d, status %d\n",
                    i, ret));
            }

            if (portchange & USB_PORT_STAT_C_OVERCURRENT)
            {
                rt_uint16_t status = 0;
                rt_uint16_t unused;

                RT_DEBUG_LOG(rt_debug_usb,( "over-current change on port %d\n",
                    i));
                clear_port_feature(hdev, i,
                    USB_PORT_FEAT_C_OVER_CURRENT);
                rt_thread_delay(10);    /* Cool down */
                hub_power_on(hub, RT_TRUE);
                hub_port_status(hub, i, &status, &unused);
                if (status & USB_PORT_STAT_OVERCURRENT)
                    RT_DEBUG_LOG(rt_debug_usb, ("over-current condition on port %d\n",
                        i));
            }

            if (portchange & USB_PORT_STAT_C_RESET)
            {
                RT_DEBUG_LOG(rt_debug_usb,
                    ("reset change on port %d\n",
                    i));
                clear_port_feature(hdev, i,
                    USB_PORT_FEAT_C_RESET);
            }
            if ((portchange & USB_PORT_STAT_C_BH_RESET) &&
                    hub_is_superspeed(hub->hdev)) {
                RT_DEBUG_LOG(rt_debug_usb,
                    ("warm reset change on port %d\n",
                    i));
                clear_port_feature(hdev, i,
                    USB_PORT_FEAT_C_BH_PORT_RESET);
            }
            if (portchange & USB_PORT_STAT_C_LINK_STATE)
            {
                clear_port_feature(hub->hdev, i,
                        USB_PORT_FEAT_C_PORT_LINK_STATE);
            }
            if (portchange & USB_PORT_STAT_C_CONFIG_ERROR)
            {
                RT_DEBUG_LOG(rt_debug_usb,
                    ("config error on port %d\n",
                    i));
                clear_port_feature(hub->hdev, i,
                        USB_PORT_FEAT_C_PORT_CONFIG_ERROR);
            }

            /* Warm reset a USB3 protocol port if it's in
             * SS.Inactive state.
             */
            if (hub_is_superspeed(hub->hdev) &&
                (portstatus & USB_PORT_STAT_LINK_STATE)
                    == USB_SS_PORT_LS_SS_INACTIVE) {
                rt_kprintf("warm reset port %d\n", i);
                hub_port_warm_reset(hub, i);
            }

            if (connect_change)
                hub_port_connect_change(hub, i,
                        portstatus, portchange);
        } /* end for i */

        /* deal with hub status changes */
        if (test_and_clear_bit(0, hub->event_bits) == 0)
            ;    /* do nothing */
        else if (hub_hub_status(hub, &hubstatus, &hubchange) < 0)
            RT_DEBUG_LOG(rt_debug_usb, ("get_hub_status failed\n"));
        else
        {
            if (hubchange & HUB_CHANGE_LOCAL_POWER)
            {
                RT_DEBUG_LOG(rt_debug_usb, ("power change\n"));
                clear_hub_feature(hdev, C_HUB_LOCAL_POWER);
                if (hubstatus & HUB_STATUS_LOCAL_POWER)
                    /* FIXME: Is this always true? */
                    hub->limited_power = 1;
                else
                    hub->limited_power = 0;
            }
            if (hubchange & HUB_CHANGE_OVERCURRENT)
            {
                rt_uint16_t status = 0;
                rt_uint16_t unused;

                RT_DEBUG_LOG(rt_debug_usb, ("over-current change\n"));
                clear_hub_feature(hdev, C_HUB_OVER_CURRENT);
                rt_thread_delay(50);    /* Cool down */
                            hub_power_on(hub, RT_TRUE);
                hub_hub_status(hub, &status, &unused);
                if (status & HUB_STATUS_OVERCURRENT)
                    RT_DEBUG_LOG(rt_debug_usb, ("over-current condition\n"
                    ));
            }
        }

 /* loop_disconnected: */
/* usb_unlock_device(hdev); */
/* kref_put(&hub->kref, hub_release); */
        hub_release(hub);

        }while (0) ;/* end while (1) */
}

void hub_thread(void  *ph)
{
    /* khubd needs to be freezable to avoid intefering with USB-PERSIST
     * port handover.  Otherwise it might see that a full-speed device
     * was gone before the EHCI controller had handed its port over to
     * the companion full-speed controller.
     */
    /* set_freezable(); */
    /* rt_uint32_t e; */

    do
    {
        hub_events();
        /* rt_event_recv(&event_khubd_wait,1<<0, */
        /* RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR,RT_WAITING_FOREVER,&e); */
        /* rt_kprintf("rt_sem_take:%d\n",sem_khubd_wait->value); */
        rt_sem_take(sem_khubd_wait, RT_WAITING_FOREVER);
    } while (!rt_list_isempty(&hub_event_list));

    RT_DEBUG_LOG(rt_debug_usb, ("%s: khubd exiting\n", usbcore_name));
}

static const struct usb_device_id hub_id_table[] = {
    { .match_flags = USB_DEVICE_ID_MATCH_DEV_CLASS,
      .bDeviceClass = USB_CLASS_HUB},
    { .match_flags = USB_DEVICE_ID_MATCH_INT_CLASS,
      .bInterfaceClass = USB_CLASS_HUB},
    { }                        /* Terminating entry */
};


static struct usb_driver hub_driver = {
    .name =        "hub",
    .probe =    hub_probe,
    .disconnect =    hub_disconnect,
    .id_table =    hub_id_table,
};

int usb_hub_init(void)
{
    rt_list_init(&hub_event_list);

    sem_khubd_wait = rt_sem_create("sem_khubd_wait", 0, RT_IPC_FLAG_FIFO);
    /* rt_event_init(&event_khubd_wait, "event_khubd_wait", RT_IPC_FLAG_FIFO); */


    if (usb_register_driver(&hub_driver) < 0)
    {
        rt_kprintf("%s: can't register hub driver\n",
            usbcore_name);
        return -RT_ERROR;
    }

    khubd_task = rt_thread_create("khubd", hub_thread, RT_NULL, USB_THREAD_STACK_SIZE, 10, 20);
    if (khubd_task != RT_NULL)
        {
            /* startup usb host thread */
                    rt_thread_startup(khubd_task);
        return 0;
        }
    /* Fall through if kernel_thread failed */
    usb_deregister(&hub_driver);
    rt_kprintf("%s: can't start khubd\n", usbcore_name);

    return -RT_ERROR;
}
static int dev_num = 0;
void list_usb_children(struct usb_device *dev)
{
    int i;
    char Manufacturer[256];
    char Product[256];

    for (i = 0; i < USB_MAXCHILDREN; i++)
    {
        if (dev->children[i])
        {
            rt_memset(Manufacturer, 0, sizeof(Manufacturer));
            rt_memset(Product, 0, sizeof(Product));
            usb_string(dev->children[i], 1, Manufacturer, sizeof(Manufacturer));
            usb_string(dev->children[i], 2, Product, sizeof(Product));
            rt_kprintf("    Device %02d: ID %04x:%04x  %s %s\n\n",
                dev_num, dev->children[i]->descriptor.idVendor, dev->children[i]->descriptor.idProduct, Manufacturer, Product);
            dev_num++;
            list_usb_children(dev->children[i]);
        }
    }
}

void list_usb(int argc, char *argv[])
{
    struct usb_device *hdev;

    hdev = g_hdev;
    dev_num = 0;
    rt_kprintf("\n\nUSB Device List:\n\n");
    list_usb_children(hdev);
    return;
}

MSH_CMD_EXPORT(list_usb, list_usb .....);

