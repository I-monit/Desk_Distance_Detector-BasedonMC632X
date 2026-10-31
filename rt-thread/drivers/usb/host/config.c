
#include <ch9.h>
/* #include <hcd.h> */
#include <quirks.h>
#include <rtdef.h>
#include <rtthread.h>
#include "usb_errno.h"
#include "usb.h"


#define USB_MAXALTSETTING        128    /* Hard limit */
#define USB_MAXENDPOINTS        30    /* Hard limit */

#define USB_MAXCONFIG            8    /* Arbitrary limit */


static inline const char *plural(int n)
{
    return (n == 1 ? "" : "s");
}

static inline int fls(int x)
{
    int r = 32;

    if (!x)
        return 0;
    if (!(x & 0xffff0000u))
    {
        x <<= 16;
        r -= 16;
    }
    if (!(x & 0xff000000u))
    {
        x <<= 8;
        r -= 8;
    }
    if (!(x & 0xf0000000u))
    {
        x <<= 4;
        r -= 4;
    }
    if (!(x & 0xc0000000u))
    {
        x <<= 2;
        r -= 2;
    }
    if (!(x & 0x80000000u))
    {
        x <<= 1;
        r -= 1;
    }
    return r;
}


static int find_next_descriptor(unsigned char *buffer, int size,
    int dt1, int dt2, int *num_skipped)
{
    struct usb_descriptor_header *h;
    int n = 0;
    unsigned char *buffer0 = buffer;

    /* Find the next descriptor of type dt1 or dt2 */
    while (size > 0)
    {
        h = (struct usb_descriptor_header *) buffer;
        if (h->bDescriptorType == dt1 || h->bDescriptorType == dt2)
            break;
        buffer += h->bLength;
        size -= h->bLength;
        ++n;
    }

    /* Store the number of descriptors skipped and return the
     * number of bytes skipped */
    if (num_skipped)
        *num_skipped = n;
    return buffer - buffer0;
}

static void usb_parse_ss_endpoint_companion(struct usb_device *dev, int cfgno,
        int inum, int asnum, struct usb_host_endpoint *ep,
        unsigned char *buffer, int size)
{
    struct usb_ss_ep_comp_descriptor *desc;
    int max_tx;

    /* The SuperSpeed endpoint companion descriptor is supposed to
     * be the first thing immediately following the endpoint descriptor.
     */
    desc = (struct usb_ss_ep_comp_descriptor *) buffer;
    if (desc->bDescriptorType != USB_DT_SS_ENDPOINT_COMP ||
            size < USB_DT_SS_EP_COMP_SIZE) {
            RT_DEBUG_LOG(rt_debug_usb,("No SuperSpeed endpoint companion for config %d  interface %d altsetting %d ep %d: "
                "using minimum values\n",
                cfgno, inum, asnum, ep->desc.bEndpointAddress));

        /* Fill in some default values.
         * Leave bmAttributes as zero, which will mean no streams for
         * bulk, and isoc won't support multiple bursts of packets.
         * With bursts of only one packet, and a Mult of 1, the max
         * amount of data moved per endpoint service interval is one
         * packet.
         */
        ep->ss_ep_comp.bLength = USB_DT_SS_EP_COMP_SIZE;
        ep->ss_ep_comp.bDescriptorType = USB_DT_SS_ENDPOINT_COMP;
        if (usb_endpoint_xfer_isoc(&ep->desc) ||
                usb_endpoint_xfer_int(&ep->desc))
            ep->ss_ep_comp.wBytesPerInterval =
                    ep->desc.wMaxPacketSize;
        return;
    }

    rt_memcpy(&ep->ss_ep_comp, desc, USB_DT_SS_EP_COMP_SIZE);

    /* Check the various values */
    if (usb_endpoint_xfer_control(&ep->desc) && desc->bMaxBurst != 0)
    {
            RT_DEBUG_LOG(rt_debug_usb,("Control endpoint with bMaxBurst = %d in config %d interface %d altsetting %d ep %d: "
                "setting to zero\n", desc->bMaxBurst,
                cfgno, inum, asnum, ep->desc.bEndpointAddress));
        ep->ss_ep_comp.bMaxBurst = 0;
    } else if (desc->bMaxBurst > 15)
    {
            RT_DEBUG_LOG(rt_debug_usb,( "Endpoint with bMaxBurst = %d in config %d interface %d altsetting %d ep %d: "
                "setting to 15\n", desc->bMaxBurst,
                cfgno, inum, asnum, ep->desc.bEndpointAddress));
        ep->ss_ep_comp.bMaxBurst = 15;
    }

    if ((usb_endpoint_xfer_control(&ep->desc) ||
            usb_endpoint_xfer_int(&ep->desc)) &&
                desc->bmAttributes != 0) {
            RT_DEBUG_LOG(rt_debug_usb,( "%s endpoint with bmAttributes = %d in config %d interface %d altsetting %d ep %d: "
                "setting to zero\n",
                usb_endpoint_xfer_control(&ep->desc) ? "Control" : "Bulk",
                desc->bmAttributes,
                cfgno, inum, asnum, ep->desc.bEndpointAddress));
        ep->ss_ep_comp.bmAttributes = 0;
    } else if (usb_endpoint_xfer_bulk(&ep->desc) &&
            desc->bmAttributes > 16) {
            RT_DEBUG_LOG(rt_debug_usb,( "Bulk endpoint with more than 65536 streams in "
            "config %d interface %d altsetting %d ep %d: "
                "setting to max\n",
                cfgno, inum, asnum, ep->desc.bEndpointAddress));
        ep->ss_ep_comp.bmAttributes = 16;
    } else if (usb_endpoint_xfer_isoc(&ep->desc) &&
            desc->bmAttributes > 2) {
            RT_DEBUG_LOG(rt_debug_usb,( "Isoc endpoint has Mult of %d in "
                "config %d interface %d altsetting %d ep %d: "
                "setting to 3\n", desc->bmAttributes + 1,
                cfgno, inum, asnum, ep->desc.bEndpointAddress));
        ep->ss_ep_comp.bmAttributes = 2;
    }

    if (usb_endpoint_xfer_isoc(&ep->desc))
        max_tx = (desc->bMaxBurst + 1) * (desc->bmAttributes + 1) *
            (ep->desc.wMaxPacketSize);
    else if (usb_endpoint_xfer_int(&ep->desc))
        max_tx = (ep->desc.wMaxPacketSize) *
            (desc->bMaxBurst + 1);
    else
        max_tx = 999999;
    if ((desc->wBytesPerInterval) > max_tx)
    {
            RT_DEBUG_LOG(rt_debug_usb,( "%s endpoint with wBytesPerInterval of %d in config %d interface %d altsetting %d ep %d: "
                "setting to %d\n",
                usb_endpoint_xfer_isoc(&ep->desc) ? "Isoc" : "Int",
                (desc->wBytesPerInterval),
                cfgno, inum, asnum, ep->desc.bEndpointAddress,
                max_tx));
        ep->ss_ep_comp.wBytesPerInterval = (max_tx);
    }
}

static int usb_parse_endpoint(struct usb_device *dev, int cfgno, int inum,
    int asnum, struct usb_host_interface *ifp, int num_ep,
    unsigned char *buffer, int size)
{
    unsigned char *buffer0 = buffer;
    struct usb_endpoint_descriptor *d;
    struct usb_host_endpoint *endpoint;
    int n, i, j, retval;

    d = (struct usb_endpoint_descriptor *) buffer;
    buffer += d->bLength;
    size -= d->bLength;

    if (d->bLength >= USB_DT_ENDPOINT_AUDIO_SIZE)
        n = USB_DT_ENDPOINT_AUDIO_SIZE;
    else if (d->bLength >= USB_DT_ENDPOINT_SIZE)
        n = USB_DT_ENDPOINT_SIZE;
    else
    {
        RT_DEBUG_LOG(rt_debug_usb, ("config %d interface %d altsetting %d has an invalid endpoint descriptor of length %d, skipping\n",
            cfgno, inum, asnum, d->bLength));
        goto skip_to_next_endpoint_or_interface_descriptor;
    }

    i = d->bEndpointAddress & ~USB_ENDPOINT_DIR_MASK;
    if (i >= 16 || i == 0)
    {
        RT_DEBUG_LOG(rt_debug_usb, ("config %d interface %d altsetting %d has an invalid endpoint with address 0x%X, skipping\n",
            cfgno, inum, asnum, d->bEndpointAddress));
        goto skip_to_next_endpoint_or_interface_descriptor;
    }

    /* Only store as many endpoints as we have room for */
    if (ifp->desc.bNumEndpoints >= num_ep)
        goto skip_to_next_endpoint_or_interface_descriptor;

    endpoint = &ifp->endpoint[ifp->desc.bNumEndpoints];
    ++ifp->desc.bNumEndpoints;

    rt_memcpy(&endpoint->desc, d, n);
    rt_list_init(&endpoint->urb_list);

    /* Fix up bInterval values outside the legal range. Use 32 ms if no
     * proper value can be guessed. */
    i = 0;        /* i = min, j = max, n = default */
    j = 255;
    if (usb_endpoint_xfer_int(d))
    {
        i = 1;
        switch (dev->speed)
        {
        case USB_SPEED_SUPER:
        case USB_SPEED_HIGH:
            /* Many device manufacturers are using full-speed
             * bInterval values in high-speed interrupt endpoint
             * descriptors. Try to fix those and fall back to a
             * 32 ms default value otherwise. */
            n = fls(d->bInterval*8);
            if (n == 0)
                n = 9;    /* 32 ms = 2^(9-1) uframes */
            j = 16;
            break;
        default:        /* USB_SPEED_FULL or _LOW */
            /* For low-speed, 10 ms is the official minimum.
             * But some "overclocked" devices might want faster
             * polling so we'll allow it. */
            n = 32;
            break;
        }
    } else if (usb_endpoint_xfer_isoc(d))
    {
        i = 1;
        j = 16;
        switch (dev->speed)
        {
        case USB_SPEED_HIGH:
            n = 9;        /* 32 ms = 2^(9-1) uframes */
            break;
        default:        /* USB_SPEED_FULL */
            n = 6;        /* 32 ms = 2^(6-1) frames */
            break;
        }
    }
    if (d->bInterval < i || d->bInterval > j)
    {
        RT_DEBUG_LOG(rt_debug_usb,("config %d interface %d altsetting %d endpoint 0x%X has an invalid bInterval %d, "
            "changing to %d\n",
            cfgno, inum, asnum,
            d->bEndpointAddress, d->bInterval, n));
        endpoint->desc.bInterval = n;
    }

    /* Some buggy low-speed devices have Bulk endpoints, which is
     * explicitly forbidden by the USB spec.  In an attempt to make
     * them usable, we will try treating them as Interrupt endpoints.
     */
    if (dev->speed == USB_SPEED_LOW &&
            usb_endpoint_xfer_bulk(d)) {
        RT_DEBUG_LOG(rt_debug_usb, ("config %d interface %d altsetting %d endpoint 0x%X is Bulk; changing to Interrupt\n",
            cfgno, inum, asnum, d->bEndpointAddress));
        endpoint->desc.bmAttributes = USB_ENDPOINT_XFER_INT;
        endpoint->desc.bInterval = 1;
        if ((endpoint->desc.wMaxPacketSize) > 8)
            endpoint->desc.wMaxPacketSize = (8);
    }

    /*
     * Some buggy high speed devices have bulk endpoints using
     * maxpacket sizes other than 512.  High speed HCDs may not
     * be able to handle that particular bug, so let's warn...
     */
    if (dev->speed == USB_SPEED_HIGH
            && usb_endpoint_xfer_bulk(d)) {
        unsigned int maxp;

        maxp = (endpoint->desc.wMaxPacketSize) & 0x07ff;
        if (maxp != 512)
            RT_DEBUG_LOG(rt_debug_usb, ("config %d interface %d altsetting %d bulk endpoint 0x%X has invalid maxpacket %d\n",
                cfgno, inum, asnum, d->bEndpointAddress,
                maxp));
    }

    /* Parse a possible SuperSpeed endpoint companion descriptor */
    if (dev->speed == USB_SPEED_SUPER)
        usb_parse_ss_endpoint_companion(dev, cfgno,
                inum, asnum, endpoint, buffer, size);

    /* Skip over any Class Specific or Vendor Specific descriptors;
     * find the next endpoint or interface descriptor */
    endpoint->extra = buffer;
    i = find_next_descriptor(buffer, size, USB_DT_ENDPOINT,
            USB_DT_INTERFACE, &n);
    endpoint->extralen = i;
    retval = buffer - buffer0 + i;
    if (n > 0)
        RT_DEBUG_LOG(rt_debug_usb, ("skipped %d descriptor%s after %s\n",
            n, plural(n), "endpoint"));
    return retval;

skip_to_next_endpoint_or_interface_descriptor:
    i = find_next_descriptor(buffer, size, USB_DT_ENDPOINT,
        USB_DT_INTERFACE, RT_NULL);
    return buffer - buffer0 + i;
}


void usb_release_interface_cache(struct usb_interface_cache *intfc)
{
    /* struct usb_interface_cache *intfc = ref_to_usb_interface_cache(ref); */
    int j;

    for (j = 0; j < intfc->num_altsetting; j++)
    {
        struct usb_host_interface *alt = &intfc->altsetting[j];

        RT_DEBUG_LOG(rt_debug_usb, ("--g_mem_debug usb endpoint:%d,addr:%x\n", --g_mem_debug, alt->endpoint));
        rt_free(alt->endpoint);
/* RT_DEBUG_LOG(rt_debug_usb,("rt_free usb string:%d\n",--g_mem_debug)); */
/* rt_free(alt->string); */
    }
    RT_DEBUG_LOG(rt_debug_usb, ("rt_free usb intfc:%d,addr:%x\n", --g_mem_debug, intfc));
    rt_free(intfc);
}



static int usb_parse_interface(struct usb_device *dev, int cfgno,
    struct usb_host_config *config, unsigned char *buffer, int size,
    rt_uint8_t inums[], rt_uint8_t nalts[])
{
    unsigned char *buffer0 = buffer;
    struct usb_interface_descriptor    *d;
    int inum, asnum;
    struct usb_interface_cache *intfc;
    struct usb_host_interface *alt;
    int i, n;
    int len, retval;
    int num_ep, num_ep_orig;

    d = (struct usb_interface_descriptor *) buffer;
    buffer += d->bLength;
    size -= d->bLength;

    if (d->bLength < USB_DT_INTERFACE_SIZE)
        goto skip_to_next_interface_descriptor;

    /* Which interface entry is this? */
    intfc = RT_NULL;
    inum = d->bInterfaceNumber;
    for (i = 0; i < config->desc.bNumInterfaces; ++i)
    {
        if (inums[i] == inum)
        {
            intfc = config->intf_cache[i];
            break;
        }
    }
    if (!intfc || intfc->num_altsetting >= nalts[i])
        goto skip_to_next_interface_descriptor;

    /* Check for duplicate altsetting entries */
    asnum = d->bAlternateSetting;
    for ((i = 0, alt = &intfc->altsetting[0]);
          i < intfc->num_altsetting;
         (++i, ++alt)) {
        if (alt->desc.bAlternateSetting == asnum)
        {
                RT_DEBUG_LOG(rt_debug_usb, ("Duplicate descriptor for config %d interface %d altsetting %d, skipping\n",
                cfgno, inum, asnum));
            goto skip_to_next_interface_descriptor;
        }
    }

    ++intfc->num_altsetting;
    rt_memcpy(&alt->desc, d, USB_DT_INTERFACE_SIZE);

    /* Skip over any Class Specific or Vendor Specific descriptors;
     * find the first endpoint or interface descriptor */
    alt->extra = buffer;
    i = find_next_descriptor(buffer, size, USB_DT_ENDPOINT,
        USB_DT_INTERFACE, &n);
    alt->extralen = i;
    if (n > 0)
            RT_DEBUG_LOG(rt_debug_usb, ("skipped %d descriptor%s after %s\n",
            n, plural(n), "interface"));
    buffer += i;
    size -= i;

    /* Allocate space for the right(?) number of endpoints */
    num_ep = num_ep_orig = alt->desc.bNumEndpoints;
    alt->desc.bNumEndpoints = 0;        /* Use as a counter */
    if (num_ep > USB_MAXENDPOINTS)
    {
            RT_DEBUG_LOG(rt_debug_usb, ("too many endpoints for config %d interface %d altsetting %d: %d, using maximum allowed: %d\n",
            cfgno, inum, asnum, num_ep, USB_MAXENDPOINTS));
        num_ep = USB_MAXENDPOINTS;
    }

    if (num_ep > 0)
    {
        /* Can't allocate 0 bytes */
        len = sizeof(struct usb_host_endpoint) * num_ep;
        alt->endpoint = rt_malloc(len);
        RT_DEBUG_LOG(rt_debug_usb, ("rt_malloc usb endpoint:%d,size:%d,addr:%x\n", g_mem_debug++, len, alt->endpoint));
        if (!alt->endpoint)
            return -ENOMEM;
        rt_memset(alt->endpoint, 0, len);
    }

    /* Parse all the endpoint descriptors */
    n = 0;
    while (size > 0)
    {
        if (((struct usb_descriptor_header *) buffer)->bDescriptorType
             == USB_DT_INTERFACE)
            break;
        retval = usb_parse_endpoint(dev, cfgno, inum, asnum, alt,
            num_ep, buffer, size);
        if (retval < 0)
            return retval;
        ++n;

        buffer += retval;
        size -= retval;
    }

    if (n != num_ep_orig)
        RT_DEBUG_LOG(rt_debug_usb,("config %d interface %d altsetting %d has %d endpoint descriptor%s, different from the interface "
            "descriptor's value: %d\n",
            cfgno, inum, asnum, n, plural(n), num_ep_orig));
    return buffer - buffer0;

skip_to_next_interface_descriptor:
    i = find_next_descriptor(buffer, size, USB_DT_INTERFACE,
        USB_DT_INTERFACE, RT_NULL);
    return buffer - buffer0 + i;
}

static int usb_parse_configuration(struct usb_device *dev, int cfgidx,
    struct usb_host_config *config, unsigned char *buffer, int size)
{
    unsigned char *buffer0 = buffer;
    int cfgno;
    int nintf, nintf_orig;
    int i, j, n;
    struct usb_interface_cache *intfc;
    unsigned char *buffer2;
    int size2;
    struct usb_descriptor_header *header;
    int len, retval;
    rt_uint8_t inums[USB_MAXINTERFACES], nalts[USB_MAXINTERFACES];
    unsigned int iad_num = 0;

    rt_memcpy(&config->desc, buffer, USB_DT_CONFIG_SIZE);
    if (config->desc.bDescriptorType != USB_DT_CONFIG ||
        config->desc.bLength < USB_DT_CONFIG_SIZE) {
            RT_DEBUG_LOG(rt_debug_usb, ("invalid descriptor for config index %d: type = 0x%X, length = %d\n",
            cfgidx,
            config->desc.bDescriptorType, config->desc.bLength));
        return -EINVAL;
    }
    cfgno = config->desc.bConfigurationValue;

    buffer += config->desc.bLength;
    size -= config->desc.bLength;

    nintf = nintf_orig = config->desc.bNumInterfaces;
    if (nintf > USB_MAXINTERFACES)
    {
            RT_DEBUG_LOG(rt_debug_usb, ("config %d has too many interfaces: %d, using maximum allowed: %d\n",
            cfgno, nintf, USB_MAXINTERFACES));
        nintf = USB_MAXINTERFACES;
    }

    /* Go through the descriptors, checking their length and counting the
     * number of altsettings for each interface */
    n = 0;
    for ((buffer2 = buffer, size2 = size);
          size2 > 0;
         (buffer2 += header->bLength, size2 -= header->bLength)) {

        if (size2 < sizeof(struct usb_descriptor_header))
        {
                RT_DEBUG_LOG(rt_debug_usb, ("config %d descriptor has %d excess byte%s, ignoring\n",
                cfgno, size2, plural(size2)));
            break;
        }

        header = (struct usb_descriptor_header *) buffer2;
        if ((header->bLength > size2) || (header->bLength < 2))
        {
                RT_DEBUG_LOG(rt_debug_usb, ("config %d has an invalid descriptor of length %d, skipping remainder of the config\n",
                cfgno, header->bLength));
            break;
        }

        if (header->bDescriptorType == USB_DT_INTERFACE)
        {
            struct usb_interface_descriptor *d;
            int inum;

            d = (struct usb_interface_descriptor *) header;
            if (d->bLength < USB_DT_INTERFACE_SIZE)
            {
                    RT_DEBUG_LOG(rt_debug_usb,("config %d has an invalid interface descriptor of length %d, "
                    "skipping\n", cfgno, d->bLength));
                continue;
            }

            inum = d->bInterfaceNumber;

            if ((dev->quirks & USB_QUIRK_HONOR_BNUMINTERFACES) &&
                n >= nintf_orig) {
                    RT_DEBUG_LOG(rt_debug_usb,( "config %d has more interface descriptors, than it declares in "
                    "bNumInterfaces, ignoring interface number: %d\n",
                    cfgno, inum));
                continue;
            }

            if (inum >= nintf_orig)
                    RT_DEBUG_LOG(rt_debug_usb, ("config %d has an invalid interface number: %d but max is %d\n",
                    cfgno, inum, nintf_orig - 1));

            /* Have we already encountered this interface?
             * Count its altsettings */
            for (i = 0; i < n; ++i)
            {
                if (inums[i] == inum)
                    break;
            }
            if (i < n)
            {
                if (nalts[i] < 255)
                    ++nalts[i];
            } else if (n < USB_MAXINTERFACES)
            {
                inums[n] = inum;
                nalts[n] = 1;
                ++n;
            }

        } else if (header->bDescriptorType ==
                USB_DT_INTERFACE_ASSOCIATION) {
            if (iad_num == USB_MAXIADS) {
                    RT_DEBUG_LOG(rt_debug_usb,("found more Interface "
                           "Association Descriptors "
                           "than allocated for in "
                           "configuration %d\n", cfgno));
            } else {
                config->intf_assoc[iad_num] =
                    (struct usb_interface_assoc_descriptor
                    *)header;
                iad_num++;
            }

        } else if (header->bDescriptorType == USB_DT_DEVICE ||
                header->bDescriptorType == USB_DT_CONFIG)
                RT_DEBUG_LOG(rt_debug_usb,( "config %d contains an unexpected "
                "descriptor of type 0x%X, skipping\n",
                cfgno, header->bDescriptorType));

    }    /* for ((buffer2 = buffer, size2 = size); ...) */
    size = buffer2 - buffer;
    config->desc.wTotalLength = (buffer2 - buffer0);

    if (n != nintf)
            RT_DEBUG_LOG(rt_debug_usb, ("config %d has %d interface%s, different from the descriptor's value: %d\n",
            cfgno, n, plural(n), nintf_orig));
    else if (n == 0)
            RT_DEBUG_LOG(rt_debug_usb, ("config %d has no interfaces?\n", cfgno));
    config->desc.bNumInterfaces = nintf = n;

    /* Check for missing interface numbers */
    for (i = 0; i < nintf; ++i)
    {
        for (j = 0; j < nintf; ++j)
        {
            if (inums[j] == i)
                break;
        }
        if (j >= nintf)
                RT_DEBUG_LOG(rt_debug_usb, ("config %d has no interface number %d\n",
                cfgno, i));
    }

    /* Allocate the usb_interface_caches and altsetting arrays */
    for (i = 0; i < nintf; ++i)
    {
        j = nalts[i];
        if (j > USB_MAXALTSETTING)
        {
                RT_DEBUG_LOG(rt_debug_usb,( "too many alternate settings for config %d interface %d: %d, "
                "using maximum allowed: %d\n",
                cfgno, inums[i], j, USB_MAXALTSETTING));
            nalts[i] = j = USB_MAXALTSETTING;
        }

        len = sizeof(*intfc) + sizeof(struct usb_host_interface) * j;
        config->intf_cache[i] = intfc = rt_malloc(len);
        RT_DEBUG_LOG(rt_debug_usb, ("rt_malloc intf_cache :%d size:%d,addr:%x\n", g_mem_debug++, len, config->intf_cache[i]));
        if (!intfc)
            return -ENOMEM;
        rt_memset(intfc, 0, len);
    }

    /* FIXME: parse the BOS descriptor */

    /* Skip over any Class Specific or Vendor Specific descriptors;
     * find the first interface descriptor */
    config->extra = buffer;
    i = find_next_descriptor(buffer, size, USB_DT_INTERFACE,
        USB_DT_INTERFACE, &n);
    config->extralen = i;
    if (n > 0)
            RT_DEBUG_LOG(rt_debug_usb, ("skipped %d descriptor%s after %s\n",
            n, plural(n), "configuration"));
    buffer += i;
    size -= i;

    /* Parse all the interface/altsetting descriptors */
    while (size > 0)
    {
        retval = usb_parse_interface(dev, cfgno, config,
            buffer, size, inums, nalts);
        if (retval < 0)
            return retval;

        buffer += retval;
        size -= retval;
    }

    /* Check for missing altsettings */
    for (i = 0; i < nintf; ++i)
    {
        intfc = config->intf_cache[i];
        for (j = 0; j < intfc->num_altsetting; ++j)
        {
            for (n = 0; n < intfc->num_altsetting; ++n)
            {
                if (intfc->altsetting[n].desc.
                    bAlternateSetting == j)
                    break;
            }
            if (n >= intfc->num_altsetting)
                    RT_DEBUG_LOG(rt_debug_usb, ("config %d interface %d has no altsetting %d\n",
                    cfgno, inums[i], j));
        }
    }

    return 0;
}

/* hub-only!! ... and only exported for reset/reinit path.
 * otherwise used internally on disconnect/destroy path
 */
void usb_destroy_configuration(struct usb_device *dev)
{
    int c, i;

    if (!dev->config)
        return;

    if (dev->rawdescriptors)
    {
        for (i = 0; i < dev->descriptor.bNumConfigurations; i++)
        {
            rt_free(dev->rawdescriptors[i]);
            RT_DEBUG_LOG(rt_debug_usb, ("rt_free usb rawdescriptors i:%d,addr:%x\n", --g_mem_debug, dev->rawdescriptors[i]));
        }

        rt_free(dev->rawdescriptors);
        RT_DEBUG_LOG(rt_debug_usb, ("rt_free usb rawdescriptors:%d,addr:%x\n", --g_mem_debug, dev->rawdescriptors));
        dev->rawdescriptors = RT_NULL;
    }

    for (c = 0; c < dev->descriptor.bNumConfigurations; c++)
    {
        struct usb_host_config *cf = &dev->config[c];
/* RT_DEBUG_LOG(rt_debug_usb,("rt_free usb cf string:%d\n",--g_mem_debug)); */
/* rt_free(cf->string); */
        for (i = 0; i < cf->desc.bNumInterfaces; i++)
        {
            if (cf->intf_cache[i])
            {
                cf->intf_cache[i]->ref--;
                    if (!(cf->intf_cache[i]->ref))
                        ;
                   /* usb_release_interface_cache(cf->intf_cache[i]->ref); */
            }
        }
    }
    RT_DEBUG_LOG(rt_debug_usb, ("rt_free dev config:%d,addr:%x\n", --g_mem_debug, dev->config));
    rt_free(dev->config);
    dev->config = RT_NULL;
}


/*
 * Get the USB config descriptors, cache and parse'em
 *
 * hub-only!! ... and only in reset path, or usb_new_device()
 * (used by real hubs and virtual root hubs)
 *
 * NOTE: if this is a WUSB device and is not authorized, we skip the
 *       whole thing. A non-authorized USB device has no
 *       configurations.
 */
int usb_get_configuration(struct usb_device *dev)
{
    int ncfg = dev->descriptor.bNumConfigurations;
    int result = 0;
    unsigned int cfgno, length;
    unsigned char *bigbuffer;
    struct usb_config_descriptor *desc;

    cfgno = 0;
    result = -ENOMEM;
    if (ncfg > USB_MAXCONFIG)
    {
        RT_DEBUG_LOG(rt_debug_usb, ("too many configurations: %d, using maximum allowed: %d\n",
            ncfg, USB_MAXCONFIG));
        dev->descriptor.bNumConfigurations = ncfg = USB_MAXCONFIG;
    }

    if (ncfg < 1)
    {
        RT_DEBUG_LOG(rt_debug_usb, ("no configurations\n"));
        return -EINVAL;
    }

    length = ncfg * sizeof(struct usb_host_config);
    dev->config = rt_malloc(length);
    RT_DEBUG_LOG(rt_debug_usb, ("rt_malloc dev config :%d size:%d,addr:%x\n", g_mem_debug++, length, dev->config));
    if (!dev->config)
        goto err2;
    rt_memset(dev->config, 0, length);

    length = ncfg * sizeof(char *);
    dev->rawdescriptors = rt_malloc(length);
    RT_DEBUG_LOG(rt_debug_usb, ("rt_malloc rawdescriptors :%d size:%d,addr:%x\n", g_mem_debug++, length, dev->rawdescriptors));
    if (!dev->rawdescriptors)
        {
           rt_free(dev->config);
           dev->config = RT_NULL;
        goto err2;
        }

    rt_memset(dev->rawdescriptors, 0, length);

    /* desc = rt_malloc(USB_DT_CONFIG_SIZE); */
    /* RT_DEBUG_LOG(rt_debug_usb,("rt_malloc USB_DT_CONFIG_SIZE :%d size:%d\n",g_mem_debug++,USB_DT_CONFIG_SIZE)); */
    desc = usb_dma_buffer_alloc(USB_DT_CONFIG_SIZE, dev->bus);
    if (!desc)
    {
        rt_free(dev->config);
        dev->config = RT_NULL;
        rt_free(dev->rawdescriptors);
        dev->rawdescriptors = RT_NULL;
        goto err2;
    }

    result = 0;
    for (; cfgno < ncfg; cfgno++)
    {
        /* We grab just the first descriptor so we know how long
         * the whole configuration is */
        result = usb_get_descriptor(dev, USB_DT_CONFIG, cfgno,
            desc, USB_DT_CONFIG_SIZE);
        if (result < 0)
        {
            RT_DEBUG_LOG(rt_debug_usb, ("unable to read config index %d descriptor/%s: %d\n",
                cfgno, "start", result));
            RT_DEBUG_LOG(rt_debug_usb, ("chopping to %d config(s)\n", cfgno));
            dev->descriptor.bNumConfigurations = cfgno;
            rt_free(dev->config);
            dev->config = RT_NULL;
            rt_free(dev->rawdescriptors);
            dev->rawdescriptors = RT_NULL;
            break;
        } else if (result < 4)
        {
            RT_DEBUG_LOG(rt_debug_usb,("config index %d descriptor too short (expected %i, got %i)\n",
                cfgno,
                USB_DT_CONFIG_SIZE, result));
            result = -EINVAL;
            rt_free(dev->config);
            dev->config = RT_NULL;
            rt_free(dev->rawdescriptors);
            dev->rawdescriptors = RT_NULL;
            goto err;
        }
        if (desc->wTotalLength > USB_DT_CONFIG_SIZE)
            length = desc->wTotalLength;
        else
            length = USB_DT_CONFIG_SIZE;

        /* Now that we know the length, get the whole thing */
        /* bigbuffer =rt_malloc(length); */
        /* RT_DEBUG_LOG(rt_debug_usb,("rt_malloc bigbuffer :%d size:%d\n",g_mem_debug++,length)); */
        bigbuffer = usb_dma_buffer_alloc(length, dev->bus);
        if (!bigbuffer)
        {
            result = -ENOMEM;
            rt_free(dev->config);
            dev->config = RT_NULL;
            rt_free(dev->rawdescriptors);
            dev->rawdescriptors = RT_NULL;
            goto err;
        }
        result = usb_get_descriptor(dev, USB_DT_CONFIG, cfgno,
                bigbuffer, length);
        if (result < 0)
        {
            RT_DEBUG_LOG(rt_debug_usb, ("unable to read config index %d descriptor/%s\n",
                cfgno, "all"));
            rt_free(dev->config);
            dev->config = RT_NULL;
            rt_free(dev->rawdescriptors);
            dev->rawdescriptors = RT_NULL;
            usb_dma_buffer_free(bigbuffer, dev->bus);
            goto err;
        }
        if (result < length)
        {
            RT_DEBUG_LOG(rt_debug_usb, ("config index %d descriptor too short (expected %i, got %i)\n",
                cfgno, length, result));
            length = result;
        }
        dev->rawdescriptors[cfgno] = rt_malloc(length);
        RT_DEBUG_LOG(rt_debug_usb, ("rt_malloc bigbuffer :%d size:%d,addr:%x\n", g_mem_debug++, length, dev->rawdescriptors[cfgno]));
        /* dev->rawdescriptors[cfgno] =(char *) bigbuffer; */
        rt_memcpy(dev->rawdescriptors[cfgno], bigbuffer, length);
        usb_dma_buffer_free(bigbuffer, dev->bus);
        result = usb_parse_configuration(dev, cfgno,
            &dev->config[cfgno], (unsigned char *)dev->rawdescriptors[cfgno], length);
        if (result < 0)
        {
            ++cfgno;
            rt_free(dev->config);
            dev->config = RT_NULL;
            rt_free(dev->rawdescriptors);
            dev->rawdescriptors = RT_NULL;
            goto err;
        }
    }
    result = 0;

err:
        /* RT_DEBUG_LOG(rt_debug_usb,("rt_free dev USB_DT_CONFIG_SIZE:%d\n",--g_mem_debug)); */
       /* rt_free(desc); */
     usb_dma_buffer_free(desc, dev->bus);
    dev->descriptor.bNumConfigurations = cfgno;
err2:
    if (result == -ENOMEM)
        RT_DEBUG_LOG(rt_debug_usb, ("out of memory\n"));
    return result;
}

