/*
 * netutils: ping implementation
 */

#include "lwip/opt.h"

#include <rtthread.h>
/* #include "app_cfg.h" */

/* #include "lwip/lwipmem.h" */
#include "lwip/icmp.h"
#include "lwip/netif.h"
#include "lwip/sys.h"
#include "lwip/sockets.h"
#include "lwip/inet.h"
#include "lwip/inet_chksum.h"
#include "lwip/ip.h"
#include "lwip/ip_addr.h"
#include "arpa/inet.h"
#include "lwip/netdb.h"

/**
 * PING_DEBUG: Enable debugging for PING.
 */
#ifndef PING_DEBUG
#define PING_DEBUG LWIP_DBG_ON
#endif

/** ping receive timeout - in milliseconds */
#define PING_RCV_TIMEO 2000
/** ping delay - in milliseconds */
#define PING_DELAY 10  /* 100 */

#define BUF_SIZE 1500

/** ping identifier - must fit on a u16_t */
#ifndef PING_ID
#define PING_ID 0xAFAF
#endif

/** ping additional data size to include in the packet */
#ifndef PING_DATA_SIZE
#define PING_DATA_SIZE 32
#endif

/* ping variables */
static u16_t ping_seq_num;
static rt_uint32_t ping_time;
static unsigned int ping_recv_count;
static unsigned char reply_buf[BUF_SIZE];

/** Prepare a echo ICMP request */
static void ping_prepare_echo(struct icmp_echo_hdr *iecho, u16_t len)
{
    size_t i;
    size_t data_len = len - sizeof(struct icmp_echo_hdr);

    ICMPH_TYPE_SET(iecho, ICMP_ECHO);
    ICMPH_CODE_SET(iecho, 0);
    iecho->chksum = 0;
    iecho->id     = PING_ID;
    iecho->seqno  = htons(++ping_seq_num);

    /* fill the additional data buffer with some data */
    for (i = 0; i < data_len; i++)
    {
        ((char *)iecho)[sizeof(struct icmp_echo_hdr) + i] = (char)i;
    }

#ifdef RT_LWIP_CHECKSUM_GEN
    iecho->chksum = inet_chksum(iecho, len);
#else
    iecho->chksum = 0;
#endif
}

/* Ping using the socket ip */
static err_t ping_send(int s, struct in_addr *addr, size_t size)
{
    int err;
    struct icmp_echo_hdr *iecho;
    struct sockaddr_in to;
    size_t ping_size;

    if (size == 0)
    {
        ping_size = sizeof(struct icmp_echo_hdr) + PING_DATA_SIZE;
    }
    else
    {
        ping_size = sizeof(struct icmp_echo_hdr) + size;
    }

    LWIP_ASSERT("\n\rping_size is too big", ping_size <= 0xffff);

    iecho = (struct icmp_echo_hdr *)mem_malloc(ping_size);
    if (iecho == NULL)
    {
        return ERR_MEM;
    }

    ping_prepare_echo(iecho, (u16_t)ping_size);

    to.sin_family      = AF_INET;
    to.sin_addr.s_addr = addr->s_addr;

    err =
        lwip_sendto(s, iecho, ping_size, 0, (struct sockaddr *)&to, sizeof(to));
    mem_free(iecho);

    return err ? ERR_OK : ERR_VAL;
}

static void ping_recv(int s)
{
    /* OS_ERR err; */
    rt_uint32_t reply_time;
    int fromlen, len;
    struct sockaddr_in from;
    struct ip_hdr *iphdr;
    struct icmp_echo_hdr *iecho;
    /* struct _ip_addr *addr; */
    /* static u32_t ping_recv_count = 0; */

    while ((len = lwip_recvfrom(s, reply_buf, sizeof(reply_buf), 0,
                                (struct sockaddr *)&from,
                                (socklen_t *)&fromlen)) > 0)
    {
        if (len >= (sizeof(struct ip_hdr) + sizeof(struct icmp_echo_hdr)))
        {
            /* addr = (struct _ip_addr *)&(from.sin_addr); */
            /* rt_kprintf(PING_DEBUG | LWIP_DBG_TRACE, ("@@@ping: recv */
            /* %d.%d.%d.%d, len = %d, count = %d\n", */
            /* addr->addr0, addr->addr1, addr->addr2, addr->addr3, len, */
            /* ++ping_recv_count)); */
            reply_time = rt_tick_get();

            iphdr = (struct ip_hdr *)reply_buf;
            iecho = (struct icmp_echo_hdr *)(reply_buf + (IPH_HL(iphdr) * 4));
            if (iecho->type == ICMP_ER)
            {
                if ((iecho->id == PING_ID) && (iecho->seqno == htons(ping_seq_num)))
                {
                    rt_kprintf(
                        "[ping_test] %d bytes from %s: icmp_seq=%d time=%d ms\n",
                        len - sizeof(struct ip_hdr) - sizeof(struct icmp_echo_hdr),
                        inet_ntoa(from.sin_addr), htons(iecho->seqno),
                        (reply_time - ping_time) * 1000 / RT_TICK_PER_SECOND);
                    ping_recv_count++;
                    return;
                }
                else
                {
                    if (iecho->id == PING_ID)
                    {
                        rt_kprintf("\n\r@@@ping: drop\n");
                    }
                }
            }
        }
    }

    if (len <= 0)
    {
        rt_kprintf("\n\r[ping_test] timeout\n");
    }
}

err_t ping(char *target, u32_t count, size_t size)
{
    int s;
    struct timeval timeout;
    timeout.tv_sec = PING_RCV_TIMEO/1000;
    timeout.tv_usec = (PING_RCV_TIMEO%1000)*1000;

    struct in_addr ping_target;
    u32_t send_count;

    send_count      = 0;
    ping_recv_count = 0;

    if (inet_aton(target, (struct in_addr *)&ping_target) == 0)
        return ERR_VAL;
    /* addr = (struct _ip_addr*)&ping_target; */

    if ((s = lwip_socket(AF_INET, SOCK_RAW, IP_PROTO_ICMP)) < 0)
    {
        rt_kprintf("\n\r@@@ping: create socket failled\n");
        return ERR_MEM;
    }

    lwip_setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    rt_kprintf("\n\r[ping_test] PING %s %d bytes of data\n",
               inet_ntoa(ping_target), size);

    while (1)
    {
        /* add for random size ping test */
        /* size_random = 32 + rand()%(1460-32+1); */

        /* if (ping_send(s, &ping_target, size_random) == ERR_OK) */
        if (ping_send(s, &ping_target, size) == ERR_OK)
        {
            /* rt_kprintf(PING_DEBUG | LWIP_DBG_TRACE, ("@@@ping: send */
            /* %d.%d.%d.%d, size = %d, count = %d\n", */
            /* addr->addr0, addr->addr1, addr->addr2, addr->addr3, size, */
            /* ++ping_send_count)); */
            ping_time = rt_tick_get();
            ping_recv(s);
        }
        else
        {
            rt_kprintf("\n\r@@@ping: send %s - error\n",
                       inet_ntoa(ping_target));
        }

        send_count++;
        if (send_count >= count)
            break;

        /* sys_msleep(PING_DELAY);  take a delay  */
    }

    rt_kprintf(
        "\n\r[ping_test] %d packets transmitted, %d received, %d%% packet loss\n",
        send_count, ping_recv_count,
        (send_count - ping_recv_count) * 100 / send_count);

    lwip_close(s);

    if (ping_recv_count == 0)
    {
        return ERR_TIMEOUT;
    }
    return ERR_OK;
}

void ping_entry(char *target, u32_t count, size_t size)
{
    err_t err;

    if (target == RT_NULL)
    {
        rt_kprintf("target IP is empty\n");
        return;
    }

    if ((size == 0) || (size > 0xffff))
        size = PING_DATA_SIZE;

    err = ping(target, count, size);

    if (err != ERR_OK)
    {
        rt_kprintf("\n\r@@@ping: exit because of error!\n");
    }
}

#ifdef RT_USING_FINSH
#include <finsh.h>
FINSH_FUNCTION_EXPORT(ping, ping network host);
#endif

int cmd_ping(int argc, char **argv)
{
    char ip[32];
    struct sockaddr_in *addr;
    struct addrinfo *answer, hint, *curr;
    int ret = 0;

    memset(ip, 0, 32);

    if (argc == 2)
    {
        if (memcmp(argv[1], "-h", 2) == 0)
        {
            rt_kprintf("e.g: ping 192.168.1.30\n");
            rt_kprintf("or   ping www.baidu.com\n");
            return 0;
        }
        memset(&hint, 0, sizeof(hint));
        hint.ai_family = AF_INET;
        hint.ai_socktype = SOCK_STREAM;

        ret = lwip_getaddrinfo(argv[1], NULL, &hint, &answer);
        if (ret != 0)
        {
            rt_kprintf("getaddrinfo failed. ret %d\n", ret);
            return -1;
        }

        for (curr = answer; curr != NULL; curr = curr->ai_next)
        {
            addr = (struct sockaddr_in *)(curr->ai_addr);
            inet_ntop(AF_INET, &addr->sin_addr, ip, sizeof(ip));
        }
        if (inet_addr(ip) != IPADDR_NONE)
        {
            ping(ip, 4, 64);
        }
        else
        {
            rt_kprintf("bad parameter! Wrong ipaddr!\n");
        }

        lwip_freeaddrinfo(answer);
    }
    else if (argc == 4)
    {
        ping(argv[1], (u32_t)atoi(argv[2]), (size_t)atoi(argv[3]));
    }
    else
    {
        rt_kprintf("bad parameter! e.g: ping 192.168.1.30\n");
        rt_kprintf("               or   ping www.baidu.com\n");
    }
    return 0;
}

#ifdef FINSH_USING_MSH
FINSH_FUNCTION_EXPORT_ALIAS(cmd_ping, __cmd_ping, send ICMP ECHO_REQUEST to network hosts);
#endif
