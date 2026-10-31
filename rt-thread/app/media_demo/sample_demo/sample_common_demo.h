#ifndef __SAMPLE_COMMON_DEMO_H__
#define __SAMPLE_COMMON_DEMO_H__
#include "sample_common_media.h"
#include "isp_strategy/include/sample_common_isp_demo.h"
#include "motion_and_cover_detect/include/sample_common_md_cd.h"
#include "overlay/include/sample_common_overlay.h"
#include "venc/include/sample_common_venc.h"
#include "vise_demo/include/sample_common_vise_demo.h"

#ifdef __LINUX_OS__
#include "uvc_linux/include/sample_common_uvc.h"
#elif defined __RTTHREAD_OS__
#include "uvc_rtt/include/sample_common_uvc.h"
#endif

#ifdef DUAL_CAMERA_ENABLE
#include "dual_camera/include/sample_common_dual_camera.h"
#endif

#ifdef FH_IVS_ENABLE
#include "trip_and_peri_detect/include/sample_common_ivs.h"
#endif

#ifdef FH_AF_ENABLE
#include "af/include/sample_common_af_demo.h"
#endif

#if defined FH_NN_ENABLE || defined FH_NN_AINR_ENABLE
#include "nna_app/include/sample_common_nna.h"
#endif

FH_SINT32 sample_common_start_demo(FH_VOID);

FH_SINT32 sample_common_stop_demo(FH_VOID);

#endif // __SAMPLE_COMMON_DEMO_H__
