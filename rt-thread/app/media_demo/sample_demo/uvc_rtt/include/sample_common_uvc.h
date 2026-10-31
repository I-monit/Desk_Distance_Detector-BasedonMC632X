#ifndef __SAMPLE_COMMON_UVC_H__
#define __SAMPLE_COMMON_UVC_H__
#include "libusb_rtt/usb_video/include/usb_video.h"
#include "libusb_rtt/usb_audio/include/usb_audio.h"
#include "libusb_rtt/usb_hid/include/usb_hid.h"
#include "libusb_rtt/usb_vcom/include/usb_vcom.h"
#include "uvc_rtt_config.h"
#include "uvc_config.h"
#include "vpu_config.h"
#include "vpu_bind_info.h"
#include "bgm_config.h"
#include "sample_common_sensor.h"
#include "sample_common_vise.h"

FH_SINT32 sample_common_uvc_start(FH_VOID);

FH_SINT32 sample_common_uvc_stop(FH_VOID);

#endif // __SAMPLE_COMMON_UVC_H__
