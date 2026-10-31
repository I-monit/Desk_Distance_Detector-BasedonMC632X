#ifndef __FBV_INIT_NNA_H__
#define __FBV_INIT_NNA_H__



#include "dsp/fh_nna_mpi.h"
#include "dsp/fh_nna_mpipara.h"
#include "dsp/fh_nnprimary_det_post_mpi.h"

#if defined ENABLE_Face_Recognition
#include "dsp/fha_face_align_mpi.h"
#include "dsp/fh_nnfacerec_post_mpi.h"
#include "dsp/fha_face_rec_mpi.h"
#include "dsp/fh_nnfacedet_full_post_mpi.h"
#include "dsp/fha_face_quality_mpi.h"
#endif

#define BOX_MAX_NUM 50
#define FH_FACE_REC_MODE 1
#define FH_FACE_CAPTURE_TH 300

#define FH_FACE_ROLL_TH 45
#define FH_FACE_YAW_TH 45
#define FH_FACE_PITCH_TH 45

extern unsigned int nn_code;
extern unsigned int nn_code_end;

#define NN_RESULT_NUM 200
#define NN_WIDTH 512
#define NN_HEIGHT 288
#define FH_REC_EMBEDDING_LEN 128
#define LANDMARK_PTS_NUM 5

#define NN_DEBUG 0
#define FH_SDK_SUCCESS 0
#define FH_SDK_FAILED -1

typedef struct
{
    FH_VOID *modelHandle;
    FH_VOID *postHandle;
    FH_VOID *tskHandle;
} NN_HANDLE_t;

typedef struct
{
    FH_FLOAT x; /* 矩形框x坐标 | [0.0 - 1.0] */
    FH_FLOAT y; /* 矩形框y坐标 | [0.0 - 1.0] */
    FH_FLOAT w; /* 矩形框宽度　| [0.0 - 1.0] */
    FH_FLOAT h; /* 矩形框高度　| [0.0 - 1.0] */
} NN_RECT;

typedef struct
{
    FH_FLOAT x;
    FH_FLOAT y;
} NN_POINT;

typedef struct nn_result_face_snap_info
{
    FH_UINT16 class_id; /*类别索引 | [ ]*/
    NN_RECT box;        /*矩形信息 | [ ]*/
    FH_FLOAT conf;      /*置信度 | [0.0 - 1.0]*/
    NN_POINT landmark[LANDMARK_PTS_NUM];
    FH_FLOAT occlude[LANDMARK_PTS_NUM];
    FH_FLOAT quality;
} NN_RESULT_FACE_SNAP_INFO;

typedef struct nn_result_face_snap
{
    FH_SINT32 nn_result_num;
    NN_RESULT_FACE_SNAP_INFO nn_result_info[NN_RESULT_NUM];
} NN_RESULT_FACE_SNAP;

typedef struct
{
    FH_UINT64 frame_id;   /*矩形信息对应的帧序 | [ ]*/
    FH_UINT64 time_stamp; /*矩形信息对应帧的时间戳 | [ ]*/

    FH_FLOAT scale;
    FH_SINT32 zero_point;
    FH_UINT8 val[FH_REC_EMBEDDING_LEN];
    FH_UINT32 length;
} NN_RESULT_FACE_REC;

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

typedef struct nn_result
{
    union
    {
        NN_RESULT_DET result_det;             /*检测结果 | [ ]*/
        NN_RESULT_FACE_SNAP result_face_snap; /*抓拍结果 | [ ]*/
        NN_RESULT_FACE_REC result_face_rec;   /*抓拍结果 | [ ]*/
    };
} NN_RESULT;

typedef enum
{
    IMAGE_TYPE_RGB888 = (1 << 1), /*输入图像的颜色格式 | [ ]*/
    IMAGE_TYPE_RRGGBB = (1 << 2)
} NN_IMAGE_TYPE;

typedef struct nn_input_data
{
    MEM_DESC data;           /*输入图像的地址信息 | [ ]*/
    FH_UINT32 width;         /*输入图像数据的宽 | [ ]*/
    FH_UINT32 height;        /*输入图像数据的高 | [ ]*/
    FH_UINT32 stride;        /*输入图像的stride | [ ]*/
    NN_IMAGE_TYPE imageType; /*输入图像的像素格式 | [RGB888]*/
    FH_UINT64 frame_id;      /*帧序 | [ ]*/
    FH_UINT32 pool_id;       /*vb pool id(非VB模式建议配置为-1UL) | []*/
    FH_UINT64 timestamp;     /*输入图像时间戳 | [ ]*/
} NN_INPUT_DATA;

typedef FH_VOID *NN_HANDLE;

FH_SINT32 fbv_nna_release(FH_VOID);
FH_SINT32 fbv_nna_process(FH_VIDEO_FRAME *nn_src, NN_RESULT_DET *nn_result);

FH_SINT32 fbv_get_face_snap_result(NN_HANDLE nn_handle, NN_INPUT_DATA nn_input_data, NN_RESULT *nn_result);
FH_SINT32 fbv_get_face_rec_result(NN_HANDLE nn_handle, NN_INPUT_DATA nn_input_data, NN_RESULT *nn_result);
FH_SINT32 fbvgetVPUStreamToNN(FH_VIDEO_FRAME chnStr, NN_INPUT_DATA *nn_input_data);
#endif /* __FBV_INIT_NNA_H__ */
