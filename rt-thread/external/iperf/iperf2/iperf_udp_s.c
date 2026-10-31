#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <rtthread.h>
#define IPERF_PORT 10000

#define RECV_BUF_SIZE       (0x1000)

void udprecv_entry(void *parameter)
{
    char c;
    int i;
    int fd;
    int ret;
    int recvlen = 0;
    float f;
    rt_tick_t tick1, tick2;
    struct sockaddr_in addr;
    socklen_t addrlen;
    char *recv_buf;

    recv_buf = rt_malloc(RECV_BUF_SIZE);
    c = 1;
    for (i = 0; i < RECV_BUF_SIZE; i++)
    {
        recv_buf[i] = c++;
    }

    fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (fd == -1)
    {
        printf("create socket failed!\n");
        return;
    }

    addr.sin_family      = PF_INET;
    addr.sin_port        = htons(IPERF_PORT);
    addr.sin_addr.s_addr = 0;

    ret = bind(fd, (const struct sockaddr *)&addr, sizeof(addr));

    tick1 = rt_tick_get();
    while (1)
    {
        tick2 = rt_tick_get();
        if (tick2 - tick1 >= RT_TICK_PER_SECOND)
        {
            f = recvlen * RT_TICK_PER_SECOND / 125 / (tick2 - tick1);
            f /= 1000;
            printf("%2.2f Mbps!\n", f);
            tick1   = tick2;
            recvlen = 0;
        }

        ret = recvfrom(fd, recv_buf, RECV_BUF_SIZE, 0,
                       (struct sockaddr *)&addr, &addrlen);

        if (ret <= 0)
        {
            rt_thread_delay(1);
            continue;
        }

        recvlen += ret;
    }
}

#ifdef RT_USING_FINSH
#include <finsh.h>
static rt_thread_t echo_tid = RT_NULL;
void iperf_udp_s(rt_uint32_t startup)
{
    if (startup && echo_tid == RT_NULL)
    {
        echo_tid =
            rt_thread_create("udprecv", udprecv_entry, RT_NULL, 4096, RT_LWIP_TCPTHREAD_PRIORITY + 30, 10);
        if (echo_tid != RT_NULL) rt_thread_startup(echo_tid);
    }
    else
    {
        if (echo_tid != RT_NULL) rt_thread_delete(echo_tid); /* delete thread */
        echo_tid = RT_NULL;
    }
}
FINSH_FUNCTION_EXPORT(iperf_udp_s, startup or stop UDP recv);
#endif
