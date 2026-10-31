#ifndef __SAMPLE_COMMON_DEMO_H__
#define __SAMPLE_COMMON_DEMO_H__
#include "sample_common_media.h"
#include "overlay/include/sample_common_overlay.h"

#if defined FH_NN_ENABLE
#include "nna_app/include/sample_common_nna.h"
#endif

FH_SINT32 sample_common_start_demo(FH_VOID);

FH_SINT32 sample_common_stop_demo(FH_VOID);

#endif // __SAMPLE_COMMON_DEMO_H__
