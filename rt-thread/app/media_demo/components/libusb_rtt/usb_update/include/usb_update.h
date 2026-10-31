#ifndef __usb_update_h__
#define __usb_update_h__

#include "types/type_def.h"

#if defined(CONFIG_ARCH_MC632X) || defined(CONFIG_ARCH_FH8852V301_FH8662V100)
#define ISP_PARAM_ADDR 0xd0000
#define ISP_PARAM_ADDR_BAK  (ISP_PARAM_ADDR + 0x6000)
#define UPDATE_CFG_ADDR 0x80000
#define FIRMWARE_VERSION_ADDR 0xcFFF0
#define FIRMWARE_VERSION_ADDR 0xcFFF0
#define FLASH_ADDR_UVC_CONFIG_DATA  0xe0000
#define UVC_INFO_ADDR       (FLASH_ADDR_UVC_CONFIG_DATA + 0x4000)
#define UVC_INFO_BAK_ADDR   (FLASH_ADDR_UVC_CONFIG_DATA + 0x6000)
#define ISP_USER_PARAM_ADDR (FLASH_ADDR_UVC_CONFIG_DATA + 4*1024)

#define FH_ISP_PARA_SIZE    (0x00006000)
#else
#define ISP_PARAM_ADDR 0x190000
#define ISP_PARAM_ADDR_BAK  (ISP_PARAM_ADDR + 0x6000)
#define UPDATE_CFG_ADDR 0x80000
#define FIRMWARE_VERSION_ADDR 0x18FFF0
#define FIRMWARE_VERSION_ADDR 0x18FFF0
#define FLASH_ADDR_UVC_CONFIG_DATA  0x1a0000
#define UVC_INFO_ADDR       (FLASH_ADDR_UVC_CONFIG_DATA + 0x4000)
#define UVC_INFO_BAK_ADDR   (FLASH_ADDR_UVC_CONFIG_DATA + 0x6000)
#define ISP_USER_PARAM_ADDR (FLASH_ADDR_UVC_CONFIG_DATA + 4*1024)
#define FH_ISP_PARA_SIZE    (0x00006000)
#endif

enum
{
    UVC_UPDATE_WAIT,
    UVC_UPDATE_UDISK,
    UVC_UPDATE_VCOM,
    UVC_UPDATE_DFUA,
    UVC_UPDATE_DFUP,
    UVC_UPDATE_HID,
};

struct update_cmd
{
    FH_UINT8 header;
    FH_UINT8 resv;
    FH_UINT8 len;
    FH_UINT8 crc;
    FH_UINT8 data[32];
};

#define HID_CMD_HEADER 0x99
#define HID_CMD_UPDATE 0x88

FH_SINT32 Flash_Base_Init(FH_VOID);
FH_UINT32 Flash_Base_Read(FH_SINT32 offset, FH_VOID *buffer, FH_UINT32 size);
FH_UINT32 Flash_Base_Write(FH_SINT32 offset, FH_VOID *buffer, FH_UINT32 size);
FH_VOID fh_uvc_update_init(FH_VOID);
FH_VOID makeCRCTable(FH_UINT32 seed);
FH_VOID usb_update_check(FH_UINT8 *report, FH_UINT32 size);

extern FH_SINT32 update_flash;
extern FH_UINT32 crc32_table_init;

#endif /*__usb_update_h__*/