/*
 * drivers/usb/driver.c - most of the driver model stuff for usb
 *
 * (C) Copyright 2005 Greg Kroah-Hartman <gregkh@suse.de>
 *
 * based on drivers/usb/usb.c which had the following copyrights:
 *    (C) Copyright Linus Torvalds 1999
 *    (C) Copyright Johannes Erdfelt 1999-2001
 *    (C) Copyright Andreas Gal 1999
 *    (C) Copyright Gregory P. Smith 1999
 *    (C) Copyright Deti Fliegl 1999 (new USB architecture)
 *    (C) Copyright Randy Dunlap 2000
 *    (C) Copyright David Brownell 2000-2004
 *    (C) Copyright Yggdrasil Computing, Inc. 2000
 *        (usb_device_id matching changes by Adam J. Richter)
 *    (C) Copyright Greg Kroah-Hartman 2002-2003
 *
 * NOTE! This is not actually a driver at all, rather this is
 * just a collection of helper routines that implement the
 * matching, probing, releasing, suspending and resuming for
 * real drivers.
 *
 */

#include <quirks.h>
/* #include <hcd.h> */
#include <rtdef.h>
#include "usb.h"
#include "usb_errno.h"

rt_list_t rt_usb_driver_list;


#if 0
static const struct usb_device_id *usb_match_dynamic_id(struct usb_interface *intf,
                            struct usb_driver *drv)
{
    struct usb_dynid *dynid;

    list_for_each_entry(dynid, &drv->dynids.list, node) {
        if (usb_match_one_id(intf, &dynid->id))
        {
            return &dynid->id;
        }
    }
    return RT_NULL;
}
#endif


/* returns 0 if no match, 1 if match */
int usb_match_device(struct usb_device *dev, const struct usb_device_id *id)
{
    if ((id->match_flags & USB_DEVICE_ID_MATCH_VENDOR) &&
        id->idVendor != (dev->descriptor.idVendor))
        return 0;

    if ((id->match_flags & USB_DEVICE_ID_MATCH_PRODUCT) &&
        id->idProduct != (dev->descriptor.idProduct))
        return 0;

    /* No need to test id->bcdDevice_lo != 0, since 0 is never
       greater than any unsigned number. */
    if ((id->match_flags & USB_DEVICE_ID_MATCH_DEV_LO) &&
        (id->bcdDevice_lo > (dev->descriptor.bcdDevice)))
        return 0;

    if ((id->match_flags & USB_DEVICE_ID_MATCH_DEV_HI) &&
        (id->bcdDevice_hi < (dev->descriptor.bcdDevice)))
        return 0;

    if ((id->match_flags & USB_DEVICE_ID_MATCH_DEV_CLASS) &&
        (id->bDeviceClass != dev->descriptor.bDeviceClass))
        return 0;

    if ((id->match_flags & USB_DEVICE_ID_MATCH_DEV_SUBCLASS) &&
        (id->bDeviceSubClass != dev->descriptor.bDeviceSubClass))
        return 0;

    if ((id->match_flags & USB_DEVICE_ID_MATCH_DEV_PROTOCOL) &&
        (id->bDeviceProtocol != dev->descriptor.bDeviceProtocol))
        return 0;

    return 1;
}

/* returns 0 if no match, 1 if match */
int usb_match_one_id(struct usb_interface *interface,
             const struct usb_device_id *id)
{
    struct usb_host_interface *intf;
    struct usb_device *dev;

    /* proc_connectinfo in devio.c may call us with id == NULL. */
    if (id == RT_NULL)
        return 0;

    intf = interface->cur_altsetting;
    dev = interface->usb_dev;

    if (!usb_match_device(dev, id))
        return 0;

    /* The interface class, subclass, and protocol should never be
     * checked for a match if the device class is Vendor Specific,
     * unless the match record specifies the Vendor ID. */
    if (dev->descriptor.bDeviceClass == USB_CLASS_VENDOR_SPEC &&
            !(id->match_flags & USB_DEVICE_ID_MATCH_VENDOR) &&
            (id->match_flags & (USB_DEVICE_ID_MATCH_INT_CLASS |
                USB_DEVICE_ID_MATCH_INT_SUBCLASS |
                USB_DEVICE_ID_MATCH_INT_PROTOCOL)))
        return 0;

    if ((id->match_flags & USB_DEVICE_ID_MATCH_INT_CLASS) &&
        (id->bInterfaceClass != intf->desc.bInterfaceClass))
        return 0;

    if ((id->match_flags & USB_DEVICE_ID_MATCH_INT_SUBCLASS) &&
        (id->bInterfaceSubClass != intf->desc.bInterfaceSubClass))
        return 0;

    if ((id->match_flags & USB_DEVICE_ID_MATCH_INT_PROTOCOL) &&
        (id->bInterfaceProtocol != intf->desc.bInterfaceProtocol))
        return 0;

    return 1;
}

/**
 * usb_match_id - find first usb_device_id matching device or interface
 * @interface: the interface of interest
 * @id: array of usb_device_id structures, terminated by zero entry
 *
 * usb_match_id searches an array of usb_device_id's and returns
 * the first one matching the device or interface, or null.
 * This is used when binding (or rebinding) a driver to an interface.
 * Most USB device drivers will use this indirectly, through the usb core,
 * but some layered driver frameworks use it directly.
 * These device tables are exported with MODULE_DEVICE_TABLE, through
 * modutils, to support the driver loading functionality of USB hotplugging.
 *
 * What Matches:
 *
 * The "match_flags" element in a usb_device_id controls which
 * members are used.  If the corresponding bit is set, the
 * value in the device_id must match its corresponding member
 * in the device or interface descriptor, or else the device_id
 * does not match.
 *
 * "driver_info" is normally used only by device drivers,
 * but you can create a wildcard "matches anything" usb_device_id
 * as a driver's "modules.usbmap" entry if you provide an id with
 * only a nonzero "driver_info" field.  If you do this, the USB device
 * driver's probe() routine should use additional intelligence to
 * decide whether to bind to the specified interface.
 *
 * What Makes Good usb_device_id Tables:
 *
 * The match algorithm is very simple, so that intelligence in
 * driver selection must come from smart driver id records.
 * Unless you have good reasons to use another selection policy,
 * provide match elements only in related groups, and order match
 * specifiers from specific to general.  Use the macros provided
 * for that purpose if you can.
 *
 * The most specific match specifiers use device descriptor
 * data.  These are commonly used with product-specific matches;
 * the USB_DEVICE macro lets you provide vendor and product IDs,
 * and you can also match against ranges of product revisions.
 * These are widely used for devices with application or vendor
 * specific bDeviceClass values.
 *
 * Matches based on device class/subclass/protocol specifications
 * are slightly more general; use the USB_DEVICE_INFO macro, or
 * its siblings.  These are used with single-function devices
 * where bDeviceClass doesn't specify that each interface has
 * its own class.
 *
 * Matches based on interface class/subclass/protocol are the
 * most general; they let drivers bind to any interface on a
 * multiple-function device.  Use the USB_INTERFACE_INFO
 * macro, or its siblings, to match class-per-interface style
 * devices (as recorded in bInterfaceClass).
 *
 * Note that an entry created by USB_INTERFACE_INFO won't match
 * any interface if the device class is set to Vendor-Specific.
 * This is deliberate; according to the USB spec the meanings of
 * the interface class/subclass/protocol for these devices are also
 * vendor-specific, and hence matching against a standard product
 * class wouldn't work anyway.  If you really want to use an
 * interface-based match for such a device, create a match record
 * that also specifies the vendor ID.  (Unforunately there isn't a
 * standard macro for creating records like this.)
 *
 * Within those groups, remember that not all combinations are
 * meaningful.  For example, don't give a product version range
 * without vendor and product IDs; or specify a protocol without
 * its associated class and subclass.
 */
const struct usb_device_id *usb_match_id(struct usb_interface *interface,
                     const struct usb_device_id *id)
{
    /* proc_connectinfo in devio.c may call us with id == NULL. */
    if (id == RT_NULL)
        return RT_NULL;

    /* It is important to check that id->driver_info is nonzero,
       since an entry that is all zeroes except for a nonzero
       id->driver_info is the way to create an entry that
       indicates that the driver want to examine every
       device and interface. */
    for (; id->idVendor || id->idProduct || id->bDeviceClass ||
           id->bInterfaceClass || id->driver_info; id++) {
        if (usb_match_one_id(interface, id))
            return id;
    }

    return RT_NULL;
}



int usb_device_add(struct usb_interface *dev)
{

    struct rt_list_node *node = RT_NULL;
    struct usb_driver *usb_drv = RT_NULL;
    const struct usb_device_id *id = RT_NULL;
    rt_uint32_t ret = 0;

    /* enter critical */
    /* if (rt_thread_self() != RT_NULL) */
    /* rt_enter_critical(); */

    for (node = rt_usb_driver_list.next; node != &rt_usb_driver_list; node = node->next)
    {
        usb_drv = (struct usb_driver *)rt_list_entry(node, struct usb_driver, list);
        id = usb_match_id(dev, usb_drv->id_table);
        if (id)
        {
            dev->driver = usb_drv;
            ret = usb_drv->probe(dev, id);
            break;
        }

/* id = usb_match_dynamic_id(dev, usb_drv); */
/* if (id) */
/* return 1; */
    }


      /* leave critical */
    /* if (rt_thread_self() != RT_NULL) */
    /* rt_exit_critical(); */

    return ret;
}


/**
 * usb_register_driver - register a USB interface driver
 * @new_driver: USB operations for the interface driver
 * @owner: module owner of this driver.
 * @mod_name: module name string
 *
 * Registers a USB interface driver with the USB core.  The list of
 * unattached interfaces will be rescanned whenever a new driver is
 * added, allowing the new driver to attach to any recognized interfaces.
 * Returns a negative error code on failure and 0 on success.
 *
 * NOTE: if you want your driver to use the USB major number, you must call
 * usb_register_dev() to enable that functionality.  This function no longer
 * takes care of that.
 */
int usb_register_driver(struct usb_driver *new_driver)
{
    if (new_driver == RT_NULL)
        return -RT_ERROR;

    /* insert class driver into driver list */
    rt_list_insert_after(&rt_usb_driver_list, &(new_driver->list));

    return RT_EOK;


}


/**
 * usb_deregister - unregister a USB interface driver
 * @driver: USB operations of the interface driver to unregister
 * Context: must be able to sleep
 *
 * Unlinks the specified driver from the internal USB driver list.
 *
 * NOTE: If you called usb_register_dev(), you still need to call
 * usb_deregister_dev() to clean up your driver's allocated minor numbers,
 * this * call will no longer do it for you.
 */
void usb_deregister(struct usb_driver *driver)
{

    RT_ASSERT(driver != RT_NULL);

    /* remove class driver from driver list */
    rt_list_remove(&(driver->list));

}

