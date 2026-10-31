#include "sample_common_dsp.h"

static char *model_name = MODEL_TAG_DSP;

static FH_VOID dsp_proc_init(FH_VOID)
{
    WR_PROC_DEV(VPU_PROC, "procmode_1"); // vpu proc
#if defined CH0_LPBUF_ENABLE_G0 || defined CH0_LPBUF_ENABLE_G1
    WR_PROC_DEV(VPU_PROC, "cirbuf_on"); // vpu proc
#else
    WR_PROC_DEV(VPU_PROC, "cirbuf_off"); // vpu proc
#endif
}

static FH_SINT32 VB_init(FH_VOID)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    FH_UINT32 blk_size = 0;
    VB_CONF_S stVbConf;

    memset(&stVbConf, 0, sizeof(VB_CONF_S));
    FH_VB_Exit();

    stVbConf.u32MaxPoolCnt = 0;
    for (FH_SINT32 grp_id = 0; grp_id < MAX_GRP_NUM; grp_id++)
    {
        for (FH_SINT32 chn_id = 0; chn_id < MAX_VPU_CHN_NUM; chn_id++)
        {
            ret = sample_common_vpu_get_frame_blk_size(grp_id, chn_id, &blk_size);
            SDK_FUNC_ERROR("VB", ret);
            if (blk_size)
            {
                stVbConf.astCommPool[stVbConf.u32MaxPoolCnt].u32BlkSize = blk_size;
                stVbConf.astCommPool[stVbConf.u32MaxPoolCnt].u32BlkCnt = 3;
                stVbConf.u32MaxPoolCnt++;
                SDK_PRT("VB DSP", "GRP[%d] CHN[%d] blk_size=0x%X(%d)\n", grp_id, chn_id, blk_size, blk_size);
            }
        }

        ret = sample_common_vicap_get_frame_blk_size(grp_id, &blk_size);
        SDK_FUNC_ERROR("VB", ret);
        if (blk_size)
        {
            stVbConf.astCommPool[stVbConf.u32MaxPoolCnt].u32BlkSize = blk_size;
            stVbConf.astCommPool[stVbConf.u32MaxPoolCnt].u32BlkCnt = 3;
            stVbConf.u32MaxPoolCnt++;
            SDK_PRT("VB VICAP", "GRP[%d] blk_size=0x%X(%d)\n", grp_id, blk_size, blk_size);
        }

        ret = sample_common_isp_get_frame_blk_size(grp_id, &blk_size);
        SDK_FUNC_ERROR("VB", ret);
        if (blk_size)
        {
            stVbConf.astCommPool[stVbConf.u32MaxPoolCnt].u32BlkSize = blk_size;
            stVbConf.astCommPool[stVbConf.u32MaxPoolCnt].u32BlkCnt = 3;
            stVbConf.u32MaxPoolCnt++;
            SDK_PRT("VB ISP", "GRP[%d] blk_size=0x%X(%d)\n", grp_id, blk_size, blk_size);
        }
    }

    ret = FH_VB_SetConf(&stVbConf);
    SDK_FUNC_ERROR("VB", ret);

    ret = FH_VB_Init();
    SDK_FUNC_ERROR("VB", ret);

    return ret;
}

static FH_SINT32 VB_exit(FH_VOID)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;

    ret = FH_VB_Exit();
    SDK_FUNC_ERROR(model_name, ret);

    return ret;
}

FH_SINT32 sample_common_dsp_system_init(FH_VOID)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;

    dsp_proc_init();

    ret = FH_SYS_Init();
    SDK_FUNC_ERROR(model_name, ret);

    ret = API_ISP_Open();
    SDK_FUNC_ERROR(model_name, ret);

    ret = VB_init();
    SDK_FUNC_ERROR(model_name, ret);
    return ret;
}

FH_SINT32 sample_common_dsp_system_exit(FH_VOID)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;

    ret = API_ISP_Close();
    SDK_FUNC_ERROR(model_name, ret);

    ret = FH_SYS_Exit();
    SDK_FUNC_ERROR(model_name, ret);

    ret = VB_exit();
    SDK_FUNC_ERROR(model_name, ret);

    return ret;
}

FH_SINT32 sample_common_dsp_init(FH_VOID)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;

    ret = sample_common_vpu_init();
    SDK_FUNC_ERROR(model_name, ret);

    ret = sample_common_vpu_chn_init();
    SDK_FUNC_ERROR(model_name, ret);

    ret = sample_common_enc_init();
    SDK_FUNC_ERROR(model_name, ret);

    ret = sample_common_vou_init();
    SDK_FUNC_ERROR(model_name, ret);

    ret = sample_common_vpu_bind();
    SDK_FUNC_ERROR(model_name, ret);

    return ret;
}

FH_SINT32 sample_common_dsp_exit(FH_VOID)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;

    ret = sample_common_vou_exit();
    SDK_FUNC_ERROR(model_name, ret);

    ret = sample_common_enc_exit();
    SDK_FUNC_ERROR(model_name, ret);

    return ret;
}