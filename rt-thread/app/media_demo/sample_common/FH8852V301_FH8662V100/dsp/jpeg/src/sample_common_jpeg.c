#include "sample_common_jpeg.h"

static char *model_name = MODEL_TAG_JPEG;

FH_SINT32 sample_common_mjpeg_start(FH_VOID)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    ENC_INFO *mjpeg_info;

    for (FH_SINT32 grp_id = 0; grp_id < MAX_GRP_NUM; grp_id++)
    {
        for (FH_SINT32 chn_id = 0; chn_id < MAX_VPU_CHN_NUM; chn_id++)
        {
            mjpeg_info = get_mjpeg_config(grp_id, chn_id);
            if (mjpeg_info->enable)
            {
                ret = mjpeg_create_chn(grp_id, chn_id);
                SDK_FUNC_ERROR(model_name, ret);

                ret = mjpeg_set_chn_attr(grp_id, chn_id);
                SDK_FUNC_ERROR(model_name, ret);

                ret = mjpeg_start(grp_id, chn_id);
                SDK_FUNC_ERROR(model_name, ret);

                ret = vpu_bind_jpeg(grp_id, chn_id, mjpeg_info->channel);
                SDK_FUNC_ERROR(model_name, ret);
            }
        }
    }
    SDK_FUNC_ERROR(model_name, ret);

    return ret;
}

FH_SINT32 sample_common_mjpeg_stop(FH_VOID)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    ENC_INFO *mjpeg_info;

    for (FH_SINT32 grp_id = 0; grp_id < MAX_GRP_NUM; grp_id++)
    {
        for (FH_SINT32 chn_id = 0; chn_id < MAX_VPU_CHN_NUM; chn_id++)
        {
            mjpeg_info = get_mjpeg_config(grp_id, chn_id);
            if (mjpeg_info->enable)
            {
                ret = mjpeg_stop(grp_id, chn_id);
                SDK_FUNC_ERROR(model_name, ret);

                ret = mjpeg_destroy(grp_id, chn_id);
                SDK_FUNC_ERROR(model_name, ret);
            }
        }
    }

    ret = jpeg_destroy();
    SDK_FUNC_ERROR(model_name, ret);

    return ret;
}