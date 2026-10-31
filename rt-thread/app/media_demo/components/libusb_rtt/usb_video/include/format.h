#ifndef __FORMAT_H__
#define __FORMAT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "uvc_init.h"
#include "config.h"
#include "vpu_chn_config.h"

/*
    Frame Interval = 10000000/ fps
        60 : 166666
        30 : 333333
        15 : 666666
        10: 1000000
        2: 5000000
    These should match with descriptor in webcam.c
 */

#define FPS(x)  (10000000/x)

#define ARRAY_SIZE(a) ((sizeof(a) / sizeof(a[0])))

#ifndef UVC_FASTBOOT

#ifdef FH_APP_VISE_PIP
#define VISE_WEIGHT_W 1
#define VISE_WEIGHT_H 1
#elif defined FH_APP_VISE_LRS
#define VISE_WEIGHT_W 2
#define VISE_WEIGHT_H 1
#elif defined FH_APP_VISE_TBS
#define VISE_WEIGHT_W 1
#define VISE_WEIGHT_H 2
#elif defined FH_APP_VISE_MBS
#define VISE_WEIGHT_W 2
#define VISE_WEIGHT_H 2
#else
#define VISE_WEIGHT_W 1
#define VISE_WEIGHT_H 1
#endif

static const struct uvc_frame_info uvc_frames_nv12[] = {
    { 1920 * VISE_WEIGHT_W, 1080 * VISE_WEIGHT_H, {FPS(30), FPS(25), FPS(15), 0 }, 1},
    { 1280 * VISE_WEIGHT_W, 720 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, 1},
    { 1024 * VISE_WEIGHT_W, 768 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, 1},
    { 1024 * VISE_WEIGHT_W, 576 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, 1},
    { 640  * VISE_WEIGHT_W, 480 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, 2},
    { 640  * VISE_WEIGHT_W, 368 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, 2},
    { 320  * VISE_WEIGHT_W, 240 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, 3},
    { 0, 0, { 0, }, },
};

static const struct uvc_frame_info uvc_frames_yuy2[] = {
#ifdef ENABLE_PSRAM
    { 1920 * VISE_WEIGHT_W, 1080 * VISE_WEIGHT_H, {FPS(5), 0 }, 1},
    { 1280 * VISE_WEIGHT_W, 720 * VISE_WEIGHT_H,  {FPS(20), FPS(15), 0 }, 1},
    { 1024 * VISE_WEIGHT_W, 768 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, 1},
    { 1024 * VISE_WEIGHT_W, 576 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, 1},
    { 640  * VISE_WEIGHT_W, 480 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, 2},
    { 640  * VISE_WEIGHT_W, 368 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, 2},
    { 320  * VISE_WEIGHT_W, 240 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, 3},
#else
    { 1920 * VISE_WEIGHT_W, 1080 * VISE_WEIGHT_H, {FPS(30), FPS(25), FPS(15), 0 }, 1},
    { 1280 * VISE_WEIGHT_W, 720 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, 1},
    { 1024 * VISE_WEIGHT_W, 768 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, 1},
    { 1024 * VISE_WEIGHT_W, 576 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, 1},
    { 640  * VISE_WEIGHT_W, 480 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, 2},
    { 640  * VISE_WEIGHT_W, 368 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, 2},
    { 320  * VISE_WEIGHT_W, 240 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, 3},
#endif
    { 0, 0, { 0, }, },
};

static const struct uvc_frame_info uvc_frames_mjpeg[] = {
    { 1920 * VISE_WEIGHT_W, 1080 * VISE_WEIGHT_H, {FPS(30), FPS(25), FPS(15), 0 }, 1},
#ifdef _800W_SUPPORT
    { 3840 * VISE_WEIGHT_W, 2160 * VISE_WEIGHT_H, {FPS(30), FPS(25), FPS(15), 0 }, 1},
#endif
#ifdef _500W_SUPPORT
    { 2560 * VISE_WEIGHT_W, 1944 * VISE_WEIGHT_H, {FPS(30), FPS(25), FPS(15), 0 }, 1},
#endif
    { 2560 * VISE_WEIGHT_W, 1440 * VISE_WEIGHT_H, {FPS(30), FPS(25), FPS(15), 0 }, 1},
    { 1280 * VISE_WEIGHT_W, 720 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, 2},
    { 1024 * VISE_WEIGHT_W, 768 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, 2},
    { 640  * VISE_WEIGHT_W, 480 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, 4},
    { 640  * VISE_WEIGHT_W, 368 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, 4},
    { 352  * VISE_WEIGHT_W, 288 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, 6},
    { 320  * VISE_WEIGHT_W, 240 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, 7},
    { 176  * VISE_WEIGHT_W, 144 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, 8},
    { 0, 0, { 0, }, },
};

static const struct uvc_frame_info uvc_frames_h264[] = {
    { 1920 * VISE_WEIGHT_W, 1080 * VISE_WEIGHT_H, {FPS(30), FPS(25), FPS(15), 0 }, 4},
#ifdef _800W_SUPPORT
    { 3840 * VISE_WEIGHT_W, 2160 * VISE_WEIGHT_H, {FPS(30), FPS(25), FPS(15), 0 }, 3},
#endif
#ifdef _500W_SUPPORT
    { 2560 * VISE_WEIGHT_W, 1944 * VISE_WEIGHT_H, {FPS(30), FPS(25), FPS(15), 0 }, 3},
#endif
    { 2560 * VISE_WEIGHT_W, 1440 * VISE_WEIGHT_H, {FPS(30), FPS(25), FPS(15), 0 }, 3},
    { 1280 * VISE_WEIGHT_W, 720 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, 5},
    { 1024 * VISE_WEIGHT_W, 768 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, 5},
    { 640  * VISE_WEIGHT_W, 480 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, 6},
    { 640  * VISE_WEIGHT_W, 368 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, 7},
    { 352  * VISE_WEIGHT_W, 288 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, 8},
    { 320  * VISE_WEIGHT_W, 240 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, 8},
    { 176  * VISE_WEIGHT_W, 144 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, 8},
    { 0, 0, { 0, }, },
};

static const struct uvc_frame_info uvc_frames_h265[] = {
    { 1920 * VISE_WEIGHT_W, 1080 * VISE_WEIGHT_H, {FPS(30), FPS(25), FPS(15), 0 }, 4},
#ifdef _800W_SUPPORT
    { 3840 * VISE_WEIGHT_W, 2160 * VISE_WEIGHT_H, {FPS(30), FPS(25), FPS(15), 0 }, 3},
#endif
#ifdef _500W_SUPPORT
    { 2560 * VISE_WEIGHT_W, 1944 * VISE_WEIGHT_H, {FPS(30), FPS(25), FPS(15), 0 }, 3},
#endif
    { 2560 * VISE_WEIGHT_W, 1440 * VISE_WEIGHT_H, {FPS(30), FPS(25), FPS(15), 0 }, 3},
    { 1280 * VISE_WEIGHT_W, 720 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, 5},
    { 1024 * VISE_WEIGHT_W, 768 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, 5},
    { 640  * VISE_WEIGHT_W, 480 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, 6},
    { 640  * VISE_WEIGHT_W, 368 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, 7},
    { 352  * VISE_WEIGHT_W, 288 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, 8},
    { 320  * VISE_WEIGHT_W, 240 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, 8},
    { 176  * VISE_WEIGHT_W, 144 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, 8},
    { 0, 0, { 0, }, },
};

#ifdef UVC_STILL_IMAGE_CAPTURE_METHOD2
static const struct uvc_frame_info uvc_frames_still[] = {
    { 1920 * VISE_WEIGHT_W, 1080 * VISE_WEIGHT_H, {0}, },
    { 2560 * VISE_WEIGHT_W, 1440 * VISE_WEIGHT_H, {0}, },
    { 1280 * VISE_WEIGHT_W, 720 * VISE_WEIGHT_H,  {0}, },
    { 1024 * VISE_WEIGHT_W, 768 * VISE_WEIGHT_H,  {0}, },
    { 1024 * VISE_WEIGHT_W, 576 * VISE_WEIGHT_H,  {0}, },
    { 640  * VISE_WEIGHT_W, 480 * VISE_WEIGHT_H,  {0}, },
    { 0, 0, {0}, },
};
#endif

static const struct uvc_format_info uvc_formats[] = {
    { V4L2_PIX_FMT_MJPEG, uvc_frames_mjpeg, 1},
#ifdef YUY2_SUPPORT
    { V4L2_PIX_FMT_YUY2, uvc_frames_yuy2, 1},
#endif
#ifndef ENABLE_PSRAM
#ifndef UVC_WINDOWS_HELLO_FACE
    { V4L2_PIX_FMT_H264, uvc_frames_h264, 1},
    { V4L2_PIX_FMT_H265, uvc_frames_h265, 1},
#ifdef NV12_SUPPORT
    { V4L2_PIX_FMT_NV12, uvc_frames_nv12, 1},
#endif
#endif
#endif
#ifdef UVC_STILL_IMAGE_CAPTURE_METHOD2
    { V4L2_PIX_FMT_STILL_MJPEG, uvc_frames_still, 1},
#endif
};

static const struct uvc_frame_info ss_uvc_frames_nv12[] = {
    { 1920 * VISE_WEIGHT_W, 1080 * VISE_WEIGHT_H, {FPS(30), FPS(25), FPS(15), 0 }, },
    { 1280 * VISE_WEIGHT_W, 720 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, },
    { 1024 * VISE_WEIGHT_W, 768 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, },
    { 1024 * VISE_WEIGHT_W, 576 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, },
    { 640  * VISE_WEIGHT_W, 480 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, },
    { 640  * VISE_WEIGHT_W, 368 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, },
    { 320  * VISE_WEIGHT_W, 240 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, },
    { 0, 0, { 0, }, },
};

static const struct uvc_frame_info ss_uvc_frames_yuy2[] = {
    { 1920 * VISE_WEIGHT_W, 1080 * VISE_WEIGHT_H, {FPS(30), FPS(25), FPS(15), 0 }, },
    { 1280 * VISE_WEIGHT_W, 720 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, },
    { 1024 * VISE_WEIGHT_W, 768 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, },
    { 1024 * VISE_WEIGHT_W, 576 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, },
    { 640  * VISE_WEIGHT_W, 480 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, },
    { 640  * VISE_WEIGHT_W, 368 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, },
    { 320  * VISE_WEIGHT_W, 240 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, },
    { 0, 0, { 0, }, },
};

static const struct uvc_frame_info ss_uvc_frames_mjpeg[] = {
    { 1920 * VISE_WEIGHT_W, 1080 * VISE_WEIGHT_H, {FPS(30), FPS(25), FPS(15), 0 }, },
#ifdef _800W_SUPPORT
    { 3840 * VISE_WEIGHT_W, 2160 * VISE_WEIGHT_H, {FPS(30), FPS(25), FPS(15), 0 }, },
#endif
#ifdef _500W_SUPPORT
    { 2560 * VISE_WEIGHT_W, 1944 * VISE_WEIGHT_H, {FPS(30), FPS(25), FPS(15), 0 }, },
#endif
    { 2560 * VISE_WEIGHT_W, 1440 * VISE_WEIGHT_H, {FPS(30), FPS(25), FPS(15), 0 }, },
    { 1280 * VISE_WEIGHT_W, 720 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, },
    { 1024 * VISE_WEIGHT_W, 768 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, },
    { 640  * VISE_WEIGHT_W, 480 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, },
    { 640  * VISE_WEIGHT_W, 368 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, },
    { 352  * VISE_WEIGHT_W, 288 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, },
    { 320  * VISE_WEIGHT_W, 240 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, },
    { 176  * VISE_WEIGHT_W, 144 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, },
    { 0, 0, { 0, }, },
};

static const struct uvc_frame_info ss_uvc_frames_h264[] = {
    { 1920 * VISE_WEIGHT_W, 1080 * VISE_WEIGHT_H, {FPS(30), FPS(25), FPS(15), 0 }, },
#ifdef _800W_SUPPORT
    { 3840 * VISE_WEIGHT_W, 2160 * VISE_WEIGHT_H, {FPS(30), FPS(25), FPS(15), 0 }, },
#endif
#ifdef _500W_SUPPORT
    { 2560 * VISE_WEIGHT_W, 1944 * VISE_WEIGHT_H, {FPS(30), FPS(25), FPS(15), 0 }, },
#endif
    { 2560 * VISE_WEIGHT_W, 1440 * VISE_WEIGHT_H, {FPS(30), FPS(25), FPS(15), 0 }, },
    { 1280 * VISE_WEIGHT_W, 720 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, },
    { 1024 * VISE_WEIGHT_W, 768 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, },
    { 640  * VISE_WEIGHT_W, 480 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, },
    { 640  * VISE_WEIGHT_W, 368 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, },
    { 352  * VISE_WEIGHT_W, 288 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, },
    { 320  * VISE_WEIGHT_W, 240 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, },
    { 176  * VISE_WEIGHT_W, 144 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, },
    { 0, 0, { 0, }, },
};

static const struct uvc_frame_info ss_uvc_frames_h265[] = {
    { 1920 * VISE_WEIGHT_W, 1080 * VISE_WEIGHT_H, {FPS(30), FPS(25), FPS(15), 0 }, },
#ifdef _800W_SUPPORT
    { 3840 * VISE_WEIGHT_W, 2160 * VISE_WEIGHT_H, {FPS(30), FPS(25), FPS(15), 0 }, },
#endif
#ifdef _500W_SUPPORT
    { 2560 * VISE_WEIGHT_W, 1944 * VISE_WEIGHT_H, {FPS(30), FPS(25), FPS(15), 0 }, },
#endif
    { 2560 * VISE_WEIGHT_W, 1440 * VISE_WEIGHT_H, {FPS(30), FPS(25), FPS(15), 0 }, },
    { 1280 * VISE_WEIGHT_W, 720 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, },
    { 1024 * VISE_WEIGHT_W, 768 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, },
    { 640  * VISE_WEIGHT_W, 480 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, },
    { 640  * VISE_WEIGHT_W, 368 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, },
    { 352  * VISE_WEIGHT_W, 288 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, },
    { 320  * VISE_WEIGHT_W, 240 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, },
    { 176  * VISE_WEIGHT_W, 144 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, },
    { 0, 0, { 0, }, },
};

#ifdef UVC_STILL_IMAGE_CAPTURE_METHOD2
static const struct uvc_frame_info ss_uvc_frames_still[] = {
    { 1920 * VISE_WEIGHT_W, 1080 * VISE_WEIGHT_H, {0}, },
    { 2560 * VISE_WEIGHT_W, 1440 * VISE_WEIGHT_H, {0}, },
    { 1280 * VISE_WEIGHT_W, 720 * VISE_WEIGHT_H,  {0}, },
    { 1024 * VISE_WEIGHT_W, 768 * VISE_WEIGHT_H,  {0}, },
    { 1024 * VISE_WEIGHT_W, 576 * VISE_WEIGHT_H,  {0}, },
    { 640  * VISE_WEIGHT_W, 480 * VISE_WEIGHT_H,  {0}, },
    { 0, 0, {0}, },
};
#endif

static const struct uvc_format_info ss_uvc_formats[] = {
    { V4L2_PIX_FMT_H264, ss_uvc_frames_h264, 1},
    { V4L2_PIX_FMT_H265, ss_uvc_frames_h265, 1},
    { V4L2_PIX_FMT_MJPEG, ss_uvc_frames_mjpeg, 1},
#ifdef NV12_SUPPORT
    { V4L2_PIX_FMT_NV12, ss_uvc_frames_nv12, 1},
#endif
#ifdef YUY2_SUPPORT
    { V4L2_PIX_FMT_YUY2, ss_uvc_frames_yuy2, 1},
#endif
#ifdef UVC_STILL_IMAGE_CAPTURE_METHOD2
    { V4L2_PIX_FMT_STILL_MJPEG, ss_uvc_frames_still, 1},
#endif
};
#else
static const struct uvc_frame_info frames0[10];
static const struct uvc_frame_info frames1[10];
static const struct uvc_frame_info frames2[10];
static const struct uvc_frame_info frames3[10];
static const struct uvc_frame_info frames4[10];
struct uvc_format_info uvc_formats[5] = {
    { 0, frames0, 10},
    { 0, frames1, 10},
    { 0, frames2, 10},
    { 0, frames3, 10},
    { 0, frames4, 10},
};

static const struct uvc_frame_info ss_frames0[10];
static const struct uvc_frame_info ss_frames1[10];
static const struct uvc_frame_info ss_frames2[10];
static const struct uvc_frame_info ss_frames3[10];
static const struct uvc_frame_info ss_frames4[10];
struct uvc_format_info ss_uvc_formats[5] = {
    { 0, ss_frames0, 10},
    { 0, ss_frames1, 10},
    { 0, ss_frames2, 10},
    { 0, ss_frames3, 10},
    { 0, ss_frames4, 10},
};

#endif

#ifdef UVC_DOUBLE_STREAM
#ifndef UVC_FASTBOOT

static const struct uvc_frame_info uvc_frames2_nv12[] = {
    { 1920 * VISE_WEIGHT_W, 1080 * VISE_WEIGHT_H, {FPS(30), FPS(25), FPS(15), 0 }, },
    { 1280 * VISE_WEIGHT_W, 720 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, },
    { 1024 * VISE_WEIGHT_W, 576 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, },
    { 640  * VISE_WEIGHT_W, 480 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, },
    { 640  * VISE_WEIGHT_W, 368 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, },
    { 320  * VISE_WEIGHT_W, 240 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, },
    { 0, 0, { 0, }, },
};

static const struct uvc_frame_info uvc_frames2_yuy2[] = {
    { 1920 * VISE_WEIGHT_W, 1080 * VISE_WEIGHT_H, {FPS(30), FPS(25), FPS(15), 0 }, },
    { 1280 * VISE_WEIGHT_W, 720 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, },
    { 1024 * VISE_WEIGHT_W, 576 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, },
    { 640  * VISE_WEIGHT_W, 480 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, },
    { 640  * VISE_WEIGHT_W, 368 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, },
    { 320  * VISE_WEIGHT_W, 240 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, },
    { 0, 0, { 0, }, },
};

static const struct uvc_frame_info uvc_frames2_mjpeg[] = {
    { 1920 * VISE_WEIGHT_W, 1080 * VISE_WEIGHT_H, {FPS(30), FPS(25), FPS(15), 0 }, },
    { 1280 * VISE_WEIGHT_W, 720 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, },
    { 1024 * VISE_WEIGHT_W, 576 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, },
    { 640  * VISE_WEIGHT_W, 480 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, },
    { 640  * VISE_WEIGHT_W, 368 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, },
    { 320  * VISE_WEIGHT_W, 240 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, },
    { 0, 0, { 0, }, },
};

static const struct uvc_frame_info uvc_frames2_h264[] = {
    { 1920 * VISE_WEIGHT_W, 1080 * VISE_WEIGHT_H, {FPS(30), FPS(25), FPS(15), 0 }, },
    { 1280 * VISE_WEIGHT_W, 720 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, },
    { 1024 * VISE_WEIGHT_W, 576 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, },
    { 640  * VISE_WEIGHT_W, 480 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, },
    { 640  * VISE_WEIGHT_W, 368 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, },
    { 320  * VISE_WEIGHT_W, 240 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, },
    { 0, 0, { 0, }, },
};

static const struct uvc_frame_info uvc_frames2_h265[] = {
    { 1920 * VISE_WEIGHT_W, 1080 * VISE_WEIGHT_H, {FPS(30), FPS(25), FPS(15), 0 }, },
    { 1280 * VISE_WEIGHT_W, 720 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, },
    { 1024 * VISE_WEIGHT_W, 576 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, },
    { 640  * VISE_WEIGHT_W, 480 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, },
    { 640  * VISE_WEIGHT_W, 368 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, },
    { 320  * VISE_WEIGHT_W, 240 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, },
    { 0, 0, { 0, }, },
};

#ifdef UVC_WINDOWS_HELLO_FACE
static const struct uvc_frame_info uvc_frames2_ir[] = {
    { 624 * VISE_WEIGHT_W, 352 * VISE_WEIGHT_H, {FPS(30), FPS(25), FPS(20), FPS(15), FPS(10), FPS(7), FPS(5), 0 }, },
    { 0, 0, { 0, }, },
};
#endif

#ifdef UVC_STILL_IMAGE_CAPTURE_METHOD2
static const struct uvc_frame_info uvc_frames2_still[] = {
    { 1920 * VISE_WEIGHT_W, 1080 * VISE_WEIGHT_H, {0}, },
    { 1280 * VISE_WEIGHT_W, 720 * VISE_WEIGHT_H,  {0}, },
    { 1024 * VISE_WEIGHT_W, 768 * VISE_WEIGHT_H,  {0}, },
    { 1024 * VISE_WEIGHT_W, 576 * VISE_WEIGHT_H,  {0}, },
    { 640  * VISE_WEIGHT_W, 480 * VISE_WEIGHT_H,  {0}, },
    { 0, 0, {0}, },
};
#endif

static const struct uvc_format_info uvc_formats2[] = {
#ifdef UVC_WINDOWS_HELLO_FACE
    { V4L2_PIX_FMT_IR, uvc_frames2_ir, 1},
#else
    { V4L2_PIX_FMT_H264, uvc_frames2_h264, 4},
    { V4L2_PIX_FMT_H265, uvc_frames2_h265, 4},
    { V4L2_PIX_FMT_MJPEG, uvc_frames2_mjpeg, 4},
#ifdef NV12_SUPPORT
    { V4L2_PIX_FMT_NV12, uvc_frames2_nv12, 4},
#endif
#ifdef YUY2_SUPPORT
    { V4L2_PIX_FMT_YUY2, uvc_frames2_yuy2, 4},
#endif
#ifdef UVC_STILL_IMAGE_CAPTURE_METHOD2
    { V4L2_PIX_FMT_STILL_MJPEG, uvc_frames2_still, 5},
#endif
#endif
};

static const struct uvc_frame_info ss_uvc_frames2_nv12[] = {
    { 1920 * VISE_WEIGHT_W, 1080 * VISE_WEIGHT_H, {FPS(30), FPS(25), FPS(15), 0 }, },
    { 1280 * VISE_WEIGHT_W, 720 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, },
    { 1024 * VISE_WEIGHT_W, 576 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, },
    { 640  * VISE_WEIGHT_W, 480 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, },
    { 640  * VISE_WEIGHT_W, 368 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, },
    { 320  * VISE_WEIGHT_W, 240 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, },
    { 0, 0, { 0, }, },
};

static const struct uvc_frame_info ss_uvc_frames2_yuy2[] = {
    { 1920 * VISE_WEIGHT_W, 1080 * VISE_WEIGHT_H, {FPS(30), FPS(25), FPS(15), 0 }, },
    { 1280 * VISE_WEIGHT_W, 720 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, },
    { 1024 * VISE_WEIGHT_W, 576 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, },
    { 640  * VISE_WEIGHT_W, 480 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, },
    { 640  * VISE_WEIGHT_W, 368 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, },
    { 320  * VISE_WEIGHT_W, 240 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, },
    { 0, 0, { 0, }, },
};

static const struct uvc_frame_info ss_uvc_frames2_mjpeg[] = {
    { 1920 * VISE_WEIGHT_W, 1080 * VISE_WEIGHT_H, {FPS(30), FPS(25), FPS(15), 0 }, },
    { 1280 * VISE_WEIGHT_W, 720 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, },
    { 1024 * VISE_WEIGHT_W, 576 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, },
    { 640  * VISE_WEIGHT_W, 480 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, },
    { 640  * VISE_WEIGHT_W, 368 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, },
    { 320  * VISE_WEIGHT_W, 240 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, },
    { 0, 0, { 0, }, },
};

static const struct uvc_frame_info ss_uvc_frames2_h264[] = {
    { 1920 * VISE_WEIGHT_W, 1080 * VISE_WEIGHT_H, {FPS(30), FPS(25), FPS(15), 0 }, },
    { 1280 * VISE_WEIGHT_W, 720 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, },
    { 1024 * VISE_WEIGHT_W, 576 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, },
    { 640  * VISE_WEIGHT_W, 480 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, },
    { 640  * VISE_WEIGHT_W, 368 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, },
    { 320  * VISE_WEIGHT_W, 240 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, },
    { 0, 0, { 0, }, },
};

static const struct uvc_frame_info ss_uvc_frames2_h265[] = {
    { 1920 * VISE_WEIGHT_W, 1080 * VISE_WEIGHT_H, {FPS(30), FPS(25), FPS(15), 0 }, },
    { 1280 * VISE_WEIGHT_W, 720 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, },
    { 1024 * VISE_WEIGHT_W, 576 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, },
    { 640  * VISE_WEIGHT_W, 480 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, },
    { 640  * VISE_WEIGHT_W, 368 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, },
    { 320  * VISE_WEIGHT_W, 240 * VISE_WEIGHT_H,  {FPS(30), FPS(25), FPS(15), 0 }, },
    { 0, 0, { 0, }, },
};

#ifdef UVC_WINDOWS_HELLO_FACE
static const struct uvc_frame_info ss_uvc_frames2_ir[] = {
    {624 * VISE_WEIGHT_W, 352 * VISE_WEIGHT_H, {FPS(30), 0 }, },
    { 0, 0, { 0, }, },
};
#endif

#ifdef UVC_STILL_IMAGE_CAPTURE_METHOD2
static const struct uvc_frame_info ss_uvc_frames2_still[] = {
    { 1920 * VISE_WEIGHT_W, 1080 * VISE_WEIGHT_H, {0}, },
    { 1280 * VISE_WEIGHT_W, 720 * VISE_WEIGHT_H,  {0}, },
    { 1024 * VISE_WEIGHT_W, 768 * VISE_WEIGHT_H,  {0}, },
    { 1024 * VISE_WEIGHT_W, 576 * VISE_WEIGHT_H,  {0}, },
    { 640  * VISE_WEIGHT_W, 480 * VISE_WEIGHT_H,  {0}, },
    { 0, 0, {0}, },
};
#endif

static const struct uvc_format_info ss_uvc_formats2[] = {
#ifdef UVC_WINDOWS_HELLO_FACE
    { V4L2_PIX_FMT_IR, ss_uvc_frames2_ir, 1},
#else
    { V4L2_PIX_FMT_H264, ss_uvc_frames2_h264, 1},
    { V4L2_PIX_FMT_H265, ss_uvc_frames2_h265, 1},
    { V4L2_PIX_FMT_MJPEG, ss_uvc_frames2_mjpeg, 1},
#ifdef NV12_SUPPORT
    { V4L2_PIX_FMT_NV12, ss_uvc_frames2_nv12, 1},
#endif
#ifdef YUY2_SUPPORT
    { V4L2_PIX_FMT_YUY2, ss_uvc_frames2_yuy2, 1},
#endif
#ifdef UVC_STILL_IMAGE_CAPTURE_METHOD2
    { V4L2_PIX_FMT_STILL_MJPEG, ss_uvc_frames2_still, 1},
#endif
#endif
};
#else //UVC_FASTBOOT
static const struct uvc_frame_info frames2_0[10];
static const struct uvc_frame_info frames2_1[10];
static const struct uvc_frame_info frames2_2[10];
static const struct uvc_frame_info frames2_3[10];
static const struct uvc_frame_info frames2_4[10];
struct uvc_format_info uvc_formats2[5] = {
    { 0, frames2_0, 10},
    { 0, frames2_1, 10},
    { 0, frames2_2, 10},
    { 0, frames2_3, 10},
    {0, frames2_4, 10},
};
static const struct uvc_frame_info ss_frames2_0[10];
static const struct uvc_frame_info ss_frames2_1[10];
static const struct uvc_frame_info ss_frames2_2[10];
static const struct uvc_frame_info ss_frames2_3[10];
static const struct uvc_frame_info ss_frames2_4[10];
struct uvc_format_info ss_uvc_formats2[5] = {
    { 0, ss_frames2_0, 10},
    { 0, ss_frames2_1, 10},
    { 0, ss_frames2_2, 10},
    { 0, ss_frames2_3, 10},
    {0, ss_frames2_4, 10},
};
#endif //UVC_FASTBOOT
#endif
#ifdef __cplusplus
}
#endif

#endif //__FORMAT_H__
