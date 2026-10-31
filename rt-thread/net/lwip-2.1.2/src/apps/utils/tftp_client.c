#include <rtthread.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <finsh.h>

#define TFTP_PORT    69
/* opcode */
#define TFTP_RRQ            1     /* read request */
#define TFTP_WRQ            2    /* write request */
#define TFTP_DATA            3    /* data */
#define TFTP_ACK            4    /* ACK */
#define TFTP_ERROR            5    /* error */

#define PING_RCV_TIMEO 5000

rt_uint8_t tftp_buffer[512 + 4];
rt_uint8_t file_name[256];
/* tftp client */
void tftp_get(const char *host, const char *dir, const char *filename)
{
    int fd, sock_fd;
    struct timeval sock_opt;
    int flag = 0;

    sock_opt.tv_sec = PING_RCV_TIMEO/1000;
    sock_opt.tv_usec = (PING_RCV_TIMEO%1000)*1000;

    struct sockaddr_in tftp_addr, from_addr;
    int length = 0;
    socklen_t fromlen;

    /* make local file name */
    rt_snprintf((char *)file_name, sizeof(file_name),
        "%s/%s", dir, filename);

    /* open local file for write */
    fd = open((char *)file_name, O_RDWR | O_CREAT, 0);
    if (fd < 0)
    {
        rt_kprintf("can't open local filename\n");
        return;
    }

    /* connect to tftp server */
    inet_aton(host, (struct in_addr *)&(tftp_addr.sin_addr));
    tftp_addr.sin_family = AF_INET;
    tftp_addr.sin_port = htons(TFTP_PORT);

    sock_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock_fd < 0)
    {
        close(fd);
        rt_kprintf("can't create a socket\n");
        return;
    }

    /* set socket option */
    /* sock_opt = 5000;  5 seconds */
    setsockopt(sock_fd, SOL_SOCKET, SO_RCVTIMEO, &sock_opt, sizeof(sock_opt));

    /* make tftp request */
    tftp_buffer[0] = 0;            /* opcode */
    tftp_buffer[1] = TFTP_RRQ;     /* RRQ */
    length = rt_sprintf((char *)&tftp_buffer[2], "%s", filename) + 2;
    tftp_buffer[length] = 0;
    length ++;
    length += rt_sprintf((char *)&tftp_buffer[length], "%s", "octet");
    tftp_buffer[length] = 0;
    length ++;

    fromlen = sizeof(struct sockaddr_in);
    /* send request */
    sendto(sock_fd, tftp_buffer, length, 0,
        (struct sockaddr *)&tftp_addr, fromlen);

    do
    {
        length = recvfrom(sock_fd, tftp_buffer, sizeof(tftp_buffer), 0,
            (struct sockaddr *)&from_addr, &fromlen);
        if (length > 0)
        {
            if (tftp_buffer[1] == TFTP_ERROR)
            {
                flag = -1;
                break;
            }
            write(fd, (char *)&tftp_buffer[4], length - 4);
            rt_kprintf("#");

            /* make ACK */
            tftp_buffer[0] = 0; tftp_buffer[1] = TFTP_ACK; /* opcode */
            /* send ACK */
            sendto(sock_fd, tftp_buffer, 4, 0,
                (struct sockaddr *)&from_addr, fromlen);
            flag = 1;
        }
    } while (length == 516);


    close(fd);
    close(sock_fd);

    if (flag > 0)
    {
        rt_kprintf("done! %s\n", strerror(errno));
    }
    else
    {
        if (length > 0)
            rt_kprintf("error! %s\n", &tftp_buffer[4]);
        else
            rt_kprintf("error! %s\n", strerror(errno));
        remove((const char *)file_name);
    }

}
FINSH_FUNCTION_EXPORT(tftp_get, tftp_get(ip, local_dir, filename));

void tftp_put(const char *host, const char *dir, const char *filename)
{
    int fd, sock_fd;
    struct timeval sock_opt;

    sock_opt.tv_sec = PING_RCV_TIMEO/1000;
    sock_opt.tv_usec = (PING_RCV_TIMEO%1000)*1000;

    struct sockaddr_in tftp_addr, from_addr;
    int length = 0;
    rt_uint32_t  block_number = 0;
    socklen_t fromlen;

    /* make local file name */
    rt_snprintf((char *)tftp_buffer, sizeof(tftp_buffer),
        "%s/%s", dir, filename);

    /* open local file for write */
    fd = open((char *)tftp_buffer, O_RDONLY, 0);
    if (fd < 0)
    {
        rt_kprintf("can't open local filename\n");
        return;
    }

    /* connect to tftp server */
    inet_aton(host, (struct in_addr *)&(tftp_addr.sin_addr));
    tftp_addr.sin_family = AF_INET;
    tftp_addr.sin_port = htons(TFTP_PORT);

    sock_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock_fd < 0)
    {
        close(fd);
        rt_kprintf("can't create a socket\n");
        return;
    }
    /* set socket option */
    /* sock_opt = 5000; 5 seconds */
    setsockopt(sock_fd, SOL_SOCKET, SO_RCVTIMEO, &sock_opt, sizeof(sock_opt));

    /* make tftp request */
    tftp_buffer[0] = 0;            /* opcode */
    tftp_buffer[1] = TFTP_WRQ;     /* WRQ */
    length = rt_sprintf((char *)&tftp_buffer[2], "%s", filename) + 2;
    tftp_buffer[length] = 0;
    length ++;
    length += rt_sprintf((char *)&tftp_buffer[length], "%s", "octet");
    tftp_buffer[length] = 0;
    length ++;

    fromlen = sizeof(struct sockaddr_in);

    /* send request */
    sendto(sock_fd, tftp_buffer, length, 0,
        (struct sockaddr *)&tftp_addr, fromlen);

    /* wait ACK 0 */
    length = recvfrom(sock_fd, tftp_buffer, sizeof(tftp_buffer), 0,
        (struct sockaddr *)&from_addr, &fromlen);
    if (!(tftp_buffer[0] == 0 &&
        tftp_buffer[1] == TFTP_ACK &&
        tftp_buffer[2] == 0 &&
        tftp_buffer[3] == 0))
    {
        rt_kprintf("tftp server error! %s\n", strerror(errno));
        close(fd);
        close(sock_fd);
        return;
    }

    block_number = 1;

    while (1)
    {
        length = read(fd, (char *)&tftp_buffer[4], 512);
        if (length > 0)
        {
            /* make opcode and block number */
            tftp_buffer[0] = 0; tftp_buffer[1] = TFTP_DATA;
            tftp_buffer[2] = (block_number >> 8) & 0xff;
            tftp_buffer[3] = block_number & 0xff;

            sendto(sock_fd, tftp_buffer, length + 4, 0,
                (struct sockaddr *)&from_addr, fromlen);
        }
        else
        {
            rt_kprintf("done!\n");
            break; /* no data yet */
        }

        /* receive ack */
        length = recvfrom(sock_fd, tftp_buffer, sizeof(tftp_buffer), 0,
            (struct sockaddr *)&from_addr, &fromlen);
        if (length > 0)
        {
            if ((tftp_buffer[0] == 0 &&
                tftp_buffer[1] == TFTP_ACK &&
                tftp_buffer[2] == ((block_number >> 8) & 0xff)) &&
                tftp_buffer[3] == (block_number & 0xff))
            {
                block_number ++;
                rt_kprintf("#");
            }
            else
            {

                rt_kprintf("server respondes with an error\n");
                break;
            }
        }
        else
        {
            rt_kprintf("error! %s\n", strerror(errno));
            break;
        }
    }

    close(fd);
    close(sock_fd);
}
FINSH_FUNCTION_EXPORT(tftp_put, tftp_put(ip, local_dir, filename));

static void tftp_help()
{
  rt_kprintf("Usage: tftp[options]...host\n"
             "-g, --get remote file\n"
             "-p, --put local file \n"
             "-l, --local file name\n"
             "-r, --remote file name\n");
  rt_kprintf("e.g: tftp -g -r filename hostip\n");
  rt_kprintf("e.g: tftp -p -l filename hostip\n");
}

#include <optparse.h>
static struct optparse options;
int cmd_tftp(int argc, char **argv)
{
    int c;
    int get = 0, put = 0;
    char localname[256];
    char remotename[256];
    char dir[256];
    char ip[16];
    int i = 1;

    memset(ip, 0, 16);
    memset(localname, 0, 256);
    memset(remotename, 0, 256);
    while (i < argc)
    {
        if ((!strcmp(argv[i], "-r")) || (!strcmp(argv[i], "-l"))
            || (!strcmp(argv[i], "-gr")) || (!strcmp(argv[i], "-pl")))
        {
            i = i + 2;
        }
        else if((!strcmp(argv[i], "-g")) || (!strcmp(argv[i], "-p")))
        {
            i = i + 1;
        }
        else
        {
            memcpy(ip, argv[i], strlen(argv[i]));
            break;
        }
    }

    if (i >= argc)
    {
        rt_kprintf("bad parameter! e.g: tftp -g -r filename hostip\n");
        rt_kprintf("               e.g: tftp -p -l filename hostip\n");
        return -1;
    }

    optparse_init(&options, argv);
    while ((c = optparse(&options, "gphl:r:")) != -1)
    {
        switch (c)
        {
            case 'g':
                get = 1;
                put = 0;
                break;
            case 'p':
                get = 0;
                put = 1;
                break;
            case 'l':
                if (*options.optarg)
                {
                    strcpy(localname, options.optarg);
                }
                break;
            case 'r':
                if (*options.optarg)
                {
                    strcpy(remotename, options.optarg);
                }
                break;
            case 'h':
            default:
                tftp_help();
                return 0;
        }
    }

    if (inet_addr(ip) == IPADDR_NONE)
    {
        rt_kprintf("bad parameter! Wrong ipaddr! \n");
        return -1;
    }

    getcwd(dir, 256);
    printf("dir %s\n", dir);
    if (get)
    {
        tftp_get(ip, dir, remotename);
    }
    else if (put)
    {
        tftp_put(ip, dir, localname);
    }
    else
    {
        rt_kprintf("bad parameter! e.g: tftp -g -r filename hostip\n");
        rt_kprintf("               e.g: tftp -p -l filename hostip\n");
    }

    return 0;
}
#ifdef FINSH_USING_MSH
FINSH_FUNCTION_EXPORT_ALIAS(cmd_tftp, __cmd_tftp, tftp get/put);
#endif
