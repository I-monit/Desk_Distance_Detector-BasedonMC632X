/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2015-02-17     Bernard      First version
 * 2018-05-17     ChenYong     Add socket abstraction layer
 */

#ifndef SYS_SOCKET_H_
#define SYS_SOCKET_H_

#include <sys/types.h>
#include <sys/uio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned int socklen_t;
typedef int ssize_t;
typedef unsigned int size_t;
typedef unsigned char uint8_t;
typedef unsigned short sa_family_t;
typedef unsigned short in_port_t;

/* Protocol families. */
#define PF_UNSPEC   0               /* Unspecified. */
#define PF_LOCAL    1               /* Local to host (pipes and file-domain). */
#define PF_UNIX     PF_LOCAL        /* POSIX name for PF_LOCAL. */
#define PF_INET     2               /* IP protocol family. */
#define PF_INET6    10              /* IP version 6. */
#define PF_PACKET   17              /* Packet family. */
#define PF_CAN      29              /* Controller Area Network. */
#define PF_AT       45              /* AT socket */
#define PF_WIZ      46              /* WIZnet socket */
#define PF_MAX      (PF_WIZ + 1)    /* For now.. */


/* Address families. */
#define AF_UNSPEC   PF_UNSPEC
#define AF_LOCAL    PF_LOCAL
#define AF_UNIX     PF_UNIX
#define AF_INET     PF_INET
#define AF_INET6    PF_INET6
#define AF_PACKET   PF_PACKET
#define AF_CAN      PF_CAN
#define AF_AT       PF_AT
#define AF_WIZ      PF_WIZ
#define AF_MAX      PF_MAX

/* Maximum queue length specifiable by listen.  */
#define SOMAXCONN   128

struct sockaddr
{
    sa_family_t    sa_family;
    char           sa_data[14];
};

struct sockaddr_storage
{
    sa_family_t    ss_family;
    char           s2_data1[2];
    unsigned int   s2_data2[3];
    unsigned int   s2_data3[3];
};

#if 0
#define MSG_PEEK        0x01    /* Peeks at an incoming message */
#define MSG_WAITALL     0x02    /* Unimplemented: Requests that the function block until the full amount of data requested can be returned */
#define MSG_OOB         0x04    /* Unimplemented: Requests out-of-band data. The significance and semantics of out-of-band data are protocol-specific */
#define MSG_DONTWAIT    0x08    /* Nonblocking i/o for this operation only */
#define MSG_MORE        0x10    /* Sender will send more */
#define MSG_NOSIGNAL    0       /* Do not generate SIGPIPE.  */
#else
#define MSG_OOB         0x01    /* Unimplemented: Requests out-of-band data. The significance and semantics of out-of-band data are protocol-specific */
#define MSG_PEEK        0x02    /* Peeks at an incoming message */
#define MSG_DONTWAIT    0x40    /* Nonblocking i/o for this operation only */
#define MSG_WAITALL     0x100   /* Unimplemented: Requests that the function block until the full amount of data requested can be returned */
#define MSG_MORE        0x8000  /* Sender will send more */
#define MSG_NOSIGNAL    0x4000  /* Do not generate SIGPIPE.  */
#endif

struct msghdr
{
  void         *msg_name;
  socklen_t     msg_namelen;
  struct iovec *msg_iov;
  int           msg_iovlen;
  void         *msg_control;
  socklen_t     msg_controllen;
  int           msg_flags;
};

struct linger
{
    int l_onoff;                /* option on/off */
    int l_linger;               /* linger time in seconds */
};

/* Socket protocol types (TCP/UDP/RAW) */
#define SOCK_STREAM     1
#define SOCK_DGRAM      2
#define SOCK_RAW        3
#define SOCK_MAX        (SOCK_RAW + 1) /* Linux Undefined */

#if 0
#define SOL_SOCKET      0xfff    /* options for socket level */
#define SO_BINDTODEVICE 0x100b
#else
#define SOL_SOCKET      1    /* options for socket level */
#endif

/* Option flags per-socket. These must match the SOF_ flags in ip.h (checked in init.c) */
#if 0
#define SO_REUSEADDR    0x0004 /* Allow local address reuse */
#define SO_KEEPALIVE    0x0008 /* keep connections alive */
#define SO_BROADCAST    0x0020 /* permit to send and to receive broadcast messages (see IP_SOF_BROADCAST option) */

/* Additional options, not kept in so_options */
#define SO_DEBUG        0x0001 /* Unimplemented: turn on debugging info recording */
#define SO_ACCEPTCONN   0x0002 /* socket has had listen() */
#define SO_DONTROUTE    0x0010 /* Unimplemented: just use interface addresses */
#define SO_USELOOPBACK  0x0040 /* Unimplemented: bypass hardware when possible */
#define SO_LINGER       0x0080 /* linger on close if data present */
#define SO_DONTLINGER   ((int)(~SO_LINGER))
#define SO_OOBINLINE    0x0100 /* Unimplemented: leave received OOB data in line */
#define SO_REUSEPORT    0x0200 /* Unimplemented: allow local address & port reuse */
#define SO_SNDBUF       0x1001 /* Unimplemented: send buffer size */
#define SO_RCVBUF       0x1002 /* receive buffer size */
#define SO_SNDLOWAT     0x1003 /* Unimplemented: send low-water mark */
#define SO_RCVLOWAT     0x1004 /* Unimplemented: receive low-water mark */
#define SO_SNDTIMEO     0x1005 /* send timeout */
#define SO_RCVTIMEO     0x1006 /* receive timeout */
#define SO_ERROR        0x1007 /* get error status and clear */
#define SO_TYPE         0x1008 /* get socket type */
#define SO_CONTIMEO     0x1009 /* Unimplemented: connect timeout */
#define SO_NO_CHECK     0x100a /* don't create UDP checksum */
#else
#define SO_DEBUG        1  /* Unimplemented: turn on debugging info recording */
#define SO_REUSEADDR    2  /* Allow local address reuse */
#define SO_TYPE         3  /* get socket type */
#define SO_ERROR        4  /* get error status and clear */
#define SO_DONTROUTE    5  /* Unimplemented: just use interface addresses */
#define SO_BROADCAST    6  /* permit to send and to receive broadcast messages (see IP_SOF_BROADCAST option) */
#define SO_SNDBUF       7  /* Unimplemented: send buffer size */
#define SO_RCVBUF       8  /* receive buffer size */
#define SO_KEEPALIVE    9  /* keep connections alive */
#define SO_OOBINLINE    10 /* Unimplemented: leave received OOB data in line */
#define SO_NO_CHECK     11 /* don't create UDP checksum */
#define SO_LINGER       13 /* linger on close if data present */
#define SO_REUSEPORT    15 /* Unimplemented: allow local address & port reuse */
#define SO_RCVLOWAT     18 /* Unimplemented: receive low-water mark */
#define SO_SNDLOWAT     19 /* Unimplemented: send low-water mark */
#define SO_RCVTIMEO     20 /* receive timeout */
#define SO_SNDTIMEO     21 /* send timeout */
#define SO_BINDTODEVICE 25
#define SO_ACCEPTCONN   30 /* socket has had listen() */
#endif

#if 0
#define IPPROTO_IP      0
#define IPPROTO_ICMP    1
#define IPPROTO_TCP     6
#define IPPROTO_UDP     17
#define IPPROTO_IPV6    41
#define IPPROTO_ICMPV6  58
#define IPPROTO_UDPLITE 136
#define IPPROTO_RAW     255

#define IP_TOS             1
#define IP_TTL             2


/* Options for level IPPROTO_TCP */
#define TCP_NODELAY     0x01    /* don't delay send to coalesce packets */
#define TCP_KEEPALIVE   0x02    /* send KEEPALIVE probes when idle for pcb->keep_idle milliseconds */
#define TCP_KEEPIDLE    0x03    /* set pcb->keep_idle  - Same as TCP_KEEPALIVE, but use seconds for get/setsockopt */
#define TCP_KEEPINTVL   0x04    /* set pcb->keep_intvl - Use seconds for get/setsockopt */
#define TCP_KEEPCNT     0x05    /* set pcb->keep_cnt   - Use number of probes sent for get/setsockopt */

/* Options and types related to multicast membership */
#define IP_ADD_MEMBERSHIP  3
#define IP_DROP_MEMBERSHIP 4
#endif

#ifndef SHUT_RD
  #define SHUT_RD       0
  #define SHUT_WR       1
  #define SHUT_RDWR     2
#endif


#if 0
struct in_addr
{
    in_addr_t s_addr;
};
#endif

#if 0
struct sockaddr_in
{
    uint8_t        sin_len;
    sa_family_t    sin_family;
    in_port_t      sin_port;
    struct in_addr sin_addr;
#define SIN_ZERO_LEN 8
    char            sin_zero[SIN_ZERO_LEN];
};
#endif

/** 255.255.255.255 */
#define IPADDR_NONE         ((unsigned int)0xffffffffUL)
/** 127.0.0.1 */
#ifndef INADDR_LOOPBACK
#define IPADDR_LOOPBACK     ((unsigned int)0x7f000001UL)
#endif
/** 0.0.0.0 */
#define IPADDR_ANY          ((unsigned int)0x00000000UL)
/** 255.255.255.255 */
#define IPADDR_BROADCAST    ((unsigned int)0xffffffffUL)

/** 255.255.255.255 */
#define INADDR_NONE         IPADDR_NONE
/** 127.0.0.1 */
#ifndef INADDR_LOOPBACK
#define INADDR_LOOPBACK     IPADDR_LOOPBACK
#endif
/** 0.0.0.0 */
#define INADDR_ANY          IPADDR_ANY
/** 255.255.255.255 */
#define INADDR_BROADCAST    IPADDR_BROADCAST

#define IN6ADDR_ANY_INIT {{{0, 0, 0, 0}}}

#if 0
#define IP_MULTICAST_TTL   5
#define IP_MULTICAST_IF    6
#define IP_MULTICAST_LOOP  7

typedef struct ip_mreq
{
    struct in_addr imr_multiaddr; /* IP multicast address of group */
    struct in_addr imr_interface; /* local IP address of interface */
} ip_mreq;
#endif

int accept(int s, struct sockaddr *addr, socklen_t *addrlen);
int bind(int s, const struct sockaddr *name, socklen_t namelen);
int shutdown(int s, int how);
int getpeername(int s, struct sockaddr *name, socklen_t *namelen);
int getsockname(int s, struct sockaddr *name, socklen_t *namelen);
int getsockopt(int s, int level, int optname, void *optval, socklen_t *optlen);
int setsockopt(int s, int level, int optname, const void *optval, socklen_t optlen);
int connect(int s, const struct sockaddr *name, socklen_t namelen);
int listen(int s, int backlog);
int recv(int s, void *mem, size_t len, int flags);
int recvfrom(int s, void *mem, size_t len, int flags,
      struct sockaddr *from, socklen_t *fromlen);
int send(int s, const void *dataptr, size_t size, int flags);
int sendto(int s, const void *dataptr, size_t size, int flags,
    const struct sockaddr *to, socklen_t tolen);
int socket(int domain, int type, int protocol);
ssize_t recvmsg(int s, struct msghdr *msg, int flags);
ssize_t sendmsg(int s, const struct msghdr *msg, int flags);


#ifdef __cplusplus
}
#endif

#endif /* _SYS_SOCKET_H_ */
