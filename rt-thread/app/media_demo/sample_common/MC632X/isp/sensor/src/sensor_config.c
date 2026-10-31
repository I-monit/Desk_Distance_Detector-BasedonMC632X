#include "sensor_config.h"
#ifdef __RTTHREAD_OS__
#include "gpio.h"
#endif

static FH_CHAR *model_name = MODEL_TAG_SENSOR_CONFIG;

SENSOR_CONFIG g_sensor_config[MAX_GRP_NUM] = {
    /*********************G0************************/
    {
        .grp_id = 0,
        .gpio = FH_GPIO_SENSOR_RESET_G0,
        .i2c = FH_I2C_CHOOSE_G0,
    },
    /*********************G1************************/
    {
        .grp_id = 1,
        .gpio = FH_GPIO_SENSOR_RESET_G1,
        .i2c = FH_I2C_CHOOSE_G1,
    },
};

#ifdef __LINUX_OS__
#define SYSFS_GPIO_DIR "/sys/class/gpio"
#define SYSFS_GPIO_NAME "/GPIO"
#define MAX_BUF 64

int gpio_export(unsigned int gpio)
{
    int fd, len;
    char buf[MAX_BUF];

    fd = open(SYSFS_GPIO_DIR "/export", O_WRONLY);
    if (fd < 0)
    {
        perror("gpio/export");
        return fd;
    }

    memset(buf, 0, sizeof(buf));
    len = snprintf(buf, sizeof(buf), "%d\n", gpio);
    write(fd, buf, len + 1);
    close(fd);

    return 0;
}

int gpio_unexport(unsigned int gpio)
{
    int fd, len;
    char buf[MAX_BUF];

    fd = open(SYSFS_GPIO_DIR "/unexport", O_WRONLY);
    if (fd < 0)
    {
        perror("gpio/export");
        return fd;
    }

    memset(buf, 0, sizeof(buf));
    len = snprintf(buf, sizeof(buf), "%d\n", gpio);
    write(fd, buf, len + 1);
    close(fd);

    return 0;
}

int gpio_set_direction(unsigned int gpio, const char *direction)
{
    int fd;
    char buf[MAX_BUF];

    memset(buf, 0, sizeof(buf));
    snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR SYSFS_GPIO_NAME "%d/direction", gpio);

    fd = open(buf, O_WRONLY);
    if (fd < 0)
    {
        perror("gpio/direction");
        return fd;
    }

    write(fd, direction, strlen(direction) + 1);
    close(fd);

    return 0;
}

int gpio_setv(unsigned int gpio, unsigned int value)
{
    int fd;
    char buf[MAX_BUF];

    memset(buf, 0, sizeof(buf));
    snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR SYSFS_GPIO_NAME "%d/value", gpio);

    fd = open(buf, O_WRONLY);
    if (fd < 0)
    {
        perror("gpio/set-value");
        return fd;
    }

    if (value)
        write(fd, "1\n", 3);
    else
        write(fd, "0\n", 3);
    close(fd);

    return 0;
}

static int fh_gpio_set_value(unsigned int gpio, int value)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    char cmd[100];

    memset(cmd, 0, sizeof(cmd));
    sprintf(cmd, SYSFS_GPIO_DIR SYSFS_GPIO_NAME "%d", gpio);
    if (access(cmd, F_OK) != 0)
    {
        memset(cmd, 0, sizeof(cmd));
        ret = gpio_export(gpio);
        SDK_FUNC_ERROR(model_name, ret);
    }

    ret = gpio_set_direction(gpio, "out\n");
    SDK_FUNC_ERROR(model_name, ret);

    ret = gpio_setv(gpio, value);
    SDK_FUNC_ERROR(model_name, ret);

    ret = gpio_unexport(gpio);
    SDK_FUNC_ERROR(model_name, ret);

    return ret;
}
#else
static int fh_gpio_set_value(unsigned int gpio, int value)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;

    ret = gpio_request(gpio);
    SDK_FUNC_ERROR(model_name, ret);

    ret = gpio_direction_output(gpio, value);
    SDK_FUNC_ERROR(model_name, ret);

    ret = gpio_release(gpio);
    SDK_FUNC_ERROR(model_name, ret);

    return ret;
}
#endif

FH_SINT32 choose_i2c(FH_SINT32 grp_id)
{
    SDK_CHECK_MAX_VALID(model_name, grp_id, MAX_GRP_NUM, "grp_id > MAX_GRP_NUM");
    SDK_CHECK_MIN_VALID(model_name, grp_id, 0, "grp_id < 0");

    return g_sensor_config[grp_id].i2c;
}

FH_VOID pulldown_sensor(FH_SINT32 grp_id)
{
    SDK_PRT(model_name, "Pulldown Sensor[%d]!\n", grp_id);
    fh_gpio_set_value(g_sensor_config[grp_id].gpio, 0);
}

FH_VOID pullup_sensor(FH_SINT32 grp_id)
{
    SDK_PRT(model_name, "Pullup Sensor[%d]!\n", grp_id);
    fh_gpio_set_value(g_sensor_config[grp_id].gpio, 1);
}

FH_VOID reset_sensor(FH_SINT32 grp_id)
{
    SDK_PRT(model_name, "Start Reset Sensor[%d]!\n", grp_id);
    switch (grp_id)
    {
    case 0:
        fh_gpio_set_value(g_sensor_config[grp_id].gpio, 0);
        usleep(100 * 1000);
        fh_gpio_set_value(g_sensor_config[grp_id].gpio, 1);
        break;
    case 1:
        fh_gpio_set_value(g_sensor_config[grp_id].gpio, 0);
        usleep(100 * 1000);
        fh_gpio_set_value(g_sensor_config[grp_id].gpio, 1);
        break;
    }
    usleep(100 * 1000);
    SDK_PRT(model_name, "End Reset Sensor[%d]!\n", grp_id);
}

FH_SINT32 change_sensor_slaveaddr(FH_SINT32 grp_id, FH_CHAR *sensor_name)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;

#ifdef CONFIG_SENSOR_SLAVE_MODE
    if (!strcmp(sensor_name, "sc3336_mipi")) // 单双路的从机地址设置,目前sc3336支持
    {
        if (grp_id == 0)
        {
            ret = API_ISP_ChangeSnsSlaveAddr(grp_id, 0x64);
            SDK_FUNC_ERROR(model_name, ret);
        }
        else if (grp_id == 1)
        {
            ret = API_ISP_ChangeSnsSlaveAddr(grp_id, 0x68);
            SDK_FUNC_ERROR(model_name, ret);
        }
        SDK_PRT(model_name, "Set Sensor[%d] slave addr!\n", grp_id);
    }
#else
    SDK_PRT(model_name, "NOT Set Sensor[%d] slave addr!\n", grp_id);
#endif

    return ret;
}

FH_SINT32 set_sensor_sync(FH_SINT32 grp_id, FH_UINT16 sensorFps) // 同步信号设置
{
    FH_SINT32 ret = FH_SDK_SUCCESS;

#ifdef CONFIG_SENSOR_SLAVE_MODE
    FH_VICAP_FRAME_SEQ_CFG_S VicapSync;

    VicapSync.enSyncType = FH_VICAP_HSYNC;
    VicapSync.u16Period = VICAP_HSYNC_TOTAL;
    VicapSync.u16Duty = (VicapSync.u16Period) / 2;
    VicapSync.bPolatiry = 0;
    ret = FH_VICAP_SetFrameSeqGenCfg(grp_id, &VicapSync);
    SDK_FUNC_ERROR(model_name, ret);

    VicapSync.enSyncType = FH_VICAP_VSYNC;
    VicapSync.u16Period = (VICAP_CLK) / (sensorFps * VICAP_HSYNC_TOTAL * 32) + 1;
    VicapSync.u16Duty = (VicapSync.u16Period) / 2;
    ret = FH_VICAP_SetFrameSeqGenCfg(grp_id, &VicapSync);
    SDK_FUNC_ERROR(model_name, ret);
#endif

    return ret;
}

FH_SINT32 start_sensor_sync(FH_SINT32 grp_id)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;

#ifdef CONFIG_SENSOR_SLAVE_MODE
#ifdef VIDEO_GRP1
    if (grp_id == 1) // 双路情况下，保证一起开启
    {
        ret = FH_VICAP_SetFrameSeqGenEn(0x3);
        SDK_FUNC_ERROR(model_name, ret);
        SDK_PRT(model_name, "Start Sensor[%d] sync!\n", grp_id);
    }
#else
    ret = FH_VICAP_SetFrameSeqGenEn(0x1);
    SDK_FUNC_ERROR(model_name, ret);
    SDK_PRT(model_name, "Start Sensor[%d] sync!\n", grp_id);
#endif
#endif

    return ret;
}

FH_SINT32 sensor_sleep(FH_SINT32 grp_id)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    SDK_PRT(model_name, "Sensor[%d] sleep!\n", grp_id);
    return ret;
}

FH_SINT32 sensor_wakeup(FH_SINT32 grp_id)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    SDK_PRT(model_name, "Sensor[%d] wakeup!\n", grp_id);
    return ret;
}
