#ifndef __CDC_VCOM_H__
#define __CDC_VCOM_H__

#define FH_CDC_COOLVIEW_MODE    0
#define FH_CDC_CMD_MODE         1
#define FH_CDC_UPDATE_MODE      2
#define FH_CDC_VCOM_MODE        3

/*
 *功能:usb vcom驱动初始化
 */
void fh_cdc_init(void);

/*
 *功能:获取cdc_vcom的模式状态信息
 *return: FH_CDC_COOLVIEW_MODE/FH_CDC_CMD_MODE/FH_CDC_UPDATE_MODE/FH_CDC_VCOM_MODE
 *注意：模式转换由host发起。
 */
extern int fh_cdc_mode_get(void);

#endif
