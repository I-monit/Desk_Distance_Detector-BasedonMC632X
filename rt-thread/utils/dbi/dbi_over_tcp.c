/*
 * dbi_over_tcp.c
 *
 *  Created on: 2016.5.5
 *      Author: gaoyb
 */
#include <rtthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include "dbi_over_tcp.h"
#include "di/debug_interface.h"
#include <sys/prctl.h>

struct dbi_over_tcp
{
    int tcp_port;
    int tcp_connection;
    int server_fd;

    struct Debug_Interface *di;
};

static int tcp_send(void *obj, unsigned char *buf, int size)
{
    struct timeval tv;
    fd_set fds;
/* int total_write_bytes; */
/* int write_bytes; */
    int ret = 0, ret_send = 0;
    struct dbi_over_tcp *tcp = (struct dbi_over_tcp *)obj;

    do
    {
        tv.tv_sec  = 1;
        tv.tv_usec = 0;
        FD_ZERO(&fds);
        FD_SET(tcp->tcp_connection, &fds);
        ret = select(tcp->tcp_connection + 1, NULL, &fds, NULL, &tv);
        if (ret > 0)
        {
            if (FD_ISSET(tcp->tcp_connection, &fds))
            {
                ret_send = send(tcp->tcp_connection, buf, size, 0);
            }
        }
        else if (ret == 0)
        {
            continue;
        }
        else
        {
            return -1;
        }
    } while (ret == 0);

    return ret_send;
}

static int tcp_recv(void *obj, unsigned char *buf, int size)
{
    fd_set fds;
    struct timeval tv;
    int read_bytes = 0;
    int ret;
    struct dbi_over_tcp *tcp = (struct dbi_over_tcp *)obj;

    do
    {
        tv.tv_sec  = 2;
        tv.tv_usec = 0;
        FD_ZERO(&fds);
        FD_SET(tcp->tcp_connection, &fds);

        if ((ret = select(tcp->tcp_connection + 1, &fds, NULL, NULL, &tv)) > 0)
        {
            if (FD_ISSET(tcp->tcp_connection, &fds))
            {
                read_bytes = recv(tcp->tcp_connection, buf, size, 0);
                if (read_bytes == 0)
                {
                    return 0; /* timeout */
                }
                else if (read_bytes < 0)
                {
                    return -1; /* error */
                }
            }
        }
        else if (ret == 0)
        {
            return 0; /* timeout */
        }
        else
        {
            return -1; /* error */
        }

    } while (ret == 0);

    return read_bytes;
}

static int tcp_connect(struct dbi_over_tcp *tcp)
{
    int fd;

    /* get connect */
    fd                  = 1;
    tcp->tcp_connection = fd;

    /* since this thread my exits, we'd better use a non-blocking socket. */
    if (listen(tcp->server_fd, 5) == -1)
    {
        perror("socket error");
        return -1;
    }

    return 0;
}

static int tcp_accept(struct dbi_over_tcp *tcp)
{
    struct sockaddr_in server_addr;

    server_addr.sin_family      = AF_INET;
    server_addr.sin_port        = 0;
    server_addr.sin_addr.s_addr = 0;
    unsigned long sin_size      = sizeof(struct sockaddr_in);

    tcp->tcp_connection =
        accept(tcp->server_fd, (struct sockaddr *)&server_addr,
               (socklen_t *)&sin_size);
    if (tcp->tcp_connection < 0)
        return -1;
    return 0;
}

struct dbi_over_tcp *tcp_dbi_create(int port)
{
    struct dbi_over_tcp *tcp;
    struct timeval timeout;
    timeout.tv_sec  = 60;
    timeout.tv_usec = 0;

    tcp = malloc(sizeof(struct dbi_over_tcp));
    if (tcp == RT_NULL)
    {
        rt_kprintf("malloc tcp failed when create dbi object.\n");
        return RT_NULL;
    }
    memset(tcp, 0, sizeof(struct dbi_over_tcp));

    tcp->tcp_port = port;

    int ret, fd;
    int on = 1;
    struct sockaddr_in local_addr;

    fd = socket(AF_INET, SOCK_STREAM, 0);

    if (fd < 0)
    {
        perror("create socket failed");
        free(tcp);
        return NULL;
    }
    tcp->server_fd             = fd;
    local_addr.sin_family      = AF_INET;
    local_addr.sin_port        = htons(port);
    local_addr.sin_addr.s_addr = INADDR_ANY;

    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    ret = bind(fd, (struct sockaddr *)&local_addr, sizeof(local_addr));
    if (ret < 0)
    {
        free(tcp);
        perror("bind error");
        return NULL;
    }

    struct DI_config di_cfg;

    di_cfg.obj  = tcp;
    di_cfg.send = tcp_send;
    di_cfg.recv = tcp_recv;

    tcp->di = DI_create(&di_cfg);

    return tcp;
}

int tcp_dbi_destroy(struct dbi_over_tcp *tcp)
{
    close(tcp->server_fd);
    DI_destroy(tcp->di);

    free(tcp);

    return 0;
}

int *tcp_dbi_thread(struct dbi_tcp_config *conf)
{
    int ret;
    int *exit                = conf->cancel;
    struct dbi_over_tcp *tcp = tcp_dbi_create(conf->port);

    prctl(PR_SET_NAME, "tcp_dbi_thread");

    if (tcp == RT_NULL)
    {
        return 0;
    }
    if (tcp_connect(tcp) < 0)
    {
        tcp_dbi_destroy(tcp);
        return 0;
    }

    while (!*exit)
    {
        if (tcp_accept(tcp) < 0)
        {
            continue;
        }

        while (!*exit)
        {
            ret = DI_handle(tcp->di);
            if (ret == -1)
            {
                close(tcp->tcp_connection);
                break;
            }
        }
    }

    tcp_dbi_destroy(tcp);
    return 0;
}
