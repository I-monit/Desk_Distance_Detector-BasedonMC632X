/**
 ****************************************************************************************************
 * @file        atk_ms53l1m_uart.h
 * @brief       ATK-MS53L1M 模块 UART 接口驱动 (MC632X/RT-Thread 版本)
 *
 * 底层基于 RT-Thread 标准 serial device ("uart1"), 通过软件空闲超时判定一帧结束
 *  - STM32 的 IDLE 中断在 MC632X BSP 的 ISR 中不回调用户, 改为 rx_get_frame() 轮询
 *    时排空环形缓冲 + rt_tick_get() 判定 ≥1 tick (10ms@100Hz) 无新数据即帧完成
 *  - 引脚由 appboard_iopad.h 中已配的 "UART1" (GP0_2=TX, GP0_3=RX) 提供
 ****************************************************************************************************
 */

#ifndef __ATK_MS53L1M_UART_H
#define __ATK_MS53L1M_UART_H

#include <rtthread.h>
#include <stdint.h>

/* RT-Thread 设备名 - 与 menuconfig FH_USING_UART1 配合, 板级 pinmux 已配 (GP0_2/GP0_3) */
#ifndef ATK_MS53L1M_UART_DEV_NAME
#define ATK_MS53L1M_UART_DEV_NAME        "uart1"
#endif

/* UART 收发缓冲区大小 */
#define ATK_MS53L1M_UART_RX_BUF_SIZE     128

/* 软件判帧空闲门限 (单位 ticks). 100Hz tick 下 1 tick = 10ms,
 * 对 115200 速率下的整帧 (<2ms 传完) 留足余量. */
#define ATK_MS53L1M_UART_IDLE_TICKS      1

/* 外部接口函数 (签名与 STM32 版本保持一致, 协议层零改动) */
void     atk_ms53l1m_uart_init(uint32_t baudrate);          /* ATK-MS53L1M UART 初始化 */
void     atk_ms53l1m_uart_send(uint8_t *dat, uint8_t len);  /* ATK-MS53L1M UART 发送数据 */
void     atk_ms53l1m_uart_rx_restart(void);                 /* ATK-MS53L1M UART 重新开始接收数据 */
uint8_t *atk_ms53l1m_uart_rx_get_frame(void);               /* 获取 ATK-MS53L1M UART 接收到的一帧数据 */
uint16_t atk_ms53l1m_uart_rx_get_frame_len(void);           /* 获取 ATK-MS53L1M UART 接收到的一帧数据的长度 */

/* 本帧首字节到达时刻 | [ms] (rt_tick 基准, 0=本帧尚无字节; Time_Stamp Phase③) */
rt_uint32_t atk_ms53l1m_uart_first_rx_ms(void);

/* 调试用: 不发送任何字节, 仅打开 / 配置 uart1, 返回 <0 表示失败 */
int      atk_ms53l1m_uart_probe(uint32_t baudrate);
/* 调试用: uart1 自发自收 'a'..'d' 后从环形缓冲读回, 通过 console(uart0) 透显
 *         使用前: 拔掉 TOF 模块, 并将 uart1 的 TX 与 RX 物理短接 */
int      atk_ms53l1m_uart_loopback(void);

#endif