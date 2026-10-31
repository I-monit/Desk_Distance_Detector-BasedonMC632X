#ifndef __TOF_SENSOR_H__
#define __TOF_SENSOR_H__

#include <rtthread.h>
#include "sensor.h"    /* struct rt_sensor_data (rt_device_read 出参) */

/*
 * ATK-MS53L1M TOF -> RT-Thread sensor 框架薄适配层
 *
 * alg 层不再直接调 vendor 函数, 改为标准设备接口取数:
 *   rt_device_find("tof_ms53l1m") -> rt_device_open(RT_DEVICE_FLAG_RDONLY)
 *   -> rt_device_read(&rt_sensor_data) -> data.data.proximity | [mm]
 * fetch_data 内部调 atk_ms53l1m_modbus_get_data (vendor 层处理 uart1/modbus 时序)
 */

/* 初始化 vendor 层 (uart1 find/open/波特率) 并注册 sensor 设备 "tof_ms53l1m"
 * 返回 0 成功; 设备只注册一次 (guard 重启 tof_alg 线程不重复注册) */
int tof_sensor_init(void);

/* vendor 层分配的模块 id (调试打印用) */
rt_uint16_t tof_sensor_id(void);

/* 最近一次测量的曝光区间 [trig_ms, first_ms] | [ms] (Time_Stamp Phase③):
 * trig = modbus 请求发出时刻, first = 应答首字节到达时刻, 中点即观测时刻最佳估计 */
void tof_sensor_last_interval(rt_uint32_t *trig_ms, rt_uint32_t *first_ms);

#endif /* __TOF_SENSOR_H__ */
