#ifndef __FH_NNA_MPIPARA_H__
#define __FH_NNA_MPIPARA_H__
/**|NNA|**/

#include "types/type_def.h"
#include "fh_common.h"
#define FH_MAX_PATH 256
#define FH_MAX_BBOX_NUM 150
#define FH_MAX_LK_NUM 5
#define FH_MAX_LPD_NUM 4
#define FH_LPR_LLEN 8
#define FH_MAX_EMBEDDING_LEN 128
#define FH_FACEKPT_MAX_NUM 68
#define FH_MAX_DIM_NUM 4
#define FH_MAX_OUTPUT_NUM 4
#define FH_HIGH_DETCT_BASE (0x80000000)

#ifdef __cplusplus
#if __cplusplus
extern "C"
{
#endif
#endif /* __cplusplus */
#pragma pack(4)

typedef struct
{
	FH_UINT32 base;  // 物理地址 | [ ]
	FH_VOID * vbase; // 虚拟地址 | [ ]
	FH_UINT32 size;  // 内存大小 | [ ]
}FH_USR_MEM_INFO;

typedef enum
{
	FN_ROT_0   = 0, // 旋转0度 | [ ]
	FN_ROT_90  = 1, // 旋转90度 | [ ]
	FN_ROT_180 = 2, // 旋转180度 | [ ]
	FN_ROT_270 = 3  // 旋转270度 | [ ]
}FH_NNA_ROT;

typedef struct{
    FH_MEM_INFO hw_output_mem;  //NN硬件输出结果 | [ ]
    FH_UINT32 current_rotate;   //当前帧的rotate参数 | [ ]
    FH_UINT64 current_stamp;    //当前帧的时间戳 | [ ]
    FH_UINT64 frame_id;         //当前帧的帧序 | [ ]
}FH_NN_OUT_INFO;

typedef struct {
    FH_UINT32 size;             //需要的buffer大小 | [ ]
    FH_UINT32 alignByteSize;    //该buffer的对齐要求 | [ ]
}FH_NN_MEM_ALLOC_INFO;

typedef struct{
    FH_NN_MEM_ALLOC_INFO cfg_mem_info;  //cfg_mem的分配需求 | [ ]
    FH_NN_MEM_ALLOC_INFO fm_mem_info;   //fm_mem的分配需求 | [ ]
}FH_NN_MODEL_MEM_ALLOC_INFO;

typedef struct{
    FH_NN_MEM_ALLOC_INFO post_mem_info;  //post_mem的分配需求 | [ ]
}FH_NN_TASK_MEM_ALLOC_INFO;

typedef struct{
    FH_MEM_INFO cfg_mem;          //cfg_mem的buffer信息 | [ ]
    FH_MEM_INFO fm_mem;           //fm_mem的buffer信息 | [ ]
}FH_NN_MODEL_MEM_INFO;

typedef struct{
    FH_MEM_INFO post_mem;         //post_mem的buffer信息 | [ ]
}FH_NN_TASK_MEM_INFO;

typedef struct
{
	FH_MEM_INFO nbg_data_mem;   //nn模型数据mem.非RPC方案只使用vbase,RPC方案下,base为0,将额外进行一次copy操作,base不为0,则使用base,省一次copy操作 | [ ]
	FH_UINT32 type;             //网络检测类型 | [ ]
	FH_UINT32 src_w_in;         //网络检测宽度 | [ ]
	FH_UINT32 src_h_in;         //网络检测高度 | [ ]
	FH_UINT32 src_c_in;         //网络检测深度 | [ ]
	FH_VOID *reserved;          //后续兼容使用 | [ ]
}FH_NN_MODEL_INIT_PARAM_T;

typedef struct
{
	FH_NNA_ROT rotate;          //旋转角度 | [ ]
	FH_VOID *reserved;          //后续兼容使用 | [ ]
}FH_NN_TASK_INIT_PARAM_T;

#pragma pack()
#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif
