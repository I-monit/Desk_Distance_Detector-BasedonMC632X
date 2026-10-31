#ifndef __USB_VCOM_H__
#define __USB_VCOM_H__

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <fcntl.h>
#include <sys/prctl.h>
#include <pthread.h>
#include <cdc_vcom.h>
#include "config.h"

#include "types/type_def.h"
#include "libusb_rtt/usb_update/include/usb_update.h"
#include "sample_common.h"

#define CDC_RX_BUFSIZE 1024

int cdc_init(void);

#endif /* __USB_VCOM_H__ */
