/**
 ****************************************************************************************************
 * @file        atk_ms53l1m.h
 * @brief       ATK-MS53L1M 模块驱动 (移植自正点原子)
 ****************************************************************************************************
 */

#ifndef __ATK_MS53L1M_H
#define __ATK_MS53L1M_H

#include <rtthread.h>
#include <stdint.h>
#include "ATK_MS53L1M/atk_ms53l1m_uart.h"

/* ATK-MS53L1M 模块功能码 */
enum
{
    ATK_MS53L1M_FUNCODE_SYS             = 0x00, /* 系统功能 */
    ATK_MS53L1M_FUNCODE_BACKRATE        = 0x01, /* 回传频率设置 */
    ATK_MS53L1M_FUNCODE_BAUDRATE        = 0x02, /* 波特率设置 */
    ATK_MS53L1M_FUNCODE_IDSET           = 0x03, /* 设备地址设置 */
    ATK_MS53L1M_FUNCODE_MEAUDATA        = 0x05, /* 测量数据获取 */
    ATK_MS53L1M_FUNCODE_OUTPUTSTATUS    = 0x07, /* 输出状态 */
    ATK_MS53L1M_FUNCODE_MEAUMODE        = 0x08, /* 测量模式设置 */
    ATK_MS53L1M_FUNCODE_CALIMODE        = 0x09, /* 校准模式 */
    ATK_MS53L1M_FUNCODE_WORKMODE        = 0x0A, /* 工作模式 */
    ATK_MS53L1M_FUNCODE_TIMEBUDGET      = 0x0B, /* 定时预算 */
    ATK_MS53L1M_FUNCODE_TIMRPERIOD      = 0x0D, /* 测量周期 */
    ATK_MS53L1M_FUNCODE_ERRORFRAM       = 0x0F, /* 错误帧信息 */
    ATK_MS53L1M_FUNCODE_VERSION         = 0x10, /* 版本信息 */
};

/* 系统配置参数 */
enum
{
    ATK_MS53L1M_SYS_PARAM_RESET         = 0x01, /* 系统恢复默认 */
    ATK_MS53L1M_SYS_RESET               = 0x02, /* 系统复位 */
};

/* 回传频率配置参数 */
enum
{
    ATK_MS53L1M_BACKRATE_01HZ           = 0x00, /* 0.1Hz */
    ATK_MS53L1M_BACKRATE_02HZ           = 0x01, /* 0.2Hz */
    ATK_MS53L1M_BACKRATE_05HZ           = 0x02, /* 0.5Hz */
    ATK_MS53L1M_BACKRATE_1HZ            = 0x03, /* 1Hz */
    ATK_MS53L1M_BACKRATE_2HZ            = 0x04, /* 2Hz */
    ATK_MS53L1M_BACKRATE_5HZ            = 0x05, /* 5Hz */
    ATK_MS53L1M_BACKRATE_10HZ           = 0x06, /* 10Hz */
    ATK_MS53L1M_BACKRATE_20HZ           = 0x07, /* 20Hz */
    ATK_MS53L1M_BACKRATE_50HZ           = 0x08, /* 50Hz */
    ATK_MS53L1M_BACKRATE_100HZ          = 0x09, /* 100Hz */
};

/* 串口波特率配置参数 */
enum
{
    ATK_MS53L1M_BAUDRATE_2400           = 0x00, /* 2400bps */
    ATK_MS53L1M_BAUDRATE_4800           = 0x01, /* 4800bps */
    ATK_MS53L1M_BAUDRATE_9600           = 0x02, /* 9600bps */
    ATK_MS53L1M_BAUDRATE_19200          = 0x03, /* 19200bps */
    ATK_MS53L1M_BAUDRATE_38400          = 0x04, /* 38400bps */
    ATK_MS53L1M_BAUDRATE_57600          = 0x05, /* 57600bps */
    ATK_MS53L1M_BAUDRATE_115200         = 0x06, /* 115200bps */
    ATK_MS53L1M_BAUDRATE_230400         = 0x07, /* 230400bps */
    ATK_MS53L1M_BAUDRATE_460800         = 0x08, /* 460800bps */
    ATK_MS53L1M_BAUDRATE_921600         = 0x09, /* 921600bps */
};

/* 测量模式配置参数 */
enum
{
    ATK_MS53L1M_MEAUMODE_SHORT          = 0x00, /* 短距离 */
    ATK_MS53L1M_MEAUMODE_MIDDLE         = 0x01, /* 中距离 */
    ATK_MS53L1M_MEAUMODE_LONG           = 0x02, /* 长距离 */
};

/* 工作模式配置参数 */
enum
{
    ATK_MS53L1M_WORKMODE_NORMAL         = 0x00, /* Normal 模式 */
    ATK_MS53L1M_WORKMODE_MODBUS         = 0x01, /* Modbus 模式 */
    ATK_MS53L1M_WORKMODE_IIC            = 0x02, /* IIC 模式 */
};

/* 错误帧配置参数 */
enum
{
    ATK_MS53L1M_ERRORFRAM_OFF           = 0x00, /* 关闭 */
    ATK_MS53L1M_ERRORFRAM_ON            = 0x01, /* 开启 */
};

/* 错误代码 */
#define ATK_MS53L1M_EOK         0   /* 没有错误 */
#define ATK_MS53L1M_ERROR       1   /* 错误 */
#define ATK_MS53L1M_ETIMEOUT    2   /* 超时错误 */
#define ATK_MS53L1M_EFRAME      3   /* 帧错误 */
#define ATK_MS53L1M_ECRC        4   /* CRC 校验错误 */
#define ATK_MS53L1M_EOPT        5   /* 操作错误 */

/* 外部接口函数 */
uint8_t atk_ms53l1m_read_data(uint16_t addr, uint8_t code, uint8_t len, uint16_t *dat); /* 通过模块功能码读取数据 */
uint8_t atk_ms53l1m_write_data(uint16_t addr, uint8_t reg, uint8_t dat);                /* 通过模块功能码写入1字节数据 */
uint8_t atk_ms53l1m_write_data16(uint16_t addr, uint8_t reg, uint16_t dat);             /* 通过模块功能码写入2字节数据 (大端序) */
uint8_t atk_ms53l1m_init(uint32_t baudrate, uint16_t *id);                              /* ATK-MS53L1M 初始化 */
uint8_t atk_ms53l1m_normal_get_data(uint16_t *dat);                                     /* ATK-MS53L1M Normal 工作模式获取测量值 */
uint8_t atk_ms53l1m_modbus_get_data(uint16_t id, uint16_t *dat);                        /* ATK-MS53L1M Modbus 工作模式获取测量值 */

/* 最近一次测量的曝光区间 [trig_ms, first_ms] | [ms, rt_tick 基准] (Time_Stamp Phase③):
 * trig = modbus 请求发出时刻, first = 应答首字节到达时刻, 区间中点即观测时刻最佳估计 */
void    atk_ms53l1m_get_last_interval(rt_uint32_t *trig_ms, rt_uint32_t *first_ms);

#endif
