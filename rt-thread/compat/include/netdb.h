/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2015-02-17     Bernard      First version
 * 2108-05-24     ChenYong     Add socket abstraction layer
 */

#ifndef NETDB_H__
#define NETDB_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <netinet/in.h>

#define EAI_BADFLAGS    -1
#define EAI_NONAME      -2
#define EAI_FAIL        -4
#define EAI_FAMILY      -6
#define EAI_SERVICE     -8
#define EAI_MEMORY      -10
#define EAI_SYSTEM      -11

#define HOST_NOT_FOUND  1
#define TRY_AGAIN       2
#define NO_RECOVERY     3
#define NO_DATA         4

/* input flags for structure addrinfo */
#define AI_PASSIVE      0x01
#define AI_CANONNAME    0x02
#define AI_NUMERICHOST  0x04
#define AI_V4MAPPED     0x8
#define AI_ALL          0x10
#define AI_ADDRCONFIG   0x20

#define AI_NUMERICSERV  0x0400

#define NI_NUMERICHOST  1   /* Don't try to look up hostname.  */
#define NI_NUMERICSERV  2   /* Don't convert port number to name.  */
#define NI_NOFQDN       4   /* Only return nodename portion.  */
#define NI_NAMEREQD     8   /* Don't return numeric addresses.  */
#define NI_DGRAM        16   /* Look up UDP service rather than TCP.  */

#define NI_MAXHOST  1025
#define NI_MAXSERV  32


struct hostent
{
    char  *h_name;      /* Official name of the host. */
    char **h_aliases;   /* A pointer to an array of pointers to alternative host names,
                           terminated by a null pointer. */
    int    h_addrtype;  /* Address type. */
    int    h_length;    /* The length, in bytes, of the address. */
    char **h_addr_list; /* A pointer to an array of pointers to network addresses (in
                           network byte order) for the host, terminated by a null pointer. */
#define h_addr h_addr_list[0] /* for backward compatibility */
};

struct addrinfo
{
    int               ai_flags;      /* Input flags. */
    int               ai_family;     /* Address family of socket. */
    int               ai_socktype;   /* Socket type. */
    int               ai_protocol;   /* Protocol of socket. */
    unsigned int         ai_addrlen;    /* Length of socket address. */
    struct sockaddr  *ai_addr;       /* Socket address of socket. */
    char             *ai_canonname;  /* Canonical name of service location. */
    struct addrinfo  *ai_next;       /* Pointer to next in list. */
};


struct hostent *gethostbyname(const char *name);

int gethostbyname_r(const char *name, struct hostent *ret, char *buf,
                size_t buflen, struct hostent **result, int *h_errnop);
void freeaddrinfo(struct addrinfo *ai);
int getaddrinfo(const char *nodename,
       const char *servname,
       const struct addrinfo *hints,
       struct addrinfo **res);
int getnameinfo(const struct sockaddr *sa, socklen_t addrlen,
        char *host, size_t hostlen, char *serv, size_t servlen, int flags);

#ifdef __cplusplus
}
#endif

#endif
