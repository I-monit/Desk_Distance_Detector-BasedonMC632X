/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2015-02-17     Bernard      First version
 */

#ifndef INET_H__
#define INET_H__
#include <netinet/in.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned int socklen_t;

in_addr_t inet_addr(const char *cp);
char *inet_ntoa(struct in_addr addr);
int inet_aton(const char *cp, struct in_addr *addr);
const char *inet_ntop(int af, const void *src, char *dst, socklen_t size);
int inet_pton(int af, const char *src, void *dst);

#ifdef __cplusplus
}
#endif

#endif
