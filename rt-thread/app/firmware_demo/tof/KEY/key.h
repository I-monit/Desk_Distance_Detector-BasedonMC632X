/**
 ****************************************************************************************************
 * @file        key.h
 * @brief       按键事件队列 (MC632X 适配层 - 纯软件注入版, 无物理按键)
 *
 * KEY1 事件源 (GPIO2_6 中断已随物理按键弃用移除):
 *   - TCP "KEY1\n" 命令 (tcp_ctrl 线程解析, Qt 远程标定)
 *   - msh 命令 alg_key (串口调试)
 *
 * 头文件接口与 STM32 版本完全一致 (KEY1_PRES / WKUP_PRES), 业务层代码零改动.
 ****************************************************************************************************
 */

#ifndef __KEY_H
#define __KEY_H

#include <rtthread.h>
#include <stdint.h>

#define KEY1_PRES    2              /* KEY1 按下 */
#define WKUP_PRES    4              /* WK_UP 按下 */

void     key_init(void);                /* 清空挂起的按键事件 (tof_alg 启动 / guard 重启时调用) */
uint8_t  key_scan(uint8_t mode);        /* 按键扫描函数 (取走挂起的按键事件) */

/* 挂起一个按键事件: 由 tcp_ctrl 命令解析或 msh alg_key 调用 */
void     key_push_event(uint8_t key);

#endif