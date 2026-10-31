#include <rtthread.h>

#include "api_wifi/generalWifi.h"

int wifi_conn_ap(char *ssid, char *wifi_passwd)
{
    int ret = -1;
    int id = 0;
#ifdef WIFI_SDIO
    int bitwidth = 4;
    #ifdef USING_1BIT_MODE
    bitwidth = 1;
    #endif
    int sdio_clock = 25000000;
    int sdio_irq_gpio = -1;
    #ifdef WIFI_USING_HI3861L
    sdio_clock = 50000000;
    sdio_irq_gpio = 40;
    #endif
#ifdef WIFI_USING_MARVEL
extern void mw8801_config(void);
    mw8801_config();
#endif
    (void)w_sdio_config(WIFI_SDIO, bitwidth, sdio_clock, sdio_irq_gpio);
#endif
    rt_kprintf("%s-%d: ssid:%s wifi_passwd:%s\n", __func__, __LINE__, ssid, wifi_passwd);
    ret = w_start(0, 0);
    if (ret)
    {
        rt_kprintf("w_start(0) ret %d\n", ret);
        return ret;
    }
    ret = w_sta_cfg_ap(id, 1);
    if (ret)
    {
        rt_kprintf("w_sta_cfg_ap(%d, 1) ret %d\n", id, ret);
        return ret;
    }
    ret = w_sta_cfg_ssid(id, ssid);
    if (ret)
    {
        rt_kprintf("w_sta_cfg_ssid(%d, %s) ret %d\n", id, ssid, ret);
        return ret;
    }
    ret = w_sta_cfg_psk(id, wifi_passwd);
    if (ret)
    {
        rt_kprintf("w_sta_cfg_psk(%d, %s) ret %d\n", id, wifi_passwd, ret);
        return ret;
    }

    ret = w_list_cfg_ap();
    if (ret)
    {
        rt_kprintf("w_list_cfg_ap() ret %d\n", ret);
        return ret;
    }

    ret = w_sta_sel_ap(id);
    if (ret)
        rt_kprintf("w_sta_sel_ap(%d) ret %d\n", id, ret);

    w_sta_status();

    return ret;
}

int wifi_work_ap(char *ssid, char *wifi_passwd, unsigned int chan_id)
{
    int ret = -1;
#ifdef WIFI_SDIO
    int bitwidth = 4;
    #ifdef USING_1BIT_MODE
    bitwidth = 1;
    #endif
#ifdef WIFI_USING_MARVEL
extern void mw8801_config(void);
    mw8801_config();
#endif
    (void)w_sdio_config(WIFI_SDIO, bitwidth, 25000000, -1);
#endif
    ret = w_start(1, 0);
    if (ret)
    {
        rt_kprintf("w_start(1) ret %d\n", ret);
        return ret;
    }

    rt_kprintf("%s-%d: w_ap_on() ssid:%s pswd:%s channel:%d\n", __func__, __LINE__, ssid, wifi_passwd, chan_id);
    ret = w_ap_on(ssid, wifi_passwd, chan_id);
    if (ret)
    {
        rt_kprintf("w_ap_on() ret %d\n", ret);
        return ret;
    }

    return ret;
}

#if defined (WIFI_USING_USBWIFI_8188F) || \
defined(WIFI_USING_USBWIFI_RTL8192F) || \
defined(WIFI_USING_USBWIFI_7601) || \
defined (WIFI_USING_MARVEL) || \
defined (WIFI_USING_HI3861L) || \
defined(WIFI_USING_RTL8192FS) || \
defined(WIFI_USING_USBWIFI_RTL8192E) || \
defined(WIFI_USING_USBWIFI_RTL8731BU) || \
defined (WIFI_USING_USBWIFI_MTK7603U)
static void fh_print_rssi(void)
{
	int ret;
	short rssi;

	ret = w_rssi_get(&rssi);
	if (ret != 0)
	{
		rt_kprintf("failed with ret=%d.\n", ret);
	}
	else
	{
		rt_kprintf("rssi=%d.\n", (int)rssi);
	}
}
#endif

void power_config(void)
{
    extern void rtl_power_config(void);
    extern void mtk_power_config(void);

    rtl_power_config();
    mtk_power_config();
}
#ifdef WIFI_RTL_POWER_CTL
extern void get_tx_power_by_rate(void);
extern void get_tx_power_limit(void);
#define pri_power_rate get_tx_power_by_rate
#define pri_power_limit get_tx_power_limit
MSH_CMD_EXPORT(pri_power_rate, get tx power by rate);
MSH_CMD_EXPORT(pri_power_limit, get tx power limit);
#endif

#ifdef RT_USING_FINSH
#include <finsh.h>
FINSH_FUNCTION_EXPORT(wifi_work_ap, wifi_work_ap(char *ssid, char *wifi_passwd, unsigned int chan_id));
FINSH_FUNCTION_EXPORT(wifi_conn_ap, wifi_conn_ap(char *ssid, char *wifi_passwd));

FINSH_FUNCTION_EXPORT(w_ap_on, w_ap_on(FH_CHAR *ssid, FH_CHAR *passwd, FH_UINT32 chan_id));
FINSH_FUNCTION_EXPORT(w_start, w_start(FH_UINT32 mode, FH_UINT32 sleep_flag));
FINSH_FUNCTION_EXPORT(w_stop, w_stop(FH_UINT32 mode));
FINSH_FUNCTION_EXPORT(w_sta_scan, w_sta_scan(FH_VOID));
FINSH_FUNCTION_EXPORT(w_sta_status, w_sta_status(FH_VOID));
FINSH_FUNCTION_EXPORT(w_list_cfg_ap, w_list_cfg_ap(FH_VOID));
FINSH_FUNCTION_EXPORT(w_sta_cfg_ap, w_sta_cfg_ap(FH_UINT32 id, FH_UINT32 add));
FINSH_FUNCTION_EXPORT(w_sta_cfg_ssid, w_sta_cfg_ssid(FH_UINT32 id, FH_CHAR *ssid));
FINSH_FUNCTION_EXPORT(w_sta_cfg_psk, w_sta_cfg_psk(FH_UINT32 id, FH_CHAR *passwd));
FINSH_FUNCTION_EXPORT(w_sta_sel_ap, w_sta_sel_ap(FH_UINT32 id));
FINSH_FUNCTION_EXPORT(w_sta_rm_ap, w_sta_rm_ap(FH_UINT32 id));
FINSH_FUNCTION_EXPORT(w_sta_disconn, w_sta_disconn(FH_UINT32 id));
#if defined (WIFI_USING_USBWIFI_8188F)
FINSH_FUNCTION_EXPORT(w_get_channel_plan, w_get_channel_plan(FH_VOID));
#endif
#ifndef WIFI_USING_HI3861L
FINSH_FUNCTION_EXPORT(w_pkt_filter_en, w_pkt_filter_en(FH_UINT32 enable, FH_UINT32 filter_id));
FINSH_FUNCTION_EXPORT(w_sta_monit_en, w_sta_monit_en(FH_UINT32 enable));
#endif
#if defined (WIFI_USING_USBWIFI_8188F) || \
defined(WIFI_USING_USBWIFI_RTL8192F) || \
defined(WIFI_USING_USBWIFI_7601) || \
defined (WIFI_USING_MARVEL) || \
defined (WIFI_USING_HI3861L) || \
defined(WIFI_USING_RTL8192FS) || \
defined(WIFI_USING_USBWIFI_RTL8192E) || \
defined(WIFI_USING_USBWIFI_RTL8731BU) || \
defined (WIFI_USING_USBWIFI_MTK7603U)
FINSH_FUNCTION_EXPORT_ALIAS(fh_print_rssi, w_rssi_get, FH_SINT32 w_rssi_get(FH_SINT16 *rssi));
#endif
#if defined(WIFI_USING_CY43438_WLTOOL) || defined(WIFI_USING_CY43455_WLTOOL)
extern int wl_tool_main(char *cmd);
FINSH_FUNCTION_EXPORT_ALIAS(wl_tool_main, wl_tool, "wl cmd");
#endif

#ifdef WIFI_USING_SDIOWIFI
FINSH_FUNCTION_EXPORT(w_sdio_config, w_sdio_config(FH_SINT32 sdio_id, FH_SINT32 bit_width, FH_SINT32 clock, FH_SINT32 gpio_num_irq));
#endif

#ifdef WIFI_USING_MARVEL
void mw8801_config(void);
FINSH_FUNCTION_EXPORT(mw8801_config, mw8801_config(void));
#endif

#endif /* RT_USING_FINSH */

#include <finsh.h>
#include <optparse.h>

typedef enum
{
    CMD_WORK_AP,
    CMD_CONN_AP,
    CMD_START,
    CMD_STOP,
    CMD_AP_ON,
    CMD_STA_SCAN,
    CMD_STA_STATUS,
    CMD_LIST_CFG_AP,
    CMD_STA_CFG_AP,
    CMD_STA_CFG_SSID,
    CMD_STA_CFG_PSK,
    CMD_STA_SEL_AP,
    CMD_STA_RM_AP,
    CMD_STA_DISCONN,
    CMD_SDIO_CONF,
    CMD_MAX
} WIFI_CMD_ID_E;

static void wifi_help(void)
{
	printf("Usage:\n"
		"wifi --help (show usage manual)\n"
		"wifi --work_ap --ssid x --chan_id x (--passwd x)\n"
		"wifi --conn_ap --ssid x (--passwd x)\n"
		"wifi --stop --mode 0/1\n"
		"wifi --start --mode 0/1 --sleep_flag 0/1\n"
		"wifi --ap_on --ssid x --chan_id x (--passwd x)\n"
		"wifi --sta_scan\n"
		"wifi --sta_status\n"
		"wifi --list_cfg_ap\n"
		"wifi --sta_cfg_ap --id x --add 0/1\n"
		"wifi --sta_cfg_ssid --id x --ssid x\n"
		"wifi --sta_cfg_psk --id x --passwd x\n"
		"wifi --sta_sel_ap --id x\n"
		"wifi --sta_rm_ap --id x\n"
		"wifi --sta_disconn --id x\n"
		"wifi --sdio_config --sdio_id x --bit_width x --clock x (--gpio_num_irq x)\n"
#if defined (WIFI_USING_USBWIFI_8188F)
        "wifi --channel_plan\n"
#endif
	);
}

int wifi(int argc, char **argv)
{
    struct optparse_long longopts[] = {
        {"help", 'h', OPTPARSE_NONE},
        {"work_ap", 'w', OPTPARSE_NONE},
        {"conn_ap", 'c', OPTPARSE_NONE},
        {"start", 's', OPTPARSE_NONE},
        {"stop", 'S', OPTPARSE_NONE},
        {"ap_on", 'a', OPTPARSE_NONE},
        {"sta_scan", '\'', OPTPARSE_NONE},
        {"sta_status", ' ', OPTPARSE_NONE},
        {"list_cfg_ap", 'l', OPTPARSE_NONE},
        {"sta_cfg_ap", '0', OPTPARSE_NONE},
        {"sta_cfg_ssid", '1', OPTPARSE_NONE},
        {"sta_cfg_psk", '2', OPTPARSE_NONE},
        {"sta_sel_ap", '3', OPTPARSE_NONE},
        {"sta_rm_ap", '4', OPTPARSE_NONE},
        {"sta_disconn", '5', OPTPARSE_NONE},
        {"sdio_config", '6', OPTPARSE_NONE},
        {"mode", 1, OPTPARSE_REQUIRED},
        {"sleep_flag", 2, OPTPARSE_REQUIRED},
        {"ssid", 3, OPTPARSE_REQUIRED},
        {"passwd", 4, OPTPARSE_REQUIRED},
        {"chan_id", 5, OPTPARSE_REQUIRED},
        {"id", 6, OPTPARSE_REQUIRED},
        {"add", 7, OPTPARSE_REQUIRED},
        {"sdio_id", 8, OPTPARSE_REQUIRED},
        {"bit_width", 9, OPTPARSE_REQUIRED},
        {"clock", 10, OPTPARSE_REQUIRED},
        {"gpio_num_irq", 11, OPTPARSE_REQUIRED},
#if defined (WIFI_USING_USBWIFI_8188F)
        {"channel_plan", 12, OPTPARSE_NONE},
#endif
        {0}
    };

    WIFI_CMD_ID_E cmd_id = CMD_MAX;
    int mode = -1, sleep_flag = 0, chan_id = -1, id = -1, add = -1;
#ifdef WIFI_USING_SDIOWIFI
    int sdio_id = 0, bit_width = 4, clock = 25000000;
    int gpio_num_irq = -1;
#endif
    char ssid[33] = {0}, passwd[65]={0};

    int option;
    struct optparse options;

    optparse_init(&options, argv);
    while ((option = optparse_long(&options, longopts, NULL)) != -1) {
        switch (option) {
        case 'h':
            wifi_help();
            return 0;
        case 'w':/* work_ap */
            cmd_id = CMD_WORK_AP;
            break;
        case 'c':/* conn_ap */
            cmd_id = CMD_CONN_AP;
            break;
        case 's':/* start */
            cmd_id = CMD_START;
            break;
        case 'S':/* stop */
            cmd_id = CMD_STOP;
            break;
        case 'a':/* ap_on */
            cmd_id = CMD_AP_ON;
            break;
        case '\'':/* sta_scan */
            cmd_id = CMD_STA_SCAN;
            break;
        case ' ':/* sta_status */
            cmd_id = CMD_STA_STATUS;
            break;
        case 'l':/* list_cfg_ap */
            cmd_id = CMD_LIST_CFG_AP;
            break;
        case '0':/* sta_cfg_ap */
            cmd_id = CMD_STA_CFG_AP;
            break;
        case '1':/* sta_cfg_ssid */
            cmd_id = CMD_STA_CFG_SSID;
            break;
        case '2':/* sta_cfg_psk */
            cmd_id = CMD_STA_CFG_PSK;
            break;
        case '3':/* sta_sel_ap */
            cmd_id = CMD_STA_SEL_AP;
            break;
        case '4':/* sta_rm_ap */
            cmd_id = CMD_STA_RM_AP;
            break;
        case '5':/* sta_disconn */
            cmd_id = CMD_STA_DISCONN;
            break;
        case '6':/* sdio_config */
            cmd_id = CMD_SDIO_CONF;
            break;

        case 1:/* mode */
            mode = atoi(options.optarg);
            break;
        case 2:/* sleep_flag */
            sleep_flag = atoi(options.optarg);
            break;
        case 3:/* ssid */
            strcpy(ssid, options.optarg);
            break;
        case 4:/* passwd */
            strcpy(passwd, options.optarg);
            break;
        case 5:/* chan_id */
            chan_id = atoi(options.optarg);
            break;
        case 6:/* id */
            id = atoi(options.optarg);
            break;
        case 7:/* add */
            add = atoi(options.optarg);
            break;
#ifdef WIFI_USING_SDIOWIFI
        case 8:/* sdio_id */
            sdio_id = atoi(options.optarg);
            break;
        case 9:/* bit_width */
            bit_width = atoi(options.optarg);
            break;
        case 10:/* clock */
            clock = atoi(options.optarg);
            break;
        case 11:/* gpio_num_irq */
            gpio_num_irq = atoi(options.optarg);
            break;
#endif
#if defined (WIFI_USING_USBWIFI_8188F)
        case 12:/* gpio_num_irq */
            w_get_channel_plan();
            break;
#endif
        case '?':
            printf("%s %s\n", argv[0], options.errmsg);
            return -1;
        default:
            printf("%s-%d option %c not support\n", __func__, __LINE__, option);
            return -2;
        }
    }

    switch (cmd_id)
    {
        case CMD_WORK_AP:
            wifi_work_ap(ssid, passwd, chan_id);
            break;
        case CMD_CONN_AP:
            wifi_conn_ap(ssid, passwd);
            break;
        case CMD_START:
            w_start(mode, sleep_flag);
            break;
        case CMD_STOP:
            w_stop(mode);
            break;
        case CMD_AP_ON:
            w_ap_on(ssid, passwd, chan_id);
            break;
        case CMD_STA_SCAN:
            w_sta_scan();
            break;
        case CMD_STA_STATUS:
            printf("status %d\n", w_sta_status());
            break;
        case CMD_LIST_CFG_AP:
            w_list_cfg_ap();
            break;
        case CMD_STA_CFG_AP:
            w_sta_cfg_ap(id, add);
            break;
        case CMD_STA_CFG_SSID:
            w_sta_cfg_ssid(id, ssid);
            break;
        case CMD_STA_CFG_PSK:
            w_sta_cfg_psk(id, passwd);
            break;
        case CMD_STA_SEL_AP:
            w_sta_sel_ap(id);
            break;
        case CMD_STA_RM_AP:
            w_sta_rm_ap(id);
            break;
        case CMD_STA_DISCONN:
            w_sta_disconn(id);
            break;
#ifdef WIFI_USING_SDIOWIFI
        case CMD_SDIO_CONF:
            w_sdio_config(sdio_id, bit_width, clock, gpio_num_irq);
            break;
#endif
        default:
            wifi_help();
            return -3;
    }

    return 0;
}
MSH_CMD_EXPORT(wifi, wifi api);
