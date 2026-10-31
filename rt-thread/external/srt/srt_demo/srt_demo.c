/*
 * SRT - Secure, Reliable, Transport
 * Copyright (c) 2017 Haivision Systems Inc.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; If not, see <http://www.gnu.org/licenses/>
 */


#include <stdio.h>
#include <stdlib.h>
#ifdef _WIN32
#define usleep(x) Sleep(x / 1000)
#else
#include <unistd.h>
#endif

#include "srt.h"
#include <pthread.h>
#define RST_PORT 12345

int srt_client_proc()
{
    int ss, st;
    struct sockaddr_in sa;
    int yes = 1;
    const char message [] = "This message should be sent to the other side";

    printf("srt startup\n");
    srt_startup();

    printf("srt socket\n");
    ss = srt_create_socket();
    if (ss == SRT_ERROR)
    {
        fprintf(stderr, "srt_socket: %s\n", srt_getlasterror_str());
        return 1;
    }

    printf("srt remote address\n");
    sa.sin_family = AF_INET;
    sa.sin_port = htons(RST_PORT);
    if (inet_pton(AF_INET, "127.0.0.1", &sa.sin_addr) != 1)
    {
        return 1;
    }

    printf("srt setsockflag\n");
    srt_setsockflag(ss, SRTO_SENDER, &yes, sizeof yes);

    // Test deprecated
    //srt_setsockflag(ss, SRTO_STRICTENC, &yes, sizeof yes);

    printf("srt connect\n");
    st = srt_connect(ss, (struct sockaddr*)&sa, sizeof sa);
    if (st == SRT_ERROR)
    {
        fprintf(stderr, "srt_connect: %s\n", srt_getlasterror_str());
        return 1;
    }

    int i;
    for (i = 0; i < 100; i++)
    {
        printf("srt sendmsg2 #%d >> %s\n",i,message);
        st = srt_sendmsg2(ss, message, sizeof message, NULL);
        if (st == SRT_ERROR)
        {
            fprintf(stderr, "srt_sendmsg: %s\n", srt_getlasterror_str());
            return 1;
        }

        usleep(1000);   // 1 ms
    }

    sleep(2);


    printf("srt close\n");
    st = srt_close(ss);
    if (st == SRT_ERROR)
    {
        fprintf(stderr, "srt_close: %s\n", srt_getlasterror_str());
        return 1;
    }

    printf("srt cleanup\n");
    srt_cleanup();
    return 0;
}

void srt_client()
{
    pthread_attr_t attr;
    pthread_t thrd;
    struct sched_param param;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_attr_setstacksize(&attr, 10240);

    param.sched_priority = 130;
    pthread_attr_setschedparam(&attr, &param);

    if (pthread_create(&thrd, &attr, srt_client_proc, NULL) != 0)
    {
        printf("Error: Create sample_isp_proc thread failed!\n");
    }
}

int srt_server_proc()
{
    int ss, st;
    struct sockaddr_in sa;
    int yes = 1;
    struct sockaddr_storage their_addr;

    printf("srt startup\n");
    srt_startup();

    printf("srt socket\n");
    ss = srt_create_socket();
    if (ss == SRT_ERROR)
    {
        fprintf(stderr, "srt_socket: %s\n", srt_getlasterror_str());
        return 1;
    }

    printf("srt bind address\n");
    sa.sin_family = AF_INET;
    sa.sin_port = htons(RST_PORT);
    if (inet_pton(AF_INET, "127.0.0.1", &sa.sin_addr) != 1)
    {
        return 1;
    }

    printf("srt setsockflag\n");
    srt_setsockflag(ss, SRTO_RCVSYN, &yes, sizeof yes);

    printf("srt bind\n");
    st = srt_bind(ss, (struct sockaddr*)&sa, sizeof sa);
    if (st == SRT_ERROR)
    {
        fprintf(stderr, "srt_bind: %s\n", srt_getlasterror_str());
        return 1;
    }

    printf("srt listen\n");
    st = srt_listen(ss, 2);
    if (st == SRT_ERROR)
    {
        fprintf(stderr, "srt_listen: %s\n", srt_getlasterror_str());
        return 1;
    }

    printf("srt accept\n");
    int addr_size = sizeof their_addr;
    int their_fd = srt_accept(ss, (struct sockaddr *)&their_addr, &addr_size);

    int i;
    for (i = 0; i < 100; i++)
    {
        printf("srt recvmsg #%d... ",i);
        char msg[2048];
        st = srt_recvmsg(their_fd, msg, sizeof msg);
        if (st == SRT_ERROR)
        {
            fprintf(stderr, "srt_recvmsg: %s\n", srt_getlasterror_str());
            goto end;
        }

        printf("Got msg of len %d << %s\n", st, msg);
    }

end:
    printf("srt close\n");
    st = srt_close(ss);
    if (st == SRT_ERROR)
    {
        fprintf(stderr, "srt_close: %s\n", srt_getlasterror_str());
        return 1;
    }

    printf("srt cleanup\n");
    srt_cleanup();
    return 0;
}

void srt_server()
{
    pthread_attr_t attr;
    pthread_t thrd;
    struct sched_param param;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_attr_setstacksize(&attr, 10240);

    param.sched_priority = 130;
    pthread_attr_setschedparam(&attr, &param);

    if (pthread_create(&thrd, &attr, srt_server_proc, NULL) != 0)
    {
        printf("Error: Create sample_isp_proc thread failed!\n");
    }
}

#include <rttshell.h>
SHELL_CMD_EXPORT(srt_client, srt_client());
SHELL_CMD_EXPORT(srt_server, srt_server());
