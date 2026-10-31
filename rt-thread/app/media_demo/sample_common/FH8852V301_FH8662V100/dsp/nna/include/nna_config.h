#ifndef __NNA_CONFIG_H__
#define __NNA_CONFIG_H__
#include "sample_common_media.h"
#include "dsp/fh_nna_mpi.h"
#include "fh_nnprimary_det_post_mpi.h"

#define NN_RESULT_NUM 200
#define NN_MAX_CHN 8

#define NN_DEBUG 0

typedef FH_VOID *NN_HANDLE;

typedef enum
{
    PERSON_DET = 0,          // 人型检测 | [ ]
    PERSON_VEHICLE_DET,      // 人车检测 | [ ]
    PERSON_VEHICLE_FIRE_DET, // 人车火检测 | [ ]
    PERSON_KPT_DET,          // 人型5点关键点检测 | [ ]
    FACE_DET,                // 人脸检测 | [ ]
    FACE_KPT_DET,            // 人脸5点关键点检测 | [ ]
    FACE_68_KPT_DET,         // 人脸68点关键点检测 | [ ]
    FACE_REC,                // 人脸识别 | [ ]
    FACE_ATTR,               // 人脸属性 | [ ]
    LP_DET,                  // 车牌检测 | [ ]
    LP_REC,                  // 车牌识别 | [ ]
    FIRE_DET,                // 火焰检测 | [ ]
    PET_DET,                 // 宠物检测 | [ ]
    LIVE_DET,                // 活体检测 | [ ]
    FIVE_IN_ONE,             // 检测5合一 | [ ]
    MODEL_MAX,               // max
} NN_TYPE;

typedef struct
{
    FH_FLOAT x; // 矩形框x坐标 | [0.0 - 1.0]
    FH_FLOAT y; // 矩形框y坐标 | [0.0 - 1.0]
    FH_FLOAT w; // 矩形框宽度　| [0.0 - 1.0]
    FH_FLOAT h; // 矩形框高度　| [0.0 - 1.0]
} NN_RECT;

typedef struct
{
    FH_FLOAT x;
    FH_FLOAT y;
} NN_POINT;

typedef struct nn_result_info
{
    FH_UINT32 class_id; // 类别索引　| [ ]
    NN_RECT box;        // 矩形信息 w,h为0时，则为一个点 | [ ]
    FH_FLOAT conf;      // 置信度　　| [0.0 - 1.0]
} NN_RESULT_DET_INFO;

typedef struct nn_result_det
{
    FH_SINT32 nn_result_num;
    NN_RESULT_DET_INFO nn_result_info[NN_RESULT_NUM];
} NN_RESULT_DET;

typedef struct nn_result
{
    NN_TYPE net_type;
    union
    {
        NN_RESULT_DET result_det; // 检测结果 | [ ]
    };
} NN_RESULT;

typedef enum
{
    IMAGE_TYPE_RGB888 = (1 << 1), // 输入图像的颜色格式 | [ ]
    IMAGE_TYPE_RRGGBB = (1 << 2)
} NN_IMAGE_TYPE;

typedef struct nn_input_data
{
    MEM_DESC data;           // 输入图像的地址信息 | [ ]
    FH_UINT32 width;         // 输入图像数据的宽 | [ ]
    FH_UINT32 height;        // 输入图像数据的高 | [ ]
    FH_UINT32 stride;        // 输入图像的stride | [ ]
    NN_IMAGE_TYPE imageType; // 输入图像的像素格式 | [RGB888]
    FH_UINT64 frame_id;      // 帧序 | [ ]
    FH_UINT32 pool_id;       // vb pool id(非VB模式建议配置为-1UL) | []
    FH_UINT64 timestamp;     // 输入图像时间戳 | [ ]
} NN_INPUT_DATA;

typedef struct fh_model_info
{
    FH_VOID *mdlHdl;
    FH_VOID *postHdl;
    FH_CHAR *mdl_path;
    FH_UINT32 mdl_width;
    FH_UINT32 mdl_height;
    NN_TYPE type;
    FH_SINT32 task_count;
} FH_MODEL_INFO;

typedef struct
{
    FH_SINT32 id;
    FH_MODEL_INFO *model_info;
    FH_VOID *tskHdl;
} NN_HANDLE_t;

FH_SINT32 nna_init(FH_SINT32 nn_id, NN_HANDLE *nn_handle, NN_TYPE nn_type, FH_CHAR *model_path, FH_SINT32 model_width, FH_SINT32 model_height);

FH_SINT32 nna_release(FH_SINT32 nn_id, NN_HANDLE *nn_handle);

FH_SINT32 nna_get_obj_det_result(NN_HANDLE nn_handle, NN_INPUT_DATA nn_input_data, NN_RESULT *nn_result);

#endif // __NNA_CONFIG_H__
