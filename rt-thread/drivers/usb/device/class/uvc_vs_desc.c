#include <rthw.h>
#include <rtthread.h>
#include <rtservice.h>
#include <rtdevice.h>
#include "../core/usbcommon.h"
#include "../core/usbdevice.h"
#include "ch9.h"
#include "video.h"
#include "libuvc.h"
#include <string.h>
#include "uvc_init.h"

DECLARE_UVC_INPUT_HEADER_DESCRIPTOR(1, 1);

static const struct UVC_INPUT_HEADER_DESCRIPTOR(1, 1) uvc_input_header = {
    .bLength        = UVC_DT_INPUT_HEADER_SIZE(1, 1),
    .bDescriptorType    = USB_DT_CS_INTERFACE,
    .bDescriptorSubType    = UVC_VS_INPUT_HEADER,
    .bNumFormats        = 1,    /* dynamic */
    .wTotalLength        = 0, /* dynamic */
    .bEndpointAddress    = 0, /* dynamic */
    .bmInfo            = 0,
    .bTerminalLink        = UVC_OUTPUT_TERMINAL_ID,
    .bStillCaptureMethod    = 0,
    .bTriggerSupport    = 0,
    .bTriggerUsage        = 0,
    .bControlSize        = 1,
    .bmaControls[0][0]    = 0,
    /* .bmaControls[1][0]    = 4, */
    /* .bmaControls[2][0]    = 4, */
};

static /*const*/ struct uvc_format_uncompressed uvc_format_nv12 = {
    .bLength        = UVC_DT_FORMAT_UNCOMPRESSED_SIZE,
    .bDescriptorType    = USB_DT_CS_INTERFACE,
    .bDescriptorSubType    = UVC_VS_FORMAT_UNCOMPRESSED,
    .bFormatIndex        = 1,
    .bNumFrameDescriptors    = 3,
    .guidFormat = {
    'N',  'V',  '1',  '2', 0x00, 0x00, 0x10, 0x00,
    0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71},
    .bBitsPerPixel      = 12,
    .bDefaultFrameIndex    = 1,
    .bAspectRatioX        = 0,
    .bAspectRatioY        = 0,
    .bmInterfaceFlags    = 0,
    .bCopyProtect        = 0,
};

static /*const*/ struct uvc_format_uncompressed uvc_format_yuy2 = {
    .bLength        = UVC_DT_FORMAT_UNCOMPRESSED_SIZE,
    .bDescriptorType    = USB_DT_CS_INTERFACE,
    .bDescriptorSubType    = UVC_VS_FORMAT_UNCOMPRESSED,
    .bFormatIndex        = 1,
    .bNumFrameDescriptors    = 3,
    .guidFormat = { 'Y', 'U',  'Y', '2', 0x00, 0x00, 0x10, 0x00,
                    0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71},
    .bBitsPerPixel      = 16,
    .bDefaultFrameIndex    = 1,
    .bAspectRatioX        = 0,
    .bAspectRatioY        = 0,
    .bmInterfaceFlags    = 0,
    .bCopyProtect        = 0,
};

static /*const*/ struct uvc_format_mjpeg uvc_format_mjpg = {
    .bLength        = UVC_DT_FORMAT_MJPEG_SIZE,
    .bDescriptorType    = USB_DT_CS_INTERFACE,
    .bDescriptorSubType    = UVC_VS_FORMAT_MJPEG,
    .bFormatIndex        = 1,
    .bNumFrameDescriptors    = 3,
    .bmFlags        = 1,
    .bDefaultFrameIndex    = 1,
    .bAspectRatioX        = 0,
    .bAspectRatioY        = 0,
    .bmInterfaceFlags    = 0,
    .bCopyProtect        = 0,
};

static /*const*/ struct uvc_format_frameBased uvc_format_h264 = {
    .bLength        = UVC_DT_FORMAT_FRAMEBASED_SIZE,
    .bDescriptorType    = USB_DT_CS_INTERFACE,
    .bDescriptorSubType = UVC_VS_FORMAT_FRAME_BASED,
    .bFormatIndex        = 1,
    .bNumFrameDescriptors    = 1,
    .guidFormat = {
         'H',  '2',  '6',  '4', 0x00, 0x00, 0x10, 0x00,
            0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71},
    .bBitsPerPixel        = 0,
    .bDefaultFrameIndex    = 1,
    .bAspectRatioX        = 0,
    .bAspectRatioY        = 0,
    .bmInterfaceFlags    = 0,
    .bCopyProtect        = 0,
    .bVariableSize        = 1,
};

static /*const*/ struct uvc_format_frameBased uvc_format_hevc = {
    .bLength        = UVC_DT_FORMAT_FRAMEBASED_SIZE,
    .bDescriptorType    = USB_DT_CS_INTERFACE,
    .bDescriptorSubType = UVC_VS_FORMAT_FRAME_BASED,
    .bFormatIndex        = 1,
    .bNumFrameDescriptors    = 1,
    .guidFormat = {
         'H',  '2',  '6',  '5', 0x00, 0x00, 0x10, 0x00,
            0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71},
    .bBitsPerPixel        = 0,
    .bDefaultFrameIndex    = 1,
    .bAspectRatioX        = 0,
    .bAspectRatioY        = 0,
    .bmInterfaceFlags    = 0,
    .bCopyProtect        = 0,
    .bVariableSize        = 1,
};

static /*const*/ struct uvc_format_frameBased uvc_format_ir = {
    .bLength        = UVC_DT_FORMAT_FRAMEBASED_SIZE,
    .bDescriptorType    = USB_DT_CS_INTERFACE,
    .bDescriptorSubType = UVC_VS_FORMAT_FRAME_BASED,
    .bFormatIndex        = 1,
    .bNumFrameDescriptors    = 1,
    .guidFormat = {
         0x32,  0x00,  0x00,  0x00, 0x02, 0x00, 0x10, 0x00,
            0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71},
    .bBitsPerPixel        = 0x08,
    .bDefaultFrameIndex    = 1,
    .bAspectRatioX        = 0,
    .bAspectRatioY        = 0,
    .bmInterfaceFlags    = 0,
    .bCopyProtect        = 0,
    .bVariableSize        = 0,
};
static const struct uvc_color_matching_descriptor uvc_color_matching = {
    .bLength        = UVC_DT_COLOR_MATCHING_SIZE,
    .bDescriptorType    = USB_DT_CS_INTERFACE,
    .bDescriptorSubType    = UVC_VS_COLORFORMAT,
    .bColorPrimaries    = 1,
    .bTransferCharacteristics    = 1,
    .bMatrixCoefficients    = 4,
};

#define MAX_FRAME_DESCRIPTORS_NUM   10
#define MAX_FRAME_INTERVAL_NUM      5
#define MAX_FORMAT_DESCRIPTORS_NUM  6
#define TOTAL_FRAME_DESCRIPTORS_NUM (MAX_FRAME_DESCRIPTORS_NUM*MAX_FORMAT_DESCRIPTORS_NUM)
struct uvc_frm_mjpg_info
{
    __u8  bLength;
    __u8  bDescriptorType;
    __u8  bDescriptorSubType;
    __u8  bFrameIndex;
    __u8  bmCapabilities;
    __u16 wWidth;
    __u16 wHeight;
    __u32 dwMinBitRate;
    __u32 dwMaxBitRate;
    __u32 dwMaxVideoFrameBufferSize;
    __u32 dwDefaultFrameInterval;
    __u8  bFrameIntervalType;
    __u32 dwFrameInterval[MAX_FRAME_INTERVAL_NUM];
} __packed;

struct uvc_frm_yuv_info
{
    __u8  bLength;
    __u8  bDescriptorType;
    __u8  bDescriptorSubType;
    __u8  bFrameIndex;
    __u8  bmCapabilities;
    __u16 wWidth;
    __u16 wHeight;
    __u32 dwMinBitRate;
    __u32 dwMaxBitRate;
    __u32 dwMaxVideoFrameBufferSize;
    __u32 dwDefaultFrameInterval;
    __u8  bFrameIntervalType;
    __u32 dwFrameInterval[MAX_FRAME_INTERVAL_NUM];
} __packed;

struct uvc_frm_h264_info
{
    __u8  bLength;
    __u8  bDescriptorType;
    __u8  bDescriptorSubType;
    __u8  bFrameIndex;
    __u8  bmCapabilities;
    __u16 wWidth;
    __u16 wHeight;
    __u32 dwMinBitRate;
    __u32 dwMaxBitRate;
    __u32 dwDefaultFrameInterval;
    __u8  bFrameIntervalType;
    __u32 dwBytesPerLine;
    __u32 dwFrameInterval[MAX_FRAME_INTERVAL_NUM];
} __packed;

struct uvc_frm_ir_info
{
    __u8  bLength;
    __u8  bDescriptorType;
    __u8  bDescriptorSubType;
    __u8  bFrameIndex;
    __u8  bmCapabilities;
    __u16 wWidth;
    __u16 wHeight;
    __u32 dwMinBitRate;
    __u32 dwMaxBitRate;
    __u32 dwDefaultFrameInterval;
    __u8  bFrameIntervalType;
    __u32 dwBytesPerLine;
    __u32 dwFrameInterval[MAX_FRAME_INTERVAL_NUM];
} __packed;
DECLARE_UVC_INPUT_HEADER_DESCRIPTOR(1, 5);

struct uvc_fmt_array_data
{
    struct UVC_INPUT_HEADER_DESCRIPTOR(1, 5) uvc_input_header;
    struct uvc_format_uncompressed uvc_format_nv12;
    struct uvc_format_uncompressed uvc_format_yuy2;
    struct uvc_format_mjpeg uvc_format_mjpg;
    struct uvc_format_frameBased uvc_format_h264;
    struct uvc_format_frameBased uvc_format_hevc;
    struct uvc_format_frameBased uvc_format_ir;

    struct uvc_frm_yuv_info yuv_frames[MAX_FRAME_DESCRIPTORS_NUM];
    struct uvc_frm_yuv_info yuy2_frames[MAX_FRAME_DESCRIPTORS_NUM];
    struct uvc_frm_mjpg_info mjpg_frames[MAX_FRAME_DESCRIPTORS_NUM];
    struct uvc_frm_h264_info h264_frames[MAX_FRAME_DESCRIPTORS_NUM];
    struct uvc_frm_h264_info hevc_frames[MAX_FRAME_DESCRIPTORS_NUM];
    struct uvc_frm_h264_info ir_frames[MAX_FRAME_DESCRIPTORS_NUM];
    struct uvc_descriptor_header *uvc_streaming_data[TOTAL_FRAME_DESCRIPTORS_NUM];

    int fmt_num;
};

static struct uvc_fmt_array_data uvc_fmt_array[2];
static struct uvc_fmt_array_data *pCurFmtArray;

static int uvc_stream_idx = 0;
static int uvc_fmt_idx = 1;

int get_frame_array_num(const struct uvc_frame_info *pFrms)
{
    const struct uvc_frame_info *pFrmData = pFrms;
    int num = 0;

    while (1)
    {
        if (pFrmData->width == 0)
        {
            break;
        }
        num++;
        pFrmData++;
    }

    if (num > MAX_FRAME_DESCRIPTORS_NUM)
    {
        num = MAX_FRAME_DESCRIPTORS_NUM;
    }
    return num;
}

int get_frame_intervals_num(const struct uvc_frame_info *pFrms)
{
    const struct uvc_frame_info *pFrmData = pFrms;
    int num = 0;

    while (1)
    {
        if (pFrmData->intervals[num] == 0)
        {
            break;
        }
        num++;
    }

    if (num > MAX_FRAME_INTERVAL_NUM)
    {
        num = MAX_FRAME_INTERVAL_NUM;
    }
    return num;
}

static void gen_yuv_frame_data(struct uvc_frm_yuv_info *pYuv, int idx, const struct uvc_frame_info *pFrm)
{
    int i;
    int num = get_frame_intervals_num(pFrm);

    pYuv->bLength                = UVC_DT_FRAME_UNCOMPRESSED_SIZE(num);
    pYuv->bDescriptorType        = USB_DT_CS_INTERFACE;
    pYuv->bDescriptorSubType    = UVC_VS_FRAME_UNCOMPRESSED;
    pYuv->bFrameIndex            = idx;
    pYuv->bmCapabilities        = 0;
    pYuv->wWidth                = cpu_to_le16(pFrm->width);
    pYuv->wHeight                = cpu_to_le16(pFrm->height);
    pYuv->dwMinBitRate            = (pFrm->width*pFrm->height*2*8*5);
    pYuv->dwMaxBitRate            = (pFrm->width*pFrm->height*2*8/* bit */*30/* fps */);
    pYuv->dwMaxVideoFrameBufferSize    = (pFrm->width * pFrm->height * 2);
    pYuv->dwDefaultFrameInterval    = cpu_to_le32(pFrm->intervals[0]);

    pYuv->bFrameIntervalType    = num;
    for (i = 0; i < num; i++)
    {
        pYuv->dwFrameInterval[i]    = cpu_to_le32(pFrm->intervals[i]);
    }
}


static void gen_mjpg_frame_data(struct uvc_frm_mjpg_info *pMjpg, int idx, const struct uvc_frame_info *pFrm)
{
    int i;
    int num = get_frame_intervals_num(pFrm);

    pMjpg->bLength                        = UVC_DT_FRAME_MJPEG_SIZE(num);
    pMjpg->bDescriptorType                = USB_DT_CS_INTERFACE;
    pMjpg->bDescriptorSubType            = UVC_VS_FRAME_MJPEG;
    pMjpg->bFrameIndex                    = idx;
    pMjpg->bmCapabilities                = 0;
    pMjpg->wWidth                        = cpu_to_le16(pFrm->width);
    pMjpg->wHeight                        = cpu_to_le16(pFrm->height);
    pMjpg->dwMinBitRate                    = (pFrm->width*pFrm->height*2*8*5);
    pMjpg->dwMaxBitRate                    = (pFrm->width*pFrm->height*2*8/* bit */*30);
    pMjpg->dwMaxVideoFrameBufferSize    = (pFrm->width * pFrm->height * 2);
    pMjpg->dwDefaultFrameInterval        = cpu_to_le32(pFrm->intervals[0]);

    pMjpg->bFrameIntervalType    = num;
    for (i = 0; i < num; i++)
    {
        pMjpg->dwFrameInterval[i]    = cpu_to_le32(pFrm->intervals[i]);
    }
}

static void gen_h264_frame_data(struct uvc_frm_h264_info *pH264, int idx, const struct uvc_frame_info *pFrm)
{
    int i;
    int num = get_frame_intervals_num(pFrm);

    pH264->bLength                = UVC_DT_FRAME_FRAMEBASED_SIZE(num);
    pH264->bDescriptorType        = USB_DT_CS_INTERFACE;
    pH264->bDescriptorSubType   = UVC_VS_FRAME_FRAME_BASED;
    pH264->bFrameIndex            = idx;
    pH264->bmCapabilities        = 0;
    pH264->wWidth                = cpu_to_le16(pFrm->width);
    pH264->wHeight                = cpu_to_le16(pFrm->height);
    pH264->dwMinBitRate            = (pFrm->width*pFrm->height*2*8*5);
    pH264->dwMaxBitRate            = (pFrm->width*pFrm->height*2*8/* bit */*30);
    pH264->dwDefaultFrameInterval    = cpu_to_le32(pFrm->intervals[0]);
    pH264->dwBytesPerLine        = 0;

    pH264->bFrameIntervalType    = num;
    for (i = 0; i < num; i++)
    {
        pH264->dwFrameInterval[i]    = cpu_to_le32(pFrm->intervals[i]);
    }
}

static void gen_ir_frame_data(struct uvc_frm_h264_info *pir, int idx, const struct uvc_frame_info *pFrm)
{
    int i;
    int num = get_frame_intervals_num(pFrm);
    pir->bLength                = UVC_DT_FRAME_FRAMEBASED_SIZE(num);
    pir->bDescriptorType        = USB_DT_CS_INTERFACE;
    pir->bDescriptorSubType   = UVC_VS_FRAME_FRAME_BASED;
    pir->bFrameIndex            = idx;
    pir->bmCapabilities        = 0;
    pir->wWidth                = cpu_to_le16(pFrm->width);
    pir->wHeight                = cpu_to_le16(pFrm->height);
    pir->dwMinBitRate            = (pFrm->width*pFrm->height*8*30);
    pir->dwMaxBitRate            = (pFrm->width*pFrm->height*8*30);
    pir->dwDefaultFrameInterval    = cpu_to_le32(pFrm->intervals[0]);
    pir->dwBytesPerLine        = cpu_to_le16(pFrm->width);
    pir->bFrameIntervalType    = num;
    for (i = 0; i < num; i++)
    {
        pir->dwFrameInterval[i]    = cpu_to_le32(pFrm->intervals[i]);
    }
}

void uvc_stream_append_data(void *data)
{
    if (uvc_stream_idx + 1 > TOTAL_FRAME_DESCRIPTORS_NUM)
    {
        rt_kprintf("[UVC ERROR]：%s-%d: uvc_stream_idx %d > uvc_streaming_data size %d\n",
            __func__, __LINE__, uvc_stream_idx + 1, TOTAL_FRAME_DESCRIPTORS_NUM);
        RT_ASSERT(0);
    }
    pCurFmtArray->uvc_streaming_data[uvc_stream_idx++] = (struct uvc_descriptor_header *)data;
}

void deal_frms_array(unsigned int fcc, const struct uvc_frame_info *pFrms, unsigned int defaultindex)
{
    int frm_num = get_frame_array_num(pFrms);
    const struct uvc_frame_info *pFrmData = pFrms;
    int i;

    if (frm_num <= 0)
        return;

    if (defaultindex > frm_num)
    {
        rt_kprintf("%s--%d: defaultindex %d > frm_num %d\n",
            __func__, __LINE__, defaultindex, frm_num);
        defaultindex = frm_num;
    }

    switch (fcc)
    {
    case V4L2_PIX_FMT_NV12:
        uvc_stream_append_data(&pCurFmtArray->uvc_format_nv12);
        pCurFmtArray->uvc_format_nv12.bFormatIndex = uvc_fmt_idx++;
        pCurFmtArray->uvc_format_nv12.bNumFrameDescriptors = frm_num + 1;
        pCurFmtArray->uvc_format_nv12.bDefaultFrameIndex = defaultindex;
        for (i = 0; i < frm_num; i++)
        {
            gen_yuv_frame_data(&pCurFmtArray->yuv_frames[i], i+1, pFrmData);
            uvc_stream_append_data(&pCurFmtArray->yuv_frames[i]);
            pFrmData++;
        }
        pFrmData = pFrms;
        pFrmData = pFrmData + defaultindex - 1;
        gen_yuv_frame_data(&pCurFmtArray->yuv_frames[i], i+1, pFrmData);
        uvc_stream_append_data(&pCurFmtArray->yuv_frames[i]);
        break;

    case V4L2_PIX_FMT_YUY2:
        uvc_stream_append_data(&pCurFmtArray->uvc_format_yuy2);
        pCurFmtArray->uvc_format_yuy2.bFormatIndex = uvc_fmt_idx++;
        pCurFmtArray->uvc_format_yuy2.bNumFrameDescriptors = frm_num + 1;
        pCurFmtArray->uvc_format_yuy2.bDefaultFrameIndex = defaultindex;
        for (i = 0; i < frm_num; i++)
        {
            gen_yuv_frame_data(&pCurFmtArray->yuy2_frames[i], i+1, pFrmData);
            uvc_stream_append_data(&pCurFmtArray->yuy2_frames[i]);
            pFrmData++;
        }
        pFrmData = pFrms;
        pFrmData = pFrmData + defaultindex - 1;
        gen_yuv_frame_data(&pCurFmtArray->yuy2_frames[i], i+1, pFrmData);
        uvc_stream_append_data(&pCurFmtArray->yuy2_frames[i]);
        break;

    case V4L2_PIX_FMT_MJPEG:
        uvc_stream_append_data(&pCurFmtArray->uvc_format_mjpg);
        pCurFmtArray->uvc_format_mjpg.bFormatIndex = uvc_fmt_idx++;
        pCurFmtArray->uvc_format_mjpg.bNumFrameDescriptors = frm_num + 1;
        pCurFmtArray->uvc_format_mjpg.bDefaultFrameIndex = defaultindex;
        for (i = 0; i < frm_num; i++)
        {
            gen_mjpg_frame_data(&pCurFmtArray->mjpg_frames[i], i+1, pFrmData);
            uvc_stream_append_data(&pCurFmtArray->mjpg_frames[i]);
            pFrmData++;
        }
        pFrmData = pFrms;
        pFrmData = pFrmData + defaultindex - 1;
        gen_mjpg_frame_data(&pCurFmtArray->mjpg_frames[i], i+1, pFrmData);
        uvc_stream_append_data(&pCurFmtArray->mjpg_frames[i]);

        break;

    case V4L2_PIX_FMT_H264:
        uvc_stream_append_data(&pCurFmtArray->uvc_format_h264);
        pCurFmtArray->uvc_format_h264.bFormatIndex = uvc_fmt_idx++;
        pCurFmtArray->uvc_format_h264.bNumFrameDescriptors = frm_num + 1;
        pCurFmtArray->uvc_format_h264.bDefaultFrameIndex = defaultindex;
        for (i = 0; i < frm_num; i++)
        {
            gen_h264_frame_data(&pCurFmtArray->h264_frames[i], i+1, pFrmData);
            uvc_stream_append_data(&pCurFmtArray->h264_frames[i]);
            pFrmData++;
        }
        pFrmData = pFrms;
        pFrmData = pFrmData + defaultindex - 1;
        gen_h264_frame_data(&pCurFmtArray->h264_frames[i], i+1, pFrmData);
        uvc_stream_append_data(&pCurFmtArray->h264_frames[i]);
        break;

    case V4L2_PIX_FMT_H265:
        uvc_stream_append_data(&pCurFmtArray->uvc_format_hevc);
        pCurFmtArray->uvc_format_hevc.bFormatIndex = uvc_fmt_idx++;
        pCurFmtArray->uvc_format_hevc.bNumFrameDescriptors = frm_num + 1;
        pCurFmtArray->uvc_format_hevc.bDefaultFrameIndex = defaultindex;
        for (i = 0; i < frm_num; i++)
        {
            gen_h264_frame_data(&pCurFmtArray->hevc_frames[i], i+1, pFrmData);
            uvc_stream_append_data(&pCurFmtArray->hevc_frames[i]);
            pFrmData++;
        }
        pFrmData = pFrms;
        pFrmData = pFrmData + defaultindex - 1;
        gen_h264_frame_data(&pCurFmtArray->hevc_frames[i], i+1, pFrmData);
        uvc_stream_append_data(&pCurFmtArray->hevc_frames[i]);
        break;
    case V4L2_PIX_FMT_IR:
        uvc_stream_append_data(&pCurFmtArray->uvc_format_ir);
        pCurFmtArray->uvc_format_ir.bFormatIndex = uvc_fmt_idx++;
        pCurFmtArray->uvc_format_ir.bNumFrameDescriptors = frm_num;
        pCurFmtArray->uvc_format_ir.bDefaultFrameIndex = 1;
        for (i = 0; i < frm_num; i++)
        {
            gen_ir_frame_data(&pCurFmtArray->ir_frames[i], i+1, pFrmData);
            uvc_stream_append_data(&pCurFmtArray->ir_frames[i]);
            pFrmData++;
        }
        pFrmData = pFrms;
        pFrmData = pFrmData + defaultindex - 1;
        uvc_stream_append_data(&pCurFmtArray->ir_frames[i]);
        break;
    }
}

void gen_uvc_fmt_array(int id, struct uvc_format_info *fmt, int fmt_num)
{
    struct uvc_format_info *pFmtData = fmt;
    unsigned int i;

    uvc_stream_idx = 0;

    pCurFmtArray = &uvc_fmt_array[id&0x01];
    pCurFmtArray->fmt_num = fmt_num;
    rt_memcpy(&pCurFmtArray->uvc_input_header, &uvc_input_header, sizeof(uvc_input_header));
    rt_memcpy(&pCurFmtArray->uvc_format_nv12, &uvc_format_nv12, sizeof(uvc_format_nv12));
    rt_memcpy(&pCurFmtArray->uvc_format_yuy2, &uvc_format_yuy2, sizeof(uvc_format_yuy2));
    rt_memcpy(&pCurFmtArray->uvc_format_mjpg, &uvc_format_mjpg, sizeof(uvc_format_mjpg));
    rt_memcpy(&pCurFmtArray->uvc_format_h264, &uvc_format_h264, sizeof(uvc_format_h264));
    rt_memcpy(&pCurFmtArray->uvc_format_hevc, &uvc_format_hevc, sizeof(uvc_format_hevc));
    rt_memcpy(&pCurFmtArray->uvc_format_ir, &uvc_format_ir, sizeof(uvc_format_ir));

    uvc_stream_append_data((void *)&pCurFmtArray->uvc_input_header);
    pCurFmtArray->uvc_input_header.bNumFormats = fmt_num;
    pCurFmtArray->uvc_input_header.bLength = UVC_DT_INPUT_HEADER_SIZE(1, fmt_num);

    for (i = 0; i < fmt_num; i++)
    {
        deal_frms_array(pFmtData->fcc, pFmtData->frames, pFmtData->defaultindex);
        uvc_stream_append_data((void *)&uvc_color_matching);
        pFmtData++;
    }

    // uvc_stream_append_data((void *)&uvc_color_matching);
    uvc_stream_append_data(NULL);
}

void gen_stream_descriptor_array(struct uvc_fmt_array_info *pFai)
{
    uvc_fmt_idx = 1;
    gen_uvc_fmt_array(0, pFai->pFmts, pFai->fmt_num);

#ifdef UVC_DOUBLE_STREAM
    uvc_fmt_idx = 1;
    gen_uvc_fmt_array(1, pFai->pFmts2, pFai->fmt2_num);
#endif

}

struct uvc_format_info *uvc_formats[2];
int uvc_formats_num[2];

static struct uvc_fmt_array_info pFai;

void fh_uvc_fmt_init(int stream_id, struct uvc_format_info *fmt, int fmt_num)
{
    if (stream_id == STREAM_ID1)
    {
        pFai.fmt_num = fmt_num;
        pFai.pFmts = fmt;
    }
#ifdef UVC_DOUBLE_STREAM
    else
    {
        pFai.fmt2_num = fmt_num;
        pFai.pFmts2 = fmt;
    }
#endif
    uvc_formats[stream_id] = fmt;
    uvc_formats_num[stream_id] = fmt_num;

}

void change_usb_support_fmt(struct uvc_device *uvc)
{
    if (pFai.pFmts != NULL)
    {
        gen_stream_descriptor_array(&pFai);
        uvc->comm->desc.hs_streaming = uvc_fmt_array[0].uvc_streaming_data;

#ifdef UVC_DOUBLE_STREAM
        uvc->comm->desc.hs_streaming2 = uvc_fmt_array[1].uvc_streaming_data;
#endif
    }
    else
    {
        rt_kprintf("uvc user format is NULL! Call fh_uvc_fmt_init init format.\n ");
        return;
    }
    uvc_copy_descriptors(uvc, 0);
    //uvc_copy_descriptors(uvc, 1);
}

