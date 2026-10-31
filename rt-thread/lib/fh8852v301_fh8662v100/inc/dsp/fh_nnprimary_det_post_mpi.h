#ifndef __FH_BASE_DET_POST_MPI_H__
#define __FH_BASE_DET_POST_MPI_H__
#include "fh_nnpost_common_mpi.h"
/**
 * 本后处理库可以支持的模型ID
*/
enum {
	FN_DET_TYPE_PERSON = 1,    //人形检测 | [ ]
	FN_DET_TYPE_FACE = 2,      //人脸检测 | [ ]
	FN_DET_TYPE_C2 = 3,        //人车检测 | [ ]
};

/**
 * 后处理相关结构体定义
 */
#define FH_MAX_BBOX_NUM 150


typedef struct
{
	FH_UINT16 clsType;   //类别索引 | [ ]
	FH_RECT_T bbox;      //矩形信息 | [ ]
	FH_FLOAT conf;       //置信度 | [0.0 - 1.0]
}FH_BASE_DET_BBOX_T;

typedef struct
{
	FH_UINT64 frame_id;                     //矩形信息对应的帧序 | [ ]
	FH_UINT64 time_stamp;                   //矩形信息对应帧的时间戳 | [ ]
	FH_UINT32 boxNum;                       //矩形个数 | [ ]
	FH_BASE_DET_BBOX_T detBBox[FH_MAX_BBOX_NUM]; //具体矩形信息 | [ ]
}FH_DETECTION_T;

typedef struct
{
	FH_UINT32 cls_id;   ///检测类型
	FH_FLOAT conf_thr; //网络检测阈值 | [0.0 - 1.0]
}FH_SETPARAM_T;

/*
*   Name: FH_NNPost_PrimaryDet_Create
*            PrimaryDet 后处理初始化函数
*
*   Parameters:
*
*       [out] FH_VOID** postHandle
*            初始化后处理后返回的后处理句柄，供后续其他函数调用
*
*       [in] NnPostInitParam* post_init_param
*            初始化后处理需要的参数
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*   Note:
*          添加说明
*
*/
FH_SINT32 FH_NNPost_PrimaryDet_Create(FH_VOID** postHandle, NnPostInitParam* post_init_param);

/*
*   Name: FH_NNPost_PrimaryDet_Create_Lite
*            PrimaryDet 后处理初始化函数（快启模式）
*
*   Parameters:
*
*
*       [out] FH_VOID** postHandle
*            初始化后处理后返回的后处理句柄，供后续其他函数调用
*
*       [in] NnPostInitParam* post_init_param
*            初始化后处理需要的参数
*       [in] FH_MEM_INFO_POST*  lite_conf_in
*            快启初始化后处理需要的bin文件
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*   Note:
*          添加说明
*
*/
FH_SINT32 FH_NNPost_PrimaryDet_Create_Lite(FH_VOID** postHandle, NnPostInitParam* post_init_param, FH_MEM_INFO_POST*  lite_conf_in);
/*
*   Name: FH_NNPost_PrimaryDet_Destroy
*            PrimaryDet 后处理销毁函数
*
*   Parameters:
*
*       [in] FH_VOID* postHandle
*            已创建好的后处理句柄
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*   Note:
*          添加说明
*
*/
FH_SINT32 FH_NNPost_PrimaryDet_Destroy(FH_VOID* postHandle);
/*
*   Name: FH_NNPost_PrimaryDet_Process
*            PrimaryDet 后处理处理函数
*
*   Parameters:
*
*       [in] FH_VOID* postHandle
*            已创建好的后处理句柄
*
*       [in] FH_NN_POST_INPUT_INFO* input_info
*            后处理一帧输入信息,包括硬件地址,时间戳,帧号等
*
*       [out] FH_DETECTION_T* out
*            后处理的输出信息,包括检测数量,矩形框等
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*   Note:
*          添加说明
*
*/
FH_SINT32 FH_NNPost_PrimaryDet_Process(FH_VOID* postHandle, FH_NN_POST_INPUT_INFO* input_info, FH_DETECTION_T* out);

/*
*   Name: FH_NNPost_PrimaryDet_Version
*            获取后处理库版本信息
*
*   Parameters:
*
*       [in] FH_UINT32 print_enable
*            是否打印版本信息
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*
*/
FH_CHAR *FH_NNPost_PrimaryDet_Version(FH_UINT32 print_enable);

/*
*   Name: FH_NNPost_PrimaryDet_SetParam
*            实时配置后处理参数,目前允许配置的参数为,按检测类型配置网络检测阈值
*
*   Parameters:
*
*       [in] FH_VOID* postHandle
*            已创建好的后处理句柄
*
*       [in] FH_SETPARAM_T *setParams
*            后处理参数
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*
*/
FH_SINT32 FH_NNPost_PrimaryDet_SetParam(FH_VOID *postHandle, FH_SETPARAM_T *setParams);
#endif
