/*
 * File      : webclient.c
 * COPYRIGHT (C) 2012-2013, Shanghai Real-Thread Technology Co., Ltd
 *
 * Change Logs:
 * Date           Author       Notes
 * 2013-05-05     Bernard      the first version
 * 2013-06-10     Bernard      fix the slow speed issue when download file.
 */

#include "webclient.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <netdb.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <arpa/inet.h>

#define WEBCLIENT_SOCKET_TIMEO    6000 /* 6 second */
#define BUF_SZ  2048

extern long int strtol(const char *nptr, char **endptr, int base);

static char *webclient_header_skip_prefix(char *line, const char *prefix)
{
    char *ptr = line;

    while (1)
    {
        if (*prefix == '\0')
            break;
        if (toupper((unsigned char)(*ptr)) != toupper((unsigned char)(*prefix)))
            return RT_NULL;

        ptr++;
        prefix++;
    }

    /* skip whitespace */
    while (*ptr && (*ptr == ' ' || *ptr == '\t'))
        ptr += 1;

    line = ptr;
    ptr = strstr(line, "\r\n");
    if (ptr != RT_NULL)
    {
        *ptr = '\0';
    }

    return line;
}

/*
 * When a request has been sent, we can expect mime headers to be
 * before the data.  We need to read exactly to the end of the headers
 * and no more data.  This readline reads a single char at a time.
 */
static int webclient_read_line(int socket, char *buffer, int size)
{
    int rc;
    char *ptr = buffer;
    int count = 0;

    /* Keep reading until we fill the buffer. */
    while (count < size)
    {
        rc = recv(socket, ptr, 1, 0);
        if (rc <= 0)
            return rc;

        if (*ptr == '\n')
        {
            ptr++;
            count++;
            break;
        }

        /* increment after check for cr.  Don't want to count the cr. */
        count++;
        ptr++;
    }

    /* add terminate string */
    *ptr = '\0';

    return count;
}

/*
 * resolve server address
 * @param server the server sockaddress
 * @param url the input URL address, for example, http://www.rt-thread.org/index.html
 * @param host_addr the buffer pointer to save server host address
 * @param request the pointer to point the request url, for example, /index.html
 *
 * @return 0 on resolve server address OK, others failed
 */
static int webclient_resolve_address(struct sockaddr_in *server, const char *url, char *host_addr, char **request)
{
    char *ptr;
    char port[6] = "80"; /* default port of 80(webclient) */
    int i = 0, is_domain;
    struct hostent *hptr;

    /* strip webclient: */
    ptr = strchr(url, ':');
    if (ptr != NULL)
    {
        url = ptr + 1;
    }

    /* URL must start with double forward slashes. */
    if ((url[0] != '/') || (url[1] != '/'))
        return -1;

    url += 2;
    is_domain = 0;
    i = 0;
    /* allow specification of port in URL like http://www.server.net:8080/ */
    while (*url)
    {
        if (*url == '/')
            break;
        if (*url == ':')
        {
            unsigned char w;

            for (w = 0; w < 5 && url[w + 1] != '/' && url[w + 1] != '\0'; w++)
                port[w] = url[w + 1];

            /* get port ok */
            port[w] = '\0';
            url += w + 1;
            break;
        }

        if ((*url < '0' || *url > '9') && *url != '.')
            is_domain = 1;
        host_addr[i++] = *url;
        url++;
    }
    *request = (char *)url;

    /* get host addr ok. */
    host_addr[i] = '\0';

    if (is_domain)
    {
        /* resolve the host name. */
        hptr = gethostbyname(host_addr);
        if (hptr == 0)
        {
            rt_kprintf("WEBCLIENT: failed to resolve domain '%s'\n", host_addr);
            return -1;
        }
        memcpy(&server->sin_addr, *hptr->h_addr_list, sizeof(server->sin_addr));
    }
    else
    {
        inet_aton(host_addr, (struct in_addr *)&(server->sin_addr));
    }
    /* set the port */
    server->sin_port = htons((int) strtol(port, NULL, 10));
    server->sin_family = AF_INET;

    return 0;
}

int webclient_send_header(struct webclient_session *session, int method,
                          const char *header, size_t header_sz)
{
    int rc = WEBCLIENT_OK;
    unsigned char *header_buffer = RT_NULL, *header_ptr;

    if (header == RT_NULL)
    {
        header_buffer = rt_malloc(WEBCLIENT_HEADER_BUFSZ);
        if (header_buffer == RT_NULL)
        {
            rc = -WEBCLIENT_NOMEM;
            goto __exit;
        }

        header_ptr = header_buffer;
        header_ptr += rt_snprintf((char *)header_ptr, WEBCLIENT_HEADER_BUFSZ - (header_ptr - header_buffer),
                                  "GET %s HTTP/1.1\r\n", session->request ? session->request : "/");
        header_ptr += rt_snprintf((char *)header_ptr, WEBCLIENT_HEADER_BUFSZ - (header_ptr - header_buffer),
                                  "Host: %s\r\n", session->host);
        header_ptr += rt_snprintf((char *)header_ptr, WEBCLIENT_HEADER_BUFSZ - (header_ptr - header_buffer),
                                  "User-Agent: RT-Thread HTTP Agent\r\n\r\n");

        webclient_write(session, header_buffer, header_ptr - header_buffer);
    }
    else
    {
        if (method != WEBCLIENT_USER_METHOD)
        {
            header_buffer = rt_malloc(WEBCLIENT_HEADER_BUFSZ);
            if (header_buffer == RT_NULL)
            {
                rc = -WEBCLIENT_NOMEM;
                goto __exit;
            }

            header_ptr = header_buffer;

            if (strstr(header, "HTTP/1.") == RT_NULL)
            {
                if (method == WEBCLIENT_GET)
                    header_ptr += rt_snprintf((char *)header_ptr, WEBCLIENT_HEADER_BUFSZ - (header_ptr - header_buffer),
                                              "GET %s HTTP/1.1\r\n", session->request ? session->request : "/");
                else if (method == WEBCLIENT_POST)
                    header_ptr += rt_snprintf((char *)header_ptr, WEBCLIENT_HEADER_BUFSZ - (header_ptr - header_buffer),
                                              "POST %s HTTP/1.1\r\n", session->request ? session->request : "/");
            }

            if (strstr(header, "Host:") == RT_NULL)
            {
                header_ptr += rt_snprintf((char *)header_ptr, WEBCLIENT_HEADER_BUFSZ - (header_ptr - header_buffer),
                                          "Host: %s\r\n", session->host);
            }

            if (strstr(header, "User-Agent:") == RT_NULL)
            {
                header_ptr += rt_snprintf((char *)header_ptr, WEBCLIENT_HEADER_BUFSZ - (header_ptr - header_buffer),
                                          "User-Agent: RT-Thread HTTP Agent\r\n");
            }

            if (strstr(header, "Accept: ") == RT_NULL)
            {
                header_ptr += rt_snprintf((char *)header_ptr, WEBCLIENT_HEADER_BUFSZ - (header_ptr - header_buffer),
                                          "Accept: */*\r\n");
            }

            if ((WEBCLIENT_HEADER_BUFSZ - (header_ptr - header_buffer)) < (int)header_sz + 3)
            {
                rc = -WEBCLIENT_NOBUFFER;
                goto __exit;
            }

            /* append user's header */
            memcpy(header_ptr, header, header_sz);
            header_ptr += header_sz;
            header_ptr += rt_snprintf((char *)header_ptr, WEBCLIENT_HEADER_BUFSZ - (header_ptr - header_buffer),
                                      "\r\n");

            webclient_write(session, header_buffer, header_ptr - header_buffer);
        }
        else
        {
            webclient_write(session, (unsigned char *)header, header_sz);
        }
    }

__exit:
    rt_free(header_buffer);
    return rc;
}

static int webclient_handle_response(struct webclient_session *session)
{
    int rc;
    char mimeBuffer[128], *mime_ptr;

    /* We now need to read the header information */
    while (1)
    {
        int i;

        /* read a line from the header information. */
        rc = webclient_read_line(session->socket, mimeBuffer, 128);
        if (rc < 0)
            return rc;

        /* End of headers is a blank line.  exit. */
        if (rc == 0)
            break;
        if ((rc == 2) && (mimeBuffer[0] == '\r'))
            break;

        mime_ptr = webclient_header_skip_prefix(mimeBuffer, "HTTP/1.");
        if (mime_ptr != RT_NULL)
        {
            mime_ptr += 1;
            while (*mime_ptr && (*mime_ptr == ' ' || *mime_ptr == '\t'))
                mime_ptr ++;
            /* Terminate string after status code */
            for (i = 0; ((mime_ptr[i] != ' ') && (mime_ptr[i] != '\t')); i++)
                ;
            mime_ptr[i] = '\0';

            session->response = (int)strtol(mime_ptr, RT_NULL, 10);
        }
        mime_ptr = webclient_header_skip_prefix(mimeBuffer, "Last-Modified:");
        if (mime_ptr != RT_NULL)
        {
            session->last_modified = rt_strdup(mime_ptr);
        }
        mime_ptr = webclient_header_skip_prefix(mimeBuffer, "Content-Type:");
        if (mime_ptr != RT_NULL)
        {
            session->content_type = rt_strdup(mime_ptr);
        }
        mime_ptr = webclient_header_skip_prefix(mimeBuffer, "Content-Length:");
        if (mime_ptr != RT_NULL)
        {
            session->content_length = (int)strtol(mime_ptr, RT_NULL, 10);
        }
    }

    return session->response;
}

/*
 This is the main HTTP client connect work.  Makes the connection
 and handles the protocol and reads the return headers.  Needs
 to leave the stream at the start of the real data.
*/
int webclient_connect(struct webclient_session *session, const char *URI)
{
    int rc;
    int socket_handle;
    int timeout = WEBCLIENT_SOCKET_TIMEO;
    struct sockaddr_in server;
    char *request, host_addr[32];

    RT_ASSERT(session != RT_NULL);

    /* initialize the socket of session */
    session->socket = -1;

    /* Check valid IP address and URL */
    rc = webclient_resolve_address(&server, URI, &host_addr[0], &request);
    if (rc != WEBCLIENT_OK)
        return rc;

    /* copy host address */
    session->host = rt_strdup(host_addr);
    if (*request)
        session->request = rt_strdup(request);
    else
        session->request = RT_NULL;

    socket_handle = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socket_handle < 0)
        return -WEBCLIENT_NOSOCKET;

    /* set recv timeout option */
    setsockopt(socket_handle, SOL_SOCKET, SO_RCVTIMEO, (void *)&timeout, sizeof(timeout));
    setsockopt(socket_handle, SOL_SOCKET, SO_SNDTIMEO, (void *)&timeout, sizeof(timeout));

    if (connect(socket_handle, (struct sockaddr *)&server, sizeof(struct sockaddr)) != 0)
    {
        /* connect failed, close socket handle */
        close(socket_handle);
        return -WEBCLIENT_CONNECT_FAILED;
    }

    session->socket = socket_handle;
    return WEBCLIENT_OK;
}

struct webclient_session *webclient_open(const char *URI)
{
    struct webclient_session *session;

    /* create session */
    session = (struct webclient_session *) rt_calloc(1, sizeof(struct webclient_session));
    if (session == RT_NULL)
        return RT_NULL;

    if (webclient_connect(session, URI) < 0)
    {
        /* connect to webclient server failed. */
        webclient_close(session);
        return RT_NULL;
    }

    if (webclient_send_header(session, WEBCLIENT_GET, RT_NULL, 0) != WEBCLIENT_OK)
    {
        /* connect to webclient server failed. */
        webclient_close(session);
        return RT_NULL;
    }

    /* handle the response header of webclient server */
    webclient_handle_response(session);

    /* open successfully */
    return session;
}

struct webclient_session *webclient_open_header(const char *URI, int method, const char *header, size_t header_sz)
{
    struct webclient_session *session;

    /* create session */
    session = (struct webclient_session *) rt_calloc(1, sizeof(struct webclient_session));
    if (session == RT_NULL)
        return RT_NULL;

    memset(session, 0, sizeof(struct webclient_session));

    if (webclient_connect(session, URI) < 0)
    {
        /* connect to webclient server failed. */
        webclient_close(session);
        return RT_NULL;
    }

    /* write request header */
    if (webclient_send_header(session, method, header, header_sz) != WEBCLIENT_OK)
    {
        /* send request header failed. */
        webclient_close(session);
        return RT_NULL;
    }

    /* handle the response header of webclient server */
    if (method == WEBCLIENT_GET)
    {
        webclient_handle_response(session);
    }

    /* open successfully */
    return session;
}

int webclient_read(struct webclient_session *session, unsigned char *buffer, size_t length)
{
    int bytesRead = 0;
    int totalRead = 0;
    int left = length;

    RT_ASSERT(session != RT_NULL);
    if (session->socket < 0)
        return -WEBCLIENT_DISCONNECT;

    /*
     * Read until: there is an error, we've read "size" bytes or the remote
     * side has closed the connection.
     */
    do
    {
        bytesRead = recv(session->socket, buffer + totalRead, left, 0);
        if (bytesRead <= 0)
            break;

        left -= bytesRead;
        totalRead += bytesRead;
    } while (left);

    return totalRead;
}

int webclient_write(struct webclient_session *session, const unsigned char *buffer, size_t length)
{
    int bytesWrite = 0;
    int totalWrite = 0;
    int left = length;

    RT_ASSERT(session != RT_NULL);
    if (session->socket < 0)
        return -WEBCLIENT_DISCONNECT;

    /*
     * Send all of data on the buffer.
     */
    do
    {
        bytesWrite = send(session->socket, buffer + totalWrite, left, 0);
        if (bytesWrite <= 0)
            break;

        left -= bytesWrite;
        totalWrite += bytesWrite;
    } while (left);

    return totalWrite;
}

/*
 * close a webclient client session.
 */
int webclient_close(struct webclient_session *session)
{
    RT_ASSERT(session != RT_NULL);

    if (session->socket >= 0)
        close(session->socket);
    rt_free(session->content_type);
    rt_free(session->last_modified);
    rt_free(session->host);
    rt_free(session->request);
    rt_free(session);

    return 0;
}

/*
 * High level APIs for webclient client
 */
int webclient_transfer(const char *URI, const char *header, size_t header_sz,
                       const char *data, size_t data_sz,
                       char *result, size_t result_sz)
{
    int rc = 0;
    size_t length, total_read = 0;
    unsigned char *buf_ptr;
    struct webclient_session *session = RT_NULL;

    /* create session */
    session = (struct webclient_session *) rt_calloc(1, sizeof(struct webclient_session));
    if (session == RT_NULL)
    {
        rc = -WEBCLIENT_NOMEM;
        goto __exit;
    }

    rc = webclient_connect(session, URI);
    if (rc < 0)
        goto __exit;

    /* send header */
    rc = webclient_send_header(session, WEBCLIENT_POST, header, header_sz);
    if (rc < 0)
        goto __exit;

    /* POST data */
    webclient_write(session, (unsigned char *)data, data_sz);

    /* handle the response header of webclient server */
    webclient_handle_response(session);
    if (session->response != 200)
    {
        rt_kprintf("HTTP response: %d\n", session->response);
        goto __exit;
    }

    /* read response data */
    if (result == RT_NULL)
        goto __exit;

    if (session->content_length == 0)
    {
        total_read = 0;
        buf_ptr = (unsigned char *)result;
        while (1)
        {
            /* read result */
            length = webclient_read(session, buf_ptr + total_read, result_sz - total_read);
            if (length <= 0)
                break;

            buf_ptr += length;
            total_read += length;
        }
    }
    else
    {
        buf_ptr = (unsigned char *)result;
        for (total_read = 0; total_read < session->content_length; )
        {
            length = webclient_read(session, buf_ptr,
                                    session->content_length - total_read > BUF_SZ ? BUF_SZ:session->content_length - total_read);

            if (length <= 0)
                break;

            buf_ptr += length;
            total_read += length;
        }
    }

__exit:
    if (session != RT_NULL)
        webclient_close(session);
    if (rc < 0)
        return rc;

    return total_read;
}

#ifdef RT_USING_DFS
#define DOWNLOADING_TIME        (RT_TICK_PER_SECOND/2)
#define LAST_MODIFIED_FILE        "/fw.ver"

int webclient_get_file(const char *URI, const char *filename)
{
    int fd = -1, fw_fd = -1;
    int continuous_pos = 0;
    rt_tick_t tick1, tick2;
    char *last_modified = RT_NULL;
    int last_modified_len = 0;
    size_t offset = 0;
    size_t length;
    rt_uint8_t *ptr = RT_NULL;
    rt_uint8_t *buffer = RT_NULL;
    char *name_buf = RT_NULL;
    struct webclient_session *session = RT_NULL;

    name_buf = (char *)rt_malloc(32);
    if (name_buf == RT_NULL)
    {
        rt_kprintf("out of memory, can't malloc for name_buf\n");
        goto __exit;
    }
    /* judge that temporary file exists or not */
    rt_sprintf(name_buf, "%s.tmp", filename);
    if ((fd = open(name_buf, O_RDONLY, 0)) < 0)
    {
        int mo_fd = -1;

        /* No .tmp file, clear file which stores last-modified value */
        mo_fd = open(LAST_MODIFIED_FILE, O_RDWR | O_CREAT | O_TRUNC, 0);
        if (mo_fd < 0)
            rt_kprintf("Truncate or Create %s failed\n", name_buf);
        else
            close(mo_fd);
    }
    else
    {
        close(fd);
        fd = -1;
    }

    if ((fd = open(name_buf, O_RDWR | O_CREAT, 0)) < 0)
    {
        rt_kprintf("Open file[%s] failed\n", name_buf);
        goto __exit;
    }
    /* get continuous position */
    continuous_pos = lseek(fd, 0, SEEK_END);
    rt_kprintf("continuous position byte[%d]\n", continuous_pos);
    if (continuous_pos < 0)
    {
        rt_kprintf("seek breakpoint position failed\n");
        goto __exit;
    }

    buffer = ptr = (rt_uint8_t *)rt_malloc(BUF_SIZE);
    if (ptr == RT_NULL)
    {
        rt_kprintf("out of memory\n");
        goto __exit;
    }
    while (1)
    {
        buffer = ptr;
        ptr += rt_snprintf((char *)ptr, BUF_SIZE - (ptr - buffer),
                           "Host: %s\r\n", "www.rt-thread.com");
        /* After lseek the end of file, add "range" field into webclient session structure. */
        /* if (continuous_pos != 0) */
        {
            /* continuous_pos = lseek(fd, 0, SEEK_END); */
            /* rt_kprintf("continuous position byte[%d]\n", continuous_pos); */
            /* if (continuous_pos < 0) */
            /* { */
            /* rt_kprintf("seek breakpoint position failed\n"); */
            /* goto __exit; */
            /* } */
            ptr += rt_snprintf((char *)ptr, BUF_SIZE - (ptr - buffer),
                               "Range: bytes=%d-\r\n", continuous_pos);
            /* rt_kprintf("%d-%d\n", continuous_pos, continuous_pos+BUF_SIZE-1); */
        }
        ptr += rt_snprintf((char *)ptr, BUF_SIZE - (ptr - buffer),
                           "Accept: */*\r\n");

        session = webclient_open_header(URI, WEBCLIENT_GET, (char *)buffer, ptr - buffer);
        ptr = buffer;
        if (session == RT_NULL)
        {
            rt_kprintf("open website failed\n");
            goto __exit;
        }
        if (session->response != 200 && session->response != 206)
        {
            rt_kprintf("wrong response: %d\n", session->response);
            goto __exit;
        }

        if (session->last_modified == RT_NULL)
            break;

        /* record last modified timestamp of firmware */
        if ((fw_fd = open(LAST_MODIFIED_FILE, O_RDWR | O_CREAT, 0)) < 0)
        {
            /* Open file failed */
            rt_kprintf("Open file[%s] with O_CREAT failed\n", LAST_MODIFIED_FILE);
            rt_kprintf("Last-Modified in server: %s", session->last_modified);
            break;
        }

        /* Open successfully */
        rt_kprintf("Open file[%s] successfully\n", LAST_MODIFIED_FILE);
        last_modified_len = strlen(session->last_modified);
        if (last_modified_len != lseek(fw_fd, 0, SEEK_END))
        {
            if (fw_fd >= 0)
            {
                close(fw_fd);
            }
            fw_fd = open(LAST_MODIFIED_FILE, O_RDWR | O_TRUNC, 0);
            if (fw_fd < 0)
            {
                rt_kprintf("Open file[%s] with O_CREAT O_TRUNC failed\n", LAST_MODIFIED_FILE);
                goto __exit;
            }

            if (write(fw_fd, (char *)session->last_modified, last_modified_len)\
                    != last_modified_len)
            {
                rt_kprintf("Write Last-Modified to file failed\n");
            }

            rt_kprintf("Update Last-Modified in file\n");
        }

        last_modified = (char *)rt_malloc(((last_modified_len + 3)/4)*4 + 4);
        if (last_modified == RT_NULL)
        {
            rt_kprintf("No memory, compare Last-Modified failed\n");
            goto __exit;
        }
        rt_memset(last_modified, 0, ((last_modified_len + 3)/4)*4 + 4);
        lseek(fw_fd, 0, SEEK_SET);
        if (last_modified_len != read(fw_fd, (char *)last_modified, last_modified_len))
        {
            rt_kprintf("Read Last-Modified in file failed\n");
            rt_kprintf("Last-Modified in file:\n%s\n", last_modified);

            if (last_modified != RT_NULL)
                rt_free(last_modified);
            goto __exit;
        }

        rt_kprintf("----\n");
        rt_kprintf("Last-Modified in file:\n%s\n", last_modified);
        rt_kprintf("Last-Modified in server:\n%s\n", session->last_modified);
        rt_kprintf("----\n");
        /* firmware was updated */
        if (strncmp(last_modified, session->last_modified, last_modified_len) != 0)
        {
            rt_kprintf("firmware was updated in server\n");
            rt_kprintf("Request to download new firmware\n");
            /* Resource modified, reset continuous position */
            continuous_pos = 0;
            lseek(fd, 0, SEEK_SET);
            if (session)
            {
                webclient_close(session);
                session = RT_NULL;
            }
            continue;
        }

        rt_free(last_modified);
        last_modified = RT_NULL;
        if (fw_fd >= 0)
        {
            close(fw_fd);
            fw_fd = -1;
        }
        break;
    }

    rt_kprintf("Downloading...\n");
    tick1 = rt_tick_get();
    if (session->content_length == 0)
    {
        rt_kprintf("content-length=0\n");
        while (1)
        {
            length = webclient_read(session, ptr, BUF_SIZE);
            if (length > 0)
                write(fd, ptr, length);
            else if (length == 0)
            {
                if (fd >= 0)
                {
                    close(fd);
                    fd = -1;
                }
                if (unlink(filename) != 0)
                {
                    rt_kprintf("delete %s failed\n", filename);
                }
                rename(name_buf, filename);
                rt_kprintf("\nDownload finished\n");

                /* delelte file which stores last-modified value */
                if (unlink(LAST_MODIFIED_FILE) != 0)
                {
                    int last_fd = -1;

                    rt_kprintf("delete %s failed\n", LAST_MODIFIED_FILE);
                    if ((last_fd = open(LAST_MODIFIED_FILE, O_TRUNC, 0)) < 0)
                        rt_kprintf("Truncate %s failed\n", LAST_MODIFIED_FILE);
                    else close(last_fd);
                }

                break;
            }

            tick2 = rt_tick_get();
            if (tick2 > tick1)
            {
                if ((tick2 - tick1) >= DOWNLOADING_TIME)
                {
                    rt_kprintf("\r%d of %d %d%%", offset, session->content_length, offset*100/session->content_length);/* rt_kprintf("."); */
                    tick1 =  rt_tick_get();
                }
            }
            else if (tick2 < tick1)
            {
                if ((RT_TICK_MAX - (tick1 - tick2)) >= DOWNLOADING_TIME)
                {
                    rt_kprintf("\r%d of %d %d%%", offset, session->content_length, offset*100/session->content_length);/* rt_kprintf("."); */
                    tick1 =  rt_tick_get();
                }
            }
        }
    }
    else
    {
        for (offset = 0; offset < session->content_length; )
        {
            length = webclient_read(session, ptr,
                                    session->content_length - offset > BUF_SIZE ? BUF_SIZE:session->content_length - offset);

            if (length > 0)
            {
                write(fd, ptr, length);
                close(fd);
                fd = open(name_buf, O_WRONLY | O_APPEND, 0);
                if (fd < 0)
                {
                    rt_kprintf("Opening %s to write data sequentially is failed\n");
                    goto __exit;
                }
            }
            else break;

            offset += length;

            tick2 = rt_tick_get();
            if (tick2 > tick1)
            {
                if ((tick2 - tick1) >= DOWNLOADING_TIME)
                {
                    rt_kprintf("\r%d of %d %d%%", offset, session->content_length, offset*100/session->content_length);/* rt_kprintf("."); */
                    tick1 =  rt_tick_get();
                }
            }
            else if (tick2 < tick1)
            {
                if ((RT_TICK_MAX - (tick1 - tick2)) >= DOWNLOADING_TIME)
                {
                    rt_kprintf("\r%d of %d %d%%", offset, session->content_length, offset*100/session->content_length);/* rt_kprintf("."); */
                    tick1 =  rt_tick_get();
                }
            }
        }

        if (offset == session->content_length)
        {
            if (fd >= 0)
            {
                close(fd);
                fd = -1;
            }
            if (unlink(filename) != 0)
                rt_kprintf("delete %s failed\n", filename);
            if (rename(name_buf, filename) != 0)
                rt_kprintf("rename %s failed\n", name_buf);

            rt_kprintf("\nDownload finished\n");

            /* delelte file which stores last-modified value */
            if (unlink(LAST_MODIFIED_FILE) != 0)
            {
                int last_fd = -1;

                rt_kprintf("delete %s failed\n", LAST_MODIFIED_FILE);
                if ((last_fd = open(LAST_MODIFIED_FILE, O_TRUNC, 0)) < 0)
                    rt_kprintf("Truncate %s failed\n", LAST_MODIFIED_FILE);
                else close(last_fd);
            }
            /* if (session != RT_NULL) webclient_close(session); */
            /* session = RT_NULL; */
        }
    }

__exit:
    if (fd >= 0)
        close(fd);
    if (fw_fd >= 0)
        close(fw_fd);
    if (session != RT_NULL)
        webclient_close(session);
    if (buffer != RT_NULL)
        rt_free(buffer);
    if (name_buf != RT_NULL)
        rt_free(name_buf);

    return 0;
}

int webclient_post_file(const char *URI, const char *filename, const char *form_data)
{
    size_t length;
    char boundary[60];
    int fd = -1, rc = WEBCLIENT_OK;
    char *header = RT_NULL, *header_ptr;
    unsigned char *buffer = RT_NULL, *buffer_ptr;
    struct webclient_session *session = RT_NULL;

    fd = open(filename, O_RDONLY, 0);
    if (fd < 0)
    {
        rc = -WEBCLIENT_FILE_ERROR;
        goto __exit;
    }
    /* get the size of file */
    length = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);

    buffer = rt_malloc(BUF_SIZE);
    if (buffer == RT_NULL)
    {
        rc = -WEBCLIENT_NOMEM;
        goto __exit;
    }

    session = (struct webclient_session *) rt_calloc(1, sizeof(struct webclient_session));
    if (session == RT_NULL)
    {
        rc = -WEBCLIENT_NOMEM;
        goto __exit;
    }

    rc = webclient_connect(session, URI);
    if (rc < 0)
        goto __exit;

    header = (char *) rt_malloc(WEBCLIENT_HEADER_BUFSZ);
    if (header == RT_NULL)
    {
        rc = -WEBCLIENT_NOMEM;
        goto __exit;
    }
    header_ptr = header;

    /* build boundary */
    rt_snprintf(boundary, sizeof(boundary), "----------------------------%012d",
                rt_tick_get());

    /* build encapsulated mime_multipart information*/
    buffer_ptr = buffer;
    /* first boundary */
    buffer_ptr += rt_snprintf((char *)buffer_ptr, BUF_SIZE - (buffer_ptr - buffer),
                              "--%s\r\n", boundary);
    buffer_ptr += rt_snprintf((char *)buffer_ptr, BUF_SIZE - (buffer_ptr - buffer),
                              "Content-Disposition: form-data; %s\r\n", form_data);
    buffer_ptr += rt_snprintf((char *)buffer_ptr, BUF_SIZE - (buffer_ptr - buffer),
                              "Content-Type: application/octet-stream\r\n\r\n");
    /* calculate content-length */
    length += buffer_ptr - buffer;
    length += strlen(boundary) + 6; /* add the last boundary */

    /* build header for upload */
    header_ptr += rt_snprintf(header_ptr, WEBCLIENT_HEADER_BUFSZ - (header_ptr - header),
                              "Content-Length: %d\r\n", length);
    header_ptr += rt_snprintf(header_ptr, WEBCLIENT_HEADER_BUFSZ - (header_ptr - header),
                              "Content-Type: multipart/form-data; boundary=%s\r\n", boundary);
    /* send header */
    rc = webclient_send_header(session, WEBCLIENT_POST, header, header_ptr - header);
    if (rc < 0)
        goto __exit;

    /* send mime_multipart */
    webclient_write(session, buffer, buffer_ptr - buffer);

    /* send file data */
    while (1)
    {
        length = read(fd, buffer, BUF_SIZE);
        if (length <= 0)
            break;
        webclient_write(session, buffer, length);
    }

    /* send last boundary */
    rt_snprintf((char *)buffer, BUF_SIZE, "\r\n--%s--\r\n", boundary);
    webclient_write(session, buffer, strlen(boundary) + 6);

__exit:
    if (fd >= 0)
        close(fd);
    if (session != RT_NULL)
        webclient_close(session);
    if (buffer != RT_NULL)
        rt_free(buffer);
    if (header != RT_NULL)
        rt_free(header);

    return 0;
}
#endif /* RT_USING_DFS */

