#include "sample_common_enc.h"

static char *model_name = MODEL_TAG_ENC;

FH_SINT32 sample_common_enc_init(FH_VOID)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    VPU_INFO *vpu_info;
    ENC_INFO *enc_info;

    ret = enc_mod_set();
    SDK_FUNC_ERROR(model_name, ret);

    for (FH_SINT32 grp_id = 0; grp_id < MAX_GRP_NUM; grp_id++)
    {
        vpu_info = get_vpu_config(grp_id);
        if (vpu_info->enable)
        {
            for (FH_SINT32 chn_id = 0; chn_id < MAX_VPU_CHN_NUM; chn_id++)
            {
                enc_info = get_enc_config(grp_id, chn_id);
                if (enc_info->enable)
                {
                    ret = enc_create_chn(grp_id, chn_id);
                    SDK_FUNC_ERROR(model_name, ret);

                    ret = enc_set_chn_attr(grp_id, chn_id);
                    SDK_FUNC_ERROR(model_name, ret);

                    ret = enc_start(grp_id, chn_id);
                    SDK_FUNC_ERROR(model_name, ret);
                }
            }
        }
    }

    return ret;
}
