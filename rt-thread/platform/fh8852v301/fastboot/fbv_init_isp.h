#ifndef __FHV_ISP_H__
#define __FHV_ISP_H__

typedef struct dev_isp_info
{
    FH_SINT32 enable;
    FH_SINT32 grpid;
    FH_SINT32 isp_init_width_pre;
    FH_SINT32 isp_init_height_pre;
    FH_SINT32 isp_init_width;
    FH_SINT32 isp_init_height;
    FH_SINT32 isp_format_pre;
    FH_SINT32 isp_format;
    FH_CHAR sensor_name[50];
    struct isp_sensor_if *sensor;
    FH_SINT8 lut2dWorkMode;
} ISP_INFO;

#ifdef FH_USING_PS5270_MIPI_G0
#define ISP_SENSOR_NAME_G0 "ps5270_mipi"
#define ISP_SENSOR_NAME_G1 "ps5270_mipi"
#endif

#ifdef FH_USING_OVOS02K_MIPI_G0
#define ISP_SENSOR_NAME_G0 "ovos02k_mipi"
#define ISP_SENSOR_NAME_G1 "ovos02k_mipi"
#endif

#ifdef FH_USING_SC3336_MIPI_G0
#define ISP_SENSOR_NAME_G0 "sc3336_mipi"
#define ISP_SENSOR_NAME_G1 "sc3336_mipi"
#endif

#ifdef FH_USING_OVOS04C10_MIPI_G0
#define ISP_SENSOR_NAME_G0 "ovos04c10_mipi"
#define ISP_SENSOR_NAME_G1 "ovos04c10_mipi"
#endif

FH_SINT32 fbv_isp_init(FH_VOID);
FH_VOID fbv_isp_start_proc(FH_VOID);
FH_SINT32 fbv_isp_pause(FH_SINT32 grp_id);
FH_SINT32 fbv_isp_resume(FH_SINT32 grp_id);
FH_SINT32 fbv_isp_w(FH_SINT32 grp_id);
FH_SINT32 fbv_isp_w_pre(FH_SINT32 grp_id);
FH_SINT32 fbv_isp_h(FH_SINT32 grp_id);
FH_SINT32 fbv_isp_h_pre(FH_SINT32 grp_id);
FH_SINT32 isp_change_resolution(FH_SINT32 grp_id);
#endif /*__FHV_ISP_H__*/
