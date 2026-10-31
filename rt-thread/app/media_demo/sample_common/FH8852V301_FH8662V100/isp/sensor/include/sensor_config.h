#ifndef __SENSOR_CONFIG__
#define __SENSOR_CONFIG__
#include "sample_common_media.h"
#include "dsp/fh_system_mpi.h"
#include "vicap/fh_vicap_mpi.h"
#include "isp/isp_api.h"

#define VICAP_CLK 225000000

typedef struct sensor_config
{
    FH_SINT32 grp_id;
    FH_SINT32 i2c;
    FH_SINT32 gpio;
} SENSOR_CONFIG;

// FH8862V100
#ifndef FH_I2C_CHOOSE_G0
#define FH_I2C_CHOOSE_G0 2
#endif

#ifndef FH_I2C_CHOOSE_G1
#define FH_I2C_CHOOSE_G1 0
#endif

// FH8852V301
// #ifndef FH_I2C_CHOOSE_G0
// #define FH_I2C_CHOOSE_G0 0
// #endif

// #ifndef FH_I2C_CHOOSE_G1
// #define FH_I2C_CHOOSE_G1 1
// #endif

#ifndef FH_GPIO_SENSOR_RESET_G0
#define FH_GPIO_SENSOR_RESET_G0 49
#endif

#ifndef FH_GPIO_SENSOR_RESET_G1
#define FH_GPIO_SENSOR_RESET_G1 48
#endif

FH_SINT32 choose_i2c(FH_SINT32 grp_id);

FH_VOID reset_sensor(FH_SINT32 grp_id);

FH_SINT32 set_sensor_sync(FH_SINT32 grp_id, FH_UINT16 sensorFps);

FH_SINT32 start_sensor_sync(FH_SINT32 grp_id);

FH_SINT32 sensor_sleep(FH_SINT32 grp_id);

FH_SINT32 sensor_wakeup(FH_SINT32 grp_id);
#endif // __SENSOR_CONFIG__
