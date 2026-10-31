#include "sample_common_vicap.h"

static char *model_name = MODEL_TAG_VICAP;

FH_SINT32 sample_common_vicap_get_frame_blk_size(FH_SINT32 grp_id, FH_UINT32 *blk_size)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    FH_VICAP_BUF_SIZE_ATTR_S stBufSizeAttr;
    ISP_INFO *isp_info;

    *blk_size = 0;
    memset(&stBufSizeAttr, 0, sizeof(FH_VICAP_BUF_SIZE_ATTR_S));
    isp_info = get_isp_config(grp_id);
    if (isp_info->enable)
    {
#ifdef CONFIG_VICAP_OFFLINE_MODE
        stBufSizeAttr.u8DevNum = 1;
        stBufSizeAttr.astBufCfg[0].u8DevId = grp_id;
        stBufSizeAttr.astBufCfg[0].stDevAttr.enWorkMode = VICAP_WORK_MODE_OFFLINE;
        stBufSizeAttr.astBufCfg[0].stDevAttr.stSize.u16Width = isp_info->isp_init_width;
        stBufSizeAttr.astBufCfg[0].stDevAttr.stSize.u16Height = isp_info->isp_init_height;
        ret = FH_VICAP_GetBufSize(&stBufSizeAttr);
        SDK_FUNC_ERROR(model_name, ret);
        *blk_size = stBufSizeAttr.astBufCfg[0].u32BufSize / 6;
#else
        *blk_size = 0;
        SDK_PRT(model_name, "Vicap[%d] Online!\n", grp_id);
#endif
    }
    else
    {
        *blk_size = 0;
    }

    return ret;
}
