/*
 *  * Copyright (c) 2015-2019 Shanghai Fullhan Microelectronics Co., Ltd.
 *  *
 *  * SPDX-License-Identifier: Apache-2.0
 *  *
 *  * Change Logs:
 *  * Date           Author       Notes
 *  * 2019-01-18      {fullhan}   the first version
 *  *
 */
#ifndef __NETINET_UDP_H
#define __NETINET_UDP_H

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>

struct udphdr
{
  unsigned short source;
  unsigned short dest;
  unsigned short len;
  unsigned short check;
};

#ifdef __cplusplus
}
#endif

#endif
