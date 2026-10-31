#include <rtthread.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "generalWifi.h"
#ifdef WIFI_USING_CYPRESS
/*
 * wifi fast connect demo for cypress 43438/43455
 */
#define CONN_FILE "/fast_conn.dat"
#define MAC_FMT "%d %d %d %d %d %d"
#define MAC_ARG(x) ((unsigned char *)(x))[0], ((unsigned char *)(x))[1], ((unsigned char *)(x))[2], ((unsigned char *)(x))[3], ((unsigned char *)(x))[4], ((unsigned char *)(x))[5]

WIFI_PMK_LINK_CB pmk_link;
/* save WIFI_PMK_LINK_PARAM to flash or SD */
static int pmk_param_set(WIFI_PMK_LINK_PARAM *param)
{
    int fd, length;
    char buf_write[128] = {0};

    sprintf(buf_write, "%s "MAC_FMT" %d %d\n", param->pmk, MAC_ARG(param->bssid), param->channel, param->security);
    /* rt_kprintf("%s\n", buf_write); */

    fd = open(CONN_FILE, O_WRONLY | O_CREAT, 0);
    if (fd < 0)
    {
        /* rt_kprintf("open CONN_FILE for write failed\n"); */
        return -1;
    }

    length = write(fd, buf_write, strlen(buf_write));
    if (length != strlen(buf_write))
    {
        /* rt_kprintf("write data failed\n"); */
        close(fd);
        return -2;
    }
    close(fd);

    return 0;
}

/* get WIFI_PMK_LINK_PARAM from flash or SD */
static int pmk_param_get(WIFI_PMK_LINK_PARAM *param)
{
    int fd, length, i = 0;
    char buf_read[128] = {0};
    const char *delim = " ";

    memset(param, 0, sizeof(WIFI_PMK_LINK_PARAM));

    fd = open(CONN_FILE, O_RDONLY, 0);
    if (fd < 0)
    {
        /* rt_kprintf("open CONN_FILE for read failed\n"); */
        return -1;
    }
    length = read(fd, buf_read, sizeof(buf_read));
    /* rt_kprintf("%s %s\n", __func__, buf_read); */
    strncpy(param->pmk, strtok(buf_read, delim), 64);

    while (i < 6)
        param->bssid[i++] = atoi(strtok(NULL, delim));

    param->channel = atoi(strtok(NULL, delim));
    param->security = atoi(strtok(NULL, delim));
    /* rt_kprintf("%s "MAC_FMT" %01x %08x\n", param->pmk, MAC_ARG(param->bssid), param->channel, param->security); */
    close(fd);

    return 0;
}

void user_demo(void)
{
extern int wifi_conn_ap(char *ssid, char *wifi_passwd);

    /* enable wifi fast connect */
    pmk_link.param_set = pmk_param_set;
    pmk_link.param_get = pmk_param_get;
    w_register_pmk_link_cb(pmk_link);

    /* rt_kprintf("%s-%d start wifi_conn_ap...\n", __func__, __LINE__); */
    wifi_conn_ap("ASUS-AC750", "12345678");
}
#endif /* WIFI_USING_CYPRESS */
