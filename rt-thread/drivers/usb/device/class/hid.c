/*
 * File      : hid.c
 * COPYRIGHT (C) 2008 - 2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2017-03-13     Urey         the first version
 * 2017-11-16     ZYH          Update to common hid
 */

#include <rthw.h>
#include <rtthread.h>
#include <rtservice.h>
#include <rtdevice.h>
#include "../core/usbdevice.h"
#include "../core/usbcommon.h"
#include "../udc/udc_otg_dwc.h"

#include "hid.h"

#define HID_STR_ASSOCIATION "fullhan hid"

static struct hid_s  *g_hiddev = NULL;
static rt_uint8_t hid_mq_pool[(sizeof(struct hid_report)+sizeof(void*))*32];
struct hid_s
{
    struct rt_device parent;
    struct ufunction *func;
    uep_t ep_in;
    uep_t ep_out;
    int status;
    rt_uint16_t protocol;
    rt_uint8_t report_buf[MAX_REPORT_SIZE];
    struct rt_messagequeue hid_mq;
    struct rt_completion completion;
    struct rt_mutex       tx_mutex;
    struct rt_mutex       rx_mutex;
    rt_uint32_t report;
};

/* CustomHID_ConfigDescriptor */

#if 1
const rt_uint8_t _report_desc[] = {
#ifdef RT_USB_DEVICE_HID_KEYBOARD
    USAGE_PAGE(1),      0x01,
    USAGE(1),           0x06,
    COLLECTION(1),      0x01,
    REPORT_ID(1),       HID_REPORT_ID_KEYBOARD1,

    USAGE_PAGE(1),      0x07,
    USAGE_MINIMUM(1),   0xE0,
    USAGE_MAXIMUM(1),   0xE7,
    LOGICAL_MINIMUM(1), 0x00,
    LOGICAL_MAXIMUM(1), 0x01,
    REPORT_SIZE(1),     0x01,
    REPORT_COUNT(1),    0x08,
    INPUT(1),           0x02,
    REPORT_COUNT(1),    0x01,
    REPORT_SIZE(1),     0x08,
    INPUT(1),           0x01,


    REPORT_COUNT(1),    0x05,
    REPORT_SIZE(1),     0x01,
    USAGE_PAGE(1),      0x08,
    USAGE_MINIMUM(1),   0x01,
    USAGE_MAXIMUM(1),   0x05,
    OUTPUT(1),          0x02,
    REPORT_COUNT(1),    0x01,
    REPORT_SIZE(1),     0x03,
    OUTPUT(1),          0x01,


    REPORT_COUNT(1),    0x06,
    REPORT_SIZE(1),     0x08,
    LOGICAL_MINIMUM(1), 0x00,
    LOGICAL_MAXIMUM(1), 0x65,
    USAGE_PAGE(1),      0x07,
    USAGE_MINIMUM(1),   0x00,
    USAGE_MAXIMUM(1),   0x65,
    INPUT(1),           0x00,
    END_COLLECTION(0),
#if RT_USB_DEVICE_HID_KEYBOARD_NUMBER>1
    /****keyboard2*****/
    USAGE_PAGE(1),      0x01,
    USAGE(1),           0x06,
    COLLECTION(1),      0x01,
    REPORT_ID(1),       HID_REPORT_ID_KEYBOARD2,

    USAGE_PAGE(1),      0x07,
    USAGE_MINIMUM(1),   0xE0,
    USAGE_MAXIMUM(1),   0xE7,
    LOGICAL_MINIMUM(1), 0x00,
    LOGICAL_MAXIMUM(1), 0x01,
    REPORT_SIZE(1),     0x01,
    REPORT_COUNT(1),    0x08,
    INPUT(1),           0x02,
    REPORT_COUNT(1),    0x01,
    REPORT_SIZE(1),     0x08,
    INPUT(1),           0x01,

    REPORT_COUNT(1),    0x06,
    REPORT_SIZE(1),     0x08,
    LOGICAL_MINIMUM(1), 0x00,
    LOGICAL_MAXIMUM(1), 0x65,
    USAGE_PAGE(1),      0x07,
    USAGE_MINIMUM(1),   0x00,
    USAGE_MAXIMUM(1),   0x65,
    INPUT(1),           0x00,
    END_COLLECTION(0),
#if RT_USB_DEVICE_HID_KEYBOARD_NUMBER>2
    USAGE_PAGE(1),      0x01,
    USAGE(1),           0x06,
    COLLECTION(1),      0x01,
    REPORT_ID(1),       HID_REPORT_ID_KEYBOARD3,

    USAGE_PAGE(1),      0x07,
    USAGE_MINIMUM(1),   0xE0,
    USAGE_MAXIMUM(1),   0xE7,
    LOGICAL_MINIMUM(1), 0x00,
    LOGICAL_MAXIMUM(1), 0x01,
    REPORT_SIZE(1),     0x01,
    REPORT_COUNT(1),    0x08,
    INPUT(1),           0x02,
    REPORT_COUNT(1),    0x01,
    REPORT_SIZE(1),     0x08,
    INPUT(1),           0x01,

    REPORT_COUNT(1),    0x06,
    REPORT_SIZE(1),     0x08,
    LOGICAL_MINIMUM(1), 0x00,
    LOGICAL_MAXIMUM(1), 0x65,
    USAGE_PAGE(1),      0x07,
    USAGE_MINIMUM(1),   0x00,
    USAGE_MAXIMUM(1),   0x65,
    INPUT(1),           0x00,
    END_COLLECTION(0),
#endif
#endif
#endif
#ifdef RT_USB_DEVICE_HID_KEYBOARD_CONSUMER
    USAGE_PAGE(1),      0x0c,
    USAGE(1),           0x01,
    COLLECTION(1),      0x01,
    REPORT_ID(1),       HID_REPORT_ID_KEYBOARD4,

    REPORT_COUNT(1),    0x01,
    REPORT_SIZE(1),     0x10,
    LOGICAL_MINIMUM(1), 0x00,
    LOGICAL_MAXIMUM(2), 0x3c,0x02,
    USAGE_MINIMUM(1),   0x00,
    USAGE_MAXIMUM(2),   0x3c,0x02,
    INPUT(1),           0x00,
    REPORT_COUNT(1),    0x06,
    REPORT_SIZE(1),     0x08,
    INPUT(1),           0x01,
    END_COLLECTION(0),
#endif
    // Media Control
#ifdef RT_USB_DEVICE_HID_MEDIA
    USAGE_PAGE(1),      0x0C,
    USAGE(1),           0x01,
    COLLECTION(1),      0x01,
    REPORT_ID(1),       HID_REPORT_ID_MEDIA,
    USAGE_PAGE(1),      0x0C,
    LOGICAL_MINIMUM(1), 0x00,
    LOGICAL_MAXIMUM(1), 0x01,
    REPORT_SIZE(1),     0x01,
    REPORT_COUNT(1),    0x07,
    USAGE(1),           0xB5,             // Next Track
    USAGE(1),           0xB6,             // Previous Track
    USAGE(1),           0xB7,             // Stop
    USAGE(1),           0xCD,             // Play / Pause
    USAGE(1),           0xE2,             // Mute
    USAGE(1),           0xE9,             // Volume Up
    USAGE(1),           0xEA,             // Volume Down
    INPUT(1),           0x02,             // Input (Data, Variable, Absolute)
    REPORT_COUNT(1),    0x01,
    INPUT(1),           0x01,
    END_COLLECTION(0),
#endif

#ifdef RT_USB_DEVICE_HID_GENERAL
    USAGE_PAGE(1),      0x8c,
    USAGE(1),           0x01,
    COLLECTION(1),      0x01,
    REPORT_ID(1),       HID_REPORT_ID_GENERAL,

    REPORT_COUNT(1),    RT_USB_DEVICE_HID_GENERAL_IN_REPORT_LENGTH,
    USAGE(1),           0x03,
    REPORT_SIZE(1),     0x08,
    LOGICAL_MINIMUM(1), 0x00,
    LOGICAL_MAXIMUM(1), 0xFF,
    INPUT(1),           0x02,

    REPORT_COUNT(1),    RT_USB_DEVICE_HID_GENERAL_OUT_REPORT_LENGTH,
    USAGE(1),           0x04,
    REPORT_SIZE(1),     0x08,
    LOGICAL_MINIMUM(1), 0x00,
    LOGICAL_MAXIMUM(1), 0xFF,
    OUTPUT(1),          0x02,
    END_COLLECTION(0),
#endif
#ifdef RT_USB_DEVICE_HID_TELEPHONY

    USAGE_PAGE(1),      0x0B,
    USAGE(1),           0x05,
    COLLECTION(1),      0x01,
    REPORT_ID(1),       HID_REPORT_ID_TELEPHONY,
    LOGICAL_MINIMUM(1), 0x00,
    LOGICAL_MAXIMUM(1), 0x01,
    REPORT_COUNT(1),    0x01,
    REPORT_SIZE(1),     0x01,
    USAGE(1),           0x20,
    INPUT(1),           0x22,
    USAGE(1),           0x2f,
    INPUT(1),           0x06,
    USAGE(1),           0x21,
    INPUT(1),           0x02,
    USAGE(1),           0x24,
    INPUT(1),           0x06,
    USAGE(1),           0x07,
    USAGE_PAGE(1),      0x09,
    USAGE(1),           0x01,
    INPUT(1),           0x06,
    REPORT_SIZE(1),     0x03,
    INPUT(1),           0x01,

    USAGE_PAGE(1),      0x08,
    REPORT_COUNT(1),    0x01,
    REPORT_SIZE(1),     0x01,
    USAGE(1),           0x18,
    OUTPUT(1),          0x22,

    USAGE(1),           0x1e,
    OUTPUT(1),          0x22,

    USAGE(1),           0x09,
    OUTPUT(1),          0x22,

    USAGE(1),           0x17,
    OUTPUT(1),          0x22,

    USAGE(1),           0x20,
    OUTPUT(1),          0x22,
    REPORT_SIZE(1),     0x03,
    OUTPUT(1),          0x01,
    END_COLLECTION(0),
#endif
#ifdef RT_USB_DEVICE_HID_MOUSE
    USAGE_PAGE(1),      0x01,           // Generic Desktop
    USAGE(1),           0x02,           // Mouse
    COLLECTION(1),      0x01,           // Application
    USAGE(1),           0x01,           // Pointer
    COLLECTION(1),      0x00,           // Physical
    REPORT_ID(1),       HID_REPORT_ID_MOUSE,
    REPORT_COUNT(1),    0x03,
    REPORT_SIZE(1),     0x01,
    USAGE_PAGE(1),      0x09,           // Buttons
    USAGE_MINIMUM(1),   0x1,
    USAGE_MAXIMUM(1),   0x3,
    LOGICAL_MINIMUM(1), 0x00,
    LOGICAL_MAXIMUM(1), 0x01,
    INPUT(1),           0x02,
    REPORT_COUNT(1),    0x01,
    REPORT_SIZE(1),     0x05,
    INPUT(1),           0x01,
    REPORT_COUNT(1),    0x03,
    REPORT_SIZE(1),     0x08,
    USAGE_PAGE(1),      0x01,
    USAGE(1),           0x30,           // X
    USAGE(1),           0x31,           // Y
    USAGE(1),           0x38,           // scroll
    LOGICAL_MINIMUM(1), 0x81,
    LOGICAL_MAXIMUM(1), 0x7f,
    INPUT(1),           0x06,
    END_COLLECTION(0),
    END_COLLECTION(0),
#endif
}; /* CustomHID_ReportDescriptor */
#endif

/* hid interface descriptor */
ALIGN(4)
static struct uhid_comm_descriptor _hid_comm_desc =
{
#ifdef FH_RT_USB_DEVICE_COMPOSITE
    /* Interface Association Descriptor */
    {
        USB_DESC_LENGTH_IAD,
        USB_DESC_TYPE_IAD,
        USB_DYNAMIC,
        0x01,
        0x03,                       /* bInterfaceClass: HID */
#if defined(RT_USB_DEVICE_HID_KEYBOARD)||defined(RT_USB_DEVICE_HID_MOUSE)
        USB_HID_SUBCLASS_BOOT,    /* bInterfaceSubClass : 1=BOOT, 0=no boot */
#else
        USB_HID_SUBCLASS_NOBOOT,    /* bInterfaceSubClass : 1=BOOT, 0=no boot */
#endif
#if !defined(RT_USB_DEVICE_HID_KEYBOARD) && !defined(RT_USB_DEVICE_HID_MOUSE) && !defined(RT_USB_DEVICE_HID_MEDIA)
        USB_HID_PROTOCOL_NONE,      /* nInterfaceProtocol : 0=none, 1=keyboard, 2=mouse */
#elif !defined(RT_USB_DEVICE_HID_MOUSE)
        USB_HID_PROTOCOL_KEYBOARD,  /* nInterfaceProtocol : 0=none, 1=keyboard, 2=mouse */
#else
        USB_HID_PROTOCOL_MOUSE,     /* nInterfaceProtocol : 0=none, 1=keyboard, 2=mouse */
#endif
        0x00,
    },
#endif
    /* Interface Descriptor */
    {
        USB_DESC_LENGTH_INTERFACE,
        USB_DESC_TYPE_INTERFACE,
        USB_DYNAMIC,                /* bInterfaceNumber: Number of Interface */
        0x00,                       /* bAlternateSetting: Alternate setting */
        0x02,                       /* bNumEndpoints */
        0x03,                       /* bInterfaceClass: HID */
#if defined(RT_USB_DEVICE_HID_KEYBOARD)||defined(RT_USB_DEVICE_HID_MOUSE)
        USB_HID_SUBCLASS_BOOT,    /* bInterfaceSubClass : 1=BOOT, 0=no boot */
#else
        USB_HID_SUBCLASS_NOBOOT,    /* bInterfaceSubClass : 1=BOOT, 0=no boot */
#endif
#if !defined(RT_USB_DEVICE_HID_KEYBOARD) && !defined(RT_USB_DEVICE_HID_MOUSE) && !defined(RT_USB_DEVICE_HID_MEDIA)
        USB_HID_PROTOCOL_NONE,      /* nInterfaceProtocol : 0=none, 1=keyboard, 2=mouse */
#elif !defined(RT_USB_DEVICE_HID_MOUSE)
        USB_HID_PROTOCOL_KEYBOARD,  /* nInterfaceProtocol : 0=none, 1=keyboard, 2=mouse */
#else
        USB_HID_PROTOCOL_MOUSE,     /* nInterfaceProtocol : 0=none, 1=keyboard, 2=mouse */
#endif
        0,                          /* iInterface: Index of string descriptor */
    },

    /* HID Descriptor */
    {
        HID_DESCRIPTOR_SIZE,        /* bLength: HID Descriptor size */
        HID_DESCRIPTOR_TYPE,        /* bDescriptorType: HID */
        0x0110,                     /* bcdHID: HID Class Spec release number */
        0x00,                       /* bCountryCode: Hardware target country */
        0x01,                       /* bNumDescriptors: Number of HID class descriptors to follow */
        {
            {
                0x22,                       /* bDescriptorType */
                sizeof(_report_desc),       /* wItemLength: Total length of Report descriptor */
            },
        },
    },

    /* Endpoint Descriptor IN */
    {
        USB_DESC_LENGTH_ENDPOINT,
        USB_DESC_TYPE_ENDPOINT,
        USB_DYNAMIC | USB_DIR_IN,
        USB_EP_ATTR_INT,
        0x40,
        0x03,
    },

    /* Endpoint Descriptor OUT */
    {
        USB_DESC_LENGTH_ENDPOINT,
        USB_DESC_TYPE_ENDPOINT,
        USB_DYNAMIC | USB_DIR_OUT,
        USB_EP_ATTR_INT,
        0x40,
        0x03,
    },
};

static void dump_data(uint8_t *data, rt_size_t size)
{
    rt_size_t i;
    for (i = 0; i < size; i++)
    {
        rt_kprintf("%02x ", *data++);
        if ((i + 1) % 8 == 0)
        {
            rt_kprintf("\n");
        }else if ((i + 1) % 4 == 0){
            rt_kprintf(" ");
        }
    }
}
static void dump_report(struct hid_report * report)
{
    rt_kprintf("\nHID Recived:");
    rt_kprintf("\nReport ID %02x \n", report->report_id);
    dump_data(report->report,report->size);
}

static rt_err_t _ep_out_handler(ufunction_t func, rt_size_t size)
{
    struct hid_s *data;
    struct hid_report report;
    int ret = 0;
    RT_ASSERT(func != RT_NULL);
    RT_ASSERT(func->device != RT_NULL);
    data = (struct hid_s *) func->user_data;

    if(size != 0)
    {
        rt_memcpy((void *)&report,(void*)data->ep_out->buffer,size);
        report.size = size-1;
        ret = rt_mq_send(&data->hid_mq,(void *)&report,sizeof(report));
        if (ret)
            rt_kprintf("WARNING[HID]:%s-%d hid recv rt_mq_send failed %d\n", __func__, __LINE__, ret);
    }

    data->ep_out->request.buffer = data->ep_out->buffer;
    data->ep_out->request.size = EP_MAXPACKET(data->ep_out);
    data->ep_out->request.req_type = UIO_REQUEST_READ_BEST;
    rt_usbd_io_request(func->device, data->ep_out, &data->ep_out->request);
    return RT_EOK;
}

static rt_err_t _ep_in_handler(ufunction_t func, rt_size_t size)
{
    struct hid_s *data;
    RT_ASSERT(func != RT_NULL);
    RT_ASSERT(func->device != RT_NULL);

    data = (struct hid_s *) func->user_data;
    if(data->parent.tx_complete != RT_NULL)
    {
        data->parent.tx_complete(&data->parent,RT_NULL);
    }
    rt_completion_done(&data->completion);
    return RT_EOK;
}

static rt_err_t _hid_set_report_callback(udevice_t device, rt_size_t size)
{
    RT_DEBUG_LOG(RT_DEBUG_USB, ("_hid_set_report_callback\n"));
    struct hid_report report;
    struct hid_s *data;
    int ret = 0;

    data = g_hiddev;
    if(size != 0)
    {
        rt_memcpy((void *)&report,(void*)data->report_buf,size);
        report.size = size-1;
        ret = rt_mq_send(&data->hid_mq,(void *)&report,sizeof(report));
        if (ret)
            rt_kprintf("WARNING[HID]:%s-%d hid recv rt_mq_send failed %d\n", __func__, __LINE__, ret);
    }

    dcd_ep0_send_status(device->dcd);

    return RT_EOK;
}

/**
 * This function will handle hid interface bRequest.
 *
 * @param device the usb device object.
 * @param setup the setup bRequest.
 *
 * @return RT_EOK on successful.
 */
static rt_err_t _interface_handler(ufunction_t func, ureq_t setup)
{
    RT_ASSERT(func != RT_NULL);
    RT_ASSERT(func->device != RT_NULL);
    RT_ASSERT(setup != RT_NULL);

    struct hid_s *data = (struct hid_s *) func->user_data;

    // if(setup->wIndex != 0)
    //     return -RT_EIO;

    switch (setup->bRequest)
    {
    case USB_REQ_GET_DESCRIPTOR:
        if((setup->wValue >> 8) == USB_DESC_TYPE_REPORT)
        {
            rt_usbd_ep0_write(func->device, (void *)(&_report_desc[0]), sizeof(_report_desc));
        }
        else if((setup->wValue >> 8) == USB_DESC_TYPE_HID)
        {

            rt_usbd_ep0_write(func->device, (void *)(&_hid_comm_desc.hid_desc), sizeof(struct uhid_descriptor));
        }
        break;
    case USB_HID_REQ_GET_REPORT:
        if(setup->wLength == 0)
        {
            rt_usbd_ep0_set_stall(func->device);
            break;
        }
        if((setup->wLength == 0) || (setup->wLength > MAX_REPORT_SIZE))
            setup->wLength = MAX_REPORT_SIZE;
        rt_usbd_ep0_write(func->device, data->report_buf,setup->wLength);
        break;
    case USB_HID_REQ_GET_IDLE:
        dcd_ep0_send_status(func->device->dcd);
        break;
    case USB_HID_REQ_GET_PROTOCOL:
        rt_usbd_ep0_write(func->device, &data->protocol,2);
        break;
    case USB_HID_REQ_SET_REPORT:
        if((setup->wLength == 0) || (setup->wLength > MAX_REPORT_SIZE))
            rt_usbd_ep0_set_stall(func->device);

        rt_usbd_ep0_read(func->device, data->report_buf, setup->wLength, _hid_set_report_callback);
        break;
    case USB_HID_REQ_SET_IDLE:
        dcd_ep0_send_status(func->device->dcd);
        break;
    case USB_HID_REQ_SET_PROTOCOL:
        data->protocol = setup->wValue;

        dcd_ep0_send_status(func->device->dcd);
        break;
    }
    return RT_EOK;
}


/**
 * This function will run cdc function, it will be called on handle set configuration bRequest.
 *
 * @param func the usb function object.
 *
 * @return RT_EOK on successful.
 */
static rt_err_t _function_enable(ufunction_t func)
{
    struct hid_s *data;

    RT_ASSERT(func != RT_NULL);
    RT_ASSERT(func->device != RT_NULL);
    data = (struct hid_s *) func->user_data;

    RT_DEBUG_LOG(RT_DEBUG_USB, ("hid function enable\n"));
//
//    _vcom_reset_state(func);
//
    if(data->ep_out->buffer == RT_NULL)
    {
        data->ep_out->buffer        = rt_malloc(HID_RX_BUFSIZE);
    }
    data->ep_out->request.buffer    = data->ep_out->buffer;
    data->ep_out->request.size      = EP_MAXPACKET(data->ep_out);
    data->ep_out->request.req_type  = UIO_REQUEST_READ_BEST;
    rt_usbd_io_request(func->device, data->ep_out, &data->ep_out->request);

    return RT_EOK;
}

/**
 * This function will stop cdc function, it will be called on handle set configuration bRequest.
 *
 * @param func the usb function object.
 *
 * @return RT_EOK on successful.
 */
static rt_err_t _function_disable(ufunction_t func)
{
    struct hid_s *data;

    RT_ASSERT(func != RT_NULL);
    RT_ASSERT(func->device != RT_NULL);
    data = (struct hid_s *) func->user_data;

    RT_DEBUG_LOG(RT_DEBUG_USB, ("hid function disable\n"));

    if(data->ep_out->buffer != RT_NULL)
    {
        rt_free(data->ep_out->buffer);
        data->ep_out->buffer = RT_NULL;
    }

    return RT_EOK;
}

// static rt_size_t _hid_write(rt_device_t dev, rt_off_t pos, const void *buffer, rt_size_t size)
static rt_size_t _hid_write(rt_device_t dev, rt_off_t pos, const void *buffer, rt_size_t size)
{
    struct hid_s *hiddev = (struct hid_s *)dev;
    struct hid_report report;
    int ret = 0;

    if ((hiddev->report & (1 << pos)) == 0)
    {
        rt_kprintf("WARNING[HID]: The report idx %d is invalid and data will not be send!\n", pos);
        return 0;
    }
    rt_mutex_take(&(hiddev->tx_mutex), RT_WAITING_FOREVER);
    if (hiddev->func->device->state == USB_STATE_CONFIGURED)
    {
        report.report_id = pos;
        rt_memcpy((void *)report.report,(void *)buffer,size);
        report.size = size;
        hiddev->ep_in->request.buffer = (void *)&report;
        hiddev->ep_in->request.size = (size+1) > 64 ? 64 : size+1;
        hiddev->ep_in->request.req_type = UIO_REQUEST_WRITE;
        rt_usbd_io_request(hiddev->func->device, hiddev->ep_in, &hiddev->ep_in->request);
        ret = rt_completion_wait(&hiddev->completion, RT_TICK_PER_SECOND);
        if (ret)
            rt_kprintf("ERROR[HID]: %s-%d hid write failed ret = %d\n", __func__, __LINE__, ret);
        rt_mutex_release(&(hiddev->tx_mutex));
        return size;
    }
    rt_mutex_release(&(hiddev->tx_mutex));
    return 0;
}

static rt_size_t _hid_read(rt_device_t dev, rt_off_t pos, void *buffer, rt_size_t size)
{
    struct hid_s *hiddev = (struct hid_s *)dev;
    struct hid_report report;
    int ret = 0;
    int read_size = 0;

    rt_mutex_take(&(hiddev->rx_mutex), RT_WAITING_FOREVER);
    if (hiddev->func->device->state == USB_STATE_CONFIGURED)
    {
        ret = rt_mq_recv(&hiddev->hid_mq, &report, sizeof(report),RT_WAITING_FOREVER);
        if (ret != RT_EOK)
        {
            rt_mutex_release(&(hiddev->rx_mutex));
            return ret;
        }

        read_size = size > (report.size + 1) ? (report.size + 1) : size;
        rt_memcpy(buffer, (void *)&report, read_size);
        rt_mutex_release(&(hiddev->rx_mutex));
        return read_size;
    }
    rt_mutex_release(&(hiddev->rx_mutex));
    return 0;
}

RT_WEAK void HID_Report_Received(hid_report_t report)
{
    dump_report(report);
}

#ifdef RT_USING_DEVICE_OPS
const static struct rt_device_ops hid_device_ops =
{
    RT_NULL,
    RT_NULL,
    RT_NULL,
    RT_NULL,
    _hid_write,
    RT_NULL,
};
#endif

static void rt_usb_hid_init(struct ufunction *func)
{
    struct hid_s *hiddev;
    int i;
    hiddev = (struct hid_s *)func->user_data;
    rt_memset(&hiddev->parent, 0, sizeof(hiddev->parent));

#ifdef RT_USING_DEVICE_OPS
    hiddev->parent.ops   = &hid_device_ops;
#else
    hiddev->parent.write = _hid_write;
    hiddev->parent.read = _hid_read;
#endif
    hiddev->func = func;

    hiddev->report = 0;
#ifdef RT_USB_DEVICE_HID_MOUSE
    hiddev->report |= 1 << HID_REPORT_ID_MOUSE;
#endif
#ifdef RT_USB_DEVICE_HID_GENERAL
    hiddev->report |= 1 << HID_REPORT_ID_GENERAL;
#endif
#ifdef RT_USB_DEVICE_HID_TELEPHONY
    hiddev->report |= 1 << HID_REPORT_ID_TELEPHONY;
#endif
#ifdef RT_USB_DEVICE_HID_MEDIA
    hiddev->report |= 1 << HID_REPORT_ID_MEDIA;
#endif

#ifdef RT_USB_DEVICE_HID_KEYBOARD_CONSUMER
    hiddev->report |= 1 << HID_REPORT_ID_KEYBOARD4;
#endif
#ifdef RT_USB_DEVICE_HID_KEYBOARD
    i = RT_USB_DEVICE_HID_KEYBOARD_NUMBER;
    do
    {
        hiddev->report |= 1 << i;
    } while (i--);
#endif
    rt_mutex_init(&hiddev->tx_mutex, "hid rx mutex",
                    RT_IPC_FLAG_FIFO);
    rt_mutex_init(&hiddev->rx_mutex, "hid tx mutex",
                    RT_IPC_FLAG_FIFO);
    rt_completion_init(&hiddev->completion);
    rt_mq_init(&hiddev->hid_mq, "hiddmq", hid_mq_pool, sizeof(struct hid_report),
                            sizeof(hid_mq_pool), RT_IPC_FLAG_FIFO);
    rt_device_register(&hiddev->parent, "hidd", RT_DEVICE_FLAG_RDWR);

}

static rt_err_t
hid_set_alt(struct ufunction *f, unsigned int interface, unsigned int alt)
{
    struct udevice* device = f->device;
    struct hid_s *data = f->user_data;

    dcd_ep_disable(device->dcd, data->ep_in);
    dcd_ep_enable(device->dcd, data->ep_in);

    dcd_ep_disable(device->dcd, data->ep_out);
    dcd_ep_enable(device->dcd, data->ep_out);

    return 0;
}

static const struct usb_descriptor_header * const hid_desc[] =
{
    (struct usb_descriptor_header *) &_hid_comm_desc.iad_desc,
    (struct usb_descriptor_header *) &_hid_comm_desc.intf_desc,
    (struct usb_descriptor_header *) &_hid_comm_desc.hid_desc,
    (struct usb_descriptor_header *) &_hid_comm_desc.ep_in_desc,
    (struct usb_descriptor_header *) &_hid_comm_desc.ep_out_desc,
    NULL,
};

#define UVC_COPY_DESCRIPTORS(mem, dst, src) \
        do { \
            const struct usb_descriptor_header * const *__src; \
            for (__src = src; *__src; ++__src) { \
                rt_memcpy(mem, *__src, (*__src)->bLength); \
                *dst++ = mem; \
                mem += (*__src)->bLength; \
            } \
        } while (0)

static void hid_copy_descriptors(ufunction_t f)
{
    const struct usb_descriptor_header * const *src;
    struct usb_descriptor_header **dst;
    struct usb_descriptor_header **hdr;
    unsigned int n_desc = 0;
    unsigned int bytes = 0;
    void *mem;
    int needBufLen;

    for (src = hid_desc; *src; ++src)
    {
        bytes += (*src)->bLength;
        n_desc++;
    }

    if (f->desc_hdr_mem)
    {
        rt_free(f->desc_hdr_mem);
        f->desc_hdr_mem = RT_NULL;
    }

    needBufLen = (n_desc + 1) * sizeof(*src) + bytes;

    mem = rt_malloc(needBufLen);
    if (mem == NULL)
        return;

    hdr = mem;
    dst = mem;
    mem += (n_desc + 1) * sizeof(*src);

    f->desc_hdr_mem = hdr;
    f->descriptors = mem;
    f->desc_size = bytes;

    /* Copy the descriptors. */
    UVC_COPY_DESCRIPTORS(mem, dst, hid_desc);
    *dst = NULL;
}

int hid_bind_config(uconfig_t c)
{
    ufunction_t     func;
    struct hid_s   *data;
    udevice_t device = c->device;
    int ret = -1;

    /* parameter check */
    RT_ASSERT(device != RT_NULL);

    /* create a cdc function */
    func = rt_usbd_function_new(device);

    /* allocate memory for cdc vcom data */
    data = (struct hid_s*)rt_malloc(sizeof(struct hid_s));
    rt_memset(data, 0, sizeof(struct hid_s));
    g_hiddev = data;
    func->user_data = (void*)data;

    ret = usb_string_id(c, HID_STR_ASSOCIATION);
    if (ret < 0)
        goto error;
    _hid_comm_desc.iad_desc.iFunction = ret;
    _hid_comm_desc.intf_desc.iInterface = ret;

    func->enable = _function_enable;
    func->disable = _function_disable;
    func->setup = _interface_handler;
    func->set_alt = hid_set_alt;

    ret = usb_interface_id(c, func);
    _hid_comm_desc.iad_desc.bFirstInterface = ret;
    _hid_comm_desc.intf_desc.bInterfaceNumber = ret;

    data->ep_out = usb_ep_autoconfig(func, &_hid_comm_desc.ep_out_desc, _ep_out_handler);
    data->ep_in = usb_ep_autoconfig(func, &_hid_comm_desc.ep_in_desc, _ep_in_handler);

    hid_copy_descriptors(func);

    rt_usbd_config_add_function(c, func);

    /* initilize hid */
    rt_usb_hid_init(func);

    return 0;
error:
    rt_kprintf("error: hid bind config\n");
    return ret;
}

void fh_hid_init(void)
{
    rt_usb_device_init();
}

/* int hid_init(int argc, char *argv[])
{
    rt_usb_device_init();
    return 0;
}
MSH_CMD_EXPORT(hid_init, usb_4G begin to dial); */
