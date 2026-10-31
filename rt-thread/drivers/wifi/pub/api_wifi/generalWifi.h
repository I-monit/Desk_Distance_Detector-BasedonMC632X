#ifndef __FHIPC_WIFIMANAGER_H__
#define __FHIPC_WIFIMANAGER_H__
#include <rtthread.h>
#include "stdint.h"
#include "types/type_def.h"

#define IFNAME_STA "w0"
#define IFNAME_AP "ap"

#ifndef FH_AP_DEFAULT_SSID
#define FH_AP_DEFAULT_SSID "FHxxxx"
#endif
#ifndef FH_AP_DEFAULT_PSK
#define FH_AP_DEFAULT_PSK "12345678"
#endif
#ifndef FH_AP_DEFAULT_CHANNEL
#define FH_AP_DEFAULT_CHANNEL 11
#endif

#ifndef FH_AP_ADDR_DEFAULT_VALUE
#define FH_AP_ADDR_DEFAULT_VALUE   0xAC100A01UL    /* IP Addr - 172.16.10.1 */
#endif
#ifndef FH_AP_GW_DEFAULT_VALUE
#define FH_AP_GW_DEFAULT_VALUE     0xAC100A01UL    /* Gateway - 172.16.10.1 */
#endif
#ifndef FH_AP_MASK_DEFAULT_VALUE
#define FH_AP_MASK_DEFAULT_VALUE   0xffff0000UL    /* Netmask - 255.255.0.0 */
#endif

#ifndef FHEN_WIFI_GENERAL_MODE_DEFAULT
#define FHEN_WIFI_GENERAL_MODE_DEFAULT FHEN_WIFI_GENERAL_MODE_STATION
#endif

#ifndef FH_STA_DEFAULT_SSID
#define FH_STA_DEFAULT_SSID "Fullhan-Vistor"
#endif
#ifndef FH_STA_DEFAULT_PSK
#define FH_STA_DEFAULT_PSK "fullhan558"
#endif

#ifndef FH_STA_ADDR_DEFAULT_VALUE
#define FH_STA_ADDR_DEFAULT_VALUE 0xAC10673EUL
#endif
#ifndef FH_STA_GW_DEFAULT_VALUE
#define FH_STA_GW_DEFAULT_VALUE 0xAC100001UL
#endif
#ifndef FH_STA_MASK_DEFAULT_VALUE
#define FH_STA_MASK_DEFAULT_VALUE 0xFFFF0000UL
#endif
#ifndef FH_STA_DNS_DEFAULT_VALUE
#define FH_STA_DNS_DEFAULT_VALUE 0xC0A85801UL
#endif

#define FH_WIFI_AP_SSID_PSK_GEN 1

#define APP_INCLUDE_WIFI_FIRMWARE 1

#define FLASH_INCLUDE_WIFI_FIRMWARE 0

/***sta connect state(shared by eapol and wifi driver)***/
#define STATE_IDLE                   0
#define AUTH_START                   1
#define AUTHENTICATING               2
#define AUTHENTICATING1              3
#define AUTH_SUCCESS                 4
#define AUTH_FAILED                  5
#define ASSOC_START                  6
#define ASSOCIATING                  7
#define REASSOCIATING                8
#define DISSOCIATE                   9
#define ASSOC_SUCCESS                10
#define ASSOC_FAILED                 11
#define WPA_START                    12
#define WPA_WAIT_MSG_3               13
#define WPA_WAIT_GROUP               14
#define WPA_SUCCESS                  15
#define WPA_FAILED                   16
#define STA_LINK_LOST                17
#define STA_CONNECTED                18

/* STA_CON_STATUS_E is used by APP from w_sta_status() in wifi Marvell/RTL8188ftv/RTL8192fc */
typedef enum
{
	STA_STATUS_CONNECTED = 0,      /* 已连上AP */
	STA_STATUS_AP_NOT_FOUND = -4,  /* 目标AP未扫描到 */
	STA_STATUS_INVALID_KEY = -3,   /* 密码长度与加密模式不匹配 */
	STA_STATUS_AUTH_FAILURE = -5,  /* 认证或关联失败 */
	STA_STATUS_EAPOL_KEY_FAILURE = -6,  /* EAPOL-key 4-way handshake 失败 */
	STA_STATUS_CON_LOST = -7,       /* 连接已断开 */
	STA_STATUS_STATUS_UNKNOWN = -11  /* 初始状态 */
} STA_CON_STATUS_E;
#define CFG_AP_NUM  3 /* state machine is managed using specific CFG_AP */

#define WIFI_API_DEBUG
#define WIFI_STATE_DEBUG
#ifdef WIFI_STATE_DEBUG
#define w_state_d(fmt, ...) \
 do { \
        printf("\n[WIFI][w_state chg]%s-%d " fmt, __func__, __LINE__, ##__VA_ARGS__); \
    } while(0)
#else
#define w_state_d(fmt, ...)
#endif

#ifdef WIFI_API_DEBUG
#define wifi_api_d(fmt, ...) \
 do { \
        printf("\n[WIFI]%s-%d " fmt, __func__, __LINE__, ##__VA_ARGS__); \
    } while(0)
#else
#define wifi_api_d(fmt, ...)
#endif

typedef enum
{
    FHEN_WIFI_POWER_MODE_NORMAL = 0,
    FHEN_WIFI_POWER_MODE_LOW,
    FHEN_WIFI_POWER_MODE_CLOSE,

    FHEN_WIFI_POWER_MODE_END = 0X1FFFFFFF,
}FH_Wifi_Power_Mode_e;

#define FH_WIFI_SECURITY_UNKNOWN         0  /*unknown*/
#define FH_WIFI_SECURITY_OPEN            1  /*open system, without password*/
#define FH_WIFI_SECURITY_WEP             2  /*wep*/
#define FH_WIFI_SECURITY_WPA             4  /*wpa*/
#define FH_WIFI_SECURITY_WPA2            8  /*wpa2*/

#define FH_WIFI_MODE_UNKNOWN             0  /*unknown network type*/
#define FH_WIFI_MODE_ADHOC               1  /*adhoc network*/
#define FH_WIFI_MODE_INFRA               2  /*basic infrastructure network*/

typedef struct
{
    FH_UINT8  bssid[6];     /*AP's MAC address*/
    FH_UINT8  ssid[32 + 1];
    FH_SINT32 mode;         /*FH_WIFI_MODE_XXX*/
    FH_SINT32 rssi;         /*[-100, 0]*/
    FH_UINT32 security;     /*FH_WIFI_SECURITY_XXX*/
    FH_SINT32 channel;      /*AP's current channel*/
}FH_WiFi_AccessPoint;

typedef struct
{
    FH_SINT32 count;
    FH_WiFi_AccessPoint *ap;
}FH_WiFi_AccessPoint_List;

#ifdef WIFI_USING_CYPRESS

/**
 * wakeup packet pattern
 */
typedef struct
{
    /* Offset in packet to start looking */
    uint32_t match_offset;
    /* Size of mask in bytes */
    uint32_t mask_size;
    /* Pointer to memory holding the mask */
    const uint8_t *mask;
    /* Size of pattern in bytes */
    uint32_t pattern_size;
    /* pointer to memory holding the pattern */
    const uint8_t *pattern;
} wwd_packet_pattern_t;
typedef wwd_packet_pattern_t pkt_pattern_t;

/* wifi fast connect */
typedef struct
{
         char pmk[64 + 4];
         unsigned char bssid[8];
         unsigned int channel;
         unsigned int security;
}WIFI_PMK_LINK_PARAM;

typedef struct
{
         int (*param_set)(WIFI_PMK_LINK_PARAM *param);
         int (*param_get)(WIFI_PMK_LINK_PARAM *param);
}WIFI_PMK_LINK_CB;

FH_SINT32 w_register_pmk_link_cb(WIFI_PMK_LINK_CB cb);
#endif
/*
 *限制: 目前仅marvell 8801需要调用此函数,其他wifi忽略它.
 *
 *功能: 为了节省存储firmware需要的DDR空间,firmware可以存储在外部flash上,
 *      因此增加此函数来取得firmware数据. 此函数由用户程序来实现.
 *
 *参数描述: cb,用户实现的获取firmware数据的回调函数
 *
 *注意: 此函数必须在w_start之前调用.
 *
 *返回值: 0表示成功,其他值表示错误.
 *
 *回调函数参数描述:
 *      offset[IN], 期望从哪个偏移量开始获取firmware数据.
 *      data[OUT],  用来保存firmware数据.
 *                  [注意:当data为NULL时,表示查询从offset开始,
 *                   后续firmware还有多少字节.但是最多不要超过len.
 *                   比如,我们假定firmware长度为fw_len字节,那么本次
 *                   调用返回值为 MIN((fw_len - offset), len).]
 *      len[IN],    期望获取的firmware数据字节数.
 *
 *回调函数返回值: 返回本次实际获取的firmware字节数,如果<len,那么表示获取结束.
 */
FH_SINT32 w_firmware_set_cb(FH_SINT32 (*cb) (FH_UINT32 offset, FH_UINT8 *data, FH_SINT32 len));

/*
 *限制: 目前仅marvell 8801支持此函数,其他wifi忽略它.
 *      并且,目前仅对marvell 8801 station模式下才起作用.
 *      marvell 8801 AP模式下无效.
 *
 *功能: 为了优化干扰环境下的wifi传输性能,增加此函数.
 *      当然，对这个函数的调用不是必须的。
 *
 *参数描述: cb,用户实现的获取抗干扰参数的回调函数
 *
 *注意: 此函数必须在w_start之前调用.
 *
 *返回值: 0表示成功,其他值表示错误.
 *
 *回调函数参数描述:
 *      para[OUT], 用来保存获取的抗干扰参数
 *      para_len[IN], para指向的缓冲区字节数大小
 *
 *回调函数返回值: 返回实际取得的抗干扰参数字节数大小.
 */
FH_SINT32 w_anti_interference_set_cb(FH_SINT32 (*cb) (FH_UINT8 *para, FH_SINT32 para_len));

#if defined(WIFI_RTL_POWER_CTL) || defined(WIFI_MTK_POWER_CTL)
/*
 *限制: 目前仅rtl & mtk wifi支持此函数,其他wifi忽略它.
 *
 *功能: 为了支持功率配置，设置power rate。
 *      当然，对这个函数的调用不是必须的。
 *
 *参数描述: cb,用户实现的设置power rate。
 *
 *注意: 此函数必须在usbwifi_sta_init之前调用.
 *
 *返回值: 0表示成功,其他值表示错误.
 *
 *回调函数参数描述:
 *      buf[OUT], 用来保存获取的power rate
 *
 *回调函数返回值: 返回实际取得的power rate字节数大小.
 */
FH_SINT32 w_power_rate_set_cb(int (*cb)(FH_UINT8 *buf, int buf_len));

/*
 *限制: 目前仅rtl wifi支持此函数,其他wifi忽略它.
 *
 *功能: 为了支持功率配置，设置power limit。
 *      当然，对这个函数的调用不是必须的。
 *
 *参数描述: cb,用户实现的设置power limit。
 *
 *注意: 此函数必须在usbwifi_sta_init之前调用.
 *
 *返回值: 0表示成功,其他值表示错误.
 *
 *回调函数参数描述:
 *      buf[OUT], 用来保存获取的power limit
 *
 *回调函数返回值: 返回实际取得的power limit字节数大小.
 */
FH_SINT32 w_power_limit_set_cb(int (*cb)(FH_UINT8 *buf, int len));
#endif

/*
 *限制: 目前仅rtl wifi支持此函数,其他wifi忽略它.
 *
 *功能: 实现了修改efuse中mac地址的功能，等同于linux平台的rtwpriv wlan* efuse_set mac,xxxxxxxx操作。
 *
 *参数描述: 需要写入efuse的mac地址字符串，如10:A4:BE:72:74:C0则输入为“10a4be7274c0”
 *
 *注意: 此函数必须在wifi网卡up之后调用.
 *
 *返回值: 0表示成功,其他值表示错误.
 */
int w_set_efuse_mac(char *mac_addr);

/*
 *限制: 目前仅rtl8188FTV wifi支持此函数,其他wifi忽略它.
 *
 *功能: 实现了修改channel_plan的dump功能。
 *
 *返回值: 0表示成功,其他值表示错误.
 */
int w_get_channel_plan(void);

/*
 * func: sdio hardware config
 * sdio_id: sdio index. (0:sdio0,  1:sdio1, 2:sdio2, ...)
 * bit_width: sdio bit width. (1:using 1bit mode, 4:using 4bit mode)
 * clock: sdio clock.  (25000000, 50000000, ...)
 * gpio_num_irq: when using GPIO interrupt, it means which gpio do you use
 *              to receive interrupt...
 *              (-1: to use SDIO interrupt.  >=0, the GPIO pin index...
 */
FH_SINT32 w_sdio_config(FH_SINT32 sdio_id, FH_SINT32 bit_width, FH_SINT32 clock, FH_SINT32 gpio_num_irq);

/*
 * func: power up and recognize wifi, then initialize firmware
 * para1: mode (0: staion, 1: AP)
 * para2: sleep_flag (0: for common state, 1: for low power state), which is set according to current state of wifi
 * XXX: multi-thread context
 * NOTE: w_start()/w_stop() should be called in the same thread
 */
FH_SINT32 w_start(FH_UINT32 mode, FH_UINT32 sleep_flag);

/*
 * func: show the info of connect status in station mode (Please refer to wpa_cli in linux)
 * para1: void
 * return: 0 for success, other for fail
 * XXX: multi-thread context
 */
FH_SINT32 w_sta_status(FH_VOID);
/*
 * func: show profiles of configured APs (Please refer to wpa_cli in linux)
 * para1: void
 * XXX: multi-thread context
 */
FH_SINT32 w_list_cfg_ap(FH_VOID);

/**********************************************
 函数名： w_pkt_filter_en
 函数功能：使能或关闭一个 ethernet packet filter(wifi只接收匹配中已使能filter中报文格式的报文)
 输入参数： enable:1表示使能，0表示关闭；filter_id表示ethernet packet filter 编号
 输出参数：无
 返回值定义：0：配置成功；非0：配置失败
 **********************************************/
FH_SINT32 w_pkt_filter_en(FH_UINT32 enable, FH_UINT32 filter_id);

/**********************************************
 函数名： w_sta_monit_en
 函数功能：使能或关闭monitor模式
 输入参数： enable:1表示使能，0表示关闭
 输出参数：无
 返回值定义：0：配置成功；非0：配置失败
 **********************************************/
FH_SINT32 w_sta_monit_en(FH_UINT32 enable);

/*
 * 扫描周边热点,此函数仅仅打印出扫描结果,供调试用.
 */
FH_SINT32 w_sta_scan(FH_VOID);

/*
 * 功能: 扫描周边热点,并且返回扫描到的AP列表.
 *
 *
 * 注意: 1.在调用函数w_sta_scan_ext之后,你必须调用函数w_sta_free_scaninfo来
 *         释放扫描过程中动态分配的内存.
 *       2.设置scan_channel_down与scan_channel_up指定信息扫描功能尚未实现
 *
 * 参数描述:
 * max_ap_num[IN]:        期望返回的扫描到的AP的最大数目. 0表示没有限制.
 * ssid[IN]:              带ssid的扫描,一般情况下填NULL即可.
 * scan_channel_down[IN]: 指定扫描信道的起始信道.
 * scan_channel_up[IN]:   指定扫描信道的结束信道.
 *      比如: scan_channel_down=4, scan_channel_up=6, 表示在[4,5,6]三个信道扫描
 *      比如: scan_channel_down=0, scan_channel_up=0, 表示全信道扫描
 *
 * 返回值: NULL表示扫描失败, 否则表示扫描到的AP列表.
 */
FH_WiFi_AccessPoint_List* w_sta_scan_ext(FH_UINT32 max_ap_num, FH_CHAR *ssid, FH_UINT32 scan_channel_down, FH_UINT32 scan_channel_up);

/*
 * 功能:释放函数w_sta_scan_ext动态分配的内存.
 * 注意:再次调用w_sta_scan_ext之前,你必须调用w_sta_free_scaninfo
 *      来释放此前扫描过程中动态分配的内存
 */
FH_VOID w_sta_free_scaninfo(FH_WiFi_AccessPoint_List *aplist);

/*
 * func: add or delete a NULL profile of AP with specific id (Please refer to wpa_cli in linux)
 * para1: id (id for different APs)
 * para2: add (0: delete, 1: add)
 */
FH_SINT32 w_sta_cfg_ap(FH_UINT32 id, FH_UINT32 add);

/*
 * func: configure the ssid for a profile of AP with specific id (Please refer to wpa_cli in linux)
 * para1: id (id for different APs)
 * para2: ssid
 */
FH_SINT32 w_sta_cfg_ssid(FH_UINT32 id, FH_CHAR *ssid);

/*
 * func: configure the passwd for a profile of AP with specific id (Please refer to wpa_cli in linux)
 * para1: id (id for different APs)
 * para2: passwd(NULL: open, WEP?)
 */
FH_SINT32 w_sta_cfg_psk(FH_UINT32 id, FH_CHAR *passwd);

/*
 * func: connect AP with a profile id of AP(Please refer to wpa_cli in linux)
 * para1: id (configured id for different APs)
 */
FH_SINT32 w_sta_sel_ap(FH_UINT32 id);

/* disconnect ap and delete cfg ap in station mode */
FH_SINT32 w_sta_rm_ap(FH_UINT32 id);

/* disconnect ap in station mode, no matter whether connected or not */
FH_SINT32 w_sta_disconn(FH_UINT32 id);

/*
 * func: stop wifi according to mode, including resource release of wifi and protocol stack
 * para1: mode (0: staion, 1: AP)
 * NOTE: w_start()/w_stop() should be called in the same thread
 */
FH_SINT32 w_stop(FH_UINT32 mode);

/*
 * func: start and work as AP
 * para1: ssid
 * para2: passwd
 * para3: chan_id
 */
FH_SINT32 w_ap_on(FH_CHAR *ssid, FH_CHAR *passwd, FH_UINT32 chan_id);

/*
 * 功能: AP模式下,获取其所关联的station数量
 * 限制: 目前仅RTL8188FTV实现了这个函数
 * 返回值：成功返回>=0; 失败<0
 */
FH_SINT32 w_get_assoc_num(FH_VOID);

/*
    wifi 功率设置(dBm)
    return:
        1.成功返回 0
        2.失败返回 < 0
*/
FH_SINT32 w_pa_set(FH_SINT32 power);

/*
    wifi 功率获取(dBm)
    return:
        1.成功返回 0
        2.失败返回 < 0
*/
FH_SINT32 w_pa_get(FH_SINT32 *power);

/*
    wifi 模式获取(bgn)
    return:
        1.成功返回 FHEN_WifiMode_e
        2.失败返回 < 0
*/
FH_SINT32 w_mode_get(FH_VOID);

/*
    wifi  MAC信息获取
    return:
        1.成功返回 0
        2.失败返回 < 0
*/
FH_SINT32 w_mac_get(FH_UINT8 mac[6]);

/*
    wifi PMK 获取
    return:
        1.成功返回 0
        2.失败返回 < 0
*/
FH_SINT32 w_pmk_get(FH_SINT8 *pmk);

/*
    wifi 信号质量获取(dBm)
    return:
        1.成功返回 (dBm)
        2.失败返回 < 0
*/
FH_SINT32 w_rssi_get(FH_SINT16 *rssi);

/*
    wifi 工作速率获取 (Mbps)
    return:
        1.成功返回 Mbps值
        2.失败返回 < 0
*/
FH_SINT32 w_datarate_get(FH_VOID);

/*
    wifi 工作速率设置 (Mbps)
    此函数可以多次调用。
        屏蔽1M的 filter_rate置1，half_flag置0
        屏蔽5.5M filter_rate置5，half_flag置1
    return:
        1.成功返回 0
        2.失败返回 < 0
*/
FH_SINT32 w_datarate_filter(FH_SINT32 filter_rate, FH_SINT32 half_flag);

/*
    wifi 省电模式,关闭模式,唤醒进入正常模式 的设置
    return:
        1.成功返回 0
        2.失败返回 < 0
*/
FH_SINT32 w_powermode_set(FH_UINT32 mode);

#ifdef WIFI_USING_CY43438_LOWPOWER
#include <netinet/in.h>
/* tcp keep-alive packet parameters */
typedef struct
{
    int sock;
    uint16_t period;
    uint16_t retry_count;
    uint16_t data_len;
    uint8_t data[128];
    struct sockaddr_in serveraddr_kp;
} tcpinfo_kp_t;

/*
 * 设置休眠时听 Beacon 的时间，值应大于等于100ms，一般设置为 1000ms
 * return:
 *       all ways return 0
 */
FH_SINT32 wl_set_listen_beacon_time(FH_SINT32 ms);
/*
 * 让 43438 休眠前配置一系列参数，调用这个函数返回成功后就可以给主控断电
 * 入参分别为预设的tcp keepalive 和 wakeup 报文格式
 *
 */
FH_SINT32 wl_keep_alive_main(tcpinfo_kp_t *tcpinfo, pkt_pattern_t *pat);

#ifdef RT_LWIP_DHCP
/*
 * 如果休眠需要时主动发送 DHCP request 包，需要配置一下连接 ap 的 ip 地址和 ap 的租赁地址时间
 * return:
 *      all ways return 0
 */
FH_SINT32 wl_set_dhcp_ap_ip(FH_UINT32 ip);


FH_SINT32 wl_set_dhcp_ap_lease_time(FH_UINT32 lease_time);
#endif /* RT_LWIP_DHCP */
#endif /* WIFI_USING_CY43438_LOWPOWER  */

#endif

