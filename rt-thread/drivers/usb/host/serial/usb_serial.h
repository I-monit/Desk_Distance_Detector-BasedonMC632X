#ifndef __USB_SERIAL_H__
#define __USB_SERIAL_H__

#include <usb.h>
#include <rtdef.h>
#include <rtthread.h>
#include <rtdevice.h>
#include <sys/types.h>

#define N_IN_URB 4
#define N_OUT_URB 4
#define IN_BUFLEN 4096
#define OUT_BUFLEN 4096


typedef unsigned char cc_t;
typedef unsigned int speed_t;
typedef unsigned int tcflag_t;

#define NCCS 32

struct serial_us_data
{
    struct rt_device parent;
    struct usb_device    *pusb_dev;
    struct usb_interface    *pusb_intf;
    struct usb_serial_data  *serial_data;
    unsigned char    name[8];
    unsigned char    ep_in;            /* in endpoint */
    unsigned char    ep_out;            /* out ....... */
    unsigned char    attached;
    unsigned char  disconnected;
    struct urb      *int_in_urb;
    struct usb_endpoint_descriptor  *in_desc;
    struct usb_endpoint_descriptor  *out_desc;
    struct usb_endpoint_descriptor  *int_desc;
    unsigned int   interrupt_in_endpointAddress;
    unsigned int   *interrupt_in_buffer;
    rt_sem_t write_urb_sem;
    const struct usb_device_id *id;

    int serial_port_id; /*FH_USB_SERIAL_PORT_PT or FH_USB_SERIAL_PORT_AT*/

};

struct usb_serial_data
{
    struct urb *in_urbs[N_IN_URB];
    unsigned char *in_buffer[N_IN_URB];
    /* Output endpoints and buffer for this port */
    struct urb *out_urbs[N_OUT_URB];
    unsigned char *out_buffer[N_OUT_URB];
    unsigned long out_busy;    /* Bit vector of URBs in use */
    int opened;
    int in_flight;
    /* Settings for the port */
    int rts_state;             /* Handshaking pins (outputs) */
    int dtr_state;
    int cts_state;             /* Handshaking pins (inputs) */
    int dsr_state;
    int dcd_state;
    int ri_state;
    unsigned int suspended;
    int (*send_setup)(struct serial_us_data *serial_us_data);
    unsigned long tx_start_time[N_OUT_URB];
};


extern int  usb_serial_init(void);

#endif /* ___USB_SERIAL_H */

