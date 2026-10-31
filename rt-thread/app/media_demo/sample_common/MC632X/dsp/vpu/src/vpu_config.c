#include "vpu_config.h"

static char *model_name = MODEL_TAG_VPU_CONFIG;

static VPU_INFO g_vpu_grp_infos[MAX_GRP_NUM] = {

	{
		.enable = 0,
#if defined(VPU_MODE_MEM_G0) || defined(VPU_MODE_ISP_G0)
		.enable = 1,
		.channel = 0,
#ifdef YCMEAN_EN_G0
		.grp_info.ycmean_ds = YCMEAN_DS_G0,
		.grp_info.ycmean_en = 1,
#endif
		.mode = VPU_INMODE_TYPE_G0,
#endif /* VIDEO_GRP0 */
	},

	{
		.enable = 0,
#if defined(VPU_MODE_MEM_G1) || defined(VPU_MODE_ISP_G1)
		.enable = 1,
		.channel = 0,
#ifdef YCMEAN_EN_G1
		.grp_info.ycmean_ds = YCMEAN_DS_G1,
		.grp_info.ycmean_en = 1,
#endif
		.mode = VPU_INMODE_TYPE_G1,
#endif /* VIDEO_GRP0 */
	},
};

VPU_MEM_IN g_mem_in[MAX_GRP_NUM] = {
	{
		.enable = 0,
		.bStop = 0,
		.bStart = 0,
#if defined(VPU_MODE_MEM_G0) || defined(VPU_MODE_OFFLINE_2DLUT_DDR_G0)
		.enable = 1,
		.u32Grp = 0,
		.SendFrameInfo.u32Width = MEM_IN_WIDTH_G0,
		.SendFrameInfo.u32Height = MEM_IN_HEIGHT_G0,
		.SendFrameInfo.u32StrideY = MEM_IN_WIDTH_G0,
		.SendFrameInfo.u32StrideUV = MEM_IN_HEIGHT_G0,
		.data_format = MEM_IN_TYPE_G0,
		.time_stamp = MEM_IN_TIMESTAMP_G0,
#ifdef MEM_IN_YUVFILE_G0
		.SendFrameInfo.yuvfile = MEM_IN_YUVFILE_G0,
		.SendFrameInfo.yfile = NULL,
		.SendFrameInfo.uvfile = NULL,
#else
		.SendFrameInfo.yuvfile = NULL,
		.SendFrameInfo.yfile = MEM_IN_YFILE_G0,
		.SendFrameInfo.uvfile = MEM_IN_UVFILE_G0,
#endif
#endif
	},

	{
		.enable = 0,
		.bStop = 0,
		.bStart = 0,
#if defined(VPU_MODE_MEM_G1) || defined(VPU_MODE_OFFLINE_2DLUT_DDR_G1)
		.enable = 1,
		.u32Grp = 1,
		.SendFrameInfo.u32Width = MEM_IN_WIDTH_G1,
		.SendFrameInfo.u32Height = MEM_IN_HEIGHT_G1,
		.SendFrameInfo.u32StrideY = MEM_IN_WIDTH_G1,
		.data_format = MEM_IN_TYPE_G1,
		.time_stamp = MEM_IN_TIMESTAMP_G1,
#ifdef MEM_IN_YUVFILE_G1
		.SendFrameInfo.yuvfile = MEM_IN_YUVFILE_G1,
		.SendFrameInfo.yfile = NULL,
		.SendFrameInfo.uvfile = NULL,
#else
		.SendFrameInfo.yuvfile = NULL,
		.SendFrameInfo.yfile = MEM_IN_YFILE_G1,
		.SendFrameInfo.uvfile = MEM_IN_UVFILE_G1,
#endif
#endif
	},
};

VPU_INFO *get_vpu_config(FH_SINT32 grp_id)
{
	return &g_vpu_grp_infos[grp_id];
}

VPU_MEM_IN *get_vpu_mem_in_config(FH_SINT32 grp_id)
{
	return &g_mem_in[grp_id];
}

FH_UINT32 get_vpu_max_vi_w(FH_SINT32 grp_id)
{
	FH_UINT32 vi_w = 0;

	vi_w = get_isp_max_w(grp_id);

	return vi_w;
}

FH_UINT32 get_vpu_max_vi_h(FH_SINT32 grp_id)
{
	FH_UINT32 vi_h = 0;

	vi_h = get_isp_max_h(grp_id);

	return vi_h;
}

FH_UINT32 get_vpu_vi_w(FH_SINT32 grp_id)
{
	FH_UINT32 vi_w = 0;

	vi_w = get_isp_w(grp_id);

	return vi_w;
}

FH_UINT32 get_vpu_vi_h(FH_SINT32 grp_id)
{
	FH_UINT32 vi_h = 0;

	vi_h = get_isp_h(grp_id);

	return vi_h;
}

FH_UINT32 get_vpu_vi_info(FH_SINT32 grp_id, FH_SINT32 *vi_w, FH_SINT32 *vi_h)
{
	FH_SINT32 ret = FH_SDK_SUCCESS;
	FH_VPU_SIZE pstViconfig;

	ret = FH_VPSS_GetViAttr(grp_id, &pstViconfig);
	SDK_FUNC_ERROR(model_name, ret);

	if (pstViconfig.crop_area.crop_en)
	{
		*vi_w = pstViconfig.crop_area.vpu_crop_area.u32Width;
		*vi_h = pstViconfig.crop_area.vpu_crop_area.u32Height;
	}
	else
	{
		*vi_w = pstViconfig.vi_size.u32Width;
		*vi_h = pstViconfig.vi_size.u32Height;
	}

	return ret;
}

FH_SINT32 vpu_init(FH_SINT32 grp_id)
{
	FH_SINT32 ret = FH_SDK_SUCCESS;
	FH_VPU_SIZE vi_pic;

	g_vpu_grp_infos[grp_id].grp_info.vi_max_size.u32Width = get_vpu_max_vi_w(grp_id);
	g_vpu_grp_infos[grp_id].grp_info.vi_max_size.u32Height = get_vpu_max_vi_h(grp_id);

	ret = FH_VPSS_CreateGrp(grp_id, &g_vpu_grp_infos[grp_id].grp_info);
	SDK_FUNC_ERROR(model_name, ret);

	vi_pic.vi_size.u32Width = get_vpu_vi_w(grp_id);
	vi_pic.vi_size.u32Height = get_vpu_vi_h(grp_id);
	vi_pic.crop_area.crop_en = 0;
	vi_pic.crop_area.vpu_crop_area.u32X = 0;
	vi_pic.crop_area.vpu_crop_area.u32Y = 0;
	vi_pic.crop_area.vpu_crop_area.u32Width = get_vpu_vi_w(grp_id);
	vi_pic.crop_area.vpu_crop_area.u32Height = get_vpu_vi_h(grp_id);

	ret = FH_VPSS_SetViAttr(grp_id, &vi_pic);
	SDK_FUNC_ERROR(model_name, ret);

	ret = FH_VPSS_Enable(grp_id, g_vpu_grp_infos[grp_id].mode);
	SDK_FUNC_ERROR(model_name, ret);

	return ret;
}

FH_SINT32 vpu_grp_disable(FH_SINT32 grp_id)
{
	FH_SINT32 ret = FH_SDK_SUCCESS;

	/* 去使能vpu */
	ret = FH_VPSS_Disable(grp_id);
	SDK_FUNC_ERROR(model_name, ret);

	return ret;
}

FH_SINT32 vpu_grp_enable(FH_SINT32 grp_id)
{
	FH_SINT32 ret = FH_SDK_SUCCESS;

	/* 使能vpu */
	ret = FH_VPSS_Enable(grp_id, g_vpu_grp_infos[grp_id].mode); /*ISP直通模式,可以节省内存开销*/
	SDK_FUNC_ERROR(model_name, ret);

	return ret;
}

FH_SINT32 vpu_grp_freeze(FH_SINT32 grp_id, FH_SINT32 freeze)
{
	FH_SINT32 ret = FH_SDK_SUCCESS;

	if (freeze)
	{
		ret = FH_VPSS_FreezeVideo(grp_id);
		SDK_FUNC_ERROR(model_name, ret);
	}

	else
	{
		ret = FH_VPSS_UnfreezeVideo(grp_id);
		SDK_FUNC_ERROR(model_name, ret);
	}

	return ret;
}

FH_SINT32 vpu_grp_set_vi_attr(FH_SINT32 grp_id, FH_SINT32 w, FH_SINT32 h)
{
	FH_SINT32 ret = FH_SDK_SUCCESS;
	FH_VPU_SIZE vi_pic;

	ret = FH_VPSS_GetViAttr(grp_id, &vi_pic);
	SDK_FUNC_ERROR(model_name, ret);

	vi_pic.vi_size.u32Width = w;
	vi_pic.vi_size.u32Height = h;

	ret = FH_VPSS_SetViAttr(grp_id, &vi_pic);
	SDK_FUNC_ERROR(model_name, ret);

	return ret;
}

FH_SINT32 sendStreamToVpu(VPU_MEM_IN *vpu_mem_in, FH_PHYADDR ydata, FH_PHYADDR uvdata)
{
	FH_SINT32 ret = FH_SDK_SUCCESS;
	SDK_FUNC_ERROR(model_name, ret);
	return ret; // not support
}

FH_SINT32 sendStreamToVpu_Adv(VPU_MEM_IN *vpu_mem_in, FH_PHYADDR ydata, FH_PHYADDR uvdata, FH_UINT32 blkidx)
{
	FH_SINT32 ret = FH_SDK_SUCCESS;
	SDK_FUNC_ERROR(model_name, ret);
	return ret; // not support
}

FH_SINT32 getYuvSizeFromeYuvType(VPU_MEM_IN *vpu_mem_in, FH_UINT64 *ySize, FH_UINT64 *uvSize)
{
	FH_SINT32 ret = FH_SDK_SUCCESS;

	switch (vpu_mem_in->data_format)
	{
	case VPU_VOMODE_NV16:
		*ySize = vpu_mem_in->SendFrameInfo.u32Width * vpu_mem_in->SendFrameInfo.u32Height;
		*uvSize = vpu_mem_in->SendFrameInfo.u32Width * vpu_mem_in->SendFrameInfo.u32Height;
		break;
	case VPU_VOMODE_SCAN:
		*ySize = vpu_mem_in->SendFrameInfo.u32Width * vpu_mem_in->SendFrameInfo.u32Height;
		*uvSize = vpu_mem_in->SendFrameInfo.u32Width * vpu_mem_in->SendFrameInfo.u32Height / 2;
		break;
	default:
		ret = -1;
		SDK_PRT(model_name, "This Yuv Type Not support!\n");
		break;
	}

	SDK_FUNC_ERROR(model_name, ret);

	return ret;
}
