#ifndef _ADAPTER_NETPACKET_PACKET_H_
#define _ADAPTER_NETPACKET_PACKET_H_

#ifdef __cplusplus
extern "C" {
#endif

struct sockaddr_ll
{
    unsigned short int sll_family;
    unsigned short int sll_protocol;
    int sll_ifindex;
    unsigned short int sll_hatype;
    unsigned char sll_pkttype;
    unsigned char sll_halen;
    unsigned char sll_addr[8];
};

#ifdef __cplusplus
}
#endif

#endif
