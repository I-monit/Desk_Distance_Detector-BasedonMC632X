/**
* iperf-liked network performance tool
*
* usage: iperf_server(int port)
*
*/

#include <rtdef.h>
#include <rtthread.h>
#include <rtdevice.h>
#include "fh_def.h"
#include "fh_arch.h"
#include "interrupt.h"
/* #include "tc_comm.h" */
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

static int iperf_port = 5001;
static char recv_buf[1500];
static rt_thread_t tid = RT_NULL;

static void tcprecv_entry()
{
    int fd, connected;
    int ret;
    int recv_len;
    float f;
    rt_tick_t tick1, tick2;
    struct sockaddr_in server_addr, client_addr;
   socklen_t sin_size;
   int port = iperf_port;

    while (1)
    {
        fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (fd == -1)
        {
            printf("create socket failed!\n");
            rt_thread_delay(RT_TICK_PER_SECOND);
            continue;
        }

        server_addr.sin_family      = AF_INET;
        server_addr.sin_port        = htons(port);
        server_addr.sin_addr.s_addr = INADDR_ANY;
        rt_memset(&(server_addr.sin_zero),8, sizeof(server_addr.sin_zero));
        /* 绑定socket到服务端地址 */
        if (bind(fd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1)
        {
            /* 绑定失败 */
            rt_kprintf("Unable to bind\n");
            close(fd);
            rt_thread_delay(RT_TICK_PER_SECOND);
            continue;
        }

        /* 在socket上进行监听 */
        if (listen(fd, 5) == -1)
        {
            rt_kprintf("Listen error\n");
            close(fd);
            rt_thread_delay(RT_TICK_PER_SECOND);
            continue;
        }

        sin_size = sizeof(struct sockaddr_in);
        while (1) {
            /* 接受一个客户端连接socket的请求，这个函数调用是阻塞式的 */
            connected = accept(fd, (struct sockaddr *)&client_addr, &sin_size);
            /* 返回的是连接成功的socket */

            /* 接受返回的client_addr指向了客户端的地址信息 */
            rt_kprintf("I got a connection from (%s , %d)\n",
                       inet_ntoa(client_addr.sin_addr),ntohs(client_addr.sin_port));

            recv_len = 0;
            tick1   = rt_tick_get();
            while (1)
            {
                tick2 = rt_tick_get();
                if (tick2 - tick1 >= RT_TICK_PER_SECOND * 2)
                {
                    f = recv_len / 125 * RT_TICK_PER_SECOND / (tick2 - tick1);
                    f /= 1000;
                    printf("%2.2f Mbps!\n", f);
                    tick1   = tick2;
                    recv_len = 0;
                }

                //ret = send(fd, send_buf, sizeof(send_buf), 0);
                ret = recv(connected, recv_buf, 1024, 0);
                if (ret > 0)
                {
                    recv_len += ret;
                }

                if (ret <= 0)
                {
                    break;
                }
            }
        }
        close(fd);
        break;
    }
    return;
}

int iperf_server(int port)
{
    rt_err_t result = RT_EOK;

    if (port && tid == RT_NULL)
    {
        iperf_port = port;
        tid = rt_thread_create("iperf_server", tcprecv_entry, NULL, 4096, RT_LWIP_TCPTHREAD_PRIORITY + 30, 10);
        if (tid != RT_NULL)
            result = rt_thread_startup(tid);
        RT_ASSERT(result == RT_EOK);
        return result;
    } else
    {
        if (tid != RT_NULL)
        {
            rt_thread_delete(tid); /* delete thread */
            tid = RT_NULL;
        }
        return 0;
    }
}

#ifdef RT_USING_FINSH
#include <finsh.h>
FINSH_FUNCTION_EXPORT(iperf_server, iperf_server(port));
#endif
