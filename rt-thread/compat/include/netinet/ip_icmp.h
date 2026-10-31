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
#ifndef __IP_ICMP_H
#define __IP_ICMP_H

#ifdef __cplusplus
extern "C" {
#endif

#include <netinet/ip.h>
#include <netinet/in.h>
#include <sys/types.h>

#if 0
#define ICMP_ER   0    /* echo reply */
#define ICMP_DUR  3    /* destination unreachable */
#define ICMP_SQ   4    /* source quench */
#define ICMP_RD   5    /* redirect */
#define ICMP_ECHO 8    /* echo */
#define ICMP_TE  11    /* time exceeded */
#define ICMP_PP  12    /* parameter problem */
#define ICMP_TS  13    /* timestamp */
#define ICMP_TSR 14    /* timestamp reply */
#define ICMP_IRQ 15    /* information request */
#define ICMP_IR  16    /* information reply */
#define ICMP_AM  17    /* address mask request */
#define ICMP_AMR 18    /* address mask reply */
#else
#define ICMP_ECHOREPLY      0   /* Echo Reply           */
#define ICMP_DEST_UNREACH   3   /* Destination Unreachable  */
#define ICMP_SOURCE_QUENCH  4   /* Source Quench        */
#define ICMP_REDIRECT       5   /* Redirect (change route)  */
#define ICMP_ECHO           8   /* Echo Request         */
#define ICMP_TIME_EXCEEDED  11  /* Time Exceeded        */
#define ICMP_PARAMETERPROB  12  /* Parameter Problem        */
#define ICMP_TIMESTAMP      13  /* Timestamp Request        */
#define ICMP_TIMESTAMPREPLY 14  /* Timestamp Reply      */
#define ICMP_INFO_REQUEST   15  /* Information Request      */
#define ICMP_INFO_REPLY     16  /* Information Reply        */
#define ICMP_ADDRESS        17  /* Address Mask Request     */
#define ICMP_ADDRESSREPLY   18  /* Address Mask Reply       */
#endif

/*
 * Internal of an ICMP Router Advertisement
 */
struct icmp_ra_addr
{
  unsigned int ira_addr;
  unsigned int ira_preference;
};

struct icmp
{
  u_char  icmp_type;    /* type of message, see below */
  u_char  icmp_code;    /* type sub code */
  u_short icmp_cksum;    /* ones complement checksum of struct */
  union
  {
    u_char ih_pptr;        /* ICMP_PARAMPROB */
    struct in_addr ih_gwaddr;    /* gateway address */
    struct ih_idseq        /* echo datagram */
    {
      u_short icd_id;
      u_short icd_seq;
    } ih_idseq;
    unsigned int ih_void;

    /* ICMP_UNREACH_NEEDFRAG -- Path MTU Discovery (RFC1191) */
    struct ih_pmtu
    {
      u_short ipm_void;
      u_short ipm_nextmtu;
    } ih_pmtu;

    struct ih_rtradv
    {
      u_char irt_num_addrs;
      u_char irt_wpa;
      u_short irt_lifetime;
    } ih_rtradv;
  } icmp_hun;
#define    icmp_pptr    icmp_hun.ih_pptr
#define    icmp_gwaddr    icmp_hun.ih_gwaddr
#define    icmp_id        icmp_hun.ih_idseq.icd_id
#define    icmp_seq    icmp_hun.ih_idseq.icd_seq
#define    icmp_void    icmp_hun.ih_void
#define    icmp_pmvoid    icmp_hun.ih_pmtu.ipm_void
#define    icmp_nextmtu    icmp_hun.ih_pmtu.ipm_nextmtu
#define    icmp_num_addrs    icmp_hun.ih_rtradv.irt_num_addrs
#define    icmp_wpa    icmp_hun.ih_rtradv.irt_wpa
#define    icmp_lifetime    icmp_hun.ih_rtradv.irt_lifetime
  union
  {
    struct
    {
      unsigned int its_otime;
      unsigned int its_rtime;
      unsigned int its_ttime;
    } id_ts;
    struct
    {
      struct ip idi_ip;
      /* options and then 64 bits of data */
    } id_ip;
    struct icmp_ra_addr id_radv;
    unsigned int   id_mask;
    u_char    id_data[1];
  } icmp_dun;
#define    icmp_otime    icmp_dun.id_ts.its_otime
#define    icmp_rtime    icmp_dun.id_ts.its_rtime
#define    icmp_ttime    icmp_dun.id_ts.its_ttime
#define    icmp_ip        icmp_dun.id_ip.idi_ip
#define    icmp_radv    icmp_dun.id_radv
#define    icmp_mask    icmp_dun.id_mask
#define    icmp_data    icmp_dun.id_data
};

#ifdef __cplusplus
}
#endif

#endif
