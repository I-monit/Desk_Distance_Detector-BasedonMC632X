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
#ifndef _ADAPTER_LINUX_IF_ETHER_H_
#define _ADAPTER_LINUX_IF_ETHER_H_

#ifdef __cplusplus
extern "C" {
#endif

#define ETH_ALEN    6       /* Octets in one ethernet addr   */
#define ETH_P_ALL   0x0003      /* Every packet (be careful!!!) */
#define ETH_P_IP    0x0800      /* Internet Protocol packet */
#define ETH_P_ARP   0x0806      /* Address Resolution packet    */

#ifdef __cplusplus
}
#endif

#endif
