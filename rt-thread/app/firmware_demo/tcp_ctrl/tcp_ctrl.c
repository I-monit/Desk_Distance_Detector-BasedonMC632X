/**
 ****************************************************************************************************
 * @file        tcp_ctrl.c
 * @brief       TCP 控制通道实现 (板端 PC-Qt GUI 适配, Qt_GUI 分支)
 *
 * 实现: 单线程 select 循环 (参照 components/libdbi_over_tcp_rtt 与 librtsp 的 lwIP 套接字用法):
 *   - listen fd + client fd 同时 select, 100ms 超时保证 stop 命令与 200ms 推送节拍
 *   - 单客户端策略: 新连接进来时踢掉旧连接 (last-wins), Qt 重连即恢复
 *   - 推送前不阻塞: client 套接字设 TCP_NODELAY + SO_SNDTIMEO(1s), 发送失败即断开,
 *     避免 PC 端卡死时拖住线程 (与 RTSP 560 推流线程互不影响)
 *   - 命令按行解析 ('\n' 结尾, 兼容 '\r\n')
 *
 * 数据源: alg_get_info() (nna_app/algorithm.c), 与 msh alg_state 同源 (原 LVGL lvgl_alg_ui.c 已随板端 UI 移除)
 * 按键注入: key_push_event(KEY1_PRES) (tof/KEY/key.c), 后续防抖/标定窗口逻辑由 alg_glue 复用
 ****************************************************************************************************
 */

#include <rtthread.h>
#include <rttshell.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

#include "tcp_ctrl.h"
#include "KEY/key.h"
#include "nna_app/include/algorithm.h"
#include "clk_bridge.h"

#define TCP_CTRL_THREAD_STACK 8192
#define TCP_CTRL_THREAD_PRIO  25
#define TCP_CTRL_PUSH_MS      100   /* 状态推送周期 10Hz: 降低D值陈旧度, 供PC帧级配对 */
#define TCP_CTRL_SELECT_MS    100   /* select 超时: 兼顾 stop 响应与推送节拍 */
#define TCP_CTRL_SNDTIMEO_MS  1000  /* 发送超时: PC 端僵死时丢弃本连接 */
#define TCP_CTRL_RXBUF_SIZE   256
#define TCP_CTRL_LINE_MAX     32
#define TCP_CTRL_TXBUF_SIZE   256  /* 状态 JSON 含 ts/fid/pts/ats 后最长约 150 字节, 留余量防截断 */

static rt_thread_t g_tcp_ctrl_thread = RT_NULL;
static volatile int g_tcp_ctrl_stop  = 0;
static int g_tcp_ctrl_port           = TCP_CTRL_DEFAULT_PORT;
static volatile rt_uint32_t g_tcp_ctrl_hb = 0;  /* 心跳: select 循环每轮 +1, guard 冻结检测用 */

/* 仅服务线程内访问 */
static int g_server_fd = -1;
static int g_client_fd = -1;
static char g_rxbuf[TCP_CTRL_RXBUF_SIZE];
static int  g_rxlen = 0;

/* ============================ JSON 状态组包 ============================ */

static const char *alg_state_name(ALG_STATE state)
{
    switch (state)
    {
    case ALG_STATE_READY:      return "READY";
    case ALG_STATE_CALIBRATING:return "CALIBRATING";
    case ALG_STATE_IDLE:
    default:                   return "IDLE";
    }
}

static void build_status_json(char *buf, int size)
{
    ALG_INFO info;
    FH_UINT64 anchor_venc = 0;
    FH_UINT64 anchor_rt   = 0;

    clk_bridge_get(&anchor_venc, &anchor_rt); /* 无采样过时保持 0, PC 端按无效处理 */

    if (alg_get_info(&info) != ALG_OK)
    {
        snprintf(buf, size,
                 "{\"state\":\"IDLE\",\"D\":-1,\"C\":-1,\"W\":-1,\"faces\":0,\"pairs\":0,\"nn\":0,\"tof\":0,"
                 "\"ts\":0,\"fid\":0,\"tts\":0,\"tfid\":0,\"tdev\":0,\"twin\":0,"
                 "\"pts\":%llu,\"ats\":%llu}\n",
                 (unsigned long long)anchor_venc, (unsigned long long)anchor_rt);
        return;
    }

    if (info.state == ALG_STATE_READY)
    {
        /* READY: D=实时距离mm, C=针孔常数mm*px, W=最近一次人脸像素宽度;
         * faces=最近一帧NNA检测目标数, PC端 faces != 1 判定 D/W 陈旧无效 (显示null);
         * ts/fid = D 对应模型帧的采集时刻(Time_Stamp Phase②)与帧号, pts/ats = 时钟桥锚点对
         * (PC 端: 帧PTS -> rt_of(pts)=ats+(pts_frame-pts), 与 ts 比较即完成帧级配对);
         * tts/tfid/tdev/twin = 标定期最近一次配对质量 (READY 期清 0) */
        snprintf(buf, size,
                 "{\"state\":\"%s\",\"D\":%d,\"C\":%d,\"W\":%d,\"faces\":%u,"
                 "\"pairs\":%u,\"nn\":%u,\"tof\":%u,"
                 "\"ts\":%llu,\"fid\":%llu,\"tts\":0,\"tfid\":0,\"tdev\":0,\"twin\":0,"
                 "\"pts\":%llu,\"ats\":%llu}\n",
                 alg_state_name(info.state),
                 (int)(info.distance_mm + 0.5f),
                 (int)(info.k + 0.5f),
                 (int)(info.face_width_px + 0.5f),
                 (unsigned)info.face_num,
                 (unsigned)info.pair_num,
                 (unsigned)info.nn_samples,
                 (unsigned)info.tof_samples,
                 (unsigned long long)info.update_ts,
                 (unsigned long long)info.update_fid,
                 (unsigned long long)anchor_venc,
                 (unsigned long long)anchor_rt);
    }
    else if (info.state == ALG_STATE_CALIBRATING)
    {
        /* CALIBRATING: 数值字段无效填 -1, faces 仍实时上报, pairs/nn/tof 供 Qt 端显示配对进度;
         * tts/tfid/tdev/twin = Time_Stamp Phase④ 最近一次成功配对的观测质量
         * (tdev = |NN采集时刻-TOF曝光中点|, PC 端据此显示配对可信度) */
        snprintf(buf, size,
                 "{\"state\":\"%s\",\"D\":-1,\"C\":-1,\"W\":-1,\"faces\":%u,"
                 "\"pairs\":%u,\"nn\":%u,\"tof\":%u,"
                 "\"ts\":0,\"fid\":0,"
                 "\"tts\":%llu,\"tfid\":%llu,\"tdev\":%llu,\"twin\":%u,"
                 "\"pts\":%llu,\"ats\":%llu}\n",
                 alg_state_name(info.state),
                 (unsigned)info.face_num,
                 (unsigned)info.pair_num,
                 (unsigned)info.nn_samples,
                 (unsigned)info.tof_samples,
                 (unsigned long long)info.pair_last_tts,
                 (unsigned long long)info.pair_last_tfid,
                 (unsigned long long)info.pair_last_tdev,
                 (unsigned)info.pair_last_win,
                 (unsigned long long)anchor_venc,
                 (unsigned long long)anchor_rt);
    }
    else
    {
        snprintf(buf, size,
                 "{\"state\":\"%s\",\"D\":-1,\"C\":-1,\"W\":-1,\"faces\":%u,"
                 "\"pairs\":0,\"nn\":0,\"tof\":0,"
                 "\"ts\":0,\"fid\":0,\"tts\":0,\"tfid\":0,\"tdev\":0,\"twin\":0,"
                 "\"pts\":%llu,\"ats\":%llu}\n",
                 alg_state_name(info.state),
                 (unsigned)info.face_num,
                 (unsigned long long)anchor_venc,
                 (unsigned long long)anchor_rt);
    }
}

/* ============================ 套接字辅助 ============================ */

static int send_all(int fd, const char *buf, int len)
{
    int sent = 0;

    while (sent < len)
    {
        int n = send(fd, buf + sent, len - sent, 0);

        if (n <= 0)
        {
            return -1;   /* 含 SO_SNDTIMEO 超时, 由调用方断开 */
        }
        sent += n;
    }
    return 0;
}

static void drop_client(const char *reason)
{
    if (g_client_fd >= 0)
    {
        close(g_client_fd);
        g_client_fd = -1;
        g_rxlen     = 0;
        rt_kprintf("[tcp_ctrl] client dropped: %s\n", reason);
    }
}

static void accept_new_client(void)
{
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);
    int fd = accept(g_server_fd, (struct sockaddr *)&addr, &addr_len);
    int on = 1;
    struct timeval tv;

    if (fd < 0)
    {
        return;
    }

    if (g_client_fd >= 0)
    {
        drop_client("replaced by new client");   /* 单客户端, last-wins */
    }

    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &on, sizeof(on));
    tv.tv_sec  = TCP_CTRL_SNDTIMEO_MS / 1000;
    tv.tv_usec = (TCP_CTRL_SNDTIMEO_MS % 1000) * 1000;
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

    g_client_fd = fd;
    rt_kprintf("[tcp_ctrl] client connected: %s:%d\n",
               inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));

    /* 连接建立立即推一帧, Qt 端无需等满 200ms */
    {
        char buf[TCP_CTRL_TXBUF_SIZE];

        build_status_json(buf, sizeof(buf));
        if (send_all(fd, buf, strlen(buf)) < 0)
        {
            drop_client("send welcome failed");
        }
    }
}

/* ============================ 命令解析 (按行) ============================ */

static void handle_line(const char *line)
{
    if (!strcmp(line, "KEY1"))
    {
        key_push_event(KEY1_PRES);   /* 注入 KEY1 事件 (与 msh alg_key 等价), 后续流程由 alg_glue 线程处理 */
        rt_kprintf("[tcp_ctrl] KEY1 injected (from Qt)\n");
    }
    else if (!strcmp(line, "PING"))
    {
        if (send_all(g_client_fd, "PONG\n", 5) < 0)
        {
            drop_client("send pong failed");
        }
    }
    else if (line[0] != '\0')
    {
        rt_kprintf("[tcp_ctrl] unknown cmd: \"%s\"\n", line);
    }
}

static void recv_from_client(void)
{
    int n = recv(g_client_fd, g_rxbuf + g_rxlen, TCP_CTRL_RXBUF_SIZE - 1 - g_rxlen, 0);

    if (n == 0)
    {
        drop_client("peer closed");
        return;
    }
    if (n < 0)
    {
        drop_client("recv error");
        return;
    }

    g_rxlen += n;
    g_rxbuf[g_rxlen] = '\0';

    /* 逐行拆出处理, 剩余不完整行留在缓冲区 */
    {
        char *start = g_rxbuf;
        char *nl;

        while ((nl = strchr(start, '\n')) != RT_NULL)
        {
            char line[TCP_CTRL_LINE_MAX];
            int len = (int)(nl - start);

            if (len > 0 && start[len - 1] == '\r')
            {
                len--;
            }
            if (len >= TCP_CTRL_LINE_MAX)
            {
                len = TCP_CTRL_LINE_MAX - 1;
            }
            memcpy(line, start, len);
            line[len] = '\0';
            handle_line(line);
            start = nl + 1;
        }

        g_rxlen = (int)(g_rxbuf + g_rxlen - start);
        memmove(g_rxbuf, start, g_rxlen);

        if (g_rxlen >= TCP_CTRL_RXBUF_SIZE - 1)
        {
            drop_client("rx overflow");   /* 无换行的垃圾数据, 防御性断开 */
        }
    }
}

/* ============================ 服务线程 ============================ */

static void tcp_ctrl_thread_entry(void *param)
{
    struct timeval tv;
    rt_int64_t last_push = 0;
    char txbuf[TCP_CTRL_TXBUF_SIZE];

    (void)param;

    rt_kprintf("[tcp_ctrl] server started on port %d\n", g_tcp_ctrl_port);

    while (!g_tcp_ctrl_stop)
    {
        g_tcp_ctrl_hb++;   /* 心跳: guard 冻结检测用 */

        fd_set rfds;
        int maxfd = -1;
        int ret;
        rt_int64_t now;

        FD_ZERO(&rfds);
        if (g_server_fd >= 0)
        {
            FD_SET(g_server_fd, &rfds);
            maxfd = g_server_fd;
        }
        if (g_client_fd >= 0)
        {
            FD_SET(g_client_fd, &rfds);
            if (g_client_fd > maxfd)
            {
                maxfd = g_client_fd;
            }
        }
        if (maxfd < 0)
        {
            break;
        }

        tv.tv_sec  = 0;
        tv.tv_usec = TCP_CTRL_SELECT_MS * 1000;
        ret = select(maxfd + 1, &rfds, RT_NULL, RT_NULL, &tv);
        if (ret > 0)
        {
            if (g_server_fd >= 0 && FD_ISSET(g_server_fd, &rfds))
            {
                accept_new_client();
            }
            if (g_client_fd >= 0 && FD_ISSET(g_client_fd, &rfds))
            {
                recv_from_client();
            }
        }

        now = rt_tick_get_millisecond();
        if (g_client_fd >= 0 && (now - last_push) >= TCP_CTRL_PUSH_MS)
        {
            last_push = now;
            build_status_json(txbuf, sizeof(txbuf));
            if (send_all(g_client_fd, txbuf, strlen(txbuf)) < 0)
            {
                drop_client("push timeout");
            }
        }
    }

    if (g_client_fd >= 0)
    {
        close(g_client_fd);
        g_client_fd = -1;
    }
    if (g_server_fd >= 0)
    {
        close(g_server_fd);
        g_server_fd = -1;
    }

    rt_kprintf("[tcp_ctrl] server stopped\n");
    g_tcp_ctrl_stop    = 0;
    g_tcp_ctrl_thread  = RT_NULL;   /* 允许重新 svc_qt 启动 */
}

/* ============================ 对外接口 / msh ============================ */

int tcp_ctrl_start(int port)
{
    struct sockaddr_in addr;
    int on = 1;

    if (g_tcp_ctrl_thread != RT_NULL)
    {
        rt_kprintf("[tcp_ctrl] already running\n");
        return 0;
    }

    g_server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (g_server_fd < 0)
    {
        rt_kprintf("[tcp_ctrl] create socket failed\n");
        return -1;
    }

    setsockopt(g_server_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    addr.sin_family      = AF_INET;
    addr.sin_port        = htons((unsigned short)port);
    addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(g_server_fd, (struct sockaddr *)&addr, sizeof(addr)) != 0)
    {
        rt_kprintf("[tcp_ctrl] bind port %d failed\n", port);
        close(g_server_fd);
        g_server_fd = -1;
        return -1;
    }
    if (listen(g_server_fd, 1) != 0)
    {
        rt_kprintf("[tcp_ctrl] listen failed\n");
        close(g_server_fd);
        g_server_fd = -1;
        return -1;
    }

    g_tcp_ctrl_port = port;
    g_tcp_ctrl_stop = 0;
    g_rxlen         = 0;

    g_tcp_ctrl_thread = rt_thread_create("tcp_ctrl", tcp_ctrl_thread_entry, RT_NULL,
                                         TCP_CTRL_THREAD_STACK, TCP_CTRL_THREAD_PRIO, 20);
    if (g_tcp_ctrl_thread == RT_NULL)
    {
        rt_kprintf("[tcp_ctrl] create thread failed\n");
        close(g_server_fd);
        g_server_fd = -1;
        return -1;
    }
    rt_thread_startup(g_tcp_ctrl_thread);
    return 0;
}

int tcp_ctrl_stop(void)
{
    int wait_ms;

    if (g_tcp_ctrl_thread == RT_NULL)
    {
        return 0;
    }

    g_tcp_ctrl_stop = 1;
    for (wait_ms = 0; wait_ms < 2000 && g_tcp_ctrl_thread != RT_NULL; wait_ms += 100)
    {
        rt_thread_mdelay(100);
    }

    if (g_tcp_ctrl_thread != RT_NULL)
    {
        rt_kprintf("[tcp_ctrl] stop timeout (select loop blocked?)\n");
        return -1;
    }
    return 0;
}

static void svc_qt(int argc, char *argv[])
{
    int port = TCP_CTRL_DEFAULT_PORT;

    if (argc >= 2)
    {
        if (!strcmp(argv[1], "stop"))
        {
            tcp_ctrl_stop();
            return;
        }
        port = (int)strtol(argv[1], NULL, 0);
        if (port <= 0 || port > 65535)
        {
            rt_kprintf("[tcp_ctrl] invalid port \"%s\"\n", argv[1]);
            return;
        }
    }

    tcp_ctrl_start(port);
}

SHELL_CMD_EXPORT(svc_qt, start tcp ctrl server for Qt GUI (svc_qt [port] | stop));

/* ---- 软件看门狗 (tof/guard/guard.c) 读取接口 ---- */
rt_bool_t tcp_ctrl_alive(void)   /* 线程句柄判活 (退出路径已置 NULL, tcp_ctrl.c:355) */
{
    return (g_tcp_ctrl_thread != RT_NULL) ? RT_TRUE : RT_FALSE;
}

rt_uint32_t tcp_ctrl_hb(void)
{
    return g_tcp_ctrl_hb;
}
