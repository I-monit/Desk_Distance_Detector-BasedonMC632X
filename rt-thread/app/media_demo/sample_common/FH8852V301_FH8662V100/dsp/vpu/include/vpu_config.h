#ifndef __VPU_CONFIG_H__
#define __VPU_CONFIG_H__
#include "sample_common_media.h"
#include "isp_config.h"
#include "dsp/fh_vpu_mpi.h"
#include "dsp/fh_vpu_mpipara.h"
#include "vpu_chn_config.h"

//====================GRP0=======================
#ifdef VPU_MODE_ISP_G0
#define VPU_INMODE_TYPE_G0 VPU_MODE_ISP
#endif

#ifdef VPU_MODE_MEM_G0
#define VPU_INMODE_TYPE_G0 VPU_MODE_MEM
#endif

#ifdef MEM_IN_MODE_NV16_G0
#define MEM_IN_TYPE_G0 VPU_VOMODE_NV16
#endif
//====================GRP0=======================

//====================GRP1=======================
#ifdef VPU_MODE_ISP_G1
#define VPU_INMODE_TYPE_G1 VPU_MODE_ISP
#endif

#ifdef VPU_MODE_MEM_G1
#define VPU_INMODE_TYPE_G1 VPU_MODE_MEM
#endif

#ifdef MEM_IN_MODE_NV16_G1
#define MEM_IN_TYPE_G1 VPU_VOMODE_NV16
#endif
//====================GRP1=======================

typedef struct grp_vpu_info
{
    FH_SINT32 channel;
    FH_SINT32 enable;
    FH_VPU_SET_GRP_INFO grp_info;
    FH_VPU_VI_MODE mode;
    FH_UINT32 bgm_enable;
    FH_UINT32 cpy_enable;
    FH_UINT32 sad_enable;
    FH_UINT32 bgm_ds;
} VPU_INFO;

typedef struct vpu_send_frameinfo
{
    FH_UINT32 u32Width;
    FH_UINT32 u32Height;
    FH_UINT32 u32StrideY;
    FH_UINT32 u32StrideUV;
    FH_CHAR *yfile;   // send Stream 文件灌图需要
    FH_CHAR *uvfile;  // send Stream 文件灌图需要
    FH_CHAR *yuvfile; // send Stream 文件灌图需要
} VPU_SEND_FRAMEINFO;

typedef struct vpu_mem_in
{
    FH_BOOL bStop;  // send Stream 文件灌图流程需要
    FH_BOOL bStart; // send Stream 文件灌图流程需要
    FH_BOOL enable; // send Stream 文件灌图流程需要
    FH_UINT32 u32Grp;
    FH_UINT64 time_stamp;       // 时间戳 | [ ]
    FH_UINT64 frame_id;         // 帧号 | [ ]
    FH_VPU_VO_MODE data_format; // DDR输入格式,支持格式参考开发手册 | [ ]
    VPU_SEND_FRAMEINFO SendFrameInfo;
} VPU_MEM_IN;

VPU_INFO *get_vpu_config(FH_SINT32 grp_id);

VPU_MEM_IN *get_vpu_mem_in_config(FH_SINT32 grp_id);

FH_UINT32 get_vpu_max_vi_w(FH_SINT32 grp_id);

FH_UINT32 get_vpu_max_vi_h(FH_SINT32 grp_id);

FH_UINT32 get_vpu_vi_w(FH_SINT32 grp_id);

FH_UINT32 get_vpu_vi_h(FH_SINT32 grp_id);

FH_UINT32 get_vpu_vi_info(FH_SINT32 grp_id, FH_SINT32 *vi_w, FH_SINT32 *vi_h);

FH_SINT32 vpu_mod_init(FH_VOID);

FH_SINT32 vpu_init(FH_SINT32 grp_id);

FH_SINT32 vpu_grp_disable(FH_SINT32 grp_id);

FH_SINT32 vpu_grp_enable(FH_SINT32 grp_id);

FH_SINT32 vpu_grp_freeze(FH_SINT32 grp_id, FH_SINT32 freeze);

FH_SINT32 vpu_grp_set_vi_attr(FH_SINT32 grp_id, FH_SINT32 w, FH_SINT32 h);

FH_SINT32 sendStreamToVpu(VPU_MEM_IN *vpu_mem_in, FH_PHYADDR ydata, FH_PHYADDR uvdata);

FH_SINT32 sendStreamToVpu_Adv(VPU_MEM_IN *vpu_mem_in, FH_PHYADDR ydata, FH_PHYADDR uvdata, FH_UINT32 blkidx);

FH_SINT32 getYuvSizeFromeYuvType(VPU_MEM_IN *vpu_mem_in, FH_UINT64 *ySize, FH_UINT64 *uvSize);
#endif // __VPU_CONFIG_H__
