#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/prctl.h>
#include <sys/ioctl.h>
#include <rttshell.h>

#include "pwm.h"

static int pwm_fd;
static int int_all_count = 0;
static struct fh_pwm_chip_data pwm;

void pwm_finish_once(struct fh_pwm_chip_data *pwm)
{
    int_all_count++;
}

int pwm_func(int pwm_fd)
{

    pwm.id = 0;
    pwm.config.period_ns = 10000000;
    pwm.config.duty_ns = 5000000;
    pwm.config.percent = 0;
    pwm.config.delay_ns = 0;
    pwm.config.phase_ns = 0;
    pwm.config.pulses = FH_PWM_PULSE_LIMIT;
    pwm.config.pulse_num = 20;
    pwm.config.finish_all = 0;
    pwm.config.finish_once = 1;
    pwm.finishall_callback = NULL;
    pwm.finishonce_callback = pwm_finish_once;

    ioctl(pwm_fd, DISABLE_PWM, &pwm);
    ioctl(pwm_fd, SET_PWM_CONFIG, &pwm);
    ioctl(pwm_fd, ENABLE_PWM, &pwm);

    sleep(1);
    ioctl(pwm_fd, GET_PWM_CONFIG, &pwm);

    return 0;
}

void *pwm_demo_main(void *para)
{
    prctl(PR_SET_NAME, "pwm demo");
    printf("[pwm_demo] pwm demo start:\n");

    pwm_fd = open("/dev/pwm", O_RDWR);
    if (pwm_fd == -1)
    {
        printf("[pwm_demo] open pwm failed\n");
        return NULL;
    }
    pwm_func(pwm_fd);

    return NULL;
}

int pwm_demo_init(void)
{
    int ret;
    pthread_t pwm_thread;
    pthread_attr_t attr;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_attr_setstacksize(&attr, 10 * 1024);

    ret = pthread_create(&pwm_thread, &attr, pwm_demo_main, NULL);
    if(ret)
    {
        printf("[pwm_demo] Error: Create pwm_demo_main thread failed!\n");
        return -1;
    }

    printf("[pwm_demo] start int_all_count:%d\n", int_all_count);
    sleep(5);
    printf("[pwm_demo] end int_all_count:%d\n", int_all_count);


    return 0;
}
