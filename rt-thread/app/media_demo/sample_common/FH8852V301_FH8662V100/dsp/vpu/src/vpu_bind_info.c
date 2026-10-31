#include "vpu_bind_info.h"

static char *model_name = MODEL_TAG_VPU_BIND_INFO;

VPU_BIND_INFO g_vpu_bind_info[MAX_GRP_NUM][MAX_VPU_CHN_NUM] = {
    /*********************G0************************/
    {
        {
            .toVpu = 0,
            .toEnc = 0,
            .toVou = 0,
#if defined CH0_BIND_VPU_G0
            .toVpu = 1,
#endif
#if defined CH0_BIND_ENC_G0
            .toEnc = 1,
#endif
        },

        {
            .toVpu = 0,
            .toEnc = 0,
            .toVou = 0,
#if defined CH1_BIND_VPU_G0
            .toVpu = 1,
#endif
#if defined CH1_BIND_ENC_G0
            .toEnc = 1,
#endif
        },

        {
            .toVpu = 0,
            .toEnc = 0,
            .toVou = 0,
#if defined CH2_BIND_VPU_G0
            .toVpu = 1,
#endif
#if defined CH2_BIND_ENC_G0
            .toEnc = 1,
#endif
        },
    },
    /*********************G1************************/
    {
        {
            .toVpu = 0,
            .toEnc = 0,
            .toVou = 0,
#if defined CH0_BIND_VPU_G1
            .toVpu = 1,
#endif
#if defined CH0_BIND_ENC_G1
            .toEnc = 1,
#endif
        },

        {
            .toVpu = 0,
            .toEnc = 0,
            .toVou = 0,
#if defined CH1_BIND_VPU_G1
            .toVpu = 1,
#endif
#if defined CH1_BIND_ENC_G1
            .toEnc = 1,
#endif
        },

        {
            .toVpu = 0,
            .toEnc = 0,
            .toVou = 0,
#if defined CH2_BIND_VPU_G1
            .toVpu = 1,
#endif
#if defined CH2_BIND_ENC_G1
            .toEnc = 1,
#endif
        },
    },
};

VPU_BIND_INFO *get_vpu_bind_info(FH_SINT32 grp_id, FH_SINT32 chn_id)
{
    return &g_vpu_bind_info[grp_id][chn_id];
}

FH_SINT32 vpu_bind_bgm(FH_SINT32 grp_id)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    SDK_FUNC_ERROR(model_name, ret); // not support bgm
    return ret;
}

FH_SINT32 vpu_bind_enc(FH_SINT32 grp_id, FH_SINT32 chn_id)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    FH_BIND_INFO src, dst;

    src.obj_id = FH_OBJ_VPU_VO;
    src.dev_id = grp_id;
    src.chn_id = chn_id;

    dst.obj_id = FH_OBJ_ENC;
    dst.dev_id = 0;
    dst.chn_id = grp_id * MAX_VPU_CHN_NUM + chn_id;

    ret = FH_SYS_Bind(src, dst);
    SDK_FUNC_ERROR(model_name, ret);

    return ret;
}

FH_SINT32 vpu_bind_vpu(FH_UINT32 src_grp_id, FH_UINT32 src_chn_id, FH_UINT32 dst_grp_id)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    FH_BIND_INFO src, dst;

    src.obj_id = FH_OBJ_VPU_VO;
    src.dev_id = src_grp_id;
    src.chn_id = src_chn_id;

    dst.obj_id = FH_OBJ_VPU_VI;
    dst.dev_id = dst_grp_id;
    dst.chn_id = 0;

    ret = FH_SYS_Bind(src, dst);
    SDK_FUNC_ERROR(model_name, ret);

    return ret;
}

FH_SINT32 vpu_bind_jpeg(FH_UINT32 src_grp_id, FH_UINT32 src_chn_id, FH_UINT32 dst_chn_id)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    FH_BIND_INFO src, dst;

    src.obj_id = FH_OBJ_VPU_VO;
    src.dev_id = src_grp_id;
    src.chn_id = src_chn_id;

    dst.obj_id = FH_OBJ_JPEG;
    dst.dev_id = 0;
    dst.chn_id = dst_chn_id;

    ret = FH_SYS_Bind(src, dst);
    SDK_FUNC_ERROR(model_name, ret);

    return ret;
}

FH_SINT32 vpu_bind_vise(FH_UINT32 src_grp_id, FH_UINT32 src_chn_id, FH_UINT32 dst_grp_id, FH_UINT32 dst_chn_id)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    FH_BIND_INFO src, dst;

    src.obj_id = FH_OBJ_VPU_VO;
    src.dev_id = src_grp_id;
    src.chn_id = src_chn_id;

    dst.obj_id = FH_OBJ_VISE;
    dst.dev_id = dst_grp_id;
    dst.chn_id = dst_chn_id;

    ret = FH_SYS_Bind(src, dst);
    SDK_FUNC_ERROR(model_name, ret);

    return ret;
}

FH_SINT32 unbind_vpu_chn(FH_SINT32 grp_id, FH_SINT32 chn_id)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    FH_BIND_INFO src;

    src.obj_id = FH_OBJ_VPU_VO;
    src.dev_id = grp_id;
    src.chn_id = chn_id;
    ret = FH_SYS_UnBindbySrc(src);
    SDK_FUNC_ERROR(model_name, ret);

    return ret;
}

FH_SINT32 unbind_enc_chn(FH_SINT32 grp_id, FH_SINT32 chn_id)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    FH_BIND_INFO dst;

    dst.obj_id = FH_OBJ_ENC;
    dst.dev_id = 0;
    dst.chn_id = grp_id * MAX_VPU_CHN_NUM + chn_id;

    ret = FH_SYS_UnBindbyDst(dst);
    SDK_FUNC_ERROR(model_name, ret);

    return ret;
}

FH_SINT32 unbind_jpeg_chn(FH_UINT32 jpeg_chn)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    FH_BIND_INFO dst;

    dst.obj_id = FH_OBJ_JPEG;
    dst.dev_id = 0;
    dst.chn_id = jpeg_chn;

    ret = FH_SYS_UnBindbyDst(dst);
    SDK_FUNC_ERROR(model_name, ret);

    return ret;
}
