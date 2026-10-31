/**
 ****************************************************************************************************
 * @file        key.c
 * @brief       按键事件队列 (MC632X 适配层 - 纯软件注入版, 无物理按键)
 *
 * KEY1 事件源 (GPIO2_6 中断已随物理按键弃用而移除):
 *   - tcp_ctrl 线程: TCP "KEY1\n" 命令 (Qt 远程标定)
 *   - msh 命令 alg_key (串口调试)
 * 消费者: tof_alg 线程主循环 key_scan (tof/alg_glue.c)
 ****************************************************************************************************
 */

#include "KEY/key.h"

static volatile uint8_t g_pending_key = 0;    /* 当前挂起的按键 (KEY1_PRES / WKUP_PRES / 0) */

void key_init(void)
{
    g_pending_key = 0;    /* 仅清事件队列 (tof_alg 启动 / guard 重启时调用) */
}

void key_push_event(uint8_t key)
{
    g_pending_key = key;
}

uint8_t key_scan(uint8_t mode)
{
    uint8_t keyval = 0;

    (void)mode;   /* mode 参数保留兼容 */

    if (g_pending_key != 0)
    {
        keyval        = g_pending_key;   /* 取走最新挂起的事件 (单事件队列, 后到覆盖) */
        g_pending_key = 0;
    }

    return keyval;
}
