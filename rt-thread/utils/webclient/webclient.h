/*
 * File      : webclient.h
 * COPYRIGHT (C) 2012-2013, Shanghai Real-Thread Technology Co., Ltd
 *
 * Change Logs:
 * Date           Author       Notes
 * 2013-05-05     Bernard      the first version
 */

#ifndef __WEBCLIENT_H__
#define __WEBCLIENT_H__

#include <rtthread.h>

#define WEBCLIENT_HEADER_BUFSZ    1024
#define BUF_SIZE        2048

typedef unsigned int size_t;

enum WEBCLIENT_STATUS
{
    WEBCLIENT_OK,
    WEBCLIENT_NOMEM,
    WEBCLIENT_NOSOCKET,
    WEBCLIENT_NOBUFFER,
    WEBCLIENT_CONNECT_FAILED,
    WEBCLIENT_DISCONNECT,
    WEBCLIENT_FILE_ERROR
};

enum WEBCLIENT_METHOD
{
    WEBCLIENT_USER_METHOD,
    WEBCLIENT_GET,
    WEBCLIENT_POST,
};

struct webclient_session
{
    /* the session socket */
    int socket;
    /* the response code of HTTP request */
    int response;

    /* content_type of HTTP response */
    char *content_type;
    /* content_length of HTTP response */
    size_t content_length;

    /* last modified timestamp of resource */
    char *last_modified;

    /* server host */
    char *host;
    /* HTTP request */
    char *request;

    /* position of reading */
    unsigned int position;
};

struct webclient_session *webclient_open(const char *URI);
struct webclient_session *webclient_open_header(const char *URI, int method, const char *header, size_t header_sz);
int webclient_close(struct webclient_session *session);

int webclient_read(struct webclient_session *session, unsigned char *buffer, size_t size);
int webclient_write(struct webclient_session *session, const unsigned char *buffer, size_t size);

int webclient_send_header(struct webclient_session *session, int method,
                                   const char *header, size_t header_sz);
int webclient_connect(struct webclient_session *session, const char *URI);


/* hight level APIs for HTTP client */
int webclient_transfer(const char *URI, const char *header, size_t header_sz,
                         const char *data, size_t data_sz,
                         char *result, size_t result_sz);


#ifdef RT_USING_DFS
int webclient_get_file(const char *URI, const char *filename);
int webclient_post_file(const char *URI, const char *filename, const char *form_data);
#endif

#endif
