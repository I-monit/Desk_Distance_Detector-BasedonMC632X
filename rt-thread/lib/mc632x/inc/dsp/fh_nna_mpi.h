#ifndef __FH_NNA_MPI_H__
#define __FH_NNA_MPI_H__
/**NNA**/

//#include "fh_common.h"
//#include "media_process.h"
#include "types/type_def.h"
#include "fh_nna_mpipara.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {

#endif
#endif /* __cplusplus */

/*
*   Name: FH_NNA_Model_Init
*            根据输入的网路文件初始化整个网络，并创建文件句柄.
*
*   Parameters:
*
*       [out] FH_VOID **modelHandle
*            初始化网络后返回的文件句柄，供后续其他函数调用．
*
*       [in] FH_NN_MODEL_INIT_PARAM_T *model_param
*        model的初始化参数
*
*       [in] FH_NN_MODEL_MEM_INFO *model_mem_info
*        通常情况下,该指针传入0指针.用户需要自行分配NN内存时,传入分配好的mem指针,对应mem所需大小可调用 FH_NNA_GetModelMemSizeWithInitParam 获取.
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*   Note:
*/
FH_SINT32 FH_NNA_Model_Init(FH_VOID **modelHandle, FH_NN_MODEL_INIT_PARAM_T *model_param, FH_NN_MODEL_MEM_INFO *model_mem_info);

/*
*   Name: FH_NNA_Task_Init
*            根据输入的model句柄例化一个task，并创建task句柄.
*
*   Parameters:
*
*       [out] FH_VOID **taskHandle
*            初始化task后返回的句柄，供后续其他函数调用．
*
*       [in] FH_VOID *modelHandle
*            创建好的model句柄.
*
*       [in] FH_NN_TASK_INIT_PARAM_T *task_param
*        task的初始化参数
*
*       [in] FH_NN_TASK_MEM_INFO *task_mem_info
*        通常情况下,该指针传入0指针.用户需要自行分配NN内存时,传入分配好的mem指针,对应mem所需大小可调用 FH_NNA_GetTaskMemSize 获取.
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*   Note:
*/
FH_SINT32 FH_NNA_Task_Init(FH_VOID **taskHandle, FH_VOID *modelHandle, FH_NN_TASK_INIT_PARAM_T *task_param, FH_NN_TASK_MEM_INFO *task_mem_info);

/*
*   Name: FH_NNA_DetectProcess
*            根据输入的文件句柄，完成网络的前向推理，得到网络检测到的Bbox信息等供应用使用该信息
*
*   Parameters:
*
*       [in] FH_VOID *taskHandle
*            创建的task句柄.
*
*       [in] FH_VIDEO_FRAME *src
*            网络输入图像数据信息，包括图像格式，图像的宽高．以及输入的buffer地址，需要物理地址．
*
*       [out] FH_NN_OUT_INFO *out
*            网络前向推理的硬件输出结果， 以及相应帧的时间戳,帧序等信息．
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*   Note:
*          前向推理内部会检测输入图像的幅面信息是否跟网络自身检测范围匹配．同时会检测输入的图像格式．
* 　　　　 同时需要知道输入的buffer物理地址．
*
*
*/
FH_SINT32 FH_NNA_DetectProcess(FH_VOID *taskHandle, FH_VIDEO_FRAME *src, FH_NN_OUT_INFO *out);

/*
*   Name: FH_NNA_DetectForward
*            根据输入的文件句柄，完成网络的前向推理，得到网络检测到的Bbox信息以及时间戳等供应用使用该信息
*
*   Parameters:
*
*       [in] FH_VOID *taskHandle
*            创建的task句柄.
*
*       [out] FH_NN_OUT_INFO *out
*            网络前向推理的硬件输出结果， 以及相应帧的时间戳,帧序等信息．
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*   Note:
*/
FH_SINT32 FH_NNA_DetectForward(FH_VOID *taskHandle, FH_NN_OUT_INFO *out);

/*
*   Name: FH_NNA_DetectRelease
*            释放硬件输出占用的内存资源,需要在后处理对相关资源的使用结束后再释放
*
*   Parameters:
*
*       [in] FH_VOID *taskHandle
*            创建的task句柄.
*
*       [out] FH_NN_OUT_INFO *out
*            网络前向推理的硬件输出结果， 以及相应帧的时间戳,帧序等信息．
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*   Note:
*/
FH_SINT32 FH_NNA_DetectRelease(FH_VOID *taskHandle, FH_NN_OUT_INFO *out);

/*
*   Name: FH_NNA_ReleaseTask
*            task资源的销毁
*
*   Parameters:
*
*       [in] FH_VOID *taskHandle
*            创建的task句柄.
*
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*   Note:
*          销毁相应的资源(包括网络资源及进程资源).
*
*
*/
FH_SINT32 FH_NNA_ReleaseTask(FH_VOID *taskHandle);

/*
*   Name: FH_NNA_ReleaseModel
*            model资源的销毁
*
*   Parameters:
*
*       [in] FH_VOID *modelHandle
*            创建好的model句柄.
*
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*   Note:
*          销毁相应的资源(包括网络资源及进程资源).
*
*
*/
FH_SINT32 FH_NNA_ReleaseModel(FH_VOID *modelHandle);

/*
*   Name: FH_NNA_GetModelVer
*            获取模型版本号
*
*   Parameters:
*
*       [in] FH_VOID *modelHandle
*            创建好的model句柄.
*
*       [out] FH_UINT32 *netVersion
*            网络模型版本号
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*/
FH_SINT32 FH_NNA_GetModelVer(FH_VOID *modelHandle, FH_UINT32 *netVersion);

/*
*   Name: FH_NNA_GetPpconfig
*            获取模型Ppconfig参数
*
*   Parameters:
*
*       [in] FH_VOID *modelHandle
*            创建好的model句柄.
*
*       [out] FH_MEM_INFO *pp_conf
*            例化好的Ppconfig参数内存
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*/
FH_SINT32 FH_NNA_GetPpconfig(FH_VOID *modelHandle, FH_MEM_INFO *pp_conf);

/*
*   Name: FH_NNA_SetRotate
*            设置NN网络旋转角度
*
*   Parameters:
*
*       [in] FH_VOID *taskHandle
*            创建的task句柄.
*
*       [in] FH_NNA_ROT rotate
*            旋转角度
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*/
FH_SINT32 FH_NNA_SetRotate(FH_VOID *taskHandle, FH_NNA_ROT rotate);

/*
*   Name: FH_NNA_GetModelMemSizeWithInitParam
*            根据model的初始化参数获取模型需要的内存大小.
*
*   Parameters:
*
*       [in] FH_NN_MODEL_INIT_PARAM_T *model_param
*        model的初始化参数
*
*       [out] FH_NN_MODEL_MEM_ALLOC_INFO *model_alloc_info
*        需要的内存大小信息
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*   Note:
*/
FH_SINT32 FH_NNA_GetModelMemSizeWithInitParam(FH_NN_MODEL_INIT_PARAM_T *model_param, FH_NN_MODEL_MEM_ALLOC_INFO *model_alloc_info);

/*
*   Name: FH_NNA_GetModelVerWithInitParam
*            根据model的初始化参数获取模型版本信息.
*
*   Parameters:
*
*       [in] FH_NN_MODEL_INIT_PARAM_T *model_param
*        model的初始化参数
*
*       [out] FH_UINT32 *model_ver
*        模型版本信息
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*   Note:
*/
FH_SINT32 FH_NNA_GetModelVerWithInitParam(FH_NN_MODEL_INIT_PARAM_T *model_param, FH_UINT32 *model_ver);

/*
*   Name: FH_NNA_GetModelMemSize
*            获取例化task需要的内存大小.
*
*   Parameters:
*
*       [in] FH_VOID *modelHandle
*            创建好的model句柄.
*
*       [out] FH_NN_TASK_MEM_ALLOC_INFO *task_alloc_info
*        需要的内存大小信息
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*   Note:
*/
FH_SINT32 FH_NNA_GetTaskMemSize(FH_VOID *modelHandle, FH_NN_TASK_MEM_ALLOC_INFO *task_alloc_info);

/*
*   Name: FH_NNA_GetTaskId
*            获取task id,用于明确绑定时NN的通道号.
*
*   Parameters:
*
*       [in] FH_VOID *taskHandle
*            创建好的task句柄.
*
*       [out] FH_UINT32 *task_id
*        task id信息
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*   Note:
*/
FH_SINT32 FH_NNA_GetTaskId(FH_VOID *taskHandle, FH_UINT32 *task_id);

/*
*   Name: FH_NNA_Suspend
*            aov保存nna硬件现场
*
*   Parameters:
*
*            None
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*   Note:
*/
FH_SINT32 FH_NNA_Suspend(FH_VOID);

/*
*   Name: FH_NNA_Resume
*            aov恢复nna硬件现场
*
*   Parameters:
*
*            None
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*   Note:
*/
FH_SINT32 FH_NNA_Resume(FH_VOID);
#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif
