/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2012-10-02     Yi Qiu       first version
 * 2012-12-12     heyuanjie87  change endpoints and function handler
 * 2013-06-25     heyuanjie87  remove SOF mechinism
 * 2013-07-20     Yi Qiu       do more test
 * 2016-02-01     Urey         Fix some error
 */

#include <rthw.h>
#include <rtdef.h>
#include <rtthread.h>
#include <rtservice.h>
#include <rtdevice.h>
#include <drivers/serial.h>
#include "../core/usbdevice.h"
#include "ch9.h"
#include "cdc.h"
#include "cdc_vcom.h"
#include <string.h>


#define    CDC_STR_ASSOCIATION "usb-com"
#define    CDC_STR_CONTROL "cdc Control"
#define    CDC_STR_STREAMING "cdc Streaming"

#ifdef RT_VCOM_TX_TIMEOUT
#define VCOM_TX_TIMEOUT      RT_VCOM_TX_TIMEOUT
#else /*!RT_VCOM_TX_TIMEOUT*/
#define VCOM_TX_TIMEOUT      1000
#endif /*RT_VCOM_TX_TIMEOUT*/

#define CDC_RX_BUFSIZE          1024
#define CDC_MAX_PACKET_SIZE     64
#define VCOM_DEVICE             "vcom"

#ifdef RT_VCOM_TASK_STK_SIZE
#define VCOM_TASK_STK_SIZE      RT_VCOM_TASK_STK_SIZE
#else /*!RT_VCOM_TASK_STK_SIZE*/
#define VCOM_TASK_STK_SIZE      4 * 1024
#endif /*RT_VCOM_TASK_STK_SIZE*/

#ifdef RT_VCOM_TX_USE_DMA
#define VCOM_TX_USE_DMA
#endif /*RT_VCOM_TX_USE_DMA*/

#ifdef RT_VCOM_SERNO
#define _SER_NO RT_VCOM_SERNO
#else /*!RT_VCOM_SERNO*/
#define _SER_NO  "32021919830108"
#endif /*RT_VCOM_SERNO*/

#ifdef RT_VCOM_SER_LEN
#define _SER_NO_LEN RT_VCOM_SER_LEN
#else /*!RT_VCOM_SER_LEN*/
#define _SER_NO_LEN 14 /*rt_strlen("32021919830108")*/
#endif /*RT_VCOM_SER_LEN*/

#define CDC_TX_BUFSIZE    1024*4
#define CDC_BULKIN_MAXSIZE (CDC_TX_BUFSIZE / 8)

#define CDC_TX_HAS_DATA   0x01

struct vcom
{
    struct rt_serial_device serial;
    uep_t ep_out;
    uep_t ep_in;
    uep_t ep_cmd;
    rt_bool_t connected;
    rt_bool_t in_sending;
    struct rt_completion wait;
    rt_uint8_t rx_rbp[CDC_RX_BUFSIZE];
    struct rt_ringbuffer rx_ringbuffer;
    rt_uint8_t tx_rbp[CDC_TX_BUFSIZE];
    struct rt_ringbuffer tx_ringbuffer;
    struct rt_event  tx_event;
    rt_uint32_t coolview_mode;
};

struct vcom_tx_msg
{
    struct rt_serial_device *serial;
    const char *buf;
    rt_size_t size;
};

extern void cdc_keyboard_change(int mode);
ALIGN(4)
static rt_uint8_t vcom_thread_stack[VCOM_TASK_STK_SIZE];
static struct rt_thread vcom_thread;
static struct ucdc_line_coding line_coding;
static ufunction_t g_func;


/* communcation interface descriptor */
ALIGN(4)
static struct ucdc_comm_descriptor _comm_desc = {

#ifdef FH_RT_USB_DEVICE_COMPOSITE
    /* Interface Association Descriptor */
    {
        USB_DESC_LENGTH_IAD,
        USB_DESC_TYPE_IAD,
        USB_DYNAMIC,
        0x02,
        USB_CDC_CLASS_COMM,
        USB_CDC_SUBCLASS_ACM,
        USB_CDC_PROTOCOL_V25TER,
        0x00,
    },
#endif
    /* Interface Descriptor */
    {
        USB_DESC_LENGTH_INTERFACE,
        USB_DESC_TYPE_INTERFACE,
        USB_DYNAMIC,
        0x00,
        0x01,
        USB_CDC_CLASS_COMM,
        USB_CDC_SUBCLASS_ACM,
        USB_CDC_PROTOCOL_V25TER,
        0x00,
    },
    /* Header Functional Descriptor */
    {
        0x05,
        USB_CDC_CS_INTERFACE,
        USB_CDC_SCS_HEADER,
        0x0110,
    },
    /* Call Management Functional Descriptor */
    {
        0x05,
        USB_CDC_CS_INTERFACE,
        USB_CDC_SCS_CALL_MGMT,
        0x00,
        USB_DYNAMIC,
    },
    /* Abstract Control Management Functional Descriptor */
    {
        0x04,
        USB_CDC_CS_INTERFACE,
        USB_CDC_SCS_ACM,
        0x02,
    },
    /* Union Functional Descriptor */
    {
        0x05,
        USB_CDC_CS_INTERFACE,
        USB_CDC_SCS_UNION,
        USB_DYNAMIC,
        USB_DYNAMIC,
    },
    /* Endpoint Descriptor */
    {
        USB_DESC_LENGTH_ENDPOINT,
        USB_DESC_TYPE_ENDPOINT,
        USB_DYNAMIC | USB_DIR_IN,
        USB_EP_ATTR_INT,
        0x08,
        0x01,
    },
};

/* data interface descriptor */
ALIGN(4)
static struct ucdc_data_descriptor _data_desc = {

    /* interface descriptor */
    {
        USB_DESC_LENGTH_INTERFACE,
        USB_DESC_TYPE_INTERFACE,
        USB_DYNAMIC,
        0x00,
        0x02,
        USB_CDC_CLASS_DATA,
        0x00,
        0x00,
        0x00,
    },
    /* endpoint, bulk out */
    {
        USB_DESC_LENGTH_ENDPOINT,
        USB_DESC_TYPE_ENDPOINT,
        USB_DYNAMIC | USB_DIR_OUT,
        USB_EP_ATTR_BULK,
        USB_CDC_BUFSIZE,
        0x00,
    },
    /* endpoint, bulk in */
    {
        USB_DESC_LENGTH_ENDPOINT,
        USB_DESC_TYPE_ENDPOINT,
        USB_DYNAMIC | USB_DIR_IN,
        USB_EP_ATTR_BULK,
        USB_CDC_BUFSIZE,
        0x00,
    },
};



static const struct usb_descriptor_header * const cdc_desc[] = {
    (struct usb_descriptor_header *) &_comm_desc.iad_desc,
    (struct usb_descriptor_header *) &_comm_desc.intf_desc,
    (struct usb_descriptor_header *) &_comm_desc.hdr_desc,
    (struct usb_descriptor_header *) &_comm_desc.call_mgmt_desc,
    (struct usb_descriptor_header *) &_comm_desc.acm_desc,
    (struct usb_descriptor_header *) &_comm_desc.union_desc,
    (struct usb_descriptor_header *) &_comm_desc.ep_desc,
    (struct usb_descriptor_header *) &_data_desc.intf_desc,
    (struct usb_descriptor_header *) &_data_desc.ep_in_desc,
    (struct usb_descriptor_header *) &_data_desc.ep_out_desc,
    NULL,
};


ALIGN(4)
static char serno[_SER_NO_LEN + 1] = {'\0'};
RT_WEAK rt_err_t vcom_get_stored_serno(char *serno, int size);

rt_err_t vcom_get_stored_serno(char *serno, int size)
{
    return RT_ERROR;
}

static void _vcom_reset_state(ufunction_t func)
{
    struct vcom *data;
    int lvl;

    RT_ASSERT(func != RT_NULL)

    data = (struct vcom *)func->user_data;

    lvl = rt_hw_interrupt_disable();
    data->connected = RT_FALSE;
    data->in_sending = RT_FALSE;
    /*rt_kprintf("reset USB serial\n", cnt);*/
    rt_hw_interrupt_enable(lvl);
}

/**
 * This function will handle cdc bulk in endpoint request.
 *
 * @param func the usb function object.
 * @param size request size.
 *
 * @return RT_EOK.
 */
static rt_err_t _ep_in_handler(ufunction_t func, rt_size_t size)
{
    struct vcom *data;
    rt_size_t request_size;

    RT_ASSERT(func != RT_NULL);

    data = (struct vcom *)func->user_data;
    request_size = data->ep_in->request.size;
    RT_DEBUG_LOG(RT_DEBUG_USB, ("_ep_in_handler %d\n", request_size));
    if (request_size > 512)
        rt_kprintf("_ep_in_handler %d\n", request_size);
    if ((request_size != 0) && ((request_size % EP_MAXPACKET(data->ep_in)) == 0))
    {
        /* don't have data right now. Send a zero-length-packet to
         * terminate the transaction.
         *
         * FIXME: actually, this might not be the right place to send zlp.
         * Only the rt_device_write could know how much data is sending.
         */
        data->in_sending = RT_TRUE;

        data->ep_in->request.buffer = RT_NULL;
        data->ep_in->request.size = 0;
        data->ep_in->request.req_type = UIO_REQUEST_WRITE;
        rt_usbd_io_request(func->device, data->ep_in, &data->ep_in->request);

        return RT_EOK;
    }

    rt_completion_done(&data->wait);

    return RT_EOK;
}


/**
 * This function will handle cdc bulk out endpoint request.
 *
 * @param func the usb function object.
 * @param size request size.
 *
 * @return RT_EOK.
 */

static rt_err_t _ep_out_handler(ufunction_t func, rt_size_t size)
{
    rt_uint32_t level;
    struct vcom *data;

    RT_ASSERT(func != RT_NULL);

    RT_DEBUG_LOG(RT_DEBUG_USB, ("_ep_out_handler %d\n", size));

    data = (struct vcom *)func->user_data;

    /* ensure serial is active */
    if ((data->serial.parent.flag & RT_DEVICE_FLAG_ACTIVATED)
        && (data->serial.parent.open_flag & RT_DEVICE_OFLAG_OPEN))
    {

        /* receive data from USB VCOM */
        level = rt_hw_interrupt_disable();

        /* if (!DealCoolviewData(data->ep_out->buffer, size)) */
        {
            rt_ringbuffer_put(&data->rx_ringbuffer, data->ep_out->buffer, size);
        }
        rt_hw_interrupt_enable(level);

        /* notify receive data */
        if (data->serial.parent.open_flag & RT_DEVICE_FLAG_INT_RX)
            rt_hw_serial_isr(&data->serial, RT_SERIAL_EVENT_RX_IND);

        /* update cmd */
        if (fh_cdc_mode_get() == FH_CDC_VCOM_MODE &&
            data->ep_out->buffer[0] == 0x99 &&
            data->ep_out->buffer[2] == 4 &&
            data->ep_out->buffer[4] == 0x88 &&
            data->ep_out->buffer[5] == 0x63 &&
            data->ep_out->buffer[6] == 0x6f &&
            data->ep_out->buffer[7] == 0x6d)
        {
            cdc_keyboard_change(FH_CDC_UPDATE_MODE);
        }
        /* keyboard Esc = 1B*/
        if ((fh_cdc_mode_get() == FH_CDC_VCOM_MODE) &&
            size == 1 &&
            data->ep_out->buffer[0] == 0x1B)
        {
            cdc_keyboard_change(FH_CDC_CMD_MODE);
        }
        if ((fh_cdc_mode_get() == FH_CDC_VCOM_MODE) &&
            size == 5 &&
            data->ep_out->buffer[0] == 0x11 && data->ep_out->buffer[1] == 0x98)
        {
            data->coolview_mode = 3;
            cdc_keyboard_change(FH_CDC_COOLVIEW_MODE);
        }
    }

    data->ep_out->request.buffer = data->ep_out->buffer;
    data->ep_out->request.size = EP_MAXPACKET(data->ep_out);
    data->ep_out->request.req_type = UIO_REQUEST_READ_BEST;
    rt_usbd_io_request(func->device, data->ep_out, &data->ep_out->request);

    return RT_EOK;
}

/**
 * This function will handle cdc interrupt in endpoint request.
 *
 * @param device the usb device object.
 * @param size request size.
 *
 * @return RT_EOK.
 */
static rt_err_t _ep_cmd_handler(ufunction_t func, rt_size_t size)
{
    RT_ASSERT(func != RT_NULL);

    RT_DEBUG_LOG(RT_DEBUG_USB, ("_ep_cmd_handler\n"));

    return RT_EOK;
}

/**
 * This function will handle cdc_get_line_coding request.
 *
 * @param device the usb device object.
 * @param setup the setup request.
 *
 * @return RT_EOK on successful.
 */
static rt_err_t _cdc_get_line_coding(udevice_t device, ureq_t setup)
{
    struct ucdc_line_coding data;
    rt_uint16_t size;

    RT_ASSERT(device != RT_NULL);
    RT_ASSERT(setup != RT_NULL);

    RT_DEBUG_LOG(RT_DEBUG_USB, ("_cdc_get_line_coding\n"));

    data.dwDTERate = 115200;
    data.bCharFormat = 0;
    data.bDataBits = 8;
    data.bParityType = 0;
    size = setup->wLength > 7 ? 7 : setup->wLength;
    rt_usbd_ep0_write(device, (void *)&data, size);

    return RT_EOK;
}

static rt_err_t _cdc_set_line_coding_callback(udevice_t device, rt_size_t size)
{
    RT_DEBUG_LOG(RT_DEBUG_USB, ("_cdc_set_line_coding_callback\n"));

    dcd_ep0_send_status(device->dcd);

    return RT_EOK;
}

/**
 * This function will handle cdc_set_line_coding request.
 *
 * @param device the usb device object.
 * @param setup the setup request.
 *
 * @return RT_EOK on successful.
 */
static rt_err_t _cdc_set_line_coding(udevice_t device, ureq_t setup)
{
    RT_ASSERT(device != RT_NULL);
    RT_ASSERT(setup != RT_NULL);

    RT_DEBUG_LOG(RT_DEBUG_USB, ("_cdc_set_line_coding\n"));

    rt_usbd_ep0_read(device, (void *)&line_coding, sizeof(struct ucdc_line_coding),
        _cdc_set_line_coding_callback);

    return RT_EOK;
}

/**
 * This function will handle cdc interface request.
 *
 * @param device the usb device object.
 * @param setup the setup request.
 *
 * @return RT_EOK on successful.
 */
extern int coolview_mode;
static rt_err_t _interface_handler(ufunction_t func, ureq_t setup)
{
    struct vcom *data;

    RT_ASSERT(func != RT_NULL);
    RT_ASSERT(func->device != RT_NULL);
    RT_ASSERT(setup != RT_NULL);

    data = (struct vcom *)func->user_data;

    switch (setup->bRequest)
    {
    case CDC_SEND_ENCAPSULATED_COMMAND:
        break;
    case CDC_GET_ENCAPSULATED_RESPONSE:
        break;
    case CDC_SET_COMM_FEATURE:
        break;
    case CDC_GET_COMM_FEATURE:
        break;
    case CDC_CLEAR_COMM_FEATURE:
        break;
    case CDC_SET_LINE_CODING:
        _cdc_set_line_coding(func->device, setup);
        data->connected = 1;
        break;
    case CDC_GET_LINE_CODING:
        _cdc_get_line_coding(func->device, setup);
        break;
    case CDC_SET_CONTROL_LINE_STATE:
        data->connected = (setup->wValue & 0x01) > 0?RT_TRUE:RT_FALSE;
        if (!data->connected)
        {
            if (!data->coolview_mode && !coolview_mode)
            {
                cdc_keyboard_change(FH_CDC_VCOM_MODE);
            }
            else
            {
                if (data->coolview_mode)
                    data->coolview_mode--;
                if (coolview_mode)
                    coolview_mode--;
            }

        }
        dcd_ep0_send_status(func->device->dcd);
        break;
    case CDC_SEND_BREAK:
        break;
    default:
        rt_kprintf("unknown cdc request\n",setup->bRequestType);
        return -RT_ERROR;
    }

    return RT_EOK;
}

/**
 * This function will run cdc function, it will be called on handle set configuration request.
 *
 * @param func the usb function object.
 *
 * @return RT_EOK on successful.
 */
static rt_err_t _function_enable(ufunction_t func)
{
    struct vcom *data;

    RT_ASSERT(func != RT_NULL);

    RT_DEBUG_LOG(RT_DEBUG_USB, ("cdc function enable\n"));

    _vcom_reset_state(func);

    data = (struct vcom *)func->user_data;
    data->ep_out->buffer = rt_malloc(CDC_RX_BUFSIZE);

    data->ep_out->request.buffer = data->ep_out->buffer;
    data->ep_out->request.size = EP_MAXPACKET(data->ep_out);

    data->ep_out->request.req_type = UIO_REQUEST_READ_BEST;
    rt_usbd_io_request(func->device, data->ep_out, &data->ep_out->request);

    return RT_EOK;
}

/**
 * This function will stop cdc function, it will be called on handle set configuration request.
 *
 * @param func the usb function object.
 *
 * @return RT_EOK on successful.
 */
static rt_err_t _function_disable(ufunction_t func)
{
    struct vcom *data;

    RT_ASSERT(func != RT_NULL);

    RT_DEBUG_LOG(RT_DEBUG_USB, ("cdc function disable\n"));

    _vcom_reset_state(func);

    data = (struct vcom *)func->user_data;
    if (data->ep_out->buffer != RT_NULL)
    {
        rt_free(data->ep_out->buffer);
        data->ep_out->buffer = RT_NULL;
    }

    return RT_EOK;
}

/**
 * This function will configure cdc descriptor.
 *
 * @param comm the communication interface number.
 * @param data the data interface number.
 *
 * @return RT_EOK on successful.
 */

#define UVC_COPY_DESCRIPTORS(mem, dst, src) \
        do \
        { \
            const struct usb_descriptor_header * const *__src; \
            for (__src = src; *__src; ++__src) \
            { \
                rt_memcpy(mem, *__src, (*__src)->bLength); \
                *dst++ = mem; \
                mem += (*__src)->bLength; \
            } \
        } while (0)

static void cdc_copy_descriptors(ufunction_t f)
{
    const struct usb_descriptor_header * const *src;
    struct usb_descriptor_header **dst;
    struct usb_descriptor_header **hdr;
    unsigned int n_desc = 0;
    unsigned int bytes = 0;
    void *mem;
    int needBufLen;


    for (src = cdc_desc; *src; ++src)
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
    f->other_descriptors = mem;
    f->other_desc_size = bytes;

    /* Copy the descriptors. */
    UVC_COPY_DESCRIPTORS(mem, dst, cdc_desc);
    *dst = NULL;

}



static rt_err_t
cdc_function_set_alt(struct ufunction *f, unsigned int interface, unsigned int alt)
{
    struct udevice *device = f->device;
    struct vcom *data = f->user_data;

    dcd_ep_disable(device->dcd, data->ep_in);
    dcd_ep_enable(device->dcd, data->ep_in);

    dcd_ep_disable(device->dcd, data->ep_out);
    dcd_ep_enable(device->dcd, data->ep_out);

    dcd_ep_disable(device->dcd, data->ep_cmd);
    dcd_ep_enable(device->dcd, data->ep_cmd);

    return 0;
}



uep_t usb_ep_autoconfig(ufunction_t f, uep_desc_t ep_desc, udep_handler_t handler);


/**
 * This function will create a cdc function instance.
 *
 * @param device the usb device object.
 *
 * @return RT_EOK on successful.
 */
int cdc_bind_config(uconfig_t c)
{
    ufunction_t f;
    struct vcom *data;
    udevice_t device = c->device;
    int ret = 0;

    rt_memset(serno, 0, _SER_NO_LEN + 1);
    if (vcom_get_stored_serno(serno, _SER_NO_LEN) != RT_EOK)
    {
        rt_memset(serno, 0, _SER_NO_LEN + 1);
        rt_memcpy(serno, _SER_NO, rt_strlen(_SER_NO));
    }
    ret = usb_string_id(c, CDC_STR_ASSOCIATION);
    if (ret < 0)
        goto error;
    _comm_desc.iad_desc.iFunction = ret;

    ret = usb_string_id(c, CDC_STR_CONTROL);
    if (ret < 0)
        goto error;
    _comm_desc.intf_desc.iInterface = ret;

    ret = usb_string_id(c, CDC_STR_STREAMING);
    if (ret < 0)
        goto error;
    _data_desc.intf_desc.iInterface = ret;

    f = rt_usbd_function_new(device);
    f->enable = _function_enable;
    f->disable = _function_disable;
    f->setup = _interface_handler;
    f->set_alt = cdc_function_set_alt;
    g_func = f;

    data = (struct vcom *)rt_malloc(sizeof(struct vcom));
    rt_memset(data, 0, sizeof(struct vcom));
    f->user_data = (void *)data;

    ret = usb_interface_id(c, f);
    _comm_desc.iad_desc.bFirstInterface = ret;
    _comm_desc.intf_desc.bInterfaceNumber = ret;

     ret = usb_interface_id(c, f);
    _comm_desc.call_mgmt_desc.data_interface = ret;
    _comm_desc.union_desc.master_interface = _comm_desc.iad_desc.bFirstInterface;
    _comm_desc.union_desc.slave_interface0 = ret;
    _data_desc.intf_desc.bInterfaceNumber = ret;


    data->ep_cmd = usb_ep_autoconfig(f, &_comm_desc.ep_desc, _ep_cmd_handler);
    data->ep_out = usb_ep_autoconfig(f, &_data_desc.ep_out_desc, _ep_out_handler);
    data->ep_in = usb_ep_autoconfig(f, &_data_desc.ep_in_desc, _ep_in_handler);

    cdc_copy_descriptors(f);

    rt_usbd_config_add_function(c, f);

    return 0;
error:
    rt_kprintf("error: uac bind config\n");
    return ret;
}

/**
 * UART device in RT-Thread
 */
static rt_err_t _vcom_configure(struct rt_serial_device *serial,
                                struct serial_configure *cfg)
{
    return RT_EOK;
}

static rt_err_t _vcom_control(struct rt_serial_device *serial,
                              int cmd, void *arg)
{
    switch (cmd)
    {
    case RT_DEVICE_CTRL_CLR_INT:
        /* disable rx irq */
        break;
    case RT_DEVICE_CTRL_SET_INT:
        /* enable rx irq */
        break;
    }

    return RT_EOK;
}

static int _vcom_getc(struct rt_serial_device *serial)
{
    int result;
    rt_uint8_t ch;
    rt_uint32_t level;
    struct ufunction *func;
    struct vcom *data;

    func = (struct ufunction *)serial->parent.user_data;
    data = (struct vcom *)func->user_data;

    result = -1;

    level = rt_hw_interrupt_disable();

    if (rt_ringbuffer_getchar(&data->rx_ringbuffer, &ch) != 0)
    {
        result = ch;
    }

    rt_hw_interrupt_enable(level);

    return result;
}
static rt_size_t _vcom_tx(struct rt_serial_device *serial, rt_uint8_t *buf, rt_size_t size, int direction)
{
    rt_uint32_t level;
    struct ufunction *func;
    struct vcom *data;
    rt_uint32_t baksize;
    rt_size_t ptr = 0;
    int empty = 0;
    rt_uint8_t crlf[2] = {'\r', '\n',};

    func = (struct ufunction *)serial->parent.user_data;
    data = (struct vcom *)func->user_data;

    size = (size >= CDC_BULKIN_MAXSIZE) ? CDC_BULKIN_MAXSIZE : size;
    baksize = size;

    RT_ASSERT(serial != RT_NULL);
    RT_ASSERT(buf != RT_NULL);

    RT_DEBUG_LOG(RT_DEBUG_USB, ("%s\n",__func__));

    if (data->connected)
    {
        size = 0;
        if ((serial->parent.open_flag & RT_DEVICE_FLAG_STREAM))
        {
            empty = 0;
            while (ptr < baksize)
            {
                while (ptr < baksize && buf[ptr] != '\n')
                {
                    ptr++;
                }
                if (ptr < baksize)
                {
                    level = rt_hw_interrupt_disable();
                    size += rt_ringbuffer_put_force(&data->tx_ringbuffer, (const rt_uint8_t *)&buf[size], ptr - size);
                    rt_hw_interrupt_enable(level);

                    /* no data was be ignored */
                    if (size == ptr)
                    {
                        level = rt_hw_interrupt_disable();
                        if (rt_ringbuffer_space_len(&data->tx_ringbuffer) >= 2)
                        {
                            rt_ringbuffer_put_force(&data->tx_ringbuffer, crlf, 2);
                            size++;
                        }
                        rt_hw_interrupt_enable(level);
                    }
                    else
                    {
                        empty = 1;
                        break;
                    }

                    /* ring buffer is full */
                    if (size == ptr)
                    {
                        empty = 1;
                        break;
                    }
                    ptr++;
                }
                else
                {
                    break;
                }
            }
        }

        if (size < baksize && !empty)
        {
            level = rt_hw_interrupt_disable();
            size += rt_ringbuffer_put_force(&data->tx_ringbuffer, (rt_uint8_t *)&buf[size], baksize - size);
            rt_hw_interrupt_enable(level);
        }

        if (size)
        {
            rt_event_send(&data->tx_event, CDC_TX_HAS_DATA);
        }
    }
    else
    {
        /* recover dataqueue resources */
        rt_hw_serial_isr(&data->serial, RT_SERIAL_EVENT_TX_DMADONE);
    }

    return size;
}
static int _vcom_putc(struct rt_serial_device *serial, char c)
{
    rt_uint32_t level;
    struct ufunction *func;
    struct vcom *data;

    func = (struct ufunction *)serial->parent.user_data;
    data = (struct vcom *)func->user_data;

    RT_ASSERT(serial != RT_NULL);

    if (data->connected)
    {
        if (c == '\n' && (serial->parent.open_flag & RT_DEVICE_FLAG_STREAM))
        {
            level = rt_hw_interrupt_disable();
            rt_ringbuffer_putchar_force(&data->tx_ringbuffer, '\r');
            rt_hw_interrupt_enable(level);
            rt_event_send(&data->tx_event, CDC_TX_HAS_DATA);
        }
        level = rt_hw_interrupt_disable();
        rt_ringbuffer_putchar_force(&data->tx_ringbuffer, c);
        rt_hw_interrupt_enable(level);
        rt_event_send(&data->tx_event, CDC_TX_HAS_DATA);
    }

    return 1;
}

static const struct rt_uart_ops usb_vcom_ops = {

    _vcom_configure,
    _vcom_control,
    _vcom_putc,
    _vcom_getc,
    _vcom_tx
};

/* Vcom Tx Thread */
static void vcom_tx_thread_entry(void *parameter)
{
    rt_uint32_t level;
    rt_uint32_t res = 0;
    struct ufunction *func = (struct ufunction *)parameter;
    struct vcom *data = (struct vcom *)func->user_data;
    rt_uint8_t ch[CDC_BULKIN_MAXSIZE];

    while (1)
    {
        if ((rt_event_recv(&data->tx_event, CDC_TX_HAS_DATA,
                    RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR,
                    RT_WAITING_FOREVER, &res) != RT_EOK)
                    || (!(res & CDC_TX_HAS_DATA)))
        {
            continue;
        }

        if (!(res & CDC_TX_HAS_DATA))
        {
            continue;
        }

        while (rt_ringbuffer_data_len(&data->tx_ringbuffer))
        {
            level = rt_hw_interrupt_disable();
            res = rt_ringbuffer_get(&data->tx_ringbuffer, ch, CDC_BULKIN_MAXSIZE);
            rt_hw_interrupt_enable(level);

            if (!res)
            {
                continue;
            }
            if (!data->connected)
            {
                if (data->serial.parent.open_flag &
#ifndef VCOM_TX_USE_DMA
                         RT_DEVICE_FLAG_INT_TX
#else
                         RT_DEVICE_FLAG_DMA_TX
#endif
                )
                {
                /* drop msg */
#ifndef VCOM_TX_USE_DMA
                    rt_hw_serial_isr(&data->serial, RT_SERIAL_EVENT_TX_DONE);
#else
                    rt_hw_serial_isr(&data->serial, RT_SERIAL_EVENT_TX_DMADONE);
#endif
                }
                continue;
            }
            rt_completion_init(&data->wait);
            data->ep_in->request.buffer     = ch;
            data->ep_in->request.size       = res;

            data->ep_in->request.req_type   = UIO_REQUEST_WRITE;

            rt_usbd_io_request(func->device, data->ep_in, &data->ep_in->request);

            if (rt_completion_wait(&data->wait, VCOM_TX_TIMEOUT) != RT_EOK)
            {
                RT_DEBUG_LOG(RT_DEBUG_USB, ("vcom tx timeout\n"));
            }
            if (data->serial.parent.open_flag &
#ifndef VCOM_TX_USE_DMA
                         RT_DEVICE_FLAG_INT_TX
#else
                         RT_DEVICE_FLAG_DMA_TX
#endif
            )
            {
#ifndef VCOM_TX_USE_DMA
                rt_hw_serial_isr(&data->serial, RT_SERIAL_EVENT_TX_DONE);
#else
                rt_hw_serial_isr(&data->serial, RT_SERIAL_EVENT_TX_DMADONE);
#endif
            }
        }

    }
}

void fh_cdc_init(void)
{
    rt_err_t result = RT_EOK;
    struct serial_configure config;
    struct vcom *data = NULL;

    rt_usb_device_init();

    data = (struct vcom *)g_func->user_data;
    if (!data)
    {
        rt_kprintf("cdc vcom data is NULL!\n");
        return;
    }
    /* initialize ring buffer */
    rt_ringbuffer_init(&data->rx_ringbuffer, data->rx_rbp, CDC_RX_BUFSIZE);
    rt_ringbuffer_init(&data->tx_ringbuffer, data->tx_rbp, CDC_TX_BUFSIZE);

    rt_event_init(&data->tx_event, "vcom", RT_IPC_FLAG_FIFO);

    config.baud_rate    = BAUD_RATE_115200;
    config.data_bits    = DATA_BITS_8;
    config.stop_bits    = STOP_BITS_1;
    config.parity       = PARITY_NONE;
    config.bit_order    = BIT_ORDER_LSB;
    config.invert       = NRZ_NORMAL;
    config.bufsz        = CDC_RX_BUFSIZE;

    data->serial.ops        = &usb_vcom_ops;
    data->serial.serial_rx  = RT_NULL;
    data->serial.config     = config;

    /* register vcom device */
    rt_hw_serial_register(&data->serial, VCOM_DEVICE,
#ifndef VCOM_TX_USE_DMA
                          RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_INT_RX | RT_DEVICE_FLAG_INT_TX,
#else
                          RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_INT_RX | RT_DEVICE_FLAG_DMA_TX,
#endif
                          g_func);

    /* init usb device thread */
    rt_thread_init(&vcom_thread, "vcom",
                   vcom_tx_thread_entry, g_func,
                   vcom_thread_stack, VCOM_TASK_STK_SIZE,
                   16, 20);
    result = rt_thread_startup(&vcom_thread);
    RT_ASSERT(result == RT_EOK);

    extern void cdc_shell_check_init(void);
    cdc_shell_check_init();
}

