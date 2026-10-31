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
#include "usb_serial.h"
#include "usb_serial_skel.h"
#include "fh_usb_serial_api.h"


/* #define RT_USING_USB_SERIAL */
#ifdef RT_USING_USB_SERIAL

#define FH_DMA_ALIGNED_SIZE 32

#ifndef RT_USING_USB_SERIAL_POSIX
static struct serial_us_data *g_serial_us_data[2];
#endif

static unsigned char serial_id_prefix[32];
#define SERIAL_NUM_NONE (127)
#define SERIAL_NUM_AT(info) ((unsigned long)info & 0xff)
#define SERIAL_NUM_PT(info) (((unsigned long)info >> 8) & 0xff)
#define SERIAL_NUM_INFO(at, pt)  ((pt << 8) | at)

static struct usb_device_id usb_serial_usb_ids[] = {
    {USB_DEVICE(0x1286, 0x4e3d), .driver_info = SERIAL_NUM_INFO(2, 3),},/* LUAT AIR720H */
    {USB_DEVICE(0x1508, 0x1001), .driver_info = SERIAL_NUM_INFO(3, 1),},/* FIBOCOM NL668 */
    {USB_DEVICE(0x05c6, 0x9003), .driver_info = SERIAL_NUM_INFO(2, 3),},/* Quectel UC20 */
    {USB_DEVICE(0x05c6, 0x9215), .driver_info = SERIAL_NUM_INFO(2, 3),},/* Quectel EC20 */
    {USB_DEVICE(0x2c7c, 0x0125), .driver_info = SERIAL_NUM_INFO(5, 4),},/* Quectel EC25 on usbnet mode 3, mode 1 with(1,2)*/
    {USB_DEVICE(0x2c7c, 0x0121), .driver_info = SERIAL_NUM_INFO(2, 3),},/* Quectel EC21 */
    {USB_DEVICE(0x2c7c, 0x6026), .driver_info = SERIAL_NUM_INFO(4, 3),},/* Quectel EC200 */
    {USB_DEVICE(0x2c7c, 0x6002), .driver_info = SERIAL_NUM_INFO(4, 3),},/* Quectel EC200s */
    {USB_DEVICE(0x1c9e, 0x9b3c), .driver_info = SERIAL_NUM_INFO(2, 1),},/* Longsung U9300C */
    {USB_DEVICE(0x305a, 0x1421), .driver_info = SERIAL_NUM_INFO(0, 1),},/* GM800-5G */
    {USB_DEVICE(0x305a, 0x1404), .driver_info = SERIAL_NUM_INFO(2, 4),},/* GM800-5G */
    {USB_DEVICE(0x305a, 0x1422), .driver_info = SERIAL_NUM_INFO(2, 5),},/* GM800-5G */
    {USB_DEVICE(0x305a, 0x1406), .driver_info = SERIAL_NUM_INFO(5, 2),},/* GM800-5G */
    {USB_DEVICE(0x305a, 0x1403), .driver_info = SERIAL_NUM_INFO(1, 2),},/* GM800-5G */
    {USB_DEVICE(0x05c6, 0x90b6), .driver_info = SERIAL_NUM_INFO(SERIAL_NUM_NONE, 2),},/* chuangjing CJ907 */
    {USB_DEVICE_AND_INTERFACE_INFO(0x19d2, 0x0579, 0xff, 0xff, 0xff), SERIAL_NUM_INFO(2, 3),}, /* WH-G405f */
    { }
};

static int option_probe(struct usb_interface *intf, const struct usb_device_id *id)
{
    /* Bandrich modem and AT command interface is 0xff */
    if ((intf->usb_dev->descriptor.idVendor == 0x1A8D ||
         intf->usb_dev->descriptor.idVendor == 0x1266) &&
         intf->cur_altsetting->desc.bInterfaceClass != 0xff)
        return -ENODEV;

    /* ec2x-2xx */
    if (intf->usb_dev->descriptor.idVendor == 0x2C7C)
    {
        /* unsigned short idProduct = intf->usb_dev->descriptor.idProduct; */

        if (intf->cur_altsetting->desc.bInterfaceClass != 0xFF)
            return -ENODEV;
    }
    /* u9300c */
    if (intf->usb_dev->descriptor.idVendor == 0x1C9E &&
        intf->usb_dev->descriptor.idProduct == 0x9B3C &&
        (intf->cur_altsetting->desc.bInterfaceNumber == 4 ||
        intf->cur_altsetting->desc.bInterfaceNumber == 0))
        return -ENODEV;
    /* wh-g405 */
    if ((intf->usb_dev->descriptor.idVendor == 0x19D2 ||
         intf->usb_dev->descriptor.idVendor == 0x0579) &&
         intf->cur_altsetting->desc.bInterfaceClass != 0xff)
        return -ENODEV;
    /* air 720 */
    if ((intf->usb_dev->descriptor.idVendor == 0x1286 ||
         intf->usb_dev->descriptor.idVendor == 0x4e3d) &&
         intf->cur_altsetting->desc.bInterfaceClass != 0xff)
        return -ENODEV;
    /* CJ 907 */
    if ((intf->usb_dev->descriptor.idVendor == 0x05c6 ||
         intf->usb_dev->descriptor.idVendor == 0x90b6) &&
         intf->cur_altsetting->desc.bInterfaceNumber == 0)
        return -ENODEV;

    return 0;
}

void *serial_malloc(int size)
{
    int align;
    unsigned char  *buffer;

    buffer = rt_malloc(size + FH_DMA_ALIGNED_SIZE*2);
    if (!buffer)
    {
        rt_kprintf("%s: error with size=%d\n", __func__, size);
        return NULL;
    }

    align = FH_DMA_ALIGNED_SIZE - ((unsigned long)buffer & (FH_DMA_ALIGNED_SIZE - 1));
    buffer += align;
    buffer[-1] = align;

    return (void *)buffer;
}

void serial_free(void *ptr)
{
    unsigned char *buffer = (unsigned char *)ptr;

    buffer = buffer - buffer[-1];
    rt_free(buffer);
}

void usb_serial_debug_4gdata(
        const char *function, int linedata, int size,
        const unsigned char *data)
{
#if 0
    int i;

    rt_kprintf("\n%s-%d: length = %d, data =",
            function, linedata, size);

    for (i = 0; i < size; ++i)
    {
        if (i%16 == 0)
            rt_kprintf("\n");
        rt_kprintf("%2x ", data[i]);
    }
    rt_kprintf("\n");
#endif
}

static void usb_serial_instat_callback(struct urb *urb)
{
    int err;
    struct usb_ctrlrequest *req_pkt;

    req_pkt = (struct usb_ctrlrequest *)urb->transfer_buffer;
    if (urb->status != 0 || !req_pkt)
    {
        rt_kprintf("%s: error, status=%d,pkt=%p\n", __func__, urb->status, req_pkt);
        return;
    }

    if ((req_pkt->bRequestType == 0xA1) && (req_pkt->bRequest == 0x20))
    {
        dbg("%s: signal x%x\n", __func__, *((unsigned char *)(req_pkt + 1)));
    }
    else
    {
        rt_kprintf("%s: error request type!\n", __func__);
    }

    /* Resubmit urb so we continue receiving IRQ data */
    err = usb_submit_urb(urb, 0);
    if (err)
        rt_kprintf("%s: resubmit intr urb failed. (%d)\n", __func__, err);
}
#ifdef RT_USING_USB_SERIAL_POSIX
static rt_list_t rx_urb_list = RT_LIST_OBJECT_INIT(rx_urb_list);
static rt_sem_t  rx_full_sem;
static int _serial_rx_push(struct urb *urb);
#endif

static void usb_serial_in_callback(struct urb *urb)
{
    int endpoint;
    unsigned char *data = urb->transfer_buffer;
    int status = urb->status;
#ifndef RT_USING_USB_SERIAL_POSIX
    struct serial_us_data *us = urb->context;
#endif
    usb_serial_debug_4gdata(__func__, __LINE__, urb->actual_length, data);

    endpoint = usb_pipeendpoint(urb->pipe);

    if (status)
    {
        rt_kprintf("%s: error,status=%d,endpoint=%02x.\n", __func__, status, endpoint);
    }
    else
    {
#ifndef RT_USING_USB_SERIAL_POSIX
        urb->status = us->serial_port_id;/*just use it*/
        usb_serial_rx_push(urb);
#else
        _serial_rx_push(urb);
#endif
    }

}

static void usb_serial_submit_read_urb(struct urb *urb)
{
    int err;
    int status = urb->status;

    if (status != -ESHUTDOWN)
    {
        err = usb_submit_urb(urb, 0);
        if (err)
        {
            rt_kprintf("%s: resubmit read urb failed.(%d)\n", __func__, err);
        }
    }
    else
    {
        rt_kprintf("usb serial in callback,status=ESHUTDOWN\n");
    }
}

static void usb_serial_out_callback(struct urb *urb)
{
    int i;
    struct usb_serial_data *sdata;
    struct serial_us_data *us;

    us = urb->context;
    sdata = us->serial_data;
    sdata->in_flight--;
    for (i = 0; i < N_OUT_URB; i++)
    {
        if (sdata->out_urbs[i] == urb)
        {
            clear_bit(i, &sdata->out_busy);
            if (us->write_urb_sem)
                rt_sem_release(us->write_urb_sem);
            break;
        }
    }
}

static int serial_send_setup(struct serial_us_data *data)
{
    int ifNum = data->pusb_intf->cur_altsetting->desc.bInterfaceNumber;
    int val = 0;

    if (data->serial_data->dtr_state)
        val |= 0x01;
    if (data->serial_data->rts_state)
        val |= 0x02;

    return usb_control_msg(data->pusb_dev,
            usb_rcvctrlpipe(data->pusb_dev, 0),
            0x22, 0x21, val, ifNum, NULL, 0, USB_CTRL_SET_TIMEOUT);
}

static struct urb *usb_serial_setup_urb(struct usb_device *usb_dev,
        int endpoint,
        int ep_dir,
        void *ctx,
        char *buf,
        int len,
        void (*callback)(struct urb *))
{
    struct urb *urb;

    urb = usb_alloc_urb(0, 0);    /* No ISO */
    if (urb)
    {
        usb_fill_bulk_urb(urb, usb_dev,
                usb_sndbulkpipe(usb_dev, endpoint) | ep_dir,
                buf, len, callback, ctx);
    }

    return urb;
}

static int usb_serial_setup(struct serial_us_data *us)
{
    int j;
    int err;
    unsigned int buffer_size;
    struct usb_serial_data *sdata;

    /* Now setup per port private data */
    sdata = rt_calloc(1, sizeof(struct usb_serial_data));
    if (!sdata)
        return -1;

    for (j = 0; j < N_IN_URB; j++)
    {
        sdata->in_buffer[j] = (unsigned char *)serial_malloc(IN_BUFLEN);
        if (!sdata->in_buffer[j])
            goto free_memory;

        sdata->in_urbs[j] = usb_serial_setup_urb(us->pusb_dev,
                us->ep_in,
                USB_DIR_IN,
                us,
                (char *)sdata->in_buffer[j],
                IN_BUFLEN,
                usb_serial_in_callback);
        if (!sdata->in_urbs[j])
            goto free_memory;
    }

    for (j = 0; j < N_OUT_URB; j++)
    {
        sdata->out_buffer[j] = serial_malloc(OUT_BUFLEN);
        if (!sdata->out_buffer[j])
            goto free_memory;

        sdata->out_urbs[j] = usb_serial_setup_urb(us->pusb_dev,
                us->ep_out,
                USB_DIR_OUT,
                us,
                (char *)sdata->out_buffer[j],
                OUT_BUFLEN,
                usb_serial_out_callback);
        if (!sdata->out_urbs[j])
            goto free_memory;
    }

    if (us->int_desc)
    {
        /* we had found an interrupt endpoint, prepare irq pipe
         * set up the IRQ pipe and handler
         */
        us->int_in_urb = usb_alloc_urb(0, 0);
        if (!us->int_in_urb)
            goto free_memory;

        buffer_size = us->int_desc->wMaxPacketSize;
        us->interrupt_in_endpointAddress = us->int_desc->bEndpointAddress;
        /*usb 问题，wMaxPacketSize = 10，usb在填充时会多填充2字节，所以这里放大点*/
        us->interrupt_in_buffer = serial_malloc(buffer_size);
        if (!us->interrupt_in_buffer)
            goto free_memory;

        usb_fill_int_urb(us->int_in_urb, us->pusb_dev,
                usb_rcvintpipe(us->pusb_dev,
                    us->int_desc->bEndpointAddress),
                us->interrupt_in_buffer, buffer_size,
                usb_serial_instat_callback, us,
                us->int_desc->bInterval);

        err = usb_submit_urb(us->int_in_urb, 0);
        if (err)
        {
            rt_kprintf("%s: submit int_in_urb failed %d", __func__, err);
            goto free_memory;
        }
    }

    sdata->send_setup = serial_send_setup;
    us->serial_data = sdata;
    return 0;

free_memory:
    if (us->int_in_urb)
    {
        usb_free_urb(us->int_in_urb);
        us->int_in_urb = NULL;
    }

    if (us->interrupt_in_buffer)
    {
        serial_free(us->interrupt_in_buffer);
        us->interrupt_in_buffer = NULL;
    }

    for (j = 0; j < N_OUT_URB; j++)
    {
        if (sdata->out_buffer[j])
            serial_free(sdata->out_buffer[j]);
        usb_free_urb(sdata->out_urbs[j]);
    }

    for (j = 0; j < N_IN_URB; j++)
    {
        if (sdata->in_buffer[j])
            serial_free(sdata->in_buffer[j]);
        usb_free_urb(sdata->in_urbs[j]);
    }

    rt_free(sdata);

    return -1;
}

#ifndef RT_USING_USB_SERIAL_POSIX
static int usb_serial_open(FH_USB_SERIAL_PORT port)
{
    int i, err, j;
    struct urb *urb;
    struct serial_us_data *us = g_serial_us_data[port];
    struct usb_serial_data *sdata;

    if (!us)
        return -1;

    sdata = us->serial_data;
    if (us->write_urb_sem == NULL)
    {
        us->write_urb_sem = rt_sem_create("serial_write_urb", N_OUT_URB, RT_IPC_FLAG_FIFO);
        if (!us->write_urb_sem)
        {
            rt_kprintf("create write urb sem failed!\n");
            return -2;
        }
    }

    /* Start reading from the IN endpoint */
    if (sdata->opened == 0)
    {
        for (i = 0; i < N_IN_URB; i++)
        {
            urb = sdata->in_urbs[i];
            if (!urb)
                continue;

            err = usb_submit_urb(urb, 0);
            if (err)
            {
                rt_kprintf("%s: submit urb %d failed(%d) %d",
                        __func__, i, err, urb->transfer_buffer_length);
                for (j = i; j >= 0; j--)
                {
                    usb_kill_urb(sdata->in_urbs[j]);
                }
                return err;
            }
        }
        if (sdata->send_setup)
        {   /* Not essential ,printf error are misleading*/
            err = sdata->send_setup(us);
            /* if (err < 0) */
                /*rt_kprintf("%s-%d: serial_send_setup() err %d\n", __func__, __LINE__, err);*/
        }
        rt_kprintf("serial device open !\n");
    }
    sdata->opened = 1;
    return 0;
}

static int usb_serial_close(FH_USB_SERIAL_PORT port)
{
    int i;
    struct urb *urb;
    struct serial_us_data *us = g_serial_us_data[port];
    struct usb_serial_data *sdata;

    if (!us)
        return -1;

    sdata = us->serial_data;

    /* Start reading from the IN endpoint */
    if (sdata->opened == 1)
    {
        for (i = 0; i < N_IN_URB; i++)
        {

            urb = sdata->in_urbs[i];
            if (!urb)
                continue;
            if (urb->use_count > 0)
                usb_kill_urb(urb);
        }
        rt_kprintf("serial device close !\n");
    }
    if (us->write_urb_sem)
    {
        rt_sem_delete(us->write_urb_sem);
        us->write_urb_sem = NULL;
    }
    sdata->opened = 0;
    return 0;
}

static int usb_serial_write(FH_USB_SERIAL_PORT port, void *buf, int len)
{
    struct urb *this_urb = NULL;
    struct serial_us_data *us = g_serial_us_data[port];
    struct usb_serial_data *sdata;
    int i, err = 0;
    int left, todo;
    unsigned int tick;
    void *temp_buf = buf;

    if (len > OUT_BUFLEN)
    {
        rt_kprintf("%s: too large pkt %d!\n", len);
        return -1;
    }

    if (!us)
        return -1;

    sdata = us->serial_data;
    if (us->write_urb_sem)
    {
        err = rt_sem_take(us->write_urb_sem, 1000);
        if (err != RT_EOK)
        {
            rt_kprintf("take write urb sem failed.\n");
            /* return -1; */
        }
    }

    left = len;
    for (i = 0; left > 0 && i < N_OUT_URB; i++)
    {
        todo = left;
        if (todo > OUT_BUFLEN)
            todo = OUT_BUFLEN;
        this_urb = sdata->out_urbs[i];
        err = test_and_set_bit(i, &sdata->out_busy);
        if (err)
        {
            tick = rt_tick_get();
            if ((tick - sdata->tx_start_time[i]) < 1000)    /* 如果等待时间不超过10s,使用下一个urb */
                continue;
            rt_kprintf("%s-%d: submit urb[%d] timeout, will unlink this urb!\n", __func__, __LINE__, i);
            usb_kill_urb(this_urb);         /* 若超过10s,urb仍未完成，则取消本次urb */
            continue;
        }
        /* send the data */
        rt_memcpy(this_urb->transfer_buffer, temp_buf, todo);
        this_urb->transfer_buffer_length = todo;

        if (sdata->suspended)
        {
            rt_kprintf("[%s-LINE:%d]usb be suspended, should execute usb_anchor_urb\n"
                    , __func__, __LINE__);
        } else
        {
            sdata->in_flight++;
            err = usb_submit_urb(this_urb, 0);
            if (err)
            {
                rt_kprintf("usb_submit_urb %p(write bulk) failed"
                        "(%d)", this_urb, err);
                clear_bit(i, &sdata->out_busy);
                if (us->write_urb_sem)
                    rt_sem_release(us->write_urb_sem);
                sdata->in_flight--;
                break;
            }
            sdata->tx_start_time[i] = rt_tick_get();
        }
        temp_buf += todo;
        left -= todo;
    }
    len -= left;

    dbg("%s: wrote(did %d)", __func__, len);
    return len;
}

static struct usb_serial_ops g_usb_serial_ops = {
    .name  = "usb_serial",
    .open  = usb_serial_open,
    .close  = usb_serial_close,
    .write = usb_serial_write,
    .rx_back = usb_serial_submit_read_urb,
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
    struct serial_us_data *us = (struct serial_us_data *)dev;
    int i, err, j;
    struct urb *urb;
    struct usb_serial_data *sdata;

    if (!us)
        return -1;

    sdata = us->serial_data;
    if (us->write_urb_sem == NULL)
    {
        us->write_urb_sem = rt_sem_create("serial_write_urb", N_OUT_URB, RT_IPC_FLAG_FIFO);
        if (!us->write_urb_sem)
        {
            rt_kprintf("create write urb sem failed!\n");
            return -2;
        }
    }

    /* Start reading from the IN endpoint */
    if (sdata->opened == 0)
    {
        for (i = 0; i < N_IN_URB; i++)
        {
            urb = sdata->in_urbs[i];
            if (!urb)
                continue;

            err = usb_submit_urb(urb, 0);
            if (err)
            {
                rt_kprintf("%s: submit urb %d failed(%d) %d",
                        __func__, i, err, urb->transfer_buffer_length);
                for (j = i; j >= 0; j--)
                {
                    usb_kill_urb(sdata->in_urbs[j]);
                }
                return err;
            }
        }
        if (sdata->send_setup)
        {   /* Not essential ,printf error are misleading*/
            err = sdata->send_setup(us);
            /* if (err < 0) */
                /*rt_kprintf("%s-%d: serial_send_setup() err %d\n", __func__, __LINE__, err);*/
        }
        rt_kprintf("serial device open !\n");
    }
    sdata->opened = 1;
    return 0;
}

static rt_err_t _serial_close(rt_device_t dev)
{
    struct serial_us_data *us = (struct serial_us_data *)dev;
    int i;
    struct urb *urb;
    struct usb_serial_data *sdata;

    if (!us)
        return -1;

    sdata = us->serial_data;

    /* Start reading from the IN endpoint */
    if (sdata->opened == 1)
    {
        for (i = 0; i < N_IN_URB; i++)
        {

            urb = sdata->in_urbs[i];
            if (!urb)
                continue;
            if (urb->use_count > 0)
                usb_kill_urb(urb);
        }
        rt_kprintf("serial device close !\n");
    }
    if (us->write_urb_sem)
    {
        rt_sem_delete(us->write_urb_sem);
        us->write_urb_sem = NULL;
    }
    sdata->opened = 0;
    return 0;
}

static rt_size_t _serial_write(rt_device_t dev, rt_off_t pos, const void *buffer, rt_size_t size)
{
    struct serial_us_data *us = (struct serial_us_data *)dev;
    struct urb *this_urb = NULL;
    struct usb_serial_data *sdata;
    int i, err = 0;
    int left, todo;
    unsigned int tick;
    void *temp_buf = buffer;
    int len;


    if (!us)
        return -1;

    sdata = us->serial_data;
    if (sdata->opened == 0)
    {
        rt_kprintf("serial device not open !\n");
        return -1;
    }
    if (us->write_urb_sem)
    {
        err = rt_sem_take(us->write_urb_sem, 1000);
        if (err != RT_EOK)
        {
            rt_kprintf("take write urb sem failed.\n");
            /* return -1; */
        }
    }
    len = size;
    left = len;
    for (i = 0; left > 0 && i < N_OUT_URB; i++)
    {
        todo = left;
        if (todo > OUT_BUFLEN)
            todo = OUT_BUFLEN;
        this_urb = sdata->out_urbs[i];
        err = test_and_set_bit(i, &sdata->out_busy);
        if (err)
        {
            tick = rt_tick_get();
            if ((tick - sdata->tx_start_time[i]) < 1000)    /* 如果等待时间不超过10s,使用下一个urb */
                continue;
            rt_kprintf("%s-%d: submit urb[%d] timeout, will unlink this urb!\n", __func__, __LINE__, i);
            usb_kill_urb(this_urb);         /* 若超过10s,urb仍未完成，则取消本次urb */
            continue;
        }
        /* send the data */
        rt_memcpy(this_urb->transfer_buffer, temp_buf, todo);
        this_urb->transfer_buffer_length = todo;

        if (sdata->suspended)
        {
            rt_kprintf("[%s-LINE:%d]usb be suspended, should execute usb_anchor_urb\n"
                    , __func__, __LINE__);
        } else
        {
            sdata->in_flight++;
            err = usb_submit_urb(this_urb, 0);
            if (err)
            {
                rt_kprintf("usb_submit_urb %p(write bulk) failed"
                        "(%d)", this_urb, err);
                clear_bit(i, &sdata->out_busy);
                if (us->write_urb_sem)
                    rt_sem_release(us->write_urb_sem);
                sdata->in_flight--;
                break;
            }
            sdata->tx_start_time[i] = rt_tick_get();
        }
        temp_buf += todo;
        left -= todo;
    }
    len -= left;

    dbg("%s: wrote(did %d)", __func__, len);
    return len;
}

static rt_size_t _serial_read(rt_device_t dev, rt_off_t pos, void *buffer, rt_size_t size)
{
    struct serial_us_data *us = (struct serial_us_data *)dev;
    struct urb *urb;
    struct usb_serial_data *sdata;

    if (!us)
        return -1;

    sdata = us->serial_data;
    if (sdata->opened == 0)
    {
        rt_kprintf("serial device not open !\n");
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
    usb_serial_submit_read_urb(urb);
    return size;
}

#ifdef RT_USING_DEVICE_OPS
const static struct rt_device_ops serial_device_ops =
{
    RT_NULL,
    _serial_open,
    _serial_close,
    _serial_read,
    _serial_write,
    RT_NULL,
};
#endif/* RT_USING_DEVICE_OPS */
#endif/* RT_USING_USB_SERIAL_POSIX */

static int usb_serial_probe(struct usb_interface *intf,
        const struct usb_device_id *id)
{
    struct serial_us_data *us;
    struct usb_endpoint_descriptor *ep_desc;
    struct usb_host_endpoint *endpoint;
    unsigned int i;
    unsigned int ret;
    int serial_port_id = FH_USB_SERIAL_PORT_PT;
    int at_num;
    int pt_num;
    char serial_name[16];

    dbg("######[%s----%d]\n", __func__, __LINE__);
    intf->user_data = NULL;
    ret = option_probe(intf, id);
    if (ret)
    {
        dbg("sub driver rejected device");
        return ret;
    }

    if (SERIAL_NUM_AT(id->driver_info) != SERIAL_NUM_NONE)
    {
        at_num = SERIAL_NUM_AT(id->driver_info);
        rt_strncpy(serial_id_prefix, intf->dev.parent.name, rt_strlen(intf->dev.parent.name) - 1);
        rt_sprintf(serial_name, "%s%d", serial_id_prefix, at_num);
        if (!rt_strcmp(intf->dev.parent.name, serial_name))
        {
            serial_port_id = FH_USB_SERIAL_PORT_AT;
            goto match_ok;
        }
    }

    if (SERIAL_NUM_PT(id->driver_info) != SERIAL_NUM_NONE)
    {
        pt_num = SERIAL_NUM_PT(id->driver_info);
        rt_strncpy(serial_id_prefix, intf->dev.parent.name, rt_strlen(intf->dev.parent.name) - 1);
        rt_sprintf(serial_name, "%s%d", serial_id_prefix, pt_num);
        if (!rt_strcmp(intf->dev.parent.name, serial_name))
        {
            goto match_ok;
        }
    }

    /*not matched, just return 0*/
    return 0;

match_ok:
    us = rt_malloc(sizeof(struct serial_us_data));
    if (us == NULL)
    {
        rt_kprintf("serial_us_data alloc failed!\n");
        goto probe_error;
    }

    rt_memset(us, 0, sizeof(struct serial_us_data));
    us->pusb_intf = intf;
    us->pusb_dev = intf->usb_dev;
    us->id = id;
    intf->user_data = us;
    intf->dev.user_data = us;
    rt_memcpy(us->name, intf->dev.parent.name, 8);

    dbg("bInterfaceNumber = %d->\n", intf->cur_altsetting->desc.bInterfaceNumber);
    for (i = 0; i < intf->cur_altsetting->desc.bNumEndpoints; i++)
    {
        endpoint = intf->cur_altsetting->endpoint+i;
        ep_desc = &(endpoint->desc);
        /* is it an BULK endpoint? */
        dbg("bmAttributes = %d->", ep_desc->bmAttributes);
        dbg("bEndpointAddress = %x\n", ep_desc->bEndpointAddress);
        if ((ep_desc->bmAttributes &
                    USB_ENDPOINT_XFERTYPE_MASK) == USB_ENDPOINT_XFER_BULK) {
            if (ep_desc->bEndpointAddress & USB_DIR_IN)
            {
                us->ep_in = ep_desc->bEndpointAddress &
                    USB_ENDPOINT_NUMBER_MASK;
                us->in_desc = ep_desc;
            } else
            {
                us->ep_out = ep_desc->bEndpointAddress &
                    USB_ENDPOINT_NUMBER_MASK;
                us->out_desc = ep_desc;
            }
        }

        /* is it an interrupt endpoint? */
        if ((ep_desc->bmAttributes &
                    USB_ENDPOINT_XFERTYPE_MASK) == USB_ENDPOINT_XFER_INT) {
            us->int_desc = ep_desc;
        }
    }

    ret = usb_serial_setup(us);
    if (ret < 0)
        goto probe_error;

    us->attached = 1;
    us->disconnected = 0;
#ifndef RT_USING_USB_SERIAL_POSIX
    us->serial_port_id = serial_port_id;
    g_serial_us_data[serial_port_id] = us;


    if ((SERIAL_NUM_AT(id->driver_info) == SERIAL_NUM_NONE ||
                g_serial_us_data[FH_USB_SERIAL_PORT_AT] != NULL) &&
                g_serial_us_data[FH_USB_SERIAL_PORT_PT] != NULL)
    {
        usb_serial_ops_register(&g_usb_serial_ops);
    }
#else
    char dev_name[20] = { 0 };
    if (rx_full_sem == NULL)
    {
        rx_full_sem = rt_sem_create("usb_serial_rx", 0, RT_IPC_FLAG_FIFO);
        if (rx_full_sem == RT_NULL)
        {
            goto probe_error;
        }
    }
    rt_memset(&us->parent, 0, sizeof(us->parent));
#ifdef RT_USING_DEVICE_OPS
    us->parent.ops   = &serial_device_ops;
#else
    us->parent.open = _serial_open;
    us->parent.write = _serial_write;
    us->parent.read = _serial_read;
    us->parent.close = _serial_close;
#endif

    rt_sprintf(dev_name, "serial%d", serial_port_id);
    rt_device_register(&us->parent, dev_name, RT_DEVICE_FLAG_RDWR);
#endif

    rt_kprintf("######[%s----%d]device %s registered! idVendor(0x%x),idProduct(0x%x)\n",
            __func__, __LINE__, intf->dev.parent.name,
            intf->usb_dev->descriptor.idVendor,
            intf->usb_dev->descriptor.idProduct);

    return 0;

probe_error:
    rt_kprintf("%s: failed!\n", __func__);
    return -EIO;
}

static void usb_serial_disconnect(struct usb_interface *interface)
{
    struct serial_us_data *us = interface->user_data;
    struct usb_serial_data *sdata;
    int j;

    if (!us)
        return;

    usb_serial_rx_clear();

    usb_serial_ops_register(NULL);

#ifndef RT_USING_USB_SERIAL_POSIX
    g_serial_us_data[FH_USB_SERIAL_PORT_AT] = NULL;
    g_serial_us_data[FH_USB_SERIAL_PORT_PT] = NULL;
#endif

    sdata = us->serial_data;

    if (us->int_desc)
    {
        usb_kill_urb(us->int_in_urb);
        usb_free_urb(us->int_in_urb);
        serial_free(us->interrupt_in_buffer);
    }
    /* Now free them */
    for (j = 0; j < N_IN_URB; j++)
    {
        usb_kill_urb(sdata->in_urbs[j]);
        usb_free_urb(sdata->in_urbs[j]);
        serial_free(sdata->in_buffer[j]);
        sdata->in_urbs[j] = NULL;
    }
    for (j = 0; j < N_OUT_URB; j++)
    {
        usb_kill_urb(sdata->out_urbs[j]);
        usb_free_urb(sdata->out_urbs[j]);
        serial_free(sdata->out_buffer[j]);
        sdata->out_urbs[j] = NULL;
    }
    /* Now free per port private data */
    if (us->write_urb_sem)
    {
        rt_sem_delete(us->write_urb_sem);
        us->write_urb_sem = NULL;
    }
    us->disconnected = 1;
    sdata->opened = 0;
#ifdef RT_USING_USB_SERIAL_POSIX
    rt_device_unregister(&us->parent);
#endif
    rt_free(sdata);
    rt_free(us);
    usb_set_intfdata(interface, NULL);
    rt_kprintf(" USB serial driver disconnected...\n");
}

static struct usb_driver usb_serial_driver = {
    .name       = "usb-serial",
    .probe      = usb_serial_probe,
    .disconnect = usb_serial_disconnect,
    .id_table   = usb_serial_usb_ids,
};

int usb_serial_init(void)
{
    int retval;

    RT_DEBUG_LOG(rt_debug_usb, ("Initializing USB serial driver...\n"));
    dbg("######[%s----%d]\n", __func__, __LINE__);
    /* register the driver, return usb_register return code if error */
    retval = usb_register_driver(&usb_serial_driver);
    if (retval == 0)
    {

        RT_DEBUG_LOG(rt_debug_usb, ("USB driver registered.\n"));
    }

    return retval;
}

#endif
