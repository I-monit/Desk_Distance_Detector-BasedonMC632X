#include "sample_common_vou.h"

static char *model_name = MODEL_TAG_VOU;

FH_SINT32 sample_common_vou_test(FH_VOID)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;

    ret = vou_send_stream();
    SDK_FUNC_ERROR(model_name, ret);

    return ret;
}

FH_SINT32 sample_common_vou_device_init(FH_VOID)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    DHD_INFO *dhd_info;

    for (FH_SINT32 dhd_id = 0; dhd_id < MAX_DHD_DEV_NUM; dhd_id++)
    {
        dhd_info = get_dhd_config(dhd_id);
        if (dhd_info->enable)
        {
            ret = dhd_device_init(dhd_id);
            SDK_FUNC_ERROR(model_name, ret);
        }
    }

    return ret;
}

FH_SINT32 sample_common_video_layer_init(FH_VOID)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    VIDEO_LAYER_INFO *later_info;

    for (FH_SINT32 layer_id = 0; layer_id < MAX_VIDEO_LAYER_NUM; layer_id++)
    {
        later_info = get_video_layer_config(layer_id);
        if (later_info->enable)
        {
            ret = vou_video_layer_init(layer_id);
            SDK_FUNC_ERROR(model_name, ret);
        }
    }

    return ret;
}

FH_SINT32 sample_common_video_layer_chn_init(FH_VOID)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    VIDEO_LAYER_CHN_INFO *later_chn_info;

    for (FH_SINT32 chn_id = 0; chn_id < MAX_VIDEO_LAYER_CHN_NUM; chn_id++)
    {
        later_chn_info = get_video_layer_chn_config(chn_id);
        if (later_chn_info->enable)
        {
            ret = vou_video_layer_chn_init(chn_id);
            SDK_FUNC_ERROR(model_name, ret);
        }
    }

    return ret;
}

FH_SINT32 sample_common_osd_layer_init(FH_VOID)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    OSD_LAYER_INFO *later_info;

    for (FH_SINT32 layer_id = 0; layer_id < MAX_OSD_LAYER_NUM; layer_id++)
    {
        later_info = get_osd_layer_config(layer_id);
        if (later_info->enable)
        {
            ret = vou_osd_layer_init(layer_id);
            SDK_FUNC_ERROR(model_name, ret);

            ret = osd_send_stream(layer_id);
            SDK_FUNC_ERROR(model_name, ret);
        }
    }

    return ret;
}

FH_SINT32 sample_common_vou_init(FH_VOID)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;

    ret = sample_common_vou_device_init();
    SDK_FUNC_ERROR(model_name, ret);

    ret = sample_common_video_layer_init();
    SDK_FUNC_ERROR(model_name, ret);

    ret = sample_common_video_layer_chn_init();
    SDK_FUNC_ERROR(model_name, ret);

    ret = sample_common_osd_layer_init();
    SDK_FUNC_ERROR(model_name, ret);

    // ret = sample_common_vou_test();
    // SDK_FUNC_ERROR(model_name, ret);
    // printf("sample_common_vou_test\n");

    return ret;
}

FH_SINT32 sample_common_video_layer_chn_exit(FH_VOID)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    VIDEO_LAYER_CHN_INFO *later_chn_info;

    for (FH_SINT32 chn_id = 0; chn_id < MAX_VIDEO_LAYER_CHN_NUM; chn_id++)
    {
        later_chn_info = get_video_layer_chn_config(chn_id);
        if (later_chn_info->enable)
        {
            ret = vou_video_layer_chn_exit(chn_id);
            SDK_FUNC_ERROR(model_name, ret);
        }
    }

    return ret;
}

FH_SINT32 sample_common_osd_layer_exit(FH_VOID)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    OSD_LAYER_INFO *later_info;

    for (FH_SINT32 layer_id = 0; layer_id < MAX_OSD_LAYER_NUM; layer_id++)
    {
        later_info = get_osd_layer_config(layer_id);
        if (later_info->enable)
        {
            ret = vou_osd_layer_exit(layer_id);
            SDK_FUNC_ERROR(model_name, ret);
        }
    }

    return ret;
}

FH_SINT32 sample_common_video_layer_exit(FH_VOID)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    VIDEO_LAYER_CHN_INFO *later_chn_info;

    for (FH_SINT32 chn_id = 0; chn_id < MAX_VIDEO_LAYER_CHN_NUM; chn_id++)
    {
        later_chn_info = get_video_layer_chn_config(chn_id);
        if (later_chn_info->enable)
        {
            ret = vou_video_layer_exit(chn_id);
            SDK_FUNC_ERROR(model_name, ret);
        }
    }

    return ret;
}

FH_SINT32 sample_common_vou_device_exit(FH_VOID)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    DHD_INFO *dhd_info;

    for (FH_SINT32 dhd_id = 0; dhd_id < MAX_DHD_DEV_NUM; dhd_id++)
    {
        dhd_info = get_dhd_config(dhd_id);
        if (dhd_info->enable)
        {
            ret = dhd_device_exit(dhd_id);
            SDK_FUNC_ERROR(model_name, ret);
        }
    }

    ret = vou_close_fd();
    SDK_FUNC_ERROR(model_name, ret);

    return ret;
}

FH_SINT32 sample_common_vou_exit(FH_VOID)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;

    ret = sample_common_video_layer_chn_exit();
    SDK_FUNC_ERROR(model_name, ret);

    ret = sample_common_osd_layer_exit();
    SDK_FUNC_ERROR(model_name, ret);

    ret = sample_common_video_layer_exit();
    SDK_FUNC_ERROR(model_name, ret);

    ret = sample_common_vou_device_exit();
    SDK_FUNC_ERROR(model_name, ret);

    return ret;
}
