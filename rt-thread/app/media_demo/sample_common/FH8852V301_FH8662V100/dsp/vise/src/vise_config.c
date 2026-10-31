#include "vise_config.h"

static char *model_name = MODEL_TAG_VISE_CONFIG;

FH_SINT32 vise_canvas_init(FH_SINT32 vise_id, FH_VISE_CANVAS_ATTR *pstcanvasattr)
{
	FH_SINT32 ret = FH_SDK_SUCCESS;
	VPU_CHN_INFO *vpu_chn_info = NULL;

	vpu_chn_info = get_vpu_chn_config(vise_id, 0);
	if (vpu_chn_info->lpbuf_en && vpu_chn_info->enable)
	{
		pstcanvasattr->canvas_lpbuf_en = 1;
	}
	else
	{
		pstcanvasattr->canvas_lpbuf_en = 0;
	}

	ret = FH_VISE_CreateCanvas(vise_id, pstcanvasattr);
	SDK_FUNC_ERROR(model_name, ret);

	return ret;
}

FH_SINT32 vise_pip_change_area_attr(FH_SINT32 vise_id, FH_AREA outside, FH_AREA inside, FH_SINT32 yuv_type)
{
	FH_SINT32 ret = FH_SDK_SUCCESS;
	FH_VISE_CANVAS_ATTR pstcanvasattr;

	ret = FH_VISE_GetCanvasAttr(vise_id, &pstcanvasattr);
	SDK_FUNC_ERROR(model_name, ret);

	pstcanvasattr.data_format = yuv_type;
	pstcanvasattr.size.u32Width = outside.u32Width;
	pstcanvasattr.size.u32Height = outside.u32Height;

	pstcanvasattr.canvas_area[0].area.u32X = outside.u32X;
	pstcanvasattr.canvas_area[0].area.u32Y = outside.u32Y;
	pstcanvasattr.canvas_area[0].area.u32Width = outside.u32Width;
	pstcanvasattr.canvas_area[0].area.u32Height = outside.u32Height;

	pstcanvasattr.canvas_area[1].area.u32X = inside.u32X;
	pstcanvasattr.canvas_area[1].area.u32Y = inside.u32Y;
	pstcanvasattr.canvas_area[1].area.u32Width = inside.u32Width;
	pstcanvasattr.canvas_area[1].area.u32Height = inside.u32Height;

	ret = FH_VISE_SetCanvasAttr(vise_id, &pstcanvasattr);
	SDK_FUNC_ERROR(model_name, ret);

	return ret;
}

FH_SINT32 vise_lrs_change_area_attr(FH_SINT32 vise_id, FH_AREA left, FH_AREA right, FH_SINT32 yuv_type)
{
	FH_SINT32 ret = FH_SDK_SUCCESS;
	FH_VISE_CANVAS_ATTR pstcanvasattr;

	ret = FH_VISE_GetCanvasAttr(vise_id, &pstcanvasattr);
	SDK_FUNC_ERROR(model_name, ret);

	pstcanvasattr.data_format = yuv_type;
	pstcanvasattr.size.u32Width = left.u32Width + right.u32Width;
	pstcanvasattr.size.u32Height = left.u32Height;

	pstcanvasattr.canvas_area[0].area.u32X = left.u32X;
	pstcanvasattr.canvas_area[0].area.u32Y = left.u32Y;
	pstcanvasattr.canvas_area[0].area.u32Width = left.u32Width;
	pstcanvasattr.canvas_area[0].area.u32Height = left.u32Height;

	pstcanvasattr.canvas_area[1].area.u32X = right.u32X;
	pstcanvasattr.canvas_area[1].area.u32Y = right.u32Y;
	pstcanvasattr.canvas_area[1].area.u32Width = right.u32Width;
	pstcanvasattr.canvas_area[1].area.u32Height = right.u32Height;

	ret = FH_VISE_SetCanvasAttr(vise_id, &pstcanvasattr);
	SDK_FUNC_ERROR(model_name, ret);

	return ret;
}

FH_SINT32 vise_tbs_change_area_attr(FH_SINT32 vise_id, FH_AREA top, FH_AREA bottom, FH_SINT32 yuv_type)
{
	FH_SINT32 ret = FH_SDK_SUCCESS;
	FH_VISE_CANVAS_ATTR pstcanvasattr;

	ret = FH_VISE_GetCanvasAttr(vise_id, &pstcanvasattr);
	SDK_FUNC_ERROR(model_name, ret);

	pstcanvasattr.data_format = yuv_type;
	pstcanvasattr.size.u32Width = top.u32Width;
	pstcanvasattr.size.u32Height = top.u32Height + bottom.u32Height;

	pstcanvasattr.canvas_area[0].area.u32X = top.u32X;
	pstcanvasattr.canvas_area[0].area.u32Y = top.u32Y;
	pstcanvasattr.canvas_area[0].area.u32Width = top.u32Width;
	pstcanvasattr.canvas_area[0].area.u32Height = top.u32Height;

	pstcanvasattr.canvas_area[1].area.u32X = bottom.u32X;
	pstcanvasattr.canvas_area[1].area.u32Y = bottom.u32Y;
	pstcanvasattr.canvas_area[1].area.u32Width = bottom.u32Width;
	pstcanvasattr.canvas_area[1].area.u32Height = bottom.u32Height;

	ret = FH_VISE_SetCanvasAttr(vise_id, &pstcanvasattr);
	SDK_FUNC_ERROR(model_name, ret);

	return ret;
}

FH_SINT32 vise_mbs_change_area_attr(FH_SINT32 vise_id, FH_AREA left_top, FH_AREA left_bottom, FH_AREA right_top, FH_AREA right_bottom, FH_SINT32 yuv_type)
{
	FH_SINT32 ret = FH_SDK_SUCCESS;
	FH_VISE_CANVAS_ATTR pstcanvasattr;

	ret = FH_VISE_GetCanvasAttr(vise_id, &pstcanvasattr);
	SDK_FUNC_ERROR(model_name, ret);

	pstcanvasattr.data_format = yuv_type;
	pstcanvasattr.size.u32Width = left_top.u32Width + right_top.u32Width;
	pstcanvasattr.size.u32Height = left_top.u32Height + left_bottom.u32Height;

	pstcanvasattr.canvas_area[0].area.u32X = left_top.u32X;
	pstcanvasattr.canvas_area[0].area.u32Y = left_top.u32Y;
	pstcanvasattr.canvas_area[0].area.u32Width = left_top.u32Width;
	pstcanvasattr.canvas_area[0].area.u32Height = left_top.u32Height;

	pstcanvasattr.canvas_area[1].area.u32X = left_bottom.u32X;
	pstcanvasattr.canvas_area[1].area.u32Y = left_bottom.u32Y;
	pstcanvasattr.canvas_area[1].area.u32Width = left_bottom.u32Width;
	pstcanvasattr.canvas_area[1].area.u32Height = left_bottom.u32Height;

	pstcanvasattr.canvas_area[2].area.u32X = right_top.u32X;
	pstcanvasattr.canvas_area[2].area.u32Y = right_top.u32Y;
	pstcanvasattr.canvas_area[2].area.u32Width = right_top.u32Width;
	pstcanvasattr.canvas_area[2].area.u32Height = right_top.u32Height;

	pstcanvasattr.canvas_area[3].area.u32X = right_bottom.u32X;
	pstcanvasattr.canvas_area[3].area.u32Y = right_bottom.u32Y;
	pstcanvasattr.canvas_area[3].area.u32Width = right_bottom.u32Width;
	pstcanvasattr.canvas_area[3].area.u32Height = right_bottom.u32Height;

	ret = FH_VISE_SetCanvasAttr(vise_id, &pstcanvasattr);
	SDK_FUNC_ERROR(model_name, ret);

	return ret;
}

FH_SINT32 vise_set_buffer(FH_SINT32 vise_id, FH_UINT32 vb_pool_id)
{
	FH_SINT32 ret = FH_SDK_SUCCESS;

	ret = FH_VISE_AttachVbPool(vise_id, vb_pool_id);
	SDK_FUNC_ERROR(model_name, ret);

	return ret;
}

FH_SINT32 vise_detach_buffer(FH_SINT32 vise_id)
{
	FH_SINT32 ret = FH_SDK_SUCCESS;

	ret = FH_VISE_DetachVbPool(vise_id);
	SDK_FUNC_ERROR(model_name, ret);

	return ret;
}

FH_SINT32 vise_get_stream(FH_SINT32 vise_id, FH_VIDEO_FRAME *pstframeinfo)
{
	FH_SINT32 ret = FH_SDK_SUCCESS;

	ret = FH_VISE_GetChnFrame(vise_id, pstframeinfo, 10 * 1000, FH_TRUE);
	SDK_FUNC_ERROR(model_name, ret);

	return ret;
}

FH_SINT32 vise_release_stream(FH_SINT32 vise_id, FH_VIDEO_FRAME *pstframeinfo)
{
	FH_SINT32 ret = FH_SDK_SUCCESS;

	ret = FH_VISE_ReleaseChnFrame(vise_id, pstframeinfo);
	SDK_FUNC_ERROR(model_name, ret);

	return ret;
}

FH_SINT32 vise_grp_start(FH_SINT32 vise_id)
{
	FH_SINT32 ret = FH_SDK_SUCCESS;

	ret = FH_VISE_Start(vise_id);
	SDK_FUNC_ERROR(model_name, ret);

	return ret;
}

FH_SINT32 vise_grp_stop(FH_SINT32 vise_id)
{
	FH_SINT32 ret = FH_SDK_SUCCESS;

	ret = FH_VISE_Stop(vise_id);
	SDK_FUNC_ERROR(model_name, ret);

	return ret;
}

FH_SINT32 vise_exit(FH_SINT32 vise_id)
{

	FH_SINT32 ret = FH_SDK_SUCCESS;

	ret = FH_VISE_DestroyCanvas(vise_id);
	SDK_FUNC_ERROR(model_name, ret);

	return ret;
}
