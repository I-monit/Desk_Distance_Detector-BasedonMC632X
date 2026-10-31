/**
 ****************************************************************************************************
 * @file        tcp_ctrl.h
 * @brief       TCP 控制通道 (板端 PC-Qt GUI 适配, Qt_GUI 分支)
 *
 * 板端服务单客户端连接 (默认端口 6000):
 *   板->PC : 100ms 周期推送算法状态 JSON 一行 ('\n' 结尾, UTF-8)
 *            {"state":"READY","D":1234,"C":56789,"W":96,"faces":1,"pairs":3,"nn":12,"tof":8,
 *             "ts":123456,"fid":7734,"pts":234567,"ats":234500}
 *            state 取值 IDLE / CALIBRATING / READY; 当前状态无效的数值字段填 -1
 *            faces = 最近一帧 NNA 检测目标数; faces != 1 时 D/W 为陈旧数据, PC 端判无效显示 null
 *            ts/fid = D 对应模型帧的 rt_tick 时间戳(ms)与VPU帧号 (仅READY有效, 其余为0)
 *            pts/ats = VENC时钟(ms)<->rt_tick(ms) 时钟桥锚点对, 见 clk_bridge.h,
 *                      PC 端帧级配对: rt_of(帧PTS) = ats + (帧PTS_ms - pts)
 *   PC->板 : "KEY1\n" -> key_push_event(KEY1_PRES), 注入 KEY1 事件 (与 msh alg_key 等价), 触发标定/喂样本
 *            "PING\n" -> 应答 "PONG\n", 链路心跳
 *
 * msh 命令: svc_qt [port] 启动 (缺省 6000), svc_qt stop 停止
 * 启动顺序: ifconfig e0 ... -> svc_rtsp -p 554 -> svc_qt
 ****************************************************************************************************
 */

#ifndef __TCP_CTRL_H
#define __TCP_CTRL_H

#include <rtthread.h>

#define TCP_CTRL_DEFAULT_PORT 6000  /* svc_qt 缺省端口 (guard 重启亦用此端口) */

int tcp_ctrl_start(int port);   /* 创建服务线程 (幂等: 已在运行则提示并返回) */
int tcp_ctrl_stop(void);        /* 通知线程退出并等待释放 (幂等) */

/* 软件看门狗 (tof/guard/guard.c) 读取接口 */
rt_bool_t   tcp_ctrl_alive(void);  /* 服务线程存活 (句柄非空) */
rt_uint32_t tcp_ctrl_hb(void);     /* 心跳计数 (select 循环每轮 +1) */

#endif
