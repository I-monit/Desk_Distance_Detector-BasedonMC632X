#include <rtdef.h>
#include <rthw.h>
#include <rtthread.h>
#include <rtservice.h>
#include <rtdevice.h>
#include <string.h>
#include "../core/usbdevice.h"

#include "ch9.h"
#include "video.h"
#include "libuvc.h"
#include "uvc_init.h"

#ifdef DEBUG1
#define UVC_INFO(format, arg...)                                            \
do {                                                                   \
    rt_kprintf( format "\n", ##arg);        \
} while (0)
#else
#define UVC_INFO(format, arg...)
#endif

struct uendpoint *video_stream1_ep1;
struct uendpoint *video_control_ep;

#ifdef UVC_DOUBLE_STREAM
struct uendpoint *video_stream2_ep1;
#endif

static unsigned int gLastDirOut = 0;
static unsigned int gLastEvtLen = 0;


/* --------------------------------------------------------------------------
 * Function descriptors
 */

#define UVC_STRING_CONTROL              "Video Control"
#define UVC_STRING_STREAMING            "Video Streaming"
#define UVC_STRING_STREAMING_2            "Video-2 Streaming"


#define UVC_INTF_VIDEO_CONTROL            0
#define UVC_INTF_VIDEO_STREAMING        1

static struct usb_interface_assoc_descriptor uvc_iad = {
    .bLength        = sizeof(uvc_iad),
    .bDescriptorType    = USB_DT_INTERFACE_ASSOCIATION,
    .bFirstInterface    = 0,
    .bInterfaceCount    = 2,
    .bFunctionClass        = USB_CLASS_VIDEO,
    .bFunctionSubClass    = UVC_SC_VIDEO_INTERFACE_COLLECTION,
    .bFunctionProtocol    = 0x00,
    .iFunction        = 0,
};

static struct usb_interface_descriptor uvc_control_intf  = {
    .bLength        = USB_DT_INTERFACE_SIZE,
    .bDescriptorType    = USB_DT_INTERFACE,
    .bInterfaceNumber    = UVC_INTF_VIDEO_CONTROL,
    .bAlternateSetting    = 0,
    .bNumEndpoints        = 1,
    .bInterfaceClass    = USB_CLASS_VIDEO,
    .bInterfaceSubClass    = UVC_SC_VIDEOCONTROL,
    .bInterfaceProtocol    = 0x00,
    .iInterface        = 0,
};

DECLARE_UVC_HEADER_DESCRIPTOR(2);

static  struct UVC_HEADER_DESCRIPTOR(2) uvc_control_header = {
#ifdef UVC_DOUBLE_STREAM
    .bLength        = UVC_DT_HEADER_SIZE(2),
#else
    .bLength        = UVC_DT_HEADER_SIZE(1),
#endif
    .bDescriptorType    = USB_DT_CS_INTERFACE,
    .bDescriptorSubType    = UVC_VC_HEADER,
    .bcdUVC            = cpu_to_le16(0x0100),
    .wTotalLength        = 0, /* dynamic */
    .dwClockFrequency    = cpu_to_le32(30000000),
    .bInCollection        = 0, /* dynamic */
    .baInterfaceNr[0]    = 0, /* dynamic */
};

static const struct uvc_camera_terminal_descriptor uvc_camera_terminal = {
    .bLength        = UVC_DT_CAMERA_TERMINAL_SIZE(3),
    .bDescriptorType    = USB_DT_CS_INTERFACE,
    .bDescriptorSubType    = UVC_VC_INPUT_TERMINAL,
    .bTerminalID        = UVC_CAMERAL_TERMINAL_ID,
    .wTerminalType        = cpu_to_le16(0x0201),
    .bAssocTerminal        = 0,
    .iTerminal        = 0,
    .wObjectiveFocalLengthMin    = cpu_to_le16(0),
    .wObjectiveFocalLengthMax    = cpu_to_le16(0),
    .wOcularFocalLength        = cpu_to_le16(0),
    .bControlSize        = 3,
    .bmControls[0]        = CAMERA_TERMINAL_CONTROLS & 0xff, /* 0x0e */
    .bmControls[1]        = (CAMERA_TERMINAL_CONTROLS >> 8) & 0xff, /* 0xff */
    .bmControls[2]        = (CAMERA_TERMINAL_CONTROLS >> 16) & 0xff, /* 0x07 */
};

static const struct uvc_processing_unit_descriptor uvc_processing = {
    .bLength        = UVC_DT_PROCESSING_UNIT_SIZE(2),
    .bDescriptorType    = USB_DT_CS_INTERFACE,
    .bDescriptorSubType    = UVC_VC_PROCESSING_UNIT,
    .bUnitID        = UVC_PROCESSING_UNIT_ID,
    .bSourceID        = UVC_CAMERAL_TERMINAL_ID,
    .wMaxMultiplier        = cpu_to_le16(16*1024),
    .bControlSize        = 2,
    .bmControls[0]        = PROCESSING_UNIT_CONTROLS & 0xff, /* 0xff */
    .bmControls[1]        = (PROCESSING_UNIT_CONTROLS >> 8) & 0xff, /* 0xff */
    .iProcessing        = 0,
};

DECLARE_UVC_EXTENSION_UNIT_DESCRIPTOR(1, 3);

static const struct UVC_EXTENSION_UNIT_DESCRIPTOR(1, 3) h264_extension_unit = {
    .bLength        = UVC_DT_EXTENSION_UNIT_SIZE(1, 3),
    .bDescriptorType    = USB_DT_CS_INTERFACE,
    .bDescriptorSubType    = UVC_VC_EXTENSION_UNIT,
    .bUnitID        = UVC_H264_EXTENSION_UNIT_ID,
    .guidExtensionCode = { 0x41, 0x76, 0x9e, 0xa2, 0x04, 0xde, 0xe3, 0x47, 0x8b,
            0x2b, 0xf4, 0x34, 0x1a, 0xff, 0x88, 0x88},
    .bNumControls = 22,
    .bNrInPins = 1,
    .baSourceID[0]        = UVC_PROCESSING_UNIT_ID,
    .bControlSize        = 3,
    .bmControls[0]        = EXTENSION_UNIT_CONTROLS & 0xff,
    .bmControls[1]        = (EXTENSION_UNIT_CONTROLS >> 8) & 0xff,
    .bmControls[2]        = (EXTENSION_UNIT_CONTROLS >> 16) & 0xff,

    .iExtension        = 0,
};

/* Microsoft extension */
DECLARE_UVC_EXTENSION_UNIT_DESCRIPTOR(1, 2);
static const struct UVC_EXTENSION_UNIT_DESCRIPTOR(1, 2) msxu_face_extension_unit = {
    .bLength        = UVC_DT_EXTENSION_UNIT_SIZE(1, 2),
    .bDescriptorType    = USB_DT_CS_INTERFACE,
    .bDescriptorSubType    = UVC_VC_EXTENSION_UNIT,
    .bUnitID        = UVC_MSXU_FACE_EXTENSION_UNIT_ID,
    .guidExtensionCode = { 0xdc, 0x95, 0x3f, 0x0f, 0x32, 0x26, 0x4e, 0x4c, 0x92,
            0xc9, 0xa0, 0x47, 0x82, 0xf4, 0x3b, 0xc8},
    .bNumControls = 0x02,
    .bNrInPins = 1,
    .baSourceID[0]        = UVC_PROCESSING_UNIT_ID,
    .bControlSize        = 2,
    .bmControls[0]        = MSXU_EXTENSION_UNIT_CONTROLS & 0xff,
    .bmControls[1]        = (MSXU_EXTENSION_UNIT_CONTROLS >> 8) & 0xff,
    .iExtension        = 0,
};
static const struct uvc_output_terminal_descriptor uvc_output_terminal = {
    .bLength        = UVC_DT_OUTPUT_TERMINAL_SIZE,
    .bDescriptorType    = USB_DT_CS_INTERFACE,
    .bDescriptorSubType    = UVC_VC_OUTPUT_TERMINAL,
    .bTerminalID        = UVC_OUTPUT_TERMINAL_ID,
    .wTerminalType        = cpu_to_le16(0x0101),
    .bAssocTerminal        = 0,
    .bSourceID        = UVC_PROCESSING_UNIT_ID,
    .iTerminal        = 0,
};

#ifdef UVC_DOUBLE_STREAM
static const struct uvc_output_terminal_descriptor uvc_output_terminal2 = {
    .bLength        = UVC_DT_OUTPUT_TERMINAL_SIZE,
    .bDescriptorType    = USB_DT_CS_INTERFACE,
    .bDescriptorSubType    = UVC_VC_OUTPUT_TERMINAL,
    .bTerminalID        = UVC_OUTPUT_TERMINAL2_ID,
    .wTerminalType        = cpu_to_le16(0x0101),
    .bAssocTerminal        = 0,
    .bSourceID        = UVC_H264_EXTENSION_UNIT_ID,
    .iTerminal        = 0,
};
#endif

static struct usb_endpoint_descriptor uvc_control_ep = {
    .bLength        = USB_DT_ENDPOINT_SIZE,
    .bDescriptorType    = USB_DT_ENDPOINT,
    .bEndpointAddress    = USB_DIR_IN,
    .bmAttributes        = USB_ENDPOINT_XFER_INT, /* USB_ENDPOINT_XFER_CONTROL */
    .wMaxPacketSize        = cpu_to_le16(64),
    .bInterval        = 8, /* 8 */
};

static struct uvc_control_endpoint_descriptor uvc_control_cs_ep  = {
    .bLength        = UVC_DT_CONTROL_ENDPOINT_SIZE,
    .bDescriptorType    = USB_DT_CS_ENDPOINT,
    .bDescriptorSubType    = UVC_EP_INTERRUPT,
    .wMaxTransferSize    = cpu_to_le16(64),
};

static struct usb_interface_descriptor uvc_streaming1_intf_alt0 = {
    .bLength        = USB_DT_INTERFACE_SIZE,
    .bDescriptorType    = USB_DT_INTERFACE,
    .bInterfaceNumber    = UVC_INTF_VIDEO_STREAMING,
    .bAlternateSetting    = 0,
#ifdef FH_RT_UVC_EP_TYPE_ISOC
    .bNumEndpoints        = 0,
#else/* FH_RT_UVC_EP_TYPE_BULK */
    .bNumEndpoints        = 1,
#endif
    .bInterfaceClass    = USB_CLASS_VIDEO,
    .bInterfaceSubClass    = UVC_SC_VIDEOSTREAMING,
    .bInterfaceProtocol    = 0x00,
    .iInterface        = 0,
};

#ifdef FH_RT_UVC_EP_TYPE_ISOC
static struct usb_interface_descriptor uvc_streaming1_intf_alt1  = {
    .bLength        = USB_DT_INTERFACE_SIZE,
    .bDescriptorType    = USB_DT_INTERFACE,
    .bInterfaceNumber    = UVC_INTF_VIDEO_STREAMING,
    .bAlternateSetting    = 1,
    .bNumEndpoints        = 1,
    .bInterfaceClass    = USB_CLASS_VIDEO,
    .bInterfaceSubClass    = UVC_SC_VIDEOSTREAMING,
    .bInterfaceProtocol    = 0x00,
    .iInterface        = 0,
};
#endif

static struct usb_endpoint_descriptor uvc_streaming1_ep = {
    .bLength        = USB_DT_ENDPOINT_SIZE,
    .bDescriptorType    = USB_DT_ENDPOINT,
    .bEndpointAddress    = USB_DIR_IN,
#ifdef FH_RT_UVC_EP_TYPE_ISOC
    .bmAttributes        = USB_ENDPOINT_SYNC_ASYNC | USB_ENDPOINT_XFER_ISOC,
    .wMaxPacketSize        = cpu_to_le16((ISOC_CNT_ON_MICROFRAME-1)<<11|(UVC_PACKET_SIZE_ALT1/ISOC_CNT_ON_MICROFRAME)),
    .bInterval        = 1,
#else/* FH_RT_UVC_EP_TYPE_BULK */
    .bmAttributes        = USB_ENDPOINT_XFER_BULK,
    .wMaxPacketSize        = cpu_to_le16(UVC_PACKET_SIZE),
#endif

};

#ifdef FH_RT_UVC_EP_TYPE_ISOC

#define uvc_streaming1_intf_alt(n) \
static struct usb_interface_descriptor uvc_streaming1_intf_alt##n = \
{\
    .bLength        = USB_DT_INTERFACE_SIZE,\
    .bDescriptorType    = USB_DT_INTERFACE,\
    .bInterfaceNumber    = UVC_INTF_VIDEO_STREAMING,\
    .bAlternateSetting    = n,\
    .bNumEndpoints        = 1,\
    .bInterfaceClass    = USB_CLASS_VIDEO,\
    .bInterfaceSubClass    = UVC_SC_VIDEOSTREAMING,\
    .bInterfaceProtocol    = 0x00,\
    .iInterface        = 0,\
};

#define uvc_streaming1_ep_alt(n, pkt, cnt)\
static struct usb_endpoint_descriptor uvc_streaming1_ep_alt##n = \
{\
    .bLength        = USB_DT_ENDPOINT_SIZE,\
    .bDescriptorType    = USB_DT_ENDPOINT,\
    .bEndpointAddress    = USB_DIR_IN,\
    .bmAttributes        = USB_ENDPOINT_SYNC_ASYNC | USB_ENDPOINT_XFER_ISOC,\
    .wMaxPacketSize        = cpu_to_le16((cnt-1)<<11|(pkt/cnt)),\
    .bInterval        = 1,\
};

uvc_streaming1_intf_alt(2);
uvc_streaming1_ep_alt(2, UVC_PACKET_SIZE_ALT2, ISOC_CNT_ON_MICROFRAME);

uvc_streaming1_intf_alt(3);
uvc_streaming1_ep_alt(3, UVC_PACKET_SIZE_ALT3, ISOC_CNT_ON_MICROFRAME);

uvc_streaming1_intf_alt(4);
uvc_streaming1_ep_alt(4, UVC_PACKET_SIZE_ALT4, 1);

uvc_streaming1_intf_alt(5);
uvc_streaming1_ep_alt(5, UVC_PACKET_SIZE_ALT5, 1);

uvc_streaming1_intf_alt(6);
uvc_streaming1_ep_alt(6, UVC_PACKET_SIZE_ALT6, 1);

uvc_streaming1_intf_alt(7);
uvc_streaming1_ep_alt(7, UVC_PACKET_SIZE_ALT7, 1);

uvc_streaming1_intf_alt(8);
uvc_streaming1_ep_alt(8, UVC_PACKET_SIZE_ALT8, 1);

#endif

static struct usb_endpoint_descriptor uvc_streaming1_ep_other = {
    .bLength        = USB_DT_ENDPOINT_SIZE,
    .bDescriptorType    = USB_DT_ENDPOINT,
    .bEndpointAddress    = USB_DIR_IN,
#ifdef FH_RT_UVC_EP_TYPE_ISOC
    .bmAttributes        = USB_ENDPOINT_SYNC_ASYNC | USB_ENDPOINT_XFER_ISOC,
    .wMaxPacketSize        = cpu_to_le16(1023),
    .bInterval        = 1,
#else/* FH_RT_UVC_EP_TYPE_BULK */
    .bmAttributes        = USB_ENDPOINT_XFER_BULK,
    .wMaxPacketSize        = cpu_to_le16(32),
#endif

};

static const struct uvc_descriptor_header * const uvc_control_cls[] = {
    (const struct uvc_descriptor_header *) &uvc_control_header,
    (const struct uvc_descriptor_header *) &uvc_camera_terminal,
    (const struct uvc_descriptor_header *) &uvc_processing,
    (const struct uvc_descriptor_header *) &h264_extension_unit,
    (const struct uvc_descriptor_header *) &msxu_face_extension_unit,
    (const struct uvc_descriptor_header *) &uvc_output_terminal,
#ifdef UVC_DOUBLE_STREAM
    (const struct uvc_descriptor_header *) &uvc_output_terminal2,
#endif
    NULL,
};

static const struct usb_descriptor_header * const uvc_hs_streaming1[] = {
#ifdef FH_RT_UVC_EP_TYPE_ISOC
    (struct usb_descriptor_header *) &uvc_streaming1_intf_alt1,
#endif
    (struct usb_descriptor_header *) &uvc_streaming1_ep,
#ifdef FH_RT_UVC_EP_TYPE_ISOC
    (struct usb_descriptor_header *) &uvc_streaming1_intf_alt2,
    (struct usb_descriptor_header *) &uvc_streaming1_ep_alt2,
    (struct usb_descriptor_header *) &uvc_streaming1_intf_alt3,
    (struct usb_descriptor_header *) &uvc_streaming1_ep_alt3,
    (struct usb_descriptor_header *) &uvc_streaming1_intf_alt4,
    (struct usb_descriptor_header *) &uvc_streaming1_ep_alt4,
    (struct usb_descriptor_header *) &uvc_streaming1_intf_alt5,
    (struct usb_descriptor_header *) &uvc_streaming1_ep_alt5,
    (struct usb_descriptor_header *) &uvc_streaming1_intf_alt6,
    (struct usb_descriptor_header *) &uvc_streaming1_ep_alt6,
    (struct usb_descriptor_header *) &uvc_streaming1_intf_alt7,
    (struct usb_descriptor_header *) &uvc_streaming1_ep_alt7,
    (struct usb_descriptor_header *) &uvc_streaming1_intf_alt8,
    (struct usb_descriptor_header *) &uvc_streaming1_ep_alt8,
#endif
    NULL,
};

static const struct usb_descriptor_header * const uvc_hs_streaming1_other[] = {
#ifdef FH_RT_UVC_EP_TYPE_ISOC
    (struct usb_descriptor_header *) &uvc_streaming1_intf_alt1,
#endif
    (struct usb_descriptor_header *) &uvc_streaming1_ep_other,
    NULL,
};

static struct usb_interface_descriptor uvc_streaming2_intf_alt0 = {
    .bLength        = USB_DT_INTERFACE_SIZE,
    .bDescriptorType    = USB_DT_INTERFACE,
    .bInterfaceNumber    = UVC_INTF_VIDEO_STREAMING,
    .bAlternateSetting    = 0,
#ifdef FH_RT_UVC_EP_TYPE_ISOC
    .bNumEndpoints        = 0,
#else/* FH_RT_UVC_EP_TYPE_BULK */
    .bNumEndpoints        = 1,
#endif
    .bInterfaceClass    = USB_CLASS_VIDEO,
    .bInterfaceSubClass    = UVC_SC_VIDEOSTREAMING,
    .bInterfaceProtocol    = 0x00,
    .iInterface        = 0,
};

#ifdef FH_RT_UVC_EP_TYPE_ISOC
static struct usb_interface_descriptor uvc_streaming2_intf_alt1  = {
    .bLength        = USB_DT_INTERFACE_SIZE,
    .bDescriptorType    = USB_DT_INTERFACE,
    .bInterfaceNumber    = UVC_INTF_VIDEO_STREAMING,
    .bAlternateSetting    = 1,
    .bNumEndpoints        = 1,
    .bInterfaceClass    = USB_CLASS_VIDEO,
    .bInterfaceSubClass    = UVC_SC_VIDEOSTREAMING,
    .bInterfaceProtocol    = 0x00,
    .iInterface        = 0,
};
#endif

#ifdef UVC_DOUBLE_STREAM
static struct usb_endpoint_descriptor uvc_streaming2_ep = {
    .bLength        = USB_DT_ENDPOINT_SIZE,
    .bDescriptorType    = USB_DT_ENDPOINT,
    .bEndpointAddress    = USB_DIR_IN,
#ifdef FH_RT_UVC_EP_TYPE_ISOC
    .bmAttributes        = USB_ENDPOINT_SYNC_ASYNC | USB_ENDPOINT_XFER_ISOC,
    .wMaxPacketSize        = cpu_to_le16(UVC_PACKET_SIZE),
    .bInterval        = 1,
#else/* FH_RT_UVC_EP_TYPE_BULK */
    .bmAttributes        = USB_ENDPOINT_XFER_BULK,
    .wMaxPacketSize        = cpu_to_le16(UVC_PACKET_SIZE),
#endif
};

static struct usb_endpoint_descriptor uvc_streaming2_ep_other = {
    .bLength        = USB_DT_ENDPOINT_SIZE,
    .bDescriptorType    = USB_DT_ENDPOINT,
    .bEndpointAddress    = USB_DIR_IN,
#ifdef FH_RT_UVC_EP_TYPE_ISOC
    .bmAttributes        = USB_ENDPOINT_SYNC_ASYNC | USB_ENDPOINT_XFER_ISOC,
    .wMaxPacketSize        = cpu_to_le16(1023),
    .bInterval        = 1,
#else/* FH_RT_UVC_EP_TYPE_BULK */
    .bmAttributes        = USB_ENDPOINT_XFER_BULK,
    .wMaxPacketSize        = cpu_to_le16(32),
#endif
};

static const struct usb_descriptor_header * const uvc_hs_streaming2[] = {
#ifdef FH_RT_UVC_EP_TYPE_ISOC
    (struct usb_descriptor_header *) &uvc_streaming2_intf_alt1,
#endif
    (struct usb_descriptor_header *) &uvc_streaming2_ep,
    NULL,
};


static const struct usb_descriptor_header * const uvc_hs_streaming2_other[] = {
#ifdef FH_RT_UVC_EP_TYPE_ISOC
    (struct usb_descriptor_header *) &uvc_streaming2_intf_alt1,
#endif
    (struct usb_descriptor_header *) &uvc_streaming2_ep_other,
    NULL,
};
#endif

static void uvc_ep_alt_cfg(struct uendpoint *ep, unsigned int alt)
{
#ifdef FH_RT_UVC_EP_TYPE_ISOC
    switch(alt)
    {
    case 1:
        ep->ep_desc = (uep_desc_t)&uvc_streaming1_ep;
        break;
    case 2:
        ep->ep_desc = (uep_desc_t)&uvc_streaming1_ep_alt2;
        break;
    case 3:
        ep->ep_desc = (uep_desc_t)&uvc_streaming1_ep_alt3;
        break;
    case 4:
        ep->ep_desc = (uep_desc_t)&uvc_streaming1_ep_alt4;
        break;
    case 5:
        ep->ep_desc = (uep_desc_t)&uvc_streaming1_ep_alt5;
        break;
    case 6:
        ep->ep_desc = (uep_desc_t)&uvc_streaming1_ep_alt6;
        break;
    case 7:
        ep->ep_desc = (uep_desc_t)&uvc_streaming1_ep_alt7;
        break;
    case 8:
        ep->ep_desc = (uep_desc_t)&uvc_streaming1_ep_alt8;
        break;
    default:
        rt_kprintf("%s--%d:[warning] please check alt cfg!\n", __func__, __LINE__);
    }
#endif
}

struct udevice *g_uvcdevice;
void uvc_send_response(struct uvc_request_data *resp)
{
    unsigned int len  = resp->length;

    if (gLastDirOut == 0)
    {
        if (resp->length >= 0)
        {
            if (gLastEvtLen < resp->length)
            {
                len = gLastEvtLen;
            }

            rt_usbd_ep0_write(g_uvcdevice, resp->data, len ? len : 1);
        } else if (resp->length < 0)
        {
            rt_usbd_ep0_set_stall(g_uvcdevice);
        }
        // else
        // {
        //     rt_kprintf("[UVC] ERROR:%s-%d resp->length = %d\n", __func__, __LINE__, resp->length);
        //     dcd_ep0_send_status(g_uvcdevice->dcd);
        // }
    }
}

static struct uvc_common *to_common(struct ufunction *f)
{
    return f->uvc_comm;
}

void v4l2_event_queue(struct v4l2_event *event)
{
    struct uvc_request_data *resp;

    resp = uvc_events_process(event->stream_id, (void *)event->u.data, event->type);
    if (resp)
        uvc_send_response(resp);
}

extern struct uvc_stream_info *stream_info[2];
static struct v4l2_event gEventData;
static rt_err_t read_out_data_done(udevice_t device, rt_size_t size)
{
    struct uvc_stream_info *pDev = stream_info[gEventData.stream_id];

    pDev->error_code = UVC_REQ_ERROR_NO;
    v4l2_event_queue(&gEventData);

    if (pDev->error_code == 0)
        dcd_ep0_send_status(device->dcd);
    else
        rt_usbd_ep0_set_stall(device);

    return RT_EOK;
}

static rt_err_t
uvc_function_setup(struct ufunction *f, ureq_t setup)
{
    struct v4l2_event v4l2_event;
    struct uvc_event *uvc_event = (void *)&v4l2_event.u.data;
    struct usb_ctrlrequest *ctrl = (struct usb_ctrlrequest *)setup;

    if ((ctrl->bRequestType & USB_TYPE_MASK) != USB_TYPE_CLASS)
    {
        UVC_INFO("invalid request type\n");
        return -EINVAL;
    }

    /* Stall too big requests. */
    if (le16_to_cpu(ctrl->wLength) > UVC_MAX_REQUEST_SIZE)
        return -EINVAL;

    gLastDirOut = !(ctrl->bRequestType & USB_DIR_IN);
    gLastEvtLen = ctrl->wLength;
    rt_memset(&v4l2_event, 0, sizeof(v4l2_event));
    v4l2_event.type = UVC_EVENT_SETUP;
    rt_memcpy(&uvc_event->req, ctrl, sizeof(uvc_event->req));

#ifdef UVC_DOUBLE_STREAM
    unsigned int intf = ctrl->wIndex & 0xff;
    struct uvc_common *comm = to_common(f);
    if (intf == comm->uvc2->streaming_intf)
        v4l2_event.stream_id = STREAM_ID2;
    else
        v4l2_event.stream_id = STREAM_ID1;
#else
    v4l2_event.stream_id = STREAM_ID1;
#endif

    v4l2_event_queue(&v4l2_event);

    if (gLastDirOut)
    {
        if (ctrl->wLength > 0)
        {
            rt_memset(&gEventData, 0, sizeof(gEventData));
            uvc_event = (void *)&gEventData.u.data;
            gEventData.type = UVC_EVENT_DATA;
            gEventData.stream_id = v4l2_event.stream_id;
            uvc_event->data.length = ctrl->wLength;
            rt_usbd_ep0_read(g_uvcdevice, (uint8_t *)uvc_event->data.data,  ctrl->wLength, read_out_data_done);
        }
#ifdef FH_RT_UVC_EP_TYPE_BULK
        /* set streaminterface commit*/
        if (ctrl->bRequest == 1 && ctrl->wValue == 0x200 && (ctrl->wIndex & 0xf))
        {
            uvc_driver_buf_reinit(v4l2_event.stream_id, 0);
            dcd_ep_disable(f->device->dcd, video_stream1_ep1);
            dcd_ep_enable(f->device->dcd, video_stream1_ep1);
            uvc_sem_release(v4l2_event.stream_id);
        }
#endif

    }

    return 0;
}

static rt_err_t
uvc_function_get_alt(struct ufunction *f, unsigned int interface)
{
    struct uvc_common *comm = to_common(f);

    UVC_INFO("uvc_function_get_alt(%u)\n", interface);

    if (interface == comm->control_intf)
    {
        return 0;
    }
    else if (interface == comm->uvc1->streaming_intf)
    {
        return comm->uvc1->alt;
    }
#ifdef UVC_DOUBLE_STREAM
    else if (interface == comm->uvc2->streaming_intf)
    {
        return comm->uvc2->alt;
    }
#endif
    else
    {
        return -EINVAL;
    }
}

static rt_err_t
uvc_function_set_alt(struct ufunction *f, unsigned int interface, unsigned int alt)
{
    struct uvc_device *uvc;
    struct uvc_common *comm = to_common(f);
    struct uvc_device *uvc1 = comm->uvc1;
    struct uvc_device *uvc2 = comm->uvc2;
    struct udevice *device = f->device;

    uvc = uvc1;

    if (interface == uvc1->streaming_intf)
    {
        uvc = uvc1;
    }
#ifdef UVC_DOUBLE_STREAM
    else if (interface == uvc2->streaming_intf)
    {
        uvc = uvc2;
    }
#endif

    rt_kprintf("uvc_function_set_alt(%u, %u)\n", interface, alt);

    if (interface == comm->control_intf)
    {
        if (alt)
            return -EINVAL;

        if (uvc1->state == UVC_STATE_DISCONNECTED)
        {
            dcd_ep_enable(device->dcd, comm->control_ep);
            uvc1->state = UVC_STATE_CONNECTED;
        }
#ifdef UVC_DOUBLE_STREAM
        if (uvc2->state == UVC_STATE_DISCONNECTED)
        {
            uvc2->state = UVC_STATE_CONNECTED;
        }
#endif
        return 0;
    }

    UVC_INFO("uvc_function_set_alt 11(%u, %u)\n", interface, alt);
    if (interface != uvc1->streaming_intf && interface != uvc2->streaming_intf)
        return -EINVAL;

    uvc->alt = alt;
    switch (alt)
    {
    case 0:
        if (uvc->state != UVC_STATE_STREAMING)
            return 0;
        if (interface == uvc1->streaming_intf)
        {
            fh_uvc_status_set(STREAM_ID1, UVC_STREAM_OFF);
            uvc_driver_buf_reinit(STREAM_ID1, 0);
            dcd_ep_disable(device->dcd, uvc->ep);
            UVC_INFO("stream 1 off\n");
        }
#ifdef UVC_DOUBLE_STREAM
        else if (interface == uvc2->streaming_intf)
        {
            fh_uvc_status_set(STREAM_ID2, UVC_STREAM_OFF);
            uvc_driver_buf_reinit(STREAM_ID2, 0);
            dcd_ep_disable(device->dcd, uvc->ep);
            UVC_INFO("stream 2 off\n");
        }
#endif
        uvc->state = UVC_STATE_CONNECTED;
        break;

    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
        if (uvc->state != UVC_STATE_CONNECTED)
            return 0;
        if (interface == uvc1->streaming_intf)
        {
            UVC_INFO("stream 1 on\n");
            uvc->ep = video_stream1_ep1;
            uvc_ep_alt_cfg(uvc->ep, alt);
            uvc_driver_buf_reinit(STREAM_ID1, EP_MAXPACKET(uvc->ep) * EP_MAXPACKET_CNT(uvc->ep));
            if (uvc && uvc->ep)
            {
                dcd_ep_enable(device->dcd, uvc->ep);
            }

            uvc_sem_release(STREAM_ID1);
        }
#ifdef UVC_DOUBLE_STREAM
        else if (interface == uvc2->streaming_intf)
        {
            UVC_INFO("stream 2 on\n");
            uvc_driver_buf_reinit(STREAM_ID2, UVC_PACKET_SIZE);
            uvc->ep = video_stream2_ep1;
            if (uvc->ep)
            {
                dcd_ep_enable(device->dcd, uvc->ep);
            }
            uvc_sem_release(STREAM_ID2);
        }
#endif
        uvc->state = UVC_STATE_STREAMING;
        break;

    default:
        return -EINVAL;
    }
    UVC_INFO("uvc_function_set_alt end(%u, %u)\n", interface, alt);
    return 0;
}

static rt_err_t
uvc_function_disable(struct ufunction *f)
{
    struct uvc_common *comm = to_common(f);

    UVC_INFO("uvc_function_disable\n");

    comm->uvc1->state = UVC_STATE_DISCONNECTED;
#ifdef UVC_DOUBLE_STREAM
    comm->uvc2->state = UVC_STATE_DISCONNECTED;
#endif
    return 0;
}

#define UVC_COPY_DESCRIPTOR(mem, dst, desc) \
    do \
    {\
        rt_memcpy(mem, desc, (desc)->bLength); \
        *(dst)++ = mem; \
        mem += (desc)->bLength; \
    } while (0)

#define UVC_COPY_DESCRIPTORS(mem, dst, src) \
    do \
    {\
        const struct usb_descriptor_header * const *__src; \
        for (__src = src; *__src; ++__src) \
        {\
            rt_memcpy(mem, *__src, (*__src)->bLength); \
            *dst++ = mem; \
            mem += (*__src)->bLength; \
        } \
    } while (0)

void uvc_copy_descriptors(struct uvc_device *uvc, int other_speed)
{
    struct uvc_input_header_descriptor *uvc_streaming_header;
    struct uvc_header_descriptor *uvc_control_header;
    struct uvc_descriptor_header **uvc_streaming_cls;
    const struct usb_descriptor_header * const *uvc_streaming_std;
    const struct usb_descriptor_header * const *src;
    struct usb_descriptor_header **dst;
    struct usb_descriptor_header **hdr;
    unsigned int control_size;
    unsigned int streaming_size;
    unsigned int n_desc;
    unsigned int bytes;
    void *mem;
    int needBufLen;

    ufunction_t f = uvc->comm->func;

    if (f->desc_hdr_mem && other_speed == 0)
    {
        rt_free(f->desc_hdr_mem);
        f->desc_hdr_mem = RT_NULL;
    }
    else if (f->other_desc_hdr_mem && other_speed == 1)
    {
        rt_free(f->other_desc_hdr_mem);
        f->other_desc_hdr_mem = RT_NULL;
    }

    uvc_streaming_cls = uvc->comm->desc.hs_streaming;
    if (other_speed == 0)
        uvc_streaming_std = uvc_hs_streaming1;
    else
        uvc_streaming_std = uvc_hs_streaming1_other;

#ifdef UVC_DOUBLE_STREAM
    unsigned int stream2_valid = 0;
    struct uvc_descriptor_header **uvc_streaming2_cls = NULL;
    const struct usb_descriptor_header * const *uvc_streaming2_std = NULL;
    unsigned int streaming2_size = 0;

    stream2_valid = (uvc->comm->desc.hs_streaming2 != NULL);
    if (stream2_valid)
    {
        uvc_streaming2_cls = uvc->comm->desc.hs_streaming2;
        if (other_speed == 0)
            uvc_streaming2_std = uvc_hs_streaming2;
        else
            uvc_streaming2_std = uvc_hs_streaming2_other;
        uvc_iad.bInterfaceCount = 3;
    }
#endif
    /* Descriptors layout
     *
     * uvc_iad
     * uvc_control_intf
     * Class-specific UVC control descriptors
     * uvc_control_ep
     * uvc_control_cs_ep
     * uvc_streaming_intf_alt0
     * Class-specific UVC streaming descriptors
     * uvc_{fs|hs}_streaming
     */

    /* Count descriptors and compute their size. */

    control_size = 0;
    streaming_size = 0;
    bytes = uvc_iad.bLength + uvc_control_intf.bLength
          + uvc_control_ep.bLength + uvc_control_cs_ep.bLength
          + uvc_streaming1_intf_alt0.bLength;
    n_desc = 5;

    for (src = (const struct usb_descriptor_header **)uvc->comm->desc.control; *src; ++src)
    {
        control_size += (*src)->bLength;
        bytes += (*src)->bLength;
        n_desc++;
    }

    for (src = (const struct usb_descriptor_header **)uvc_streaming_cls; *src; ++src)
    {
        streaming_size += (*src)->bLength;
        bytes += (*src)->bLength;
        n_desc++;
    }

    for (src = uvc_streaming_std; *src; ++src)
    {
        bytes += (*src)->bLength;
        n_desc++;
    }

#ifdef UVC_DOUBLE_STREAM
    if (stream2_valid)
    {
        bytes += uvc_streaming2_intf_alt0.bLength;
        n_desc++;
        for (src = (const struct usb_descriptor_header **)uvc_streaming2_cls; *src; ++src)
        {
            streaming2_size += (*src)->bLength;
            bytes += (*src)->bLength;
            n_desc++;
        }
        for (src = uvc_streaming2_std; *src; ++src)
        {
            bytes += (*src)->bLength;
            n_desc++;
        }
    }
#endif

    needBufLen = (n_desc + 1) * sizeof(*src) + bytes;

    mem = rt_malloc(needBufLen);
    if (mem == NULL)
        return;

    hdr = mem;
    dst = mem;
    mem += (n_desc + 1) * sizeof(*src);

    if (other_speed == 0)
    {
        f->desc_hdr_mem = hdr;
        f->descriptors = mem;
        f->desc_size = bytes;
    } else
    {
        f->other_desc_hdr_mem = hdr;
        f->other_descriptors = mem;
        f->other_desc_size = bytes;
    }

    /* Copy the descriptors. */
    UVC_COPY_DESCRIPTOR(mem, dst, &uvc_iad);
    UVC_COPY_DESCRIPTOR(mem, dst, &uvc_control_intf);

    uvc_control_header = mem;
    UVC_COPY_DESCRIPTORS(mem, dst,
        (const struct usb_descriptor_header **)uvc->comm->desc.control);
    uvc_control_header->wTotalLength = cpu_to_le16(control_size);
    uvc_control_header->baInterfaceNr[0] = uvc->streaming_intf;
#ifdef UVC_DOUBLE_STREAM
    if (stream2_valid)
    {
        uvc_control_header->baInterfaceNr[1] = uvc->comm->uvc2->streaming_intf;
        uvc_control_header->bInCollection = 2;
    }
#else
    uvc_control_header->bInCollection = 1;
#endif

    UVC_COPY_DESCRIPTOR(mem, dst, &uvc_control_ep);
    UVC_COPY_DESCRIPTOR(mem, dst, &uvc_control_cs_ep);
    UVC_COPY_DESCRIPTOR(mem, dst, &uvc_streaming1_intf_alt0);

    uvc_streaming_header = mem;
    UVC_COPY_DESCRIPTORS(mem, dst,
        (const struct usb_descriptor_header **)uvc_streaming_cls);
    uvc_streaming_header->wTotalLength = cpu_to_le16(streaming_size);
    uvc_streaming_header->bEndpointAddress = uvc_streaming1_ep.bEndpointAddress;

    UVC_COPY_DESCRIPTORS(mem, dst, uvc_streaming_std);

#ifdef UVC_DOUBLE_STREAM
    if (stream2_valid)
    {
        UVC_COPY_DESCRIPTOR(mem, dst, &uvc_streaming2_intf_alt0);
        uvc_streaming_header = mem;
        UVC_COPY_DESCRIPTORS(mem, dst,
            (const struct usb_descriptor_header **)uvc_streaming2_cls);
        uvc_streaming_header->wTotalLength = cpu_to_le16(streaming2_size);
        uvc_streaming_header->bEndpointAddress = uvc_streaming2_ep.bEndpointAddress;
        uvc_streaming_header->bTerminalLink++;
        UVC_COPY_DESCRIPTORS(mem, dst, uvc_streaming2_std);
    }
#endif

    *dst = NULL;
}


#define INFO rt_kprintf

rt_err_t uvc_ep_in_handler(ufunction_t func, rt_size_t size)
{
    RT_ASSERT(func != RT_NULL);
    RT_ASSERT(func->device != RT_NULL);
    struct uendpoint *ep;
    struct uvc_common *comm;

    comm = (struct uvc_common *)func->uvc_comm;
    ep = comm->uvc1->ep;
    uvc_video_complete(func, ep, size);

    return RT_EOK;
}

rt_err_t uvc_ep_in2_handler(ufunction_t func, rt_size_t size)
{
    RT_ASSERT(func != RT_NULL);
    RT_ASSERT(func->device != RT_NULL);
    struct uendpoint *ep;
    struct uvc_common *comm;

    comm = (struct uvc_common *)func->uvc_comm;
    ep = comm->uvc2->ep;
    uvc_video_complete(func, ep, size);

    return RT_EOK;
}

rt_err_t uvc_ep_control_handler(ufunction_t func, rt_size_t size)
{
    RT_ASSERT(func != RT_NULL);
    RT_ASSERT(func->device != RT_NULL);

    return RT_EOK;
}

static int uvc_function_bind(struct uconfig *c, struct ufunction *f)
{
    struct uvc_device *uvc;
    struct uvc_common *comm = (struct uvc_common *)f->uvc_comm;
    struct uvc_device *uvc1;
#ifdef UVC_DOUBLE_STREAM
    struct uvc_device *uvc2;
#endif
    uep_t ep;

    uvc = comm->uvc1;


    int ret = -EINVAL;

    uvc1 = comm->uvc1;
#ifdef UVC_DOUBLE_STREAM
    uvc2 = comm->uvc2;
#endif
    UVC_INFO("uvc_function_bind\n");

    /* Allocate endpoints. */
    ep = usb_ep_autoconfig(f, (uep_desc_t)&uvc_control_ep, uvc_ep_control_handler);
    if (!ep)
    {
        rt_kprintf("Unable to allocate control EP\n");
        goto error;
    }
    comm->control_ep = ep;
    ep->driver_data = uvc;
    video_control_ep = ep;

    ep = usb_ep_autoconfig(f, (uep_desc_t)&uvc_streaming1_ep, uvc_ep_in_handler);
    if (!ep)
    {
        rt_kprintf("Unable to allocate streaming EP uvc_streaming1_ep\n");
        goto error;
    }
    video_stream1_ep1 = ep;
#ifdef FH_RT_UVC_EP_TYPE_ISOC
    uvc_streaming1_ep_alt2.bEndpointAddress = ep->ep_desc->bEndpointAddress;
    uvc_streaming1_ep_alt3.bEndpointAddress = ep->ep_desc->bEndpointAddress;
    uvc_streaming1_ep_alt4.bEndpointAddress = ep->ep_desc->bEndpointAddress;
    uvc_streaming1_ep_alt5.bEndpointAddress = ep->ep_desc->bEndpointAddress;
    uvc_streaming1_ep_alt6.bEndpointAddress = ep->ep_desc->bEndpointAddress;
    uvc_streaming1_ep_alt7.bEndpointAddress = ep->ep_desc->bEndpointAddress;
    uvc_streaming1_ep_alt8.bEndpointAddress = ep->ep_desc->bEndpointAddress;
#endif
    uvc->ep = ep;
    uvc1->ep = ep;
    /* ep->driver_data = uvc1; */

    /* Allocate interface IDs. */
    ret = usb_interface_id(c, f);
    if (ret < 0)
        goto error;
    uvc_iad.bFirstInterface = ret;
    uvc_control_intf.bInterfaceNumber = ret;
    comm->control_intf = ret;

    ret = usb_interface_id(c, f);
    if (ret < 0)
        goto error;
    uvc_streaming1_intf_alt0.bInterfaceNumber = ret;
#ifdef FH_RT_UVC_EP_TYPE_ISOC
    uvc_streaming1_intf_alt1.bInterfaceNumber = ret;
    uvc_streaming1_intf_alt2.bInterfaceNumber = ret;
    uvc_streaming1_intf_alt3.bInterfaceNumber = ret;
    uvc_streaming1_intf_alt4.bInterfaceNumber = ret;
    uvc_streaming1_intf_alt5.bInterfaceNumber = ret;
    uvc_streaming1_intf_alt6.bInterfaceNumber = ret;
    uvc_streaming1_intf_alt7.bInterfaceNumber = ret;
    uvc_streaming1_intf_alt8.bInterfaceNumber = ret;
#endif
    uvc1->streaming_intf = ret;

#ifdef UVC_DOUBLE_STREAM
    video_stream2_ep1 = usb_ep_autoconfig(f, (uep_desc_t)&uvc_streaming2_ep, uvc_ep_in2_handler);
    if (!video_stream2_ep1)
    {
        rt_kprintf("Unable to allocate streaming EP video_stream2_ep1\n");
        goto error;
    }
    uvc2->ep = video_stream2_ep1;
    /* video_stream2_ep1->driver_data = uvc2; */

    ret = usb_interface_id(c, f);
    if (ret < 0)
        goto error;
    uvc_streaming2_intf_alt0.bInterfaceNumber = ret;
#ifdef FH_RT_UVC_EP_TYPE_ISOC
    uvc_streaming2_intf_alt1.bInterfaceNumber = ret;
#endif
    uvc2->streaming_intf = ret;
#endif
    return 0;

error:
    /* uvc_function_unbind(c, f); */
    rt_kprintf("uvc_function_bind error\n");
    return ret;
}

/**
 * uvc_bind_config - add a UVC function to a configuration
 * @c: the configuration to support the UVC instance
 * Context: single threaded during gadget setup
 *
 * Returns zero on success, else negative errno.
 *
 * Caller must have called @uvc_setup(). Caller is also responsible for
 * calling @uvc_cleanup() before module unload.
 */
char *uvc_interface_string_association;

int
uvc_bind_config(struct uconfig *c)
{
    struct uvc_device *uvc;
    struct uvc_device *uvc1;
#ifdef UVC_DOUBLE_STREAM
    struct uvc_device *uvc2;
#endif
    struct uvc_common *comm;
    ufunction_t  f;

    int ret = 0;

    g_uvcdevice = c->device;

    uvc = rt_calloc(sizeof(*uvc), 1);
    if (uvc == NULL)
        return -ENOMEM;

    uvc->state = UVC_STATE_DISCONNECTED;


    comm = rt_calloc(sizeof(*comm), 1);
    if (comm == NULL)
        return -ENOMEM;

    uvc->comm = comm;
    comm->uvc1 = uvc;
    uvc1 = uvc;
    uvc1->vdev = uvc1;
    uvc1->stream_id = 0;

#ifdef UVC_DOUBLE_STREAM
    uvc2 = rt_calloc(sizeof(*uvc), 1);
    if (uvc == NULL)
        return -ENOMEM;

    uvc2->state = UVC_STATE_DISCONNECTED;
    uvc2->comm = comm;
    comm->uvc2 = uvc2;
    uvc2->vdev = uvc2;
    uvc2->stream_id = 1;
#endif

    uvc->comm->desc.control = (struct uvc_descriptor_header **)uvc_control_cls;

    /* Allocate string descriptor numbers. */
    ret = usb_string_id(c, uvc_interface_string_association);
    if (ret < 0)
        goto error;
    uvc_iad.iFunction = ret;
    uvc_control_intf.iInterface = ret;

    ret = usb_string_id(c, UVC_STRING_STREAMING);
    if (ret < 0)
        goto error;
    uvc_streaming1_intf_alt0.iInterface = ret;
#ifdef FH_RT_UVC_EP_TYPE_ISOC
    uvc_streaming1_intf_alt1.iInterface = ret;
    uvc_streaming1_intf_alt2.iInterface = ret;
    uvc_streaming1_intf_alt3.iInterface = ret;
    uvc_streaming1_intf_alt4.iInterface = ret;
    uvc_streaming1_intf_alt5.iInterface = ret;
    uvc_streaming1_intf_alt6.iInterface = ret;
    uvc_streaming1_intf_alt7.iInterface = ret;
    uvc_streaming1_intf_alt8.iInterface = ret;
#endif

    ret = usb_string_id(c, UVC_STRING_STREAMING_2);
    if (ret < 0)
        goto error;
    uvc_streaming2_intf_alt0.iInterface = ret;
#ifdef FH_RT_UVC_EP_TYPE_ISOC
    uvc_streaming2_intf_alt1.iInterface = ret;
#endif
    /* Register the function. */

    f = rt_usbd_function_new(c->device);
    f->disable = uvc_function_disable;
    f->set_alt = uvc_function_set_alt;
    f->get_alt = uvc_function_get_alt;
    f->setup = uvc_function_setup;

    comm->func = f;
    f->uvc_comm = comm;

    uvc_function_bind(c, f);

    rt_usbd_config_add_function(c, f);

    change_usb_support_fmt(uvc);

    return 0;
error:
    rt_kprintf("######### f_uvc_bind_error#########\n");
    return ret;
}
