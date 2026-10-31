#include <rtthread.h>
#include <rtdevice.h>
#include <gpio.h>

#if defined(RT_USING_USB_RNDIS_TELE) || defined(RT_USING_USB_ECM_TELE)

#include "fh_tele_at.h"

extern int dhcpc_start(char *ifname);
extern int list_if(void);

static volatile int tele_running;
static volatile int tele_quit;

/* static unsigned char *dail_cmd1 = "AT+CGACT=1,1\r"; */  /* WH_G405 */
/* static unsigned char *dail_cmd2 = "AT+ZGACT=1,1\r"; */  /* WH_G405 */

 unsigned char *dail_cmd1 = (unsigned char *)"AT\r";               /* LUAT AIR720 & CJ907 & NL668 for autoconnectt mode!!! */
/* unsigned char *dail_cmd1 = "AT+QNETDEVCTL=1,1\r"; */ /* EC200 */
/* unsigned char *dail_cmd1 = "AT+ECMDUP=1,1,1,0,1\r"; */ /* FIBOCOM NL668 config autoconnect mode IPV4 */
/* unsigned char *dail_cmd1 = "AT+ECMDUP=1,1,1,0,0\r"; */ /* FIBOCOM NL668 manual connect mode IPV4 */
/* unsigned char *dail_cmd1 = "at\$mynetact=0,1\r";*/  /* Neoway N720V5 */
/* unsigned char *dail_cmd1 = "AT\r";*/

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
    int ret = 0;
    unsigned int pwron_gpio;

    /* hardware reset */
    /* The code should be modified according to the hardware GPIO*/
    pwron_gpio = 2;
    gpio_request(pwron_gpio);
    gpio_direction_output(pwron_gpio, 0);
    rt_thread_delay(1);
    gpio_release(pwron_gpio);

    rt_thread_delay(RT_TICK_PER_SECOND);

    gpio_request(pwron_gpio);
    gpio_direction_output(pwron_gpio, 1);
    rt_thread_delay(1);
    gpio_release(pwron_gpio);

    return ret;
}

static void dial_once(int do_reset)
{
    int times;
    int ret = -1;
    unsigned char *response;
    struct fh_tele_at_cmd test_command = {"AT\r", "OK", 1, 1, 10, 0};

    /*reset module*/
    if (do_reset)
    {
        user_tele_hardware_reset();
    }

    /*wait module plug in*/
    times = 30;
    while (!tele_quit && times-- > 0)
    {
        if (fh_tele_is_pluged_in())
            break;
        rt_thread_delay(RT_TICK_PER_SECOND); /*wait for one second*/
    }

    if (tele_quit || !fh_tele_is_pluged_in())
        return;

    /*test if module is ready?*/
    times = 10;
    while (times-- > 0)
    {
        if (tele_quit || !fh_tele_is_pluged_in())
            return;
        ret = fh_tele_at_send_multi(&test_command, 1);
        if (ret == 0)
            break;
        rt_thread_delay(RT_TICK_PER_SECOND);/*wait for one second*/
    }

    /*start send dial command*/
    ret = fh_tele_at_send(dail_cmd1, &response, 1000);
    if (ret)
    {
        rt_kprintf("Tele: failed to send dail command %s\n", dail_cmd1);
        return;
    }
/*
    ret = fh_tele_at_send(dail_cmd2, &response, 1000);
    if (ret)
    {
        rt_kprintf("Tele: failed to send dail command %s\n", dail_cmd2);
        return;
    }
*/
    /*start dhcp client to get internet IP address*/
    ret = dhcpc_start("u0");
    if (ret)
        return;
    list_if();
    /*loop for network detect*/
    while (!tele_quit)
    {
        if (!fh_tele_is_pluged_in() || user_tele_network_check() != 0)
            break;
        rt_thread_delay(RT_TICK_PER_SECOND);
    }

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

    if (argc == 2)
    {
        argv_len = rt_strlen(argv[1]);
        rt_memcpy(cmd, argv[1], argv_len);
        if ((cmd[argv_len - 2] == '\\') && cmd[argv_len - 1] == 'r')
            rt_memcpy(cmd + argv_len - 2, "\r", 2);
        else
            rt_memcpy(cmd + argv_len, "\r", 2);
    } else
    {
        rt_memcpy(cmd, "AT\r", 3);
        rt_kprintf("example: tele_send_cmd AT\r\n");
    }

    pResponse = &Response;
    timeout = 1000;
    ret = fh_tele_at_send(cmd, pResponse, timeout/*milliseconds*/);
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
    ret = fh_tele_at_send(cmd, pResponse, timeout/*milliseconds*/);
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
FINSH_FUNCTION_EXPORT(tele_send_cmd, tele_send_PT() send cmd to PT);
#endif

#endif /*RT_USING_USB_RNDIS_TELE*/
