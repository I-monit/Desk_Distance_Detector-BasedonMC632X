#ifndef __SAMPLE_COMMON_SENSOR_H__
#define __SAMPLE_COMMON_SENSOR_H__
#include "sample_common_media.h"
#include "isp/isp_sensor_if.h"
#include "isp/isp_api.h"
#include "isp/isp_enum.h"
#include "dsp_ext/FHAdv_Isp_mpi_v3.h"
#include "isp_config.h"

#define SAMPLE_SENSOR_FLAG_NORMAL (0x00)
#define SAMPLE_SENSOR_FLAG_NIGHT (0x02)
#define SAMPLE_SENSOR_FLAG_WDR (0x01)
#define SAMPLE_SENSOR_FLAG_WDR_NIGHT (SAMPLE_SENSOR_FLAG_NIGHT | SAMPLE_SENSOR_FLAG_WDR)

#define SENSOR_CREATE_MULTI(n) Sensor_Create##_##n
#define SENSOR_DESTROY_MULTI(n) Sensor_Destroy##_##n

extern struct isp_sensor_if *SENSOR_CREATE_MULTI(dummy_sensor)();
extern struct isp_sensor_if *SENSOR_CREATE_MULTI(ovos02k_mipi)();
extern struct isp_sensor_if *SENSOR_CREATE_MULTI(ovos02d_mipi)();
extern struct isp_sensor_if *SENSOR_CREATE_MULTI(ovos02h_mipi)();
extern struct isp_sensor_if *SENSOR_CREATE_MULTI(ovos02g10_mipi)();
extern struct isp_sensor_if *SENSOR_CREATE_MULTI(ovos03a_mipi)();
extern struct isp_sensor_if *SENSOR_CREATE_MULTI(ovos04c10_mipi)();
extern struct isp_sensor_if *SENSOR_CREATE_MULTI(ovos05_mipi)();
extern struct isp_sensor_if *SENSOR_CREATE_MULTI(ovos08_mipi)();
extern struct isp_sensor_if *SENSOR_CREATE_MULTI(imx415_mipi)();
extern struct isp_sensor_if *SENSOR_CREATE_MULTI(sc3335_mipi)();
extern struct isp_sensor_if *SENSOR_CREATE_MULTI(sc3336_mipi)();
extern struct isp_sensor_if *SENSOR_CREATE_MULTI(sc4336_mipi)();
extern struct isp_sensor_if *SENSOR_CREATE_MULTI(sc5336_mipi)();
extern struct isp_sensor_if *SENSOR_CREATE_MULTI(sc200ai_mipi)();
extern struct isp_sensor_if *SENSOR_CREATE_MULTI(sc450ai_mipi)();
extern struct isp_sensor_if *SENSOR_CREATE_MULTI(sc530ai_mipi)();
extern struct isp_sensor_if *SENSOR_CREATE_MULTI(ov2735_mipi)();
extern struct isp_sensor_if *SENSOR_CREATE_MULTI(gc2083_mipi)();
extern struct isp_sensor_if *SENSOR_CREATE_MULTI(cv2005_mipi)();
extern void SENSOR_DESTROY_MULTI(dummy_sensor)(struct isp_sensor_if *sensor_name);
extern void SENSOR_DESTROY_MULTI(ovos02k_mipi)(struct isp_sensor_if *sensor_name);
extern void SENSOR_DESTROY_MULTI(ovos02d_mipi)(struct isp_sensor_if *sensor_name);
extern void SENSOR_DESTROY_MULTI(ovos02h_mipi)(struct isp_sensor_if *sensor_name);
extern void SENSOR_DESTROY_MULTI(ovos02g10_mipi)(struct isp_sensor_if *sensor_name);
extern void SENSOR_DESTROY_MULTI(ovos03a_mipi)(struct isp_sensor_if *sensor_name);
extern void SENSOR_DESTROY_MULTI(ovos04c10_mipi)(struct isp_sensor_if *sensor_name);
extern void SENSOR_DESTROY_MULTI(ovos05_mipi)(struct isp_sensor_if *sensor_name);
extern void SENSOR_DESTROY_MULTI(ovos08_mipi)(struct isp_sensor_if *sensor_name);
extern void SENSOR_DESTROY_MULTI(imx415_mipi)(struct isp_sensor_if *sensor_name);
extern void SENSOR_DESTROY_MULTI(sc3335_mipi)(struct isp_sensor_if *sensor_name);
extern void SENSOR_DESTROY_MULTI(sc3336_mipi)(struct isp_sensor_if *sensor_name);
extern void SENSOR_DESTROY_MULTI(sc4336_mipi)(struct isp_sensor_if *sensor_name);
extern void SENSOR_DESTROY_MULTI(sc5336_mipi)(struct isp_sensor_if *sensor_name);
extern void SENSOR_DESTROY_MULTI(sc200ai_mipi)(struct isp_sensor_if *sensor_name);
extern void SENSOR_DESTROY_MULTI(sc450ai_mipi)(struct isp_sensor_if *sensor_name);
extern void SENSOR_DESTROY_MULTI(sc530ai_mipi)(struct isp_sensor_if *sensor_name);
extern void SENSOR_DESTROY_MULTI(ov2735_mipi)(struct isp_sensor_if *sensor_name);
extern void SENSOR_DESTROY_MULTI(gc2083_mipi)(struct isp_sensor_if *sensor_name);
extern void SENSOR_DESTROY_MULTI(cv2005_mipi)(struct isp_sensor_if *sensor_name);

struct isp_sensor_if *start_sensor(FH_CHAR *sensor_name, FH_SINT32 grp_id);

void destroy_sensor(FH_CHAR *sensor_name, struct isp_sensor_if *sensor);

FH_CHAR *getSensorHex(FH_SINT32 grp_id, FH_CHAR *sensor_name, FH_SINT32 flags, FH_SINT32 *olen);

FH_VOID freeSensorHex(FH_CHAR *param);

FH_SINT32 get_sensorFormat_wdr_mode(FH_SINT32 format);

FH_SINT32 changeSensorFPS(FH_SINT32 grp_id, FH_SINT32 fps);

#endif // __SAMPLE_COMMON_SENSOR_H__
