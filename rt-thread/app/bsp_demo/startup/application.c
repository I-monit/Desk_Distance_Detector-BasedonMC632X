#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "rttshell.h"

extern void sadc_demo_init(void);
extern int sdcard_demo_init(void);
extern int aes_demo_init(void);
extern int pwm_demo_init(void);
extern int gpio_demo_init(void);
extern int uart_demo_init(void);
extern int i2c_demo_init(void);
extern int rtc_demo_init(void);

void user_main(void)
{
/*    sleep(5);
    aes_demo_init();
    i2c_demo_init();
    rtc_demo_init();
    sadc_demo_init();
    sdcard_demo_init();
    pwm_demo_init();
    gpio_demo_init();
    uart_demo_init();
    */
}

static void bsp_demo_usage(void)
{
    printf("Usage:\n");
    printf("  bsp_demo -e:   run aes demo\n");
    printf("  bsp_demo -t:   run rtc demo\n");
    printf("  bsp_demo -i:   run i2c demo\n");
    printf("  bsp_demo -a:   run sadc demo\n");
    printf("  bsp_demo -p:   run pwm demo\n");
    printf("  bsp_demo -g:   run gpio demo\n");
    printf("  bsp_demo -u:   run uart demo\n");
    printf("  bsp_demo -c:   run sdcard demo\n");
}

static void bsp_demo(int argc, char *argv[])
{
    if (argc < 2)
    {
        bsp_demo_usage();
        return;
    }
    if (strcmp(argv[1], "-e") == 0)
    {
        aes_demo_init();
    }
    else if (strcmp(argv[1], "-u") == 0)
    {
        uart_demo_init();
    }
    else if (strcmp(argv[1], "-g") == 0)
    {
        gpio_demo_init();
    }
    else if (strcmp(argv[1], "-p") == 0)
    {
        pwm_demo_init();
    }
    else if (strcmp(argv[1], "-c") == 0)
    {
        sdcard_demo_init();
    }
    else if (strcmp(argv[1], "-a") == 0)
    {
        sadc_demo_init();
    }
    else if (strcmp(argv[1], "-t") == 0)
    {
        rtc_demo_init();
    }
    else if (strcmp(argv[1], "-i") == 0)
    {
        i2c_demo_init();
    }
    else
    {
        bsp_demo_usage();
    }
}
SHELL_CMD_EXPORT(bsp_demo, bsp demo)