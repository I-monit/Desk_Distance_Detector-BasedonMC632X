#include <rtdef.h>
#include <rtthread.h>
#include <rthw.h>
#include "fh_usb_serial_api.h"
#include "fh_tele_at.h"

static int at_debug_level;
static unsigned char at_response_buf[512];

static void at_response_clear(void)
{
    rt_base_t level;

    level = rt_hw_interrupt_disable();
    rt_memset(at_response_buf, 0, sizeof(at_response_buf));
    rt_hw_interrupt_enable(level);
}

static unsigned char *at_response_get(void)
{
    if (at_response_buf[0])
        return at_response_buf;

    return NULL;
}

static void at_response_callback(FH_USB_SERIAL_PORT port, unsigned char *data, int len)
{
    int prelen;
    rt_base_t level;

    if (at_debug_level & FH_TELE_AT_DEBUG_RECV)
    {
        rt_kprintf("Tele: recvResp(%s)\n", data);
    }

    if (len <= 0 || len >= sizeof(at_response_buf))
    {
        rt_kprintf("Tele: large AT response packet(%d)!\n", len);
        return;
    }

    level = rt_hw_interrupt_disable();
    prelen = rt_strlen((char *)at_response_buf);
    if (prelen + len + 1 > sizeof(at_response_buf))
    {
        rt_kprintf("Tele: too short at response buffer!\n");
    }
    else
    {
        rt_memcpy(&at_response_buf[prelen], data, len);
        at_response_buf[prelen+len] = 0; /*end flag*/
    }
    rt_hw_interrupt_enable(level);
}

static int fh_tele_at_send_command(int oob,
        unsigned char *cmd, /*the send command*/
        unsigned char *expect_resp, /*expected response to be checked*/
        unsigned char **response,
        int timeout)
{
    static volatile int sending = 0;

    rt_base_t level;
    int ret;
    int len;
    int tick_start;
    int diff;
    int port;
    unsigned char *resp;

    if (response)
        *response = NULL;

    /*calculate ticks*/
    timeout = timeout * RT_TICK_PER_SECOND / 1000;
    if (timeout <= 0)
        timeout = 1;

    /*Step1: wait to send.*/
Retry:
    level = rt_hw_interrupt_disable();
    if (sending)
    {
        rt_hw_interrupt_enable(level);
        rt_thread_delay(10);
        goto Retry;
    }
    sending = 1;
    rt_hw_interrupt_enable(level);

    /*Step2: select which serial port to send, open it*/
    port = oob ? FH_USB_SERIAL_PORT_AT : FH_USB_SERIAL_PORT_PT;
    ret = fh_usb_serial_open(port);
    if (ret)
    {
        rt_kprintf("Tele: cann't open serial port,oob=%d!\n", oob);
        ret = FH_TELE_AT_ERR_NO_DEVICE;
        goto Exit;
    }

    /*step3: prepare to receive AT response*/
    ret = fh_usb_serial_register_rx_callback_try(port, at_response_callback);
    if (ret)
    {
        ret = FH_TELE_AT_ERR_BUZY;
        goto Exit;
    }
    at_response_clear();

    /*step4: send command out*/
    len = rt_strlen((char *)cmd);
    if (fh_usb_serial_write(port, cmd, len) != len)
    {
        rt_kprintf("Tele: serial write failed!\n");
        ret = FH_TELE_AT_ERR_SEND;
        goto Exit;
    }
    tick_start = rt_tick_get();
    if (at_debug_level & FH_TELE_AT_DEBUG_SEND)
    {
        rt_kprintf("Tele: sendAT(%s)\n", cmd);
    }
    rt_thread_delay(50*RT_TICK_PER_SECOND/1000); /*fixed to delay 50ms, to let write complete*/


    /*step5: wait response until AT response received or timeout*/
    if (expect_resp || response)
    {
        while (1)
        {
            resp = at_response_get();
            if (resp)
            {
                if (!expect_resp || rt_strstr((char *)resp, (char *)expect_resp) != RT_NULL)
                {
                    if (response)
                        *response = resp;
                    break;
                }
            }

            diff = rt_tick_get() - tick_start;
            if (diff > timeout)
            {
                ret = FH_TELE_AT_ERR_TIMEOUT; /*timeout*/
                break;
            }

            rt_thread_delay(20*RT_TICK_PER_SECOND/1000); /*delay 20ms*/
        }
    }

Exit:
    sending = 0;
    return ret;
}

int fh_tele_is_pluged_in(void)
{
    return fh_usb_serial_is_pluged_in();
}

int fh_tele_at_debug(int debug_level)
{
    at_debug_level = debug_level;
    return 0;
}

int fh_tele_at_send(unsigned char *cmd, unsigned char **response, int timeout/*milliseconds*/)
{
    return fh_tele_at_send_command(0, cmd, NULL, response, timeout);
}

int fh_tele_at_send_oob(unsigned char *cmd, unsigned char **response, int timeout/*milliseconds*/)
{
    return fh_tele_at_send_command(1, cmd, NULL, response, timeout);
}

int fh_tele_at_send_multi(struct fh_tele_at_cmd *commands, int command_num)
{
    int i;
    int ret = 0;
    unsigned int timeout;
    int retry_times;
    unsigned char **pResponse;
    unsigned char *Response;

    if (!commands || command_num <= 0)
        return FH_TELE_AT_ERR_PARAM;

    for (i = 0; i < command_num; i++, commands++)
    {
        retry_times = commands->retry_times;
        timeout = (unsigned int)commands->retry_interval * 100;
        if (!commands->cmd)
            continue;

        while (retry_times-- > 0)
        {
            pResponse = commands->wait_resp ? &Response : RT_NULL;
            ret = fh_tele_at_send_command(0, (unsigned char *)commands->cmd, (unsigned char *)commands->expect_resp, pResponse, timeout);
            if (!ret)
                break;
        }

        if (ret && !(commands->control & FH_TELE_AT_IGNORE))
        {
            rt_kprintf("Tele: failed @ AT command %s\n", commands->cmd);
            break;
        }

        ret = 0;
    }

    return ret;
}
