#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/prctl.h>
#include <rttshell.h>

int sayhelloworld(char name[])
{
    int ret = 0;

    if (name == NULL)
        name = "";

    printf("Hello, World! \n");

    return ret;
}

void *helloworld_main(void *param)
{
    prctl(PR_SET_NAME, "hello");

    while (1)
    {
        sayhelloworld(param);
        break;
    }

    return NULL;
}

void helloworld_init(void)
{
    pthread_attr_t attr;
    pthread_t thrd;
    struct sched_param param;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_attr_setstacksize(&attr, 10240);

    param.sched_priority = 80;
    pthread_attr_setschedparam(&attr, &param);

    if (pthread_create(&thrd, &attr, helloworld_main, NULL) != 0)
    {
        printf("Error: Create sample_isp_proc thread failed!\n");
    }
}

void helloworld(int argc, char *argv[])
{
    helloworld_init();
}

SHELL_CMD_EXPORT(helloworld, helloworld("ZhangYu"));
