/*
 * netutils: ping implementation
 */

#include "lwip/opt.h"

#include <rtthread.h>
#include "lwip/icmp.h"
#include "lwip/netif.h"
#include "lwip/sys.h"
#include "lwip/sockets.h"
#include "lwip/inet_chksum.h"
#include "lwip/ip.h"
#include "lwip/ip_addr.h"
#include "lwip/netdb.h"
#include <unistd.h>
/**
 * PING_DEBUG: Enable debugging for PING.
 */
#ifndef PING_DEBUG
#define PING_DEBUG LWIP_DBG_ON
#endif

/** ping receive timeout - in milliseconds */
#define PING_RCV_TIMEO 2000
/** ping delay - in milliseconds */
#define PING_DELAY 10  // 100

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
static char ip[128];
static char netif_name[10];

/** Prepare a echo ICMP request */
static void ping6_prepare_echo(struct icmp6_echo_hdr *iecho, u16_t len)
{
    size_t i;
    size_t data_len = len - sizeof(struct icmp6_echo_hdr);
    ICMPH_TYPE_SET(iecho, ICMP6_TYPE_EREQ);
    ICMPH_CODE_SET(iecho, 0);
    iecho->chksum = 0;
    iecho->id     = PING_ID;
    iecho->seqno = htons(++ping_seq_num);

    /* fill the additional data buffer with some data */
    for (i = 0; i < data_len; i++)
    {
        ((char *)iecho)[sizeof(struct icmp6_echo_hdr) + i] = (char)i;
    }

   iecho->chksum = 0;
}

static int get_ifindex(int fd, const char *sname)
{
    int ret;
    struct ifreq ifr;

    if (sname == NULL)
    {
        rt_kprintf("invalid ifname.\n");
        return -1;
    }
    memset(&ifr, 0, sizeof(ifr));
    strcpy(ifr.ifr_name, sname);
    ret = lwip_ioctl(fd, SIOCGIFINDEX, &ifr);
    if (ret < 0)
    {
        rt_kprintf("get ifindex error.\n");
        return -1;
    }

    return ifr.ifr_ifindex;
}

/* Ping using the socket ip */
static err_t ping6_send(int s, struct in6_addr *addr, size_t size)
{
    int err;
    struct icmp6_echo_hdr *iecho;
    struct sockaddr_in6 to;
    size_t ping_size;
    if (size == 0)
    {
        ping_size = sizeof(struct icmp6_echo_hdr) + PING_DATA_SIZE;
    }
    else
    {
        ping_size = sizeof(struct icmp6_echo_hdr) + size;
    }

    memset(&to, 0, sizeof(to));

    LWIP_ASSERT("\n\rping_size is too big", ping_size <= 0xffff);

    iecho = (struct icmp6_echo_hdr *)mem_malloc(ping_size);
    if (iecho == NULL)
    {
        return ERR_MEM;
    }
    ping6_prepare_echo(iecho, (u16_t)ping_size);

    to.sin6_family      = AF_INET6;
    to.sin6_addr = *addr;
    to.sin6_scope_id = get_ifindex(s, netif_name);
    if (!to.sin6_scope_id)
    {
        rt_kprintf("bad netif! not exit!!\n");
        mem_free(iecho);
        return ERR_IF;
    }
    err =
        lwip_sendto(s, iecho, ping_size, 0, (struct sockaddr *)&to, sizeof(to));
    mem_free(iecho);

    return (err ? ERR_OK : ERR_VAL);
}
static void ping6_recv(int s)
{
    rt_uint32_t reply_time;
    int fromlen, len;
    struct sockaddr_in6 from;
    struct icmp6_echo_hdr *iecho;

    memset(&from, 0, sizeof(from));
    fromlen = sizeof(from);

    while ((len = lwip_recvfrom(s, reply_buf, sizeof(reply_buf), 0,
                                (struct sockaddr *)&from,
                                (socklen_t *)&fromlen)) > 0)
    {
        if (len >= (sizeof(struct ip6_hdr) + sizeof(struct icmp6_echo_hdr)))
        {
            reply_time = rt_tick_get();

            iecho = (struct icmp6_echo_hdr *)(reply_buf + sizeof(struct ip6_hdr));
            if (iecho->type == ICMP6_TYPE_EREP)
            {
                if ((iecho->id == PING_ID) && (iecho->seqno == htons(ping_seq_num)))
                {
                    rt_kprintf(
                        "\n\r[ping_test] %d bytes from %s: icmp_seq=%d time=%d ms",
                        len - sizeof(struct ip6_hdr) - sizeof(struct icmp6_echo_hdr),
                        inet6_ntoa(from.sin6_addr), htons(iecho->seqno),
                        (reply_time - ping_time) * 1000 / RT_TICK_PER_SECOND);
                    ping_recv_count++;
                    return;
                }
                else
                {
                    if ((iecho->id == PING_ID) && (iecho->type == ICMP6_TYPE_EREP))
                    {
                        rt_kprintf("\n\r@@@ping: drop\n");
                    }
                }
            }
        }
    }

    if (len <= 0)
    {
        rt_kprintf("\n\r[ping_test] timeout");
    }
}

err_t ping6(char *target, u32_t count, size_t size)
{
    int s;
    struct timeval timeout;
    timeout.tv_sec = PING_RCV_TIMEO/1000;
    timeout.tv_usec = (PING_RCV_TIMEO%1000)*1000;
    ip_addr_t ping_target;
    u32_t send_count;
    send_count      = 0;
    ping_recv_count = 0;
    err_t err;

    if (ipaddr_aton(target, (ip_addr_t *)&ping_target) == 0) return ERR_VAL;

    if ((s = lwip_socket(AF_INET6, SOCK_RAW, IP6_NEXTH_ICMP6)) < 0)
    {
        rt_kprintf("\n\r@@@ping: create socket failled");
        return ERR_MEM;
    }

    lwip_setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    rt_kprintf("\n\r[ping_test] PING %s %d bytes of data\n",
               ipaddr_ntoa(&ping_target), size);
    while (1)
    {
        if ((err = ping6_send(s, (struct in6_addr *)&ping_target, size)) == ERR_OK)
        {
            ping_time = rt_tick_get();
            ping6_recv(s);
        }
        else if (err == ERR_IF)
        {
            lwip_close(s);
            return ERR_IF;
        }
        else
        {
            rt_kprintf("\n\r@@@ping: send %s - error\n",
                       ipaddr_ntoa(&ping_target));
        }

        send_count++;
        if (send_count >= count) break; /* send ping times reached, stop */
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

#ifdef RT_USING_FINSH
#include <finsh.h>
FINSH_FUNCTION_EXPORT(ping6, ping6 network host);
#endif

int cmd_ping6(int argc, char **argv)
{
    char *temp = NULL;
    ip_addr_t ip_target;
    int len = 0;
    struct sockaddr_in6 *addr;
    struct addrinfo *answer, hint, *curr;
    char str[128];
    int ret = 0;

    memset(ip, 0, 128);
    memset(netif_name, 0, 10);
    memset(str, 0, sizeof(str));

    if (argc == 2)
    {
        if (memcmp(argv[1], "-h", 2) == 0)
        {
            rt_kprintf("e.g: ping6 fe80::216%e0\n");
            rt_kprintf("or   ping6 aliyun.com%e0\n");
            return 0;
        }
        len = strlen(argv[1]);
        temp = strtok(argv[1], "%");
        if (temp)
        {
            if (len <= strlen(temp))
            {
                rt_kprintf("bad parameter!  Invalid argument!\n");
                return -1;
            }
            memcpy(ip, temp, strlen(temp));
            memcpy(netif_name, argv[1]+strlen(temp)+1, 2);

            memset(&hint, 0, sizeof(hint));
            hint.ai_family = AF_INET6;
            hint.ai_socktype = SOCK_STREAM;

            ret = lwip_getaddrinfo(ip, NULL, &hint, &answer);
            if (ret != 0)
            {
                rt_kprintf("getaddrinfo failed. ret %d\n", ret);
                return -1;
            }

            for (curr = answer; curr != NULL; curr = curr->ai_next)
            {
                addr = (struct sockaddr_in6 *)(curr->ai_addr);
                inet_ntop(AF_INET6, &addr->sin6_addr, str, sizeof(str));
            }

            if (ipaddr_aton(str, (ip_addr_t *)&ip_target) > 0)
            {
                ping6(str, 4, 64);
            }
            else
            {
                rt_kprintf("bad parameter! Wrong ipaddr!\n");
                rt_kprintf("e.g: ping6 fe80::216%e0\n");
                rt_kprintf("or   ping6 aliyun.com%e0\n");
            }

            lwip_freeaddrinfo(answer);
        }
    }
    else if (argc == 4)
    {
        ping6(ip, (u32_t)atoi(argv[2]), (size_t)atoi(argv[3]));
    }
    else
    {
        rt_kprintf("bad parameter! e.g: ping6 fe80::216%e0\n");
        rt_kprintf("               or   ping6 aliyun.com%e0\n");
    }
    return 0;
}

#ifdef FINSH_USING_MSH
FINSH_FUNCTION_EXPORT_ALIAS(cmd_ping6, __cmd_ping6, send ICMPv6 ECHO_REQUEST to network hosts);
#endif
