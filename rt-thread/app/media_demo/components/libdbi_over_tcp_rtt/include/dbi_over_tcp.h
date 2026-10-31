#ifndef __DBI_OVER_TCP_H__
#define __DBI_OVER_TCP_H__

struct dbi_tcp_config
{
    int port;
    int *cancel;
    int *is_running;
};

/* dbi_over_tcp main thread */
int *libtcp_dbi_thread(struct dbi_tcp_config *conf);

int tcp_dbi_destroy(void);

#endif
