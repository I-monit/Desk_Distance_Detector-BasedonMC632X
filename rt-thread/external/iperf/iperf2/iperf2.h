#ifndef __IPERF2_H
#define __IPERF2_H

#include <stdlib.h>
#include <stdio.h>

typedef struct
{
    char c_Host[16];
    int p_port;
    int t_second;
    int b_mps;
    int l_len;
    int i_intvl;
} IPERF_PARAM;

#endif