/**
 ****************************************************************************************************
 * @file        tof_sensor.c
 * @brief       ATK-MS53L1M TOF -> RT-Thread sensor 框架薄适配层
 *
 * 注册类别 RT_SENSOR_CLASS_TOF (vendored v4.1.1 框架原生支持, 设备名自动加
 * "tof_" 前缀 => "tof_ms53l1m"); 仅 POLLING 模式 (uart 轮询量测, 无中断引脚,
 * 框架 INT/FIFO 已按本板情况裁剪, 见 sensor.c 头注释)
 *
 * 数据通道: rt_device_read -> rt_sensor_read -> tof_fetch_data
 *           -> atk_ms53l1m_modbus_get_data (FH vendor 层)
 * timestamp 由 alg 层在读取完成时刻另取 rt_tick_get_millisecond (与
 * alg_input_nn 同钟基准, 与原直调路径节拍一致)
 ****************************************************************************************************
 */

#include <rtthread.h>
#include <string.h>
#include "sensor.h"
#include "tof_sensor.h"
#include "ATK_MS53L1M/atk_ms53l1m.h"

#define TOF_SENSOR_DEV_NAME     "ms53l1m"   /* 注册名, 框架按类别自动加 "tof_" 前缀 */
#define TOF_SENSOR_RANGE_MIN    20          /* MS53L1M 量程下限 | [mm] */
#define TOF_SENSOR_RANGE_MAX    2500        /* MS53L1M 量程上限 | [mm] */
#define TOF_SENSOR_PERIOD_MIN   140         /* 单次测量耗时 (TIMEBUDGET) | [ms] */

static struct rt_sensor_device g_tof_sensor;
static uint16_t                g_tof_id        = 0;
static rt_bool_t               g_tof_registered = RT_FALSE;

/* Time_Stamp Phase③: 最近一次 fetch 的 TOF 曝光区间 (rt_sensor_data 仅一个 ts 字段,
 * 不够装区间, 经此侧信道带出; fetch 成功后立即读, alg_glue 消费) */
static rt_uint32_t g_last_trig_ms  = 0;
static rt_uint32_t g_last_first_ms = 0;

/* 轮询模式单次量测: 经 vendor 层 modbus 读取, 填一帧 rt_sensor_data */
static rt_size_t tof_fetch_data(struct rt_sensor_device *sensor, void *buf, rt_size_t len)
{
    struct rt_sensor_data *data = (struct rt_sensor_data *)buf;
    uint16_t dist_mm = 0;

    RT_ASSERT(sensor != RT_NULL);

    if (buf == RT_NULL || len == 0)
        return 0;

    if (atk_ms53l1m_modbus_get_data(g_tof_id, &dist_mm) != ATK_MS53L1M_EOK)
        return 0;

    data->type = RT_SENSOR_CLASS_TOF;
    data->timestamp = (rt_uint32_t)rt_tick_get_millisecond(); /* 全项目统一 ms 基准 */
    data->data.proximity = (rt_int32_t)dist_mm;  /* 单位 mm (info.unit = RT_SENSOR_UNIT_MM) */

    atk_ms53l1m_get_last_interval(&g_last_trig_ms, &g_last_first_ms); /* 曝光区间快照 */

    return 1;
}

/* 最近一次测量的曝光区间 [trig_ms, first_ms] | [ms] (Time_Stamp Phase③, alg 层配对用) */
void tof_sensor_last_interval(rt_uint32_t *trig_ms, rt_uint32_t *first_ms)
{
    if (trig_ms != NULL)  *trig_ms  = g_last_trig_ms;
    if (first_ms != NULL) *first_ms = g_last_first_ms;
}

static rt_err_t tof_control(struct rt_sensor_device *sensor, int cmd, void *arg)
{
    switch (cmd)
    {
    case RT_SENSOR_CTRL_SET_MODE:
        return ((rt_ubase_t)arg == RT_SENSOR_MODE_POLLING) ? RT_EOK : -RT_ERROR;
    case RT_SENSOR_CTRL_SET_POWER:
        /* uart 总线模块无独立电源模式命令: 接受 NORMAL/DOWN 但不下发 */
        return ((rt_ubase_t)arg == RT_SENSOR_POWER_NORMAL ||
                (rt_ubase_t)arg == RT_SENSOR_POWER_DOWN) ? RT_EOK : -RT_ERROR;
    case RT_SENSOR_CTRL_GET_ID:
        if (arg == RT_NULL)
            return -RT_EINVAL;
        *(rt_uint32_t *)arg = g_tof_id;
        return RT_EOK;
    default:
        return -RT_ERROR;
    }
}

static const struct rt_sensor_ops g_tof_ops =
{
    .fetch_data = tof_fetch_data,
    .control    = tof_control,
};

int tof_sensor_init(void)
{
    /* vendor 层 init: uart1 find/open + 波特率 (重复调用幂等, guard 重启线程安全) */
    if (atk_ms53l1m_init(115200, &g_tof_id) != ATK_MS53L1M_EOK)
        return -1;

    if (g_tof_registered)              /* 设备只注册一次, 防 rt_device_register 重名 */
        return 0;

    memset(&g_tof_sensor, 0, sizeof(g_tof_sensor));

    g_tof_sensor.info.type      = RT_SENSOR_CLASS_TOF;
    g_tof_sensor.info.vendor    = RT_SENSOR_VENDOR_UNKNOWN;
    g_tof_sensor.info.model     = "atk-ms53l1m";
    g_tof_sensor.info.unit      = RT_SENSOR_UNIT_MM;
    g_tof_sensor.info.intf_type = RT_SENSOR_INTF_UART;
    g_tof_sensor.info.range_max = TOF_SENSOR_RANGE_MAX;
    g_tof_sensor.info.range_min = TOF_SENSOR_RANGE_MIN;
    g_tof_sensor.info.period_min = TOF_SENSOR_PERIOD_MIN;
    g_tof_sensor.info.fifo_max  = 0;   /* 纯轮询无 FIFO (module 为 NULL, open 不分配缓存) */
    g_tof_sensor.ops            = &g_tof_ops;
    g_tof_sensor.module         = RT_NULL;

    if (rt_hw_sensor_register(&g_tof_sensor, TOF_SENSOR_DEV_NAME,
                              RT_DEVICE_FLAG_RDONLY, RT_NULL) != RT_EOK)
    {
        rt_kprintf("[tof_sensor] register device failed\n");
        return -1;
    }

    g_tof_registered = RT_TRUE;
    rt_kprintf("[tof_sensor] device 'tof_%s' registered (polling, mm)\n", TOF_SENSOR_DEV_NAME);
    return 0;
}

rt_uint16_t tof_sensor_id(void)
{
    return g_tof_id;
}
