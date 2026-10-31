#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <sys/prctl.h>
#include <fcntl.h>
#include <rttshell.h>
#include "sadc.h"

#define CHANNEL_NUM 4

void *sadc_demo_main(void *param)
{
    unsigned int fd;
    unsigned int raw_data;
    SADC_INFO info;
    unsigned int chn_num = 0;

    info.channel   = 0;
    info.sadc_data = 0;

    prctl(PR_SET_NAME, "sadc demo");
    fd = open( "/dev/sadc", O_RDWR, 0);

    if (!fd)
    {
        printf("[sadc_demo] cann't open the sadc dev\n");
    }

    int i;
    for (i = 0; i < 4; i++)
    {
        info.channel   = chn_num%4;
        ioctl(fd, SADC_CMD_READ_RAW_DATA,
                              &info);  /* //get digit data */
        raw_data = info.sadc_data;
        ioctl(fd, SADC_CMD_READ_VOLT,
                          &info);  /* //get digit data */

        printf("[sadc_demo] channel:%x digt data:%x volt:%d mv\n", info.channel,raw_data,
                   info.sadc_data);
        chn_num++;
        sleep(1);
    }

    close(fd);
    return NULL;
}

void sadc_demo_init(void)
{
    pthread_attr_t attr;
    pthread_t thrd;
    struct sched_param param;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_attr_setstacksize(&attr, 10240);

    param.sched_priority = 80;
    pthread_attr_setschedparam(&attr, &param);

    if (pthread_create(&thrd, &attr, sadc_demo_main, NULL) != 0)
    {
        printf("[sadc_demo] Error: Create sadc demo thread failed!\n");
    }
}
