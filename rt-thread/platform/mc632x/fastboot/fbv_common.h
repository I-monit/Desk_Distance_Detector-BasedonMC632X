#include <rtthread.h>
#include "timer.h"
#include "types/type_def.h"
#include "types/bufCtrl.h"
#include "isp/isp_common.h"
#include "isp/isp_api.h"
#include "isp/isp_enum.h"
#include "vicap/fh_vicap_mpi.h"
#include "dsp/fh_common.h"
#include "dsp/fh_vpu_mpi.h"
#include "dsp/fh_vpu_mpipara.h"
#include "dsp/fh_system_mpi.h"
#include "dsp/fh_venc_mpi.h"
#include "dsp/fh_venc_mpipara.h"
#include "mpp/fh_vb_mpi.h"
#include "mpp/fh_vb_mpipara.h"
#include "platform_def.h"
#include "fastconfig.h"
#include "fbv_enc_config.h"
#include "fbv_init_dsp.h"
#include "fbv_init_isp.h"
#include "fh_time_stat.h"

#define FBV_FUNC_ERROR_PRT(func_name, ret)                   \
    do                                                       \
    {                                                        \
        if (ret)                                             \
        {                                                    \
            rt_kprintf("%s faild ret:%x\n", func_name, ret); \
            return ret;                                      \
        }                                                    \
    } while (0)

#define FBV_FUNC_ERROR_GOTO(func_name, ret)                  \
    do                                                       \
    {                                                        \
        if (ret != 0)                                        \
        {                                                    \
            rt_kprintf("%s faild ret:%x\n", func_name, ret); \
            goto Exit;                                       \
        }                                                    \
    } while (0)

#define FBV_SDK_PRT(fmt, ...)                                          \
    do                                                                 \
    {                                                                  \
        rt_kprintf("[%s:%d]:" fmt, __func__, __LINE__, ##__VA_ARGS__); \
    } while (0)

#define FBV_SDK_CHECK_NULL_PTR(func_name, ptr)           \
    do                                                   \
    {                                                    \
        if (NULL == ptr)                                 \
        {                                                \
            rt_kprintf("%s null pointer!\n", func_name); \
            goto Exit;                                   \
        }                                                \
    } while (0)