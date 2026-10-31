#include "bgm_config.h"

static char *model_name = MODEL_TAG_BGM;

FH_SINT32 get_bgm_initStatus(FH_SINT32 grp_id)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    SDK_FUNC_ERROR(model_name, ret);
    return ret; // bgm not support
}

FH_SINT32 bgm_init(FH_SINT32 grp_id)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    SDK_FUNC_ERROR(model_name, ret);
    return ret; // bgm not support
}

FH_SINT32 bgm_exit(FH_SINT32 grp_id)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    SDK_FUNC_ERROR(model_name, ret);
    return ret; // bgm not support
}
