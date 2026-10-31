/*
 * Note lowpower state here means that fullhan host platform stay power-down while wifi is power-save state
 */
#include <rtdef.h>
#include <rtthread.h>
#include <stdbool.h>
#include <unistd.h>
#include "generalWifi.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#ifdef WIFI_USING_CY43438_LOWPOWER
/* wakeup reason of platform, which is given from MCU */
#define WAKEUP_BY_BUTTON 102
#define WAKEUP_BY_WIFI 104
#define WAKEUP_BY_CW2015 106
#define WAKEUP_BY_PIR 108

int wake_up_finish = 0;
static void do_wakeup(void);

extern void set_if(char *netif_name, char *ip_addr, char *gw_addr, char *nm_addr);
extern void gpio_output(int gpio_num, int value);
extern void gpio_blink(int gpio_num,  int value);

/* w_cy_43438_lowpower_entry() is the entry of wifi 43438 lowpower */
void w_cy_43438_lowpower_entry(void)
{
    #if 0
    gpio_output(56, 1);

    /* init uart communication between MCU and platform */
    if (init_uart() != 0)
    {
        rt_kprintf("fatal error: init uart failed\n");
        return;
    }
    #endif
    /* get wakeup reason from MCU and then do different actions */
    do_wakeup();

    return;
}

/* notify_sleep is responsible to notify MCU to cut down the power of host platform */
int notify_sleep(void)
{
    return 0;
}

/* Note to modify the following MACROs according to your AP,tcp port and ip of server */
#define SERVER_TCP_PORT 12345
#define SERVER_IP "192.168.50.94"
#define SERVER_GW "192.168.50.1"
#define LOCAL_IP  "192.168.50.38"
#define SSID "ASUS-AC750"
#define PASSWORD "12345678"

#define KEEP_ALIVE_TIME 60 /* s */
#define KEEP_ALIVE_RETRY_COUNT 2 /* tcp retry count */
static tcpinfo_kp_t tcpinfo_kp;
static pkt_pattern_t pkt_wakeup;
const uint8_t mask_wakeup[] = { 0xFF };
const uint8_t pattern_wakeup[] = {0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe};

static WIFI_PMK_LINK_CB pmk_link;
/* implementations of pmk_param_set and pmk_param_get can refer to w_cy_fastconnect_demo.c */
static int pmk_param_set(WIFI_PMK_LINK_PARAM *param)
{
    rt_kprintf("%s param->pmk:%s\n", __func__, param->pmk);
    return 0;
}
static int pmk_param_get(WIFI_PMK_LINK_PARAM *param)
{
    unsigned char pmk[68] = {0};

    rt_kprintf("%s pmk:%s\n", __func__, pmk);
    return 0;
}

/*
 * keep_alive() gives a demo that wifi 43438 enter powersave and tcp keepalive with remote server(server_ip,server_port)
 */
int keep_alive(char *server_ip, int server_port)
{
    int ret = 0;

    rt_kprintf("%s, server_ip:%s, dst_port:%d\r\n", __func__, server_ip, server_port);
    if (inet_aton(server_ip, &tcpinfo_kp.serveraddr_kp.sin_addr) == 0)
    {
        rt_kprintf("server ip address error!\n");
        return -1;
    }
    tcpinfo_kp.serveraddr_kp.sin_family = AF_INET;
    tcpinfo_kp.serveraddr_kp.sin_port = htons(server_port);
    rt_kprintf("%s, port:%d\r\n", __func__, tcpinfo_kp.serveraddr_kp.sin_port);

    tcpinfo_kp.sock = socket(AF_INET, SOCK_STREAM, 0);
    if (tcpinfo_kp.sock < 0)
    {
        rt_kprintf("%s-%d: create socket failed!\n", __func__, __LINE__);
        return -2;
    }
    if (connect(tcpinfo_kp.sock, (struct sockaddr *)&tcpinfo_kp.serveraddr_kp, sizeof(struct sockaddr_in)) < 0)
    {
        rt_kprintf("%s-%d: connect(server:%s)\n", __func__, __LINE__, server_ip);
        ret = -3;
        goto err_exit;
    }
    rt_kprintf("connect to server success!\n");

    tcpinfo_kp.period = KEEP_ALIVE_TIME;
    tcpinfo_kp.retry_count = KEEP_ALIVE_RETRY_COUNT;
    tcpinfo_kp.data_len = 4;
    tcpinfo_kp.data[0] = 0x1;
    tcpinfo_kp.data[1] = 0x2;
    tcpinfo_kp.data[2] = 0x3;
    tcpinfo_kp.data[3] = 0x4;

   /*
    * 设置唤醒包内容
    * match_offset 可设置从报文的任一字节处开始匹配: 54 means matching from app payload, 0 means matching from MAC header
    * mask 每个 BIT 的 1 对应使能 pattern 中对应的某个 byte
    * mask_len 和 pattern_len 分别是 mask 和 pattern 数组的长度，pattern_len >= (mask_len * 8)
    */
    pkt_wakeup.match_offset = 54;
    pkt_wakeup.mask = mask_wakeup;
    pkt_wakeup.mask_size = sizeof(mask_wakeup);
    pkt_wakeup.pattern = pattern_wakeup;
    pkt_wakeup.pattern_size = sizeof(pattern_wakeup);
    /* 设置在休眠时听 Beacon 的时间， 设置大于等于100ms，一般设置为 1000ms */
    wl_set_listen_beacon_time(1000);

    ret = wl_keep_alive_main(&tcpinfo_kp, &pkt_wakeup);
    if (ret)
    {
        rt_kprintf("%s-%d: wl_keep_alive_main() ret %d!", __func__, __LINE__, ret);
        goto err_exit;
    }
    return ret;

err_exit:
    close(tcpinfo_kp.sock);
    return ret;
}

/* if wifi is in power-save, call sta_connect_ap() using resume=1 to connect server again; otherwise using resume=0
 * Note you can add actions before keep_alive(), such as video transmission!
 */
int sta_connect_ap(int conn, int resume, int dhcp)
{
    int ret = -1;
    int id = 1;

    if (conn == 0)
    {
        rt_kprintf("%s-%d conn == 0\n", __func__, __LINE__);
    }
    else
    {
        (void)w_sdio_config(0, 4, 25000000, -1);
        ret = w_start(0, resume);
        rt_kprintf("%s-%d: w_start(0, %d) ret %d\n", __func__, __LINE__, resume, ret);
        w_sta_cfg_ap(id, 1);
        w_sta_cfg_ssid(id, SSID);
        w_sta_cfg_psk(id, PASSWORD);
        w_sta_sel_ap(id);
        w_sta_status();
    }
    if (dhcp == 0)
        set_if("w0", LOCAL_IP, SERVER_GW, "255.255.255.0");
    else
    {
#ifdef RT_LWIP_DHCP
extern int dhcpc_start(char *ifname);
        dhcpc_start("w0");
#endif
        rt_thread_delay(600);
    }

    do
    {
        rt_thread_delay(RT_TICK_PER_SECOND * 5);
        ret = w_sta_status();
    } while (ret);
    rt_kprintf("\n%s-%d: CONNECT SUCCESS, to keep_alive(%s, %d)\n", __func__, __LINE__, SERVER_IP, SERVER_TCP_PORT);

    do
    {
        ret = keep_alive(SERVER_IP, SERVER_TCP_PORT);
        rt_kprintf("keep_alive() ret %d\n", ret);
    } while (ret);

    /* send cmd let mcu cut down host(fullhan platform) power */
    rt_thread_delay(1);
    rt_kprintf("%s-%d to notify_sleep\n", __func__, __LINE__);
    notify_sleep();

    return ret;
}

/*
 * notify_wakeup is in charge of getting wakeup reason of host platform from MCU
 * !!! Note it should modified to get real reason from MCU!!!
 */
int notify_wakeup(void)
{
    static unsigned int count_g = 1;

    if (count_g++ == 1)
    {
        return WAKEUP_BY_BUTTON;
    } else
    {
        return WAKEUP_BY_WIFI;
    }
}
/* get wakeup reason from MCU and then do different actions */
void do_wakeup(void)
{
    int response;
    int ret = 0;
    int id = 1;

    response = notify_wakeup();
    /* test time */
    #if 0
    gpio_blink(56, 0);
    #endif

    rt_kprintf("%s response:%d\n", __func__, response);

    switch (response)
    {
    case WAKEUP_BY_WIFI:
        {
            wake_up_finish = 0;
            sta_connect_ap(1, 1, 1);
            wake_up_finish = 1;
        }
        break;
    case WAKEUP_BY_BUTTON:
        {

#if 1 /* enable pmk fast connect */
            pmk_link.param_set = pmk_param_set;
            pmk_link.param_get = pmk_param_get;
            w_register_pmk_link_cb(pmk_link);
#endif
            wake_up_finish = 0;
            sta_connect_ap(1, 0, 1);
            wake_up_finish = 1;

        }
        break;
    default:
        wake_up_finish = 0;
        sta_connect_ap(1, 1, 1);
        wake_up_finish = 1;
        break;
    }
}

#endif /* WIFI_USING_CY43438_LOWPOWER */
