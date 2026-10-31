#include "sample_common_vise.h"

static char *model_name = MODEL_TAG_VISE;

typedef struct vise_vb_pool
{
    FH_UINT32 pool_id;
    FH_SINT32 size;
} VISE_VB_POOL;

VISE_VB_POOL g_vb_pool[MAX_VISE_CANVAS_NUM] = {0};
FH_SINT32 g_vise_status[MAX_VISE_CANVAS_NUM] = {0, 0, 0, 0};

FH_SINT32 checkViseAttr(FH_SINT32 chn_id, FH_SINT32 width, FH_SINT32 height, FH_SINT32 yuv_type)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    switch (yuv_type)
    {
    case VPU_VOMODE_BLK:
        SDK_CHECK_ALIGN(model_name, width, 16);
        SDK_CHECK_ALIGN(model_name, height, 16);
        break;

    case VPU_VOMODE_SCAN:
        SDK_CHECK_ALIGN(model_name, width, 8);
        SDK_CHECK_ALIGN(model_name, height, 2);
        break;
    case VPU_VOMODE_TILE192:
    case VPU_VOMODE_TILE224:
    case VPU_VOMODE_TILE256:
        SDK_CHECK_ALIGN(model_name, width, 64);
        SDK_CHECK_ALIGN(model_name, height, 4);
        break;
    }

#ifdef FH_APP_VISE_PIP
    if (width < VISE_MIN_W)
    {
        SDK_ERR_PRT(model_name, "PIP mode Vi width[%d] < Min width[%d]!\n", width, VISE_MIN_W);
        ret = FH_SDK_FAILED;
    }
    else if (height < VISE_MIN_H)
    {
        SDK_ERR_PRT(model_name, "PIP mode Vi width[%d] < Min width[%d]!\n", width, VISE_MIN_H);
        ret = FH_SDK_FAILED;
    }
#elif defined FH_APP_VISE_LRS
    if (width > VISE_MAX_W / 2)
    {
        SDK_ERR_PRT(model_name, "LRS mode Vi width[%d] > Max width[%d]!\n", width * 2, VISE_MAX_W);
        ret = FH_SDK_FAILED;
    }
#elif defined FH_APP_VISE_TBS
    if (height > VISE_MAX_H / 2)
    {
        SDK_ERR_PRT(model_name, "TBS mode Vi height[%d] > Max height[%d]!\n", height * 2, VISE_MAX_H);
        ret = FH_SDK_FAILED;
    }
#elif defined FH_APP_VISE_MBS
    if (width > VISE_MAX_W / 2)
    {
        SDK_ERR_PRT(model_name, "MBS mode Vi width[%d] > Max width[%d]!\n", width * 2, VISE_MAX_W);
        ret = FH_SDK_FAILED;
    }
    else if (height > VISE_MAX_H / 2)
    {
        SDK_ERR_PRT(model_name, "MBS mode Vi height[%d] > Max height[%d]!\n", height * 2, VISE_MAX_H);
        ret = FH_SDK_FAILED;
    }
#endif

    return ret;
}

FH_SINT32 changeVise_PipAttr(FH_SINT32 vise_id, FH_AREA outside, FH_AREA inside, FH_SINT32 yuv_type)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;

    FH_UINT32 blk_size = 0;

    ret = checkViseAttr(FH_APP_VISE_PIP_OUTSIDE_CHN, outside.u32Width, outside.u32Height, yuv_type);
    SDK_FUNC_ERROR(model_name, ret);

    ret = checkViseAttr(FH_APP_VISE_PIP_INSIDE_CHN, inside.u32Width, inside.u32Height, yuv_type);
    SDK_FUNC_ERROR(model_name, ret);

    ret = get_vpu_chn_blk_size(FH_APP_VISE_PIP_OUTSIDE_GRP, FH_APP_VISE_PIP_OUTSIDE_CHN, &blk_size);
    SDK_FUNC_ERROR(model_name, ret);

    ret = vise_grp_stop(vise_id);
    SDK_FUNC_ERROR(model_name, ret);

    if (g_vb_pool[vise_id].size < blk_size)
    {
        SDK_PRT(model_name, "vise vb resize from %d to %d\n", g_vb_pool[vise_id].size, blk_size);
        g_vb_pool[vise_id].size = blk_size;
        g_vb_pool[vise_id].pool_id = FH_VB_CreatePool(blk_size, 4, NULL); // 旧buffer未回收，应用退出后通过vb_exit一起回收
        if (g_vb_pool[vise_id].pool_id == VB_INVALID_POOLID)
        {
            SDK_ERR_PRT(model_name, "FH_VB_CreatePool failed\n");
            return FH_SDK_FAILED;
        }

        ret = vise_set_buffer(vise_id, g_vb_pool[vise_id].pool_id);
        SDK_FUNC_ERROR(model_name, ret);
    }

    ret = vise_pip_change_area_attr(vise_id, outside, inside, yuv_type);
    SDK_FUNC_ERROR(model_name, ret);

    ret = vise_grp_start(vise_id);
    SDK_FUNC_ERROR(model_name, ret);

    return ret;
}

FH_SINT32 changeVise_LrsAttr(FH_SINT32 vise_id, FH_AREA left, FH_AREA right, FH_SINT32 yuv_type)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;

    FH_UINT32 blk_size = 0;

    ret = checkViseAttr(FH_APP_VISE_LRS_LEFT_CHN, left.u32Width, left.u32Height, yuv_type);
    SDK_FUNC_ERROR(model_name, ret);

    ret = checkViseAttr(FH_APP_VISE_LRS_RIGHT_CHN, right.u32Width, right.u32Height, yuv_type);
    SDK_FUNC_ERROR(model_name, ret);

    ret = get_vpu_chn_blk_size_with_wh(FH_APP_VISE_LRS_LEFT_GRP, FH_APP_VISE_LRS_LEFT_CHN, left.u32Width + right.u32Width, left.u32Height, &blk_size);
    SDK_FUNC_ERROR(model_name, ret);

    ret = vise_grp_stop(vise_id);
    SDK_FUNC_ERROR(model_name, ret);

    if (g_vb_pool[vise_id].size < blk_size)
    {
        SDK_PRT(model_name, "vise vb resize from %d to %d\n", g_vb_pool[vise_id].size, blk_size);
        g_vb_pool[vise_id].size = blk_size;
        g_vb_pool[vise_id].pool_id = FH_VB_CreatePool(blk_size, 4, NULL); // 旧buffer未回收，应用退出后通过vb_exit一起回收
        if (g_vb_pool[vise_id].pool_id == VB_INVALID_POOLID)
        {
            SDK_ERR_PRT(model_name, "FH_VB_CreatePool failed\n");
            return FH_SDK_FAILED;
        }

        ret = vise_set_buffer(vise_id, g_vb_pool[vise_id].pool_id);
        SDK_FUNC_ERROR(model_name, ret);
    }

    ret = vise_lrs_change_area_attr(vise_id, left, right, yuv_type);
    SDK_FUNC_ERROR(model_name, ret);

    ret = vise_grp_start(vise_id);
    SDK_FUNC_ERROR(model_name, ret);

    return ret;
}

FH_SINT32 changeVise_TbsAttr(FH_SINT32 vise_id, FH_AREA top, FH_AREA bottom, FH_SINT32 yuv_type)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;

    FH_UINT32 blk_size = 0;

    ret = checkViseAttr(FH_APP_VISE_TBS_TOP_CHN, top.u32Width, top.u32Height, yuv_type);
    SDK_FUNC_ERROR(model_name, ret);

    ret = checkViseAttr(FH_APP_VISE_TBS_BOTTOM_CHN, bottom.u32Width, bottom.u32Height, yuv_type);
    SDK_FUNC_ERROR(model_name, ret);

    ret = get_vpu_chn_blk_size_with_wh(FH_APP_VISE_TBS_TOP_GRP, FH_APP_VISE_TBS_TOP_CHN, top.u32Width, top.u32Height + bottom.u32Height, &blk_size);
    SDK_FUNC_ERROR(model_name, ret);

    ret = vise_grp_stop(vise_id);
    SDK_FUNC_ERROR(model_name, ret);

    if (g_vb_pool[vise_id].size < blk_size)
    {
        SDK_PRT(model_name, "vise vb resize from %d to %d\n", g_vb_pool[vise_id].size, blk_size);
        g_vb_pool[vise_id].size = blk_size;
        g_vb_pool[vise_id].pool_id = FH_VB_CreatePool(blk_size, 4, NULL); // 旧buffer未回收，应用退出后通过vb_exit一起回收
        if (g_vb_pool[vise_id].pool_id == VB_INVALID_POOLID)
        {
            SDK_ERR_PRT(model_name, "FH_VB_CreatePool failed\n");
            return FH_SDK_FAILED;
        }

        ret = vise_set_buffer(vise_id, g_vb_pool[vise_id].pool_id);
        SDK_FUNC_ERROR(model_name, ret);
    }

    ret = vise_tbs_change_area_attr(vise_id, top, bottom, yuv_type);
    SDK_FUNC_ERROR(model_name, ret);

    ret = vise_grp_start(vise_id);
    SDK_FUNC_ERROR(model_name, ret);

    return ret;
}

FH_SINT32 changeVise_MbsAttr(FH_SINT32 vise_id, FH_AREA left_top, FH_AREA left_bottom, FH_AREA right_top, FH_AREA right_bottom, FH_SINT32 yuv_type)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;

    FH_UINT32 blk_size = 0;

    ret = checkViseAttr(FH_APP_VISE_MBS_LEFT_TOP_CHN, left_top.u32Width, left_top.u32Height, yuv_type);
    SDK_FUNC_ERROR(model_name, ret);

    ret = checkViseAttr(FH_APP_VISE_MBS_LEFT_BOTTOM_CHN, left_bottom.u32Width, left_bottom.u32Height, yuv_type);
    SDK_FUNC_ERROR(model_name, ret);

    ret = checkViseAttr(FH_APP_VISE_MBS_RIGHT_TOP_CHN, right_top.u32Width, right_top.u32Height, yuv_type);
    SDK_FUNC_ERROR(model_name, ret);

    ret = checkViseAttr(FH_APP_VISE_MBS_RIGHT_BOTTOM_CHN, right_bottom.u32Width, right_bottom.u32Height, yuv_type);
    SDK_FUNC_ERROR(model_name, ret);

    ret = get_vpu_chn_blk_size_with_wh(FH_APP_VISE_MBS_LEFT_TOP_GRP, FH_APP_VISE_MBS_LEFT_TOP_CHN, left_top.u32Width + right_top.u32Width, left_top.u32Height + left_bottom.u32Height, &blk_size);
    SDK_FUNC_ERROR(model_name, ret);

    ret = vise_grp_stop(vise_id);
    SDK_FUNC_ERROR(model_name, ret);

    if (g_vb_pool[vise_id].size < blk_size)
    {
        SDK_PRT(model_name, "vise vb resize from %d to %d\n", g_vb_pool[vise_id].size, blk_size);
        g_vb_pool[vise_id].size = blk_size;
        g_vb_pool[vise_id].pool_id = FH_VB_CreatePool(blk_size, 8, NULL); // 旧buffer未回收，应用退出后通过vb_exit一起回收
        if (g_vb_pool[vise_id].pool_id == VB_INVALID_POOLID)
        {
            SDK_ERR_PRT(model_name, "FH_VB_CreatePool failed\n");
            return FH_SDK_FAILED;
        }

        ret = vise_set_buffer(vise_id, g_vb_pool[vise_id].pool_id);
        SDK_FUNC_ERROR(model_name, ret);
    }

    ret = vise_mbs_change_area_attr(vise_id, left_top, left_bottom, right_top, right_bottom, yuv_type);
    SDK_FUNC_ERROR(model_name, ret);

    ret = vise_grp_start(vise_id);
    SDK_FUNC_ERROR(model_name, ret);

    return ret;
}

FH_SINT32 sample_common_vise_pip_init(FH_SINT32 vise_id)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;

    FH_VISE_CANVAS_ATTR pstcanvasattr;
    FH_UINT32 blk_size = 0;

    VPU_CHN_INFO *outside_chn;
    VPU_CHN_INFO *inside_chn;

    outside_chn = get_vpu_chn_config(FH_APP_VISE_PIP_OUTSIDE_GRP, FH_APP_VISE_PIP_OUTSIDE_CHN);
    inside_chn = get_vpu_chn_config(FH_APP_VISE_PIP_INSIDE_GRP, FH_APP_VISE_PIP_INSIDE_CHN);

    pstcanvasattr.size.u32Width = outside_chn->width;
    pstcanvasattr.size.u32Height = outside_chn->height;
    pstcanvasattr.offset = 0;
    pstcanvasattr.stride = 0;
    pstcanvasattr.depth = 1;
    pstcanvasattr.pipeline_num = 2;
    pstcanvasattr.data_format = outside_chn->yuv_type;
    pstcanvasattr.strict_pts_chk = 0;
    pstcanvasattr.frame_ctrl.enable = 0;
    pstcanvasattr.canvas_area_num = 2;

    pstcanvasattr.canvas_area[0].layer = 0;
    pstcanvasattr.canvas_area[0].extcopy = 0;
    pstcanvasattr.canvas_area[0].area.u32X = 0;
    pstcanvasattr.canvas_area[0].area.u32Y = 0;
    pstcanvasattr.canvas_area[0].area.u32Width = outside_chn->width;
    pstcanvasattr.canvas_area[0].area.u32Height = outside_chn->height;

    pstcanvasattr.canvas_area[1].layer = 1;
    pstcanvasattr.canvas_area[1].extcopy = 0;
    pstcanvasattr.canvas_area[1].area.u32X = outside_chn->width - inside_chn->width;
    pstcanvasattr.canvas_area[1].area.u32Y = 0;
    pstcanvasattr.canvas_area[1].area.u32Width = inside_chn->width;
    pstcanvasattr.canvas_area[1].area.u32Height = inside_chn->height;

    ret = get_vpu_chn_blk_size(FH_APP_VISE_PIP_OUTSIDE_GRP, FH_APP_VISE_PIP_OUTSIDE_CHN, &blk_size);
    SDK_FUNC_ERROR(model_name, ret);

    g_vb_pool[vise_id].size = blk_size;
    g_vb_pool[vise_id].pool_id = FH_VB_CreatePool(blk_size, 4, NULL);
    if (g_vb_pool[vise_id].pool_id == VB_INVALID_POOLID)
    {
        SDK_ERR_PRT(model_name, "FH_VB_CreatePool failed\n");
        return FH_SDK_FAILED;
    }

    ret = vise_canvas_init(vise_id, &pstcanvasattr);
    SDK_FUNC_ERROR(model_name, ret);

    ret = vise_set_buffer(vise_id, g_vb_pool[vise_id].pool_id);
    SDK_FUNC_ERROR(model_name, ret);

    return ret;
}

FH_SINT32 sample_common_vise_lrs_init(FH_SINT32 vise_id)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;

    FH_VISE_CANVAS_ATTR pstcanvasattr;
    FH_UINT32 blk_size = 0;

    VPU_CHN_INFO *left_chn;
    VPU_CHN_INFO *right_chn;

    left_chn = get_vpu_chn_config(FH_APP_VISE_LRS_LEFT_GRP, FH_APP_VISE_LRS_LEFT_CHN);
    right_chn = get_vpu_chn_config(FH_APP_VISE_LRS_RIGHT_GRP, FH_APP_VISE_LRS_RIGHT_CHN);

    pstcanvasattr.size.u32Width = left_chn->width + right_chn->width;
    pstcanvasattr.size.u32Height = left_chn->height;
    pstcanvasattr.offset = 0;
    pstcanvasattr.stride = 0;
    pstcanvasattr.depth = 1;
    pstcanvasattr.pipeline_num = 2;
    pstcanvasattr.data_format = left_chn->yuv_type;
    pstcanvasattr.strict_pts_chk = 0;
    pstcanvasattr.frame_ctrl.enable = 0;
    pstcanvasattr.canvas_area_num = 2;

    pstcanvasattr.canvas_area[0].layer = 0;
    pstcanvasattr.canvas_area[0].extcopy = 0;
    pstcanvasattr.canvas_area[0].area.u32X = 0;
    pstcanvasattr.canvas_area[0].area.u32Y = 0;
    pstcanvasattr.canvas_area[0].area.u32Width = left_chn->width;
    pstcanvasattr.canvas_area[0].area.u32Height = left_chn->height;

    pstcanvasattr.canvas_area[1].layer = 0;
    pstcanvasattr.canvas_area[1].extcopy = 0;
    pstcanvasattr.canvas_area[1].area.u32X = left_chn->width;
    pstcanvasattr.canvas_area[1].area.u32Y = 0;
    pstcanvasattr.canvas_area[1].area.u32Width = right_chn->width;
    pstcanvasattr.canvas_area[1].area.u32Height = right_chn->height;

    ret = get_vpu_chn_blk_size_with_wh(FH_APP_VISE_LRS_LEFT_GRP, FH_APP_VISE_LRS_LEFT_CHN, left_chn->width + right_chn->width, left_chn->height, &blk_size);
    SDK_FUNC_ERROR(model_name, ret);

    g_vb_pool[vise_id].size = blk_size;
    g_vb_pool[vise_id].pool_id = FH_VB_CreatePool(blk_size, 4, NULL);
    if (g_vb_pool[vise_id].pool_id == VB_INVALID_POOLID)
    {
        SDK_ERR_PRT(model_name, "FH_VB_CreatePool failed\n");
        return FH_SDK_FAILED;
    }

    ret = vise_canvas_init(vise_id, &pstcanvasattr);
    SDK_FUNC_ERROR(model_name, ret);

    ret = vise_set_buffer(vise_id, g_vb_pool[vise_id].pool_id);
    SDK_FUNC_ERROR(model_name, ret);

    return ret;
}

FH_SINT32 sample_common_vise_tbs_init(FH_SINT32 vise_id)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;

    FH_VISE_CANVAS_ATTR pstcanvasattr;
    FH_UINT32 blk_size = 0;

    VPU_CHN_INFO *top_chn;
    VPU_CHN_INFO *bottom_chn;

    top_chn = get_vpu_chn_config(FH_APP_VISE_TBS_TOP_GRP, FH_APP_VISE_TBS_TOP_CHN);
    bottom_chn = get_vpu_chn_config(FH_APP_VISE_TBS_BOTTOM_GRP, FH_APP_VISE_TBS_BOTTOM_CHN);

    pstcanvasattr.size.u32Width = top_chn->width;
    pstcanvasattr.size.u32Height = top_chn->height + bottom_chn->height;
    pstcanvasattr.offset = 0;
    pstcanvasattr.stride = 0;
    pstcanvasattr.depth = 1;
    pstcanvasattr.pipeline_num = 2;
    pstcanvasattr.data_format = top_chn->yuv_type;
    pstcanvasattr.strict_pts_chk = 0;
    pstcanvasattr.frame_ctrl.enable = 0;
    pstcanvasattr.canvas_area_num = 2;

    pstcanvasattr.canvas_area[0].layer = 0;
    pstcanvasattr.canvas_area[0].extcopy = 0;
    pstcanvasattr.canvas_area[0].area.u32X = 0;
    pstcanvasattr.canvas_area[0].area.u32Y = 0;
    pstcanvasattr.canvas_area[0].area.u32Width = top_chn->width;
    pstcanvasattr.canvas_area[0].area.u32Height = top_chn->height;

    pstcanvasattr.canvas_area[1].layer = 0;
    pstcanvasattr.canvas_area[1].extcopy = 0;
    pstcanvasattr.canvas_area[1].area.u32X = 0;
    pstcanvasattr.canvas_area[1].area.u32Y = top_chn->height;
    pstcanvasattr.canvas_area[1].area.u32Width = bottom_chn->width;
    pstcanvasattr.canvas_area[1].area.u32Height = bottom_chn->height;

    ret = get_vpu_chn_blk_size_with_wh(FH_APP_VISE_TBS_TOP_GRP, FH_APP_VISE_TBS_TOP_CHN, top_chn->width, top_chn->height + bottom_chn->height, &blk_size);
    SDK_FUNC_ERROR(model_name, ret);

    g_vb_pool[vise_id].size = blk_size;
    g_vb_pool[vise_id].pool_id = FH_VB_CreatePool(blk_size, 4, NULL);
    if (g_vb_pool[vise_id].pool_id == VB_INVALID_POOLID)
    {
        SDK_ERR_PRT(model_name, "FH_VB_CreatePool failed\n");
        return FH_SDK_FAILED;
    }

    ret = vise_canvas_init(vise_id, &pstcanvasattr);
    SDK_FUNC_ERROR(model_name, ret);

    ret = vise_set_buffer(vise_id, g_vb_pool[vise_id].pool_id);
    SDK_FUNC_ERROR(model_name, ret);

    return ret;
}

FH_SINT32 sample_common_vise_mbs_init(FH_SINT32 vise_id)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;

    FH_VISE_CANVAS_ATTR pstcanvasattr;
    FH_UINT32 blk_size = 0;

    VPU_CHN_INFO *left_top_chn;
    VPU_CHN_INFO *left_bottom_chn;
    VPU_CHN_INFO *right_top_chn;
    VPU_CHN_INFO *right_bottom_chn;

    left_top_chn = get_vpu_chn_config(FH_APP_VISE_MBS_LEFT_TOP_GRP, FH_APP_VISE_MBS_LEFT_TOP_CHN);
    left_bottom_chn = get_vpu_chn_config(FH_APP_VISE_MBS_LEFT_BOTTOM_GRP, FH_APP_VISE_MBS_LEFT_BOTTOM_CHN);
    right_top_chn = get_vpu_chn_config(FH_APP_VISE_MBS_RIGHT_TOP_GRP, FH_APP_VISE_MBS_RIGHT_TOP_CHN);
    right_bottom_chn = get_vpu_chn_config(FH_APP_VISE_MBS_RIGHT_BOTTOM_GRP, FH_APP_VISE_MBS_RIGHT_BOTTOM_CHN);

    pstcanvasattr.size.u32Width = left_top_chn->width + right_top_chn->width;
    pstcanvasattr.size.u32Height = left_top_chn->height + left_bottom_chn->height;
    pstcanvasattr.offset = 0;
    pstcanvasattr.stride = 0;
    pstcanvasattr.depth = 1;
    pstcanvasattr.pipeline_num = 2;
    pstcanvasattr.data_format = left_top_chn->yuv_type;
    pstcanvasattr.strict_pts_chk = 0;
    pstcanvasattr.frame_ctrl.enable = 0;
    pstcanvasattr.canvas_area_num = 4;

    pstcanvasattr.canvas_area[0].layer = 0;
    pstcanvasattr.canvas_area[0].extcopy = 0;
    pstcanvasattr.canvas_area[0].area.u32X = 0;
    pstcanvasattr.canvas_area[0].area.u32Y = 0;
    pstcanvasattr.canvas_area[0].area.u32Width = left_top_chn->width;
    pstcanvasattr.canvas_area[0].area.u32Height = left_top_chn->height;

    pstcanvasattr.canvas_area[1].layer = 0;
    pstcanvasattr.canvas_area[1].extcopy = 0;
    pstcanvasattr.canvas_area[1].area.u32X = 0;
    pstcanvasattr.canvas_area[1].area.u32Y = left_top_chn->height;
    pstcanvasattr.canvas_area[1].area.u32Width = left_bottom_chn->width;
    pstcanvasattr.canvas_area[1].area.u32Height = left_bottom_chn->height;

    pstcanvasattr.canvas_area[2].layer = 0;
    pstcanvasattr.canvas_area[2].extcopy = 0;
    pstcanvasattr.canvas_area[2].area.u32X = left_top_chn->width;
    pstcanvasattr.canvas_area[2].area.u32Y = 0;
    pstcanvasattr.canvas_area[2].area.u32Width = right_top_chn->width;
    pstcanvasattr.canvas_area[2].area.u32Height = right_top_chn->height;

    pstcanvasattr.canvas_area[3].layer = 0;
    pstcanvasattr.canvas_area[3].extcopy = 0;
    pstcanvasattr.canvas_area[3].area.u32X = left_top_chn->width;
    pstcanvasattr.canvas_area[3].area.u32Y = left_top_chn->height;
    pstcanvasattr.canvas_area[3].area.u32Width = right_bottom_chn->width;
    pstcanvasattr.canvas_area[3].area.u32Height = right_bottom_chn->height;

    ret = get_vpu_chn_blk_size_with_wh(FH_APP_VISE_MBS_LEFT_TOP_GRP, FH_APP_VISE_MBS_LEFT_TOP_CHN, left_top_chn->width + right_top_chn->width, left_top_chn->height + left_bottom_chn->height, &blk_size);
    SDK_FUNC_ERROR(model_name, ret);

    g_vb_pool[vise_id].size = blk_size;
    g_vb_pool[vise_id].pool_id = FH_VB_CreatePool(blk_size, 8, NULL);
    if (g_vb_pool[vise_id].pool_id == VB_INVALID_POOLID)
    {
        SDK_ERR_PRT(model_name, "FH_VB_CreatePool failed\n");
        return FH_SDK_FAILED;
    }

    ret = vise_canvas_init(vise_id, &pstcanvasattr);
    SDK_FUNC_ERROR(model_name, ret);

    ret = vise_set_buffer(vise_id, g_vb_pool[vise_id].pool_id);
    SDK_FUNC_ERROR(model_name, ret);

    return ret;
}

FH_SINT32 sample_common_vise_start(FH_SINT32 vise_id)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;

    if (!g_vise_status[vise_id])
    {
        ret = vise_grp_start(vise_id);
        SDK_FUNC_ERROR(model_name, ret);

        g_vise_status[vise_id] = 1;
    }
    else
    {
        SDK_ERR_PRT(model_name, "Vise[%d] already run!\n", vise_id);
    }

    return ret;
}

FH_SINT32 sample_common_vise_exit(FH_SINT32 vise_id)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;

    if (g_vise_status[vise_id])
    {
        ret = vise_grp_stop(vise_id);
        SDK_FUNC_ERROR(model_name, ret);

        ret = vise_detach_buffer(vise_id);
        SDK_FUNC_ERROR(model_name, ret);

        ret = vise_exit(vise_id);
        SDK_FUNC_ERROR(model_name, ret);

        g_vise_status[vise_id] = 0;
    }

    return ret;
}
