#include <lwip/sockets.h>
#include <lwip/priv/sockets_priv.h>
#include "ipc/poll.h"
#include <lwip/netdb.h>
#include <netinet/ether.h>
#include <netinet/if_ether.h>

unsigned short htons(unsigned short x)
{
    return lwip_htons(x);
}

unsigned int htonl(unsigned int x)
{
    return lwip_htonl(x);
}

in_addr_t inet_addr(const char *cp)
{
    return ipaddr_addr(cp);
}

char *inet_ntoa(struct in_addr addr)
{
    return ip4addr_ntoa((const ip4_addr_t *)&addr);
}

char *inet_ntoa_r(struct in_addr addr, char *buf, int buflen)
{
    return ip4addr_ntoa_r((const ip4_addr_t *)&addr, buf, buflen);
}

int inet_aton(const char *cp, struct in_addr *addr)
{
    return ip4addr_aton(cp, (ip4_addr_t *)addr);
}

const char *inet_ntop(int af, const void *src, char *dst, socklen_t size)
{
    return lwip_inet_ntop(af, src, dst, size);
}

int inet_pton(int af, const char *src, void *dst)
{
    return lwip_inet_pton(af, src, dst);
}

extern struct ether_addr *lwip_ether_aton_r(const char *asc, struct ether_addr *addr);
extern char *lwip_ether_ntoa_r(const struct ether_addr *addr, char *buf);

struct ether_addr *ether_aton_r(const char *asc, struct ether_addr *addr)
{
    return lwip_ether_aton_r(asc, addr);
}

struct ether_addr *ether_aton(const char *asc)
{
    static struct ether_addr result;

    return ether_aton_r(asc, &result);
}

char *ether_ntoa_r(const struct ether_addr *addr, char *buf)
{
    return lwip_ether_ntoa_r(addr, buf);
}

char *ether_ntoa(const struct ether_addr *addr)
{
    static char asc[18];

    return ether_ntoa_r(addr, asc);
}

int accept(int s, struct sockaddr *addr, socklen_t *addrlen)
{
    return lwip_accept(s, addr, addrlen);
}

int bind(int s, const struct sockaddr *name, socklen_t namelen)
{
    return lwip_bind(s, name, namelen);
}

int shutdown(int s, int how)
{
    return lwip_shutdown(s, how);
}

int getpeername(int s, struct sockaddr *name, socklen_t *namelen)
{
    return lwip_getpeername(s, name, namelen);
}

int getsockname(int s, struct sockaddr *name, socklen_t *namelen)
{
    return lwip_getsockname(s, name, namelen);
}

int getsockopt(int s, int level, int optname, void *optval, socklen_t *optlen)
{
    return lwip_getsockopt(s, level, optname, optval, optlen);
}

int setsockopt(int s, int level, int optname, const void *optval, socklen_t optlen)
{
    return lwip_setsockopt(s, level, optname, optval, optlen);
}

int connect(int s, const struct sockaddr *name, socklen_t namelen)
{
    return lwip_connect(s, name, namelen);
}

int listen(int s, int backlog)
{
    return lwip_listen(s, backlog);
}

ssize_t recv(int s, void *mem, size_t len, int flags)
{
    return lwip_recv(s, mem, len, flags);
}

ssize_t recvfrom(int s, void *mem, size_t len, int flags,
      struct sockaddr *from, socklen_t *fromlen)
{
    return lwip_recvfrom(s, mem, len, flags, from, fromlen);
}

ssize_t recvmsg(int s, struct msghdr *message, int flags)
{
    return lwip_recvmsg(s, message, flags);
}

ssize_t send(int s, const void *dataptr, size_t size, int flags)
{
    return lwip_send(s, dataptr, size, flags);
}

ssize_t sendmsg(int s, const struct msghdr *message, int flags)
{
    return lwip_sendmsg(s, message, flags);
}

ssize_t sendto(int s, const void *dataptr, size_t size, int flags,
    const struct sockaddr *to, socklen_t tolen)
{
    return lwip_sendto(s, dataptr, size, flags, to, tolen);
}

int socket(int domain, int type, int protocol)
{
    return lwip_socket(domain, type, protocol);
}

extern struct lwip_sock *lwip_tryget_socket(int fd);
extern void done_socket(struct lwip_sock *sock);
int inet_poll(int s, struct rt_pollreq *req)
{
    int mask = 0;
    struct lwip_sock *sock;

    sock = lwip_tryget_socket(s);
    if (sock != NULL)
    {
        rt_base_t level;

        rt_poll_add(&sock->wait_head, req);

        level = rt_hw_interrupt_disable();

        if ((void *)(sock->lastdata.pbuf) || sock->rcvevent)
        {
            mask |= POLLIN;
        }
        if (sock->sendevent)
        {
            mask |= POLLOUT;
        }
        if (sock->errevent)
        {
            mask |= POLLERR;
        }
        rt_hw_interrupt_enable(level);
        done_socket(sock);
    }
    else
    {
        /* mask |= POLLERR; */
        mask = POLLNVAL;
    }

    return mask;
}

struct hostent *gethostbyname(const char *name)
{
    return lwip_gethostbyname(name);
}

int gethostbyname_r(const char *name, struct hostent *ret, char *buf,
                size_t buflen, struct hostent **result, int *h_errnop)
{
    return lwip_gethostbyname_r(name, ret, buf, buflen, result, h_errnop);
}

void freeaddrinfo(struct addrinfo *ai)
{
    return lwip_freeaddrinfo(ai);
}

int getaddrinfo(const char *nodename,
       const char *servname,
       const struct addrinfo *hints,
       struct addrinfo **res)
{
    return lwip_getaddrinfo(nodename, servname, hints, res);
}

int getnameinfo(const struct sockaddr *sa, socklen_t addrlen,
        char *host, size_t hostlen, char *serv, size_t servlen, int flags)
{
    return lwip_getnameinfo(sa, addrlen, host, hostlen, serv, servlen, flags);
}