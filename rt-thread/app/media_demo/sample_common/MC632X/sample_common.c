// #include "sample_common.h"

// static char *model_name = MODEL_TAG_SAMPLE_COMMON;

// FH_SINT32 change_format(FH_SINT32 grp_id, FH_SINT32 isp_format_bak)
// {

//     FH_SINT32 ret = FH_SDK_SUCCESS;
//     FH_BIND_INFO src, dst;

//     if (!g_isp_info[grp_id].enable)
//     {
//         SDK_ERR_PRT(model_name, "change_isp_format Failed, ISP[%d] not enable!\n", grp_id);
//         return FH_SDK_FAILED;
//     }

//     /* 停止isp */
//     ret = API_ISP_Pause(grp_id);
//     SDK_FUNC_ERROR(model_name, ret);

//     dst.obj_id = FH_OBJ_ISP;
//     dst.dev_id = grp_id;
//     dst.chn_id = 0;
//     SDK_FUNC_ERROR(model_name, ret);
//     ret = FH_SYS_UnBindbyDst(dst);

//     ret = vpu_grp_disable(grp_id);
//     SDK_FUNC_ERROR(model_name, ret);

//     ret = API_ISP_SetSensorFmt(grp_id, g_isp_info[grp_id].isp_format);
//     if (ret)
//     {
//         SDK_PRT(model_name, "Setting ISP[%d] back to Format:0x%x! ret = %x\n", grp_id, isp_format_bak, ret);
//         g_isp_info[grp_id].isp_format = isp_format_bak;
//         ret = API_ISP_SetSensorFmt(grp_id, g_isp_info[grp_id].isp_format);
//         SDK_FUNC_ERROR(model_name, ret);
//     }

//     ret = vpu_grp_enable(grp_id);
//     SDK_FUNC_ERROR(model_name, ret);

//     /* 恢复isp */
//     ret = API_ISP_Resume(grp_id);
//     SDK_FUNC_ERROR(model_name, ret);

//     src.obj_id = FH_OBJ_VICAP;
//     src.dev_id = 0;
//     src.chn_id = grp_id;
//     dst.obj_id = FH_OBJ_ISP;
//     dst.dev_id = grp_id;
//     dst.chn_id = 0;
//     ret = FH_SYS_Bind(src, dst);
//     SDK_FUNC_ERROR(model_name, ret);

//     SDK_PRT(model_name, "ISP[%d] Change Format:0x%x!\n", grp_id, g_isp_info[grp_id].isp_format);

//     return ret;
// }