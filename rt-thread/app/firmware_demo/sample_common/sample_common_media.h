#ifndef __SAMPLE_COMMON_MEDIA_H__
#define __SAMPLE_COMMON_MEDIA_H__

// System Include
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <signal.h>
#include <fcntl.h>
#include <stddef.h>
#include <sys/prctl.h>
#include <semaphore.h>
#include <sys/stat.h>
#include <dirent.h>
#include <assert.h>

// SDK Include
#include "types/type_def.h"
#include "types/bufCtrl.h"
#include "types/vmm_api.h"

// User Common Include
#include "libdmc/include/libdmc_rtsp.h"
#include "libdmc/include/libdmc.h"
#include "librtsp/include/librtsp.h"
#include "libsample_common/include/libsample_common.h"

// User App Include
#include "config.h"
#include "media_proc.h"

#define MODEL_TAG_PROC "Proc"
#define MODEL_TAG_LIB_UVC "LibUvc"
#define MODEL_TAG_LIBCOMMON "LibCommon"

#define MODEL_TAG_ISP "Isp"
#define MODEL_TAG_ISP_CONFIG "IspConfig"
#define MODEL_TAG_ISP_BIND_INFO "IspBind"
#define MODEL_TAG_MIPI "Mipi"
#define MODEL_TAG_SENSOR_CONFIG "SensorConfig"
#define MODEL_TAG_SENSOR_COMMON "SensorCommon"
#define MODEL_TAG_VICAP "Vicap"

#define MODEL_TAG_DSP "Dsp"
#define MODEL_TAG_2DLUT "2dlut"
#define MODEL_TAG_BGM "Bgm"
#define MODEL_TAG_ENC "Enc"
#define MODEL_TAG_ENC_CONFIG "EncConfig"
#define MODEL_TAG_JPEG "Jpeg"
#define MODEL_TAG_JPEG_CONFIG "JpegConfig"
#define MODEL_TAG_NNA_CONFIG "NnaConfig"
#define MODEL_TAG_NNA_COMMON "NnaCommon"
#define MODEL_TAG_VISE "Vise"
#define MODEL_TAG_VISE_CONFIG "ViseConfig"
#define MODEL_TAG_VISE_BIND_INFO "ViseBind"
#define MODEL_TAG_VGS_CONFIG "VgsConfig"
#define MODEL_TAG_VPU "Vpu"
#define MODEL_TAG_VPU_CHN "VpuChn"
#define MODEL_TAG_VPU_CONFIG "VpuConfig"
#define MODEL_TAG_VPU_BIND_INFO "VpuBind"
#define MODEL_TAG_VOU "Vou"
#define MODEL_TAG_VOU_CONFIG "VouConfig"

#define MODEL_TAG_DEMO "Demo"
#define MODEL_TAG_DEMO_AF "DemoAf"
#define MODEL_TAG_COOLVIEW "CoolView"
#define MODEL_TAG_DEMO_DUAL_CAMERA "DemoDualCamera"
#define MODEL_TAG_DEMO_ISP "DemoIsp"
#define MODEL_TAG_DEMO_MD_CD "DemoMdCd"
#define MODEL_TAG_DEMO_NNA_OBJ_DET "DemoNNAObjDet"
#define MODEL_TAG_DEMO_NNA_AINR "DemoNNAAINR"
#define MODEL_TAG_DEMO_NNA_IVS "DemoNNAIvs"
#define MODEL_TAG_DEMO_NNA_FACE_SNAP "DemoNNAFaceSnap"
#define MODEL_TAG_DEMO_NNA_FACE_REC "DemoNNAFaceRec"
#define MODEL_TAG_DEMO_NNA_GES_REC "DemoNNAGesRec"
#define MODEL_TAG_DEMO_NNA_LPR "DemoNNALpr"
#define MODEL_TAG_DEMO_OVERLAY "DemoOverlay"
#define MODEL_TAG_DEMO_SMART_IR "DemoSmartIR"
#define MODEL_TAG_DEMO_OVERLAY_LOGO "DemoOverlayLogo"
#define MODEL_TAG_DEMO_OVERLAY_TOSD "DemoOverlayTosd"
#define MODEL_TAG_DEMO_OVERLAY_MASK "DemoOverlayMask"
#define MODEL_TAG_DEMO_OVERLAY_GBOX "DemoOverlayGbox"
#define MODEL_TAG_STREAM "Stream"
#define MODEL_TAG_DEMO_IVS "DemoIvs"
#define MODEL_TAG_DEMO_UVC "DemoUvc"
#define MODEL_TAG_UVC_CONFIG "UvcConfig"
#define MODEL_TAG_DEMO_VENC "DemoVenc"
#define MODEL_TAG_DEMO_VISE "DemoVise"

#define FH_SDK_SUCCESS 0
#define FH_SDK_FAILED -1

#define SDK_PRT(model, fmt, ...)                                   \
    do                                                             \
    {                                                              \
        fprintf((FILE *)stdout, "[%s]" fmt, model, ##__VA_ARGS__); \
    } while (0)

#define SDK_PRT_LINE(model)                                                      \
    do                                                                           \
    {                                                                            \
        fprintf((FILE *)stdout, "[%s]:%s[%d]\n", model, __FUNCTION__, __LINE__); \
    } while (0)

#define SDK_ERR_PRT(model, fmt, ...)                                                                            \
    do                                                                                                          \
    {                                                                                                           \
        fprintf((FILE *)stderr, "[%s] Error:%s() line:%d :" fmt, model, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
    } while (0)

#define SDK_FUNC_ERROR(model, ret)                                   \
    do                                                               \
    {                                                                \
        if (ret != 0)                                                \
        {                                                            \
            SDK_ERR_PRT(model, "failed with 0x%x(%d)!\n", ret, ret); \
            return ret;                                              \
        }                                                            \
    } while (0)

#define SDK_FUNC_ERROR_PRT(model, ret)                               \
    do                                                               \
    {                                                                \
        if (ret != 0)                                                \
        {                                                            \
            SDK_ERR_PRT(model, "failed with 0x%x(%d)!\n", ret, ret); \
        }                                                            \
    } while (0)

#define SDK_FUNC_ERROR_CONTINUE(model, ret)                      \
    if (ret != 0)                                                \
    {                                                            \
        SDK_ERR_PRT(model, "failed with 0x%x(%d)!\n", ret, ret); \
        continue;                                                \
    }

#define SDK_FUNC_ERROR_GOTO(model, ret)                              \
    do                                                               \
    {                                                                \
        if (ret != 0)                                                \
        {                                                            \
            SDK_ERR_PRT(model, "failed with 0x%x(%d)!\n", ret, ret); \
            goto Exit;                                               \
        }                                                            \
    } while (0)

#define SDK_CHECK_NULL_PTR(model, ptr)             \
    do                                             \
    {                                              \
        if (NULL == ptr)                           \
        {                                          \
            SDK_ERR_PRT(model, "null pointer!\n"); \
            goto Exit;                             \
        }                                          \
    } while (0)

#define SDK_CHECK_MAX_VALID(model, x, max, Msg, ...) \
    do                                               \
    {                                                \
        if (x > max)                                 \
        {                                            \
            SDK_ERR_PRT(model, Msg, ##__VA_ARGS__);  \
            return FH_SDK_FAILED;                    \
        }                                            \
    } while (0)

#define SDK_CHECK_MIN_VALID(model, x, min, Msg, ...) \
    do                                               \
    {                                                \
        if (x < min)                                 \
        {                                            \
            SDK_ERR_PRT(model, Msg, ##__VA_ARGS__);  \
            return FH_SDK_FAILED;                    \
        }                                            \
    } while (0)

#define SDK_CHECK_ALIGN(model, x, align)                        \
    do                                                          \
    {                                                           \
        if ((x & (align - 1)) != 0)                             \
        {                                                       \
            SDK_ERR_PRT(model, "%d not align %d!\n", x, align); \
            return FH_SDK_FAILED;                               \
        }                                                       \
    } while (0)

#define SDK_FREE_PTR(ptr) \
    do                    \
    {                     \
        if (ptr)          \
        {                 \
            free(ptr);    \
            ptr = NULL;   \
        }                 \
    } while (0)

#define ALIGN_UP(addr, edge) ((addr + edge - 1) & ~(edge - 1)) /* 数据结构对齐定义 */
#define ALIGN_BACK(addr, edge) ((edge) * (((addr) / (edge))))
#define ALIGNTO(addr, edge) ((addr + edge - 1) & ~(edge - 1))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define CLIP(x, min, max) (((x) < (min)) ? (min) : (((x) > (max)) ? (max) : (x)))

#endif // __SAMPLE_COMMON_MEDIA_H__
