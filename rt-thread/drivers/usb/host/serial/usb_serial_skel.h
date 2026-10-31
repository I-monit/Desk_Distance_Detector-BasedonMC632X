#ifndef __usb_serial_skel_h__
#define __usb_serial_skel_h__
#include "fh_usb_serial_api.h"

struct usb_serial_ops
{
    const char *name;
    int (*open)(FH_USB_SERIAL_PORT port);
    int (*close)(FH_USB_SERIAL_PORT port);
    int (*write)(FH_USB_SERIAL_PORT port, void *buf, int len);
    void (*rx_back)(struct urb *urb);
};

extern int usb_serial_ops_register(struct usb_serial_ops *ops);

/*
 * call by real usb driver in read URB callback
 */
extern int usb_serial_rx_push(struct urb *urb);

extern void usb_serial_rx_clear(void);


#endif /*__usb_serial_skel_h__*/
