#ifndef __FH_USB_DEVICE_H__
#define __FH_USB_DEVICE_H__

/*
 *功能:usb hid驱动初始化
 *提示：主要功能是初始化usb device控制器和hid驱动初始化，之后才开始枚举。
 */
extern void fh_hid_init(void);

/*
 *功能:usb mstorage驱动初始化
 *提示：主要功能是初始化usb device控制器和mstorage驱动初始化，之后才开始枚举。
 */
extern void fh_mstorage_init(void);

/*
 *功能:usb rndis驱动初始化
 *提示：主要功能是初始化usb device控制器和rndis驱动初始化，之后才开始枚举。
 */
extern void fh_rndis_init(void);
#endif
