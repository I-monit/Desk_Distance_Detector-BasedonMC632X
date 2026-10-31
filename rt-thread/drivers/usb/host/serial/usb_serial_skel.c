#include <usb.h>
#include <rtdef.h>
#include <rtthread.h>
#include <rtdevice.h>
#include <rthw.h>
#include "usb_errno.h"
#include <dma_mem.h>
#include <interrupt.h>
#include <sys/types.h>
#include "usb_serial_skel.h"
#include "fh_usb_serial_api.h"

static struct usb_serial_ops *p_usb_serial_ops;

static rt_list_t rx_urb_list = RT_LIST_OBJECT_INIT(rx_urb_list);
static rt_sem_t  rx_full_sem;

static void (*port_pt_input)(FH_USB_SERIAL_PORT port, unsigned char *data, int len);
static void (*port_at_input)(FH_USB_SERIAL_PORT port, unsigned char *data, int len);


static struct urb *usb_serial_rx_pop(void)
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


static void usb_serial_rx_loop(void *arg)
{
    struct urb *urb;
    struct usb_serial_ops *ops;
    void (*cb)(FH_USB_SERIAL_PORT port, unsigned char *data, int len);

    while (1)
    {
        rt_sem_take(rx_full_sem, RT_WAITING_FOREVER);
        urb = usb_serial_rx_pop();
        if (!urb) /*It will not happen*/
        {
            rt_thread_delay(1);
            continue;
        }

        ops = p_usb_serial_ops;
        if (!ops) /*It will not happen*/
            continue;

        if (urb->transfer_buffer && urb->actual_length)
        {
            cb = (long)urb->status == FH_USB_SERIAL_PORT_PT ? port_pt_input : port_at_input;
            if (cb)
                cb(urb->status, (unsigned char *)(urb->transfer_buffer), urb->actual_length);
        }

        urb->status = 0;
        ops->rx_back(urb);
    }
}

static int usb_serial_start_rx(void)
{
    static rt_thread_t serial_rx_th;

    if (rx_full_sem == NULL)
    {
        rx_full_sem = rt_sem_create("usb_serial_rx", 0, RT_IPC_FLAG_FIFO);
        if (rx_full_sem == RT_NULL)
        {
            return -1;
        }
    }

    if (!serial_rx_th)
    {
        serial_rx_th = rt_thread_create("usb_serial_rx", usb_serial_rx_loop, NULL, 10 * 1024, 130, 20);
        if (serial_rx_th == RT_NULL)
            return -1;
        rt_thread_startup(serial_rx_th);
    }

    return 0;
}

int usb_serial_ops_register(struct usb_serial_ops *ops)
{
    p_usb_serial_ops = ops;
    return 0;
}

int usb_serial_rx_push(struct urb *urb)
{
    register rt_base_t temp;

    temp = rt_hw_interrupt_disable();

    rt_list_init(&urb->urb_list);
    rt_list_insert_before(&rx_urb_list, &urb->urb_list);
    rt_sem_release(rx_full_sem);

    rt_hw_interrupt_enable(temp);

    return 0;
}

void usb_serial_rx_clear(void)
{
    struct urb *urb;

    while ((urb = usb_serial_rx_pop()) != RT_NULL)
    {
        urb->status = 0;
    }
}


/*************************************************************************
 * The following code is the implement for fh_usb_serial_api.h
 *************************************************************************/
int fh_usb_serial_is_pluged_in(void)
{
    return p_usb_serial_ops ? 1 : 0;
}

int fh_usb_serial_open(FH_USB_SERIAL_PORT port)
{
    int ret = -1;
    struct usb_serial_ops *ops = p_usb_serial_ops;

    if (ops)
    {
        ret = usb_serial_start_rx();
        if (ret == 0)
            ret = ops->open(port);
    }

    return ret;
}

int fh_usb_serial_close(FH_USB_SERIAL_PORT port)
{
    struct usb_serial_ops *ops = p_usb_serial_ops;

    if (!ops)
        return -1;
    return ops->close(port);
}

int fh_usb_serial_register_rx_callback_force(FH_USB_SERIAL_PORT port,
         void (*input)(FH_USB_SERIAL_PORT port, unsigned char *data, int len))
{
    rt_base_t level;

    level = rt_hw_interrupt_disable();
    if (port == FH_USB_SERIAL_PORT_PT)
        port_pt_input = input;
    else
        port_at_input = input;
    rt_hw_interrupt_enable(level);

    return 0;
}

int fh_usb_serial_register_rx_callback_try(FH_USB_SERIAL_PORT port,
         void (*input)(FH_USB_SERIAL_PORT port, unsigned char *data, int len))
{
    int ret = 0;
    rt_base_t level;

    level = rt_hw_interrupt_disable();
    if (port == FH_USB_SERIAL_PORT_PT)
    {
        if (port_pt_input && port_pt_input != input)
            ret = -1;
        else
            port_pt_input = input;
    }
    else
    {
        if (port_at_input && port_at_input != input)
            ret = -1;
        else
            port_at_input = input;
    }
    rt_hw_interrupt_enable(level);

    return ret;
}


int fh_usb_serial_write(FH_USB_SERIAL_PORT port, void *buf, int len)
{
    struct usb_serial_ops *ops = p_usb_serial_ops;

    if (!ops)
        return -1;
    return ops->write(port, buf, len);
}
