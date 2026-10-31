#include <rtthread.h>
#include <rtdevice.h>
#include <string.h>

#ifdef MOBILE_TELE_VIA_PPPOS

#include "netif/ppp/pppapi.h"
#include "netif/ppp/pppos.h"
#include "netif/ppp/pppoe.h"
#include "netif/ppp/pppol2tp.h"
#include "lwip/pbuf.h"
#include "lwip/dns.h"
#include "fh_usb_serial_api.h"
#include "fh_tele_ppp.h"

/**
 *  NOTE:
 * ppp_fatal()/ppp_error()/ppp_warn()/ppp_notice()/ppp_info()/ppp_dbglog()
 * 因均调用了ppp_log_write()均有打印输出
 */

static ppp_pcb *tele_pppos;
/* The PPP IP interface */
static struct netif tele_ppp_netif;

static rt_sem_t ppp_conn_fin_sem;

static int tele_p_status_code = -1;

#define tele_p_d(fmt, ...) rt_kprintf("\n[tele_ppp]%s-%d " fmt, __func__, __LINE__, ##__VA_ARGS__)

/*#define tele_p_d(fmt, ...)*/

/* ### internal func */
/*
 * PPP Link status callback
 * ===================
 *
 * PPP status callback is called on PPP status change (up, down, …) from lwIP
 * core thread
 */

/* PPP status callback example */
static void tele_p_status_cb(ppp_pcb *pcb, int err_code, void *ctx)
{
    struct netif *pppif = ppp_netif(pcb);

    LWIP_UNUSED_ARG(ctx);
    tele_p_d("err_code %d(phase=%d)\n", err_code, pcb->phase);

    switch (err_code)
    {
    case PPPERR_NONE:
    {
        rt_kprintf("%s: Connected and IP configed\n", __func__);
#if PPP_IPV4_SUPPORT
        rt_kprintf("   IP=%s\n", ipaddr_ntoa(&pppif->ip_addr));
        rt_kprintf("   GW=%s\n", ipaddr_ntoa(&pppif->gw));
        rt_kprintf("   netmask=%s\n", ipaddr_ntoa(&pppif->netmask));
#if LWIP_DNS
        const ip_addr_t *ns;

        ns = dns_getserver(0);
        rt_kprintf("   dns1=%s\n", ipaddr_ntoa(ns));
        ns = dns_getserver(1);
        rt_kprintf("   dns2=%s\n", ipaddr_ntoa(ns));
#endif /* LWIP_DNS */
#endif /* PPP_IPV4_SUPPORT */

#if PPP_IPV6_SUPPORT
        /*rt_kprintf("   our6_ipaddr=%s\n", ip6addr_ntoa(netif_ip6_addr(pppif, 0))); */
#endif /* PPP_IPV6_SUPPORT */
        break;
    }

    case PPPERR_PARAM:
        rt_kprintf("%s-%d: Invalid parameter\n", __func__, __LINE__);
        break;

    case PPPERR_OPEN:
        rt_kprintf("%s-%d: Unable to open PPP session\n", __func__, __LINE__);
        break;

    case PPPERR_DEVICE:
        rt_kprintf("%s-%d: Invalid I/O device for PPP\n", __func__, __LINE__);
        break;

    case PPPERR_ALLOC:
        rt_kprintf("%s-%d: Unable to allocate resources\n", __func__, __LINE__);
        break;

    case PPPERR_USER:
        if (ppp_conn_fin_sem)
            rt_sem_release(ppp_conn_fin_sem);
        rt_kprintf("%s-%d: User interrupt\n", __func__, __LINE__);
        break;

    case PPPERR_CONNECT:
        /* ppp_close() can set "pcb->err_code = PPPERR_CONNECT" */
        if (ppp_conn_fin_sem)
            rt_sem_release(ppp_conn_fin_sem);
        rt_kprintf("%s-%d: Connection lost\n", __func__, __LINE__);
        break;

    case PPPERR_AUTHFAIL:
        rt_kprintf("%s-%d: Failed authentication challenge\n", __func__, __LINE__);
        break;

    case PPPERR_PROTOCOL:
        rt_kprintf("%s-%d: Failed to meet protocol\n", __func__, __LINE__);
        break;

    case PPPERR_PEERDEAD:
        rt_kprintf("%s-%d: Connection timeout\n", __func__, __LINE__);
        break;

    case PPPERR_IDLETIMEOUT:
        rt_kprintf("%s-%d: Idle Timeout\n", __func__, __LINE__);
        break;

    case PPPERR_CONNECTTIME:
        rt_kprintf("%s-%d: Max connect time reached\n", __func__, __LINE__);
        break;

    case PPPERR_LOOPBACK:
        rt_kprintf("%s-%d: Loopback detected\n", __func__, __LINE__);
        break;

    default:
        rt_kprintf("%s-%d: Unknown error code %d\n", __func__, __LINE__, err_code);
        break;
    }
    tele_p_status_code = err_code;
}

int fh_tele_ppp_status(void)
{
    return tele_p_status_code;
}

/*
 * PPPoS serial output callback
 *
 * ppp_pcb, PPP control block
 * data, buffer to write to serial port
 * len, length of the data buffer
 * ctx, optional user-provided callback context pointer
 *
 * Return value: len if write succeed
 */
static u32_t tele_p_output_cb(ppp_pcb *pcb, u8_t *data, u32_t len, void *ctx)
{
    int ret;

    ret = fh_usb_serial_write(FH_USB_SERIAL_PORT_PT, (void *)data, len);
    if (ret != len)
    {
        rt_kprintf("%s: error, ret=%d, len=%d\n", __func__, ret, len);
    }

    return ret;
}

static void fh_mobile_ppp_input(FH_USB_SERIAL_PORT port, unsigned char *data, int len)
{
    pppos_input_tcpip(tele_pppos, data, len);
}

int fh_tele_ppp_stop(void)
{
    int ret;
    u8_t nocarrier = 0;

    /*
     * Closing PPP connection
     * ======================
     */
    /*
     * Initiate the end of the PPP session, without carrier lost signal
     * (nocarrier=0), meaning a clean shutdown of PPP protocols.
     * You can call this function at anytime.
     */
    ret = pppapi_close(tele_pppos, nocarrier);
    if (ret)
    {
        rt_kprintf("%s: ret=%d\n", __func__, ret);
        goto Exit;
    }

    /*
     * Then you must wait your status_cb() to be called, it may takes from a few
     * seconds to several tens of seconds depending on the current PPP state.
     */
    /* wait ppp dead state */
    if (ppp_conn_fin_sem)
    {
        rt_sem_take(ppp_conn_fin_sem, RT_WAITING_FOREVER);
        rt_sem_control(ppp_conn_fin_sem, RT_IPC_CMD_RESET, (void *)(0));
    }

#if 0
    /*
     * Freeing a PPP connection
     * ========================
     */
    /*
     * Free the PPP control block, can only be called if PPP session is in the
     * dead state (i.e. disconnected). You need to call ppp_close() before.
     */
    ret = pppapi_free(tele_pppos);
    if (ret)
    {
        rt_kprintf("%s-%d: pppapi_free() ret %d\n", __func__, __LINE__, ret);
        goto Exit;
    }
    tele_pppos = NULL;
#endif

Exit:
    tele_p_status_code = -1;
    fh_usb_serial_register_rx_callback_force(FH_USB_SERIAL_PORT_PT, NULL);
    return ret;
}

/* 可多次执行 */
int fh_tele_ppp_start(void)
{
    int ret = -1;
    u16_t holdoff = 0;

    tele_p_d("Tele: ppp negotiate...\n");

    fh_usb_serial_register_rx_callback_force(FH_USB_SERIAL_PORT_PT, fh_mobile_ppp_input);

    if (!ppp_conn_fin_sem)
    {
        ppp_conn_fin_sem = rt_sem_create("ppp_conn_fin", 0, RT_IPC_FLAG_FIFO);
        if (!ppp_conn_fin_sem)
        {
            rt_kprintf("%s-%d: rt_sem_create(ppp_conn_fin_sem) fail\n", __func__, __LINE__);
            goto Exit;
        }
    }

    if (!tele_pppos)
    {
        tele_pppos = pppapi_pppos_create(&tele_ppp_netif, tele_p_output_cb, tele_p_status_cb, NULL);
        if (!tele_pppos)
        {
            rt_kprintf("%s-%d: pppapi_pppos_create() fail\n", __func__, __LINE__);
            ret = -2;
            goto Exit;
        }
    }

    /* TODO: 待封装 */
    tele_ppp_netif.flags |= NETIF_FLAG_LINK_UP | NETIF_FLAG_IGMP;
    tele_ppp_netif.mtu = 1500;

    /* Set PPP netif as the default network interface */
    ret = pppapi_set_default(tele_pppos);
    if (ret)
    {
        rt_kprintf("%s-%d: pppapi_set_default() ret %d\n", __func__, __LINE__, ret);
        goto Exit;
    }

#if 1
/**
 * TODO: AUTH will fail(enter PPP_PHASE_AUTHENTICATE fail)
 * when ppp_set_auth(tele_pppos, PPPAUTHTYPE_ANY, user, passwd)
 */
    /*ppp_set_auth(tele_pppos, PPPAUTHTYPE_PAP, "user", "passwd");*/
    tele_p_d("TODO ppp_set_auth(PPPAUTHTYPE_xxx)\n");
#endif

    /*
     * Initiate PPP negotiation, without waiting (holdoff=0), can only be called
     * if PPP session is in the dead state (i.e. disconnected).
     */
    ret = pppapi_connect(tele_pppos, holdoff);
    if (ret)
    {
        rt_kprintf("%s-%d: pppapi_connect() ret %d\n", __func__, __LINE__, ret);
        goto Exit;
    }

    return 0; /*success*/

Exit:
    fh_usb_serial_register_rx_callback_force(FH_USB_SERIAL_PORT_PT, NULL);
    return ret;
}

#endif
