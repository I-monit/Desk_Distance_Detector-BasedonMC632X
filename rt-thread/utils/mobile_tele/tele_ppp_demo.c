#include <rtthread.h>
#include <rtdevice.h>
#include <gpio.h>

#ifdef MOBILE_TELE_VIA_PPPOS
#include "fh_tele_at.h"
#include "fh_tele_ppp.h"

static volatile int tele_running;
static volatile int tele_quit;

static struct fh_tele_at_cmd dial_commands[] = {

    /*test AT command*/
    {"AT\r",
     "OK",           1, 1, 10, 0},

    /*close echo display*/
    {"ATE0\r",
     "OK",           1, 1, 10, FH_TELE_AT_IGNORE},

    /*set EPS netword registration status*/
    {"AT+CEREG=0\r",
     "OK",           1, 1, 10, 0},

    /*set error message format*/
    {"AT+CMEE=2\r",
     "OK",           1, 1, 10, 0},

    /*check manufacture and model identification*/
    {"AT+CGMI;+CGMM\r",
     NULL,           1, 1, 10, FH_TELE_AT_IGNORE},

    /*check SIM status*/
    {"AT+CPIN?\r",
     "READY",        1, 5, 10, 0},

    /*check phone functionality*/
    {"AT+CFUN?\r",
     "+CFUN: 1",     1, 1, 10, FH_TELE_AT_IGNORE},

    /*check network registration status*/
    {"AT+CGREG?\r",
     "OK",           1, 60,10, 0},

    /*check EPS network registration status*/
    {"AT+CEREG?\r",
     "+CEREG: 0,1",  1, 60,10, 0},

    /*check signal quality report*/
    {"AT+CSQ\r",
     "+CSQ",         1, 1, 10, FH_TELE_AT_IGNORE},

    /*check operator selection*/
    {"AT+COPS=3,0;+COPS?\r",
     "OK",           1, 1, 10, FH_TELE_AT_IGNORE},

    /*define PDP context*/
    {"AT+CGDCONT=1,\"IP\",\"3gnet\"\r",
     "OK",           1, 1, 10, 0},

    /*Mobile Originated dial*/
    {"ATD*99#\r",
     "CONNECT",      1, 1, 10, 0},
};

/*User implement this function*/
static int user_tele_network_check(void)
{
    /*return ping("8.8.8.8", 2, 32);*/
    return 0;
}

/*
 * User implement this function.
 * AT command reset is not 100% reliable,
 * It will better if it is changed to GPIO reset.
 */
static int user_tele_hardware_reset(void)
{
    unsigned int pwron_gpio;

/* hardware reset */
/* The code should be modified according to the hardware GPIO*/
    pwron_gpio = 2; /* FH8856/FH8852/FH8532 pwron_gpio = 2; FH8626V100 pwron_gpio = 6 */

    gpio_request(pwron_gpio);
    gpio_direction_output(pwron_gpio, 0);
    rt_thread_delay(1);
    gpio_release(pwron_gpio);

    rt_thread_delay(RT_TICK_PER_SECOND);

    gpio_request(pwron_gpio);
    gpio_direction_output(pwron_gpio, 1);
    rt_thread_delay(1);
    gpio_release(pwron_gpio);

    return 0;
}

static void dial_once(int do_reset)
{
    int times = 30;
    int ret = -1;

    /*reset module*/
    if (do_reset)
    {
        user_tele_hardware_reset();
    }

    /*wait module plug in*/
    while (!tele_quit && times-- > 0)
    {
        if (fh_tele_is_pluged_in())
            break;
        rt_thread_delay(RT_TICK_PER_SECOND); /*wait for one second*/
    }

    if (tele_quit || !fh_tele_is_pluged_in())
        return;

    /*initiate dial...*/
    ret = fh_tele_at_send_multi(dial_commands,
                        sizeof(dial_commands)/sizeof(struct fh_tele_at_cmd));
    if (ret)
        goto Exit;

    /*ppp dial*/
    ret = fh_tele_ppp_start();
    if (ret)
    {
        rt_kprintf("%s-%d: fh_tele_ppp_start failed %d\n", __func__, __LINE__, ret);
        goto Exit;
    }

    while (fh_tele_ppp_status() < 0)
        rt_thread_delay(1);
    /*loop for network detect*/
    while (!tele_quit)
    {
        if (!fh_tele_is_pluged_in() || user_tele_network_check() != 0)
            break;
          rt_thread_delay(RT_TICK_PER_SECOND*5);
    }

    /*stop ppp*/
    fh_tele_ppp_stop();

Exit:
    return;
}

static void _tele_th_entry(void *arg)
{
    int do_reset = 0; /*don't reset module first time*/

    while (!tele_quit)
    {
        dial_once(do_reset++);
    }
    tele_running = 0;
}

#ifdef FINSH_USING_MSH
int tele_dial(int argc, char *argv[])
{
    rt_thread_t tele_th;

    if (tele_running)
    {
        rt_kprintf("Tele: already running!\n");
        return 0;
    }

    tele_quit = 0;
    tele_th = rt_thread_create("tele_user_th", _tele_th_entry, NULL, 10 * 1024, 240, 20);
    if (tele_th != RT_NULL)
    {
        tele_running = 1;
        rt_thread_startup(tele_th);
        rt_kprintf("Tele: user thread start...\n");
    }

    return 0;
}

int tele_exit(int argc, char *argv[])
{
    if (!tele_running)
    {
        rt_kprintf("Tele: not running yet!\n");
        return 0;
    }

    tele_quit = 1;
    while (tele_running)
    {
        rt_thread_delay(20);
    }

    return 0;
}

unsigned char *tele_send_cmd(int argc, char *argv[])
{
    int ret = 0;
    unsigned int timeout;
    unsigned char **pResponse;
    unsigned char *Response;
    unsigned char cmd[100];
    int argv_len;

    argv_len = rt_strlen(argv[1]);
    if (argc == 2)
    {
        rt_memcpy(cmd, argv[1], argv_len);
        if ((cmd[argv_len - 2] == '\\') && cmd[argv_len - 1] == 'r')
            rt_memcpy(cmd + argv_len - 2, "\r", 2);
        else
            rt_memcpy(cmd + argv_len, "\r", 2);
    } else
        rt_memcpy(cmd, "AT\r", 3);

    pResponse = &Response;
    timeout = 1000;
    ret = fh_tele_at_send_oob(cmd, pResponse, timeout/*milliseconds*/);
    if (ret)
    {
        rt_kprintf("Tele: failed @ AT command %s\n", cmd);
        return NULL;
    }
    rt_kprintf("Tele: recvResp(%s)\n", Response);
    return Response;
}
#else
int tele_dial(void)
{
    rt_thread_t tele_th;

    if (tele_running)
    {
        rt_kprintf("Tele: already running!\n");
        return 0;
    }

    tele_quit = 0;
    tele_th = rt_thread_create("tele_user_th", _tele_th_entry, NULL, 10 * 1024, 240, 20);
    if (tele_th != RT_NULL)
    {
        tele_running = 1;
        rt_thread_startup(tele_th);
        rt_kprintf("Tele: user thread start...\n");
    }

    return 0;
}

int tele_exit(void)
{
    if (!tele_running)
    {
        rt_kprintf("Tele: not running yet!\n");
        return 0;
    }

    tele_quit = 1;
    while (tele_running)
    {
        rt_thread_delay(20);
    }

    return 0;
}

unsigned char *tele_send_cmd(unsigned char *cmd)
{
    int ret = 0;
    unsigned int timeout;
    unsigned char **pResponse;
    unsigned char *Response;

    pResponse = &Response;
    timeout = 1000;
    ret = fh_tele_at_send_oob(cmd, pResponse, timeout/*milliseconds*/);
    if (ret)
    {
        rt_kprintf("Tele: failed @ AT command %s\n", cmd);
        return NULL;
    }
    rt_kprintf("Tele: recvResp(%s)\n", Response);
    return Response;
}
#endif

#ifdef FINSH_USING_MSH
MSH_CMD_EXPORT(tele_dial, usb_4G begin to dial);
MSH_CMD_EXPORT(tele_exit, usb_4G exit dial);
MSH_CMD_EXPORT(tele_send_cmd, send cmd to usb serial port);
#else
#include <finsh.h>
FINSH_FUNCTION_EXPORT(tele_dial, tele_dial() begin to dial);
FINSH_FUNCTION_EXPORT(tele_exit, tele_exit() exit dial);
FINSH_FUNCTION_EXPORT(tele_send_cmd, send cmd to usb serial port);
#endif

#endif /*MOBILE_TELE_VIA_PPPOS*/
