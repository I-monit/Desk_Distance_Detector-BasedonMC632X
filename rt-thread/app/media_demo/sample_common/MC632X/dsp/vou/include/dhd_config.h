#ifndef __DHD_CONFIG_H__
#define __DHD_CONFIG_H__
#include "sample_common_media.h"
#include "dsp/fh_vou_mpi.h"
#include "dsp/fh_vou_mpipara.h"

#define MAX_DHD_DEV_NUM 1

#ifdef DHD0_INTF_LCD
#define DHD0_INTF VO_INTF_LCD
#endif

#ifndef DHD0_INTF
#define DHD0_INTF VO_INTF_LCD
#endif

#ifdef DHD0_800X480P60
#define DHD0_INTF_SYNC VO_OUTPUT_800x480_60
#endif

#ifndef DHD0_INTF_SYNC
#define DHD0_INTF_SYNC VO_OUTPUT_800x480_60
#endif

typedef struct dhd_info
{
    FH_SINT32 enable;
    VO_DEV VoDev;
    VO_INTF_SYNC_E enIntfSync;
    VO_INTF_TYPE_E enIntfType;
    FH_UINT32 u32BgColor;
} DHD_INFO;

#endif // __DHD_CONFIG_H__
