#ifndef __HCD_H__
#define __HCD_H__

#include "rtdef.h"
#include "usb.h"

#define MAX_TOPO_LEVEL        6

/* This file contains declarations of usbcore internals that are mostly
 * used or exposed by Host Controller Drivers.
 */

/*
 * USB Packet IDs (PIDs)
 */
#define USB_PID_EXT            0xf0    /* USB 2.0 LPM ECN */
#define USB_PID_OUT            0xe1
#define USB_PID_ACK            0xd2
#define USB_PID_DATA0            0xc3
#define USB_PID_PING            0xb4    /* USB 2.0 */
#define USB_PID_SOF            0xa5
#define USB_PID_NYET            0x96    /* USB 2.0 */
#define USB_PID_DATA2            0x87    /* USB 2.0 */
#define USB_PID_SPLIT            0x78    /* USB 2.0 */
#define USB_PID_IN            0x69
#define USB_PID_NAK            0x5a
#define USB_PID_DATA1            0x4b
#define USB_PID_PREAMBLE        0x3c    /* Token mode */
#define USB_PID_ERR            0x3c    /* USB 2.0: handshake mode */
#define USB_PID_SETUP            0x2d
#define USB_PID_STALL            0x1e
#define USB_PID_MDATA            0x0f    /* USB 2.0 */

/*-------------------------------------------------------------------------*/

/*
 * USB Host Controller Driver (usb_hcd) framework
 *
 * Since "struct usb_bus" is so thin, you can't share much code in it.
 * This framework is a layer over that, and should be more sharable.
 *
 * @authorized_default: Specifies if new devices are authorized to
 *                      connect by default or they require explicit
 *                      user space authorization; this bit is settable
 *                      through /sys/class/usb_host/X/authorized_default.
 *                      For the rest is RO, so we don't lock to r/w it.
 */

/*-------------------------------------------------------------------------*/

struct usb_hcd
{

    /*
     * housekeeping
     */
    struct usb_bus        self;        /* hcd is-a bus */

    const char        *product_desc;    /* product/vendor string */
    int            speed;        /* Speed for this roothub.
                         * May be different from
                         * hcd->driver->flags & HCD_MASK
                         */
    char            irq_descr[24];    /* driver + bus # */

    rt_timer_t    rh_timer;    /* drives root-hub polling */
    struct urb        *status_urb;    /* the current status urb */
    /*
     * hardware info/state
     */
    const struct hc_driver    *driver;    /* hw-specific hooks */

    /* Flags that need to be manipulated atomically because they can
     * change while the host controller is running.  Always use
     * set_bit() or clear_bit() to change their values.
     */
    unsigned long        flags;
#define HCD_FLAG_HW_ACCESSIBLE        0    /* at full power */
#define HCD_FLAG_SAW_IRQ        1
#define HCD_FLAG_POLL_RH        2    /* poll for rh status? */
#define HCD_FLAG_POLL_PENDING        3    /* status has changed? */
#define HCD_FLAG_WAKEUP_PENDING        4    /* root hub is resuming? */
#define HCD_FLAG_RH_RUNNING        5    /* root hub is running? */
#define HCD_FLAG_DEAD            6    /* controller has died? */

    /* The flags can be tested using these macros; they are likely to
     * be slightly faster than test_bit().
     */
#define HCD_HW_ACCESSIBLE(hcd)    ((hcd)->flags & (1U << HCD_FLAG_HW_ACCESSIBLE))
#define HCD_SAW_IRQ(hcd)    ((hcd)->flags & (1U << HCD_FLAG_SAW_IRQ))
#define HCD_POLL_RH(hcd)    ((hcd)->flags & (1U << HCD_FLAG_POLL_RH))
#define HCD_POLL_PENDING(hcd)    ((hcd)->flags & (1U << HCD_FLAG_POLL_PENDING))
#define HCD_WAKEUP_PENDING(hcd)    ((hcd)->flags & (1U << HCD_FLAG_WAKEUP_PENDING))
#define HCD_RH_RUNNING(hcd)    ((hcd)->flags & (1U << HCD_FLAG_RH_RUNNING))
#define HCD_DEAD(hcd)        ((hcd)->flags & (1U << HCD_FLAG_DEAD))

    /* Flags that get set only during HCD registration or removal. */
    unsigned        rh_registered:1;/* is root hub registered? */
    unsigned        rh_pollable:1;    /* may we poll the root hub? */
    unsigned        msix_enabled:1;    /* driver has MSI-X enabled? */

    /* The next flag is a stopgap, to be removed when all the HCDs
     * support the new root-hub polling mechanism. */
    unsigned        uses_new_polling:1;
    unsigned        wireless:1;    /* Wireless USB HCD */
    unsigned        has_tt:1;    /* Integrated TT in root hub */

    int            irq;        /* irq allocated */
    unsigned int power_budget;    /* in mA, 0 = no limit */

    /* bandwidth_mutex should be taken before adding or removing
     * any new bus bandwidth constraints:
     *   1. Before adding a configuration for a new device.
     *   2. Before removing the configuration to put the device into
     *      the addressed state.
     *   3. Before selecting a different configuration.
     *   4. Before selecting an alternate interface setting.
     *
     * bandwidth_mutex should be dropped after a successful control message
     * to the device, or resetting the bandwidth after a failed attempt.
     */
    struct rt_mutex        *bandwidth_mutex;
    struct usb_hcd        *shared_hcd;
    struct usb_hcd        *primary_hcd;

    int            state;
#    define    __ACTIVE        0x01
#    define    __SUSPEND        0x04
#    define    __TRANSIENT        0x80

#    define    HC_STATE_HALT        0
#    define    HC_STATE_RUNNING    (__ACTIVE)
#    define    HC_STATE_QUIESCING    (__SUSPEND|__TRANSIENT|__ACTIVE)
#    define    HC_STATE_RESUMING    (__SUSPEND|__TRANSIENT)
#    define    HC_STATE_SUSPENDED    (__SUSPEND)

#define    HC_IS_RUNNING(state) ((state) & __ACTIVE)
#define    HC_IS_SUSPENDED(state) ((state) & __SUSPEND)


    /* The HC driver's private data is stored at the end of
     * this structure.
     */
    unsigned long hcd_priv[0]
            __attribute__ ((aligned(sizeof(unsigned long))));
};

/* 2.4 does this a bit differently ... */
static inline struct usb_bus *hcd_to_bus(struct usb_hcd *hcd)
{
    return &(hcd->self);
}

static inline struct usb_hcd *bus_to_hcd(struct usb_bus *bus)
{
    return (struct usb_hcd *)bus;
}


/*-------------------------------------------------------------------------*/


struct hc_driver
{
    const char    *description;    /* "ehci-hcd" etc */
    const char    *product_desc;    /* product/vendor string */
    rt_uint32_t hcd_priv_size;    /* size of private data */

    /* irq handler */
    rt_uint32_t	(*irq)(struct usb_hcd *hcd);

    int    flags;
#define    HCD_MEMORY    0x0001        /* HC regs use memory (else I/O) */
#define    HCD_LOCAL_MEM    0x0002        /* HC needs local memory */
#define    HCD_SHARED    0x0004        /* Two (or more) usb_hcds share HW */
#define    HCD_USB11    0x0010        /* USB 1.1 */
#define    HCD_USB2    0x0020        /* USB 2.0 */
#define    HCD_USB3    0x0040        /* USB 3.0 */
#define    HCD_MASK    0x0070

    /* called to init HCD and root hub */
    int	(*reset)(struct usb_hcd *hcd);
    int	(*start)(struct usb_hcd *hcd);

    /* NOTE:  these suspend/resume calls relate to the HC as
     * a whole, not just the root hub; they're for PCI bus glue.
     */
    /* called after suspending the hub, before entering D3 etc */
    int	(*pci_suspend)(struct usb_hcd *hcd, rt_bool_t do_wakeup);

    /* called after entering D0 (etc), before resuming the hub */
    int	(*pci_resume)(struct usb_hcd *hcd, rt_bool_t hibernated);

    /* cleanly make HCD stop writing memory and doing I/O */
    void	(*stop)(struct usb_hcd *hcd);

    /* shutdown HCD */
    void	(*shutdown)(struct usb_hcd *hcd);

    /* return current frame number */
    int	(*get_frame_number)(struct usb_hcd *hcd);

    /* manage i/o requests, device state */
    int	(*urb_enqueue)(struct usb_hcd *hcd,
                struct urb *urb);
    int	(*urb_dequeue)(struct usb_hcd *hcd,
                struct urb *urb, int status);

    /*
     * (optional) these hooks allow an HCD to override the default DMA
     * mapping and unmapping routines.  In general, they shouldn't be
     * necessary unless the host controller has special DMA requirements,
     * such as alignment contraints.  If these are not specified, the
     * general usb_hcd_(un)?map_urb_for_dma functions will be used instead
     * (and it may be a good idea to call these functions in your HCD
     * implementation)
     */
    int (*map_urb_for_dma)(struct usb_hcd *hcd, struct urb *urb
                  );
    void    (*unmap_urb_for_dma)(struct usb_hcd *hcd, struct urb *urb);

    /* hw synch, freeing endpoint resources that urb_dequeue can't */
    void    (*endpoint_disable)(struct usb_hcd *hcd,
            struct usb_host_endpoint *ep);

    /* (optional) reset any endpoint state such as sequence number
       and current window */
    void	(*endpoint_reset)(struct usb_hcd *hcd,
            struct usb_host_endpoint *ep);

    /* root hub support */
    int (*hub_status_data)(struct usb_hcd *hcd, char *buf);
    int (*hub_control)(struct usb_hcd *hcd,
                rt_uint16_t typeReq, rt_uint16_t wValue, rt_uint16_t wIndex,
                char *buf, rt_uint16_t wLength);
    int (*bus_suspend)(struct usb_hcd *);
    int (*bus_resume)(struct usb_hcd *);
    int (*start_port_reset)(struct usb_hcd *, unsigned int port_num);

        /* force handover of high-speed port to full-speed companion */
    void    (*relinquish_port)(struct usb_hcd *, int);
        /* has a port been handed over to a companion? */
    int (*port_handed_over)(struct usb_hcd *, int);

        /* CLEAR_TT_BUFFER completion callback */
    void    (*clear_tt_buffer_complete)(struct usb_hcd *,
                struct usb_host_endpoint *);

    /* xHCI specific functions */
        /* Called by usb_alloc_dev to alloc HC device structures */
    int (*alloc_dev)(struct usb_hcd *, struct usb_device *);
        /* Called by usb_disconnect to free HC device structures */
    void    (*free_dev)(struct usb_hcd *, struct usb_device *);
    /* Change a group of bulk endpoints to support multiple stream IDs */
    int (*alloc_streams)(struct usb_hcd *hcd, struct usb_device *udev,
        struct usb_host_endpoint **eps, unsigned int num_eps,
        unsigned int num_streams);
    /* Reverts a group of bulk endpoints back to not using stream IDs.
     * Can fail if we run out of memory.
     */
    int (*free_streams)(struct usb_hcd *hcd, struct usb_device *udev,
        struct usb_host_endpoint **eps, unsigned int num_eps);

    /* Bandwidth computation functions */
    /* Note that add_endpoint() can only be called once per endpoint before
     * check_bandwidth() or reset_bandwidth() must be called.
     * drop_endpoint() can only be called once per endpoint also.
     * A call to xhci_drop_endpoint() followed by a call to
     * xhci_add_endpoint() will add the endpoint to the schedule with
     * possibly new parameters denoted by a different endpoint descriptor
     * in usb_host_endpoint.  A call to xhci_add_endpoint() followed by a
     * call to xhci_drop_endpoint() is not allowed.
     */
        /* Allocate endpoint resources and add them to a new schedule */
    int (*add_endpoint)(struct usb_hcd *, struct usb_device *,
                struct usb_host_endpoint *);
        /* Drop an endpoint from a new schedule */
    int (*drop_endpoint)(struct usb_hcd *, struct usb_device *,
                 struct usb_host_endpoint *);
        /* Check that a new hardware configuration, set using
         * endpoint_enable and endpoint_disable, does not exceed bus
         * bandwidth.  This must be called before any set configuration
         * or set interface requests are sent to the device.
         */
    int (*check_bandwidth)(struct usb_hcd *, struct usb_device *);
        /* Reset the device schedule to the last known good schedule,
         * which was set from a previous successful call to
         * check_bandwidth().  This reverts any add_endpoint() and
         * drop_endpoint() calls since that last successful call.
         * Used for when a check_bandwidth() call fails due to resource
         * or bandwidth constraints.
         */
    void    (*reset_bandwidth)(struct usb_hcd *, struct usb_device *);
        /* Returns the hardware-chosen device address */
    int (*address_device)(struct usb_hcd *, struct usb_device *udev);
        /* Notifies the HCD after a hub descriptor is fetched.
         * Will block.
         */
    int (*update_hub_device)(struct usb_hcd *, struct usb_device *hdev);
    int (*reset_device)(struct usb_hcd *, struct usb_device *);
        /* Notifies the HCD after a device is connected and its
         * address is set
         */
    int (*update_device)(struct usb_hcd *, struct usb_device *);
};
enum irqreturn
{
    IRQ_NONE        = (0 << 0),
    IRQ_HANDLED        = (1 << 0),
    IRQ_WAKE_THREAD        = (1 << 1),
};

extern int usb_hcd_link_urb_to_ep(struct usb_hcd *hcd, struct urb *urb);
extern int usb_hcd_check_unlink_urb(struct usb_hcd *hcd, struct urb *urb,
        int status);
extern void usb_hcd_unlink_urb_from_ep(struct usb_hcd *hcd, struct urb *urb);

extern int usb_hcd_submit_urb(struct urb *urb);
extern int usb_hcd_unlink_urb(struct urb *urb, int status);
extern void usb_hcd_giveback_urb(struct usb_hcd *hcd, struct urb *urb,
        int status);
extern void usb_hcd_flush_endpoint(struct usb_device *udev,
        struct usb_host_endpoint *ep);
extern void usb_hcd_disable_endpoint(struct usb_device *udev,
        struct usb_host_endpoint *ep);
extern void usb_hcd_reset_endpoint(struct usb_device *udev,
        struct usb_host_endpoint *ep);
extern int usb_hcd_alloc_bandwidth(struct usb_device *udev,
        struct usb_host_config *new_config,
        struct usb_host_interface *old_alt,
        struct usb_host_interface *new_alt);
extern int usb_hcd_get_frame_number(struct usb_device *udev);

struct usb_hcd *usb_create_hcd(const struct hc_driver *driver,
        struct rt_device *dev, const char *bus_name);
struct usb_hcd *usb_create_shared_hcd(const struct hc_driver *driver,
        struct rt_device *dev, const char *bus_name);
extern int usb_hcd_is_primary_hcd(struct usb_hcd *hcd);
extern int usb_add_hcd(struct usb_hcd *hcd, unsigned int irqnum);
extern void usb_remove_hcd(struct usb_hcd *hcd);


/* generic bus glue, needed for host controllers that don't use PCI */
extern void usb_hcd_irq(int irq, void *__hcd);

extern void usb_hcd_poll_rh_status(struct usb_hcd *hcd);

/* The D0/D1 toggle bits ... USE WITH CAUTION (they're almost hcd-internal) */
#define usb_gettoggle(dev, ep, out) (((dev)->toggle[out] >> (ep)) & 1)
#define    usb_dotoggle(dev, ep, out)  ((dev)->toggle[out] ^= (1 << (ep)))
#define usb_settoggle(dev, ep, out, bit) \
        ((dev)->toggle[out] = ((dev)->toggle[out] & ~(1 << (ep))) | \
         ((bit) << (ep)))

/* -------------------------------------------------------------------------- */

/* Enumeration is only for the hub driver, or HCD virtual root hubs */
extern struct usb_device *usb_alloc_dev(struct usb_device *parent,
                    struct usb_bus *, unsigned int port);
extern int usb_new_device(struct usb_device *dev);
extern void usb_disconnect(struct usb_device **);

extern int usb_get_configuration(struct usb_device *dev);
extern void usb_destroy_configuration(struct usb_device *dev);

/*-------------------------------------------------------------------------*/

/*
 * HCD Root Hub support
 */

#include <ch11.h>

/*
 * As of USB 2.0, full/low speed devices are segregated into trees.
 * One type grows from USB 1.1 host controllers (OHCI, UHCI etc).
 * The other type grows from high speed hubs when they connect to
 * full/low speed devices using "Transaction Translators" (TTs).
 *
 * TTs should only be known to the hub driver, and high speed bus
 * drivers (only EHCI for now).  They affect periodic scheduling and
 * sometimes control/bulk error recovery.
 */

struct usb_device;

#if 0
struct usb_tt
{
    struct usb_device    *hub;    /* upstream highspeed hub */
    int            multi;    /* true means one TT per port */
    unsigned int think_time;    /* think time in ns */

    /* for control/bulk error recovery (CLEAR_TT_BUFFER) */
    spinlock_t        lock;
    struct list_head    clear_list;    /* of usb_tt_clear */
    struct work_struct    clear_work;
};

struct usb_tt_clear
{
    struct list_head    clear_list;
    unsigned int tt;
    u16            devinfo;
    struct usb_hcd        *hcd;
    struct usb_host_endpoint    *ep;
};

#endif
/* extern int usb_hub_clear_tt_buffer(struct urb *urb); */
extern void usb_ep0_reinit(struct usb_device *);

/* (shifted) direction/type/recipient from the USB 2.0 spec, table 9.2 */
#define DeviceRequest \
    ((USB_DIR_IN|USB_TYPE_STANDARD|USB_RECIP_DEVICE)<<8)
#define DeviceOutRequest \
    ((USB_DIR_OUT|USB_TYPE_STANDARD|USB_RECIP_DEVICE)<<8)

#define InterfaceRequest \
    ((USB_DIR_IN|USB_TYPE_STANDARD|USB_RECIP_INTERFACE)<<8)

#define EndpointRequest \
    ((USB_DIR_IN|USB_TYPE_STANDARD|USB_RECIP_INTERFACE)<<8)
#define EndpointOutRequest \
    ((USB_DIR_OUT|USB_TYPE_STANDARD|USB_RECIP_INTERFACE)<<8)

/* class requests from the USB 2.0 hub spec, table 11-15 */
/* GetBusState and SetHubDescriptor are optional, omitted */
#define ClearHubFeature        (0x2000 | USB_REQ_CLEAR_FEATURE)
#define ClearPortFeature    (0x2300 | USB_REQ_CLEAR_FEATURE)
#define GetHubDescriptor    (0xa000 | USB_REQ_GET_DESCRIPTOR)
#define GetHubStatus        (0xa000 | USB_REQ_GET_STATUS)
#define GetPortStatus        (0xa300 | USB_REQ_GET_STATUS)
#define SetHubFeature        (0x2000 | USB_REQ_SET_FEATURE)
#define SetPortFeature        (0x2300 | USB_REQ_SET_FEATURE)


/*-------------------------------------------------------------------------*/

/* class requests from USB 3.0 hub spec, table 10-5 */
#define SetHubDepth        (0x3000 | HUB_SET_DEPTH)
#define GetPortErrorCount    (0x8000 | HUB_GET_PORT_ERR_COUNT)

/*
 * Generic bandwidth allocation constants/support
 */
#define FRAME_TIME_USECS    1000L
#define BitTime(bytecount) (7 * 8 * bytecount / 6) /* with integer truncation */
        /* Trying not to use worst-case bit-stuffing
         * of (7/6 * 8 * bytecount) = 9.33 * bytecount */
        /* bytecount = data payload byte count */

#define NS_TO_US(ns)    ((ns + 500L) / 1000L)
            /* convert & round nanoseconds to microseconds */


/*
 * Full/low speed bandwidth allocation constants/support.
 */
#define BW_HOST_DELAY    1000L        /* nanoseconds */
#define BW_HUB_LS_SETUP    333L        /* nanoseconds */
            /* 4 full-speed bit times (est.) */

#define FRAME_TIME_BITS            12000L    /* frame = 1 millisecond */
#define FRAME_TIME_MAX_BITS_ALLOC    (90L * FRAME_TIME_BITS / 100L)
#define FRAME_TIME_MAX_USECS_ALLOC    (90L * FRAME_TIME_USECS / 100L)

/*
 * Ceiling [nano/micro]seconds (typical) for that many bytes at high speed
 * ISO is a bit less, no ACK ... from USB 2.0 spec, 5.11.3 (and needed
 * to preallocate bandwidth)
 */
#define USB2_HOST_DELAY    5    /* nsec, guess */
#define HS_NSECS(bytes) (((55 * 8 * 2083) \
    + (2083UL * (3 + BitTime(bytes))))/1000 \
    + USB2_HOST_DELAY)
#define HS_NSECS_ISO(bytes) (((38 * 8 * 2083) \
    + (2083UL * (3 + BitTime(bytes))))/1000 \
    + USB2_HOST_DELAY)
#define HS_USECS(bytes)        NS_TO_US(HS_NSECS(bytes))
#define HS_USECS_ISO(bytes)    NS_TO_US(HS_NSECS_ISO(bytes))

extern long usb_calc_bus_time(int speed, int is_input,
            int isoc, int bytecount);

/*-------------------------------------------------------------------------*/

extern void usb_set_device_state(struct usb_device *udev,
        enum usb_device_state new_state);

/*-------------------------------------------------------------------------*/

/* exported only within usbcore */

extern rt_list_t usb_bus_list;
/* extern struct rt_mutex usb_bus_list_lock; */


#define usb_endpoint_out(ep_dir)    (!((ep_dir) & USB_DIR_IN))


/* Keep track of which host controller drivers are loaded */
#define USB_UHCI_LOADED        0
#define USB_OHCI_LOADED        1
#define USB_EHCI_LOADED        2
extern unsigned long usb_hcds_loaded;

#endif /* __USB_CORE_HCD_H */
