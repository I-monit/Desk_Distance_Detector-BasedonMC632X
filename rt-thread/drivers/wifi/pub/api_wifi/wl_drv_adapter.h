#ifndef __wl_drv_adapter_h__
#define __wl_drv_adapter_h__

#include <rtthread.h>
#include "api_wifi/generalWifi.h"

/*
 * 功能：初始化wifi驱动程序
 * 返回值：0,成功，-1,失败
 */
extern int _wl_init_module(unsigned int mode, int reserve);


/*
 * 功能：发起连接AP的动作,是否连接成功需要后续调用
 *       函数_wl_sta_get_status来查询
 * 返回值：0,成功发起连接的动作； -1，失败
 */
extern int _wl_sta_connect(char* ssid, char* passwd);


/*
 * 功能：当wifi作为station去连接AP之后，可以
 *       调用此函数获取连接状态
 */
extern int _wl_sta_get_status(void);

/*
 * 功能：断开与AP的连接
 * 返回值：0,成功； -1，失败
 */
extern int _wl_sta_disconnect(void);


/*
 * 功能：启动AP热点，当passwd为空时，其工作于open非加密模式，
 *       当passwd非空时，其固定工作于WPA2-PSK模式
 *
 * 返回值：0,成功； -1，失败
 */
extern int _wl_ap_on(char *ssid, char *passwd, unsigned int channel);

/*
 * 功能：关闭AP热点
 * 返回值：0,成功； -1，失败
 */
extern int _wl_ap_off(void);

/*
 * 功能：station模式下，获取所连接AP的信号强度,
 *       其范围一般情况下是[-100, 0],可以在和
 *       AP保持连接的过程中，通过旋转天线的松
 *       紧来直观了解rssi的取值
 */
extern int _wl_sta_get_rssi(void);

/*
 * 功能：扫描周边热点，并返回AP列表
 * 
 * 参数描述：
 *   pplist[OUT]，用来存放扫描到的AP信息数组，其所需要的内存必须在此函数内部
 *                通过rt_malloc来分配，调用者自行负责调用rt_free来释放
 *   max_ap_num[IN]:    期望返回的扫描到的AP的最大数目. 0表示没有限制.
 *   ssid[IN]:          带ssid的扫描,一般情况下填NULL即可.
 *
 *   scan_channel_down[IN]: 指定扫描信道的起始信道.
 *   scan_channel_up[IN]:   指定扫描信道的结束信道.
 *          比如: scan_channel_down=4, scan_channel_up=6, 表示在[4,5,6]三个信道扫描
 *          比如: scan_channel_down=0, scan_channel_up=0, 表示全信道扫描
 *
 * 返回值：-1，失败
 *          0，成功，但是没有扫描到任何AP
 *         >0, 成功，返回值表示扫描到的热点数目
 */
extern int _wl_sta_get_scan_list(FH_WiFi_AccessPoint **pplist, char* ssid, int max_ap_num, int scan_channel_down, int scan_channel_up);

/*
 * 功能: AP模式下,获取其所关联的station数量
 * 返回值：>=0
 */
extern int _wl_get_assoc_num(void);

#endif /*__wl_drv_adapter_h__*/
