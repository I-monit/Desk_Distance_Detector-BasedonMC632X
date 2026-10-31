#ifndef __SENSOR_CONFIG__
#define __SENSOR_CONFIG__
#include "sample_common_media.h"
#include "vicap/fh_vicap_mpi.h"
#include "isp/isp_api.h"

#define VICAP_CLK 360000000
#define VICAP_HSYNC_TOTAL 600

typedef struct sensor_config
{
    FH_SINT32 grp_id;
    FH_SINT32 i2c;
    FH_SINT32 gpio;
} SENSOR_CONFIG;

#ifndef FH_I2C_CHOOSE_G0
#define FH_I2C_CHOOSE_G0 0
#endif

#ifndef FH_I2C_CHOOSE_G1
#define FH_I2C_CHOOSE_G1 0
#endif

#ifndef FH_GPIO_SENSOR_RESET_G0
#define FH_GPIO_SENSOR_RESET_G0 36
#endif

#ifndef FH_GPIO_SENSOR_RESET_G1
#define FH_GPIO_SENSOR_RESET_G1 40
#endif

FH_SINT32 choose_i2c(FH_SINT32 grp_id);

FH_VOID pulldown_sensor(FH_SINT32 grp_id);

FH_VOID pullup_sensor(FH_SINT32 grp_id);

FH_VOID reset_sensor(FH_SINT32 grp_id);

FH_SINT32 change_sensor_slaveaddr(FH_SINT32 grp_id, FH_CHAR *sensor_name);

FH_SINT32 set_sensor_sync(FH_SINT32 grp_id, FH_UINT16 sensorFps);

FH_SINT32 start_sensor_sync(FH_SINT32 grp_id);

FH_SINT32 sensor_sleep(FH_SINT32 grp_id);

FH_SINT32 sensor_wakeup(FH_SINT32 grp_id);

#endif // __SENSOR_CONFIG__
