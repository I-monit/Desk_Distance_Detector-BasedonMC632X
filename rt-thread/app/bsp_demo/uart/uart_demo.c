#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/prctl.h>
#include <rttshell.h>
static int uart_fd = -1;
void uart_func(void)
{
    int i = 50;
    char buf = 0x31;
    while(i--)
    {
        read(uart_fd, &buf, 1);
        printf("[uart_demo] buf:%x\n", buf);
        write(uart_fd, &buf, 1);
    }
}
void *uart_demo_main(void *param)
{
    prctl(PR_SET_NAME, "uart demo");
    uart_func();
    return NULL;
}
int uart_demo_init(void)
{
    int ret = 0;
    pthread_t threadUart;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_attr_setstacksize(&attr, 10 * 1024);
    uart_fd = open("/dev/uart1", O_RDWR);
    if(uart_fd < 0)
    {
        printf("[uart_demo] open uart1 failed\n");
        return -1;
    }
    ret = pthread_create(&threadUart, &attr, uart_demo_main, NULL);
    if(ret)
    {
        printf("[uart_demo] Error: Create uart_demo_main thread failed!\n");
        return -1;
    }
    return 0;
}