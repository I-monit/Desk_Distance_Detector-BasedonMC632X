#ifndef __UVC_CONFIG_H__
#define __UVC_CONFIG_H__
#include "sample_common_media.h"
#include "libusb_rtt/usb_video/include/usb_video.h"

#define UVC_THREAD_STOPPING 0
#define UVC_THREAD_RUNNING 1
#define UVC_THREAD_EXIT 2

#if defined UVC_DOUBLE_STREAM

#if defined UVC_DUAL_STREAM_GRP0
#define UVC_DUAL_STREAM_GRP 0
#elif defined UVC_DUAL_STREAM_GRP1
#define UVC_DUAL_STREAM_GRP 1
#elif defined UVC_DUAL_STREAM_GRP2
#define UVC_DUAL_STREAM_GRP 2
#endif

#if defined UVC_DUAL_STREAM_GRP1_CHN0 || defined UVC_DUAL_STREAM_GRP2_CHN0
#define UVC_DUAL_STREAM_CHN 0
#elif defined UVC_DUAL_STREAM_GRP0_CHN1 || defined UVC_DUAL_STREAM_GRP1_CHN1 || defined UVC_DUAL_STREAM_GRP2_CHN1
#define UVC_DUAL_STREAM_CHN 1
#elif defined UVC_DUAL_STREAM_GRP0_CHN2 || defined UVC_DUAL_STREAM_GRP1_CHN2 || defined UVC_DUAL_STREAM_GRP2_CHN2
#define UVC_DUAL_STREAM_CHN 2
#elif defined UVC_DUAL_STREAM_GRP0_CHN3 || defined UVC_DUAL_STREAM_GRP1_CHN3 || defined UVC_DUAL_STREAM_GRP2_CHN3
#define UVC_DUAL_STREAM_CHN 3
#endif
#else
#define UVC_DUAL_STREAM_GRP 0
#define UVC_DUAL_STREAM_CHN 0
#endif

#endif // __UVC_CONFIG_H__
