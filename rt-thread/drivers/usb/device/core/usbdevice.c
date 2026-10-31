/*
 * File      : hid.c
 * COPYRIGHT (C) 2008 - 2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2012-10-02     Yi Qiu       first version
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <rtservice.h>
#include "string.h"
#include "usbdevice.h"
#include "ch9.h"

#ifdef FH_RT_USB_DEVICE

#define USB_DEVICE_CONTROLLER_NAME      "usbd"

#define WEBCAM_VENDOR_ID 0x1d6c
#define WEBCAM_PRODUCT_ID 0x0103

#define AUDIO_VENDOR_NUM        0x1d6b    /* Linux Foundation */
#define AUDIO_PRODUCT_NUM        0x0101    /* Linux-USB Audio Gadget */

/* #define device_vendor_label     "webcam vendor" */
/* #define device_product_label    "webcam product" */
/* #define device_serialnum_label    "123456789" */
#define device_config_label     "webcam config"

#ifdef FH_RT_USB_DEVICE_COMPOSITE

#ifdef FH_RT_USB_DEVICE_UVC
#include "uvc_init.h"

static struct udevice_descriptor compsit_desc = {

    USB_DESC_LENGTH_DEVICE,     /* bLength; */
    USB_DESC_TYPE_DEVICE,       /* type; */
    USB_BCD_VERSION,            /* bcdUSB; */
    USB_CLASS_MISC,             /* bDeviceClass; */
    0x02,                       /* bDeviceSubClass; */
    0x01,                       /* bDeviceProtocol; */
    0x40,                       /* bMaxPacketSize0; */
    WEBCAM_VENDOR_ID,                 /* idVendor; */
    WEBCAM_PRODUCT_ID,                /* idProduct; */
    USB_BCD_DEVICE,             /* bcdDevice; */
    USB_STRING_MANU_INDEX,      /* iManufacturer; */
    USB_STRING_PRODUCT_INDEX,   /* iProduct; */
    USB_STRING_SERIAL_INDEX,    /* iSerialNumber; */
    USB_DYNAMIC,                /* bNumConfigurations; */
};
#elif defined(FH_RT_USB_DEVICE_UAC)
static struct udevice_descriptor compsit_desc = {

    USB_DESC_LENGTH_DEVICE,     /* bLength; */
    USB_DESC_TYPE_DEVICE,       /* type; */
    USB_BCD_VERSION,            /* bcdUSB; */
    USB_CLASS_MISC,             /* bDeviceClass; */
    0x02,                       /* bDeviceSubClass; */
    0x01,                       /* bDeviceProtocol; */
    0x40,                       /* bMaxPacketSize0; */
    AUDIO_VENDOR_NUM,                 /* idVendor; */
    AUDIO_PRODUCT_NUM,                /* idProduct; */
    USB_BCD_DEVICE,             /* bcdDevice; */
    USB_STRING_MANU_INDEX,      /* iManufacturer; */
    USB_STRING_PRODUCT_INDEX,   /* iProduct; */
    USB_STRING_SERIAL_INDEX,    /* iSerialNumber; */
    USB_DYNAMIC,                /* bNumConfigurations; */
};
#else
static struct udevice_descriptor compsit_desc = {

    USB_DESC_LENGTH_DEVICE,     /* bLength; */
    USB_DESC_TYPE_DEVICE,       /* type; */
    USB_BCD_VERSION,            /* bcdUSB; */
    USB_CLASS_MISC,             /* bDeviceClass; */
    0x02,                       /* bDeviceSubClass; */
    0x01,                       /* bDeviceProtocol; */
    0x40,                       /* bMaxPacketSize0; */
    0x0000,                 /* idVendor; */
    0x0000,                /* idProduct; */
    USB_BCD_DEVICE,             /* bcdDevice; */
    USB_STRING_MANU_INDEX,      /* iManufacturer; */
    USB_STRING_PRODUCT_INDEX,   /* iProduct; */
    USB_STRING_SERIAL_INDEX,    /* iSerialNumber; */
    USB_DYNAMIC,                /* bNumConfigurations; */
};
#endif

ALIGN(4)
static struct usb_qualifier_descriptor _dev_qualifier = {
    sizeof(_dev_qualifier),
    USB_DESC_TYPE_DEVICEQUALIFIER,
    0x0200,
    0xef, /* USB_CLASS_VIDEO USB_CLASS_DEVICE */
    0x02, /* subclass */
    0x01, /* device_protocol */
    0x40, /* max packetsize */
    0x00, /* numberConfigurations */
    0x00, /* reserved */
};

#endif

struct usb_os_comp_id_descriptor usb_comp_id_desc = {

    /* head section */
    {
        USB_DYNAMIC,
        0x0100,
        0x04,
        USB_DYNAMIC,
        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    },
};

static char *usb_device_vendor_label = DEVICE_VENDOR_LABEL;
static char *usb_device_product_label = DEVICE_PRODUCT_LABEL;
static char *usb_device_serialnum_label = DEVICE_SERIALNUM_LABEL;

extern char *uac_interface_string;
int rt_usbd_func_create(udevice_t device, struct uconfig *c)
{
    int ret;

#ifdef FH_RT_USB_DEVICE_UAC
    uac_interface_string = UAC_STR_ASSOCIATION;
#endif

#ifdef FH_RT_USB_DEVICE_UVC
    extern char *uvc_interface_string_association;
    extern struct uvc_stream_ops *g_uvc_ops;
    struct UvcInfo *uvc_info = NULL;
    char *pData = NULL;

    uvc_interface_string_association = UVC_STRING_ASSOCIATION;
    if (g_uvc_ops && g_uvc_ops->uvc_vision)
        uvc_info = g_uvc_ops->uvc_vision();
    else
        rt_kprintf("[UVC] WARNING: uvc_vision is null, device_info not available!");
    if (uvc_info && uvc_info->uvcinfo_change)
    {
        usb_device_vendor_label = uvc_info->info.infoVal.szFacture;
        usb_device_product_label = uvc_info->info.infoVal.szProduct;
        usb_device_serialnum_label = uvc_info->info.infoVal.szSerial;

        compsit_desc.idVendor  = uvc_info->info.infoVal.vid;
        compsit_desc.idProduct = uvc_info->info.infoVal.pid;
        compsit_desc.bcdDevice = uvc_info->info.infoVal.bcd;
        uvc_interface_string_association = uvc_info->info.infoVal.szVideoIntf;
#ifdef FH_RT_USB_DEVICE_UAC
        uac_interface_string = uvc_info->info.infoVal.szMicIntf;
#endif
    } else if (uvc_info && !uvc_info->uvcinfo_change)
    {
        pData = uvc_info->info.infoVal.szFacture;
        strcpy(pData, DEVICE_VENDOR_LABEL);

        pData = uvc_info->info.infoVal.szProduct;
        strcpy(pData, DEVICE_PRODUCT_LABEL);

        pData = uvc_info->info.infoVal.szSerial;
        strcpy(pData, DEVICE_SERIALNUM_LABEL);

        pData = uvc_info->info.infoVal.szVideoIntf;
        strcpy(pData, UVC_STRING_ASSOCIATION);
#ifdef FH_RT_USB_DEVICE_UAC
        pData = uvc_info->info.infoVal.szMicIntf;
        strcpy(pData, UAC_STR_ASSOCIATION);
#endif
        uvc_info->info.infoVal.vid  = compsit_desc.idVendor;
        uvc_info->info.infoVal.pid  = compsit_desc.idProduct;
        uvc_info->info.infoVal.bcd  = compsit_desc.bcdDevice;
    }
#endif

    /* Allocate string descriptor numbers ... note that string contents
    * can be overridden by the composite_dev glue.
    */
    ret = usb_string_id(c, usb_device_vendor_label);
    if (ret < 0)
        goto error;

    ret = usb_string_id(c, usb_device_product_label);
    if (ret < 0)
        goto error;

    ret = usb_string_id(c, usb_device_serialnum_label);
    if (ret < 0)
        goto error;

    ret = usb_string_id(c, device_config_label);
    if (ret < 0)
        goto error;
    c->cfg_desc.iConfiguration = ret;

    c->device = device;
    rt_usbd_device_set_qualifier(c->device, &_dev_qualifier);

#ifdef FH_RT_USB_DEVICE_UVC
    ret = uvc_bind_config(c);
    if (ret < 0)
        goto error;
#endif

#ifdef FH_RT_USB_DEVICE_UAC
    ret = uac_bind_config(c);
    if (ret < 0)
        goto error;
#endif

#ifdef FH_RT_USB_DEVICE_CDC
    ret = cdc_bind_config(c);
    if (ret < 0)
        goto error;
#endif

#ifdef FH_RT_USB_DEVICE_HID
    // compsit_desc.bDeviceClass = 0;
    // compsit_desc.bDeviceProtocol = 0;
    // compsit_desc.bDeviceSubClass = 0;
    ret = hid_bind_config(c);
    if (ret < 0)
        goto error;
#endif

#ifdef FH_RT_USB_DEVICE_MSTORAGE
    ret = mstorage_bind_config(c);
    if (ret < 0)
        goto error;
#endif

#ifdef FH_RT_USB_DEVICE_RNDIS
    ret = rndis_bind_config(c);
    if (ret < 0)
        goto error;
#endif

    return 0;
error:
    rt_kprintf("webcam bind error\n");
    return ret;
}

static int g_usb_device_init = 0;
rt_err_t rt_usb_device_init(void)
{
    rt_device_t udc;
    udevice_t udevice;
    uconfig_t cfg;

    if (g_usb_device_init)
        return RT_EOK;
    g_usb_device_init = 1;
    /* create and startup usb device thread */
    rt_usbd_core_init();

    /* create a device object */
    udevice = rt_usbd_device_new();

    udc = rt_device_find(USB_DEVICE_CONTROLLER_NAME);
    if (udc == RT_NULL)
    {
        rt_kprintf("can't find usb device controller %s\n", USB_DEVICE_CONTROLLER_NAME);
        return -RT_ERROR;
    }

    /* set usb controller driver to the device */
    rt_usbd_device_set_controller(udevice, (udcd_t)udc);

    /* create a configuration object */
    cfg = rt_usbd_config_new();

    rt_usbd_device_set_os_comp_id_desc(udevice, &usb_comp_id_desc);

    rt_usbd_func_create(udevice, cfg);

    /* set device descriptor to the device */
    rt_usbd_device_set_descriptor(udevice, &compsit_desc);

    /* add the configuration to the device */
    rt_usbd_device_add_config(udevice, cfg);

    /* set default configuration to 1 */
    rt_usbd_set_config(udevice, 1);

    /* initialize usb device controller */
    rt_device_init(udc);

    return RT_EOK;
}
#endif
