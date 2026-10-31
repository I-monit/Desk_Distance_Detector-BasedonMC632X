#ifndef __USB_WIFI_H_
#define __USB_WIFI_H_

typedef enum wifi_device_type
{
    USBWIFI_NO_DEVICE,
    USBWIFI_MTK_7601,
    USBWIFI_RTL_8188F,
}wifi_device_type_t;

typedef enum wifi_sta_link_status
{
    USBWIFI_NOT_PLUG_IN,
    USBWIFI_DISCONNECTED,
    USBWIFI_CONNECTING,
    USBWIFI_CONNECTED,
    USBWIFI_CONNECT_FAILED,
}wifi_sta_link_stats_t;

typedef enum wifi_security_type
{
    WIFI_SECURE_NONE = 1,
    WIFI_SECURE_WEP,
    WIFI_SECURE_WPA1,
    WIFI_SECURE_WPA2,
    WIFI_SECURE_ERROR,
}wifi_security_type_t;

typedef struct
{
    int is_dhcp;
    unsigned char hwaddr[6];
    unsigned char ipaddr[4];
    unsigned char gateway[4];
    unsigned char netmask[4];
    unsigned char primaryDNS[4];
    unsigned char alternateDNS[4];
}wifi_ipcfg_t;

typedef struct wifi_ap_info
{
    unsigned char NetworkType;
    unsigned char IsEncryption;
    unsigned char qual_level;
    unsigned char AuthMode;
    wifi_security_type_t EncrypType;
    unsigned char ap_mac[18];
    signed char essid[32+1];
}wifi_found_ap_info_t;

typedef struct config_wifi_info
{
    wifi_ipcfg_t ip_cfg;
    wifi_security_type_t encrypt_type;
    signed char essid[32+1];
    unsigned char key[64+1];
}wifi_sta_cfg_t;

typedef void* usbwifi_handle_t;

usbwifi_handle_t usbwifi_sta_init(wifi_device_type_t wifi_type);
void usbwifi_sta_connect(usbwifi_handle_t handle, wifi_sta_cfg_t *p_cfg);
void usbwifi_sta_disconnect(usbwifi_handle_t handle);
void usbwifi_sta_search(usbwifi_handle_t handle);
void usbwifi_sta_get_apcnt(usbwifi_handle_t handle, signed long *ap_cnt);
int usbwifi_sta_get_apinfo(usbwifi_handle_t handle, unsigned long index, wifi_found_ap_info_t *p_info);
int usbwifi_sta_status(usbwifi_handle_t handle);

#endif
