#include "sample_common_bgm.h"
#include "types/type_def.h"

static char *model_name = MODEL_TAG_BGM;

FH_SINT32 sample_common_bgm_init(FH_VOID)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    SDK_FUNC_ERROR(model_name, ret);
    return ret; // bgm not support
}

FH_SINT32 sample_common_bgm_exit(FH_VOID)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    SDK_FUNC_ERROR(model_name, ret);
    return ret; // bgm not support
}
