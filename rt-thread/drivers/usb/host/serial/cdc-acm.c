#include "usb_storage.h"
#include "usb_serial.h"
#include <usb.h>
#include <rtdef.h>
#include <rtthread.h>
#include <rtdevice.h>
#include <rthw.h>
#include "usb_errno.h"
#include <dma_mem.h>
#include <interrupt.h>
#include <sys/types.h>
#include <cdc.h>
#include <usbnet.h>
#include "cdc-acm.h"
#include "usb_serial_skel.h"
#include "fh_usb_serial_api.h"

#define FH_DMA_ALIGNED_SIZE 32

#ifndef RT_USING_USB_SERIAL_POSIX
static struct acm *g_acm_us_data[2];
#endif

#define ACM_ID_PREFIX "1-1:1."
#define ACM_NUM_NONE (127)
#define ACM_NUM_AT(info) ((unsigned long)info & 0xff)
#define ACM_NUM_PT(info) (((unsigned long)info >> 8) & 0xff)
#define ACM_NUM_INFO(at, pt)  ((pt << 8) | at)

static struct usb_device_id acm_ids[] = {
    { USB_DEVICE(0x2949, 0x8700), .driver_info = ACM_NUM_INFO(4, 6),},

    /* control interfaces without any protocol set */
    /* { USB_INTERFACE_INFO(USB_CLASS_COMM, USB_CDC_SUBCLASS_ACM, */
    /* USB_CDC_PROTO_NONE) }, */

    /* control interfaces with various AT-command sets */
    /* { USB_INTERFACE_INFO(USB_CLASS_COMM, USB_CDC_SUBCLASS_ACM, */
    /* USB_CDC_ACM_PROTO_AT_V25TER) }, */
    /* { USB_INTERFACE_INFO(USB_CLASS_COMM, USB_CDC_SUBCLASS_ACM, */
    /* USB_CDC_ACM_PROTO_AT_PCCA101) }, */
    /* { USB_INTERFACE_INFO(USB_CLASS_COMM, USB_CDC_SUBCLASS_ACM, */
    /* USB_CDC_ACM_PROTO_AT_PCCA101_WAKE) }, */
    /* { USB_INTERFACE_INFO(USB_CLASS_COMM, USB_CDC_SUBCLASS_ACM, */
    /* USB_CDC_ACM_PROTO_AT_GSM) }, */
    /* { USB_INTERFACE_INFO(USB_CLASS_COMM, USB_CDC_SUBCLASS_ACM, */
    /* USB_CDC_ACM_PROTO_AT_3G) }, */
    /* { USB_INTERFACE_INFO(USB_CLASS_COMM, USB_CDC_SUBCLASS_ACM, */
    /* USB_CDC_ACM_PROTO_AT_CDMA) }, */

    { }
};

static inline int usb_interface_claimed(struct usb_interface *iface)
{
    return (iface->driver != NULL);
}

static int acm_ctrl_msg(struct acm *acm, int request, int value,
                            void *buf, int len)
{
    int retval = usb_control_msg(acm->dev, usb_sndctrlpipe(acm->dev, 0),
        request, USB_RT_ACM, value,
        acm->control->altsetting[0].desc.bInterfaceNumber,
        buf, len, 5000);
    dbg("%s - rq 0x%02x, val %#x, len %#x, result %d\n",
            __func__, request, value, len, retval);
    return retval < 0 ? retval : 0;
}

/* devices aren't required to support these requests.
 * the cdc acm descriptor tells whether they do...
 */
#define acm_set_control(acm, control) \
    acm_ctrl_msg(acm, USB_CDC_REQ_SET_CONTROL_LINE_STATE, control, NULL, 0)
#define acm_set_line(acm, line) \
    acm_ctrl_msg(acm, USB_CDC_REQ_SET_LINE_CODING, 0, line, sizeof *(line))
#define acm_send_break(acm, ms) \
    acm_ctrl_msg(acm, USB_CDC_REQ_SEND_BREAK, ms, NULL, 0)

/*
 * Write buffer management.
 * All of these assume proper locks taken by the caller.
 */
static int acm_wb_alloc(struct acm *acm)
{
    int i;
    struct acm_wb *wb;

    for (i = 0; i < ACM_NW; i++)
    {
        wb = &acm->wb[i];
        if (!wb->use)
        {
            wb->use = 1;
            return i;
        }
    }

    return -1;
}

static void acm_write_done(struct acm *acm, struct acm_wb *wb)
{
    wb->use = 0;
    acm->transmitting--;
}

static int acm_start_wb(struct acm *acm, struct acm_wb *wb)
{
    int rc;

    acm->transmitting++;

    wb->urb->transfer_buffer = wb->buf;
    /* wb->urb->transfer_dma = wb->dmah; */
    wb->urb->transfer_buffer_length = wb->len;
    wb->urb->dev = acm->dev;

    rc = usb_submit_urb(wb->urb, GFP_ATOMIC);
    if (rc < 0)
    {
        rt_kprintf("%s - usb_submit_urb(write bulk) failed: %d\n",
            __func__, rc);
        acm_write_done(acm, wb);
    }
    return rc;
}

static int acm_write_start(struct acm *acm, int wbn)
{
    struct acm_wb *wb = &acm->wb[wbn];
    int rc;
    rt_base_t level;

    level = rt_hw_interrupt_disable();
    if (!acm->dev)
    {
        wb->use = 0;
        rt_hw_interrupt_enable(level);
        return -ENODEV;
    }

    dbg("%s - susp_count %d\n", __func__,
                            acm->susp_count);
    if (acm->susp_count)
    {
        if (!acm->delayed_wb)
            acm->delayed_wb = wb;
        rt_hw_interrupt_enable(level);
        return 0;    /* A white lie */
    }
    rc = acm_start_wb(acm, wb);
    rt_hw_interrupt_enable(level);

    return rc;

}
/* control interface reports status changes with "interrupt" transfers */
static void acm_ctrl_irq(struct urb *urb)
{
    struct acm *acm = urb->context;
    struct usb_cdc_notification *dr = urb->transfer_buffer;
    unsigned char *data;
    int newctrl;
    int retval;
    int status = urb->status;

    switch (status)
    {
    case 0:
        /* success */
        break;
    case -ECONNRESET:
    case -ENOENT:
    case -ESHUTDOWN:
        /* this urb is terminated, clean up */
        dbg("%s - urb shutting down with status: %d\n",
                __func__, status);
        return;
    default:
        dbg("%s - nonzero urb status received: %d\n",
                __func__, status);
        goto exit;
    }

    data = (unsigned char *)(dr + 1);
    switch (dr->bNotificationType)
    {
    case USB_CDC_NOTIFY_NETWORK_CONNECTION:
        dbg("%s - network connection: %d\n",
                            __func__, dr->wValue);
        break;

    case USB_CDC_NOTIFY_SERIAL_STATE:
        newctrl = data[0] | data[1] << 8;

        acm->ctrlin = newctrl;

        dbg("%s - input control lines: dcd%c dsr%c break%c "
            "ring%c framing%c parity%c overrun%c\n",
            __func__,
            acm->ctrlin & ACM_CTRL_DCD ? '+' : '-',
            acm->ctrlin & ACM_CTRL_DSR ? '+' : '-',
            acm->ctrlin & ACM_CTRL_BRK ? '+' : '-',
            acm->ctrlin & ACM_CTRL_RI  ? '+' : '-',
            acm->ctrlin & ACM_CTRL_FRAMING ? '+' : '-',
            acm->ctrlin & ACM_CTRL_PARITY ? '+' : '-',
            acm->ctrlin & ACM_CTRL_OVERRUN ? '+' : '-');
            break;

    default:
        dbg("%s - unknown notification %d received: index %d "
            "len %d data0 %d data1 %d\n",
            __func__,
            dr->bNotificationType, dr->wIndex,
            dr->wLength, data[0], data[1]);
        break;
    }
exit:
    retval = usb_submit_urb(urb, GFP_ATOMIC);
    if (retval)
        rt_kprintf("%s - usb_submit_urb failed: %d\n",
                            __func__, retval);
}

#ifdef RT_USING_USB_SERIAL_POSIX
static rt_list_t rx_urb_list = RT_LIST_OBJECT_INIT(rx_urb_list);
static rt_sem_t  rx_full_sem;
static int _serial_rx_push(struct urb *urb);
#endif

static int acm_submit_read_urb(struct acm *acm, int index, gfp_t mem_flags)
{
    int res;

    if (!test_and_clear_bit(index, &acm->read_urbs_free))
        return 0;

    dbg("%s - urb %d\n", __func__, index);
    res = usb_submit_urb(acm->read_urbs[index], mem_flags);
    if (res)
    {
        if (res != -EPERM)
        {
            rt_kprintf("%s - usb_submit_urb failed: %d\n",
                    __func__, res);
        }
        set_bit(index, &acm->read_urbs_free);
        return res;
    }

    return 0;
}

static int acm_submit_read_urbs(struct acm *acm, gfp_t mem_flags)
{
    int res;
    int i;

    for (i = 0; i < acm->rx_buflimit; ++i)
    {
        res = acm_submit_read_urb(acm, i, mem_flags);
        if (res)
            return res;
    }

    return 0;
}

static void acm_read_bulk_callback(struct urb *urb)
{
    struct acm_rb *rb = urb->context;
    struct acm *acm = rb->instance;

    set_bit(rb->index, &acm->read_urbs_free);

    if (!acm->dev)
    {
        rt_kprintf("%s - disconnected\n", __func__);
        return;
    }

    if (urb->status)
    {
        rt_kprintf("%s - non-zero urb status: %d\n",
                            __func__, urb->status);
        return;
    }
#ifndef RT_USING_USB_SERIAL_POSIX
    urb->status = rb->serial_port_id;/*just use it*/
    usb_serial_rx_push(urb);
#else
    _serial_rx_push(urb);
#endif

}

static void usb_acm_submit_read_urb(struct urb *urb)
{
    struct acm_rb *rb = urb->context;
    struct acm *acm = rb->instance;
    rt_base_t level;
    /* throttle device if requested by tty */
    level = rt_hw_interrupt_disable();
    acm->throttled = acm->throttle_req;
    if (!acm->throttled && !acm->susp_count)
    {
        rt_hw_interrupt_enable(level);
        acm_submit_read_urb(acm, rb->index, GFP_ATOMIC);
    } else
    {
        rt_hw_interrupt_enable(level);
    }
}

/* data interface wrote those outgoing bytes */
static void acm_write_bulk(struct urb *urb)
{
    struct acm_wb *wb = urb->context;
    struct acm *acm = wb->instance;
    rt_base_t level;

    if (urb->status    || (urb->actual_length != urb->transfer_buffer_length))
        dbg("%s - len %d/%d, status %d\n",
            __func__,
            urb->actual_length,
            urb->transfer_buffer_length,
            urb->status);

    level = rt_hw_interrupt_disable();
    acm_write_done(acm, wb);
    rt_hw_interrupt_enable(level);
    if (acm->write_urb_sem)
        rt_sem_release(acm->write_urb_sem);
}

static void acm_write_buffers_free(struct acm *acm)
{
    int i;
    struct acm_wb *wb;

    for (wb = &acm->wb[0], i = 0; i < ACM_NW; i++, wb++)
    {
        rt_free(wb->buf);
        wb->buf = RT_NULL;
    }
}

static void acm_read_buffers_free(struct acm *acm)
{
    int i;

    for (i = 0; i < acm->rx_buflimit; i++)
    {
        rt_free(acm->read_buffers[i].base);
        acm->read_buffers[i].base = RT_NULL;
    }
}

/* Little helper: write buffers allocate */
static int acm_write_buffers_alloc(struct acm *acm)
{
    int i;
    struct acm_wb *wb;

    for (wb = &acm->wb[0], i = 0; i < ACM_NW; i++, wb++)
    {
        wb->buf = rt_malloc(acm->writesize);
        if (!wb->buf)
        {
            while (i != 0)
            {
                --i;
                --wb;
                rt_free(wb->buf);
                wb->buf = RT_NULL;
            }
            return -ENOMEM;
        }
    }
    return 0;
}

#ifndef RT_USING_USB_SERIAL_POSIX
static int acm_open(FH_USB_SERIAL_PORT port)
{
    struct acm *acm = g_acm_us_data[port];

    if (!acm)
        return -1;

    if (!acm->write_urb_sem)
    {
        acm->write_urb_sem = rt_sem_create("acm_write_urb", ACM_NW, RT_IPC_FLAG_FIFO);
        if (!acm->write_urb_sem)
        {
            rt_kprintf("acm: create acm_write_urb failed!\n");
            return -2;
        }
    }

    if (acm->opend == 0)
    {
        acm->ctrlurb->dev = acm->dev;
        if (usb_submit_urb(acm->ctrlurb, GFP_KERNEL))
        {
            rt_kprintf("%s - usb_submit_urb(ctrl irq) failed\n", __func__);
            goto bail_out;
        }
        acm->ctrlout = ACM_CTRL_DTR | ACM_CTRL_RTS;
        if (acm_set_control(acm, acm->ctrlout) < 0 &&
            (acm->ctrl_caps & USB_CDC_CAP_LINE))
            goto bail_out;

        if (acm_submit_read_urbs(acm, GFP_KERNEL))
            goto bail_out;
        acm->opend = 1;
        /*rt_kprintf("%s---%d: acm device open!\n", __func__, __LINE__);*/
    }

    return 0;

bail_out:
    return -EIO;
}

static int acm_close(FH_USB_SERIAL_PORT port)
{
    struct acm *acm = g_acm_us_data[port];
    int i;

    if (!acm)
        return -1;

    if (acm->opend == 1)
    {
        for (i = 0; i < acm->rx_buflimit; i++)
        {
            set_bit(i, &acm->read_urbs_free);
            usb_kill_urb(acm->read_urbs[i]);
        }
        acm->opend = 0;
        if (acm->write_urb_sem)
        {
            rt_sem_delete(acm->write_urb_sem);
            acm->write_urb_sem = NULL;
        }
    }
    return 0;

}

static int acm_write(FH_USB_SERIAL_PORT port, void *buf, int len)
{
    int stat;
    int wbn;
    struct acm_wb *wb;
    rt_base_t level;
    struct acm *acm = g_acm_us_data[port];

    if (!acm)
        return -1;

    if (acm->write_urb_sem)
    {
        rt_sem_take(acm->write_urb_sem, RT_WAITING_FOREVER);
    }

    level = rt_hw_interrupt_disable();
    wbn = acm_wb_alloc(acm);
    if (wbn < 0)
    {
        rt_hw_interrupt_enable(level);
        return 0;
    }
    wb = &acm->wb[wbn];

    dbg("%s - write %d\n", __func__, len);
    rt_memcpy(wb->buf, buf, len);
    wb->len = len;
    rt_hw_interrupt_enable(level);
    stat = acm_write_start(acm, wbn);
    if (stat < 0)
    {
        rt_kprintf("%s - acm_write_start failed %d\n", __func__, stat);
        return stat;
    }
    return len;
}

static struct usb_serial_ops g_usb_acm_ops = {
    .name  = "cdc-acm",
    .open  = acm_open,
    .close  = acm_close,
    .write = acm_write,
    .rx_back = usb_acm_submit_read_urb,
};

#else
static int _serial_rx_push(struct urb *urb)
{
    register rt_base_t temp;

    temp = rt_hw_interrupt_disable();

    rt_list_init(&urb->urb_list);
    rt_list_insert_before(&rx_urb_list, &urb->urb_list);
    rt_sem_release(rx_full_sem);

    rt_hw_interrupt_enable(temp);

    return 0;
}

static struct urb *_serial_rx_pop(void)
{
    struct urb *urb;

    register rt_base_t temp;

    temp = rt_hw_interrupt_disable();

    if (rt_list_isempty(&rx_urb_list))
    {
        rt_hw_interrupt_enable(temp);
        return RT_NULL;
    }

    urb = rt_list_entry(rx_urb_list.next, struct urb, urb_list);
    rt_list_remove(&urb->urb_list);

    rt_hw_interrupt_enable(temp);

    return urb;
}

static rt_err_t _serial_open(rt_device_t dev, rt_uint16_t oflag)
{
    struct acm *acm = (struct acm *)dev;

    if (!acm)
        return -1;

    if (!acm->write_urb_sem)
    {
        acm->write_urb_sem = rt_sem_create("acm_write_urb", ACM_NW, RT_IPC_FLAG_FIFO);
        if (!acm->write_urb_sem)
        {
            rt_kprintf("acm: create acm_write_urb failed!\n");
            return -2;
        }
    }

    if (acm->opend == 0)
    {
        acm->ctrlurb->dev = acm->dev;
        if (usb_submit_urb(acm->ctrlurb, GFP_KERNEL))
        {
            rt_kprintf("%s - usb_submit_urb(ctrl irq) failed\n", __func__);
            goto bail_out;
        }
        acm->ctrlout = ACM_CTRL_DTR | ACM_CTRL_RTS;
        if (acm_set_control(acm, acm->ctrlout) < 0 &&
            (acm->ctrl_caps & USB_CDC_CAP_LINE))
            goto bail_out;

        if (acm_submit_read_urbs(acm, GFP_KERNEL))
            goto bail_out;
        acm->opend = 1;
        /*rt_kprintf("%s---%d: acm device open!\n", __func__, __LINE__);*/
    }

    return 0;

bail_out:
    return -EIO;
}

static rt_err_t _serial_close(rt_device_t dev)
{
    struct acm *acm = (struct acm *)dev;
    int i;

    if (!acm)
        return -1;

    if (acm->opend == 1)
    {
        for (i = 0; i < acm->rx_buflimit; i++)
        {
            set_bit(i, &acm->read_urbs_free);
            usb_kill_urb(acm->read_urbs[i]);
        }
        acm->opend = 0;
        if (acm->write_urb_sem)
        {
            rt_sem_delete(acm->write_urb_sem);
            acm->write_urb_sem = NULL;
        }
    }
    return 0;
}

static rt_size_t _serial_write(rt_device_t dev, rt_off_t pos, const void *buffer, rt_size_t size)
{
    struct acm *acm = (struct acm *)dev;
    int stat;
    int wbn;
    struct acm_wb *wb;
    rt_base_t level;

    if (!acm)
        return -1;

    if (size > acm->writesize)
        size = acm->writesize;

    if (acm->write_urb_sem)
    {
        rt_sem_take(acm->write_urb_sem, RT_WAITING_FOREVER);
    }

    level = rt_hw_interrupt_disable();
    wbn = acm_wb_alloc(acm);
    if (wbn < 0)
    {
        rt_hw_interrupt_enable(level);
        return 0;
    }
    wb = &acm->wb[wbn];

    dbg("%s - write %d\n", __func__, size);
    rt_memcpy(wb->buf, buffer, size);
    wb->len = size;
    rt_hw_interrupt_enable(level);
    stat = acm_write_start(acm, wbn);
    if (stat < 0)
    {
        rt_kprintf("%s - acm_write_start failed %d\n", __func__, stat);
        return stat;
    }
    return size;
}

static rt_size_t _serial_read(rt_device_t dev, rt_off_t pos, void *buffer, rt_size_t size)
{
    struct acm *acm = (struct acm *)dev;
    struct urb *urb;

    if (!acm)
        return -1;

    if (acm->opend == 0)
    {
        rt_kprintf("cdc device not open !\n");
        return -1;
    }
    rt_sem_take(rx_full_sem, RT_WAITING_FOREVER);
    urb = _serial_rx_pop();
    if (!urb) /*It will not happen*/
    {
        rt_thread_delay(1);
        return -1;
    }

    if (urb->transfer_buffer && urb->actual_length)
    {
        if (size > urb->actual_length)
            size = urb->actual_length;

        rt_memcpy(buffer, urb->transfer_buffer, size);
    }

    urb->status = 0;
    usb_acm_submit_read_urb(urb);
    return size;
}

#ifdef RT_USING_DEVICE_OPS
const static struct rt_device_ops cdc_device_ops =
{
    RT_NULL,
    _serial_open,
    _serial_close,
    _serial_read,
    _serial_write,
    RT_NULL,
};
#endif/* RT_USING_DEVICE_OPS */
#endif
static int usb_acm_probe(struct usb_interface *intf,
             const struct usb_device_id *id)
{
    struct usb_cdc_union_desc *union_header = NULL;
    struct usb_cdc_call_mgmt_descriptor *cmgmd = NULL;
    unsigned char *buffer = intf->altsetting->extra;
    int buflen = intf->altsetting->extralen;
    struct usb_interface *control_interface;
    struct usb_interface *data_interface;
    struct usb_endpoint_descriptor *epctrl = NULL;
    struct usb_endpoint_descriptor *epread = NULL;
    struct usb_endpoint_descriptor *epwrite = NULL;
    struct usb_device *usb_dev = intf->usb_dev;
    struct usb_cdc_parsed_header h;
    struct acm *acm;
    int ctrlsize, readsize;
    unsigned char *buf;
    int call_intf_num = -1;
    int data_intf_num = -1;
    unsigned long quirks = 0;
    int num_rx_buf;
    int i;
    int combined_interfaces = 0;
    char acm_name[16];
    int  at_num;
    int  pt_num;
    int serial_port_id = FH_USB_SERIAL_PORT_PT;

    intf->user_data = NULL;
#if 1 /*Added by Neoway*/
    struct usb_interface_descriptor *desc = &intf->cur_altsetting->desc;

    if ((intf->usb_dev->descriptor.idVendor == 0x2949 ||
        intf->usb_dev->descriptor.idVendor == 0x8700) &&
         (desc->bInterfaceNumber == 0 || desc->bInterfaceNumber == 1)) {
        return -ENODEV;
    }

    if (ACM_NUM_AT(id->driver_info) != ACM_NUM_NONE)
    {
        at_num = ACM_NUM_AT(id->driver_info);
        rt_sprintf(acm_name, ACM_ID_PREFIX"%d", at_num);
        if (!strcmp(intf->dev.parent.name, acm_name))
        {
            serial_port_id = FH_USB_SERIAL_PORT_AT;
            goto match_ok;
        }
    }

    if (ACM_NUM_PT(id->driver_info) != ACM_NUM_NONE)
    {
        pt_num = ACM_NUM_PT(id->driver_info);
        rt_sprintf(acm_name, ACM_ID_PREFIX"%d", pt_num);
        if (!strcmp(intf->dev.parent.name, acm_name))
        {
            goto match_ok;
        }
    }

    return 0;
#endif

match_ok:
    rt_kprintf("%s----%d quirks %d\n", __func__, __LINE__, quirks);
    /* normal quirks */
    quirks = 0UL;

    if (quirks == IGNORE_DEVICE)
        return -ENODEV;

    memset(&h, 0x00, sizeof(struct usb_cdc_parsed_header));

    num_rx_buf = (quirks == SINGLE_RX_URB) ? 1 : ACM_NR;

    /* handle quirks deadly to normal probing*/
    if (quirks == NO_UNION_NORMAL)
    {
        data_interface = usb_ifnum_to_if(usb_dev, 1);
        control_interface = usb_ifnum_to_if(usb_dev, 0);
        /* we would crash */
        if (!data_interface || !control_interface)
            return -ENODEV;
        goto skip_normal_probe;
    }

    /* normal probing*/
    if (!buffer)
    {
        rt_kprintf("Weird descriptor references\n");
        return -EINVAL;
    }

    if (!intf->cur_altsetting)
        return -EINVAL;

    if (!buflen)
    {
        if (intf->cur_altsetting->endpoint &&
                intf->cur_altsetting->endpoint->extralen &&
                intf->cur_altsetting->endpoint->extra) {
            rt_kprintf("Seeking extra descriptors on endpoint\n");
            buflen = intf->cur_altsetting->endpoint->extralen;
            buffer = intf->cur_altsetting->endpoint->extra;
        } else
        {
            rt_kprintf("Zero length descriptor references\n");
            return -EINVAL;
        }
    }

    cdc_parse_cdc_header(&h, intf, buffer, buflen);
    union_header = h.usb_cdc_union_desc;
    cmgmd = h.usb_cdc_call_mgmt_descriptor;
    if (cmgmd)
        call_intf_num = cmgmd->bDataInterface;

    if (!union_header)
    {
        if (call_intf_num > 0)
        {
            rt_kprintf("No union descriptor, using call management descriptor\n");
            /* quirks for Droids MuIn LCD */
            if (quirks & NO_DATA_INTERFACE)
                data_interface = usb_ifnum_to_if(usb_dev, 0);
            else
            {
                data_intf_num = call_intf_num;
                data_interface = usb_ifnum_to_if(usb_dev, data_intf_num);
            }
            control_interface = intf;
        } else
        {
            if (intf->cur_altsetting->desc.bNumEndpoints != 3)
            {
                rt_kprintf("No union descriptor, giving up\n");
                return -ENODEV;
            } else
            {
                rt_kprintf("No union descriptor, testing for castrated device\n");
                combined_interfaces = 1;
                control_interface = data_interface = intf;
                goto look_for_collapsed_interface;
            }
        }
    } else
    {
        data_intf_num = union_header->bSlaveInterface0;
        control_interface = usb_ifnum_to_if(usb_dev, union_header->bMasterInterface0);
        data_interface = usb_ifnum_to_if(usb_dev, data_intf_num);
    }

    if (!control_interface || !data_interface)
    {
        rt_kprintf("no interfaces\n");
        return -ENODEV;
    }
    if (!data_interface->cur_altsetting || !control_interface->cur_altsetting)
        return -ENODEV;

    if (data_intf_num != call_intf_num)
        rt_kprintf("Separate call control interface. That is not fully supported.\n");

    if (control_interface == data_interface)
    {
        /* some broken devices designed for windows work this way */
        rt_kprintf("Control and data interfaces are not separated!\n");
        combined_interfaces = 1;
        /* a popular other OS doesn't use it */
        quirks |= NO_CAP_LINE;
        if (data_interface->cur_altsetting->desc.bNumEndpoints != 3)
        {
            rt_kprintf("This needs exactly 3 endpoints\n");
            return -EINVAL;
        }
look_for_collapsed_interface:
        for (i = 0; i < 3; i++)
        {
            struct usb_endpoint_descriptor *ep;

            ep = &data_interface->cur_altsetting->endpoint[i].desc;

            if (usb_endpoint_is_int_in(ep))
                epctrl = ep;
            else if (usb_endpoint_is_bulk_out(ep))
                epwrite = ep;
            else if (usb_endpoint_is_bulk_in(ep))
                epread = ep;
            else
                return -EINVAL;
        }
        if (!epctrl || !epread || !epwrite)
            return -ENODEV;
        else
            goto made_compressed_probe;
    }

skip_normal_probe:

    /*workaround for switched interfaces */
    if (data_interface->cur_altsetting->desc.bInterfaceClass
                        != CDC_DATA_INTERFACE_TYPE) {
        if (control_interface->cur_altsetting->desc.bInterfaceClass
                        == CDC_DATA_INTERFACE_TYPE) {
            struct usb_interface *t;

            rt_kprintf("Your device has switched interfaces.\n");
            t = control_interface;
            control_interface = data_interface;
            data_interface = t;
        } else
        {
            return -EINVAL;
        }
    }

    /* Accept probe requests only for the control interface */
    if (!combined_interfaces && intf != control_interface)
        return -ENODEV;

    if (!combined_interfaces && usb_interface_claimed(data_interface))
    {
        /* valid in this context */
        rt_kprintf("The data interface isn't available\n");
        return -EBUSY;
    }


    if (data_interface->cur_altsetting->desc.bNumEndpoints < 2 ||
        control_interface->cur_altsetting->desc.bNumEndpoints == 0)
        return -EINVAL;

    epctrl = &control_interface->cur_altsetting->endpoint[0].desc;
    epread = &data_interface->cur_altsetting->endpoint[0].desc;
    epwrite = &data_interface->cur_altsetting->endpoint[1].desc;


    /* workaround for switched endpoints */
    if (!usb_endpoint_dir_in(epread))
    {
        /* descriptors are swapped */
        struct usb_endpoint_descriptor *t;

        rt_kprintf("The data interface has switched endpoints\n");
        t = epread;
        epread = epwrite;
        epwrite = t;
    }
made_compressed_probe:
    rt_kprintf("interfaces are valid\n");

    acm = rt_malloc(sizeof(struct acm));
    if (acm == NULL)
    {
        rt_kprintf("out of memory(acm kzalloc)\n");
        goto alloc_fail;
    }
    rt_memset(acm, 0, sizeof(struct acm));

    intf->dev.user_data = acm;
    acm->id = id;
    rt_memcpy(acm->name, intf->dev.parent.name, 8);

    ctrlsize = le16_to_cpu(epctrl->wMaxPacketSize);
    readsize = le16_to_cpu(epread->wMaxPacketSize) *
                (quirks == SINGLE_RX_URB ? 1 : 2);
    acm->combined_interfaces = combined_interfaces;
    acm->writesize = le16_to_cpu(epwrite->wMaxPacketSize) * 8;
    acm->control = control_interface;
    acm->data = data_interface;
    /* acm->minor = minor; */
    acm->dev = usb_dev;
    if (h.usb_cdc_acm_descriptor)
        acm->ctrl_caps = h.usb_cdc_acm_descriptor->bmCapabilities;
    if (quirks & NO_CAP_LINE)
        acm->ctrl_caps &= ~USB_CDC_CAP_LINE;
    acm->ctrlsize = ctrlsize;
    acm->readsize = readsize;
    acm->rx_buflimit = num_rx_buf;
    acm->rx_endpoint = usb_rcvbulkpipe(usb_dev, epread->bEndpointAddress);
    acm->is_int_ep = usb_endpoint_xfer_int(epread);
    if (acm->is_int_ep)
        acm->bInterval = epread->bInterval;
    acm->quirks = quirks;

    buf = rt_malloc(ctrlsize);
    if (!buf)
    {
        rt_kprintf("out of memory(ctrl buffer alloc)\n");
        goto alloc_fail2;
    }
    acm->ctrl_buffer = buf;

    if (acm_write_buffers_alloc(acm) < 0)
    {
        rt_kprintf("out of memory(write buffer alloc)\n");
        goto alloc_fail4;
    }

    acm->ctrlurb = usb_alloc_urb(0, GFP_KERNEL);
    if (!acm->ctrlurb)
    {
        rt_kprintf("out of memory(ctrlurb kmalloc)\n");
        goto alloc_fail5;
    }
    for (i = 0; i < num_rx_buf; i++)
    {
        struct acm_rb *rb = &(acm->read_buffers[i]);
        struct urb *urb;

        rb->base = rt_malloc(readsize);
        if (!rb->base)
        {
            rt_kprintf("out of memory "
                    "(read bufs usb_alloc_coherent)\n");
            goto alloc_fail6;
        }
        rb->index = i;
        rb->instance = acm;

        urb = usb_alloc_urb(0, GFP_KERNEL);
        if (!urb)
        {
            rt_kprintf("out of memory(read urbs usb_alloc_urb)\n");
            goto alloc_fail6;
        }
        urb->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;
        urb->transfer_dma = rb->dma;
        if (acm->is_int_ep)
        {
            usb_fill_int_urb(urb, acm->dev,
                     acm->rx_endpoint,
                     rb->base,
                     acm->readsize,
                     acm_read_bulk_callback, rb,
                     acm->bInterval);
        } else
        {
            usb_fill_bulk_urb(urb, acm->dev,
                      acm->rx_endpoint,
                      rb->base,
                      acm->readsize,
                      acm_read_bulk_callback, rb);
        }

        acm->read_urbs[i] = urb;
        set_bit(i, &acm->read_urbs_free);
    }
    for (i = 0; i < ACM_NW; i++)
    {
        struct acm_wb *snd = &(acm->wb[i]);

        snd->urb = usb_alloc_urb(0, GFP_KERNEL);
        if (snd->urb == NULL)
        {
            rt_kprintf("out of memory(write urbs usb_alloc_urb)\n");
            goto alloc_fail7;
        }

        if (usb_endpoint_xfer_int(epwrite))
            usb_fill_int_urb(snd->urb, usb_dev,
                usb_sndbulkpipe(usb_dev, epwrite->bEndpointAddress),
                NULL, acm->writesize, acm_write_bulk, snd, epwrite->bInterval);
        else
            usb_fill_bulk_urb(snd->urb, usb_dev,
                usb_sndbulkpipe(usb_dev, epwrite->bEndpointAddress),
                NULL, acm->writesize, acm_write_bulk, snd);
        snd->urb->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;
        if (quirks & SEND_ZERO_PACKET)
            snd->urb->transfer_flags |= URB_ZERO_PACKET;
        snd->instance = acm;
    }

    usb_set_intfdata(intf, acm);

    usb_fill_int_urb(acm->ctrlurb, usb_dev,
             usb_rcvintpipe(usb_dev, epctrl->bEndpointAddress),
             acm->ctrl_buffer, ctrlsize, acm_ctrl_irq, acm,
             /* works around buggy devices */
             epctrl->bInterval ? epctrl->bInterval : 0xff);
    acm->ctrlurb->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;
    acm->ctrlurb->transfer_dma = acm->ctrl_dma;

    /* rt_kprintf("ttyACM%d: USB ACM device\n", minor); */

    acm_set_control(acm, acm->ctrlout);

    acm->line.dwDTERate = cpu_to_le32(9600);
    acm->line.bDataBits = 8;
    acm_set_line(acm, &acm->line);
    usb_set_intfdata(data_interface, acm);

    /* rt_device_register(&(control_interface->dev), control_interface->dev.parent.name, RT_DEVICE_FLAG_RDWR); */

    if (quirks & CLEAR_HALT_CONDITIONS)
    {
        usb_clear_halt(usb_dev, usb_rcvbulkpipe(usb_dev, epread->bEndpointAddress));
        usb_clear_halt(usb_dev, usb_sndbulkpipe(usb_dev, epwrite->bEndpointAddress));
    }

#ifndef RT_USING_USB_SERIAL_POSIX
    g_acm_us_data[serial_port_id] = acm;
    if ((ACM_NUM_AT(id->driver_info) == ACM_NUM_NONE ||
                g_acm_us_data[FH_USB_SERIAL_PORT_AT] != NULL) &&
            g_acm_us_data[FH_USB_SERIAL_PORT_PT] != NULL)
    {
        usb_serial_ops_register(&g_usb_acm_ops);
    }
#else
    char dev_name[20] = { 0 };
    if (rx_full_sem == NULL)
    {
        rx_full_sem = rt_sem_create("usb_serial_rx", 0, RT_IPC_FLAG_FIFO);
        if (rx_full_sem == RT_NULL)
        {
            goto alloc_fail7;
        }
    }
    rt_memset(&acm->parent, 0, sizeof(acm->parent));
#ifdef RT_USING_DEVICE_OPS
    acm->parent.ops   = &cdc_device_ops;
#else
    acm->parent.open = _serial_open;
    acm->parent.write = _serial_write;
    acm->parent.read = _serial_read;
    acm->parent.close = _serial_close;
#endif

    rt_sprintf(dev_name, "cdc_acm%d", serial_port_id);
    rt_device_register(&acm->parent, dev_name, RT_DEVICE_FLAG_RDWR);
#endif
    rt_kprintf("######[%s----%d]device %s registered! idVendor(0x%x),idProduct(0x%x)\n",
            __func__, __LINE__, intf->dev.parent.name,
            intf->usb_dev->descriptor.idVendor,
            intf->usb_dev->descriptor.idProduct);

    return 0;
alloc_fail7:
    for (i = 0; i < ACM_NW; i++)
        usb_free_urb(acm->wb[i].urb);
alloc_fail6:
    for (i = 0; i < num_rx_buf; i++)
        usb_free_urb(acm->read_urbs[i]);
    acm_read_buffers_free(acm);
    usb_free_urb(acm->ctrlurb);
alloc_fail5:
    acm_write_buffers_free(acm);
alloc_fail4:
    rt_free(acm->ctrl_buffer);
alloc_fail2:
    rt_free(acm);
alloc_fail:
    return -ENOMEM;
}

static void acm_unregister(struct acm *acm)
{
    int i;

    usb_free_urb(acm->ctrlurb);
    for (i = 0; i < ACM_NW; i++)
        usb_free_urb(acm->wb[i].urb);
    for (i = 0; i < acm->rx_buflimit; i++)
        usb_free_urb(acm->read_urbs[i]);
    kfree(acm->country_codes);
    kfree(acm);
}

static void stop_data_traffic(struct acm *acm)
{
    int i;

    dbg("%s\n", __func__);

    usb_kill_urb(acm->ctrlurb);
    for (i = 0; i < ACM_NW; i++)
        usb_kill_urb(acm->wb[i].urb);
    for (i = 0; i < acm->rx_buflimit; i++)
        usb_kill_urb(acm->read_urbs[i]);
}

static void usb_acm_disconnect(struct usb_interface *intf)
{
    struct acm *acm = usb_get_intfdata(intf);

    /* sibling interface is already cleaning up */
    if (!acm)
        return;
    rt_kprintf("%s---%d device %s\n", __func__, __LINE__, intf->dev.parent.name);

    usb_serial_ops_register(NULL);
    acm->dev = NULL;
    usb_set_intfdata(acm->control, NULL);
    usb_set_intfdata(acm->data, NULL);

    if (acm->write_urb_sem)
    {
        rt_sem_delete(acm->write_urb_sem);
        acm->write_urb_sem = NULL;
    }

    stop_data_traffic(acm);

    acm_write_buffers_free(acm);
    rt_free(acm->ctrl_buffer);
    acm_read_buffers_free(acm);
    acm->opend = 0;
#ifdef RT_USING_USB_SERIAL_POSIX
    rt_device_unregister(&acm->parent);
#endif
    acm_unregister(acm);
}

static struct usb_driver usb_acm_driver = {
    .name =        "usb-acm",
    .probe =    usb_acm_probe,
    .disconnect =    usb_acm_disconnect,
    .id_table =    acm_ids,
};

int  usb_cdc_acm_init(void)
{
    int retval;

    RT_DEBUG_LOG(rt_debug_usb, ("Initializing USB cdc-acm driver...\n"));
    dbg("######[%s----%d]\n", __func__, __LINE__);
    /* register the driver, return usb_register return code if error */
    retval = usb_register_driver(&usb_acm_driver);
    if (retval == 0)
    {

        RT_DEBUG_LOG(rt_debug_usb, ("USB CDC-ACM driver registered.\n"));
    }

    return retval;
}

