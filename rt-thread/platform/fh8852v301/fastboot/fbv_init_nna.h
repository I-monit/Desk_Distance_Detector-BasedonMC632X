#ifndef __FBV_INIT_NNA_H__
#define __FBV_INIT_NNA_H__

#include "dsp/fh_nna_mpi.h"
#include "dsp/fh_nna_mpipara.h"
#include "dsp/fh_nnprimary_det_post_mpi.h"

extern unsigned int nn_code;
extern unsigned int nn_code_end;

#define NN_RESULT_NUM 200
#define NN_WIDTH 512
#define NN_HEIGHT 288

typedef struct
{
    FH_VOID *modelHandle;
    FH_VOID *postHandle;
    FH_VOID *tskHandle;
} NN_HANDLE_t;

typedef struct
{
    FH_UINT32 x; /* 矩形框x坐标 | [0.0 - 1.0] */
    FH_UINT32 y; /* 矩形框y坐标 | [0.0 - 1.0] */
    FH_UINT32 w; /* 矩形框宽度　| [0.0 - 1.0] */
    FH_UINT32 h; /* 矩形框高度　| [0.0 - 1.0] */
} NN_RECT;

typedef struct nn_result_info
{
    FH_UINT32 class_id; /* 类别索引　| [ ] */
    NN_RECT box;        /* 矩形信息 w,h为0时，则为一个点 | [ ] */
    FH_UINT32 conf;     /* 置信度　| [0.0 - 1.0] */
} NN_RESULT_DET_INFO;

typedef struct nn_result_det
{
    FH_SINT32 nn_result_num;
    NN_RESULT_DET_INFO nn_result_info[NN_RESULT_NUM];
} NN_RESULT_DET;

typedef FH_VOID *NN_HANDLE;

FH_SINT32 fbv_nna_release(FH_VOID);
FH_SINT32 fbv_nna_process(FH_VIDEO_FRAME *nn_src, NN_RESULT_DET *nn_result);
#endif /* __FBV_INIT_NNA_H__ */
