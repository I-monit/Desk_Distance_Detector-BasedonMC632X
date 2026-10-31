#include <rtthread.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <gpio.h>
#include <interrupt.h>
#include "delay.h"
#include "iperf2.h"

extern void do_gettimeofday(struct timeval *tv);
char iperf_udp_c_help[] = "Enter iperf_udp_c(c_Host, p_port, t_second, b_mps, l_len, i_intvl) to excute iperf -c test\n";

static IPERF_PARAM param;
static rt_thread_t tid = RT_NULL;

// used to reference the 4 byte ID number we place in UDP datagrams
// use int32_t if possible, otherwise a 32 bit bitfield (e.g. on J90)
typedef struct UDP_datagram {
#ifdef HAVE_INT32_T
    int32_t id;
    u_int32_t tv_sec;
    u_int32_t tv_usec;
#else
    signed   int id      : 32;
    unsigned int tv_sec  : 32;
    unsigned int tv_usec : 32;
#endif
} UDP_datagram;

/*
 * The client_hdr structure is sent from clients
 * to servers to alert them of things that need
 * to happen. Order must be perserved in all
 * future releases for backward compatibility.
 * 1.7 has flags, numThreads, mPort, and bufferlen
 */
typedef struct client_hdr {

#ifdef HAVE_INT32_T

    /*
     * flags is a bitmap for different options
     * the most significant bits are for determining
     * which information is available. So 1.7 uses
     * 0x80000000 and the next time information is added
     * the 1.7 bit will be set and 0x40000000 will be
     * set signifying additional information. If no
     * information bits are set then the header is ignored.
     * The lowest order diferentiates between dualtest and
     * tradeoff modes, wheither the speaker needs to start
     * immediately or after the audience finishes.
     */
    int32_t flags;
    int32_t numThreads;
    int32_t mPort;
    int32_t bufferlen;
    int32_t mWinBand;
    int32_t mAmount;
#else
    signed int flags      : 32;
    signed int numThreads : 32;
    signed int mPort      : 32;
    signed int bufferlen  : 32;
    signed int mWinBand   : 32;
    signed int mAmount    : 32;
#endif
} client_hdr;

void Settings_GenerateClientHdr(client_hdr *hdr, int t_second, int p_port, int b_mps, int l_len)
{
    long mAmount;

    hdr->flags  = 0;
    hdr->numThreads = 1;
    hdr->mPort  = p_port;

    hdr->bufferlen = l_len;

    hdr->mWinBand  = b_mps * 1000 * 1000;
    mAmount = t_second * 100.0;
    hdr->mAmount = htonl(-mAmount);
}

const double kSecs_to_usecs = 1e6;
const int kBytes_to_Bits = 8;

long subUsec(struct timeval left, struct timeval right)
{
    return (left.tv_sec - right.tv_sec) * kSecs_to_usecs + (left.tv_usec - right.tv_usec);
}


static void iperf_udp_entry(void *arg)
{
    char *c_Host;
    int p_port;
    int t_second;
    int b_mps;
    int l_len;
    RT_UNUSED int i_intvl;

    IPERF_PARAM *param = (IPERF_PARAM *)arg;
    /* int err; */

    c_Host = param->c_Host;
    p_port = param->p_port;
    t_second = param->t_second;
    b_mps = param->b_mps;
    l_len = param->l_len;
    i_intvl = param->i_intvl;

    int usdelay_tar = 0;
    int delay = 0;
    int adjust = 0;
    struct timeval now, last, lastPacketTime;

    int sock_id;
    struct sockaddr_in addr;
    /* char buf[] = "ABCDEFGHI"; */
    socklen_t socklen;
    int l_min, packetID = 0, ret;
    char* mBuf; /* sizeof(client_hdr) + sizeof(UDP_datagram) + payload l_len - sizeof(UDP_datagram) */
#if 1
        l_min = sizeof(UDP_datagram) + sizeof(client_hdr);
        l_len = l_len > l_min ? l_len : l_min;

        mBuf = (char *)malloc(l_len);
        if (mBuf == NULL)
        {
            perror("malloc error");
            exit(1);
        }
        memset(mBuf, 0, l_len);

        UDP_datagram *UDPhdr = (UDP_datagram *)mBuf;
        client_hdr *temp_hdr = (client_hdr *)(UDPhdr + 1);

        Settings_GenerateClientHdr(temp_hdr, t_second, p_port, b_mps, l_len);

        /* open a socket support udp multicast */
        sock_id = socket(AF_INET, SOCK_DGRAM, 0);
        if (sock_id < 0)
        {
            perror("socket error");
            exit(1);
        }

        /* setting address */
        memset((void *)&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = inet_addr(c_Host); /* multicast group ip */
        addr.sin_port = htons(p_port);

        socklen = sizeof(addr);

        /* compute delay for bandwidth restriction, constrained to [0,1] seconds */

        usdelay_tar = (int)(l_len * (kSecs_to_usecs * kBytes_to_Bits) / temp_hdr->mWinBand);
        printf("[%s] mWinBand: %d usdelay_tar: %d us\n", __func__, temp_hdr->mWinBand, usdelay_tar);
        do_gettimeofday(&last);
        do_gettimeofday(&lastPacketTime);

        /* err = setsockopt(sock_id, SOL_SOCKET, SO_BINDTODEVICE, "w0", sizeof("w0")); */
        /* rt_kprintf("bind to dev w0 returned: %d\n", err); */
        do
        {
            do_gettimeofday(&now);
            /* delay between writes */
            /* make an adjustment for how long the last loop iteration took */
            adjust = usdelay_tar + subUsec(lastPacketTime, now);
            memcpy(&lastPacketTime, &now, sizeof(struct timeval));
            if (adjust > 0 || delay > 0)
            {
                delay += adjust;
            }

            UDPhdr->id = htonl(packetID++);
            UDPhdr->tv_sec  = htonl(now.tv_sec);
            UDPhdr->tv_usec = htonl(now.tv_usec);

            /*send the data to the address:port */
            ret = sendto(sock_id, mBuf, l_len, 0, (struct sockaddr *)&addr, socklen);
            if (ret < 0)
            {
                /* perror("sendto error"); */
                /* exit(1); */
                rt_thread_delay(1);
            }
            /* printf("Send %d bytes to %s:%u\n",ret,inet_ntoa(addr.sin_addr.s_addr), ntohs(addr.sin_port)); */

            if (delay > 0)
            {
                /* rt_thread_delay(delay * RT_TICK_PER_SECOND / kSecs_to_usecs); */
                udelay(delay);
            }
        } while (subUsec(now, last) <= t_second * kSecs_to_usecs);

        close(sock_id);
#endif
}

/* iperf_udp_c is reference from void Client::Run( void ) in iperf 2.0.2 */
void iperf_udp_c(char *c_Host, ...)
{
    va_list args;
    va_start(args, c_Host);

    int p_port, t_second, b_mps, l_len, i_intvl;
    RT_UNUSED rt_err_t result = RT_EOK;

    if (strcmp(c_Host, "h") == 0)
    {
        printf("%s\n", iperf_udp_c_help);
    }
    else if (strcmp(c_Host, "q") == 0)
    {
        if (tid != RT_NULL)
        {
            rt_thread_delete(tid); /* delete thread */
            tid = RT_NULL;
        }
        return;
    }
    else
    {
        p_port = va_arg(args, int);
        t_second = va_arg(args, int);
        b_mps = va_arg(args, int);
        l_len = va_arg(args, int);
        i_intvl = va_arg(args, int);

        printf("[%s] 'i_intvl = %d' is not used in iperf udp client\n", __func__, i_intvl);
        memset(param.c_Host, 0, 16);
        memcpy(param.c_Host, c_Host, strlen(c_Host));
        param.p_port = p_port;
        param.t_second = t_second;
        param.b_mps = b_mps;
        param.l_len = l_len;
        param.i_intvl = i_intvl;

        tid = rt_thread_create("tiperf_c", iperf_udp_entry, &param, 4096, RT_LWIP_TCPTHREAD_PRIORITY + 30, 10);
        if (tid != RT_NULL)
        {
            result = rt_thread_startup(tid);
            RT_ASSERT(result == RT_EOK);
            return;
        }
        else
        {
            return;
        }

    }

    va_end(args);
}

#ifdef RT_USING_FINSH
#include <finsh.h>
FINSH_FUNCTION_EXPORT(iperf_udp_c, use iperf_udp_c("h") for help info);
#endif
