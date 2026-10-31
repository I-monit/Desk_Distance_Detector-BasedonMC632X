#include "vise_bind_info.h"

static char *model_name = MODEL_TAG_VISE_BIND_INFO;

FH_SINT32 vise_bind_enc(FH_SINT32 vise_id, FH_SINT32 enc_chn)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    FH_BIND_INFO src, dst;

    src.obj_id = FH_OBJ_VISE;
    src.dev_id = vise_id;
    src.chn_id = 0;

    dst.obj_id = FH_OBJ_ENC;
    dst.dev_id = 0;
    dst.chn_id = enc_chn;

    ret = FH_SYS_Bind(src, dst);
    SDK_FUNC_ERROR(model_name, ret);

    return ret;
}

FH_SINT32 vise_bind_jpeg(FH_UINT32 vise_id, FH_UINT32 jpeg_chn)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    FH_BIND_INFO src, dst;

    src.obj_id = FH_OBJ_VISE;
    src.dev_id = vise_id;
    src.chn_id = 0;

    dst.obj_id = FH_OBJ_JPEG;
    dst.dev_id = 0;
    dst.chn_id = jpeg_chn;

    ret = FH_SYS_Bind(src, dst);
    SDK_FUNC_ERROR(model_name, ret);

    return ret;
}
