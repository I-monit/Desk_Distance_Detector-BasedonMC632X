
#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

/* #include "FHIPC_Pub.h" */
/* #include "FH_VAPub.h" */
#include <stdbool.h>

/* the length of ssid is between 1 and 32 */
#define SSID_LEN 33
/* the length of psk is between 8 and 63 */
#define PSK_LEN 64
/* the length of bssid */

#define FH_IP_ADDR_DEFALUT_VALUE 0xAC10673EUL  /* 172.16.103.62 */
#define FH_IP_MASK_DEFALUT_VALUE 0xFFFF0000UL
#define FH_IP_GW_DEFALUT_VALUE 0xAC100001UL
#define FH_IP_DNS_DEFALUT_VALUE 0xC0A85801UL  /* 192.168.88.1 */

#ifndef FH_UAP_DEFAULT_SSDI
#define FH_UAP_DEFAULT_SSDI "fh8620cam"
#endif
#ifndef FH_UAP_DEFAULT_PSK
#define FH_UAP_DEFAULT_PSK "12345678"
#endif
#ifndef FH_UAP_DEFAULT_CHAN
#define FH_UAP_DEFAULT_CHAN 0
#endif

#ifndef FH_UAP_IP_ADDR_DEFALUT_VALUE
#define FH_UAP_IP_ADDR_DEFALUT_VALUE 0xAC100A01UL /* IP Addr - 172.16.10.1 */
#endif
#ifndef FH_UAP_IP_GW_DEFALUT_VALUE
#define FH_UAP_IP_GW_DEFALUT_VALUE 0xAC100A01UL /* Gateway - 172.16.10.1 */
#endif
#ifndef FH_UAP_IP_MASK_DEFALUT_VALUE
#define FH_UAP_IP_MASK_DEFALUT_VALUE 0xffff0000UL /* Netmask - 255.255.0.0 */
#endif

#ifndef FH_DEFAULT_WIFI_MODE
#define FH_DEFAULT_WIFI_MODE \
    FHTEN_WLAN_MODE_UAP  /* FHTEN_WLAN_MODE_INFRASTRUCTURE //FHTEN_WLAN_MODE_UAP */
#endif

#define FH_WIFI_UAP_SSID_PSK_GEN 1

#ifndef FH_WIFI_MAC_FROM_FLASH
#define FH_WIFI_MAC_FROM_FLASH 0
#endif

#ifndef FH_WIFI_EXT_CONFIG_SUPPORT
#define FH_WIFI_EXT_CONFIG_SUPPORT 0
#endif

#define FH_WIFI_NET_OTHER_CONFIG_NUM 1

#ifndef FH_UAP_POWER_LEVEL
#define FH_UAP_POWER_LEVEL 0
#endif

typedef enum {
    FHTEN_WLAN_MODE_INFRASTRUCTURE = 0,
    FHTEN_WLAN_MODE_ADHOC,
    FHTEN_WLAN_MODE_UAP,
    FHTEN_WLAN_MODE_P2P,
    FHTEN_WLAN_MODE_P2P_UAP,
} FHTEN_WifiMode_e;

typedef enum {
    /** The network does not use security. */
    FHTEN_WLAN_SECURITY_NONE = 0,
    /** The network uses WEP security with open key. */
    FHTEN_WLAN_SECURITY_WEP_OPEN,
    /** The network uses WEP security with shared key. */
    FHTEN_WLAN_SECURITY_WEP_SHARED,
    /** The network uses WPA security with PSK. */
    FHTEN_WLAN_SECURITY_WPA,
    /** The network uses WPA2 security with PSK. */
    FHTEN_WLAN_SECURITY_WPA2,
     /** The network uses WPA/WPA2 mixed security with PSK. */
    FHTEN_WLAN_SECURITY_WPA_WPA2_MIXED,
} FHTEN_WifiSecurityType_e;

typedef struct
{
    char ssid[33];
    char dummy[3];
    unsigned int channel;
    int mode;  /* FHTEN_WifiMode_e */
    int type;  /* FHTEN_WifiSecurityType_e */
    char psk[PSK_LEN];
    unsigned char psk_len;
} FH_WifiConfig_t;

/*! 网络配置 */
typedef struct
{
    unsigned char dhcp_ip_enable;   /* !< 1->dhcp to get ip,   0->static ip */
    unsigned char dhcp_dns_enable;  /* !< 1->dhcp to get dns(dhcp_ip_enable must */
                                    /* !be 1),  0->static dns */
    unsigned int addr;
    unsigned int mask;
    unsigned int gateway;
    unsigned int dns1;
    unsigned int dns2;
    unsigned char mac[6];
    /* unsigned short  service_port; */
} FH_WifiNetConfig_t;

int wifi_test(rt_uint32_t b_is_ap_mode, char* wifi_ssid, char* wifi_passwd, rt_uint32_t chan_id);

int wifi_marvell(rt_uint32_t b_is_ap_mode, char *wifi_ssid, char *wifi_passwd, rt_uint32_t chan_id);

int wifi_bcm(rt_uint32_t mode, char *wifi_ssid, char *wifi_passwd, rt_uint32_t chan_id, bool resume_after_deep_sleep);

int wifi_rtl(rt_uint32_t b_is_ap_mode, char *wifi_ssid, char *wifi_passwd, rt_uint32_t chan_id);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif
