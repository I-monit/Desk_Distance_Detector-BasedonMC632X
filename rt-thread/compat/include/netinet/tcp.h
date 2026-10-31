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
#ifndef __TCP_H
#define __TCP_H

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/socket.h>

/* Options for level IPPROTO_TCP */
#define TCP_NODELAY     0x01    /* don't delay send to coalesce packets */
#define TCP_MAXSEG      0x02    /* Limit MSS */
#define TCP_CORK        0x03    /* Control sending of partial frames  */
#define TCP_KEEPIDLE    0x04    /* set pcb->keep_idle  - Same as TCP_KEEPALIVE, but use seconds for get/setsockopt */
#define TCP_KEEPINTVL   0x05    /* set pcb->keep_intvl - Use seconds for get/setsockopt */
#define TCP_KEEPCNT     0x06    /* set pcb->keep_cnt   - Use number of probes sent for get/setsockopt */
#define TCP_INFO        11      /* Information about this connection. */


struct tcp_info
{
  unsigned char  tcpi_state;
  unsigned char  tcpi_ca_state;
  unsigned char  tcpi_retransmits;
  unsigned char  tcpi_probes;
  unsigned char  tcpi_backoff;
  unsigned char  tcpi_options;
  unsigned char  tcpi_snd_wscale : 4, tcpi_rcv_wscale : 4;

  unsigned int tcpi_rto;
  unsigned int tcpi_ato;
  unsigned int tcpi_snd_mss;
  unsigned int tcpi_rcv_mss;

  unsigned int tcpi_unacked;
  unsigned int tcpi_sacked;
  unsigned int tcpi_lost;
  unsigned int tcpi_retrans;
  unsigned int tcpi_fackets;

  /* Times. */
  unsigned int tcpi_last_data_sent;
  unsigned int tcpi_last_ack_sent; /* Not remembered, sorry.  */
  unsigned int tcpi_last_data_recv;
  unsigned int tcpi_last_ack_recv;

  /* Metrics. */
  unsigned int tcpi_pmtu;
  unsigned int tcpi_rcv_ssthresh;
  unsigned int tcpi_rtt;
  unsigned int tcpi_rttvar;
  unsigned int tcpi_snd_ssthresh;
  unsigned int tcpi_snd_cwnd;
  unsigned int tcpi_advmss;
  unsigned int tcpi_reordering;

  unsigned int tcpi_rcv_rtt;
  unsigned int tcpi_rcv_space;

  unsigned int tcpi_total_retrans;
};

#ifdef __cplusplus
}
#endif

#endif
