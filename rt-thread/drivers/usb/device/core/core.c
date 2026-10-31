/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2012-10-01     Yi Qiu       first version
 * 2012-12-12     heyuanjie87  change endpoint and function handler
 * 2012-12-30     heyuanjie87  change inferface handler
 * 2013-04-26     aozima       add DEVICEQUALIFIER support.
 * 2013-07-25     Yi Qiu       update for USB CV test
 * 2017-11-15     ZYH          fix ep0 transform error
 */
#include <rtdef.h>
#include <rtthread.h>
#include <rtdevice.h>
#include "usbcommon.h"
#include "usbdevice.h"
#include "ch9.h"
#include "../udc/udc_dwc.h"


/* #define MY_DWC_DEBUG */
#ifdef MY_DWC_DEBUG
#define MY_DWC_DBG(fmt, args...)   rt_kprintf(fmt, ##args)
#else
#define MY_DWC_DBG(fmt, args...)
#endif

#ifndef le16_to_cpu
#define le16_to_cpu(x)  x
#endif


static rt_list_t device_list;
extern rt_thread_t g_int_thread_id;
/* extern void usb_reset(void); */

static rt_size_t rt_usbd_ep_write(udevice_t device, uep_t ep, void *buffer, rt_size_t size);
static rt_size_t rt_usbd_ep_read_prepare(udevice_t device, uep_t ep, void *buffer, rt_size_t size);
static rt_err_t rt_usbd_ep_assign(udevice_t device, uep_t ep);
static void rt_usbd_thread_entry(struct udev_msg *msg);
static rt_err_t _function_request(udevice_t device, ureq_t setup);
rt_err_t rt_usbd_eps_unassign(udevice_t device, uep_t ep);

/**
 * This function will handle get_device_descriptor bRequest.
 *
 * @param device the usb device object.
 * @param setup the setup bRequest.
 *
 * @return RT_EOK on successful.
 */
static rt_err_t _get_device_descriptor(struct udevice *device, ureq_t setup)
{
    rt_size_t size;

    /* parameter check */
    RT_ASSERT(device != RT_NULL);
    RT_ASSERT(setup != RT_NULL);

    RT_DEBUG_LOG(RT_DEBUG_USB, ("_get_device_descriptor\n"));

    /* device descriptor wLength should less than USB_DESC_LENGTH_DEVICE*/
    size = (setup->wLength > USB_DESC_LENGTH_DEVICE) ?
           USB_DESC_LENGTH_DEVICE : setup->wLength;

    /* send device descriptor to endpoint 0 */
    rt_usbd_ep0_write(device, (rt_uint8_t *) &device->dev_desc, size);

    return RT_EOK;
}

/**
 * This function will handle get_config_descriptor bRequest.
 *
 * @param device the usb device object.
 * @param setup the setup bRequest.
 *
 * @return RT_EOK on successful.
 */

static rt_err_t _get_config_descriptor(struct udevice *device, ureq_t setup)
{
    rt_size_t size;
    ucfg_desc_t cfg_desc;

    /* parameter check */
    RT_ASSERT(device != RT_NULL);
    RT_ASSERT(setup != RT_NULL);

    RT_DEBUG_LOG(RT_DEBUG_USB, ("_get_config_descriptor\n"));

#if 0
    /* TODO */
    if ((setup->wValue >> 8) == USB_DESC_TYPE_OTHERSPEED)
        cfg_desc = &device->curr_cfg->other_cfg_desc;
    else
        cfg_desc = &device->curr_cfg->cfg_desc;
#else
    cfg_desc = &device->curr_cfg->cfg_desc;
    if ((setup->wValue >> 8) == USB_DESC_TYPE_OTHERSPEED)
        cfg_desc->type = USB_DESC_TYPE_OTHERSPEED;
    else
        cfg_desc->type = USB_DESC_TYPE_CONFIGURATION;
#endif
    size = (setup->wLength > cfg_desc->wTotalLength) ?
           cfg_desc->wTotalLength : setup->wLength;
    rt_usbd_ep0_write(device, (rt_uint8_t *)cfg_desc, size);

    return RT_EOK;
}

/**
 * _get_bos_descriptor() - prepares the BOS descriptor.
 * @cdev: pointer to usb_composite device to generate the bos
 *	descriptor for
 *
 * This function generates the BOS (Binary Device Object)
 * descriptor and its device capabilities descriptors. The BOS
 * descriptor should be supported by a SuperSpeed device.
 */
static int _get_bos_descriptor(struct udevice *device, ureq_t setup)
{
	struct usb_ext_cap_descriptor	*usb_ext;
	struct usb_bos_descriptor	*bos;
    rt_size_t size;
    unsigned char *bos_descriptor;

    bos_descriptor = device->curr_cfg->bos_descriptor;
    rt_memset(bos_descriptor, 0, sizeof(device->curr_cfg->bos_descriptor));

    bos = (struct usb_bos_descriptor *)bos_descriptor;
	bos->bLength = USB_DT_BOS_SIZE;
	bos->bDescriptorType = USB_DT_BOS;

	bos->wTotalLength = USB_DT_BOS_SIZE;
	bos->bNumDeviceCaps = 0;

	usb_ext = (struct usb_ext_cap_descriptor *)(bos_descriptor + bos->wTotalLength);
	bos->bNumDeviceCaps++;
    bos->wTotalLength += USB_DT_USB_EXT_CAP_SIZE;
	usb_ext->bLength = USB_DT_USB_EXT_CAP_SIZE;
	usb_ext->bDescriptorType = USB_DT_DEVICE_CAPABILITY;
	usb_ext->bDevCapabilityType = USB_CAP_TYPE_EXT;
	usb_ext->bmAttributes = 0;

    size = (setup->wLength > bos->wTotalLength) ?
           bos->wTotalLength : setup->wLength;

    int i;
    for(i = 0; i < size;i++)
        rt_kprintf("0x%x ", bos_descriptor[i]);
    rt_kprintf("\n size %d addr %p\n", size, bos_descriptor);
    rt_usbd_ep0_write(device, (rt_uint8_t *)bos_descriptor, size);
	return RT_EOK;
}


/**
 * This function will handle get_string_descriptor bRequest.
 *
 * @param device the usb device object.
 * @param setup the setup bRequest.
 *
 * @return RT_EOK on successful, -RT_ERROR on invalid bRequest.
 */
static rt_err_t _get_string_descriptor(struct udevice *device, ureq_t setup)
{
    struct ustring_descriptor str_desc;
    rt_uint8_t index, i;
    rt_uint32_t len;

    /* parameter check */
    RT_ASSERT(device != RT_NULL);
    RT_ASSERT(setup != RT_NULL);

    RT_DEBUG_LOG(RT_DEBUG_USB, ("_get_string_descriptor\n"));
    uconfig_t cfg  = device->curr_cfg;

    str_desc.type = USB_DESC_TYPE_STRING;
    index = setup->wValue & 0xFF;

    if (index == 0xEE
        || index == 0xef)
    {
        index = cfg->next_string_id; /* USB_STRING_OS_INDEX; */
    }

    if (index > cfg->next_string_id)/* USB_STRING_MAX) */
    {
        rt_kprintf("unknown string index, idx = %d\n", index);
        rt_usbd_ep0_set_stall(device);
        return -RT_ERROR;
    }
    else if (index == USB_STRING_LANGID_INDEX)
    {
        str_desc.bLength = 4;
        str_desc.String[0] = 0x09;
        str_desc.String[1] = 0x04;
    }
    else
    {
        /* len = rt_strlen(device->str[index]); */
        len = rt_strlen(cfg->strings[index]);
        str_desc.bLength = len*2 + 2;

        for (i = 0; i < len; i++)
        {
            str_desc.String[i*2] = cfg->strings[index][i];/* device->str[index][i]; */
            str_desc.String[i*2 + 1] = 0;
        }
    }

    if (setup->wLength > str_desc.bLength)
        len = str_desc.bLength;
    else
        len = setup->wLength;

    /* send string descriptor to endpoint 0 */
    if (len == 0)
    {
        rt_kprintf("%s index %d, string len %d\n", __func__, index, len);
        len = 1;/* IN data phase send 0 data will cause the DWC driver to switch to state phase!(IN HW_SendPKT()) */
    }
    rt_usbd_ep0_write(device, (rt_uint8_t *)&str_desc, len);
    return RT_EOK;
}

static rt_err_t _get_qualifier_descriptor(struct udevice *device, ureq_t setup)
{
    RT_DEBUG_LOG(RT_DEBUG_USB, ("_get_qualifier_descriptor\n"));

    /* parameter check */
    RT_ASSERT(device != RT_NULL);
    RT_ASSERT(setup != RT_NULL);

    if (device->dev_qualifier && device->dcd->device_is_hs)
    {
        /* send device qualifier descriptor to endpoint 0 */
        rt_usbd_ep0_write(device, (rt_uint8_t *)device->dev_qualifier,
                     sizeof(struct usb_qualifier_descriptor));
    }
    else
    {
        rt_usbd_ep0_write(device, (rt_uint8_t *)device->dev_qualifier,
                     sizeof(struct usb_qualifier_descriptor));
       /* rt_usbd_ep0_set_stall(device); */
    }

    return RT_EOK;
}

/**
 * This function will handle get_descriptor bRequest.
 *
 * @param device the usb device object.
 * @param setup the setup bRequest.
 *
 * @return RT_EOK on successful.
 */
static rt_err_t _get_descriptor(struct udevice *device, ureq_t setup)
{
    /* parameter check */
    RT_ASSERT(device != RT_NULL);
    RT_ASSERT(setup != RT_NULL);

    if (setup->bRequestType == USB_REQ_TYPE_DIR_IN)
    {
        switch (setup->wValue >> 8)
        {
        case USB_DESC_TYPE_DEVICE:
            _get_device_descriptor(device, setup);
            break;
        case USB_DESC_TYPE_CONFIGURATION:
            _get_config_descriptor(device, setup);
            break;
        case USB_DESC_TYPE_STRING:
            _get_string_descriptor(device, setup);
            break;
        case USB_DESC_TYPE_DEVICEQUALIFIER:
            _get_qualifier_descriptor(device, setup);
            break;

        case USB_DESC_TYPE_OTHERSPEED:
            _get_config_descriptor(device, setup);
            break;
        case USB_DESC_TYPE_BOS:
            _get_bos_descriptor(device, setup);
            break;
        default:
            rt_kprintf("unsupported descriptor request wValue 0x%x\n", setup->wValue);
            rt_usbd_ep0_set_stall(device);
            break;
        }
    }
    else
    {
        rt_kprintf("request direction error\n");
        rt_usbd_ep0_set_stall(device);
    }

    return RT_EOK;
}

static rt_err_t _deal_unknow_setup(struct udevice *device, ureq_t setup)
{
    return 0;
}


/**
 * This function will handle get_interface bRequest.
 *
 * @param device the usb device object.
 * @param setup the setup bRequest.
 *
 * @return RT_EOK on successful.
 */
static rt_err_t _get_interface(struct udevice *device, ureq_t setup)
{

    ufunction_t f;
    int intf = setup->wIndex & 0xFF;
    int value = -1;

    if (setup->bRequestType != (USB_DIR_IN | USB_RECIP_INTERFACE))
        return _deal_unknow_setup(device, setup);

    if (!device->curr_cfg || intf >= MAX_CONFIG_INTERFACES)
        return -RT_ERROR;

    f = device->curr_cfg->interface[intf];
    if (!f)
        return -RT_ERROR;

    /* lots of interfaces only need altsetting zero... */
    value = f->get_alt ? f->get_alt(f, setup->wIndex) : 0;
    if (value < 0)
        return -RT_ERROR;

    rt_usbd_ep0_write(device, &value, 1);

    return RT_EOK;
}

/**
 * This function will handle set_interface bRequest.
 *
 * @param device the usb device object.
 * @param setup the setup bRequest.
 *
 * @return RT_EOK on successful.
 */

extern void dwc_print_mask_val(void);
extern volatile uint8_t g_uvc_list_busy;
extern void *g_v_buf;
void video_buf_reinit(void);

static rt_err_t _set_interface(struct udevice *device, ureq_t setup)
{
    /* parameter check */
    RT_ASSERT(device != RT_NULL);
    RT_ASSERT(setup != RT_NULL);

    RT_DEBUG_LOG(RT_DEBUG_USB, ("_set_interface:%d %d\n",setup->wIndex & 0xFF,setup->wValue & 0xFF));
    if (device->state != USB_STATE_CONFIGURED)
    {
        rt_kprintf("_set_interface error, not CONFIGURED\n");
        rt_usbd_ep0_set_stall(device);
        return -RT_ERROR;
    }

    ufunction_t f;
    ureq_t ctrl = setup;
    int                value = -RT_ERROR;
    rt_uint16_t                w_index = le16_to_cpu(ctrl->wIndex);
    rt_uint8_t                intf = w_index & 0xFF;
    rt_uint16_t                w_value = le16_to_cpu(ctrl->wValue);

    if (ctrl->bRequestType != USB_RECIP_INTERFACE)
    {
        rt_kprintf("_set_interface error, bRequestType not INTERFACE!\n");
        return _deal_unknow_setup(device, setup);
    }

    if (!device->curr_cfg || intf >= MAX_CONFIG_INTERFACES)
    {
        rt_kprintf("_set_interface error: line %d!\n", __LINE__);
        return -RT_ERROR;
    }

    f = device->curr_cfg->interface[intf];
    if (!f)
    {
        rt_kprintf("_set_interface error, func is NULL!\n");
        return -RT_ERROR;
    }

    if (w_value && !f->set_alt)
    {
        rt_kprintf("_set_interface error, set_alt is NULL!\n");
        return -RT_ERROR;
    }

    value = f->set_alt(f, w_index, w_value);
    if (value != 0)
        rt_kprintf("_set_interface error, f->set_alt return %d!\n", value);

    dcd_ep0_send_status(device->dcd);

    return RT_EOK;
}

/**
 * This function will handle get_config bRequest.
 *
 * @param device the usb device object.
 * @param setup the setup bRequest.
 *
 * @return RT_EOK on successful.
 */
static rt_err_t _get_config(struct udevice *device, ureq_t setup)
{
    rt_uint8_t value;

    /* parameter check */
    RT_ASSERT(device != RT_NULL);
    RT_ASSERT(setup != RT_NULL);
    RT_ASSERT(device->curr_cfg != RT_NULL);

    RT_DEBUG_LOG(RT_DEBUG_USB, ("_get_config\n"));

    if (device->state == USB_STATE_CONFIGURED)
    {
        /* get current configuration */
        value = device->curr_cfg->cfg_desc.bConfigurationValue;
    }
    else
    {
        value = 0;
    }
    /* write the current configuration to endpoint 0 */
    rt_usbd_ep0_write(device, &value, 1);

    return RT_EOK;
}

/**
 * This function will handle set_config bRequest.
 *
 * @param device the usb device object.
 * @param setup the setup bRequest.
 *
 * @return RT_EOK on successful.
 */
static rt_err_t _set_config(struct udevice *device, ureq_t setup)
{
    uconfig_t cfg;
    int tmp;

    /* parameter check */
    RT_ASSERT(device != RT_NULL);
    RT_ASSERT(setup != RT_NULL);

    RT_DEBUG_LOG(RT_DEBUG_USB, ("_set_config\n"));


    if (setup->wValue > device->dev_desc.bNumConfigurations)
    {
        rt_kprintf("_set_config\n");
        rt_usbd_ep0_set_stall(device);
        return -RT_ERROR;
    }

    if (setup->wValue == 0)
    {
        RT_DEBUG_LOG(RT_DEBUG_USB, ("address state\n"));
        device->state = USB_STATE_ADDRESS;

        goto _exit;
    }

    /* set current configuration */
    rt_usbd_set_config(device, setup->wValue);
    cfg = device->curr_cfg;


   ufunction_t    f;

    for (tmp = 0; tmp < MAX_CONFIG_INTERFACES; tmp++)
    {
        f = cfg->interface[tmp];
        if (!f)
            break;
        FUNC_ENABLE(f);
        if (f->set_alt)
            f->set_alt(f, tmp, 0);
    }

    device->state = USB_STATE_CONFIGURED;

_exit:
    /* issue status stage */
    dcd_ep0_send_status(device->dcd);
    return RT_EOK;
}

/**
 * This function will handle set_address bRequest.
 *
 * @param device the usb device object.
 * @param setup the setup bRequest.
 *
 * @return RT_EOK on successful.
 */
static rt_err_t _set_address(struct udevice *device, ureq_t setup)
{
    /* parameter check */
    RT_ASSERT(device != RT_NULL);
    RT_ASSERT(setup != RT_NULL);

    /* set address in device control driver */
    dcd_set_address(device->dcd, setup->wValue);

    /* issue status stage */
    dcd_ep0_send_status(device->dcd);

    RT_DEBUG_LOG(RT_DEBUG_USB, ("_set_address\n"));

    device->state = USB_STATE_ADDRESS;

    return RT_EOK;
}


/**
 * This function will handle standard bRequest.
 *
 * @param device the usb device object.
 * @param setup the setup bRequest.
 *
 * @return RT_EOK on successful.
 */
static rt_err_t _standard_request(struct udevice *device, ureq_t setup)
{
    udcd_t dcd;
    uep_t ep;
    ufunction_t func;
    rt_uint16_t value = 0;

    /* parameter check */
    RT_ASSERT(device != RT_NULL);
    RT_ASSERT(setup != RT_NULL);

    dcd = device->dcd;

    switch (setup->bRequestType & USB_REQ_TYPE_RECIPIENT_MASK)
    {
    case USB_REQ_TYPE_DEVICE:
        switch (setup->bRequest)
        {
        case USB_REQ_GET_STATUS:
            rt_usbd_ep0_write(device, &value, 2);
            break;
        case USB_REQ_CLEAR_FEATURE:
            rt_usbd_clear_feature(device, setup->wValue, setup->wIndex);
            dcd_ep0_send_status(dcd);
            break;
        case USB_REQ_SET_FEATURE:
            rt_usbd_set_feature(device, setup->wValue, setup->wIndex);
            break;
        case USB_REQ_SET_ADDRESS:
            _set_address(device, setup);
            break;
        case USB_REQ_GET_DESCRIPTOR:
            _get_descriptor(device, setup);
            break;
        case USB_REQ_SET_DESCRIPTOR:
            rt_kprintf("STD: USB_REQ_SET_DESCRIPTOR\n");
            rt_usbd_ep0_set_stall(device);
            break;
        case USB_REQ_GET_CONFIGURATION:
            _get_config(device, setup);
            break;
        case USB_REQ_SET_CONFIGURATION:
            _set_config(device, setup);
            break;
        default:
            rt_kprintf("STD:unknown device request\n");
            rt_usbd_ep0_set_stall(device);
            break;
        }
        break;
    case USB_REQ_TYPE_INTERFACE:
        switch (setup->bRequest)
        {
        case USB_REQ_GET_INTERFACE:
            _get_interface(device, setup);
            break;
        case USB_REQ_SET_INTERFACE:
            _set_interface(device, setup);
            break;
        default:
            if (_function_request(device, setup) != RT_EOK)
            {
                rt_kprintf("STD:unknown interface request\n");
                rt_usbd_ep0_set_stall(device);
                return -RT_ERROR;
            }
            else
                break;
        }
        break;
    case USB_REQ_TYPE_ENDPOINT:
        switch (setup->bRequest)
        {
        case USB_REQ_GET_STATUS:
            rt_usbd_ep0_write(device, &value, 2);
            break;
        case USB_REQ_CLEAR_FEATURE:
            rt_usbd_clear_feature(device, setup->wValue, setup->wIndex);
            ep = rt_usbd_find_endpoint(device, &func, setup->wIndex);
            dcd_ep_enable(dcd, ep);
            dcd_ep0_send_status(dcd);
#ifdef FH_RT_UVC_EP_TYPE_BULK
            extern void fh_uvc_status_set(int stream_id, int set);
            if ((ep->ep_desc->bEndpointAddress & 0x0f) == UVC_STREAM1_EP1)
            {
                fh_uvc_status_set(0, 0);
            } else if ((ep->ep_desc->bEndpointAddress & 0x0f) == UVC_STREAM2_EP1)
                fh_uvc_status_set(1, 0);
#endif
            break;
        case USB_REQ_SET_FEATURE:
            rt_usbd_set_feature(device, setup->wValue, setup->wIndex);
            break;
        default:
            rt_usbd_ep0_set_stall(device);
            rt_kprintf("STD:unknown other endpoint request\n");
            break;
        }
        break;
    case USB_REQ_TYPE_OTHER:
        rt_kprintf("STD:unknown other type request\n");
        rt_usbd_ep0_set_stall(device);
        break;
    default:
        rt_kprintf("STD:unknown type request: type=%x, req=%x, val=%x, index=%x, len=%x\n",
        setup->bRequestType,
        setup->bRequest,
        setup->wValue,
        setup->wIndex,
        setup->wLength);
        rt_usbd_ep0_set_stall(device);
        break;
    }

    return RT_EOK;
}

/**
 * This function will handle function bRequest.
 *
 * @param device the usb device object.
 * @param setup the setup bRequest.
 *
 * @return RT_EOK on successful, -RT_ERROR on invalid bRequest.
 */
static rt_err_t _function_request(udevice_t device, ureq_t setup)
{
    uep_t ep;
    ufunction_t f;
    int intface = 0;
    rt_uint8_t end_addr;
    int ret = -1;

    /* parameter check */
    RT_ASSERT(device != RT_NULL);
    RT_ASSERT(setup != RT_NULL);

    switch (setup->bRequestType & USB_REQ_TYPE_RECIPIENT_MASK)
    {
    case USB_REQ_TYPE_INTERFACE:

        intface = setup->wIndex & 0xFF;
        if (intface < MAX_CONFIG_INTERFACES)
        {
            f = device->curr_cfg->interface[intface];
            if (f && f->setup)
            {
                ret = f->setup(f, setup);
                if (ret < 0)
                    rt_usbd_ep0_set_stall(device);
            }
        }
        else
        {
            rt_kprintf("Func: unkwown interface request\n");
            rt_usbd_ep0_set_stall(device);
        }
        break;
    case USB_REQ_TYPE_ENDPOINT:
        end_addr =  setup->wIndex;
        ep = rt_usbd_find_endpoint(device, &f, end_addr);
        if (ep != RT_NULL)
        {
            f->setup(f, setup);
        }
        break;
    default:
        rt_kprintf("Func: unknown function request type\n");
        rt_usbd_ep0_set_stall(device);
        break;
    }

    return RT_EOK;
}

static rt_err_t _vendor_request(udevice_t device, ureq_t setup)
{
    static rt_uint8_t *usb_comp_id_desc = RT_NULL;
    static rt_uint32_t  usb_comp_id_desc_size = 0;
    usb_os_func_comp_id_desc_t func_comp_id_desc;

    rt_kprintf("_vendor_request\n");
    switch (setup->bRequest)
    {
    case 'A':
        switch (setup->wIndex)
        {
        case 0x04:
            if (rt_list_len(&device->os_comp_id_desc->func_desc) == 0)
            {
                rt_kprintf("_vendor_request\n");
                rt_usbd_ep0_set_stall(device);
                return RT_EOK;
            }
            if (usb_comp_id_desc == RT_NULL)
            {
                rt_uint8_t *pusb_comp_id_desc;
                rt_list_t *p;

                usb_comp_id_desc_size = sizeof(struct usb_os_header_comp_id_descriptor) +
                (sizeof(struct usb_os_function_comp_id_descriptor)-sizeof(rt_list_t))*
                rt_list_len(&device->os_comp_id_desc->func_desc);

                usb_comp_id_desc = (rt_uint8_t *)rt_malloc(usb_comp_id_desc_size);
                RT_ASSERT(usb_comp_id_desc != RT_NULL);
                device->os_comp_id_desc->head_desc.dwLength = usb_comp_id_desc_size;
                pusb_comp_id_desc = usb_comp_id_desc;
                rt_memcpy((void *)pusb_comp_id_desc,
                            (void *)&device->os_comp_id_desc->head_desc,
                            sizeof(struct usb_os_header_comp_id_descriptor));
                pusb_comp_id_desc += sizeof(struct usb_os_header_comp_id_descriptor);

                for (p = device->os_comp_id_desc->func_desc.next; p != &device->os_comp_id_desc->func_desc; p = p->next)
                {
                    func_comp_id_desc = rt_list_entry(p, struct usb_os_function_comp_id_descriptor, list);
                    rt_memcpy(pusb_comp_id_desc, (void *)&func_comp_id_desc->bFirstInterfaceNumber,
                                sizeof(struct usb_os_function_comp_id_descriptor)-sizeof(rt_list_t));
                    pusb_comp_id_desc += sizeof(struct usb_os_function_comp_id_descriptor)-sizeof(rt_list_t);
                }
            }
            rt_usbd_ep0_write(device, (void *)usb_comp_id_desc, setup->wLength);
            break;
        case 0x05:
            break;
        }
        break;
    }
    return RT_EOK;
}
static rt_err_t _dump_setup_packet(ureq_t setup)
{
    RT_DEBUG_LOG(RT_DEBUG_USB, ("[\n"));
    RT_DEBUG_LOG(RT_DEBUG_USB, ("  setup_request : 0x%x\n",
                                setup->bRequestType));
    RT_DEBUG_LOG(RT_DEBUG_USB, ("  value         : 0x%x\n", setup->wValue));
    RT_DEBUG_LOG(RT_DEBUG_USB, ("  length        : 0x%x\n", setup->wLength));
    RT_DEBUG_LOG(RT_DEBUG_USB, ("  index         : 0x%x\n", setup->wIndex));
    RT_DEBUG_LOG(RT_DEBUG_USB, ("  request       : 0x%x\n", setup->bRequest));
    RT_DEBUG_LOG(RT_DEBUG_USB, ("]\n"));

    return RT_EOK;
}

/**
 * This function will handle setup bRequest.
 *
 * @param device the usb device object.
 * @param setup the setup bRequest.
 *
 * @return RT_EOK on successful, -RT_ERROR on invalid bRequest.
 */
static rt_err_t _setup_request(udevice_t device, ureq_t setup)
{
    /* parameter check */
    RT_ASSERT(device != RT_NULL);
    RT_ASSERT(setup != RT_NULL);

    _dump_setup_packet(setup);

    switch ((setup->bRequestType & USB_REQ_TYPE_MASK))
    {
    case USB_REQ_TYPE_STANDARD:
        _standard_request(device, setup);
        break;
    case USB_REQ_TYPE_CLASS:
        _function_request(device, setup);
        break;
    case USB_REQ_TYPE_VENDOR:
        _vendor_request(device, setup);
        break;
    default:

        rt_usbd_ep0_set_stall(device);
        rt_kprintf("unknown setup request: type=%x, req=%x, val=%x, index=%x, len=%x\n",
        setup->bRequestType,
        setup->bRequest,
        setup->wValue,
        setup->wIndex,
        setup->wLength);
        break;
    }

    return RT_EOK;
}

/**
 * This function will hanle data notify event.
 *
 * @param device the usb device object.
 * @param ep_msg the endpoint message.
 *
 * @return RT_EOK.
 */
 /* need modify */

static rt_err_t _data_notify(udevice_t device, struct ep_msg *ep_msg)
{
    uep_t ep;
    ufunction_t func;
    int maxpacket;
    rt_size_t size = 0;

    RT_ASSERT(device != RT_NULL);
    RT_ASSERT(ep_msg != RT_NULL);

    if (device->state != USB_STATE_CONFIGURED)
    {
        return -RT_ERROR;
    }
    ep = rt_usbd_find_endpoint(device, &func, ep_msg->ep_addr);
    if (ep == RT_NULL)
    {
        rt_kprintf("invalid endpoint\n");
        return -RT_ERROR;
    }

    if (EP_ADDRESS(ep) & USB_DIR_IN)
    {
        size = ep_msg->size;

        maxpacket = EP_MAXPACKET(ep) * EP_MAXPACKET_CNT(ep);

        if (ep->request.remain_size > maxpacket)
        {
            dcd_ep_write(device->dcd, EP_ADDRESS(ep), ep->request.buffer, maxpacket);
            ep->request.remain_size -= maxpacket;
            ep->request.buffer += maxpacket;
        }
        else if (ep->request.remain_size > 0)
        {
            dcd_ep_write(device->dcd, EP_ADDRESS(ep), ep->request.buffer, ep->request.remain_size);
            ep->request.remain_size = 0;
        }
        else
        {
            EP_HANDLER(ep, func, size);
        }
    }
    else
    {
        size = ep_msg->size;
        if (ep->request.remain_size == 0)
        {
            return RT_EOK;
        }

        if (size == 0)
        {
            size = dcd_ep_read(device->dcd, EP_ADDRESS(ep), ep->request.buffer);
        }
        ep->request.remain_size -= size;
        ep->request.buffer += size;

        if (ep->request.req_type == UIO_REQUEST_READ_BEST)
        {
            EP_HANDLER(ep, func, size);
        }
        else if (ep->request.remain_size == 0)
        {
            EP_HANDLER(ep, func, ep->request.size);
        }
        else
        {
            dcd_ep_read_prepare(device->dcd,
                                EP_ADDRESS(ep),
                                ep->request.buffer,
                                ep->request.remain_size > EP_MAXPACKET(ep) ?
                                EP_MAXPACKET(ep) : ep->request.remain_size);
        }
    }

    return RT_EOK;
}

static rt_err_t _ep0_out_notify(udevice_t device, struct ep_msg *ep_msg)
{
    uep_t ep0;
    rt_size_t size;

    RT_ASSERT(device != RT_NULL);
    RT_ASSERT(ep_msg != RT_NULL);
    RT_ASSERT(device->dcd != RT_NULL);

    ep0 = &device->dcd->ep0;
    size = ep_msg->size;
    if (ep0->request.remain_size == 0)
    {
        return RT_EOK;
    }
    if (size == 0)
    {
        size = dcd_ep_read(device->dcd, EP0_OUT_ADDR, ep0->request.buffer);
        if (size == 0)
        {
            rt_kprintf("%s-%d warning: ep0 out data size = 0\n", __func__, __LINE__);
            // return RT_EOK;
        }
    }

    ep0->request.remain_size -= size;
    ep0->request.buffer += size;
    if (ep0->request.remain_size == 0)
    {
        /* invoke callback */
        if (ep0->rx_indicate != RT_NULL)
        {
            ep0->rx_indicate(device, size);
        }
    }
    else
    {
        rt_usbd_ep0_read(device, ep0->request.buffer, ep0->request.remain_size, ep0->rx_indicate);
    }

    return RT_EOK;
}

/**
 * This function will notity sof event to all of function.
 *
 * @param device the usb device object.
 *
 * @return RT_EOK.
 */
static rt_err_t _sof_notify(udevice_t device)
{
    struct rt_list_node *i;
    ufunction_t func;

    RT_ASSERT(device != RT_NULL);

    /* to notity every function that sof event comes */
    for (i = device->curr_cfg->func_list.next;
            i != &device->curr_cfg->func_list; i = i->next)
    {
        func = (ufunction_t)rt_list_entry(i, struct ufunction, list);
        if (func->sof_handler != RT_NULL)
            func->sof_handler(func);
    }

    return RT_EOK;
}

/**
 * This function will disable all USB functions.
 *
 * @param device the usb device object.
 *
 * @return RT_EOK.
 */
static rt_err_t _stop_notify(udevice_t device)
{
    struct rt_list_node *i;
    ufunction_t func;

    RT_ASSERT(device != RT_NULL);

    /* to notity every function */
    for (i  = device->curr_cfg->func_list.next;
         i != &device->curr_cfg->func_list;
         i  = i->next)
    {
        func = (ufunction_t)rt_list_entry(i, struct ufunction, list);
        FUNC_DISABLE(func);
    }

    return RT_EOK;
}

static rt_size_t rt_usbd_ep_write(udevice_t device, uep_t ep, void *buffer, rt_size_t size)
{
    rt_uint16_t maxpacket;
    rt_uint16_t last_packet;

    RT_ASSERT(device != RT_NULL);
    RT_ASSERT(device->dcd != RT_NULL);
    RT_ASSERT(ep != RT_NULL);
    /* level = rt_hw_interrupt_disable(); */
    rt_enter_critical();
    if (EP_TYPE(ep) == USB_EP_ATTR_BULK)
    {
        ep->request.remain_size = 0;
        dcd_ep_write(device->dcd, EP_ADDRESS(ep), buffer, size);
    } else
    {
        maxpacket = EP_MAXPACKET(ep) * EP_MAXPACKET_CNT(ep);

        if (ep->request.remain_size > maxpacket)
        {
            ep->request.remain_size -= maxpacket;
            ep->request.buffer += maxpacket;
            dcd_ep_write(device->dcd, EP_ADDRESS(ep), buffer, maxpacket);
        }
        else
        {
            last_packet = ep->request.remain_size;
            ep->request.remain_size = 0;
            dcd_ep_write(device->dcd, EP_ADDRESS(ep), ep->request.buffer,
                last_packet);
        }
    }

    rt_exit_critical();
    /* rt_hw_interrupt_enable(level); */
    return size;
}

static rt_size_t rt_usbd_ep_read_prepare(udevice_t device, uep_t ep, void *buffer, rt_size_t size)
{
    RT_ASSERT(device != RT_NULL);
    RT_ASSERT(device->dcd != RT_NULL);
    RT_ASSERT(ep != RT_NULL);
    RT_ASSERT(buffer != RT_NULL);
    RT_ASSERT(ep->ep_desc != RT_NULL);

    return dcd_ep_read_prepare(device->dcd, EP_ADDRESS(ep), buffer, size > EP_MAXPACKET(ep) ? EP_MAXPACKET(ep) : size);
}

/**
 * This function will create an usb device object.
 *
 * @param ustring the usb string array to contain string descriptor.
 *
 * @return an usb device object on success, RT_NULL on fail.
 */
udevice_t rt_usbd_device_new(void)
{
    udevice_t udevice;

    RT_DEBUG_LOG(RT_DEBUG_USB, ("rt_usbd_device_new\n"));

    /* allocate memory for the object */
    udevice = rt_malloc(sizeof(struct udevice));
    if (udevice == RT_NULL)
    {
        rt_kprintf("alloc memery failed\n");
        return RT_NULL;
    }
    rt_memset(udevice, 0, sizeof(struct udevice));

    /* to initialize configuration list */
    rt_list_init(&udevice->cfg_list);

    /* insert the device object to device list */
    rt_list_insert_before(&device_list, &udevice->list);

    return udevice;
}

/**
 * This function will set usb device string description.
 *
 * @param device the usb device object.
 * @param ustring pointer to string pointer array.
 *
 * @return RT_EOK.
 */
rt_err_t rt_usbd_device_set_string(udevice_t device, const char **ustring)
{
    /* parameter check */
    RT_ASSERT(device != RT_NULL);
    RT_ASSERT(ustring != RT_NULL);

    /* set string descriptor array to the device object */
    device->str = ustring;

    return RT_EOK;
}

rt_err_t rt_usbd_device_set_os_comp_id_desc(udevice_t device, usb_os_comp_id_desc_t os_comp_id_desc)
{
    /* parameter check */
    RT_ASSERT(device != RT_NULL);
    RT_ASSERT(os_comp_id_desc != RT_NULL);

    /* set string descriptor array to the device object */
    device->os_comp_id_desc = os_comp_id_desc;
    rt_list_init(&device->os_comp_id_desc->func_desc);
    return RT_EOK;
}

rt_err_t rt_usbd_device_set_qualifier(udevice_t device, struct usb_qualifier_descriptor *qualifier)
{
    /* parameter check */
    RT_ASSERT(device != RT_NULL);
    RT_ASSERT(qualifier != RT_NULL);

    device->dev_qualifier = qualifier;

    return RT_EOK;
}

/**
 * This function will set an usb controller driver to a device.
 *
 * @param device the usb device object.
 * @param dcd the usb device controller driver.
 *
 * @return RT_EOK on successful.
 */
rt_err_t rt_usbd_device_set_controller(udevice_t device, udcd_t dcd)
{
    /* parameter check */
    RT_ASSERT(device != RT_NULL);
    RT_ASSERT(dcd != RT_NULL);

    /* set usb device controller driver to the device */
    device->dcd = dcd;

    return RT_EOK;
}

/**
 * This function will set an usb device descriptor to a device.
 *
 * @param device the usb device object.
 * @param dev_desc the usb device descriptor.
 *
 * @return RT_EOK on successful.
 */
rt_err_t rt_usbd_device_set_descriptor(udevice_t device, udev_desc_t dev_desc)
{
    /* parameter check */
    RT_ASSERT(device != RT_NULL);
    RT_ASSERT(dev_desc != RT_NULL);

    /* copy the usb device descriptor to the device */
    rt_memcpy((void *)&device->dev_desc, (void *)dev_desc, USB_DESC_LENGTH_DEVICE);

    return RT_EOK;
}

/**
 * This function will create an usb configuration object.
 *
 * @param none.
 *
 * @return an usb configuration object.
 */
uconfig_t rt_usbd_config_new(void)
{
    uconfig_t cfg;

    RT_DEBUG_LOG(RT_DEBUG_USB, ("rt_usbd_config_new\n"));

    /* allocate memory for the object */
    cfg = rt_malloc(sizeof(struct uconfig));
    if (cfg == RT_NULL)
    {
        rt_kprintf("alloc memery failed\n");
        return RT_NULL;
    }
    rt_memset(cfg, 0, sizeof(struct uconfig));

    /* set default wValue */
    cfg->cfg_desc.bLength = USB_DESC_LENGTH_CONFIG;
    cfg->cfg_desc.type = USB_DESC_TYPE_CONFIGURATION;
    cfg->cfg_desc.wTotalLength = USB_DESC_LENGTH_CONFIG;
    cfg->cfg_desc.bmAttributes = 0xC0;
    cfg->cfg_desc.MaxPower = 0x01 * 100; /* 2mA*100 */

    cfg->other_cfg_desc.bLength = USB_DESC_LENGTH_CONFIG;
    cfg->other_cfg_desc.type = USB_DESC_TYPE_OTHERSPEED;
    cfg->other_cfg_desc.wTotalLength = USB_DESC_LENGTH_CONFIG;
    cfg->other_cfg_desc.bmAttributes = 0xC0;
    cfg->other_cfg_desc.MaxPower = 0x01 * 100; /* 2mA*100 */

    /* to initialize function object list */
    rt_list_init(&cfg->func_list);

    return cfg;
}


/**
 * This function will create an usb function object.
 *
 * @param device the usb device object.
 * @param dev_desc the device descriptor.
 * @param ops the operation set.
 *
 * @return an usb function object on success, RT_NULL on fail.
 */
ufunction_t rt_usbd_function_new(udevice_t device)
{
    ufunction_t func;

    RT_DEBUG_LOG(RT_DEBUG_USB, ("rt_usbd_function_new\n"));

    /* parameter check */
    RT_ASSERT(device != RT_NULL);
   /* RT_ASSERT(dev_desc != RT_NULL); */

    /* allocate memory for the object */
    func = (ufunction_t)rt_malloc(sizeof(struct ufunction));
    if (func == RT_NULL)
    {
        rt_kprintf("alloc memery failed\n");
        return RT_NULL;
    }

    rt_memset(func, 0, sizeof(struct ufunction));

/* func->dev_desc = dev_desc; */
/* func->ops = ops; */
    func->device = device;
    func->enabled = RT_FALSE;

    /* to initialize interface list */
    rt_list_init(&func->intf_list);

    return func;
}

/**
 * This function will create an usb endpoint object.
 *
 * @param ep_desc the endpoint descriptor.
 * @handler the callback handler of object
 *
 * @return an usb endpoint object on success, RT_NULL on fail.
 */
uep_t rt_usbd_endpoint_new(uep_desc_t ep_desc, udep_handler_t handler)
{
    uep_t ep;

    RT_DEBUG_LOG(RT_DEBUG_USB, ("rt_usbd_endpoint_new\n"));

    /* parameter check */
    RT_ASSERT(ep_desc != RT_NULL);

    /* allocate memory for the object */
    ep = (uep_t)rt_malloc(sizeof(struct uendpoint));
    if (ep == RT_NULL)
    {
        rt_kprintf("alloc memery failed\n");
        return RT_NULL;
    }
    rt_memset(ep, 0, sizeof(struct uendpoint));
    ep->ep_desc = ep_desc;
    ep->handler = handler;
    ep->buffer  = RT_NULL;
    ep->stalled = RT_FALSE;
    rt_list_init(&ep->request_list);

    return ep;
}

/**
 * This function will find an usb device object.
 *
 * @dcd usd device controller driver.
 *
 * @return an usb device object on found or RT_NULL on not found.
 */
udevice_t rt_usbd_find_device(udcd_t dcd)
{
    struct rt_list_node *node;
    udevice_t device;

    /* parameter check */
    RT_ASSERT(dcd != RT_NULL);

    /* search a device in the the device list */
    for (node = device_list.next; node != &device_list; node = node->next)
    {
        device = (udevice_t)rt_list_entry(node, struct udevice, list);
        if (device->dcd == dcd)
            return device;
    }

    rt_kprintf("can't find device\n");
    return RT_NULL;
}

/**
 * This function will find an usb configuration object.
 *
 * @param device the usb device object.
 * @param wValue the configuration number.
 *
 * @return an usb configuration object on found or RT_NULL on not found.
 */
uconfig_t rt_usbd_find_config(udevice_t device, rt_uint8_t value)
{
    struct rt_list_node *node;
    uconfig_t cfg = RT_NULL;

    RT_DEBUG_LOG(RT_DEBUG_USB, ("rt_usbd_find_config\n"));

    /* parameter check */
    RT_ASSERT(device != RT_NULL);
    RT_ASSERT(value <= device->dev_desc.bNumConfigurations);

    /* search a configration in the the device */
    for (node = device->cfg_list.next; node != &device->cfg_list; node = node->next)
    {
        cfg = (uconfig_t)rt_list_entry(node, struct udevice, list);
        if (cfg->cfg_desc.bConfigurationValue == value)
        {
            return cfg;
        }
    }

    rt_kprintf("can't find configuration %d\n", value);
    return RT_NULL;
}

/**
 * This function will find an usb endpoint object.
 *
 * @param device the usb device object.
 * @param ep_addr endpoint address.
 *
 * @return an usb endpoint object on found or RT_NULL on not found.
 */
uep_t rt_usbd_find_endpoint(udevice_t device, ufunction_t *pfunc, rt_uint8_t ep_addr)
{
    uep_t ep;
    struct rt_list_node *i;
    ufunction_t func;
    int idx = 0;

    /* parameter check */
    RT_ASSERT(device != RT_NULL);

    /* search a endpoint in the current configuration */
    for (i = device->curr_cfg->func_list.next; i != &device->curr_cfg->func_list; i = i->next)
    {
        func = (ufunction_t)rt_list_entry(i, struct ufunction, list);

        for (idx = 0; idx < func->next_ep_id; idx++)
        {
            ep = func->endpoints[idx];
            if (EP_ADDRESS(ep) == ep_addr)
            {
                if (pfunc != RT_NULL)
                    *pfunc = func;
                return ep;
            }
        }

    }

    rt_kprintf("can't find endpoint 0x%x\n", ep_addr);
    return RT_NULL;
}

uep_t usb_ep_autoconfig(ufunction_t f, uep_desc_t ep_desc, udep_handler_t handler)
{
    uep_t ep = rt_usbd_endpoint_new(ep_desc, handler);

    if (ep != RT_NULL && f->next_ep_id < MAX_CONFIG_EP)
    {
        ep->handler = handler;
        f->endpoints[f->next_ep_id] = ep;
        f->next_ep_id++;

        if (rt_usbd_ep_assign(f->device, ep) != RT_EOK)
        {
            rt_kprintf("usb_ep_autoconfig: endpoint assign error %d\n", f->next_ep_id);
        }
    }
    return  ep;
}


/**
 * This function will add a configuration to an usb device.
 *
 * @param device the usb device object.
 * @param cfg the configuration object.
 *
 * @return RT_EOK.
 */
/* need modify */
rt_err_t rt_usbd_device_add_config(udevice_t device, uconfig_t cfg)
{
    struct rt_list_node *i;
    ufunction_t func;

    RT_DEBUG_LOG(RT_DEBUG_USB, ("rt_usbd_device_add_config\n"));

    /* parameter check */
    RT_ASSERT(device != RT_NULL);
    RT_ASSERT(cfg != RT_NULL);

    /* set configuration number to the configuration descriptor */
    cfg->cfg_desc.bConfigurationValue = device->dev_desc.bNumConfigurations + 1;
    /* cfg->other_cfg_desc.bConfigurationValue = device->dev_desc.bNumConfigurations + 1; */
    device->dev_desc.bNumConfigurations++;

    for (i = cfg->func_list.next; i != &cfg->func_list; i = i->next)
    {
        func = (ufunction_t)rt_list_entry(i, struct ufunction, list);
        rt_memcpy((void *)&cfg->cfg_desc.data[cfg->cfg_desc.wTotalLength - USB_DESC_LENGTH_CONFIG],
                        func->descriptors,
                        func->desc_size);
        cfg->cfg_desc.wTotalLength += func->desc_size;
        /*
        rt_memcpy((void *)&cfg->other_cfg_desc.data[cfg->other_cfg_desc.wTotalLength - USB_DESC_LENGTH_CONFIG],
                        func->other_descriptors,
                        func->other_desc_size);
        cfg->other_cfg_desc.wTotalLength += func->other_desc_size; */

    }

    cfg->cfg_desc.bNumInterfaces = cfg->next_interface_id;
    /* cfg->other_cfg_desc.bNumInterfaces = cfg->next_interface_id; */

    /* insert the configuration to the list */
    rt_list_insert_before(&device->cfg_list, &cfg->list);

    return RT_EOK;
}

/**
 * This function will add a function to a configuration.
 *
 * @param cfg the configuration object.
 * @param func the function object.
 *
 * @return RT_EOK.
 */
rt_err_t rt_usbd_config_add_function(uconfig_t cfg, ufunction_t func)
{
    RT_DEBUG_LOG(RT_DEBUG_USB, ("rt_usbd_config_add_function\n"));

    /* parameter check */
    RT_ASSERT(cfg != RT_NULL);
    RT_ASSERT(func != RT_NULL);

    /* insert the function to the list */
    rt_list_insert_before(&cfg->func_list, &func->list);

    return RT_EOK;
}


rt_err_t rt_usbd_os_comp_id_desc_add_os_func_comp_id_desc(usb_os_comp_id_desc_t os_comp_id_desc,
            usb_os_func_comp_id_desc_t os_func_comp_id_desc)
{
    RT_ASSERT(os_comp_id_desc != RT_NULL);
    RT_ASSERT(os_func_comp_id_desc != RT_NULL);
    rt_list_insert_before(&os_comp_id_desc->func_desc, &os_func_comp_id_desc->list);
    os_comp_id_desc->head_desc.bCount++;
    return RT_EOK;
}


/**
 * This function will set a configuration for an usb device.
 *
 * @param device the usb device object.
 * @param wValue the configuration number.
 *
 * @return RT_EOK.
 */
rt_err_t rt_usbd_set_config(udevice_t device, rt_uint8_t value)
{
    uconfig_t cfg;

    RT_DEBUG_LOG(RT_DEBUG_USB, ("rt_usbd_set_config\n"));

    /* parameter check */
    RT_ASSERT(device != RT_NULL);
    RT_ASSERT(value <= device->dev_desc.bNumConfigurations);

    /* find a configuration */
    cfg = rt_usbd_find_config(device, value);

    /* set as current configuration */
    device->curr_cfg = cfg;

    dcd_set_config(device->dcd, value);

    return RT_TRUE;
}

/**
 * This function will bRequest an IO transaction.
 *
 * @param device the usb device object.
 * @param ep the endpoint object.
 * @param req IO bRequest.
 *
 * @return RT_EOK.
 */
rt_size_t rt_usbd_io_request(udevice_t device, uep_t ep, uio_request_t req)
{
    rt_size_t size = 0;

    RT_ASSERT(device != RT_NULL);
    RT_ASSERT(req != RT_NULL);
    if (ep->stalled == RT_FALSE)
    {
        switch (req->req_type)
        {
        case UIO_REQUEST_READ_BEST:
        case UIO_REQUEST_READ_FULL:
            ep->request.remain_size = ep->request.size;
            size = rt_usbd_ep_read_prepare(device, ep, req->buffer, req->size);
            break;
        case UIO_REQUEST_WRITE:
            ep->request.remain_size = ep->request.size;
            size = rt_usbd_ep_write(device, ep, req->buffer, req->size);
            break;
        default:
            rt_kprintf("unknown request type\n");
            break;
        }
    }
    else
    {
        rt_list_insert_before(&ep->request_list, &req->list);
        RT_DEBUG_LOG(RT_DEBUG_USB, ("suspend a request\n"));
    }

    return size;
}

/**
 * This function will set feature for an usb device.
 *
 * @param device the usb device object.
 * @param wValue the configuration number.
 *
 * @return RT_EOK.
 */
rt_err_t rt_usbd_set_feature(udevice_t device, rt_uint16_t value, rt_uint16_t index)
{
    RT_ASSERT(device != RT_NULL);

    if (value == USB_FEATURE_DEV_REMOTE_WAKEUP)
    {
        RT_DEBUG_LOG(RT_DEBUG_USB, ("set feature remote wakeup\n"));
    }
    else if (value == USB_FEATURE_ENDPOINT_HALT)
    {
        RT_DEBUG_LOG(RT_DEBUG_USB, ("set feature stall\n"));
        dcd_ep_set_stall(device->dcd, (rt_uint32_t)(index & 0xFF));
    }
    else if (value == USB_FEATURE_TEST_MODE)
    {
        RT_DEBUG_LOG(RT_DEBUG_USB, ("set test mode\n"));
        dcd_set_test_mode(device->dcd, index>>8);
        dcd_ep0_send_status(device->dcd);
    }

    return RT_EOK;
}

/**
 * This function will clear feature for an usb device.
 *
 * @param device the usb device object.
 * @param wValue the configuration number.
 *
 * @return RT_EOK.
 */
rt_err_t rt_usbd_clear_feature(udevice_t device, rt_uint16_t value, rt_uint16_t index)
{
    RT_ASSERT(device != RT_NULL);

    if (value == USB_FEATURE_DEV_REMOTE_WAKEUP)
    {
        RT_DEBUG_LOG(RT_DEBUG_USB, ("clear feature remote wakeup\n"));
    }
    else if (value == USB_FEATURE_ENDPOINT_HALT)
    {
        RT_DEBUG_LOG(RT_DEBUG_USB, ("clear feature stall\n"));
        dcd_ep_clear_stall(device->dcd, (rt_uint32_t)(index & 0xFF));
    }

    return RT_EOK;
}

rt_err_t rt_usbd_ep0_set_stall(udevice_t device)
{
    RT_ASSERT(device != RT_NULL);

    return dcd_ep_set_stall(device->dcd, 0x80);
}

rt_err_t rt_usbd_ep0_clear_stall(udevice_t device)
{
    RT_ASSERT(device != RT_NULL);

    return dcd_ep_clear_stall(device->dcd, 0);
}

rt_err_t rt_usbd_ep_set_stall(udevice_t device, uep_t ep)
{
    rt_err_t ret;

    RT_ASSERT(device != RT_NULL);
    RT_ASSERT(ep != RT_NULL);
    RT_ASSERT(ep->ep_desc != RT_NULL);

    ret = dcd_ep_set_stall(device->dcd, EP_ADDRESS(ep));
    if (ret == RT_EOK)
    {
        ep->stalled = RT_TRUE;
    }

    return ret;
}

rt_err_t rt_usbd_ep_clear_stall(udevice_t device, uep_t ep)
{
    rt_err_t ret;

    RT_ASSERT(device != RT_NULL);
    RT_ASSERT(ep != RT_NULL);
    RT_ASSERT(ep->ep_desc != RT_NULL);

    ret = dcd_ep_clear_stall(device->dcd, EP_ADDRESS(ep));
    if (ret == RT_EOK)
    {
        ep->stalled = RT_FALSE;
    }

    return ret;
}

static rt_err_t rt_usbd_ep_assign(udevice_t device, uep_t ep)
{
    int i = 0;

    RT_ASSERT(device != RT_NULL);
    RT_ASSERT(device->dcd != RT_NULL);
    RT_ASSERT(device->dcd->ep_pool != RT_NULL);
    RT_ASSERT(ep != RT_NULL);
    RT_ASSERT(ep->ep_desc != RT_NULL);

    while (device->dcd->ep_pool[i].addr != 0xFF)
    {
        if (device->dcd->ep_pool[i].status == ID_UNASSIGNED &&
            (ep->ep_desc->bmAttributes & USB_EP_ATTR_TYPE_MASK) == device->dcd->ep_pool[i].type &&
            (EP_ADDRESS(ep) & 0x80) == device->dcd->ep_pool[i].dir)
            /* && (EP_ADDRESS(ep) & 0x0f) == device->dcd->ep_pool[i].addr */
        {
            EP_ADDRESS(ep) |= device->dcd->ep_pool[i].addr;
            ep->id = &device->dcd->ep_pool[i];
            device->dcd->ep_pool[i].status = ID_ASSIGNED;

            return RT_EOK;
        }

        i++;
    }

    return -RT_ERROR;
}

rt_err_t rt_usbd_ep_unassign(udevice_t device, uep_t ep)
{
    RT_ASSERT(device != RT_NULL);
    RT_ASSERT(device->dcd != RT_NULL);
    RT_ASSERT(device->dcd->ep_pool != RT_NULL);
    RT_ASSERT(ep != RT_NULL);
    RT_ASSERT(ep->ep_desc != RT_NULL);

    ep->id->status = ID_UNASSIGNED;

    return RT_EOK;
}

rt_err_t rt_usbd_ep0_setup_handler(udcd_t dcd, struct urequest *setup)
{
    struct udev_msg msg;
    rt_size_t size;

    RT_ASSERT(dcd != RT_NULL);

    if (setup == RT_NULL)
    {
        size = dcd_ep_read(dcd, EP0_OUT_ADDR, (void *)&msg.content.setup);
        if (size != sizeof(struct urequest))
        {
            rt_kprintf("read setup packet error\n");
            return -RT_ERROR;
        }
    }
    else
    {
        rt_memcpy((void *)&msg.content.setup, (void *)setup, sizeof(struct urequest));
    }

    msg.type = USB_MSG_SETUP_NOTIFY;
    msg.dcd = dcd;
    /* rt_usbd_event_signal(&msg); */
    rt_usbd_thread_entry(&msg);

    return RT_EOK;
}

rt_err_t rt_usbd_ep0_in_handler(udcd_t dcd)
{
    rt_int32_t remain, mps;

    RT_ASSERT(dcd != RT_NULL);

    if (dcd->stage != STAGE_DIN)
        return RT_EOK;

    mps = dcd->ep0.id->maxpacket;
    dcd->ep0.request.remain_size -= mps;
    remain = dcd->ep0.request.remain_size;

    if (remain > 0)
    {
        if (remain >= mps)
        {
            remain = mps;
        }

        dcd->ep0.request.buffer += mps;
        dcd_ep_write(dcd, EP0_IN_ADDR, dcd->ep0.request.buffer, remain);
    }
    else
    {
        /* last packet is MPS multiple, so send ZLP packet */
        if ((remain == 0) && (dcd->ep0.request.size > 0))
        {
            dcd->ep0.request.size = 0;
            dcd_ep_write(dcd, EP0_IN_ADDR, RT_NULL, 0);
        }
        else
        {
            /* receive status */
            dcd->stage = STAGE_STATUS_OUT;
            dcd_ep_read_prepare(dcd, EP0_OUT_ADDR, RT_NULL, 0);
        }
    }

    return RT_EOK;
}

rt_err_t  rt_usbd_ep0_out_handler(udcd_t dcd, rt_size_t size)
{
    struct udev_msg msg;

    RT_ASSERT(dcd != RT_NULL);

    msg.type = USB_MSG_EP0_OUT;
    msg.dcd = dcd;
    msg.content.ep_msg.size = size;
    /* rt_usbd_event_signal(&msg); */
    rt_usbd_thread_entry(&msg);

    return RT_EOK;
}

rt_err_t rt_usbd_ep_in_handler(udcd_t dcd, rt_uint8_t address, rt_size_t size)
{
    struct udev_msg msg;

    RT_ASSERT(dcd != RT_NULL);

    msg.type = USB_MSG_DATA_NOTIFY;
    msg.dcd = dcd;
    msg.content.ep_msg.ep_addr = address;
    msg.content.ep_msg.size = size;
    /* rt_usbd_event_signal(&msg); */
    rt_usbd_thread_entry(&msg);

    return RT_EOK;
}

rt_err_t rt_usbd_ep_out_handler(udcd_t dcd, rt_uint8_t address, rt_size_t size)
{
    struct udev_msg msg;

    RT_ASSERT(dcd != RT_NULL);

    msg.type = USB_MSG_DATA_NOTIFY;
    msg.dcd = dcd;
    msg.content.ep_msg.ep_addr = address;
    msg.content.ep_msg.size = size;
    /* rt_usbd_event_signal(&msg); */
    rt_usbd_thread_entry(&msg);

    return RT_EOK;
}

rt_err_t rt_usbd_reset_handler(udcd_t dcd)
{
    struct udev_msg msg;

    RT_ASSERT(dcd != RT_NULL);

    msg.type = USB_MSG_RESET;
    msg.dcd = dcd;
    /* rt_usbd_event_signal(&msg); */
    rt_usbd_thread_entry(&msg);

    return RT_EOK;
}

rt_err_t rt_usbd_connect_handler(udcd_t dcd)
{
    struct udev_msg msg;

    RT_ASSERT(dcd != RT_NULL);

    msg.type = USB_MSG_PLUG_IN;
    msg.dcd = dcd;
    /* rt_usbd_event_signal(&msg); */
    rt_usbd_thread_entry(&msg);

    return RT_EOK;
}

rt_err_t rt_usbd_disconnect_handler(udcd_t dcd)
{
    struct udev_msg msg;

    RT_ASSERT(dcd != RT_NULL);

    msg.type = USB_MSG_PLUG_OUT;
    msg.dcd = dcd;
    /* rt_usbd_event_signal(&msg); */
    rt_usbd_thread_entry(&msg);

    return RT_EOK;
}

rt_err_t rt_usbd_sof_handler(udcd_t dcd)
{
    struct udev_msg msg;

    RT_ASSERT(dcd != RT_NULL);

    msg.type = USB_MSG_SOF;
    msg.dcd = dcd;
    /* rt_usbd_event_signal(&msg); */
    rt_usbd_thread_entry(&msg);

    return RT_EOK;
}

rt_size_t rt_usbd_ep0_write(udevice_t device, void *buffer, rt_size_t size)
{
    uep_t ep0;

    RT_ASSERT(device != RT_NULL);
    RT_ASSERT(device->dcd != RT_NULL);
    RT_ASSERT(buffer != RT_NULL);
    RT_ASSERT(size > 0);

    ep0 = &device->dcd->ep0;
    ep0->request.size = size;
    ep0->request.buffer = buffer;
    ep0->request.remain_size = size;
    device->dcd->stage = STAGE_DIN;

    return dcd_ep_write(device->dcd, EP0_IN_ADDR, ep0->request.buffer, size);
}

rt_size_t rt_usbd_ep0_read(udevice_t device, void *buffer, rt_size_t size,
    rt_err_t (*rx_ind)(udevice_t device, rt_size_t size))
{
    uep_t ep0;
    rt_size_t read_size = 0;

    RT_ASSERT(device != RT_NULL);
    RT_ASSERT(device->dcd != RT_NULL);
    RT_ASSERT(buffer != RT_NULL);

    ep0 = &device->dcd->ep0;
    ep0->request.buffer = buffer;
    ep0->request.remain_size = size;
    ep0->rx_indicate = rx_ind;
    if (size >= ep0->id->maxpacket)
    {
        read_size = ep0->id->maxpacket;
    }
    else
    {
        read_size = size;
    }
    device->dcd->stage = STAGE_DOUT;
    dcd_ep_read_prepare(device->dcd, EP0_OUT_ADDR, buffer, read_size);

    return size;
}


int usb_string_id(uconfig_t cfg, LPCSTR string)
{
    if (cfg->next_string_id < 254)
    {
        /* string id 0 is reserved by USB spec for list of*/
        /* supported languages */
        /* 255 reserved as well? -- mina86 */
        cfg->next_string_id++;
        if (cfg->next_string_id >= MAX_CONFIG_STRING)
            rt_kprintf("%s--%d next_string_id too large!\n", __func__, __LINE__);
        cfg->strings[cfg->next_string_id] = string;
        return cfg->next_string_id;
    }
    return -ENODEV;
}




int usb_interface_id(uconfig_t config, ufunction_t function)
{
    unsigned int id = config->next_interface_id;

    if (id < MAX_CONFIG_INTERFACES)
    {
        config->interface[id] = function;
        config->next_interface_id = id + 1;
        return id;
    }
    return -ENODEV;
}


/**
 * This function is the main entry of usb device thread, it is in charge of
 * processing all messages received from the usb message buffer.
 *
 * @param parameter the parameter of the usb device thread.
 *
 * @return none.
 */
static void rt_usbd_thread_entry(struct udev_msg *msg)
{
    /* while(1) */
    {
        udevice_t device;

        device = rt_usbd_find_device(msg->dcd);
        if (device == RT_NULL)
        {
            rt_kprintf("invalid usb device\n");
            return;
            /* continue; */
        }

        switch (msg->type)
        {
        case USB_MSG_SOF:
            _sof_notify(device);
            break;
        case USB_MSG_DATA_NOTIFY:
            /* some buggy drivers will have USB_MSG_DATA_NOTIFY before the core*/
            /* got configured. */
            _data_notify(device, &msg->content.ep_msg);
            break;
        case USB_MSG_SETUP_NOTIFY:
            _setup_request(device, &msg->content.setup);
            break;
        case USB_MSG_EP0_OUT:
            _ep0_out_notify(device, &msg->content.ep_msg);
            break;
        case USB_MSG_RESET:
            RT_DEBUG_LOG(RT_DEBUG_USB, ("reset %d\n", device->state));
            if (device->state == USB_STATE_ADDRESS || device->state == USB_STATE_CONFIGURED)
                _stop_notify(device);
            device->state = USB_STATE_NOTATTACHED;
            break;
        case USB_MSG_PLUG_IN:
            device->state = USB_STATE_ATTACHED;
            break;
        case USB_MSG_PLUG_OUT:
            device->state = USB_STATE_NOTATTACHED;
            _stop_notify(device);
            break;
        default:
            rt_kprintf("unknown msg type %d\n", msg->type);
            break;
        }
    }
}

/**
 * This function will post an message to usb message queue,
 *
 * @param msg the message to be posted
 * @param size the size of the message .
 *
 * @return the error code, RT_EOK on successfully.
 */
rt_err_t rt_usbd_event_signal(struct udev_msg *msg)
{
    RT_ASSERT(msg != RT_NULL);

    rt_usbd_thread_entry(msg);

    /* send message to usb message queue */
    return 0;
}

#define USBD_MQ_MSG_SZ  128 /* 32 */
#define USBD_MQ_MAX_MSG 16

/* internal of the message queue: every message is associated with a pointer,*/
/* so in order to recveive USBD_MQ_MAX_MSG messages, we have to allocate more*/
/* than USBD_MQ_MSG_SZ*USBD_MQ_MAX_MSG memery. */

/**
 * This function will initialize usb device thread.
 *
 * @return none.
 *
 */
rt_err_t rt_usbd_core_init(void)
{
    rt_list_init(&device_list);
    return 0;
}

