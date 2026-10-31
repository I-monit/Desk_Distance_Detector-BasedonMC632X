#ifndef __fh_usb_serial_api_h__
#define __fh_usb_serial_api_h__

typedef enum
{
    FH_USB_SERIAL_PORT_PT = 0,
    FH_USB_SERIAL_PORT_AT = 1,
} FH_USB_SERIAL_PORT;

/**
 * @brief chcek whether the device is inserted and complered by probe
 *
 * return 1 the device is inserted and complered by probe
 * return 0 the device is unpliged or not detected
 */
extern int fh_usb_serial_is_pluged_in(void);


/* ===================================================== */
/**
 * @brief open serial or acm device, submit read urb.
 *
 * @param at_flag select open interface:at/pt
 */
extern int fh_usb_serial_open(FH_USB_SERIAL_PORT port);

/**
 * @brief close serial or acm device, kill read urb.
 *
 * @param at_flag select close interface:at/pt
 */
extern int fh_usb_serial_close(FH_USB_SERIAL_PORT port);


extern int fh_usb_serial_register_rx_callback_force(FH_USB_SERIAL_PORT port,
       void (*input)(FH_USB_SERIAL_PORT port, unsigned char *data, int len));


extern int fh_usb_serial_register_rx_callback_try(FH_USB_SERIAL_PORT port,
       void (*input)(FH_USB_SERIAL_PORT port, unsigned char *data, int len));

/**
 * @brief write data of buf to device using the serial port.
 *
 * @param at_flag serial port:at/pt
 * @param buf address of the data to be send
 * @param count the length of the data to be send
 * return the length if the data that has been written
 */
extern int fh_usb_serial_write(FH_USB_SERIAL_PORT port, void *buf, int len);


#endif /*__fh_usb_serial_api_h__*/
