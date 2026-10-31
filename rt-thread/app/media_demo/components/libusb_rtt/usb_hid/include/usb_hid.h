#ifndef __USB_HID_H__
#define __USB_HID_H__

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <fcntl.h>
#include <sys/prctl.h>
#include <pthread.h>
#include <fh_usb_device.h>

#include "types/type_def.h"
#include "config.h"
#include "libusb_rtt/usb_update/include/usb_update.h"

#define HID_REPORT_ID_KEYBOARD1         1
#define HID_REPORT_ID_KEYBOARD2         2
#define HID_REPORT_ID_KEYBOARD3         3
#define HID_REPORT_ID_KEYBOARD4         7
#define HID_REPORT_ID_MEDIA             4
#define HID_REPORT_ID_GENERAL           5
#define HID_REPORT_ID_MOUSE             6
#define HID_REPORT_ID_TELEPHONY_IN      8
#define HID_REPORT_ID_TELEPHONY_OUT      9
/**
 * Sample Output Report [8]:
 * Byte 1.0 (1b)    Ring
 * Byte 1.1 (1b)    Speaker
 * Byte 1.2 (1b)    Mute
 * Byte 1.3 (1b)    Off-Hook
 * Byte 1.4 (1b)    Hold
 *
 * Sample Input Report [8]:
 * Byte 0.0 (8b)    Report ID
 * Byte 1.0 (1b)    Hook Switch
 * Byte 1.1 (1b)    Phone Mute
 * Byte 1.2 (1b)    Flash
 * Byte 1.3 (1b)    Redial
 * Byte 1.4 (1b)    Button 7
 */

#define HID_TELEPHONY_IN_HOOK_SWITCH   (1 << 0)
#define HID_TELEPHONY_IN_PHONE_MUTE    (1 << 1)
#define HID_TELEPHONY_IN_FLASH         (1 << 2)
#define HID_TELEPHONY_IN_REDIAL        (1 << 3)
#define HID_TELEPHONY_IN_BUTTON_7      (1 << 4)

#define HID_TELEPHONY_OUT_RING          (1 << 0)
#define HID_TELEPHONY_OUT_SPEAKER       (1 << 1)
#define HID_TELEPHONY_OUT_MUTE          (1 << 2)
#define HID_TELEPHONY_OUT_OFF_HOOK      (1 << 3)
#define HID_TELEPHONY_OUT_HOLD          (1 << 4)

/**
 * Report ID = 1
 * Input report bit map:
 * Byte 0.0 (1b)    Keyboard LeftControl
 * Byte 0.1 (1b)    Keyboard LeftShift
 * Byte 0.2 (1b)    Keyboard LeftAlt
 * Byte 0.3 (1b)    Keyboard Left GUI
 * Byte 0.4 (1b)    Keyboard RightControl
 * Byte 0.5 (1b)    Keyboard RightShift
 * Byte 0.6 (1b)    Keyboard RightAlt
 * Byte 0.7 (1b)    Keyboard Right GUI
 * Byte 2.0 (8b)    Keyboard/Keypad Array
 * Byte 3.0 (8b)    Keyboard/Keypad Array
 * Byte 4.0 (8b)    Keyboard/Keypad Array
 * Byte 5.0 (8b)    Keyboard/Keypad Array
 * Byte 6.0 (8b)    Keyboard/Keypad Array
 * Byte 7.0 (8b)    Keyboard/Keypad Array
 * Output report bit map:
 * Byte 1.0 (1b)    Num Lock
 * Byte 1.1 (1b)    Caps Lock
 * Byte 1.2 (1b)    Scroll Lock
 * Byte 1.3 (1b)    Compose
 * Byte 1.4 (1b)    Kana
 **/
#define HID_INPUT_REPORT_LEN    8

/* Usage Page for key codes reference 《Universal Serial Bus HID Usage Table》 Table 12. */
#define HID_KEY_CODE_a          4
#define HID_KEY_CODE_1          30
#define HID_KEY_CODE_UpArrow    82
#define HID_KEY_CODE_DownArrow  81
#define HID_KEY_CODE_Volume_Up  (0xE9)
#define HID_KEY_CODE_Volume_Down  (0xEA)
#define HID_KEY_CODE_Volume_MUTE  (0xE2)

extern unsigned int hid_mic_mute_status;
extern unsigned char telephony_status;
extern int _audio_set_mute(unsigned int flag, unsigned int hid_flag);

int hid_demo(void);


#endif /* __USB_HID_H__ */
