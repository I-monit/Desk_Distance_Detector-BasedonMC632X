
#include "hcd.h"
#include <rtdef.h>
#include <rtthread.h>
#include "usb_errno.h"
#include"usb_storage.h"
#include "serial/usb_serial.h"
#include "cdc.h"
#include "serial/cdc-acm.h"

#ifdef RT_USING_USB_RNDIS
#include "net/cdc/rndis_host.h"
#endif
#ifdef USB_MAC_USING_ASIX
#include "net/asix.h"
#endif
#include <usb_wifi.h>

const char *usbcore_name = "usbcore";

rt_uint32_t rt_debug_usb = 0x0;
rt_int32_t g_mem_debug = 0;

#ifdef RT_USING_USB_TEST
extern  int usbtest_init(void);
#endif

extern rt_list_t rt_usb_driver_list;
extern rt_list_t usb_bus_list;

int rt_dev_set_name(rt_device_t dev, const char *fmt, ...)
{
    va_list vargs;

    va_start(vargs, fmt);
    rt_vsnprintf(dev->parent.name, sizeof(dev->parent.name), fmt, vargs);
    va_end(vargs);
    return 0;
}


/**
 * usb_find_alt_setting() - Given a configuration, find the alternate setting
 * for the given interface.
 * @config: the configuration to search (not necessarily the current config).
 * @iface_num: interface number to search in
 * @alt_num: alternate interface setting number to search for.
 *
 * Search the configuration's interface cache for the given alt setting.
 */
struct usb_host_interface *usb_find_alt_setting(
        struct usb_host_config *config,
        unsigned int iface_num,
        unsigned int alt_num)
{
    struct usb_interface_cache *intf_cache = RT_NULL;
    int i;

    for (i = 0; i < config->desc.bNumInterfaces; i++)
    {
        if (config->intf_cache[i]->altsetting[0].desc.bInterfaceNumber
                == iface_num) {
            intf_cache = config->intf_cache[i];
            break;
        }
    }
    if (!intf_cache)
        return RT_NULL;
    for (i = 0; i < intf_cache->num_altsetting; i++)
        if (intf_cache->altsetting[i].desc.bAlternateSetting == alt_num)
            return &intf_cache->altsetting[i];

    rt_kprintf("Did not find alt setting %u for intf %u, config %u\n",
            alt_num, iface_num,
            config->desc.bConfigurationValue);
    return RT_NULL;
}

/**
 * usb_ifnum_to_if - get the interface object with a given interface number
 * @dev: the device whose current configuration is considered
 * @ifnum: the desired interface
 *
 * This walks the device descriptor for the currently active configuration
 * and returns a pointer to the interface with that particular interface
 * number, or null.
 *
 * Note that configuration descriptors are not required to assign interface
 * numbers sequentially, so that it would be incorrect to assume that
 * the first interface in that descriptor corresponds to interface zero.
 * This routine helps device drivers avoid such mistakes.
 * However, you should make sure that you do the right thing with any
 * alternate settings available for this interfaces.
 *
 * Don't call this function unless you are bound to one of the interfaces
 * on this device or you have locked the device!
 */
struct usb_interface *usb_ifnum_to_if(const struct usb_device *dev,
                      unsigned int ifnum)
{
    struct usb_host_config *config = dev->actconfig;
    int i;

    if (!config)
        return RT_NULL;
    for (i = 0; i < config->desc.bNumInterfaces; i++)
        if (config->interface[i]->altsetting[0]
                .desc.bInterfaceNumber == ifnum)
            return config->interface[i];

    return RT_NULL;
}

/**
 * usb_altnum_to_altsetting - get the altsetting structure with a given alternate setting number.
 * @intf: the interface containing the altsetting in question
 * @altnum: the desired alternate setting number
 *
 * This searches the altsetting array of the specified interface for
 * an entry with the correct bAlternateSetting value and returns a pointer
 * to that entry, or null.
 *
 * Note that altsettings need not be stored sequentially by number, so
 * it would be incorrect to assume that the first altsetting entry in
 * the array corresponds to altsetting zero.  This routine helps device
 * drivers avoid such mistakes.
 *
 * Don't call this function unless you are bound to the intf interface
 * or you have locked the device!
 */
struct usb_host_interface *usb_altnum_to_altsetting(
                    const struct usb_interface *intf,
                    unsigned int altnum)
{
    int i;

    for (i = 0; i < intf->num_altsetting; i++)
    {
        if (intf->altsetting[i].desc.bAlternateSetting == altnum)
            return &intf->altsetting[i];
    }
    return RT_NULL;
}

/**
 * usb_release_dev - free a usb device structure when all users of it are finished.
 * @dev: device that's been disconnected
 *
 * Will be called only by the device core when all users of this usb device are
 * done.
 */
#if 0
static void usb_release_dev(struct usb_device *udev)
{
    usb_destroy_configuration(udev);
    rt_free(udev);
}

#endif

/* Returns 1 if @usb_bus is WUSB, 0 otherwise */
static unsigned int usb_bus_is_wusb(struct usb_bus *bus)
{
    struct usb_hcd *hcd = (struct usb_hcd *)bus;

    return hcd->wireless;
}


/**
 * usb_alloc_dev - usb device constructor (usbcore-internal)
 * @parent: hub to which device is connected; null to allocate a root hub
 * @bus: bus used to access the device
 * @port1: one-based index of port; ignored for root hubs
 * Context: !in_interrupt()
 *
 * Only hub drivers (including virtual root hub drivers for host
 * controllers) should ever call this.
 *
 * This call may not be used in a non-sleeping context.
 */

struct usb_device *usb_alloc_dev(struct usb_device *parent,
                 struct usb_bus *bus, unsigned int port1)
{
    struct usb_device *dev;
    struct usb_hcd *usb_hcd = (struct usb_hcd *)bus;
/* unsigned root_hub = 0; */

    dev = rt_malloc(sizeof(*dev));

    RT_DEBUG_LOG(rt_debug_usb, ("rt_malloc usb dev:%d size:%d,addr:%x\n", g_mem_debug++, sizeof(*dev), dev));

    if (!dev)
        return RT_NULL;
    rt_memset(dev, 0, sizeof(*dev));

    /* Root hubs aren't true devices, so don't allocate HCD resources */
    if (usb_hcd->driver->alloc_dev && parent &&
        !usb_hcd->driver->alloc_dev(usb_hcd, dev)) {
        RT_DEBUG_LOG(rt_debug_usb, ("rt_free usb dev :%d,addr:%x\n", --g_mem_debug, dev));
        rt_free(dev);
        return RT_NULL;
    }
    dev->state = USB_STATE_ATTACHED;
    dev->urbnum = 0;

    rt_list_init(&dev->ep0.urb_list);
    dev->ep0.desc.bLength = USB_DT_ENDPOINT_SIZE;
    dev->ep0.desc.bDescriptorType = USB_DT_ENDPOINT;
    /* ep0 maxpacket comes later, from device descriptor */
    usb_enable_endpoint(dev, &dev->ep0, RT_FALSE);
/* dev->can_submit = 1; */

    /* Save readable and stable topology id, distinguishing devices
     * by location for diagnostics, tools, driver model, etc.  The
     * string is a path along hub ports, from the root.  Each device's
     * dev->devpath will be stable until USB is re-cabled, and hubs
     * are often labeled with these port numbers.  The name isn't
     * as stable:  bus->busnum changes easily from modprobe order,
     * cardbus or pci hotplugging, and so on.
     */
    if (!parent)
    {
        dev->devpath[0] = '0';
        dev->route = 0;
/* rt_dev_set_name(&dev->dev, "usb%d", bus->busnum); */
/* root_hub = 1; */
    } else
    {
        /* match any labeling on the hubs; it's one-based */
        if (parent->devpath[0] == '0')
        {
            rt_snprintf(dev->devpath, sizeof(dev->devpath),
                "%d", port1);
            /* Root ports are not counted in route string */
            dev->route = 0;
        } else
        {
            rt_snprintf(dev->devpath, sizeof(dev->devpath),
                "%s.%d", parent->devpath, port1);
            /* Route string assumes hubs have less than 16 ports */
            if (port1 < 15)
                dev->route = parent->route +
                    (port1 << ((parent->level - 1)*4));
            else
                dev->route = parent->route +
                    (15 << ((parent->level - 1)*4));
        }

        /* dev->dev.parent = &parent->dev; */
/* rt_dev_set_name(&dev->dev, "%d-%s", bus->busnum, dev->devpath); */

        /* hub driver sets up TT records */
    }

    dev->portnum = port1;
    dev->bus = bus;
    dev->parent = parent;


    dev->wusb = usb_bus_is_wusb(bus) ? 1 : 0;

    return dev;
}

void usb_release_dev(struct usb_device *udev)
{
/* struct usb_hcd *hcd; */

/* hcd = bus_to_hcd(udev->bus); */

    usb_destroy_configuration(udev);
/* usb_put_hcd(hcd); */
/* kfree(udev->product); */
/* kfree(udev->manufacturer); */
/* kfree(udev->serial); */
    rt_free(udev);
    RT_DEBUG_LOG(rt_debug_usb, ("rt_free usb udev :%d\n,addr:%x", --g_mem_debug, udev));
}
/**
 * usb_get_current_frame_number - return current bus frame number
 * @dev: the device whose bus is being queried
 *
 * Returns the current frame number for the USB host controller
 * used with the given USB device.  This can be used when scheduling
 * isochronous requests.
 *
 * Note that different kinds of host controller have different
 * "scheduling horizons".  While one type might support scheduling only
 * 32 frames into the future, others could support scheduling up to
 * 1024 frames into the future.
 */
int usb_get_current_frame_number(struct usb_device *dev)
{
    return usb_hcd_get_frame_number(dev);
}

/*-------------------------------------------------------------------*/
/*
 * __usb_get_extra_descriptor() finds a descriptor of specific type in the
 * extra field of the interface and endpoint descriptor structs.
 */

int __usb_get_extra_descriptor(char *buffer, unsigned int size,
                   unsigned char type, void **ptr)
{
    struct usb_descriptor_header *header;

    while (size >= sizeof(struct usb_descriptor_header))
    {
        header = (struct usb_descriptor_header *)buffer;

        if (header->bLength < 2)
        {
            rt_kprintf("%s: bogus descriptor, type %d length %d\n",
                usbcore_name,
                header->bDescriptorType,
                header->bLength);
            return -1;
        }

        if (header->bDescriptorType == type)
        {
            *ptr = header;
            return 0;
        }

        buffer += header->bLength;
        size -= header->bLength;
    }
    return -1;
}



/*
 * Init
 */
extern void rt_hw_usbotg_init(void);


int  usb_init(void)
{
    int retval;


    rt_list_init(&rt_usb_driver_list);

    rt_list_init(&usb_bus_list);

    retval = usb_hub_init();
    if (retval)
        {

        RT_DEBUG_LOG(rt_debug_usb, ("usb_hub_init error %d\n", retval));
        return retval;
    }

#ifdef RT_USING_USB_STORAGE
    usb_stor_init();
#endif
#ifdef RT_USING_USB_SERIAL
    usb_serial_init();
#endif

#ifdef RT_USING_USB_CDC_ACM
    usb_cdc_acm_init();
#endif

#ifdef RT_USING_USB_RNDIS
    usb_rndis_init();
#endif

#ifdef RT_USING_USB_ECM
    extern int usb_ecm_init(void);
    usb_ecm_init();
#endif

#ifdef USB_MAC_USING_ASIX
    asix_usb_mac_init();
#endif

#ifdef RT_USING_USB_TEST
    usbtest_init();
#endif

#ifdef WIFI_USING_USBWIFI
    extern void* p_usb_handle;
    extern void power_config(void);
    power_config();
    p_usb_handle = usbwifi_sta_init(0/*not used yet...*/);
#endif

#ifdef RT_USING_FH_USBOTG
    /*HCD OTG init*/
    rt_hw_usbotg_init();
#endif

    return RT_EOK;
}

void sw(rt_uint32_t n)
{
    rt_debug_usb = n;
}

#ifdef RT_USING_FINSH
#include <finsh.h>
FINSH_FUNCTION_EXPORT(sw, usb_core_print_switch..);
#endif


