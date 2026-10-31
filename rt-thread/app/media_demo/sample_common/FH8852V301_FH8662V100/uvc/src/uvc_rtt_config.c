#include "uvc_rtt_config.h"

static char *model_name = MODEL_TAG_UVC_CONFIG;

extern void *fh_dma_memcpy(void *dst, const void *src, int len);

FH_SINT32 getVPUStreamToUVCList(FH_SINT32 grp_id, FH_SINT32 chn_id, struct uvc_dev_app *pDev)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    FH_VIDEO_FRAME stream;
    FH_UINT32 handle_lock;
    struct yuv_buf *yuv_buf = NULL;

    SDK_CHECK_NULL_PTR(model_name, pDev);

    ret = lock_vpu_stream(grp_id, chn_id, &stream, 1000, &handle_lock, 0);
    SDK_FUNC_ERROR(model_name, ret);

    for (int i = 0; i < UVC_YUV_BUF_NUM; i++)
    {
        yuv_buf = &pDev->yuv_buf[pDev->w_index];
        if (!yuv_buf->is_lock && !yuv_buf->is_full)
        {
            yuv_buf->is_lock = 1;
            memset(yuv_buf->data, 0, yuv_buf->bufsize);

            if (pDev->fcc == V4L2_PIX_FMT_IR)
            {
                yuv_buf->ysize = pDev->width * pDev->height;
                yuv_buf->uvsize = pDev->width * pDev->height / 2;
                fh_dma_memcpy(yuv_buf->data, stream.frm_scan.luma.data.vbase, stream.frm_scan.luma.data.size);
            }
            else if (pDev->fcc == V4L2_PIX_FMT_NV12)
            {
                yuv_buf->ysize = pDev->width * pDev->height;
                yuv_buf->uvsize = pDev->width * pDev->height / 2;
                fh_dma_memcpy(yuv_buf->data, stream.frm_scan.luma.data.vbase, stream.frm_scan.luma.data.size);
                fh_dma_memcpy(yuv_buf->data + stream.frm_scan.luma.data.size, stream.frm_scan.chroma.data.vbase, stream.frm_scan.chroma.data.size);
            }
            else if (pDev->fcc == V4L2_PIX_FMT_YUY2 && stream.data_format == VPU_VOMODE_YUYV)
            {
                yuv_buf->ysize = pDev->width * pDev->height;
                yuv_buf->uvsize = pDev->width * pDev->height;
                fh_dma_memcpy(yuv_buf->data, stream.frm_yuyv.data.vbase, stream.frm_yuyv.data.size);
            }
            else if (pDev->fcc == V4L2_PIX_FMT_YUY2 && stream.data_format == VPU_VOMODE_SCAN)
            {
                yuv_buf->ysize = pDev->width * pDev->height;
                yuv_buf->uvsize = pDev->width * pDev->height;
                fh_dma_memcpy(pDev->trans_buffer.data, stream.frm_scan.luma.data.vbase, yuv_buf->ysize);
                fh_dma_memcpy(pDev->trans_buffer.data + yuv_buf->ysize, stream.frm_scan.chroma.data.vbase, yuv_buf->uvsize / 2);
                nv12_to_yuy2((FH_CHAR *)pDev->trans_buffer.data, (FH_CHAR *)pDev->trans_buffer.data + yuv_buf->ysize, (FH_CHAR *)yuv_buf->data, pDev->width, pDev->height);
            }

            yuv_buf->time_stamp = stream.time_stamp;
            yuv_buf->is_full = 1;
            yuv_buf->is_lock = 0;

            pDev->w_index++;
            if (pDev->w_index == UVC_YUV_BUF_NUM)
            {
                pDev->w_index = 0;
            }

            ret = unlock_vpu_stream(grp_id, chn_id, handle_lock, &stream);
            SDK_FUNC_ERROR(model_name, ret);

            return ret;
        }

        pDev->w_index++;
        if (pDev->w_index == UVC_YUV_BUF_NUM)
        {
            pDev->w_index = 0;
        }
    }

    ret = unlock_vpu_stream(grp_id, chn_id, handle_lock, &stream);
    SDK_FUNC_ERROR(model_name, ret);

    SDK_ERR_PRT(model_name, "all buffer is full!\n");
Exit:
    return FH_SDK_FAILED;
}

FH_SINT32 getVPUStreamToUVC(FH_SINT32 grp_id, FH_SINT32 chn_id, struct uvc_dev_app *pDev)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    struct yuv_buf *yuv_buf = NULL;

    SDK_CHECK_NULL_PTR(model_name, pDev);

    for (int i = 0; i < UVC_YUV_BUF_NUM; i++)
    {
        yuv_buf = &pDev->yuv_buf[pDev->r_index];
        if (!yuv_buf->is_lock && yuv_buf->is_full)
        {
            yuv_buf->is_lock = 1;

            fh_uvc_stream_pts(pDev->stream_id, yuv_buf->time_stamp);
            if (pDev->fcc == V4L2_PIX_FMT_IR)
            {
                fh_uvc_stream_enqueue(pDev->stream_id, yuv_buf->data, yuv_buf->ysize);
            }
            else
            {
                fh_uvc_stream_enqueue(pDev->stream_id, yuv_buf->data, yuv_buf->ysize + yuv_buf->uvsize);
            }

            yuv_buf->is_full = 0;
            yuv_buf->is_lock = 0;

            pDev->r_index++;
            if (pDev->r_index == UVC_YUV_BUF_NUM)
            {
                pDev->r_index = 0;
            }

            return ret;
        }
    }

    usleep(10 * 1000);

    return FH_SDK_SUCCESS;
Exit:
    return FH_SDK_FAILED;
}

FH_SINT32 getViseStreamToUVCList(FH_SINT32 vise_id, FH_SINT32 chn_id, struct uvc_dev_app *pDev)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    FH_VIDEO_FRAME stream;
    struct yuv_buf *yuv_buf = NULL;

    SDK_CHECK_NULL_PTR(model_name, pDev);

    ret = vise_get_stream(vise_id, &stream);
    SDK_FUNC_ERROR(model_name, ret);

    for (int i = 0; i < UVC_YUV_BUF_NUM; i++)
    {
        yuv_buf = &pDev->yuv_buf[pDev->w_index];
        if (!yuv_buf->is_lock && !yuv_buf->is_full)
        {
            yuv_buf->is_lock = 1;
            memset(yuv_buf->data, 0, yuv_buf->bufsize);

            if (pDev->fcc == V4L2_PIX_FMT_IR)
            {
                yuv_buf->ysize = pDev->width * pDev->height;
                yuv_buf->uvsize = pDev->width * pDev->height / 2;
                fh_dma_memcpy(yuv_buf->data, stream.frm_scan.luma.data.vbase, stream.frm_scan.luma.data.size);
            }
            else if (pDev->fcc == V4L2_PIX_FMT_NV12)
            {
                yuv_buf->ysize = pDev->width * pDev->height;
                yuv_buf->uvsize = pDev->width * pDev->height / 2;
                fh_dma_memcpy(yuv_buf->data, stream.frm_scan.luma.data.vbase, stream.frm_scan.luma.data.size);
                fh_dma_memcpy(yuv_buf->data + stream.frm_scan.luma.data.size, stream.frm_scan.chroma.data.vbase, stream.frm_scan.chroma.data.size);
            }
            else if (pDev->fcc == V4L2_PIX_FMT_YUY2 && stream.data_format == VPU_VOMODE_YUYV)
            {
                yuv_buf->ysize = pDev->width * pDev->height;
                yuv_buf->uvsize = pDev->width * pDev->height;
                fh_dma_memcpy(yuv_buf->data, stream.frm_yuyv.data.vbase, stream.frm_yuyv.data.size);
            }
            else if (pDev->fcc == V4L2_PIX_FMT_YUY2 && stream.data_format == VPU_VOMODE_SCAN)
            {
                yuv_buf->ysize = pDev->width * pDev->height;
                yuv_buf->uvsize = pDev->width * pDev->height;
                fh_dma_memcpy(pDev->trans_buffer.data, stream.frm_scan.luma.data.vbase, yuv_buf->ysize);
                fh_dma_memcpy(pDev->trans_buffer.data + yuv_buf->ysize, stream.frm_scan.chroma.data.vbase, yuv_buf->uvsize / 2);
                nv12_to_yuy2((FH_CHAR *)pDev->trans_buffer.data, (FH_CHAR *)pDev->trans_buffer.data + yuv_buf->ysize, (FH_CHAR *)yuv_buf->data, pDev->width, pDev->height);
            }

            yuv_buf->time_stamp = stream.time_stamp;
            yuv_buf->is_full = 1;
            yuv_buf->is_lock = 0;

            pDev->w_index++;
            if (pDev->w_index == UVC_YUV_BUF_NUM)
            {
                pDev->w_index = 0;
            }

            ret = vise_release_stream(vise_id, &stream);
            SDK_FUNC_ERROR(model_name, ret);

            return ret;
        }

        pDev->w_index++;
        if (pDev->w_index == UVC_YUV_BUF_NUM)
        {
            pDev->w_index = 0;
        }
    }

    ret = vise_release_stream(vise_id, &stream);
    SDK_FUNC_ERROR(model_name, ret);

    SDK_ERR_PRT(model_name, "all buffer is full!\n");
Exit:
    return FH_SDK_FAILED;
}

FH_SINT32 getViseStreamToUVC(FH_SINT32 grp_id, FH_SINT32 chn_id, struct uvc_dev_app *pDev)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    struct yuv_buf *yuv_buf = NULL;

    SDK_CHECK_NULL_PTR(model_name, pDev);

    for (int i = 0; i < UVC_YUV_BUF_NUM; i++)
    {
        yuv_buf = &pDev->yuv_buf[pDev->r_index];
        if (!yuv_buf->is_lock && yuv_buf->is_full)
        {
            yuv_buf->is_lock = 1;

            fh_uvc_stream_pts(pDev->stream_id, yuv_buf->time_stamp);
            fh_uvc_stream_enqueue(pDev->stream_id, yuv_buf->data, yuv_buf->ysize + yuv_buf->uvsize);

            yuv_buf->is_full = 0;
            yuv_buf->is_lock = 0;

            pDev->r_index++;
            if (pDev->r_index == UVC_YUV_BUF_NUM)
            {
                pDev->r_index = 0;
            }

            return ret;
        }
    }

    usleep(10 * 1000);

    return FH_SDK_SUCCESS;

Exit:
    return FH_SDK_FAILED;
}

FH_SINT32 getENCStreamToUVC(FH_SINT32 grp_id, FH_SINT32 chn_id, struct uvc_dev_app *pDev, FH_STREAM_TYPE streamType)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    FH_VENC_STREAM stream;
    FH_SINT32 enc_chn = 0;
    ENC_INFO *enc_info;

    SDK_CHECK_NULL_PTR(model_name, pDev);

    enc_info = get_enc_config(grp_id, chn_id);
    enc_chn = enc_info->channel;

    ret = enc_get_chn_stream(enc_chn, &stream);
    SDK_FUNC_ERROR(model_name, ret);

    if (stream.stmtype == FH_STREAM_H264 && streamType == FH_STREAM_H264)
    {
        if (pDev->g_h264_delay > 1 && enc_chn == pDev->stream_id) // delay未实现 TODO
        {
            if (pDev->g_h264_delay == 5)
                FH_VENC_RequestIDR(enc_chn);
            pDev->g_h264_delay--;
            if (pDev->g_h264_delay == 1)
            {
                pDev->g_h264_delay = 0;
                FH_VENC_RequestIDR(enc_chn);
            }
        }

        fh_uvc_stream_pts(pDev->stream_id, stream.h264_stream.time_stamp);
        fh_uvc_stream_enqueue(pDev->stream_id, stream.h264_stream.start, stream.h264_stream.length);
    }
    else if (stream.stmtype == FH_STREAM_H265 && streamType == FH_STREAM_H265)
    {
        if (pDev->g_h265_delay > 1 && enc_chn == pDev->stream_id) // delay未实现 TODO
        {
            pDev->g_h265_delay--;
            if (pDev->g_h265_delay == 1)
            {
                pDev->g_h265_delay = 0;
                FH_VENC_RequestIDR(enc_chn);
            }
        }

        fh_uvc_stream_pts(pDev->stream_id, stream.h265_stream.time_stamp);
        fh_uvc_stream_enqueue(pDev->stream_id, stream.h265_stream.start, stream.h265_stream.length);
    }

    ret = enc_release_stream(&stream);
    SDK_FUNC_ERROR(model_name, ret);

    return ret;
Exit:
    return FH_SDK_FAILED;
}

FH_SINT32 getMJPEGStreamToUVC(FH_SINT32 grp_id, FH_SINT32 chn_id, struct uvc_dev_app *pDev, FH_STREAM_TYPE streamType, FH_SINT32 still_image)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    FH_VENC_STREAM stream;
    FH_SINT32 mjpeg_chn = 0;
    ENC_INFO *mjpeg_info;

    SDK_CHECK_NULL_PTR(model_name, pDev);

    mjpeg_info = get_mjpeg_config(grp_id, chn_id);
    mjpeg_chn = mjpeg_info->channel;

    ret = enc_get_chn_stream(mjpeg_chn, &stream);
    SDK_FUNC_ERROR(model_name, ret);

    if (stream.stmtype == FH_STREAM_MJPEG && streamType == FH_STREAM_MJPEG)
    {
        if (still_image)
        {
            fh_uvc_stream_pts(pDev->stream_id, stream.mjpeg_stream.time_stamp);
            fh_uvc_stream_enqueue(pDev->stream_id, stream.mjpeg_stream.start, stream.mjpeg_stream.length);
        }
        else
        {
            if (stream.mjpeg_stream.start[0] == 0xff)
            {
                if (uvc_get_flash_mode() == IsocDataMode_Flash)
                {
                    uvc_get_flash_data(&stream.mjpeg_stream.start, &stream.mjpeg_stream.length);
                    uvc_set_flash_mode(IsocDataMode_stream);
                }
            }

            fh_uvc_stream_pts(pDev->stream_id, stream.mjpeg_stream.time_stamp);
            fh_uvc_stream_enqueue(pDev->stream_id, stream.mjpeg_stream.start, stream.mjpeg_stream.length);
        }
    }
    else
    {
        SDK_ERR_PRT(model_name, "streamType = %d,stream.stype=%d\n", streamType, stream.stmtype);
    }

    ret = enc_release_stream(&stream);
    SDK_FUNC_ERROR(model_name, ret);

    return ret;
Exit:
    return FH_SDK_FAILED;
}
