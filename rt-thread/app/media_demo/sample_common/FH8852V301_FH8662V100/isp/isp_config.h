#ifndef __ISP_CONFIG_H__
#define __ISP_CONFIG_H__
#include "sample_common_media.h"
#include "sample_common_sensor.h"
#include "isp/isp_api.h"
#include "vicap/fh_vicap_mpi.h"
#include "dsp/fh_system_mpi.h"
#include "vpu_config.h"
#include "sensor_config.h"

typedef struct dev_isp_info
{
    FH_SINT32 enable;
    FH_SINT32 channel;
    FH_SINT32 isp_format;
    FH_SINT32 isp_init_width;
    FH_SINT32 isp_init_height;
    FH_SINT32 isp_max_width;
    FH_SINT32 isp_max_height;
    FH_CHAR sensor_name[50];
    struct isp_sensor_if *sensor;
    FH_BOOL bStop;
    FH_BOOL running;
    FH_BOOL pause;
    FH_BOOL resume;
    FH_SINT8 lut2dWorkMode;
    FH_SINT8 init_status;
} ISP_INFO;

ISP_INFO *get_isp_config(FH_SINT32 grp_id);

FH_UINT32 get_isp_max_w(FH_SINT32 grp_id);

FH_UINT32 get_isp_max_h(FH_SINT32 grp_id);

FH_UINT32 get_isp_w(FH_SINT32 grp_id);

FH_UINT32 get_isp_h(FH_SINT32 grp_id);

FH_SINT32 isp_init(FH_SINT32 grp_id);

FH_SINT32 isp_exit(FH_SINT32 grp_id);

FH_SINT32 isp_strategy_run(FH_SINT32 grp_id);

FH_VOID isp_strategy_pause(FH_SINT32 grp_id);

FH_VOID isp_strategy_resume(FH_SINT32 grp_id);

FH_SINT32 get_isp_fps(FH_SINT32 grp_id, FH_UINT32 *isp_fps);

FH_SINT32 change_isp_format(FH_SINT32 grp_id, FH_SINT32 isp_format_bak);

FH_SINT32 set_isp_mirror_flip(FH_SINT32 grp_id, FH_UINT32 mirror, FH_UINT32 flip);

FH_SINT32 set_isp_ae_mode(FH_SINT32 grp_id, FH_UINT32 mode, FH_UINT32 intt_us, FH_UINT32 gain_level);

FH_SINT32 set_isp_sharpeness(FH_SINT32 grp_id, FH_UINT32 mode, FH_UINT32 value);

FH_SINT32 set_isp_saturation(FH_SINT32 grp_id, FH_UINT32 mode, FH_UINT32 value);

FH_SINT32 set_isp_sleep(FH_SINT32 grp_id, FH_SINT32 sleep);

#endif // __ISP_CONFIG_H__
