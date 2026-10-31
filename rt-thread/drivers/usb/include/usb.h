#ifndef __USB_H__
#define __USB_H__
#include "rtdef.h"
#include "rtthread.h"
#include "rthw.h"
/* #include "usb_errno.h" */
#include "mod_devicetable.h"
#include "ch9.h"
/* Functions local to drivers/usb/core/ */
extern rt_uint32_t rt_debug_usb;

extern rt_int32_t g_mem_debug;

extern const char *usbcore_name;

#define list_for_each_entry(pos, head, member) \
         rt_list_for_each_entry(pos, head, member)

#define container_of(ptr, type, member) \
        rt_container_of(ptr, type, member)

static inline void set_bit(int nr, volatile unsigned long *addr)
{
    unsigned long mask = 1<<(nr%32);
    unsigned long *p = ((unsigned long *)addr + nr/32);
    rt_base_t level;
    level = rt_hw_interrupt_disable();
    *p  |= mask;
    rt_hw_interrupt_enable(level);

}

static inline int test_bit(int nr, const volatile unsigned long *addr)
{
    unsigned long ret;
    rt_base_t level;
    level = rt_hw_interrupt_disable();
    ret = 1UL&(addr[nr/32] >> nr&0x1f);
    rt_hw_interrupt_enable(level);
    return ret;
}

static inline int test_and_set_bit(int nr, volatile unsigned long *addr)
{
    unsigned long mask =  1<<(nr%32);
    unsigned long *p = ((unsigned long *)addr + nr/32);
    unsigned long old;
    rt_base_t level;
    level = rt_hw_interrupt_disable();
    old = *p;
    *p = old | mask;
    rt_hw_interrupt_enable(level);

    return (old & mask) != 0;
}

static inline int test_and_clear_bit(int nr, volatile unsigned long *addr)
{
    unsigned long mask = 1<<(nr%32);
    unsigned long *p = ((unsigned long *)addr + nr/32);
    unsigned long old;
    rt_base_t level;
    level = rt_hw_interrupt_disable();
    old = *p;
    *p = old & ~mask;
    rt_hw_interrupt_enable(level);

    return (old & mask) != 0;
}

static inline void clear_bit(int nr, volatile unsigned long *addr)
{
    unsigned long mask =  1<<(nr%32);
    unsigned long *p = ((unsigned long *)addr + nr/32);
    rt_base_t level;
    level = rt_hw_interrupt_disable();
    *p &= ~mask;
    rt_hw_interrupt_enable(level);
}


/*-------------------------------------------------------------------------*/

/*
 * Host-side wrappers for standard USB descriptors ... these are parsed
 * from the data provided by devices.  Parsing turns them from a flat
 * sequence of descriptors into a hierarchy:
 *
 *  - devices have one (usually) or more configs;
 *  - configs have one (often) or more interfaces;
 *  - interfaces have one (usually) or more settings;
 *  - each interface setting has zero or (usually) more endpoints.
 *  - a SuperSpeed endpoint has a companion descriptor
 *
 * And there might be other descriptors mixed in with those.
 *
 * Devices may also have class-specific or vendor-specific descriptors.
 */

struct ep_device;

/**
 * struct usb_host_endpoint - host-side endpoint descriptor and queue
 * @desc: descriptor for this endpoint, wMaxPacketSize in native byteorder
 * @ss_ep_comp: SuperSpeed companion descriptor for this endpoint
 * @urb_list: urbs queued to this endpoint; maintained by usbcore
 * @hcpriv: for use by HCD; typically holds hardware dma queue head (QH)
 *    with one or more transfer descriptors (TDs) per urb
 * @ep_dev: ep_device for sysfs info
 * @extra: descriptors following this endpoint in the configuration
 * @extralen: how many bytes of "extra" are valid
 * @enabled: URBs may be submitted to this endpoint
 *
 * USB requests are always queued to a given endpoint, identified by a
 * descriptor within an active interface in a given USB configuration.
 */
struct usb_host_endpoint
{
    struct usb_endpoint_descriptor        desc;
    struct usb_ss_ep_comp_descriptor    ss_ep_comp;
    rt_list_t urb_list;
    void                *hcpriv;

    unsigned char *extra;   /* Extra descriptors */
    int extralen;
    int enabled;
};

/* host-side wrapper for one interface setting's parsed descriptors */
struct usb_host_interface
{
    struct usb_interface_descriptor    desc;

    /* array of desc.bNumEndpoint endpoints associated with this
     * interface setting.  these will be in no particular order.
     */
    struct usb_host_endpoint *endpoint;

    char *string;        /* iInterface string, if present */
    unsigned char *extra;   /* Extra descriptors */
    int extralen;
};

enum usb_interface_condition
{
    USB_INTERFACE_UNBOUND = 0,
    USB_INTERFACE_BINDING,
    USB_INTERFACE_BOUND,
    USB_INTERFACE_UNBINDING,
};

/**
 * struct usb_interface - what usb device drivers talk to
 * @altsetting: array of interface structures, one for each alternate
 *    setting that may be selected.  Each one includes a set of
 *    endpoint configurations.  They will be in no particular order.
 * @cur_altsetting: the current altsetting.
 * @num_altsetting: number of altsettings defined.
 * @intf_assoc: interface association descriptor
 * @minor: the minor number assigned to this interface, if this
 *    interface is bound to a driver that uses the USB major number.
 *    If this interface does not use the USB major, this field should
 *    be unused.  The driver should set this value in the probe()
 *    function of the driver, after it has been assigned a minor
 *    number from the USB core by calling usb_register_dev().
 * @condition: binding state of the interface: not bound, binding
 *    (in probe()), bound to a driver, or unbinding (in disconnect())
 * @sysfs_files_created: sysfs attributes exist
 * @ep_devs_created: endpoint child pseudo-devices exist
 * @unregistering: flag set when the interface is being unregistered
 * @needs_remote_wakeup: flag set when the driver requires remote-wakeup
 *    capability during autosuspend.
 * @needs_altsetting0: flag set when a set-interface request for altsetting 0
 *    has been deferred.
 * @needs_binding: flag set when the driver should be re-probed or unbound
 *    following a reset or suspend operation it doesn't support.
 * @dev: driver model's view of this device
 * @usb_dev: if an interface is bound to the USB major, this will point
 *    to the sysfs representation for that device.
 * @pm_usage_cnt: PM usage counter for this interface
 * @reset_ws: Used for scheduling resets from atomic context.
 * @reset_running: set to 1 if the interface is currently running a
 *      queued reset so that usb_cancel_queued_reset() doesn't try to
 *      remove from the workqueue when running inside the worker
 *      thread. See __usb_queue_reset_device().
 * @resetting_device: USB core reset the device, so use alt setting 0 as
 *    current; needs bandwidth alloc after reset.
 *
 * USB device drivers attach to interfaces on a physical device.  Each
 * interface encapsulates a single high level function, such as feeding
 * an audio stream to a speaker or reporting a change in a volume control.
 * Many USB devices only have one interface.  The protocol used to talk to
 * an interface's endpoints can be defined in a usb "class" specification,
 * or by a product's vendor.  The (default) control endpoint is part of
 * every interface, but is never listed among the interface's descriptors.
 *
 * The driver that is bound to the interface can use standard driver model
 * calls such as dev_get_drvdata() on the dev member of this structure.
 *
 * Each interface may have alternate settings.  The initial configuration
 * of a device sets altsetting 0, but the device driver can change
 * that setting using usb_set_interface().  Alternate settings are often
 * used to control the use of periodic endpoints, such as by having
 * different endpoints use different amounts of reserved USB bandwidth.
 * All standards-conformant USB devices that use isochronous endpoints
 * will use them in non-default settings.
 *
 * The USB specification says that alternate setting numbers must run from
 * 0 to one less than the total number of alternate settings.  But some
 * devices manage to mess this up, and the structures aren't necessarily
 * stored in numerical order anyhow.  Use usb_altnum_to_altsetting() to
 * look up an alternate setting in the altsetting array based on its number.
 */
struct usb_interface
{
    /* array of alternate settings for this interface,
     * stored in no particular order */
    struct usb_host_interface *altsetting;

    struct usb_host_interface *cur_altsetting;    /* the currently
                     * active alternate setting */
    unsigned int num_altsetting;    /* number of alternate settings */

    /* If there is an interface association descriptor then it will list
     * the associated interfaces */
    struct usb_interface_assoc_descriptor *intf_assoc;

    enum usb_interface_condition condition;        /* state of binding */
    unsigned unregistering:1;    /* unregistration is in progress */
    unsigned needs_remote_wakeup:1;    /* driver requires remote wakeup */
    unsigned needs_altsetting0:1;    /* switch to altsetting 0 is pending */
    unsigned needs_binding:1;    /* needs delayed unbind/rebind */
    unsigned reset_running:1;
    unsigned resetting_device:1;    /* true: bandwidth alloc after reset */

    struct rt_device dev;        /* interface specific device info */
    struct usb_device *usb_dev;
    void *user_data;
    struct usb_driver *driver;
    void *ex_wifi_dev;
};
#define    to_usb_interface(d) container_of(d, struct usb_interface, dev)

/* this maximum is arbitrary */
#define USB_MAXINTERFACES    32
#define USB_MAXIADS        (USB_MAXINTERFACES/2)

/**
 * struct usb_interface_cache - long-term representation of a device interface
 * @num_altsetting: number of altsettings defined.
 * @ref: reference counter.
 * @altsetting: variable-length array of interface structures, one for
 *    each alternate setting that may be selected.  Each one includes a
 *    set of endpoint configurations.  They will be in no particular order.
 *
 * These structures persist for the lifetime of a usb_device, unlike
 * struct usb_interface (which persists only as long as its configuration
 * is installed).  The altsetting arrays can be accessed through these
 * structures at any time, permitting comparison of configurations and
 * providing support for the /proc/bus/usb/devices pseudo-file.
 */
struct usb_interface_cache
{
    unsigned int num_altsetting;    /* number of alternate settings */
    rt_uint32_t ref;        /* reference counter */

    /* variable-length array of alternate settings for this interface,
     * stored in no particular order */
    struct usb_host_interface altsetting[0];
};
#define    ref_to_usb_interface_cache(r) \
        container_of(r, struct usb_interface_cache, ref)
#define    altsetting_to_usb_interface_cache(a) \
        container_of(a, struct usb_interface_cache, altsetting[0])

/**
 * struct usb_host_config - representation of a device's configuration
 * @desc: the device's configuration descriptor.
 * @string: pointer to the cached version of the iConfiguration string, if
 *    present for this configuration.
 * @intf_assoc: list of any interface association descriptors in this config
 * @interface: array of pointers to usb_interface structures, one for each
 *    interface in the configuration.  The number of interfaces is stored
 *    in desc.bNumInterfaces.  These pointers are valid only while the
 *    the configuration is active.
 * @intf_cache: array of pointers to usb_interface_cache structures, one
 *    for each interface in the configuration.  These structures exist
 *    for the entire life of the device.
 * @extra: pointer to buffer containing all extra descriptors associated
 *    with this configuration (those preceding the first interface
 *    descriptor).
 * @extralen: length of the extra descriptors buffer.
 *
 * USB devices may have multiple configurations, but only one can be active
 * at any time.  Each encapsulates a different operational environment;
 * for example, a dual-speed device would have separate configurations for
 * full-speed and high-speed operation.  The number of configurations
 * available is stored in the device descriptor as bNumConfigurations.
 *
 * A configuration can contain multiple interfaces.  Each corresponds to
 * a different function of the USB device, and all are available whenever
 * the configuration is active.  The USB standard says that interfaces
 * are supposed to be numbered from 0 to desc.bNumInterfaces-1, but a lot
 * of devices get this wrong.  In addition, the interface array is not
 * guaranteed to be sorted in numerical order.  Use usb_ifnum_to_if() to
 * look up an interface entry based on its number.
 *
 * Device drivers should not attempt to activate configurations.  The choice
 * of which configuration to install is a policy decision based on such
 * considerations as available power, functionality provided, and the user's
 * desires (expressed through userspace tools).  However, drivers can call
 * usb_reset_configuration() to reinitialize the current configuration and
 * all its interfaces.
 */
struct usb_host_config
{
    struct usb_config_descriptor    desc;

    char *string;        /* iConfiguration string, if present */

    /* List of any Interface Association Descriptors in this
     * configuration. */
    struct usb_interface_assoc_descriptor *intf_assoc[USB_MAXIADS];

    /* the interfaces associated with this configuration,
     * stored in no particular order */
    struct usb_interface *interface[USB_MAXINTERFACES];

    /* Interface information available even when this is not the
     * active configuration */
    struct usb_interface_cache *intf_cache[USB_MAXINTERFACES];

    unsigned char *extra;   /* Extra descriptors */
    int extralen;
};

int __usb_get_extra_descriptor(char *buffer, unsigned int size,
    unsigned char type, void **ptr);
#define usb_get_extra_descriptor(ifpoint, type, ptr) \
                __usb_get_extra_descriptor((ifpoint)->extra, \
                (ifpoint)->extralen, \
                type, (void **)ptr)

/* ----------------------------------------------------------------------- */

/* USB device number allocation bitmap */
struct usb_devmap
{
    unsigned long devicemap[128 / (8*sizeof(unsigned long))];
};

/*
 * Allocated per bus (tree of devices) we have:
 */
struct usb_bus
{
    rt_device_t controller;    /* host/master side hardware */
    int busnum;            /* Bus number (in order of reg) */
    const char *bus_name;        /* stable id (PCI slot_name etc) */
    rt_uint8_t uses_dma;            /* Does the host controller use DMA? */
    rt_uint8_t  otg_port;            /* 0, or number of OTG/HNP port */
    unsigned is_b_host:1;        /* true during some HNP roleswitches */
    unsigned b_hnp_enable:1;    /* OTG: did A-Host enable HNP? */

    int devnum_next;        /* Next open device number in
                     * round-robin allocation */

    struct usb_devmap devmap;    /* device address allocation map */
    struct usb_device *root_hub;    /* Root hub */
    rt_list_t  bus_list;    /* list of busses */

    int bandwidth_allocated;    /* on this bus: how much of the time
                     * reserved for periodic (intr/iso)
                     * requests is used, on average?
                     * Units: microseconds/frame.
                     * Limits: Full/low speed reserve 90%,
                     * while high speed reserves 80%.
                     */
    int bandwidth_int_reqs;        /* number of Interrupt requests */
    int bandwidth_isoc_reqs;    /* number of Isoc. requests */
#define USB_DMA_BUFFER_LENGTH       4
    rt_list_t  dma_addr[USB_DMA_BUFFER_LENGTH];
};

/* ----------------------------------------------------------------------- */

/* This is arbitrary.
 * From USB 2.0 spec Table 11-13, offset 7, a hub can
 * have up to 255 ports. The most yet reported is 10.
 *
 * Current Wireless USB host hardware (Intel i1480 for example) allows
 * up to 22 devices to connect. Upcoming hardware might raise that
 * limit. Because the arrays need to add a bit for hub status data, we
 * do 31, so plus one evens out to four bytes.
 */
#define USB_MAXCHILDREN        (31)

/* struct usb_tt; */

/**
 * struct usb_device - kernel's representation of a USB device
 * @devnum: device number; address on a USB bus
 * @devpath: device ID string for use in messages (e.g., /port/...)
 * @route: tree topology hex string for use with xHCI
 * @state: device state: configured, not attached, etc.
 * @speed: device speed: high/full/low (or error)
 * @tt: Transaction Translator info; used with low/full speed dev, highspeed hub
 * @ttport: device port on that tt hub
 * @toggle: one bit for each endpoint, with ([0] = IN, [1] = OUT) endpoints
 * @parent: our hub, unless we're the root
 * @bus: bus we're part of
 * @ep0: endpoint 0 data (default control pipe)
 * @dev: generic device interface
 * @descriptor: USB device descriptor
 * @config: all of the device's configs
 * @actconfig: the active configuration
 * @ep_in: array of IN endpoints
 * @ep_out: array of OUT endpoints
 * @rawdescriptors: raw descriptors for each config
 * @bus_mA: Current available from the bus
 * @portnum: parent port number (origin 1)
 * @level: number of USB hub ancestors
 * @can_submit: URBs may be submitted
 * @persist_enabled:  USB_PERSIST enabled for this device
 * @have_langid: whether string_langid is valid
 * @authorized: policy has said we can use it;
 *    (user space) policy determines if we authorize this device to be
 *    used or not. By default, wired USB devices are authorized.
 *    WUSB devices are not, until we authorize them from user space.
 *    FIXME -- complete doc
 * @authenticated: Crypto authentication passed
 * @wusb: device is Wireless USB
 * @string_langid: language ID for strings
 * @product: iProduct string, if present (static)
 * @manufacturer: iManufacturer string, if present (static)
 * @serial: iSerialNumber string, if present (static)
 * @filelist: usbfs files that are open to this device
 * @usb_classdev: USB class device that was created for usbfs device
 *    access from userspace
 * @usbfs_dentry: usbfs dentry entry for the device
 * @maxchild: number of ports if hub
 * @children: child devices - USB devices that are attached to this hub
 * @quirks: quirks of the whole device
 * @urbnum: number of URBs submitted for the whole device
 * @active_duration: total time device is not suspended
 * @connect_time: time device was first connected
 * @do_remote_wakeup:  remote wakeup should be enabled
 * @reset_resume: needs reset instead of resume
 * @wusb_dev: if this is a Wireless USB device, link to the WUSB
 *    specific data for the device.
 * @slot_id: Slot ID assigned by xHCI
 *
 * Notes:
 * Usbcore drivers should not set usbdev->state directly.  Instead use
 * usb_set_device_state().
 */
struct usb_device
{
    int        devnum;
    char        devpath[16];
    rt_uint32_t route;
    enum usb_device_state    state;
    enum usb_device_speed    speed;

/* struct usb_tt    *tt; */
    int        ttport;

    unsigned int toggle[2];

    struct usb_device *parent;
    struct usb_bus *bus;
    struct usb_host_endpoint ep0;

/* struct rt_device dev; */

    struct usb_device_descriptor descriptor;
    struct usb_host_config *config;

    struct usb_host_config *actconfig;
    struct usb_host_endpoint *ep_in[16];
    struct usb_host_endpoint *ep_out[16];

    char **rawdescriptors;

    unsigned short bus_mA;
    rt_uint8_t portnum;
    rt_uint8_t  level;

    unsigned int have_langid;
    int string_langid;

    unsigned int wusb;

    int maxchild;
    struct usb_device *children[USB_MAXCHILDREN];

    rt_uint32_t quirks;
    rt_uint32_t urbnum;
};
/* #define    to_usb_device(d) container_of(d, struct usb_device, dev) */

static inline void *usb_get_intfdata(struct usb_interface *intf)
{
    return intf->user_data;
}

static inline void usb_set_intfdata(struct usb_interface *intf, void *data)
{
    intf->user_data = data;
}


/*-------------------------------------------------------------------------*/

/* for drivers using iso endpoints */
extern int usb_get_current_frame_number(struct usb_device *usb_dev);

/* Sets up a group of bulk endpoints to support multiple stream IDs. */
extern int usb_alloc_streams(struct usb_interface *interface,
        struct usb_host_endpoint **eps, unsigned int num_eps,
        unsigned int num_streams);

/* Reverts a group of bulk endpoints back to not using stream IDs. */
extern void usb_free_streams(struct usb_interface *interface,
        struct usb_host_endpoint **eps, unsigned int num_eps);

extern void usb_disable_device(struct usb_device *dev, int skip_ep0);
extern  int generic_probe(struct usb_device *udev);
extern int rt_dev_set_name(rt_device_t dev, const char *fmt, ...);
/**
 * usb_interface_claimed - returns true iff an interface is claimed
 * @iface: the interface being checked
 *
 * Returns true (nonzero) iff the interface is claimed, else false (zero).
 * Callers must own the driver model's usb bus readlock.  So driver
 * probe() entries don't need extra locking, but other call contexts
 * may need to explicitly claim that lock.
 *
 */

const struct usb_device_id *usb_match_id(struct usb_interface *interface,
                     const struct usb_device_id *id);
extern int usb_match_one_id(struct usb_interface *interface,
                const struct usb_device_id *id);

extern struct usb_interface *usb_ifnum_to_if(const struct usb_device *dev,
        unsigned int ifnum);
extern struct usb_host_interface *usb_altnum_to_altsetting(
        const struct usb_interface *intf, unsigned int altnum);
extern struct usb_host_interface *usb_find_alt_setting(
        struct usb_host_config *config,
        unsigned int iface_num,
        unsigned int alt_num);

extern int usb_device_add(struct usb_interface *dev);

static inline int usb_make_path(struct usb_device *dev, char *buf, rt_uint32_t size)
{
    int actual;

    actual = rt_snprintf(buf, size, "usb-%s-%s", dev->bus->bus_name,
              dev->devpath);
    return (actual >= (int)size) ? -1 : actual;
}
#if 0
/* translate USB error codes to codes user space understands */
static inline int usb_translate_errors(int error_code)
{
    switch (error_code)
    {
    case 0:
    case -ENOMEM:
    case -ENODEV:
        return error_code;
    default:
        return -EIO;
    }
}
#endif

/*-------------------------------------------------------------------------*/

#define USB_DEVICE_ID_MATCH_DEVICE \
        (USB_DEVICE_ID_MATCH_VENDOR | USB_DEVICE_ID_MATCH_PRODUCT)
#define USB_DEVICE_ID_MATCH_DEV_RANGE \
        (USB_DEVICE_ID_MATCH_DEV_LO | USB_DEVICE_ID_MATCH_DEV_HI)
#define USB_DEVICE_ID_MATCH_DEVICE_AND_VERSION \
        (USB_DEVICE_ID_MATCH_DEVICE | USB_DEVICE_ID_MATCH_DEV_RANGE)
#define USB_DEVICE_ID_MATCH_DEV_INFO \
        (USB_DEVICE_ID_MATCH_DEV_CLASS | \
        USB_DEVICE_ID_MATCH_DEV_SUBCLASS | \
        USB_DEVICE_ID_MATCH_DEV_PROTOCOL)
#define USB_DEVICE_ID_MATCH_INT_INFO \
        (USB_DEVICE_ID_MATCH_INT_CLASS | \
        USB_DEVICE_ID_MATCH_INT_SUBCLASS | \
        USB_DEVICE_ID_MATCH_INT_PROTOCOL)

/**
 * USB_DEVICE - macro used to describe a specific usb device
 * @vend: the 16 bit USB Vendor ID
 * @prod: the 16 bit USB Product ID
 *
 * This macro is used to create a struct usb_device_id that matches a
 * specific device.
 */
#define USB_DEVICE(vend, prod) \
    .match_flags = USB_DEVICE_ID_MATCH_DEVICE, \
    .idVendor = (vend), \
    .idProduct = (prod)
/**
 * USB_DEVICE_VER - describe a specific usb device with a version range
 * @vend: the 16 bit USB Vendor ID
 * @prod: the 16 bit USB Product ID
 * @lo: the bcdDevice_lo value
 * @hi: the bcdDevice_hi value
 *
 * This macro is used to create a struct usb_device_id that matches a
 * specific device, with a version range.
 */
#define USB_DEVICE_VER(vend, prod, lo, hi) \
    .match_flags = USB_DEVICE_ID_MATCH_DEVICE_AND_VERSION, \
    .idVendor = (vend), \
    .idProduct = (prod), \
    .bcdDevice_lo = (lo), \
    .bcdDevice_hi = (hi)

/**
 * USB_DEVICE_INTERFACE_PROTOCOL - describe a usb device with a specific interface protocol
 * @vend: the 16 bit USB Vendor ID
 * @prod: the 16 bit USB Product ID
 * @pr: bInterfaceProtocol value
 *
 * This macro is used to create a struct usb_device_id that matches a
 * specific interface protocol of devices.
 */
#define USB_DEVICE_INTERFACE_PROTOCOL(vend, prod, pr) \
    .match_flags = USB_DEVICE_ID_MATCH_DEVICE | \
               USB_DEVICE_ID_MATCH_INT_PROTOCOL, \
    .idVendor = (vend), \
    .idProduct = (prod), \
    .bInterfaceProtocol = (pr)

/**
 * USB_DEVICE_INFO - macro used to describe a class of usb devices
 * @cl: bDeviceClass value
 * @sc: bDeviceSubClass value
 * @pr: bDeviceProtocol value
 *
 * This macro is used to create a struct usb_device_id that matches a
 * specific class of devices.
 */
#define USB_DEVICE_INFO(cl, sc, pr) \
    .match_flags = USB_DEVICE_ID_MATCH_DEV_INFO, \
    .bDeviceClass = (cl), \
    .bDeviceSubClass = (sc), \
    .bDeviceProtocol = (pr)

/**
 * USB_INTERFACE_INFO - macro used to describe a class of usb interfaces
 * @cl: bInterfaceClass value
 * @sc: bInterfaceSubClass value
 * @pr: bInterfaceProtocol value
 *
 * This macro is used to create a struct usb_device_id that matches a
 * specific class of interfaces.
 */
#define USB_INTERFACE_INFO(cl, sc, pr) \
    .match_flags = USB_DEVICE_ID_MATCH_INT_INFO, \
    .bInterfaceClass = (cl), \
    .bInterfaceSubClass = (sc), \
    .bInterfaceProtocol = (pr)

/**
 * USB_DEVICE_AND_INTERFACE_INFO - describe a specific usb device with a class of usb interfaces
 * @vend: the 16 bit USB Vendor ID
 * @prod: the 16 bit USB Product ID
 * @cl: bInterfaceClass value
 * @sc: bInterfaceSubClass value
 * @pr: bInterfaceProtocol value
 *
 * This macro is used to create a struct usb_device_id that matches a
 * specific device with a specific class of interfaces.
 *
 * This is especially useful when explicitly matching devices that have
 * vendor specific bDeviceClass values, but standards-compliant interfaces.
 */
#define USB_DEVICE_AND_INTERFACE_INFO(vend, prod, cl, sc, pr) \
    .match_flags = USB_DEVICE_ID_MATCH_INT_INFO \
        | USB_DEVICE_ID_MATCH_DEVICE, \
    .idVendor = (vend), \
    .idProduct = (prod), \
    .bInterfaceClass = (cl), \
    .bInterfaceSubClass = (sc), \
    .bInterfaceProtocol = (pr)

/* ----------------------------------------------------------------------- */

/* Stuff for dynamic usb ids */
struct usb_dynids
{
    rt_list_t list;
};

struct usb_dynid
{
     rt_list_t node;
    struct usb_device_id id;
};



/**
 * struct usb_driver - identifies USB interface driver to usbcore
 * @name: The driver name should be unique among USB drivers,
 *    and should normally be the same as the module name.
 * @probe: Called to see if the driver is willing to manage a particular
 *    interface on a device.  If it is, probe returns zero and uses
 *    usb_set_intfdata() to associate driver-specific data with the
 *    interface.  It may also use usb_set_interface() to specify the
 *    appropriate altsetting.  If unwilling to manage the interface,
 *    return -ENODEV, if genuine IO errors occurred, an appropriate
 *    negative errno value.
 * @disconnect: Called when the interface is no longer accessible, usually
 *    because its device has been (or is being) disconnected or the
 *    driver module is being unloaded.
 * @unlocked_ioctl: Used for drivers that want to talk to userspace through
 *    the "usbfs" filesystem.  This lets devices provide ways to
 *    expose information to user space regardless of where they
 *    do (or don't) show up otherwise in the filesystem.
 * @suspend: Called when the device is going to be suspended by the system.
 * @resume: Called when the device is being resumed by the system.
 * @reset_resume: Called when the suspended device has been reset instead
 *    of being resumed.
 * @pre_reset: Called by usb_reset_device() when the device is about to be
 *    reset.  This routine must not return until the driver has no active
 *    URBs for the device, and no more URBs may be submitted until the
 *    post_reset method is called.
 * @post_reset: Called by usb_reset_device() after the device
 *    has been reset
 * @id_table: USB drivers use ID table to support hotplugging.
 *    Export this with MODULE_DEVICE_TABLE(usb,...).  This must be set
 *    or your driver's probe function will never get called.
 * @dynids: used internally to hold the list of dynamically added device
 *    ids for this driver.
 * @drvwrap: Driver-model core structure wrapper.
 * @no_dynamic_id: if set to 1, the USB core will not allow dynamic ids to be
 *    added to this driver by preventing the sysfs file from being created.
 * @supports_autosuspend: if set to 0, the USB core will not allow autosuspend
 *    for interfaces bound to this driver.
 * @soft_unbind: if set to 1, the USB core will not kill URBs and disable
 *    endpoints before calling the driver's disconnect method.
 *
 * USB interface drivers must provide a name, probe() and disconnect()
 * methods, and an id_table.  Other driver fields are optional.
 *
 * The id_table is used in hotplugging.  It holds a set of descriptors,
 * and specialized data may be associated with each entry.  That table
 * is used by both user and kernel mode hotplugging support.
 *
 * The probe() and disconnect() methods are called in a context where
 * they can sleep, but they should avoid abusing the privilege.  Most
 * work to connect to a device should be done when the device is opened,
 * and undone at the last close.  The disconnect code needs to address
 * concurrency issues with respect to open() and close() methods, as
 * well as forcing all pending I/O requests to complete (by unlinking
 * them as necessary, and blocking until the unlinks complete).
 */
struct usb_driver
{
    const char *name;

    int (*probe)(struct usb_interface *intf,
              const struct usb_device_id *id);

    void (*disconnect)(struct usb_interface *intf);

    const struct usb_device_id *id_table;
    rt_list_t list;

    struct usb_dynids dynids;
    unsigned int no_dynamic_id;
};



/*
 * use these in module_init()/module_exit()
 * and don't forget MODULE_DEVICE_TABLE(usb, ...)
 */
int usb_register_driver(struct usb_driver *driver);

void usb_deregister(struct usb_driver *driver);

extern int find_next_zero_bit(rt_uint32_t *p, rt_uint32_t size, rt_uint32_t offset);
extern void usb_detect_quirks(struct usb_device *udev);


/* ----------------------------------------------------------------------- */

/*
 * URB support, for asynchronous request completions
 */

/*
 * urb->transfer_flags:
 *
 * Note: URB_DIR_IN/OUT is automatically set in usb_submit_urb().
 */
#define URB_SHORT_NOT_OK    0x0001    /* report short reads as errors */
#define URB_ISO_ASAP        0x0002    /* iso-only, urb->start_frame
                     * ignored */
#define URB_NO_TRANSFER_DMA_MAP    0x0004    /* urb->transfer_dma valid on submit */
#define URB_NO_FSBR        0x0020    /* UHCI-specific */
#define URB_ZERO_PACKET        0x0040    /* Finish bulk OUT with short packet */
#define URB_NO_INTERRUPT    0x0080    /* HINT: no non-error interrupt
                     * needed */
#define URB_FREE_BUFFER        0x0100    /* Free transfer buffer with the URB */

/* The following flags are used internally by usbcore and HCDs */
#define URB_DIR_IN        0x0200    /* Transfer from device to host */
#define URB_DIR_OUT        0
#define URB_DIR_MASK        URB_DIR_IN

#define URB_DMA_MAP_SINGLE    0x00010000    /* Non-scatter-gather mapping */
#define URB_DMA_MAP_PAGE    0x00020000    /* HCD-unsupported S-G */
#define URB_DMA_MAP_SG        0x00040000    /* HCD-supported S-G */
#define URB_MAP_LOCAL        0x00080000    /* HCD-local-memory mapping */
#define URB_SETUP_MAP_SINGLE    0x00100000    /* Setup packet DMA mapped */
#define URB_SETUP_MAP_LOCAL    0x00200000    /* HCD-local setup packet */
#define URB_DMA_SG_COMBINED    0x00400000    /* S-G entries were combined */
#define URB_ALIGNED_TEMP_BUFFER    0x00800000    /* Temp buffer was alloc'd */

struct usb_iso_packet_descriptor
{
    unsigned int offset;
    unsigned int length;        /* expected length */
    unsigned int actual_length;
    int status;
};

struct urb;

typedef void (*usb_complete_t)(struct urb *);

/**
 * struct urb - USB Request Block
 * @urb_list: For use by current owner of the URB.
 * @anchor_list: membership in the list of an anchor
 * @anchor: to anchor URBs to a common mooring
 * @ep: Points to the endpoint's data structure.  Will eventually
 *    replace @pipe.
 * @pipe: Holds endpoint number, direction, type, and more.
 *    Create these values with the eight macros available;
 *    usb_{snd,rcv}TYPEpipe(dev,endpoint), where the TYPE is "ctrl"
 *    (control), "bulk", "int" (interrupt), or "iso" (isochronous).
 *    For example usb_sndbulkpipe() or usb_rcvintpipe().  Endpoint
 *    numbers range from zero to fifteen.  Note that "in" endpoint two
 *    is a different endpoint (and pipe) from "out" endpoint two.
 *    The current configuration controls the existence, type, and
 *    maximum packet size of any given endpoint.
 * @stream_id: the endpoint's stream ID for bulk streams
 * @dev: Identifies the USB device to perform the request.
 * @status: This is read in non-iso completion functions to get the
 *    status of the particular request.  ISO requests only use it
 *    to tell whether the URB was unlinked; detailed status for
 *    each frame is in the fields of the iso_frame-desc.
 * @transfer_flags: A variety of flags may be used to affect how URB
 *    submission, unlinking, or operation are handled.  Different
 *    kinds of URB can use different flags.
 * @transfer_buffer:  This identifies the buffer to (or from) which the I/O
 *    request will be performed unless URB_NO_TRANSFER_DMA_MAP is set
 *    (however, do not leave garbage in transfer_buffer even then).
 *    This buffer must be suitable for DMA; allocate it with
 *    kmalloc() or equivalent.  For transfers to "in" endpoints, contents
 *    of this buffer will be modified.  This buffer is used for the data
 *    stage of control transfers.
 * @transfer_dma: When transfer_flags includes URB_NO_TRANSFER_DMA_MAP,
 *    the device driver is saying that it provided this DMA address,
 *    which the host controller driver should use in preference to the
 *    transfer_buffer.
 * @sg: scatter gather buffer list
 * @num_sgs: number of entries in the sg list
 * @transfer_buffer_length: How big is transfer_buffer.  The transfer may
 *    be broken up into chunks according to the current maximum packet
 *    size for the endpoint, which is a function of the configuration
 *    and is encoded in the pipe.  When the length is zero, neither
 *    transfer_buffer nor transfer_dma is used.
 * @actual_length: This is read in non-iso completion functions, and
 *    it tells how many bytes (out of transfer_buffer_length) were
 *    transferred.  It will normally be the same as requested, unless
 *    either an error was reported or a short read was performed.
 *    The URB_SHORT_NOT_OK transfer flag may be used to make such
 *    short reads be reported as errors.
 * @setup_packet: Only used for control transfers, this points to eight bytes
 *    of setup data.  Control transfers always start by sending this data
 *    to the device.  Then transfer_buffer is read or written, if needed.
 * @setup_dma: DMA pointer for the setup packet.  The caller must not use
 *    this field; setup_packet must point to a valid buffer.
 * @start_frame: Returns the initial frame for isochronous transfers.
 * @number_of_packets: Lists the number of ISO transfer buffers.
 * @interval: Specifies the polling interval for interrupt or isochronous
 *    transfers.  The units are frames (milliseconds) for full and low
 *    speed devices, and microframes (1/8 millisecond) for highspeed
 *    and SuperSpeed devices.
 * @error_count: Returns the number of ISO transfers that reported errors.
 * @context: For use in completion functions.  This normally points to
 *    request-specific driver context.
 * @complete: Completion handler. This URB is passed as the parameter to the
 *    completion function.  The completion function may then do what
 *    it likes with the URB, including resubmitting or freeing it.
 * @iso_frame_desc: Used to provide arrays of ISO transfer buffers and to
 *    collect the transfer status for each buffer.
 *
 * This structure identifies USB transfer requests.  URBs must be allocated by
 * calling usb_alloc_urb() and freed with a call to usb_free_urb().
 * Initialization may be done using various usb_fill_*_urb() functions.  URBs
 * are submitted using usb_submit_urb(), and pending requests may be canceled
 * using usb_unlink_urb() or usb_kill_urb().
 *
 * Data Transfer Buffers:
 *
 * Normally drivers provide I/O buffers allocated with kmalloc() or otherwise
 * taken from the general page pool.  That is provided by transfer_buffer
 * (control requests also use setup_packet), and host controller drivers
 * perform a dma mapping (and unmapping) for each buffer transferred.  Those
 * mapping operations can be expensive on some platforms (perhaps using a dma
 * bounce buffer or talking to an IOMMU),
 * although they're cheap on commodity x86 and ppc hardware.
 *
 * Alternatively, drivers may pass the URB_NO_TRANSFER_DMA_MAP transfer flag,
 * which tells the host controller driver that no such mapping is needed for
 * the transfer_buffer since
 * the device driver is DMA-aware.  For example, a device driver might
 * allocate a DMA buffer with usb_alloc_coherent() or call usb_buffer_map().
 * When this transfer flag is provided, host controller drivers will
 * attempt to use the dma address found in the transfer_dma
 * field rather than determining a dma address themselves.
 *
 * Note that transfer_buffer must still be set if the controller
 * does not support DMA (as indicated by bus.uses_dma) and when talking
 * to root hub. If you have to trasfer between highmem zone and the device
 * on such controller, create a bounce buffer or bail out with an error.
 * If transfer_buffer cannot be set (is in highmem) and the controller is DMA
 * capable, assign NULL to it, so that usbmon knows not to use the value.
 * The setup_packet must always be set, so it cannot be located in highmem.
 *
 * Initialization:
 *
 * All URBs submitted must initialize the dev, pipe, transfer_flags (may be
 * zero), and complete fields.  All URBs must also initialize
 * transfer_buffer and transfer_buffer_length.  They may provide the
 * URB_SHORT_NOT_OK transfer flag, indicating that short reads are
 * to be treated as errors; that flag is invalid for write requests.
 *
 * Bulk URBs may
 * use the URB_ZERO_PACKET transfer flag, indicating that bulk OUT transfers
 * should always terminate with a short packet, even if it means adding an
 * extra zero length packet.
 *
 * Control URBs must provide a valid pointer in the setup_packet field.
 * Unlike the transfer_buffer, the setup_packet may not be mapped for DMA
 * beforehand.
 *
 * Interrupt URBs must provide an interval, saying how often (in milliseconds
 * or, for highspeed devices, 125 microsecond units)
 * to poll for transfers.  After the URB has been submitted, the interval
 * field reflects how the transfer was actually scheduled.
 * The polling interval may be more frequent than requested.
 * For example, some controllers have a maximum interval of 32 milliseconds,
 * while others support intervals of up to 1024 milliseconds.
 * Isochronous URBs also have transfer intervals.  (Note that for isochronous
 * endpoints, as well as high speed interrupt endpoints, the encoding of
 * the transfer interval in the endpoint descriptor is logarithmic.
 * Device drivers must convert that value to linear units themselves.)
 *
 * Isochronous URBs normally use the URB_ISO_ASAP transfer flag, telling
 * the host controller to schedule the transfer as soon as bandwidth
 * utilization allows, and then set start_frame to reflect the actual frame
 * selected during submission.  Otherwise drivers must specify the start_frame
 * and handle the case where the transfer can't begin then.  However, drivers
 * won't know how bandwidth is currently allocated, and while they can
 * find the current frame using usb_get_current_frame_number () they can't
 * know the range for that frame number.  (Ranges for frame counter values
 * are HC-specific, and can go from 256 to 65536 frames from "now".)
 *
 * Isochronous URBs have a different data transfer model, in part because
 * the quality of service is only "best effort".  Callers provide specially
 * allocated URBs, with number_of_packets worth of iso_frame_desc structures
 * at the end.  Each such packet is an individual ISO transfer.  Isochronous
 * URBs are normally queued, submitted by drivers to arrange that
 * transfers are at least double buffered, and then explicitly resubmitted
 * in completion handlers, so
 * that data (such as audio or video) streams at as constant a rate as the
 * host controller scheduler can support.
 *
 * Completion Callbacks:
 *
 * The completion callback is made in_interrupt(), and one of the first
 * things that a completion handler should do is check the status field.
 * The status field is provided for all URBs.  It is used to report
 * unlinked URBs, and status for all non-ISO transfers.  It should not
 * be examined before the URB is returned to the completion handler.
 *
 * The context field is normally used to link URBs back to the relevant
 * driver or request state.
 *
 * When the completion callback is invoked for non-isochronous URBs, the
 * actual_length field tells how many bytes were transferred.  This field
 * is updated even when the URB terminated with an error or was unlinked.
 *
 * ISO transfer status is reported in the status and actual_length fields
 * of the iso_frame_desc array, and the number of errors is reported in
 * error_count.  Completion callbacks for ISO transfers will normally
 * (re)submit URBs to ensure a constant transfer rate.
 *
 * Note that even fields marked "public" should not be touched by the driver
 * when the urb is owned by the hcd, that is, since the call to
 * usb_submit_urb() till the entry into the completion routine.
 */
struct urb
{
    /* private: usb core and host controller only fields in the urb */
    rt_uint32_t kref;        /* reference count of the URB */
    void *hcpriv;            /* private data for host controller */
    rt_uint32_t use_count;        /* concurrent submissions counter */
    rt_uint32_t reject;        /* submissions will fail */
    int unlinked;            /* unlink error code */

    /* public: documented fields in the urb that can be used by drivers */
    rt_list_t urb_list;    /* list head for use by the urb's
                     * current owner */
    struct usb_device *dev;        /* (in) pointer to associated device */
    struct usb_host_endpoint *ep;    /* (internal) pointer to endpoint */
    unsigned int pipe;        /* (in) pipe information */
    unsigned int stream_id;        /* (in) stream ID */
    int status;            /* (return) non-ISO status */
    unsigned int transfer_flags;    /* (in) URB_SHORT_NOT_OK | ...*/
    void *transfer_buffer;        /* (in) associated data buffer */
    unsigned int transfer_dma;
    unsigned int setup_dma;
    rt_uint32_t transfer_buffer_length;    /* (in) data buffer length */
    rt_uint32_t actual_length;        /* (return) actual transfer length */
    unsigned char *setup_packet;    /* (in) setup packet (control only) */
    int start_frame;        /* (modify) start frame (ISO) */
    int number_of_packets;        /* (in) number of ISO packets */
    int interval;            /* (modify) transfer interval
                     * (INT/ISO) */
    int error_count;        /* (return) number of ISO errors */
    void *context;            /* (in) context for completion */
    usb_complete_t complete;    /* (in) completion routine */
    struct usb_iso_packet_descriptor iso_frame_desc[0];
                    /* (in) ISO ONLY */
};

/* ----------------------------------------------------------------------- */

/**
 * usb_fill_control_urb - initializes a control urb
 * @urb: pointer to the urb to initialize.
 * @dev: pointer to the struct usb_device for this urb.
 * @pipe: the endpoint pipe
 * @setup_packet: pointer to the setup_packet buffer
 * @transfer_buffer: pointer to the transfer buffer
 * @buffer_length: length of the transfer buffer
 * @complete_fn: pointer to the usb_complete_t function
 * @context: what to set the urb context to.
 *
 * Initializes a control urb with the proper information needed to submit
 * it to a device.
 */
static inline void usb_fill_control_urb(struct urb *urb,
                    struct usb_device *dev,
                    unsigned int pipe,
                    unsigned char *setup_packet,
                    void *transfer_buffer,
                    int buffer_length,
                    usb_complete_t complete_fn,
                    void *context)
{
    urb->dev = dev;
    urb->pipe = pipe;
    urb->setup_packet = setup_packet;
    urb->transfer_buffer = transfer_buffer;
    urb->transfer_buffer_length = buffer_length;
    urb->complete = complete_fn;
    urb->context = context;
}

/**
 * usb_fill_bulk_urb - macro to help initialize a bulk urb
 * @urb: pointer to the urb to initialize.
 * @dev: pointer to the struct usb_device for this urb.
 * @pipe: the endpoint pipe
 * @transfer_buffer: pointer to the transfer buffer
 * @buffer_length: length of the transfer buffer
 * @complete_fn: pointer to the usb_complete_t function
 * @context: what to set the urb context to.
 *
 * Initializes a bulk urb with the proper information needed to submit it
 * to a device.
 */
static inline void usb_fill_bulk_urb(struct urb *urb,
                     struct usb_device *dev,
                     unsigned int pipe,
                     void *transfer_buffer,
                     int buffer_length,
                     usb_complete_t complete_fn,
                     void *context)
{
    urb->dev = dev;
    urb->pipe = pipe;
    urb->transfer_buffer = transfer_buffer;
    urb->transfer_buffer_length = buffer_length;
    urb->complete = complete_fn;
    urb->context = context;
}

/**
 * usb_fill_int_urb - macro to help initialize a interrupt urb
 * @urb: pointer to the urb to initialize.
 * @dev: pointer to the struct usb_device for this urb.
 * @pipe: the endpoint pipe
 * @transfer_buffer: pointer to the transfer buffer
 * @buffer_length: length of the transfer buffer
 * @complete_fn: pointer to the usb_complete_t function
 * @context: what to set the urb context to.
 * @interval: what to set the urb interval to, encoded like
 *    the endpoint descriptor's bInterval value.
 *
 * Initializes a interrupt urb with the proper information needed to submit
 * it to a device.
 *
 * Note that High Speed and SuperSpeed interrupt endpoints use a logarithmic
 * encoding of the endpoint interval, and express polling intervals in
 * microframes (eight per millisecond) rather than in frames (one per
 * millisecond).
 *
 * Wireless USB also uses the logarithmic encoding, but specifies it in units of
 * 128us instead of 125us.  For Wireless USB devices, the interval is passed
 * through to the host controller, rather than being translated into microframe
 * units.
 */
static inline void usb_fill_int_urb(struct urb *urb,
                    struct usb_device *dev,
                    unsigned int pipe,
                    void *transfer_buffer,
                    int buffer_length,
                    usb_complete_t complete_fn,
                    void *context,
                    int interval)
{
    urb->dev = dev;
    urb->pipe = pipe;
    urb->transfer_buffer = transfer_buffer;
    urb->transfer_buffer_length = buffer_length;
    urb->complete = complete_fn;
    urb->context = context;
    if (dev->speed == USB_SPEED_HIGH || dev->speed == USB_SPEED_SUPER)
        urb->interval = 1 << (interval - 1);
    else
        urb->interval = interval;
    urb->start_frame = -1;
}

void usb_init_urb(struct urb *urb);
struct urb *usb_alloc_urb(int iso_packets, int mem_flags);
void usb_free_urb(struct urb *urb);
#define usb_put_urb usb_free_urb
struct urb *usb_get_urb(struct urb *urb);
int usb_submit_urb(struct urb *urb, int mem_flags);
int usb_unlink_urb(struct urb *urb);
void usb_kill_urb(struct urb *urb);
extern int usb_hcd_submit_urb(struct urb *urb);
extern int usb_hcd_unlink_urb(struct urb *urb, int status);
/**
 * usb_urb_dir_in - check if an URB describes an IN transfer
 * @urb: URB to be checked
 *
 * Returns 1 if @urb describes an IN transfer (device-to-host),
 * otherwise 0.
 */
static inline int usb_urb_dir_in(struct urb *urb)
{
    return (urb->transfer_flags & URB_DIR_MASK) == URB_DIR_IN;
}

/**
 * usb_urb_dir_out - check if an URB describes an OUT transfer
 * @urb: URB to be checked
 *
 * Returns 1 if @urb describes an OUT transfer (host-to-device),
 * otherwise 0.
 */
static inline int usb_urb_dir_out(struct urb *urb)
{
    return (urb->transfer_flags & URB_DIR_MASK) == URB_DIR_OUT;
}

/*-------------------------------------------------------------------*
 *                         SYNCHRONOUS CALL SUPPORT                  *
 *-------------------------------------------------------------------*/

int usb_control_msg(struct usb_device *dev, unsigned int pipe,
    rt_uint8_t request, rt_uint8_t requesttype, rt_uint16_t value, rt_uint16_t index,
    void *data, rt_uint16_t size, int timeout);
int usb_interrupt_msg(struct usb_device *usb_dev, unsigned int pipe,
    void *data, int len, int *actual_length, int timeout);
int usb_bulk_msg(struct usb_device *usb_dev, unsigned int pipe,
    void *data, int len, int *actual_length,
    int timeout);

/* wrappers around usb_control_msg() for the most common standard requests */
 int usb_get_descriptor(struct usb_device *dev, unsigned char desctype,
    unsigned char descindex, void *buf, int size);
int usb_get_status(struct usb_device *dev,
    int type, int target, void *data);

/* wrappers that also update important state inside usbcore */
int usb_clear_halt(struct usb_device *dev, int pipe);
int usb_set_interface(struct usb_device *dev, int ifnum, int alternate);
void usb_reset_endpoint(struct usb_device *dev, unsigned int epaddr);

/*
 * timeouts, in milliseconds, used for sending/receiving control messages
 * they typically complete within a few frames (msec) after they're issued
 * USB identifies 5 second timeouts, maybe more in a few cases, and a few
 * slow devices (like some MGE Ellipse UPSes) actually push that limit.
 */
#define USB_CTRL_GET_TIMEOUT    500
#define USB_CTRL_SET_TIMEOUT    500

/* ----------------------------------------------------------------------- */

/*
 * For various legacy reasons, Linux has a small cookie that's paired with
 * a struct usb_device to identify an endpoint queue.  Queue characteristics
 * are defined by the endpoint's descriptor.  This cookie is called a "pipe",
 * an unsigned int encoded as:
 *
 *  - direction:    bit 7        (0 = Host-to-Device [Out],
 *                     1 = Device-to-Host [In] ...
 *                    like endpoint bEndpointAddress)
 *  - device address:    bits 8-14       ... bit positions known to uhci-hcd
 *  - endpoint:        bits 15-18      ... bit positions known to uhci-hcd
 *  - pipe type:    bits 30-31    (00 = isochronous, 01 = interrupt,
 *                     10 = control, 11 = bulk)
 *
 * Given the device address and endpoint descriptor, pipes are redundant.
 */

/* NOTE:  these are not the standard USB_ENDPOINT_XFER_* values!! */
/* (yet ... they're the values used by usbfs) */
#define PIPE_ISOCHRONOUS        0
#define PIPE_INTERRUPT            1
#define PIPE_CONTROL            2
#define PIPE_BULK            3

#define usb_pipein(pipe)    ((pipe) & USB_DIR_IN)
#define usb_pipeout(pipe)    (!usb_pipein(pipe))

#define usb_pipedevice(pipe)    (((pipe) >> 8) & 0x7f)
#define usb_pipeendpoint(pipe)    (((pipe) >> 15) & 0xf)

#define usb_pipetype(pipe)    (((pipe) >> 30) & 3)
#define usb_pipeisoc(pipe)    (usb_pipetype((pipe)) == PIPE_ISOCHRONOUS)
#define usb_pipeint(pipe)    (usb_pipetype((pipe)) == PIPE_INTERRUPT)
#define usb_pipecontrol(pipe)    (usb_pipetype((pipe)) == PIPE_CONTROL)
#define usb_pipebulk(pipe)    (usb_pipetype((pipe)) == PIPE_BULK)

static inline unsigned int __create_pipe(struct usb_device *dev,
        unsigned int endpoint)
{
    return (dev->devnum << 8) | (endpoint << 15);
}

/* Create various pipes... */
#define usb_sndctrlpipe(dev, endpoint)    \
    ((PIPE_CONTROL << 30) | __create_pipe(dev, endpoint))
#define usb_rcvctrlpipe(dev, endpoint)    \
    ((PIPE_CONTROL << 30) | __create_pipe(dev, endpoint) | USB_DIR_IN)
#define usb_sndisocpipe(dev, endpoint)    \
    ((PIPE_ISOCHRONOUS << 30) | __create_pipe(dev, endpoint))
#define usb_rcvisocpipe(dev, endpoint)    \
    ((PIPE_ISOCHRONOUS << 30) | __create_pipe(dev, endpoint) | USB_DIR_IN)
#define usb_sndbulkpipe(dev, endpoint)    \
    ((PIPE_BULK << 30) | __create_pipe(dev, endpoint))
#define usb_rcvbulkpipe(dev, endpoint)    \
    ((PIPE_BULK << 30) | __create_pipe(dev, endpoint) | USB_DIR_IN)
#define usb_sndintpipe(dev, endpoint)    \
    ((PIPE_INTERRUPT << 30) | __create_pipe(dev, endpoint))
#define usb_rcvintpipe(dev, endpoint)    \
    ((PIPE_INTERRUPT << 30) | __create_pipe(dev, endpoint) | USB_DIR_IN)

static inline struct usb_host_endpoint *
usb_pipe_endpoint(struct usb_device *dev, unsigned int pipe)
{
    struct usb_host_endpoint **eps;

    eps = usb_pipein(pipe) ? dev->ep_in : dev->ep_out;
    return eps[usb_pipeendpoint(pipe)];
}

/*-------------------------------------------------------------------------*/

static inline rt_uint16_t
usb_maxpacket(struct usb_device *udev, int pipe, int is_out)
{
    struct usb_host_endpoint    *ep;
    unsigned int epnum = usb_pipeendpoint(pipe);

    if (is_out)
    {

        ep = udev->ep_out[epnum];
    }
    else
    {
        ep = udev->ep_in[epnum];
    }
    if (!ep)
        return 0;

    /* NOTE:  only 0x07ff bits are for packet size... */
    return ep->desc.wMaxPacketSize;
}

/* ----------------------------------------------------------------------- */

/* Events from the usb core */
#define USB_DEVICE_ADD        0x0001
#define USB_DEVICE_REMOVE    0x0002
#define USB_BUS_ADD        0x0003
#define USB_BUS_REMOVE        0x0004

void usb_enable_endpoint(struct usb_device *dev, struct usb_host_endpoint *ep, rt_bool_t reset_toggle);
void usb_enable_interface(struct usb_device *dev, struct usb_interface *intf, rt_bool_t reset_toggles);
void usb_disable_endpoint(struct usb_device *dev, unsigned int epaddr,
        rt_bool_t reset_hardware);
void usb_disable_interface(struct usb_device *dev,
        struct usb_interface *intf, rt_bool_t reset_hardware);
void usb_release_interface_cache(struct usb_interface_cache *intfc);
int usb_get_device_descriptor(struct usb_device *dev,
        unsigned int size);
extern int usb_string(struct usb_device *dev, int index,
	char *buf, size_t size);
int usb_set_configuration(struct usb_device *dev, int configuration);
int usb_choose_configuration(struct usb_device *udev);

void usb_kick_khubd(struct usb_device *dev);
int usb_match_device(struct usb_device *dev,
                const struct usb_device_id *id);
rt_bool_t usb_device_is_owned(struct usb_device *udev);
void usb_release_dev(struct usb_device *udev);

int  usb_hub_init(void);
int usb_dma_buffer_create(struct usb_bus *bus);
void *usb_dma_buffer_alloc(rt_uint32_t size, struct usb_bus *bus);
int usb_dma_buffer_free(void *addr, struct usb_bus *bus);
// #define DEBUG1

#ifdef DEBUG1
#undef dbg
#define dbg(format, arg...)                                            \
do {                                                                   \
    rt_kprintf( format "\n", ##arg);        \
} while (0)
#else
#define dbg(format, arg...)
#endif
#endif
