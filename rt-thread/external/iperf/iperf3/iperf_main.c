/*
 * iperf, Copyright (c) 2014, 2015, The Regents of the University of
 * California, through Lawrence Berkeley National Laboratory (subject
 * to receipt of any required approvals from the U.S. Dept. of
 * Energy).  All rights reserved.
 *
 * If you have questions about your rights to use or distribute this
 * software, please contact Berkeley Lab's Technology Transfer
 * Department at TTD@lbl.gov.
 *
 * NOTICE.  This software is owned by the U.S. Department of Energy.
 * As such, the U.S. Government has been granted for itself and others
 * acting on its behalf a paid-up, nonexclusive, irrevocable,
 * worldwide license in the Software to reproduce, prepare derivative
 * works, and perform publicly and display publicly.  Beginning five
 * (5) years after the date permission to assert copyright is obtained
 * from the U.S. Department of Energy, and subject to any subsequent
 * five (5) year renewals, the U.S. Government is granted for itself
 * and others acting on its behalf a paid-up, nonexclusive,
 * irrevocable, worldwide license in the Software to reproduce,
 * prepare derivative works, distribute copies to the public, perform
 * publicly and display publicly, and to permit others to do so.
 *
 * This code is distributed under a BSD style license, see the LICENSE
 * file for complete information.
 */
#include "iperf_config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif
#include <netinet/tcp.h>
#include <pthread.h>

#include <rttshell.h>

#include "iperf.h"
#include "iperf_api.h"
#include "units.h"
#include "iperf_locale.h"
#include "net.h"

typedef struct
{
    char c_Host[16];
    char m_bps[10];
    char f_format;
    char reserve;
    int p_port;
    int t_second;
    int l_len;
    int i_intvl;
    int s_sndbuf;
    int r_reverse;
}iperf_parm;

static int run(struct iperf_test *test);

void *iperf_tcp_c_entry(void *arg)
{
    struct iperf_test *test;
    iperf_parm *param;

    param = (iperf_parm *)arg;

    test = iperf_new_test();
    if (!test)
        iperf_errexit(NULL, "create new test error - %s", iperf_strerror(i_errno));
    iperf_defaults(test);   /* sets defaults */

    iperf_set_test_role(test, 'c');
    iperf_set_test_server_hostname(test, param->c_Host);

    if (param->r_reverse)
    {
        iperf_set_test_reverse(test, 1);
    }
    test->server_port = param->p_port;
    test->settings->unit_format = param->f_format;
    test->stats_interval = test->reporter_interval = param->i_intvl;
    if ((test->stats_interval < MIN_INTERVAL || test->stats_interval > MAX_INTERVAL) && test->stats_interval != 0) {
        i_errno = IEINTERVAL;
        return NULL;
    }
    test->duration = param->t_second;
    if (test->duration > MAX_TIME) {
        i_errno = IEDURATION;
        return NULL;
    }

    if (param->s_sndbuf > 0)
        test->settings->socket_bufsize = param->s_sndbuf;

    if (iperf_parse_arguments(test) < 0) {
        iperf_err(test, "parameter error - %s", iperf_strerror(i_errno));
        fprintf(stderr, "\n");
        usage_long();
        return NULL;
    }

    if (run(test) < 0)
        iperf_errexit(test, "error - %s", iperf_strerror(i_errno));

    iperf_free_test(test);

    return NULL;
}

void iperf3_tcp_c(char *ip, int port, char format, int t_second, int intvl, int sndbuf, int reverse)
{
    static iperf_parm g_param_tcp_c;
    pthread_t iperf_tcp_c;
    int ret = 0;
    int stacksize = 4096;
    pthread_attr_t attr;

    memset(&g_param_tcp_c, 0, sizeof(g_param_tcp_c));

    ret = pthread_attr_init(&attr);
    if (ret != 0)
    {
        printf("pthread_attr_init failed. \n");
        return;
    }

    ret = pthread_attr_setstacksize(&attr, stacksize);
    if (ret != 0)
    {
        printf("pthread_attr_setstacksize failed. \n");
        return;
    }

    memset(g_param_tcp_c.c_Host, 0, 16);
    memcpy(g_param_tcp_c.c_Host, ip, strlen(ip));
    g_param_tcp_c.p_port = port;
    g_param_tcp_c.f_format = format;
    g_param_tcp_c.t_second = t_second;
    g_param_tcp_c.i_intvl = intvl;
    g_param_tcp_c.s_sndbuf = sndbuf;
    g_param_tcp_c.r_reverse = reverse;

    pthread_create(&iperf_tcp_c, &attr, iperf_tcp_c_entry, &g_param_tcp_c);
    ret = pthread_detach(iperf_tcp_c);
    if (ret != 0)
    {
        printf("detach failed: %d\n", ret);
        return;
    }

    return;
}

#if 0
SHELL_CMD_EXPORT(iperf3_tcp_c, iperf3_tcp_c(ip, port, t_second, interval));
#endif

void *iperf_server_entry(void *arg)
{
    struct iperf_test *test_server;
    iperf_parm *param;

    param = (iperf_parm *)arg;

    test_server = iperf_new_test();
    if (!test_server)
        iperf_errexit(NULL, "create new test error - %s", iperf_strerror(i_errno));
    iperf_defaults(test_server);   /* sets defaults */

    iperf_set_test_role(test_server, 's');
    test_server->server_port = param->p_port;
    test_server->settings->unit_format = param->f_format;
    test_server->stats_interval = test_server->reporter_interval = param->i_intvl;
    if ((test_server->stats_interval < MIN_INTERVAL || test_server->stats_interval > MAX_INTERVAL) && test_server->stats_interval != 0) {
        i_errno = IEINTERVAL;
        return NULL;
    }

    if (iperf_parse_arguments(test_server) < 0) {
        iperf_err(test_server, "parameter error - %s", iperf_strerror(i_errno));
        fprintf(stderr, "\n");
        usage_long();
        return NULL;
    }

    if (run(test_server) < 0)
        iperf_errexit(test_server, "error - %s", iperf_strerror(i_errno));

    iperf_free_test(test_server);

    return NULL;
}

void iperf3_server(int port, char format, int intvl)
{
    static iperf_parm g_param_server;
    int ret = 0;
    int stacksize = 4096;
    pthread_attr_t attr;
    pthread_t iperf_s;
    struct sched_param param;

    memset(&g_param_server, 0, sizeof(g_param_server));

    ret = pthread_attr_init(&attr);
    if (ret != 0)
    {
        printf("pthread_attr_init failed. \n");
        return;
    }

    ret = pthread_attr_setstacksize(&attr, stacksize);
    if (ret != 0)
    {
        printf("pthread_attr_setstacksize failed. \n");
        return;
    }
    param.sched_priority = 120;
    pthread_attr_setschedparam(&attr, &param);

    g_param_server.p_port = port;
    g_param_server.f_format = format;
    g_param_server.i_intvl = intvl;

    pthread_create(&iperf_s, &attr, iperf_server_entry, &g_param_server);
    ret = pthread_detach(iperf_s);
    if (ret != 0)
    {
        printf("detach failed: %d\n", ret);
        return;
    }

    return;
}

#if 0
SHELL_CMD_EXPORT(iperf3_server, iperf3_server(port, interval));
#endif

void *iperf_udp_c_entry(void *arg)
{
    struct iperf_test *test;
    iperf_parm *param;

    param = (iperf_parm *)arg;

    test = iperf_new_test();
    if (!test)
        iperf_errexit(NULL, "create new test error - %s", iperf_strerror(i_errno));
    iperf_defaults(test);   /* sets defaults */

    iperf_set_test_role(test, 'c');
    set_protocol(test, Pudp);
    iperf_set_test_server_hostname(test, param->c_Host);
    if (param->r_reverse)
    {
        iperf_set_test_reverse(test, 1);
    }

    test->server_port = param->p_port;
    test->settings->unit_format = param->f_format;
    test->stats_interval = test->reporter_interval = param->i_intvl;
    if ((test->stats_interval < MIN_INTERVAL || test->stats_interval > MAX_INTERVAL) && test->stats_interval != 0) {
        i_errno = IEINTERVAL;
        return NULL;
    }
    test->duration = param->t_second;
    if (test->duration > MAX_TIME) {
        i_errno = IEDURATION;
        return NULL;
    }

    test->settings->rate = unit_atof_rate(param->m_bps);

    if (param->l_len > 0)
        test->settings->blksize = param->l_len;

    if (iperf_parse_arguments(test) < 0) {
        iperf_err(test, "parameter error - %s", iperf_strerror(i_errno));
        fprintf(stderr, "\n");
        usage_long();
        return NULL;
    }

    if (run(test) < 0)
        iperf_errexit(test, "error - %s", iperf_strerror(i_errno));

    iperf_free_test(test);

    return NULL;
}

void iperf3_udp_c(char *ip, int port, char format, int t_second, char *bps, int len, int intvl, int reverse)
{
    static iperf_parm g_param_udp_c;
    pthread_t iperf_udp_c;
    int ret = 0;
    int stacksize = 4096;
    pthread_attr_t attr;

    memset(&g_param_udp_c, 0, sizeof(g_param_udp_c));

    ret = pthread_attr_init(&attr);
    if (ret != 0)
    {
        printf("pthread_attr_init failed. \n");
        return;
    }

    ret = pthread_attr_setstacksize(&attr, stacksize);
    if (ret != 0)
    {
        printf("pthread_attr_setstacksize failed. \n");
        return;
    }

    memset(g_param_udp_c.c_Host, 0, 16);
    memcpy(g_param_udp_c.c_Host, ip, strlen(ip));
    g_param_udp_c.p_port = port;
    g_param_udp_c.f_format = format;
    g_param_udp_c.t_second = t_second;
    g_param_udp_c.i_intvl = intvl;
    memset(g_param_udp_c.m_bps, 0, 10);
    memcpy(g_param_udp_c.m_bps, bps, strlen(bps));
    g_param_udp_c.l_len = len;
    g_param_udp_c.r_reverse = reverse;

    pthread_create(&iperf_udp_c, &attr, iperf_udp_c_entry, &g_param_udp_c);
    ret = pthread_detach(iperf_udp_c);
    if (ret != 0)
    {
        printf("detach failed: %d\n", ret);
        return;
    }

    return;
}

#if 0
SHELL_CMD_EXPORT(iperf3_udp_c, iperf3_udp_c(ip, port, t_second, bps("100m"), len, interval));

static jmp_buf sigend_jmp_buf;

static void
sigend_handler(int sig)
{
    longjmp(sigend_jmp_buf, 1);
}
#endif

/**************************************************************************/
static int
run(struct iperf_test *test)
{
    int consecutive_errors;

#if 0
    /* Termination signals. */
    iperf_catch_sigend(sigend_handler);
    if (setjmp(sigend_jmp_buf))
    iperf_got_sigend(test);
#endif

    switch (test->role) {
        case 's':
#if 0
        if (test->daemon) {
            int rc = daemon(0, 0);
            if (rc < 0)
            {
                i_errno = IEDAEMON;
                iperf_errexit(test, "error - %s", iperf_strerror(i_errno));
            }
        }
#endif
        consecutive_errors = 0;
        if (iperf_create_pidfile(test) < 0) {
        i_errno = IEPIDFILE;
        iperf_errexit(test, "error - %s", iperf_strerror(i_errno));
        }
        for (;;) {
            if (iperf_run_server(test) < 0) {
                iperf_err(test, "error - %s", iperf_strerror(i_errno));
                ++consecutive_errors;
                if (consecutive_errors >= 5) {
                    iperf_errexit(test, "too many errors, exiting");
                break;
                }
            } else
                consecutive_errors = 0;
            iperf_reset_test(test);
            if (iperf_get_test_one_off(test))
                break;
        }
        iperf_delete_pidfile(test);
            break;
    case 'c':
        if (iperf_run_client(test) < 0)
        iperf_errexit(test, "error - %s", iperf_strerror(i_errno));
            break;
        default:
            usage();
            break;
    }

#if 0
    iperf_catch_sigend(SIG_DFL);
#endif

    return 0;
}

static void set_default_parm(iperf_parm *parm)
{
    if (parm)
    {
        memcpy(parm->m_bps, "1m", 2);
        parm->p_port = PORT;
        parm->f_format = 'm';
        parm->t_second = 10;
        parm->l_len = 1470;
        parm->i_intvl = 1;
        parm->s_sndbuf = 0;
        parm->r_reverse = 0;
    }
}

static void iperf3_help(void)
{
  printf("Usage: iperf3 [-s|-c host] [options]\n"
         "Server or Client:\n"
         "  -p, --port      #         server port to listen on/connect to\n"
         "  -f, --format    [kmgKMG]  format to report: Kbits, Mbits, KBytes, MBytes\n"
         "  -i, --interval  #         seconds between periodic bandwidth reports\n"
         "  -c, --client    <host>    run in client mode, connecting to <host>\n"
         "  -s, --server              run in server mode\n"
         "  -R, --reverse             run in reverse mode (server sends, client receives)\n"
         "  -h, --help                show this message and quit\n"
         "  -u, --udp                 use UDP rather than TCP\n"
         "  -w, --window(tcp only)    set window size / socket buffer size\n"
         "  -t, --time      #         time in seconds to transmit for (default 10 secs)\n"
         "  -b, --bandwidth #[KMG][/#] target bandwidth in bits/sec (0 for unlimited)\n"
         "                            (default 1 Mbit/sec for UDP, unlimited for TCP)\n"
         "                            (optional slash and packet count for burst mode)\n"
         "  -l, --len       #[KMG]    length of buffer to read or write\n"
         "                            (default 128 KB for TCP, 8 KB for UDP)\n");
}

#include <optparse.h>
static struct optparse options;
int iperf3(int argc, char **argv)
{
    int c;
    int server = -1;
    int is_udp = 0;
    int reverse = 0;
    static iperf_parm g_iperf_parm;

    memset(&g_iperf_parm, 0, sizeof(g_iperf_parm));
    set_default_parm(&g_iperf_parm);

    optparse_init(&options, argv);
    while ((c = optparse(&options, "usRc:w:f:i:p:t:b:l:")) != -1)
    {
        switch (c)
        {
            case 's':
                server = 1;
                break;
            case 'u':
                is_udp = 1;
                break;
            case 'c':
                server = 0;
                if (*options.optarg)
                {
                    if (strlen(options.optarg) < 7)
                    {
                        printf("bad parameter! Wrong host ip\n");
                        return -1;
                    }
                    strcpy(g_iperf_parm.c_Host, options.optarg);
                }
                break;
            case 'w':
                if (*options.optarg)
                {
                    g_iperf_parm.s_sndbuf = atoi(options.optarg);
                    if (g_iperf_parm.s_sndbuf > MAX_TCP_BUFFER)
                    {
                        printf("bad parameter! MAX_TCP_BUFFER is 65535\n");
                        return -1;
                    }
                }
                break;
            case 'f':
                if (*options.optarg)
                {
                    g_iperf_parm.f_format = *options.optarg;
                }
                break;
            case 'p':
                if (*options.optarg)
                {
                    g_iperf_parm.p_port = atoi(options.optarg);
                }
                break;
            case 't':
                if (*options.optarg)
                {
                    g_iperf_parm.t_second = atoi(options.optarg);
                }
                break;
            case 'b':
                if (*options.optarg)
                {
                    strcpy(g_iperf_parm.m_bps, options.optarg);
                }
                break;
            case 'l':
                if (*options.optarg)
                {
                    g_iperf_parm.l_len = atoi(options.optarg);
                }
                break;
            case 'i':
                if (*options.optarg)
                {
                    g_iperf_parm.i_intvl = atoi(options.optarg);
                    if (g_iperf_parm.i_intvl > 60)
                    {
                        iperf3_help();
                        return -1;
                    }
                }
                break;
            case 'R':
                g_iperf_parm.r_reverse = 1;
                break;
            case 'h':
            default:
                iperf3_help();
                return 0;
        }
    }

    if (server == 1)
    {
        iperf3_server(g_iperf_parm.p_port, g_iperf_parm.f_format, g_iperf_parm.i_intvl);
    }
    else if (server == 0)
    {
        if (is_udp)
        {
            iperf3_udp_c(g_iperf_parm.c_Host, g_iperf_parm.p_port, g_iperf_parm.f_format,
                            g_iperf_parm.t_second, g_iperf_parm.m_bps, g_iperf_parm.l_len,
                            g_iperf_parm.i_intvl, g_iperf_parm.r_reverse);
        }
        else
        {
            iperf3_tcp_c(g_iperf_parm.c_Host, g_iperf_parm.p_port, g_iperf_parm.f_format,
                            g_iperf_parm.t_second, g_iperf_parm.i_intvl, g_iperf_parm.s_sndbuf, g_iperf_parm.r_reverse);
        }
    }
    else
    {
        iperf3_help();
    }

    return 0;
}
SHELL_CMD_EXPORT(iperf3, iperf3());
