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
#ifndef __ADAPTER_SOCKIOS_H__
#define __ADAPTER_SOCKIOS_H__

#ifdef __cplusplus
extern "C" {
#endif

#define TIOCOUTQ        0x5411

/* Routing table calls. */
#define SIOCADDRT       0x890B      /* add routing table entry */
#define SIOCDELRT       0x890C      /* delete routing table entry */
#define SIOCRTMSG       0x890D      /* call to routing system */

#define SIOCGIFNAME     0x8910      /* get iface name */
#define SIOCSIFLINK     0x8911      /* set iface channel */
#define SIOCGIFCONF     0x8912      /* get iface list */
#define SIOCGIFFLAGS    0x8913      /* get flags */
#define SIOCSIFFLAGS    0x8914      /* set flags */
#define SIOCGIFADDR     0x8915      /* get PA address */
#define SIOCSIFADDR     0x8916      /* set PA address */
#define SIOCGIFBRDADDR  0x8919
#define SIOCGIFNETMASK  0x891b      /* get network PA mask */
#define SIOCSIFNETMASK  0x891c      /* set network PA mask */
#define SIOCGIFMTU      0x8921      /* get MTU size */
#define SIOCSIFMTU      0x8922      /* set MTU size */
#define SIOCSIFHWADDR   0x8924      /* set hardware address */
#define SIOCGIFENCAP    0x8925      /* get/set encapsulations */
#define SIOCGIFHWADDR   0x8927      /* Get hardware address */
#define SIOCGIFINDEX    0x8933      /* name -> if_index mapping */

#define SIOCETHTOOL     0x8946      /* Ethtool interface */

/* typedef unsigned char sa_family_t; */

#ifdef __cplusplus
}
#endif

#endif

