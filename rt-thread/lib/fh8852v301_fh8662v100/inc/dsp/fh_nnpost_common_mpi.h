#ifndef __FH_NNPOST_COMMON_MPI_H__
#define __FH_NNPOST_COMMON_MPI_H__
#include "types/type_def.h"

typedef enum
{
	FN_POST_ROT_0   = 0, // 旋转0度 | [ ]
	FN_POST_ROT_90  = 1, // 旋转90度 | [ ]
	FN_POST_ROT_180 = 2, // 旋转180度 | [ ]
	FN_POST_ROT_270 = 3  // 旋转270度 | [ ]
}FH_NNA_ROT_POST;

typedef struct
{
	FH_PHYADDR          base;        // 物理地址 | [ ]
	FH_VOID *           vbase;       // 虚拟地址 | [ ]
	FH_UINT32           size;        // 内存大小 | [ ]
}FH_MEM_INFO_POST;

typedef struct{
    FH_MEM_INFO_POST hw_output_mem;        // 硬件输出结果的地址 | [ ]
    FH_UINT32 current_rotate;         // 当前帧的rotate参数 | [ ]
    FH_UINT64 current_stamp;          // 当前帧的时间戳信息 | [ ]
    FH_UINT64 frame_id;               // 当前帧帧号 | [ ]
}FH_NN_POST_INPUT_INFO;

typedef struct{
    FH_FLOAT config_th;               // 检测阈值 | [ ]
    FH_NNA_ROT_POST rotate;           // 初始化的旋转角度 | [ ]
    FH_MEM_INFO_POST pp_conf_in;           // 后处理初始化配置的保存地址 | [ ]
}NnPostInitParam;


typedef struct
{
	FH_FLOAT x;     //矩形框x坐标 | [0.0 - 1.0]
	FH_FLOAT y;     //矩形框y坐标 | [0.0 - 1.0]
	FH_FLOAT w;     //矩形框宽度 | [0.0 - 1.0]
	FH_FLOAT h;     //矩形框高度 | [0.0 - 1.0]
}FH_RECT_T;
#endif
