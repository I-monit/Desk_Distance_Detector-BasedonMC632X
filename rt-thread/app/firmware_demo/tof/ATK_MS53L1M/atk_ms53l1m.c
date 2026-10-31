/**
 ****************************************************************************************************
 * @file        atk_ms53l1m.c
 * @brief       ATK-MS53L1M 模块驱动 (移植自正点原子)
 ****************************************************************************************************
 */

#include "ATK_MS53L1M/atk_ms53l1m.h"
#include <rtthread.h>
#include <string.h>

#define ATK_MS53L1M_MASTER_FRAME_HEAD   0x51    /* 主机发送帧头 */
#define ATK_MS53L1M_SLAVE_FRAME_HEAD    0x55    /* 从机应答帧头 */
#define ATK_MS53L1M_SENSOR_TYPE         0x0B    /* ATK-MS53L1M 传感器类型 */

#define ATK_MS53L1M_FRAME_LEN_MAX       270     /* 数据帧最大长度 */
#define ATK_MS53L1M_FRAME_LEN_MIN       8       /* 数据帧最小长度 */

#define ATK_MS53L1M_OPT_READ            0x00    /* 读操作 */
#define ATK_MS53L1M_OPT_WRITE           0x01    /* 写操作 */

/* Time_Stamp Phase③: 最近一次 read_data 的曝光区间 [trig, first] | [ms, rt_tick 基准] */
static rt_uint32_t g_tof_trig_ms  = 0;
static rt_uint32_t g_tof_first_ms = 0;

static void atk_ms53l1m_interval_snapshot(void);

/**
 * @brief       计算 CRC 校验和
 * @param       buf: 数据缓冲区
 *              len: 数据长度
 * @retval      CRC 校验和值
 */
static inline uint16_t atk_ms53l1m_crc_check_sum(uint8_t *buf, uint16_t len)
{
    uint16_t check_sum = 0;
    uint16_t i;

    for (i=0; i<len; i++)
    {
        check_sum += buf[i];
    }

    return check_sum;
}

/**
 * @brief       解析接收到的数据包
 * @param       dat: 解析时用于获取数据的指针
 * @retval      ATK_MS53L1M_EOK     : 没有错误
 *              ATK_MS53L1M_ETIMEOUT: 接收数据超时
 *              ATK_MS53L1M_EFRAME  : 帧错误
 *              ATK_MS53L1M_ECRC    : CRC 校验错误
 *              ATK_MS53L1M_EOPT    : 操作错误
 */
static uint8_t atk_ms53l1m_unpack_recv_data(uint16_t *dat)
{
    uint8_t *frame_buf = NULL;
    uint16_t timeout = 0;
    uint16_t recv_len = 0;
    uint16_t frame_loop = 0;
    uint16_t frame_head_index;
    uint16_t frame_len;
    uint8_t opt_type;
    uint16_t dat_len;
    uint16_t frame_check_sum;
    uint16_t check_sum;

    while (frame_buf == NULL)
    {
        /* 等待 ATK-MS53L1M UART 接收到一帧数据 */
        frame_buf = atk_ms53l1m_uart_rx_get_frame();
        rt_thread_mdelay(1);
        timeout++;
        if (timeout == 100)
        {
            /* 接收超时错误 (MC632X@100Hz tick 下 mdelay(1) 实睡 10ms, 100 次 ≈ 1s) */
            rt_kprintf("[unpack] TIMEOUT (no frame in 1s)\n");
            return ATK_MS53L1M_ETIMEOUT;
        }
    }

    /* 获取接收数据的长度 */
    recv_len = atk_ms53l1m_uart_rx_get_frame_len();
    {
        uint16_t di;
        rt_kprintf("[unpack] recv_len=%u  hex:", recv_len);
        for (di = 0; di < recv_len && di < 32; di++) rt_kprintf(" %02X", frame_buf[di]);
        if (recv_len > 32) rt_kprintf(" ...");
        rt_kprintf("\n");
    }
    if ((recv_len < ATK_MS53L1M_FRAME_LEN_MIN) || (recv_len > ATK_MS53L1M_FRAME_LEN_MAX))
    {
        /* 数据帧长度异常错误 */
        rt_kprintf("[unpack] EFRAME: len out of range\n");
        return ATK_MS53L1M_EFRAME;
    }

    /* 查找帧头 */
    do
    {
        if ((frame_buf[frame_loop] == ATK_MS53L1M_SLAVE_FRAME_HEAD) &&
            (frame_buf[frame_loop + 1] == ATK_MS53L1M_SENSOR_TYPE))
        {
            break;
        }

        if (frame_loop != (recv_len - 2))
        {
            frame_loop++;
        }
        else
        {
            /* 帧异常 */
            rt_kprintf("[unpack] EFRAME: no 0x55 0x0B head found\n");
            return ATK_MS53L1M_EFRAME;
        }
    } while (1);

    frame_head_index = frame_loop;              /* 记录帧头位置 */
    frame_len = recv_len - frame_head_index;    /* 计算帧长度 */

    if ((frame_len < ATK_MS53L1M_FRAME_LEN_MIN) || (frame_len > ATK_MS53L1M_FRAME_LEN_MAX))
    {
        /* 数据帧长度异常错误 */
        rt_kprintf("[unpack] EFRAME: frame_len=%u out of range\n", frame_len);
        return ATK_MS53L1M_EFRAME;
    }

    opt_type = frame_buf[frame_head_index + 4]; /* 获取操作类型 */


    if (opt_type == 0x00)
    {
        dat_len = frame_buf[frame_head_index + 7];  /* 获取数据长度 */
        if ((dat_len + 10) > frame_len)
        {
            /* 帧异常 */
            rt_kprintf("[unpack] EFRAME: dat_len=%u + 10 > frame_len=%u\n", dat_len, frame_len);
            return ATK_MS53L1M_EFRAME;
        }

        frame_check_sum = atk_ms53l1m_crc_check_sum(&frame_buf[frame_head_index], dat_len + 8);   /* 计算 CRC 校验和 */
        check_sum = ((uint16_t)frame_buf[frame_head_index + dat_len + 8] << 8) + frame_buf[frame_head_index + dat_len + 9]; /* 获取帧尾 CRC 校验和 */
        if (frame_check_sum == check_sum)
        {
            if (frame_buf[frame_head_index + 7] == 1)
            {
                *dat = frame_buf[frame_head_index + 8];
            }
            else if (frame_buf[frame_head_index + 7] == 2)
            {
                *dat = ((uint16_t)frame_buf[frame_head_index + 8] << 8) + frame_buf[frame_head_index + 9];
            }
            else
            {
                /* 帧错误 */
                rt_kprintf("[unpack] EFRAME: dat_len byte=%u not 1/2\n", frame_buf[frame_head_index + 7]);
                return ATK_MS53L1M_EFRAME;
            }

            rt_kprintf("[unpack] EOK: *dat=0x%04X (id=%u)\n", *dat, *dat);
            return ATK_MS53L1M_EOK;
        }
        else
        {
            /* CRC 错误 */
            rt_kprintf("[unpack] ECRC: calc=0x%04X recv=0x%04X\n", frame_check_sum, check_sum);
            return ATK_MS53L1M_ECRC;
        }
    }
    else if (opt_type == 0x01)
    {
        frame_check_sum = atk_ms53l1m_crc_check_sum(&frame_buf[frame_head_index], 6);    /* 计算 CRC 校验和 */
        check_sum = ((uint16_t)frame_buf[frame_head_index + 6] << 8) + frame_buf[frame_head_index + 7];
        if (frame_check_sum == check_sum)
        {
            rt_kprintf("[unpack] EOK (write ack)\n");
            return ATK_MS53L1M_EOK;
        }
        else
        {
            /* CRC 错误 */
            rt_kprintf("[unpack] ECRC (write): calc=0x%04X recv=0x%04X\n", frame_check_sum, check_sum);
            return ATK_MS53L1M_ECRC;
        }
    }
    else if (opt_type == 0xFF)
    {
        if ((frame_buf[frame_head_index + 2] == 0xFF) && (frame_buf[frame_head_index + 3] == 0xFF))
        {
            frame_check_sum = atk_ms53l1m_crc_check_sum(&frame_buf[frame_head_index], 6);   /* 计算 CRC 校验和 */
            check_sum = ((uint16_t)frame_buf[frame_head_index + 6] << 8) + frame_buf[frame_head_index + 7];
            if (frame_check_sum == check_sum)
            {
                /* 异常操作 */
                rt_kprintf("[unpack] EOPT: module returned 0xFF (addr/funcode unsupported)\n");
                return ATK_MS53L1M_EOPT;
            }
            else
            {
                /* CRC 错误 */
                rt_kprintf("[unpack] ECRC (opt): calc=0x%04X recv=0x%04X\n", frame_check_sum, check_sum);
                return ATK_MS53L1M_ECRC;
            }
        }
        else
        {
            /* 帧异常 */
            rt_kprintf("[unpack] EFRAME: opt_type=0xFF but addr!=0xFFFF\n");
            return ATK_MS53L1M_EFRAME;
        }
    }
    else
    {
        /* 帧异常 */
        rt_kprintf("[unpack] EFRAME: unknown opt_type=0x%02X\n", opt_type);
        return ATK_MS53L1M_EFRAME;
    }
}

/**
 * @brief       通过模块功能码读取数据
 * @param       addr    : 设备地址
 *              fun_code: 功能码
 *              len     : 数据长度,取值范围:1~2
 *              dat     : 读取到的数据
 * @retval      ATK_MS53L1M_EOK     : 没有错误
 *              ATK_MS53L1M_ETIMEOUT: 接收数据超时
 *              ATK_MS53L1M_EFRAME  : 帧错误
 *              ATK_MS53L1M_ECRC    : CRC 校验错误
 *              ATK_MS53L1M_EOPT    : 操作错误
 */
uint8_t atk_ms53l1m_read_data(uint16_t addr, uint8_t fun_code, uint8_t len, uint16_t *dat)
{
    uint8_t ret;
    uint16_t check_sum;
    uint8_t buf[9];

    buf[0] = ATK_MS53L1M_MASTER_FRAME_HEAD;         /* 标志头 */
    buf[1] = ATK_MS53L1M_SENSOR_TYPE;               /* 传感器类型 */
    buf[2] = (uint8_t)(addr >> 8);                  /* 设备地址高 8 位 */
    buf[3] = (uint8_t)(addr & 0xFF);                /* 设备地址低 8 位 */
    buf[4] = ATK_MS53L1M_OPT_READ;                  /* 读操作 */
    buf[5] = fun_code;                              /* 功能码 */
    buf[6] = len;                                   /* 数据长度 */

    check_sum = atk_ms53l1m_crc_check_sum(buf, 7);  /* 计算 CRC 校验和 */

    buf[7] = (uint8_t)(check_sum >> 8);             /* CRC 校验码高 8 位 */
    buf[8] = (uint8_t)(check_sum & 0xFF);           /* CRC 校验码低 8 位 */

    atk_ms53l1m_uart_rx_restart();                  /* 准备重新开始接收新的一帧数据 */
    g_tof_trig_ms = rt_tick_get_millisecond();      /* Time_Stamp Phase③: 请求发出, 模块积分即将开始 */
    atk_ms53l1m_uart_send(buf, 9);                  /* 发送数据 */
    ret = atk_ms53l1m_unpack_recv_data(dat);        /* 解析应答数据 */
    atk_ms53l1m_interval_snapshot();                /* 结算本帧 [t_trig, t_first] 曝光区间 */

    return ret;
}

/* Time_Stamp Phase③: 曝光区间结算。
 * t_trig  = 请求发出时刻 (read_data 内打点);
 * t_first = 应答首字节到达时刻 (uart 层打点, 曝光刚结束的最近信号, 轮询粒度 ≤1 tick);
 * 区间中点 (t_trig+t_first)/2 即 TOF 观测时刻的最佳估计 (替代原"读取完成时刻",
 * 后者偏晚 ~560ms 且把整个事务延迟计入配对偏差) */
static void atk_ms53l1m_interval_snapshot(void)
{
    uint32_t first = atk_ms53l1m_uart_first_rx_ms();

    g_tof_first_ms = (first != 0 && first >= g_tof_trig_ms)
                         ? first
                         : rt_tick_get_millisecond(); /* 无应答字节: 退化为读完成时刻 */
}

/* 最近一次 read_data 的曝光区间 [trig_ms, first_ms] (alg 层配对用) */
void atk_ms53l1m_get_last_interval(rt_uint32_t *trig_ms, rt_uint32_t *first_ms)
{
    if (trig_ms != NULL)  *trig_ms  = g_tof_trig_ms;
    if (first_ms != NULL) *first_ms = g_tof_first_ms;
}

/**
 * @brief       通过模块功能码写入 1 字节数据
 * @param       addr    : 设备地址
 *              fun_code: 功能码
 *              dat     : 待写入的 1 字节数据
 * @retval      ATK_MS53L1M_EOK     : 没有错误
 *              ATK_MS53L1M_ETIMEOUT: 接收数据超时
 *              ATK_MS53L1M_EFRAME  : 帧错误
 *              ATK_MS53L1M_ECRC    : CRC 校验错误
 *              ATK_MS53L1M_EOPT    : 操作错误
 */
uint8_t atk_ms53l1m_write_data(uint16_t addr, uint8_t fun_code, uint8_t dat)
{
    uint8_t ret;
    uint8_t buf[10];
    uint16_t check_sum;

    buf[0] = ATK_MS53L1M_MASTER_FRAME_HEAD;         /* 标志头 */
    buf[1] = ATK_MS53L1M_SENSOR_TYPE;               /* 传感器类型 */
    buf[2] = (uint8_t)(addr >> 8);                  /* 设备地址高 8 位 */
    buf[3] = (uint8_t)(addr & 0xFF);                /* 设备地址低 8 位 */
    buf[4] = ATK_MS53L1M_OPT_WRITE;                 /* 写操作 */
    buf[5] = fun_code;                              /* 功能码 */
    buf[6] = 0x01;                                  /* 数据长度 */
    buf[7] = dat;                                   /* 数据 */

    check_sum = atk_ms53l1m_crc_check_sum(buf, 8);  /* 计算 CRC 校验和 */

    buf[8] = (uint8_t)(check_sum >> 8);             /* CRC 校验码高 8 位 */
    buf[9] = (uint8_t)(check_sum & 0xFF);           /* CRC 校验码低 8 位 */

    atk_ms53l1m_uart_rx_restart();                  /* 准备重新开始接收新的一帧数据 */
    atk_ms53l1m_uart_send(buf, 10);                 /* 发送数据 */
    ret = atk_ms53l1m_unpack_recv_data(NULL);       /* 解析应答数据 */

    return ret;
}

/**
 * @brief       通过模块功能码写入 2 字节数据 (大端序)
 * @param       addr    : 设备地址
 *              fun_code: 功能码
 *              dat     : 待写入的 2 字节数据 (取值需在功能码对应范围内)
 * @retval      ATK_MS53L1M_EOK     : 没有错误
 *              ATK_MS53L1M_ETIMEOUT: 接收数据超时
 *              ATK_MS53L1M_EFRAME  : 帧错误
 *              ATK_MS53L1M_ECRC    : CRC 校验错误
 *              ATK_MS53L1M_EOPT    : 操作错误
 *
 * @note        用于功能码 0x0B (定时预算) 等需 2 字节数据的写入场景。
 *              帧结构: [51 0B addr_H addr_L 01 fun_code 02 data_H data_L CRC_H CRC_L]
 */
uint8_t atk_ms53l1m_write_data16(uint16_t addr, uint8_t fun_code, uint16_t dat)
{
    uint8_t ret;
    uint8_t buf[11];
    uint16_t check_sum;

    buf[0] = ATK_MS53L1M_MASTER_FRAME_HEAD;         /* 标志头 */
    buf[1] = ATK_MS53L1M_SENSOR_TYPE;               /* 传感器类型 */
    buf[2] = (uint8_t)(addr >> 8);                  /* 设备地址高 8 位 */
    buf[3] = (uint8_t)(addr & 0xFF);                /* 设备地址低 8 位 */
    buf[4] = ATK_MS53L1M_OPT_WRITE;                 /* 写操作 */
    buf[5] = fun_code;                              /* 功能码 */
    buf[6] = 0x02;                                  /* 数据长度: 2 bytes */
    buf[7] = (uint8_t)(dat >> 8);                   /* 数据高字节 (big-endian) */
    buf[8] = (uint8_t)(dat & 0xFF);                 /* 数据低字节 */

    check_sum = atk_ms53l1m_crc_check_sum(buf, 9);  /* 计算 CRC 校验和 */

    buf[9] = (uint8_t)(check_sum >> 8);             /* CRC 校验码高 8 位 */
    buf[10] = (uint8_t)(check_sum & 0xFF);          /* CRC 校验码低 8 位 */

    atk_ms53l1m_uart_rx_restart();                  /* 准备重新开始接收新的一帧数据 */
    atk_ms53l1m_uart_send(buf, 11);                 /* 发送数据 */
    ret = atk_ms53l1m_unpack_recv_data(NULL);       /* 解析应答数据 */

    return ret;
}

/**
 * @brief       ATK-MS53L1M 初始化
 * @param       baudrate: ATK-MS53L1M UART 通信波特率
 *              id      : ATK-MS53L1M 设备 ID
 * @retval      ATK_MS53L1M_EOK  : ATK-MS53L1M 初始化成功,后续执行成功
 *              ATK_MS53L1M_ERROR: ATK-MS53L1M 初始化失败,后续执行失败
 */
uint8_t atk_ms53l1m_init(uint32_t baudrate, uint16_t *id)
{
    uint8_t i;
    uint8_t ret;

    /* ATK-MS53L1M UART 初始化 */
    atk_ms53l1m_uart_init(baudrate);
    rt_kprintf("[init] uart ready, baud=%lu\n", (unsigned long)baudrate);

    /* 读取设备地址 */
    i = 0;
    while (1)
    {
        ret = atk_ms53l1m_read_data(0xFFFF, ATK_MS53L1M_FUNCODE_IDSET, 2, id);
        if (ret == ATK_MS53L1M_EOK) break;
        rt_kprintf("[init] read_id attempt %u/5 failed (ret=0x%02X)\n", i + 1, ret);
        rt_thread_mdelay(100);
        if (++i == 5)
        {
            rt_kprintf("[init] give up: cannot read module ID\n");
            return ATK_MS53L1M_ERROR;
        }
    }
    rt_kprintf("[init] got id=0x%04X (%u)\n", *id, *id);

    /* 设置 ATK-MS53L1M 模块的工作模式为 Modbus 模式 */
    i = 0;
    while (1)
    {
        ret = atk_ms53l1m_write_data(*id, ATK_MS53L1M_FUNCODE_WORKMODE, ATK_MS53L1M_WORKMODE_MODBUS);
        if (ret == ATK_MS53L1M_EOK) break;
        rt_kprintf("[init] set_workmode attempt %u/5 failed (ret=0x%02X)\n", i + 1, ret);
        rt_thread_mdelay(100);
        if (++i == 5)
        {
            rt_kprintf("[init] give up: cannot set workmode\n");
            return ATK_MS53L1M_ERROR;
        }
    }
    rt_kprintf("[init] workmode set OK\n");

    return ATK_MS53L1M_EOK;
}

/**
 * @brief       ATK-MS53L1M Normal 工作模式获取测量值
 * @param       dat: 获取的测量值
 * @retval      ATK_MS53L1M_EOK  : 获取测量值成功
 *              ATK_MS53L1M_ERROR: UART 未接收到数据,获取测量值失败
 */
uint8_t atk_ms53l1m_normal_get_data(uint16_t *dat)
{
    uint8_t *buf = NULL;
    uint8_t i = 0;
    char *p;
    uint16_t dat_tmp = 0;

    atk_ms53l1m_uart_rx_restart();
    while (buf == NULL)
    {
        buf = atk_ms53l1m_uart_rx_get_frame();
        if (++i == 10)
        {
            return ATK_MS53L1M_ERROR;
        }
        rt_thread_mdelay(100);
    }

    p = strstr((char *)buf, "d:");
    while (*p != 'm')
    {
        if (*p >= '0' && *p <= '9')
        {
            dat_tmp = dat_tmp * 10 + (*p - '0');
        }
        p++;
    }

    *dat = dat_tmp;

    return ATK_MS53L1M_EOK;
}

/**
 * @brief       ATK-MS53L1M Modbus 工作模式获取测量值
 * @param       dat: 获取的测量值
 * @retval      ATK_MS53L1M_EOK  : 获取测量值成功
 *              ATK_MS53L1M_ERROR: UART 未接收到数据,获取测量值失败
 */
uint8_t atk_ms53l1m_modbus_get_data(uint16_t id, uint16_t *dat)
{
    uint8_t ret;
    uint16_t dat_tmp;

    ret = atk_ms53l1m_read_data(id, ATK_MS53L1M_FUNCODE_MEAUDATA, 2, &dat_tmp);
    if (ret != 0)
    {
        *dat = 0;
        return ATK_MS53L1M_ERROR;
    }
    else
    {
        *dat = dat_tmp;
        return ATK_MS53L1M_EOK;
    }
}
