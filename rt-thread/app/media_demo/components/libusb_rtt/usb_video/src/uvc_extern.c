#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>
#include "mqueue.h"
#include "errno.h"
#include "sys/reboot.h"
#include <sys/prctl.h>
#include "libusb_rtt/usb_video/include/uvc_extern.h"

#ifdef FH_APP_USING_HEX_PARA
static pthread_t g_thread_uvc_extern_intr;
#endif
/* ===================update========================= */
#ifdef FH_APP_UVC_PARAM_HISTORY // 使用用户设定的参数，在flash中
static pthread_t g_thread_uvc_uspara;
#endif
/* ============================================ */

/* ============get img_flash and set hex================== */
struct xu_recv_data
{
    FH_UINT32 type;
    FH_UINT32 size;
    FH_UINT32 crc;
    FH_UINT32 recvCnt;
    FH_UINT32 blkSize;
    FH_UINT32 flashAddr;
    FH_UINT32 writeResult;
    FH_UINT8 *pData;
};

static FH_UINT32 gIoscDataMode;
static FH_UINT32 gFlashAddr;
static FH_UINT32 gFlashSize;
static FH_UINT8 *gUsbTempBuf = NULL;
static struct xu_recv_data gRecvData;

#ifdef FH_APP_USING_HEX_PARA
static FH_UINT8 isp_param_buff_flash[FH_ISP_PARA_SIZE];
#endif

#define CRC_SEED 0xEDB88320
#define Data_Type_UvcCfg 0xc
#define Data_Type_Drv 0xd
#define Data_Type_Flash 0xf

#define USB_TEMP_BUF_SIZE (68 * 1024)
#define BUF_FLASH_HEAD_SIZE (16)
/* ============================================ */
/* ============version and param================== */
static Reg_Version gUvcVersion;
/* ============================================ */

#define BYTE2INT(n) (*(n + 0) | *(n + 1) << 8 | *(n + 2) << 16 | *(n + 3) << 24)

FH_VOID int2Byte(FH_UINT8 *buf, FH_UINT32 val)
{
    *buf = (val & 0xff);
    *(buf + 1) = (val >> 8 & 0xff);
    *(buf + 2) = (val >> 16 & 0xff);
    *(buf + 3) = (val >> 24 & 0xff);
}

FH_VOID GetFirmwareVersion(FH_VOID)
{
    FH_UINT32 buf[2];
    Flash_Base_Read(FIRMWARE_VERSION_ADDR, buf, 8);
    if (buf[0] == (buf[1] ^ 0xffffffff))
    {
        gUvcVersion.d32 = buf[0];
        printf("uvc version info: year=%d, mon=%d, day=%d, hour=%d, min=%d\n",
               gUvcVersion.b.year,
               gUvcVersion.b.mon,
               gUvcVersion.b.day,
               gUvcVersion.b.hour,
               gUvcVersion.b.min);
    }
    else
    {
        printf("get version error, ver=0x%08x, chk=0x%08x\n", buf[0], buf[1]);
        gUvcVersion.d32 = 0;
    }
}

#ifdef FH_APP_USING_HEX_PARA
FH_SINT32 load_flash_isppara(FH_SINT32 grpid, FH_SINT32 (*ISP_LoadIspParam)(FH_UINT32 , char *))
{
    int flash_para_load = 0;
    char *sensor_param;
    int param_len = FH_ISP_PARA_SIZE;
    int ret = 0;

    sensor_param = malloc(FH_ISP_PARA_SIZE);
    if (!sensor_param)
        printf("%s--%d sensor hex buf alloc failed!\n", __func__, __LINE__);
    else
    {
        ret = fh_uvc_ispara_load(ISP_PARAM_ADDR, sensor_param, &param_len);
        if (ret)
        {
            ret = fh_uvc_ispara_load(ISP_PARAM_ADDR_BAK, sensor_param, &param_len);
            if (ret == 0)
            {
                flash_para_load = 1;
                printf("ISP use backup param!!!\n");
            }
            else
            {
                flash_para_load = 0;
                printf("ISP param Load from Flash fail!!!\n");
            }
        } else
        {
            printf("ISP use flash param!!!\n");
            flash_para_load = 1;
        }
        if (flash_para_load)
        {
            ret = ISP_LoadIspParam(grpid, sensor_param);
            if (ret)
            {
                printf("API_ISP_LoadIspParam failed with %d\n", ret);
            }
        }
        free(sensor_param);
    }
    return ret;
}
static FH_SINT32 SaveIspPara(FH_SINT32 addr)
{
    FH_SINT32 ret;
    FH_UINT32 crc = 0;
    ISP_PARAM_CONFIG isp_param_config;

    ret = API_ISP_GetBinAddr(FH_APP_GRP_ID, &isp_param_config);
    if (ret)
    {
        printf("Error: API_ISP_GetBinAddr failed with %d\n", ret);
        return 0;
    }

    if (isp_param_config.u32BinSize <= (sizeof(isp_param_buff_flash) - 16))
    {
        memcpy(isp_param_buff_flash + 16, (FH_VOID *)isp_param_config.u32BinAddr, isp_param_config.u32BinSize);
    }
    else
    {
        printf("Error: SaveIspPara copy para %d > %d\n", isp_param_config.u32BinSize, sizeof(isp_param_buff_flash));
        return 0;
    }

    crc = calcCRC((FH_VOID *)isp_param_buff_flash + 16, isp_param_config.u32BinSize);
    int2Byte(isp_param_buff_flash, crc);
    int2Byte(isp_param_buff_flash + 4, 0xffffffff - crc);
    int2Byte(isp_param_buff_flash + 8, isp_param_config.u32BinSize);
    int2Byte(isp_param_buff_flash + 12, 0xffffffff - isp_param_config.u32BinSize);

    ret = Flash_Base_Write(addr, (FH_VOID *)isp_param_buff_flash, isp_param_config.u32BinSize + 16);
    if (ret == 0)
    {
        printf("Error: SaveIspPara fail!!!\n");
    }
    else
    {
        printf("\nSaveIspPara write =%d paralen=%d, crc=%x\n", ret, isp_param_config.u32BinSize, crc);
    }

    return (ret == isp_param_config.u32BinSize + 16);
}

FH_SINT32 fh_uvc_ispara_load(FH_UINT32 addr, FH_CHAR *sensor_param, FH_SINT32 *param_len)
{
    FH_UINT32 crc = 0;
    FH_UINT32 crc_comp = 0;
    FH_UINT32 para_len = 0;
    FH_UINT32 para_len_cmp = 0;
    FH_SINT32 len = 0;

    if (!crc32_table_init)
    {
        crc32_table_init = 1;
        makeCRCTable(CRC_SEED);
    }
    Flash_Base_Init();
    len = Flash_Base_Read(addr, isp_param_buff_flash, FH_ISP_PARA_SIZE);
    if (len <= 0)
    {
        printf("isp_load_para_flash read error !!! len = %d\n", len);
        return -1;
    }

    crc = BYTE2INT(isp_param_buff_flash);
    crc_comp = BYTE2INT(isp_param_buff_flash + 4);
    para_len = BYTE2INT(isp_param_buff_flash + 8);
    para_len_cmp = BYTE2INT(isp_param_buff_flash + 12);

    if (crc == 0xffffffff && crc_comp == 0xffffffff)
        return -1;
    if ((para_len + para_len_cmp) == 0xffffffff)
    {
        if (para_len <= FH_ISP_PARA_SIZE - 16 && ((crc + crc_comp) == 0xffffffff) && crc == calcCRC(isp_param_buff_flash + 16, para_len))
        {
            memcpy(sensor_param, isp_param_buff_flash + 16, para_len);
            *param_len = para_len;
            return 0;
        }
        else
        {
            printf("isp_load_para_flash error! addr:%x read crc:%x len:%d\n",
                   addr, (FH_UINT32)crc, (FH_SINT32)para_len);
            return -1;
        }
    }
    else
    {
        return -1;
    }
}

static FH_SINT32 uvc_isp_para_write(FH_SINT32 addr, FH_UINT8 *pData, FH_UINT32 size)
{
    FH_UINT32 ret;
    FH_UINT32 saveSize;
    FH_CHAR *sensor_param;
    FH_SINT32 param_len;
    ISP_PARAM_CONFIG isp_param_config;

    ret = API_ISP_GetBinAddr(FH_APP_GRP_ID, &isp_param_config);
    if (ret)
    {
        printf("Error: API_ISP_GetBinAddr failed with %d\n", ret);
        return 0;
    }

    saveSize = isp_param_config.u32BinSize > size ? size : isp_param_config.u32BinSize;
    memcpy((FH_VOID *)isp_param_config.u32BinAddr, pData, saveSize);
    if (!SaveIspPara(addr))
    {
        return 0;
    }

    sensor_param = malloc(FH_ISP_PARA_SIZE);
    if (!sensor_param)
    {
        free(sensor_param);
        return 0;
    }

    param_len = 0;
    fh_uvc_ispara_load(addr, sensor_param, &param_len);
    if (param_len)
    {
        ret = 1;
    }
    ret = API_ISP_LoadIspParam(FH_APP_GRP_ID, sensor_param);
    if (ret)
    {
        printf("API_ISP_LoadIspParam failed with %d\n", ret);
        free(sensor_param);
        return ret;
    }
    free(sensor_param);
    printf("CheckSavePara, ret =%d\n", ret);
    return ret;
}

static FH_VOID uvc_flash_write(struct xu_recv_data *para)
{
    struct xu_recv_data *pRecvData = para;

    if (pRecvData->type == Data_Type_Flash) /* TODO */
    {
        Flash_Base_Write(pRecvData->flashAddr, pRecvData->pData, pRecvData->size);
        printf("write flash over, addr = %x, size = %d\n", pRecvData->flashAddr, pRecvData->size);
        pRecvData->writeResult = Data_Type_Flash;
    }
    else if (pRecvData->type == Data_Type_Drv)
    {
        if (uvc_isp_para_write(ISP_PARAM_ADDR, pRecvData->pData, pRecvData->size))
        {
            printf("write Drv over, addr = %x, size = %d\n", pRecvData->flashAddr, pRecvData->size);
            pRecvData->writeResult = Data_Type_Drv;
        }
        free(pRecvData->pData);
        pRecvData->pData = NULL;
        gRecvData.pData = NULL;
    }
}
#endif

FH_UINT32 uvc_get_flash_mode(FH_VOID)
{
    return gIoscDataMode;
}

FH_VOID uvc_set_flash_mode(FH_SINT32 set)
{
    gIoscDataMode = set;
}

FH_VOID uvc_get_flash_data(FH_UINT8 **data, FH_UINT32 *data_size)
{
    FH_UINT32 chkData = 0;
    FH_SINT32 idx = 0;

    memcpy(gUsbTempBuf, "FLSH", 4);
    memcpy(gUsbTempBuf + 4, &gFlashAddr, 4);
    memcpy(gUsbTempBuf + 8, &gFlashSize, 4);

    if (gFlashAddr != 0xffffffff)
    {
        Flash_Base_Read(gFlashAddr, gUsbTempBuf + BUF_FLASH_HEAD_SIZE, gFlashSize);
    }
    for (idx = 0; idx < gFlashSize; idx++)
    {
        chkData += gUsbTempBuf[BUF_FLASH_HEAD_SIZE + idx];
    }

    memcpy(gUsbTempBuf + 12, &chkData, 4);
    *data = gUsbTempBuf;
    *data_size = gFlashSize + BUF_FLASH_HEAD_SIZE;
}

static FH_CHAR Uvc_ExtData[64] = {0};
static FH_CHAR Uvc_ExtBuf[64] = {0};
static mqd_t uvc_extern_mq = NULL;
#ifdef FH_APP_USING_HEX_PARA
static FH_VOID *uvc_extern_set(FH_VOID *arg)
{
    struct uvc_extern_data ext_data;
    struct mq_attr attr;
    FH_CHAR data[64] = {0};

    prctl(PR_SET_NAME, "uvc_extern_set");
    attr.mq_msgsize = sizeof(struct uvc_extern_data);
    attr.mq_maxmsg = 10;
    uvc_extern_mq = NULL;
    uvc_extern_mq = mq_open("uvc_extern_mq", O_RDWR | O_CREAT | O_EXCL, 0x644, &attr);
    if (uvc_extern_mq == NULL)
        printf("Error: Create uvc_extern_mq failed!\n");
    while (1)
    {
        if (mq_receive(uvc_extern_mq, (FH_CHAR *)&ext_data, sizeof(ext_data), NULL) > 0)
        {
            memcpy(data, ext_data.buf, ext_data.size);
            switch (data[0])
            {
            /* ================================save uvc info=============================*/
            case 0xa0:
                uvc_info_save_data();
                break;
            /* ================================save uvc info=============================*/
            /* ====================================reset===================================== */
            case 0xaa: /* 0xaa 0x02 reset */
                if (data[1] == 0x02)
                    reboot(0x01234567);
                break;
            /* ====================================reset===================================== */
            /* ================================udisk uptate================================== */
            case 'R':
                if (memcmp(data, "RESET", 5) == 0)
                {
                    update_flash = UVC_UPDATE_UDISK;
                }
                break;
            /* ================================udisk uptate================================== */
            /* ================================VCOM uptate================================== */
            case 'C':
                if (memcmp(data, "COMUPDATE", 9) == 0)
                {
                    update_flash = UVC_UPDATE_VCOM;
                }
                break;
            /* ================================VCOM uptate================================== */
            /* ================================HID uptate================================== */
            case 'H':
                if (data[1] == 'U')
                {
                    update_flash = UVC_UPDATE_HID;
                }
                break;
            /* ================================HID uptate================================== */
            /* ================================DFU uptate================================== */
            case XU_DFU:
                if (data[1] == 0x11)
                {
                    update_flash = UVC_UPDATE_DFUA;
                }
                else
                {
                    update_flash = UVC_UPDATE_DFUP;
                }
                break;
            /* ================================DFU uptate================================== */
            /* ================================out image==================================== */
            case 'F':
                if (data[1] == 'R')
                {
                    memcpy(&gFlashAddr, data + 2, 4);
                    memcpy(&gFlashSize, data + 6, 4);
                    if (gFlashAddr < 0xffffff)
                    {
                        if (!gUsbTempBuf)
                        {
                            gUsbTempBuf = malloc(USB_TEMP_BUF_SIZE);
                            if (!gUsbTempBuf)
                                printf("flash_img buf alloc failed!\n");
                        }
                        if (gFlashSize > USB_TEMP_BUF_SIZE - BUF_FLASH_HEAD_SIZE)
                            gFlashSize = USB_TEMP_BUF_SIZE - BUF_FLASH_HEAD_SIZE;
                        gIoscDataMode = IsocDataMode_Flash;
                    }
                    else
                    {
                        gIoscDataMode = IsocDataMode_stream;
                    }
                    printf("Read flash, addr=%x, size=%x, mode = %d\n", gFlashAddr, gFlashSize, gIoscDataMode);
                }
                else if (data[1] == 'E')
                {
                    free(gUsbTempBuf);
                    gUsbTempBuf = NULL;
                    gIoscDataMode = IsocDataMode_stream;
                }
                break;
            /* ================================out image==================================== */
            /* ================================update hex==================================== */
            case 0xda:
                /* header */
                memcpy(&gRecvData.flashAddr, data + 1, 4);
                printf("update hex flashaddr = %x!\n", gRecvData.flashAddr);
                break;
            case 0xdb:
                /* data begin */
                gRecvData.type = data[1];
                memcpy(&gRecvData.size, data + 2, 4);
                memcpy(&gRecvData.crc, data + 6, 4);
                if (gRecvData.pData)
                {
                    free(gRecvData.pData);
                    gRecvData.pData = NULL;
                }
                gRecvData.pData = malloc(gRecvData.size);
                if (!gRecvData.pData)
                    printf("param data buf alloc failed!\n");
                printf("update hex size = %x, crc = %x!\n", gRecvData.size, gRecvData.crc);
                gRecvData.recvCnt = 0;
                break;
            case 0xdd:
                /* data sending */
                gRecvData.blkSize = ext_data.size - 1;
                if (gRecvData.recvCnt + gRecvData.blkSize < gRecvData.size)
                {
                    memcpy(gRecvData.pData + gRecvData.recvCnt, data + 1, gRecvData.blkSize);
                    gRecvData.recvCnt += gRecvData.blkSize;
                }
                else
                {
                    memcpy(gRecvData.pData + gRecvData.recvCnt, data + 1, gRecvData.size - gRecvData.recvCnt);
                    gRecvData.recvCnt = gRecvData.size;
                }
                break;
            case 0xde:
                /* data over */
                if (gRecvData.recvCnt == gRecvData.size)
                {
                    if (gRecvData.crc == calcCRC(gRecvData.pData, gRecvData.size))
                    {
                        printf("recv over, check crc ok\n");
                        uvc_flash_write(&gRecvData);
                    }
                    else
                    {
                        printf("update hex oricrc = %x, datacrc = %x!\n", gRecvData.crc, calcCRC(gRecvData.pData, gRecvData.size));
                        printf("recv over, check crc error!\n");
                        free(gRecvData.pData);
                        gRecvData.pData = NULL;
                    }
                }
                else
                {
                    printf("recv over, size =%d, error!\n", gRecvData.recvCnt);
                    free(gRecvData.pData);
                    gRecvData.pData = NULL;
                }
                /* ================================update hex==================================== */
            }
        }
        else
        {
            printf("uvc mq receive failed!\n");
        }
    }

    return NULL;
}
#endif

FH_VOID uvc_extern_intr_ctrl(FH_UINT8 *data, FH_UINT32 data_size, FH_UINT32 cs)
{
    struct uvc_extern_data ext_data;

    memcpy(Uvc_ExtData, data, data_size);

    if ((cs > XU_ID_INFO_BEGIN && cs < XU_ID_INFO_END) || cs == XU_ID_AUDIO_INTF)
    {
        deal_xu_set_info((FH_UINT8)cs, Uvc_ExtData);
        return;
    }
    if (uvc_extern_mq)
    {
        memcpy(ext_data.buf, data, data_size);
        ext_data.size = data_size;
        if (mq_send(uvc_extern_mq, (const FH_CHAR *)&ext_data, sizeof(ext_data), 0))
            rt_kprintf("uvc uvc_extern_mq send failed\n");
    }
    return;
}

FH_VOID uvc_extern_intr_proc(FH_UINT8 req, FH_UINT8 cs, struct uvc_request_data *resp)
{
    FH_UINT32 len = EXTERN_DATA_LEN;

    if (cs == XU_ID_AUDIO_INTF)
        len = 0x20;

    resp->length = len;
    switch (req)
    {
    case UVC_GET_LEN:
        resp->length = 2;
        resp->data[0] = len & 0xff;
        resp->data[1] = len >> 8;
        break;

    case UVC_GET_CUR:
        /**
         * if the value could be changed only in data phase,
         * give the cur value directly
         */
        memset(Uvc_ExtBuf, 0, len);
        if ((cs > XU_ID_INFO_BEGIN && cs < XU_ID_INFO_END) || cs == XU_ID_AUDIO_INTF)
        {
            deal_xu_get_info(cs, Uvc_ExtBuf);
        }
        else
        {
            switch (Uvc_ExtData[0])
            {
            case 0xdf:
                Uvc_ExtBuf[0] = 0x0d;
                break;
            case 'V':
                memcpy(Uvc_ExtBuf, &gUvcVersion, sizeof(gUvcVersion));
                break;
            case 'U':
                strcpy(Uvc_ExtBuf, "Uvc-Cam");
                break;
            default:
                strcpy(Uvc_ExtBuf, "Fullhan");
            }
        }
        memset(Uvc_ExtData, 0, len);
        memcpy(resp->data, Uvc_ExtBuf, len);
        break;
    case UVC_GET_MIN:
        /* TODO diff type */
        memset(resp->data, 0x0, len);
        break;
    case UVC_GET_MAX:
        memset(resp->data, 0xff, len);
        break;
    case UVC_GET_DEF:
        memset(resp->data, 0x00, len);
        break;
    case UVC_GET_RES:
        memset(resp->data, 0x1, len);
        break;
    case UVC_GET_INFO:
        resp->length = 0x01;
        resp->data[0] = 0x03;
        break;
    default:
        break;
    }
}

FH_VOID fh_uvc_flash_init(FH_VOID)
{
#if defined FH_APP_USING_HEX_PARA || defined FH_APP_UVC_PARAM_HISTORY
    struct sched_param param;
    pthread_attr_t attr;
#endif
    Flash_Base_Init();
    gRecvData.pData = NULL;

#ifdef FH_APP_USING_HEX_PARA // 跟flash操作相关的，扩展协议（自定义）
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_attr_setstacksize(&attr, 4 * 1024);
    param.sched_priority = 30;
    pthread_attr_setschedparam(&attr, &param);
    if (pthread_create(&g_thread_uvc_extern_intr, &attr, uvc_extern_set, NULL) != 0)
    {
        printf("Error: Create uvc_extern_ctrl thread failed!\n");
    }
#endif

#ifdef FH_APP_UVC_PARAM_HISTORY // 使用用户设定的参数，在flash中
    extern FH_VOID *uvc_uspara_proc(FH_VOID * arg);

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_attr_setstacksize(&attr, 4 * 1024);
    param.sched_priority = 130; /* should sched_priority >= 29 */
    pthread_attr_setschedparam(&attr, &param);
    if (pthread_create(&g_thread_uvc_uspara, &attr, uvc_uspara_proc, NULL) != 0)
    {
        printf("Error: Create uvc_uspara_proc thread failed!\n");
    }
#endif
    GetFirmwareVersion();
}
