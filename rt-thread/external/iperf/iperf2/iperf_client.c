/**
 * iperf-liked network performance tool
 *
 * usage: iperf("serverip", port)
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
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include "iperf2.h"

#define IPERF_PORT 5001

static char send_buf[1500];
static IPERF_PARAM param;
static rt_thread_t tid = RT_NULL;
static volatile int g_exit_req = 0;

static void tcpsend_entry(char *serverip, int port)
{
    char c;
    int i;
    int fd;
    int ret;
    int sentlen;
    float f;
    rt_tick_t tick1, tick2;
    struct sockaddr_in addr;
    int sndbuf = (1 << 16) - 1;

    c = 1;
    for (i = 0; i < sizeof(send_buf); i++)
    {
        send_buf[i] = c++;
    }

    while (1)
    {
        fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (fd == -1)
        {
            printf("create socket failed!\n");
            rt_thread_delay(RT_TICK_PER_SECOND);
            continue;
        }

        /* 增大 TCP_SND_BUF 提升iperf tcp client吞吐量 */
        if (setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &sndbuf, sizeof(sndbuf)) < 0)
        {
            printf("Error: setsockopt(SO_SNDBUF, &sndbuf %d) failed\n", sndbuf);
        }

        addr.sin_family      = PF_INET;
        addr.sin_port        = htons(port);
        addr.sin_addr.s_addr = inet_addr((char *)serverip);

        ret = connect(fd, (const struct sockaddr *)&addr, sizeof(addr));
        if (ret == -1)
        {
            printf("connect failed!\n");

            close(fd);

            if (g_exit_req)
            {
                g_exit_req = 0;
                break;
            }
            rt_thread_delay(RT_TICK_PER_SECOND);
            continue;
        }

        printf("connect to iperf server success!\n");

        sentlen = 0;
        tick1   = rt_tick_get();
        while (1)
        {
            tick2 = rt_tick_get();
            if (tick2 - tick1 >= RT_TICK_PER_SECOND * 2)
            {
                f = sentlen / 125 * RT_TICK_PER_SECOND / (tick2 - tick1);
                f /= 1000;
                printf("%2.2f Mbps!\n", f);
                tick1   = tick2;
                sentlen = 0;
            }

            ret = send(fd, send_buf, sizeof(send_buf), 0);
            if (ret > 0)
            {
                sentlen += ret;
            }

            if (ret <= 0)
            {
                break;
            }

            if (g_exit_req)
            {
                break;
            }
        }
        close(fd);

        if (g_exit_req)
        {
            g_exit_req = 0;
            break;
        }

        rt_thread_delay(RT_TICK_PER_SECOND * 2);

        printf("disconnect!\n");
    }
}

void iperf_proc(void *arg)
{
    IPERF_PARAM *param = (IPERF_PARAM *)arg;

    tcpsend_entry(param->c_Host, param->p_port);
}

int iperf(char *serverip, int port)
{
    rt_err_t result = RT_EOK;
    if (port && tid == RT_NULL)
    {
        strcpy(param.c_Host, serverip);
        param.p_port = port;

        tid = rt_thread_create("tiperf", iperf_proc, &param, 4096, RT_LWIP_TCPTHREAD_PRIORITY + 30, 10);
        if (tid != RT_NULL)
        {
            result = rt_thread_startup(tid);
            RT_ASSERT(result == RT_EOK);
            return result;
        }
        else
        {
            return -1;
        }
    }
    else
    {
        if (tid != RT_NULL)
        {
            g_exit_req = 1;
            while (g_exit_req)
            {
                rt_thread_delay(10);
            }

            tid = RT_NULL;
        }
        return 0;
    }
}

#ifdef RT_USING_FINSH
#include <finsh.h>
FINSH_FUNCTION_EXPORT(iperf, iperf("serverip", port));
#endif

#ifdef FINSH_USING_MSH
static int atof_rate(const char *s)
{
    double    n;
    char      suffix = '\0';

    if (s == NULL)
    {
        printf("wrong rate...\n");
        return -1;
    }

    /* scan the number and any suffices */
    sscanf(s, "%lf%c", &n, &suffix);
    return n;
}/* end unit_atof_rate */

static void iperf2_help(void)
{
  printf("Usage: iperf2 [-s|-c host] [options]\n"
         "Server or Client:\n"
         "  -p, --port      #         server port to listen on/connect to\n"
         "  -f, --format(not support)    [kmgKMG]  format to report: Kbits, Mbits, KBytes, MBytes\n"
         "  -i, --interval(not support)  #         seconds between periodic bandwidth reports\n"
         "  -c, --client    <host>    run in client mode, connecting to <host>\n"
         "  -s, --server              run in server mode\n"
         "  -h, --help                show this message and quit\n"
         "  -u, --udp                 use UDP rather than TCP\n"
         "  -t, --time(udp only)      #         time in seconds to transmit for (default 10 secs)\n"
         "  -b, --bandwidth(udp only) #[KMG][/#] target bandwidth in bits/sec (0 for unlimited)\n"
         "                            (default 1 Mbit/sec for UDP, unlimited for TCP)\n"
         "                            (optional slash and packet count for burst mode)\n"
         "  -l, --len(udp only)       #[KMG]    length of buffer to read or write\n"
         "                            (default 128 KB for TCP, 8 KB for UDP)\n");
}

extern void iperf_udp_s(rt_uint32_t startup);
extern int iperf_server(int port);
extern void iperf_udp_c(char *c_Host, ...);

static void set_default_iperf(IPERF_PARAM *parm)
{
    if (parm)
    {
        parm->p_port = IPERF_PORT;
        parm->t_second = 10;
        parm->b_mps = 1;
        parm->l_len = 1470;
        parm->i_intvl = 1;
    }
}

#include <optparse.h>
static struct optparse options;
int iperf2(int argc, char **argv)
{
    int c;
    int server = -1;
    static IPERF_PARAM g_iperf2_parm;
    int is_udp = 0;
    static int tcp_flag = 0;
    static int udp_flag = 0;

    memset(&g_iperf2_parm, 0, sizeof(g_iperf2_parm));
    set_default_iperf(&g_iperf2_parm);

    optparse_init(&options, argv);
    while ((c = optparse(&options, "usc:f:i:p:t:b:l:")) != -1)
    {
        switch (c)
        {
            case 's':
                server = 1;
                break;
            case 'u':
                is_udp = 1;
                break;
            case 'c':
                server = 0;
                if (*options.optarg)
                {
                    if (strlen(options.optarg) < 7)
                    {
                        printf("bad parameter! Wrong host ip\n");
                        return -1;
                    }
                    strcpy(g_iperf2_parm.c_Host, options.optarg);
                }
                break;
            case 'f':
                break;
            case 'p':
                if (*options.optarg)
                {
                    g_iperf2_parm.p_port = atoi(options.optarg);
                }
                break;
            case 't':
                if (*options.optarg)
                {
                    g_iperf2_parm.t_second = atoi(options.optarg);
                }
                break;
            case 'b':
                if (*options.optarg)
                {
                    g_iperf2_parm.b_mps = atof_rate(options.optarg);
                    if (g_iperf2_parm.b_mps == -1)
                    {
                        printf("bad parameter! iperf2 -s/c [-u] -f m -i 1 -p 10000 [-b 100m] [-t 30]\n");
                        return -1;
                    }
                }
                break;
            case 'l':
                if (*options.optarg)
                {
                    g_iperf2_parm.l_len = atoi(options.optarg);
                }
                break;
            case 'i':
                if (*options.optarg)
                {
                    g_iperf2_parm.i_intvl = atoi(options.optarg);
                }
                break;
            case 'h':
            default:
                iperf2_help();
                return 0;
        }
    }

    if (server == 1)
    {
        if (is_udp)
        {
            if (!udp_flag)
            {
                printf("----------------------------------------\n");
                printf("Server listening on UDP port 10000\n");
                printf("----------------------------------------\n");
                udp_flag = 1;
            }
            else
            {
                udp_flag = 0;
                printf("Exit UDP server!\n");
            }
            iperf_udp_s(1);
        }
        else
        {
            if (!tcp_flag)
            {
                printf("----------------------------------------\n");
                printf("Server listening on TCP port %d\n", g_iperf2_parm.p_port);
                printf("----------------------------------------\n");
                tcp_flag = 1;
            }
            else
            {
                tcp_flag = 0;
                printf("Exit TCP server!\n");
            }
            iperf_server(g_iperf2_parm.p_port);
        }
    }
    else if (server == 0)
    {
        if (is_udp)
        {
            iperf_udp_c(g_iperf2_parm.c_Host, g_iperf2_parm.p_port, g_iperf2_parm.t_second,
                         g_iperf2_parm.b_mps, g_iperf2_parm.l_len, g_iperf2_parm.i_intvl);
        }
        else
        {
            iperf(g_iperf2_parm.c_Host, g_iperf2_parm.p_port);
        }
    }
    else
    {
        iperf2_help();
    }

    return 0;
}
MSH_CMD_EXPORT(iperf2, iperf2());
#endif
