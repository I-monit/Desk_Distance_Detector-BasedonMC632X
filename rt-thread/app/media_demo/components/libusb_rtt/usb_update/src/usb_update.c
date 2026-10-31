#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <pthread.h>
#include "errno.h"
#include <fh_usb_device.h>
#include "sys/reboot.h"
#include <sys/prctl.h>
#include "libusb_rtt/usb_update/include/usb_update.h"



static FH_SINT32 sFlashBaseInited = 0;

static FH_SINT32 flash_fd = 0;
#define FB_FLASH_DEV_NAME "dev/fh_flash"
/* ============================================ */

/* ===================update========================= */
static pthread_t g_thread_uvc_update;
FH_SINT32 update_flash = UVC_UPDATE_WAIT;
/* ============================================ */

FH_UINT32 crc32_table_init = 0;
static FH_UINT32 crc32_table[256];

#define CRC_SEED 0xEDB88320

FH_VOID makeCRCTable(FH_UINT32 seed)
{
    FH_UINT32 c;
    FH_SINT32 i = 0;
    FH_SINT32 bit = 0;

    for (i = 0; i < 256; i++)
    {
        c = (FH_UINT32)i;
        for (bit = 0; bit < 8; bit++)
        {
            if (c & 1)
            {
                c = (c >> 1) ^ (seed);
            }
            else
            {
                c = c >> 1;
            }
        }
        crc32_table[i] = c;
    }
}

FH_UINT32 calcCRC(FH_UINT8 *buf, FH_UINT32 size)
{
    FH_UINT32 crc = 0xffffffff;

    while (size--)
    {
        crc = (crc >> 8) ^ (crc32_table[(crc ^ *buf++) & 0xff]);
    }
    return crc;
}

FH_SINT32 Flash_Base_Init(FH_VOID)
{
    if (sFlashBaseInited)
        return 0;

    flash_fd = open(FB_FLASH_DEV_NAME, O_RDWR);
    if (flash_fd < 0)
    {
        printf("can not find flash device\n");
        return -1;
    }

    if (flash_fd > 0)
        sFlashBaseInited = 1;

    return 0;
}

FH_UINT32 Flash_Base_Read(FH_SINT32 offset, FH_VOID *buffer, FH_UINT32 size)
{
    FH_SINT32 ret;
    FH_UINT32 temp_size, align_offset, rema_offset;
    FH_CHAR *temp_buf = NULL;

    if (!sFlashBaseInited || !flash_fd)
    {
        printf("error, sFlashBaseInited=%x, sFlashDev=%x\n", sFlashBaseInited, flash_fd);
        return 0;
    }

    align_offset = offset & 0xfffffe00;
    rema_offset = offset & 0x1ff;
    lseek(flash_fd, align_offset, SEEK_SET);

    temp_size = (rema_offset + size + 511) & 0xfffffe00;

    temp_buf = malloc(temp_size);
    if (!temp_buf)
    {
        printf("FlashErr:MallocFail(L%d)\n", __LINE__);
        return -ENOMEM;
    }
    ret = read(flash_fd, temp_buf, temp_size);
    if (ret < 0)
    {
        printf("FlashErr:ReadFail %d(L%d)\n", ret, __LINE__);
        free(temp_buf);
        return -EIO;
    }
    memcpy(buffer, temp_buf + rema_offset, size);

    free(temp_buf);

    return size;
}

FH_UINT32 Flash_Base_Write(FH_SINT32 offset, FH_VOID *buffer, FH_UINT32 size)
{
    FH_SINT32 ret;
    FH_UINT32 temp_size, align_offset, rema_offset;
    FH_CHAR *temp_buf = NULL;

    if (!sFlashBaseInited || !flash_fd)
    {
        printf("error, sFlashBaseInited=%x, sFlashDev=%x\n", sFlashBaseInited, flash_fd);
        return 0;
    }

    align_offset = offset & 0xfffffe00;
    rema_offset = offset & 0x1ff;
    lseek(flash_fd, align_offset, SEEK_SET);

    temp_size = (rema_offset + size + 511) & 0xfffffe00;

    temp_buf = malloc(temp_size);
    if (!temp_buf)
    {
        printf("FlashErr:MallocFail(L%d)\n", __LINE__);
        return -ENOMEM;
    }
    ret = read(flash_fd, temp_buf, temp_size);
    if (ret < 0)
    {
        printf("FlashErr:ReadFail %d(L%d)\n", ret, __LINE__);
        free(temp_buf);
        return -EIO;
    }

    memcpy(temp_buf + rema_offset, buffer, size);

    lseek(flash_fd, align_offset, SEEK_SET);

    ret = write(flash_fd, temp_buf, temp_size);
    if (ret < 0)
    {
        printf("FlashErr:WriteFail %d(L%d)\n", ret, __LINE__);
        free(temp_buf);
        return -EIO;
    }

    free(temp_buf);

    return size;
}

static FH_VOID *uvc_update_proc(FH_VOID *arg)
{
    FH_CHAR buf[32] = {0};

    prctl(PR_SET_NAME, "uvc_update_proc");
    if (!crc32_table_init)
    {
        crc32_table_init = 1;
        makeCRCTable(CRC_SEED);
    }
    while (!update_flash)
    {
        usleep(100 * 1000);
    }

    if (update_flash == UVC_UPDATE_VCOM)
    {
        strcpy(buf, "COMUPDATE");
    }
    else if (update_flash == UVC_UPDATE_DFUA)
    {
        strcpy(buf, "DFUA");
    }
    else if (update_flash == UVC_UPDATE_DFUP)
    {
        strcpy(buf, "DFUP");
    }
    else if (update_flash == UVC_UPDATE_UDISK)
    {
        strcpy(buf, "UDISK UPGRADE");
    }
    else if (update_flash == UVC_UPDATE_HID)
    {
        strcpy(buf, "HID ");
        strcat(buf, "UPGRADE");
    }
    printf("---update, reboot addr:0x%x\n", UPDATE_CFG_ADDR);

    Flash_Base_Write(UPDATE_CFG_ADDR, buf, sizeof(buf));
    Flash_Base_Read(UPDATE_CFG_ADDR, buf, 20);
    printf("---updatecfg = %s, will reset!\n", buf);
    usleep(1000);
    reboot(0x01234567);
    return NULL;
}

FH_VOID fh_uvc_update_init(FH_VOID)
{
    struct sched_param param;
    pthread_attr_t attr;

    Flash_Base_Init();

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_attr_setstacksize(&attr, 4 * 1024);
    param.sched_priority = 30;
    pthread_attr_setschedparam(&attr, &param);
    if (pthread_create(&g_thread_uvc_update, &attr, uvc_update_proc, NULL) != 0)
    {
        printf("Error: Create uvc_update_proc thread failed!\n");
    }
}

static FH_VOID usb_update(FH_CHAR *szData)
{
    if (0 == strcmp(szData, "com"))
    {
        update_flash = UVC_UPDATE_VCOM;
    }
    else if (0 == strcmp(szData, "dfua"))
    {
        update_flash = UVC_UPDATE_DFUA;
    }
    else if (0 == strcmp(szData, "dfup"))
    {
        update_flash = UVC_UPDATE_DFUP;
    }
    else if (0 == strcmp(szData, "udisk"))
    {
        update_flash = UVC_UPDATE_UDISK;
    }
    else if (0 == strcmp(szData, "hid"))
    {
        update_flash = UVC_UPDATE_HID;
    }
    else if (0 == strcmp(szData, "uvc"))
    {
        update_flash = UVC_UPDATE_WAIT;
    }
    return;
}

FH_VOID usb_update_check(FH_UINT8 *report, FH_UINT32 size)
{
    struct update_cmd *pCmd = (struct update_cmd *)report;
    FH_UINT8 crc = 0;
    FH_SINT32 i;

    if (size < sizeof(struct update_cmd) || pCmd->len > (sizeof(struct update_cmd) - 4))
    {
        return;
    }

    if (pCmd->header != HID_CMD_HEADER)
    {
        printf("update_cmd header error, need=%02X, get=%02X\n", HID_CMD_HEADER, pCmd->header);
        return;
    }

    for (i = 0; i < pCmd->len; i++)
    {
        crc ^= pCmd->data[i];
    }

    if (crc != pCmd->crc)
    {
        printf("update_cmd crc error, calc=%02X, get=%02X\n", crc, pCmd->crc);
        return;
    }

    printf("update_cmd crc ok\n");
    switch (pCmd->data[0])
    {
    case HID_CMD_UPDATE:
        usb_update((FH_CHAR *)(&pCmd->data[1]));
        break;
    default:
        printf("update_cmd crc error, calc=%02X, get=%02X\n", crc, pCmd->crc);
        break;
    }
}

FH_VOID uvc_update(FH_SINT32 argc, FH_CHAR *argv[])
{
    update_flash = UVC_UPDATE_UDISK;

    if (argc < 2)
        return;

    FH_CHAR *szData = argv[1];
    if (0 == strcmp(szData, "com"))
    {
        update_flash = UVC_UPDATE_VCOM;
    }
    else if (0 == strcmp(szData, "dfua"))
    {
        update_flash = UVC_UPDATE_DFUA;
    }
    else if (0 == strcmp(szData, "dfup"))
    {
        update_flash = UVC_UPDATE_DFUP;
    }
    else if (0 == strcmp(szData, "udisk"))
    {
        update_flash = UVC_UPDATE_UDISK;
    }
    else if (0 == strcmp(szData, "hid"))
    {
        update_flash = UVC_UPDATE_HID;
    }
    else if (0 == strcmp(szData, "uvc"))
    {
        update_flash = UVC_UPDATE_WAIT;
    }
    return;
}
#include <rttshell.h>
SHELL_CMD_EXPORT(uvc_update, uvc_update());
