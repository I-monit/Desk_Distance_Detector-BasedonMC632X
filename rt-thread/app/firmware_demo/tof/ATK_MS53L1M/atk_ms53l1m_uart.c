/**
 ****************************************************************************************************
 * @file        atk_ms53l1m_uart.c
 * @brief       ATK-MS53L1M 模块 UART 接口驱动 (MC632X/RT-Thread 版本)
 *
 * 帧接收实现 (替代 STM32 的 RXNE+IDLE 中断):
 *   协议层每次轮询 atk_ms53l1m_uart_rx_get_frame() 时:
 *     1. 排空 rt_device 环形缓冲区入帧缓存 (保留原溢出处理逻辑)
 *     2. 若 (now - last_rx_tick) >= ATK_MS53L1M_UART_IDLE_TICKS 且 len>0 → 标记帧完成
 *   单次阻塞发送: rt_device_write (底层轮询模式, 简单可靠)
 ****************************************************************************************************
 */

#include "ATK_MS53L1M/atk_ms53l1m_uart.h"
#include <rtdevice.h>

static rt_device_t g_uart_dev = RT_NULL;

static struct
{
    uint8_t buf[ATK_MS53L1M_UART_RX_BUF_SIZE];              /* 帧接收缓存 */
    struct
    {
        uint16_t len    : 15;                               /* 帧接收长度 sta[14:0] */
        uint16_t         finsh  : 1;                                /* 帧接收完成标志 sta[15] */
    } sta;                                                  /* 帧状态信息 */
    rt_tick_t last_rx_tick;                                 /* 最近一次收到字节的时间 (用于空闲判帧) */
    rt_tick_t first_rx_tick;                                /* 本帧首字节到达时间 (Time_Stamp: 曝光刚结束的最近信号) */
} g_uart_rx_frame = {0};

/**
 * @brief   内部: 排空 RT-Thread 串口环形缓冲到帧缓存, 并更新最后接收时间戳.
 *          保留 STM32 原版的溢出处理 (缓冲满则丢弃旧数据, 重新开始).
 * @retval  无
 */
static void uart_rx_poll(void)
{
    uint8_t tmp;
    rt_size_t n;

    do
    {
        n = rt_device_read(g_uart_dev, 0, &tmp, 1);
        if (n != 1)
        {
            break;
        }
        g_uart_rx_frame.last_rx_tick = rt_tick_get();
        if (g_uart_rx_frame.sta.len == 0)
        {
            g_uart_rx_frame.first_rx_tick = rt_tick_get(); /* 0->1 沿: 首字节 */
        }

        if (g_uart_rx_frame.sta.len < (ATK_MS53L1M_UART_RX_BUF_SIZE - 1))
        {
            g_uart_rx_frame.buf[g_uart_rx_frame.sta.len] = tmp;
            g_uart_rx_frame.sta.len++;
        }
        else
        {
            g_uart_rx_frame.sta.len = 0;
            g_uart_rx_frame.buf[g_uart_rx_frame.sta.len] = tmp;
            g_uart_rx_frame.sta.len++;
        }
    } while (1);
}

/**
 * @brief   内部: 空闲超时判帧. 若已收到字节且距上次接收 ≥ 门限 ticks, 标记帧完成.
 * @retval  无
 */
static void uart_check_frame_done(void)
{
    if ((g_uart_rx_frame.sta.len > 0) && (g_uart_rx_frame.sta.finsh == 0))
    {
        if ((rt_tick_get() - g_uart_rx_frame.last_rx_tick) >= ATK_MS53L1M_UART_IDLE_TICKS)
        {
            g_uart_rx_frame.sta.finsh = 1;
        }
    }
}

/**
 * @brief   ATK-MS53L1M UART 初始化
 * @param   baudrate: UART 通信波特率
 * @retval  无
 */
void atk_ms53l1m_uart_init(uint32_t baudrate)
{
    struct serial_configure cfg = RT_SERIAL_CONFIG_DEFAULT;

    g_uart_dev = rt_device_find(ATK_MS53L1M_UART_DEV_NAME);
    if (g_uart_dev == RT_NULL)
    {
        rt_kprintf("[atk_ms53l1m_uart] find %s failed (check FH_USING_UART1 & pinmux)\n",
                   ATK_MS53L1M_UART_DEV_NAME);
        return;
    }

    cfg.baud_rate = baudrate;
    cfg.data_bits = DATA_BITS_8;
    cfg.stop_bits = STOP_BITS_1;
    cfg.parity    = PARITY_NONE;

    rt_device_control(g_uart_dev, RT_DEVICE_CTRL_CONFIG, &cfg);

    /* INT_RX 让驱动自动填充环形缓冲; 写操作为轮询, 简单可靠 */
    rt_device_open(g_uart_dev, RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_INT_RX);

    g_uart_rx_frame.sta.len       = 0;
    g_uart_rx_frame.sta.finsh     = 0;
    g_uart_rx_frame.last_rx_tick  = rt_tick_get();
    g_uart_rx_frame.first_rx_tick = 0;
}

/**
 * @brief   ATK-MS53L1M UART 发送数据
 * @param   dat: 发送的数据
 *          len: 发送的数据长度
 * @retval  无
 */
void atk_ms53l1m_uart_send(uint8_t *dat, uint8_t len)
{
    if (g_uart_dev == RT_NULL)
    {
        return;
    }
    rt_device_write(g_uart_dev, 0, dat, len);
}

/**
 * @brief   ATK-MS53L1M UART 重新开始接收数据
 * @param   无
 * @retval  无
 */
void atk_ms53l1m_uart_rx_restart(void)
{
    g_uart_rx_frame.sta.len        = 0;
    g_uart_rx_frame.sta.finsh      = 0;
    g_uart_rx_frame.last_rx_tick   = rt_tick_get();
    g_uart_rx_frame.first_rx_tick  = 0;
}

/* 本帧首字节到达时刻 | [ms] (与 rt_tick_get_millisecond 同换算, Time_Stamp Phase③)
 * 0 = 本帧尚无字节; 协议层轮询 mdelay(1) 实睡 10ms, 故该时刻距真实首字节 ≤ 1 tick */
rt_uint32_t atk_ms53l1m_uart_first_rx_ms(void)
{
    if (g_uart_rx_frame.first_rx_tick == 0)
        return 0;
    return (rt_uint32_t)(g_uart_rx_frame.first_rx_tick * (1000 / RT_TICK_PER_SECOND));
}

/**
 * @brief   获取 ATK-MS53L1M UART 接收到的一帧数据
 * @param   无
 * @retval  NULL    : 未接收到一帧数据
 *          其他    : 接收到的一帧数据
 */
uint8_t *atk_ms53l1m_uart_rx_get_frame(void)
{
    if (g_uart_dev == RT_NULL)
    {
        return NULL;
    }

    if (g_uart_rx_frame.sta.finsh == 0)
    {
        uart_rx_poll();
        uart_check_frame_done();
    }

    if (g_uart_rx_frame.sta.finsh == 1)
    {
        g_uart_rx_frame.buf[g_uart_rx_frame.sta.len] = '\0';
        return g_uart_rx_frame.buf;
    }

    return NULL;
}

/**
 * @brief   获取 ATK-MS53L1M UART 接收到的一帧数据的长度
 * @param   无
 * @retval  0   : 未接收到一帧数据
 *          其他: 接收到的一帧数据的长度
 */
uint16_t atk_ms53l1m_uart_rx_get_frame_len(void)
{
    if (g_uart_rx_frame.sta.finsh == 1)
    {
        return g_uart_rx_frame.sta.len;
    }

    return 0;
}

/**
 * @brief   调试用: 仅打开并配置 uart1, 不发任何字节, 用于判断设备是否存在 / open 是否成功
 * @param   baudrate: 配置波特率
 * @retval  0  : 设备找到并成功 open
 *         -1  : 设备未找到 (检查 FH_USING_UART1 与 pinmux)
 *         -2  : open 失败
 */
int atk_ms53l1m_uart_probe(uint32_t baudrate)
{
    struct serial_configure cfg = RT_SERIAL_CONFIG_DEFAULT;

    g_uart_dev = rt_device_find(ATK_MS53L1M_UART_DEV_NAME);
    if (g_uart_dev == RT_NULL)
    {
        return -1;
    }

    cfg.baud_rate = baudrate;
    cfg.data_bits = DATA_BITS_8;
    cfg.stop_bits = STOP_BITS_1;
    cfg.parity    = PARITY_NONE;
    rt_device_control(g_uart_dev, RT_DEVICE_CTRL_CONFIG, &cfg);

    if (rt_device_open(g_uart_dev, RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_INT_RX) != RT_EOK)
    {
        return -2;
    }

    g_uart_rx_frame.sta.len       = 0;
    g_uart_rx_frame.sta.finsh     = 0;
    g_uart_rx_frame.last_rx_tick  = rt_tick_get();
    return 0;
}

/**
 * @brief   调试用: uart1 自发自收 + 通过 console(uart0) 透显
 *          使用前请把模块拔掉, 并将 uart1 的 RX/TX 物理短接.
 *          流程: uart1 发送 'a'..'d' (0x61..0x64), 短接后 RX 收到回环字节,
 *                从环形缓冲一次性读出后通过 rt_kprintf(走 uart0/console) 打印到 PC 串口工具.
 * @retval  0  : 通过 (回读字节数 == 4 且内容为 'a'..'d')
 *         -1  : 设备未找到
 *         -2  : open 失败
 *         -3  : 写入失败 (rt_device_write 返回 0)
 *         -4  : 未读回任何字节
 *         -5  : 读回字节数或内容不匹配
 */
int atk_ms53l1m_uart_loopback(void)
{
    static const uint8_t tx[4] = {'a', 'b', 'c', 'd'};
    uint8_t rx[4] = {0};
    rt_size_t wr, rd;
    int i;

    if (g_uart_dev == RT_NULL)
    {
        int rc = atk_ms53l1m_uart_probe(115200);
        if (rc != 0) return (rc == -1) ? -1 : -2;
    }
    else
    {
        /* 已 open: 清空残留, 确保排空起点干净 */
        g_uart_rx_frame.sta.len      = 0;
        g_uart_rx_frame.sta.finsh    = 0;
        g_uart_rx_frame.last_rx_tick = rt_tick_get();
    }

    wr = rt_device_write(g_uart_dev, 0, tx, sizeof(tx));
    if (wr == 0)
    {
        return -3;
    }

    rt_thread_mdelay(20);

    rd = rt_device_read(g_uart_dev, 0, rx, sizeof(rx));
    if (rd == 0)
    {
        return -4;
    }
    if (rd != sizeof(tx))
    {
        return -5;
    }
    for (i = 0; i < (int)sizeof(tx); i++)
    {
        if (rx[i] != tx[i]) return -5;
    }
    rt_kprintf("[loopback] RX <- ");
    for (i = 0; i < (int)rd; i++)
    {
        rt_kprintf("%c", (char)rx[i]);
    }
    rt_kprintf("  (hex:");
    for (i = 0; i < (int)rd; i++)
    {
        rt_kprintf(" %02X", rx[i]);
    }
    rt_kprintf(")\n");
    return 0;
}