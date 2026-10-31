#ifndef __USB_AUDIO_H__
#define __USB_AUDIO_H__

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <fcntl.h>
#include <sys/prctl.h>
#include <pthread.h>

#include "types/type_def.h"
#include "dsp/fh_common.h"
#include "dsp/fh_audio_mpi.h"
#include <uac_init.h>
#include "config.h"
#ifdef USB_HID_ENABLE
#include "libusb_rtt/usb_hid/include/usb_hid.h"
#endif


#define UAC_STREAM_ON   1
#define UAC_STREAM_OFF   0

int uac_init(void);

#endif /* __USB_AUDIO_H__ */
