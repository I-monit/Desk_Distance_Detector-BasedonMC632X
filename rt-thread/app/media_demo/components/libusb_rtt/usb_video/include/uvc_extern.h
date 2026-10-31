#ifndef __FLASHBASE_h__
#define __FLASHBASE_h__

#include "usb_video.h"

extern FH_VOID rt_kprintf(const FH_CHAR *fmt, ...);

/* A.8. Video Class-Specific Request Codes */
#define UVC_RC_UNDEFINED               0x00
#define UVC_SET_CUR                    0x01
#define UVC_GET_CUR                    0x81
#define UVC_GET_MIN                    0x82
#define UVC_GET_MAX                    0x83
#define UVC_GET_RES                    0x84
#define UVC_GET_LEN                    0x85
#define UVC_GET_INFO                   0x86
#define UVC_GET_DEF                    0x87

#define XU_WRITE_REG                    0x10
#define XU_READ_REG                     0x11
#define XU_STREAM_STA                   0x80
#define XU_GPIO_CTRL                    0x81
#define XU_QP_SET                       0x82
#define XU_DFU                          0x83
#define XU_SET_TIME                     0x84
#define XU_SENSOR_I2C                   0x85
#define XU_GPIO_READ                    0x86
#define XU_I2C_RW                       0x87
#define XU_AE_SET                       0x88

#define USER_CMD_DRV_CTRL               0xaa
#define USER_DEV_CTRL_OFF               0x01
#define USER_DEV_CTRL_ON                0x02

enum
{
    IsocDataMode_stream,
    IsocDataMode_Flash,
};

typedef union
{
    struct
    {
        FH_UINT32 year   :8;
        FH_UINT32 mon    :6;
        FH_UINT32 day    :6;
        FH_UINT32 hour   :6;
        FH_UINT32 min    :6;
    } b;
    FH_UINT32 d32;
} Reg_Version;

struct uvc_extern_data
{
    FH_CHAR buf[64];
    FH_UINT32 size;
};

FH_UINT32 calcCRC(FH_UINT8 *buf, FH_UINT32 size);
FH_VOID int2Byte(FH_UINT8 *buf, FH_UINT32 val);

FH_VOID uvc_extern_intr_ctrl(FH_UINT8 *data, FH_UINT32 data_size, FH_UINT32 cs);
FH_VOID uvc_extern_intr_proc(FH_UINT8 req, FH_UINT8 cs, struct uvc_request_data *resp);
FH_VOID uvc_get_flash_data(FH_UINT8 **data, FH_UINT32 *data_size);
FH_UINT32 uvc_get_flash_mode(FH_VOID);
FH_VOID uvc_set_flash_mode(FH_SINT32 set);
/*
 *功能:加载flash中的sensor参数
 *addr: sensor参数在flash中的地址
 *sensor_param[OUT]: 用于存储sensor参数的缓存
 *param_len[OUT]: 得到的sensor参数长度
 *return: 0, 得到缓存且crc校验正确; -1, 发生错误
 */
FH_SINT32 fh_uvc_ispara_load(FH_UINT32 addr, FH_CHAR *sensor_param, FH_SINT32 *param_len);
FH_SINT32 load_flash_isppara(FH_SINT32 grpid, FH_SINT32 (*ISP_LoadIspParam)(FH_UINT32 , char *));
/*
 *功能:使能uvc flash升级功能
 */
FH_VOID fh_uvc_flash_init(FH_VOID);
#endif

