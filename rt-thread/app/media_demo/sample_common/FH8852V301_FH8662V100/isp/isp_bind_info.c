#include "isp_bind_info.h"

static char *model_name = MODEL_TAG_ISP_BIND_INFO;

ISP_BIND_INFO g_isp_bind_info[MAX_GRP_NUM] = {
    /*********************G0************************/
    {
        .toVpu = 0,
        .lut2d = 0,
#if defined(VPU_MODE_OFFLINE_2DLUT_ISP_G0)
        .lut2d = 1,
#endif
    },
    /*********************G1************************/
    {
        .toVpu = 0,
        .lut2d = 0,
#if defined(VPU_MODE_OFFLINE_2DLUT_ISP_G1)
        .lut2d = 1,
#endif
    },
};

ISP_BIND_INFO *get_isp_bind_info(FH_SINT32 grp_id)
{
    return &g_isp_bind_info[grp_id];
}

FH_SINT32 isp_bind_vpu(FH_SINT32 grp_id)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    FH_BIND_INFO src, dst;

    src.obj_id = FH_OBJ_ISP;
    src.dev_id = grp_id;
    if (g_isp_bind_info[grp_id].lut2d == 1)
    {
        src.chn_id = 1;
    }
    else
    {
        src.chn_id = 0;
    }

    dst.obj_id = FH_OBJ_VPU_VI;
    dst.dev_id = grp_id;
    dst.chn_id = 0;
    ret = FH_SYS_Bind(src, dst);
    SDK_FUNC_ERROR(model_name, ret);

    return ret;
}
