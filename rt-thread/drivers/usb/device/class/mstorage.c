/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2012-10-01     Yi Qiu       first version
 * 2012-11-25     Heyuanjie87  reduce the memory consumption
 * 2012-12-09     Heyuanjie87  change function and endpoint handler
 * 2013-07-25     Yi Qiu       update for USB CV test
 */

#include <rtthread.h>
#include <rtservice.h>
#include "../core/usbdevice.h"
#include "../core/usbcommon.h"
#include "mstorage.h"
#include "dfs_fs.h"

#include "../udc/udc_otg_dwc.h"

extern int mount(const char   *device_name,
            const char   *path,
            const char   *filesystemtype,
            unsigned long rwflag,
            const void   *data);

#define    MSTORAGE_STR_ASSOCIATION "fullhan mstorage"
#define    MSTORAGE_USB_DISK_VENDOR "FULLHAN"
#define    MSTORAGE_USB_DISK_PRODUCT "RTT U Disk"

static int g_fat32_flag = 0;

#ifdef RT_USING_RAM_BLKDEV
#define RT_USB_MSTORAGE_DISK_NAME "ramdev"
#define RAMDEV_SIZE     (10*1024*1024)
#else
#define RT_USB_MSTORAGE_DISK_NAME "mmcblk0p1"
#endif

#define USB_HANDLER_USING_THREAD

#ifdef USB_HANDLER_USING_THREAD

typedef enum
{
    EP_IN_HANDLER,
    EP_OUT_HANDLER,
} HANDLER_TYPE;

struct udisk_event
{
    ufunction_t func;
    rt_size_t size;
    HANDLER_TYPE handler_type;
};

static ufunction_t g_last_func = NULL;
static int STOP_FLAG = 0;

static struct rt_messagequeue g_udisk_mq;
#define UDISK_STACK_SIZE         (20*1024)
#define UDISK_THREAD_PREORITY    25

static struct rt_thread udisk_thread;
static rt_uint8_t udisk_thread_stack[UDISK_STACK_SIZE];
static rt_uint8_t udisk_mq_pool[(sizeof(struct udisk_event) + sizeof(void *))*8];
#endif

enum STAT
{
    STAT_CBW,
    STAT_CMD,
    STAT_CSW,
    STAT_RECEIVE,
    STAT_SEND,
};

typedef enum
{
    FIXED,
    COUNT,
    BLOCK_COUNT,
} CB_SIZE_TYPE;

typedef enum
{
    DIR_IN,
    DIR_OUT,
    DIR_NONE,
} CB_DIR;

typedef rt_size_t (*cbw_handler)(ufunction_t func, ustorage_cbw_t cbw);

struct scsi_cmd
{
    rt_uint16_t cmd;
    cbw_handler handler;
    rt_size_t cmd_len;
    CB_SIZE_TYPE type;
    rt_size_t data_size;
    CB_DIR dir;
};

struct mstorage
{
    struct ustorage_csw csw_response;
    uep_t ep_in;
    uep_t ep_out;
    int status;
    rt_uint32_t cb_data_size;
    rt_device_t disk;
    rt_off_t offset;
    rt_off_t block;
    rt_int32_t count;
    rt_int32_t size;
    rt_int32_t format;
    struct scsi_cmd* processing;
    struct rt_device_blk_geometry geometry;
};

ALIGN(4)
static struct umass_descriptor _mass_desc =
{
#ifdef FH_RT_USB_DEVICE_COMPOSITE
    /* Interface Association Descriptor */
    {
        USB_DESC_LENGTH_IAD,
        USB_DESC_TYPE_IAD,
        USB_DYNAMIC,
        0x01,
        USB_CLASS_MASS_STORAGE,
        0x06,
        0x50,
        0x00,
    },
#endif
    {
        USB_DESC_LENGTH_INTERFACE,  /* bLength; */
        USB_DESC_TYPE_INTERFACE,    /* type; */
        USB_DYNAMIC,                /* bInterfaceNumber; */
        0x00,                       /* bAlternateSetting; */
        0x02,                       /* bNumEndpoints */
        USB_CLASS_MASS_STORAGE,     /* bInterfaceClass; */
        0x06,                       /* bInterfaceSubClass; */
        0x50,                       /* bInterfaceProtocol; */
        0x00,                       /* iInterface; */
    },

    {
        USB_DESC_LENGTH_ENDPOINT,   /* bLength; */
        USB_DESC_TYPE_ENDPOINT,     /* type; */
        USB_DYNAMIC | USB_DIR_OUT,  /* bEndpointAddress; */
        USB_EP_ATTR_BULK,           /* bmAttributes; */
        512,                        /* wMaxPacketSize; */
        0x00,                       /* bInterval; */
    },

    {
        USB_DESC_LENGTH_ENDPOINT,   /* bLength; */
        USB_DESC_TYPE_ENDPOINT,     /* type; */
        USB_DYNAMIC | USB_DIR_IN,   /* bEndpointAddress; */
        USB_EP_ATTR_BULK,           /* bmAttributes; */
        512,                        /* wMaxPacketSize; */
        0x00,                       /* bInterval; */
    },
};


static const struct usb_descriptor_header * const mstorage_desc[] =
{
    (struct usb_descriptor_header *) &_mass_desc.iad_desc,
    (struct usb_descriptor_header *) &_mass_desc.intf_desc,
    (struct usb_descriptor_header *) &_mass_desc.ep_out_desc,
    (struct usb_descriptor_header *) &_mass_desc.ep_in_desc,
    NULL,
};

static rt_err_t _function_disable(ufunction_t func);
static struct scsi_cmd* _find_cbw_command(rt_uint16_t cmd);

static rt_size_t _test_unit_ready(ufunction_t func, ustorage_cbw_t cbw);
static rt_size_t _request_sense(ufunction_t func, ustorage_cbw_t cbw);
static rt_size_t _inquiry_cmd(ufunction_t func, ustorage_cbw_t cbw);
static rt_size_t _allow_removal(ufunction_t func, ustorage_cbw_t cbw);
static rt_size_t _start_stop(ufunction_t func, ustorage_cbw_t cbw);
static rt_size_t _mode_sense_6(ufunction_t func, ustorage_cbw_t cbw);
static rt_size_t _mode_sense_10(ufunction_t func, ustorage_cbw_t cbw);
static rt_size_t _read_capacities(ufunction_t func, ustorage_cbw_t cbw);
static rt_size_t _read_capacity(ufunction_t func, ustorage_cbw_t cbw);
static rt_size_t _read_10(ufunction_t func, ustorage_cbw_t cbw);
static rt_size_t _write_10(ufunction_t func, ustorage_cbw_t cbw);
static rt_size_t _verify_10(ufunction_t func, ustorage_cbw_t cbw);
/* static rt_size_t _pass_through_16(ufunction_t func, ustorage_cbw_t cbw);    // for win10 */

ALIGN(4)
static struct scsi_cmd cmd_data[] =
{
    {SCSI_TEST_UNIT_READY,      _test_unit_ready,       6,  FIXED,       0, DIR_NONE},
    {SCSI_REQUEST_SENSE,        _request_sense,         6,  COUNT,       0, DIR_IN},
    {SCSI_INQUIRY_CMD,          _inquiry_cmd,           6,  COUNT,       0, DIR_IN},
    {SCSI_ALLOW_REMOVAL,        _allow_removal,         6,  FIXED,       0, DIR_NONE},
    {SCSI_MODE_SENSE_6,         _mode_sense_6,          6,  COUNT,       0, DIR_IN},
    {SCSI_MODE_SENSE_10,         _mode_sense_10,          10,  COUNT,       0, DIR_IN},
    {SCSI_START_STOP,           _start_stop,            6,  FIXED,       0, DIR_NONE},
    {SCSI_READ_CAPACITIES,      _read_capacities,       10, COUNT,       0, DIR_NONE},
    {SCSI_READ_CAPACITY,        _read_capacity,         10, FIXED,       8, DIR_IN},
    {SCSI_WRITE_10,             _write_10,              10, BLOCK_COUNT, 0, DIR_OUT},
    {SCSI_READ_10,              _read_10,               10, BLOCK_COUNT, 0, DIR_IN},
    {SCSI_VERIFY_10,            _verify_10,             10, FIXED,       0, DIR_NONE},
    /* {ATA_PASS_THROUGH_16,       _pass_through_16,       16, FIXED,       0, DIR_IN}, */
};

static void _send_status(ufunction_t func)
{
    struct mstorage *data;

    RT_ASSERT(func != RT_NULL);

    RT_DEBUG_LOG(RT_DEBUG_USB, ("_send_status\n"));

    data = (struct mstorage*)func->user_data;
    data->ep_in->request.buffer = (rt_uint8_t*)&data->csw_response;
    data->ep_in->request.size = SIZEOF_CSW;
    data->ep_in->request.req_type = UIO_REQUEST_WRITE;
    rt_usbd_io_request(func->device, data->ep_in, &data->ep_in->request);
    data->status = STAT_CSW;
}

static rt_size_t _test_unit_ready(ufunction_t func, ustorage_cbw_t cbw)
{
    struct mstorage *data;

    RT_ASSERT(func != RT_NULL);
    RT_ASSERT(func->device != RT_NULL);

    RT_DEBUG_LOG(RT_DEBUG_USB, ("_test_unit_ready\n"));

    data = (struct mstorage*)func->user_data;
    data->csw_response.status = STOP_FLAG;

    return 0;
}

static rt_size_t _allow_removal(ufunction_t func, ustorage_cbw_t cbw)
{
    struct mstorage *data;

    RT_ASSERT(func != RT_NULL);
    RT_ASSERT(func->device != RT_NULL);

    RT_DEBUG_LOG(RT_DEBUG_USB, ("_allow_removal\n"));

    data = (struct mstorage*)func->user_data;
    data->csw_response.status = 1;

    return 0;
}

/**
 * This function will handle inquiry command request.
 *
 * @param func the usb function object.
 * @param cbw the command block wrapper.
 *
 * @return RT_EOK on successful.
 */

static rt_size_t _inquiry_cmd(ufunction_t func, ustorage_cbw_t cbw)
{
    struct mstorage *data;
    rt_uint8_t *buf;

    RT_ASSERT(func != RT_NULL);
    RT_ASSERT(func->device != RT_NULL);
    RT_ASSERT(cbw != RT_NULL);

    RT_DEBUG_LOG(RT_DEBUG_USB, ("_inquiry_cmd\n"));

    data = (struct mstorage*)func->user_data;
    buf = data->ep_in->buffer;

    *(rt_uint32_t*)&buf[0] = 0x0 | (0x80 << 8);
    *(rt_uint32_t*)&buf[4] = 31;

    rt_memset(&buf[8], 0x20, 28);
    rt_memcpy(&buf[8], MSTORAGE_USB_DISK_VENDOR, sizeof(MSTORAGE_USB_DISK_VENDOR));
    rt_memcpy(&buf[16], MSTORAGE_USB_DISK_PRODUCT, sizeof(MSTORAGE_USB_DISK_PRODUCT));

    data->cb_data_size = MIN(data->cb_data_size, SIZEOF_INQUIRY_CMD);
    data->ep_in->request.buffer = buf;
    data->ep_in->request.size = data->cb_data_size;
    data->ep_in->request.req_type = UIO_REQUEST_WRITE;
    rt_usbd_io_request(func->device, data->ep_in, &data->ep_in->request);
    data->status = STAT_CMD;

    data->csw_response.data_reside = data->cb_data_size;
    return data->cb_data_size;
}

/**
 * This function will handle sense request.
 *
 * @param func the usb function object.
 * @param cbw the command block wrapper.
 *
 * @return RT_EOK on successful.
 */
static rt_size_t _request_sense(ufunction_t func, ustorage_cbw_t cbw)
{
    struct mstorage *data;
    struct request_sense_data *buf;

    RT_ASSERT(func != RT_NULL);
    RT_ASSERT(func->device != RT_NULL);
    RT_ASSERT(cbw != RT_NULL);

    RT_DEBUG_LOG(RT_DEBUG_USB, ("_request_sense\n"));

    data = (struct mstorage*)func->user_data;
    buf = (struct request_sense_data *)data->ep_in->buffer;

    buf->ErrorCode = 0x70;
    buf->Valid = 0;
    buf->SenseKey = 5;
    buf->Information[0] = 0;
    buf->Information[1] = 0;
    buf->Information[2] = 0;
    buf->Information[3] = 0;
    buf->AdditionalSenseLength = 0x0a;
    buf->AdditionalSenseCode   = 0x3a;
    buf->AdditionalSenseCodeQualifier = 0;

    data->cb_data_size = MIN(data->cb_data_size, SIZEOF_REQUEST_SENSE);
    data->ep_in->request.buffer = (rt_uint8_t*)data->ep_in->buffer;
    data->ep_in->request.size = data->cb_data_size;
    data->ep_in->request.req_type = UIO_REQUEST_WRITE;
    rt_usbd_io_request(func->device, data->ep_in, &data->ep_in->request);
    data->status = STAT_CMD;

    return data->cb_data_size;
}

/**
 * This function will handle mode_sense_6 request.
 *
 * @param func the usb function object.
 * @param cbw the command block wrapper.
 *
 * @return RT_EOK on successful.
 */
static rt_size_t _mode_sense_6(ufunction_t func, ustorage_cbw_t cbw)
{
    struct mstorage *data;
    rt_uint8_t *buf;

    RT_ASSERT(func != RT_NULL);
    RT_ASSERT(func->device != RT_NULL);
    RT_ASSERT(cbw != RT_NULL);

    RT_DEBUG_LOG(RT_DEBUG_USB, ("_mode_sense_6\n"));

    data = (struct mstorage*)func->user_data;
    buf = data->ep_in->buffer;
    buf[0] = 3;
    buf[1] = 0;
    buf[2] = 0;
    buf[3] = 0;

    data->cb_data_size = MIN(data->cb_data_size, SIZEOF_MODE_SENSE_6);
    data->ep_in->request.buffer = buf;
    data->ep_in->request.size = data->cb_data_size;
    data->ep_in->request.req_type = UIO_REQUEST_WRITE;
    rt_usbd_io_request(func->device, data->ep_in, &data->ep_in->request);
    data->status = STAT_CMD;

    data->csw_response.data_reside = data->cb_data_size;
    return data->cb_data_size;
}

/**
 * This function will handle mode_sense_10 request.
 *
 * @param func the usb function object.
 * @param cbw the command block wrapper.
 *
 * @return RT_EOK on successful.
 */
static rt_size_t _mode_sense_10(ufunction_t func, ustorage_cbw_t cbw)
{
    struct mstorage *data;
    rt_uint8_t *buf;

    RT_ASSERT(func != RT_NULL);
    RT_ASSERT(func->device != RT_NULL);
    RT_ASSERT(cbw != RT_NULL);

    RT_DEBUG_LOG(RT_DEBUG_USB, ("_mode_sense_6\n"));

    data = (struct mstorage*)func->user_data;
    buf = data->ep_in->buffer;
    buf[0] = 0;
    buf[1] = SIZEOF_MODE_SENSE_10 - 1;
    buf[2] = 0;
    buf[3] = 0;
    buf[4] = 0;
    buf[5] = 0;
    buf[6] = 0;
    buf[7] = 0;

    data->cb_data_size = MIN(data->cb_data_size, SIZEOF_MODE_SENSE_10);
    data->ep_in->request.buffer = buf;
    data->ep_in->request.size = data->cb_data_size;
    data->ep_in->request.req_type = UIO_REQUEST_WRITE;
    rt_usbd_io_request(func->device, data->ep_in, &data->ep_in->request);
    data->status = STAT_CMD;

    data->csw_response.data_reside = data->cb_data_size;
    return data->cb_data_size;
}

/**
 * This function will handle read_capacities request.
 *
 * @param func the usb function object.
 * @param cbw the command block wrapper.
 *
 * @return RT_EOK on successful.
 */
static rt_size_t _read_capacities(ufunction_t func, ustorage_cbw_t cbw)
{
    struct mstorage *data;
    rt_uint8_t *buf;
    rt_uint32_t sector_count, sector_size;

    RT_ASSERT(func != RT_NULL);
    RT_ASSERT(func->device != RT_NULL);
    RT_ASSERT(cbw != RT_NULL);

    RT_DEBUG_LOG(RT_DEBUG_USB, ("_read_capacities\n"));

    data = (struct mstorage*)func->user_data;
    buf = data->ep_in->buffer;
    sector_count = data->geometry.sector_count;
    sector_size = data->geometry.bytes_per_sector;

    *(rt_uint32_t*)&buf[0] = 0x08000000;
    buf[4] = sector_count >> 24;
    buf[5] = 0xff & (sector_count >> 16);
    buf[6] = 0xff & (sector_count >> 8);
    buf[7] = 0xff & (sector_count);
    buf[8] = 0x02;
    buf[9] = 0xff & (sector_size >> 16);
    buf[10] = 0xff & (sector_size >> 8);
    buf[11] = 0xff & sector_size;

    data->cb_data_size = MIN(data->cb_data_size, SIZEOF_READ_CAPACITIES);
    data->ep_in->request.buffer = buf;
    data->ep_in->request.size = data->cb_data_size;
    data->ep_in->request.req_type = UIO_REQUEST_WRITE;
    rt_usbd_io_request(func->device, data->ep_in, &data->ep_in->request);
    data->status = STAT_CMD;
    data->csw_response.data_reside = data->cb_data_size;

    return data->cb_data_size;
}

/**
 * This function will handle read_capacity request.
 *
 * @param func the usb function object.
 * @param cbw the command block wapper.
 *
 * @return RT_EOK on successful.
 */
static rt_size_t _read_capacity(ufunction_t func, ustorage_cbw_t cbw)
{
    struct mstorage *data;

    rt_uint8_t *buf;
    rt_uint32_t sector_count, sector_size;

    RT_ASSERT(func != RT_NULL);
    RT_ASSERT(func->device != RT_NULL);
    RT_ASSERT(cbw != RT_NULL);

    RT_DEBUG_LOG(RT_DEBUG_USB, ("_read_capacity\n"));

    data = (struct mstorage*)func->user_data;
    buf = data->ep_in->buffer;
    sector_count = data->geometry.sector_count - 1; /* Last Logical Block Address */
    sector_size = data->geometry.bytes_per_sector;

    buf[0] = sector_count >> 24;
    buf[1] = 0xff & (sector_count >> 16);
    buf[2] = 0xff & (sector_count >> 8);
    buf[3] = 0xff & (sector_count);
    buf[4] = 0x0;
    buf[5] = 0xff & (sector_size >> 16);
    buf[6] = 0xff & (sector_size >> 8);
    buf[7] = 0xff & sector_size;

    data->cb_data_size = MIN(data->cb_data_size, SIZEOF_READ_CAPACITY);
    data->ep_in->request.buffer = buf;
    data->ep_in->request.size = data->cb_data_size;
    data->csw_response.status = STOP_FLAG;
    data->ep_in->request.req_type = UIO_REQUEST_WRITE;
    rt_usbd_io_request(func->device, data->ep_in, &data->ep_in->request);
    data->status = STAT_CMD;

    return data->cb_data_size;
}

/**
 * This function will handle read_10 request.
 *
 * @param func the usb function object.
 * @param cbw the command block wrapper.
 *
 * @return RT_EOK on successful.
 */
static rt_size_t _read_10(ufunction_t func, ustorage_cbw_t cbw)
{
    struct mstorage *data;
    rt_size_t size;
    static int second_hands = 0;

    RT_ASSERT(func != RT_NULL);
    RT_ASSERT(func->device != RT_NULL);
    RT_ASSERT(cbw != RT_NULL);

    data = (struct mstorage*)func->user_data;
    data->block = cbw->cb[2]<<24 | cbw->cb[3]<<16 | cbw->cb[4]<<8  |
                  cbw->cb[5]<<0;
    data->count = cbw->cb[7]<<8 | cbw->cb[8]<<0;

    RT_ASSERT(data->count < data->geometry.sector_count);

    data->csw_response.data_reside = data->cb_data_size;

    size = rt_device_read(data->disk, data->block - data->offset, data->ep_in->buffer, data->count);
    if (size == 0)
    {
        rt_kprintf("read data error data->disk %p data->block %d data->offset %d data->count %d\n", data->disk, data->block, data->offset, data->count);
    }

    /* fs format info */
    if (data->ep_in->buffer[0] == 0xEB &&
        data->ep_in->buffer[2] == 0x90 &&
        data->ep_in->buffer[510] == 0x55 &&
        data->ep_in->buffer[511] == 0xAA)
    {
        if (data->count == 16 && second_hands == 0)
        {
            second_hands++;
        }
        else if (data->count == 16 && second_hands)
        {
            second_hands = 0;
            if (data->format == 0)
                rt_kprintf("USB MASS STORAGE LOAD OK!!!\n");
            else if (data->format == 1)
            {
                data->format = 0;
                rt_kprintf("USB FORMAT FAT32 done!!!\n");
            } else if (data->format == 2)
            {
                data->format = 0;
                rt_kprintf("USB FORMAT exFAT32 done!!!\n");
            }
        }
    }

    data->ep_in->request.buffer = data->ep_in->buffer;
    data->ep_in->request.size = data->geometry.bytes_per_sector * data->count;
    data->ep_in->request.req_type = UIO_REQUEST_WRITE;
    rt_usbd_io_request(func->device, data->ep_in, &data->ep_in->request);

    data->status = STAT_SEND;

    return data->geometry.bytes_per_sector * data->count;
}

/**
 * This function will handle write_10 request.
 *
 * @param func the usb function object.
 * @param cbw the command block wrapper.
 *
 * @return RT_EOK on successful.
 */
static rt_size_t _write_10(ufunction_t func, ustorage_cbw_t cbw)
{
    struct mstorage *data;
    static int last_count;

    RT_ASSERT(func != RT_NULL);
    RT_ASSERT(func->device != RT_NULL);
    RT_ASSERT(cbw != RT_NULL);

    data = (struct mstorage*)func->user_data;

    data->block = cbw->cb[2]<<24 | cbw->cb[3]<<16 | cbw->cb[4]<<8  |
                  cbw->cb[5]<<0;
    data->count = cbw->cb[7]<<8 | cbw->cb[8];
    data->csw_response.data_reside = cbw->xfer_len;
    data->size = data->count * data->geometry.bytes_per_sector;

    /* 解决某些sd卡fat32格式化无法识别节点的问题 */
    if (data->count == 128 && last_count == 1 && data->format == 0)
    {
        g_fat32_flag = 1;
    }

    if (last_count == 24 &&
        data->count == 1)
    {
        data->format = 2;
        rt_kprintf("+++++++++++++format exfat+++++++++++\n");
    }
    last_count = data->count;

    RT_DEBUG_LOG(RT_DEBUG_USB, ("_write_10 count 0x%x block 0x%x 0x%x\n",
                                data->count, data->block, data->geometry.sector_count));

    data->csw_response.data_reside = data->cb_data_size;
    data->ep_out->request.buffer = data->ep_out->buffer;
    data->ep_out->request.size = data->geometry.bytes_per_sector * data->count;
    data->ep_out->request.req_type = UIO_REQUEST_READ_FULL;
    rt_usbd_io_request(func->device, data->ep_out, &data->ep_out->request);
    data->status = STAT_RECEIVE;

    return data->geometry.bytes_per_sector * data->count;
}

/**
 * This function will handle verify_10 request.
 *
 * @param func the usb function object.
 *
 * @return RT_EOK on successful.
 */
static rt_size_t _verify_10(ufunction_t func, ustorage_cbw_t cbw)
{
    struct mstorage *data;

    RT_ASSERT(func != RT_NULL);
    RT_ASSERT(func->device != RT_NULL);

    RT_DEBUG_LOG(RT_DEBUG_USB, ("_verify_10\n"));

    data = (struct mstorage*)func->user_data;
    data->csw_response.status = 0;

    return 0;
}

static rt_size_t _start_stop(ufunction_t func,
                             ustorage_cbw_t cbw)
{
    struct mstorage *data;

    RT_ASSERT(func != RT_NULL);
    RT_ASSERT(func->device != RT_NULL);

    RT_DEBUG_LOG(RT_DEBUG_USB, ("_start_stop\n"));

    data = (struct mstorage*)func->user_data;
    data->csw_response.status = 0;

    return 0;
}

/* static rt_size_t _pass_through_16(ufunction_t func, ustorage_cbw_t cbw)
{
    struct mstorage *data;

    RT_ASSERT(func != RT_NULL);
    RT_ASSERT(func->device != RT_NULL);

    data = (struct mstorage*)func->user_data;
    data->csw_response.status = 1;

    return 0;
} */

static rt_err_t _ep_in_handler(ufunction_t func, rt_size_t size)
{
    RT_ASSERT(func != RT_NULL);
    RT_ASSERT(func->device != RT_NULL);
    RT_DEBUG_LOG(RT_DEBUG_USB, ("_ep_in_handler\n"));

#ifdef USB_HANDLER_USING_THREAD

    int result;
    struct udisk_event event;

    event.func         = func;
    event.size         = size;
    event.handler_type = EP_IN_HANDLER;

    g_last_func        = func;

    result = rt_mq_send(&g_udisk_mq, (void *)&event, sizeof(struct udisk_event));
    if (result == -RT_EFULL)
    {
        rt_kprintf("%s:%d rt_mq_send\n", __func__, __LINE__);
    }
    return RT_EOK;

#else
    struct mstorage *data;

    data = (struct mstorage*)func->user_data;

    switch (data->status)
    {
        case STAT_CSW:
            if (data->ep_in->request.size != SIZEOF_CSW)
            {
                rt_kprintf("Size of csw command error\n");
                rt_usbd_ep_set_stall(func->device, data->ep_in);
            }
            else
            {
                RT_DEBUG_LOG(RT_DEBUG_USB, ("return to cbw status\n"));
                data->ep_out->request.buffer = data->ep_out->buffer;
                data->ep_out->request.size = SIZEOF_CBW;
                data->ep_out->request.req_type = UIO_REQUEST_READ_FULL;
                rt_usbd_io_request(func->device, data->ep_out, &data->ep_out->request);
                data->status = STAT_CBW;
            }
            break;
        case STAT_CMD:
            if (data->csw_response.data_reside == 0xFF)
            {
                data->csw_response.data_reside = 0;
            }
            else
            {
                data->csw_response.data_reside -= data->ep_in->request.size;
                if (data->csw_response.data_reside != 0)
                {
                    RT_DEBUG_LOG(RT_DEBUG_USB, ("data_reside %d, request %d\n",
                                                data->csw_response.data_reside, data->ep_in->request.size));
                    if (data->processing->dir == DIR_OUT)
                    {
                        rt_usbd_ep_set_stall(func->device, data->ep_out);
                    }
                    else
                    {
                        rt_usbd_ep_set_stall(func->device, data->ep_in);
                    }
                    data->csw_response.data_reside = 0;
                }
            }
            _send_status(func);
            break;
        case STAT_SEND:
            data->csw_response.data_reside -= data->ep_in->request.size;
            data->block += data->count;
            data->count = 0;
            _send_status(func);
            break;
    }

    return RT_EOK;
#endif
}

static struct scsi_cmd* _find_cbw_command(rt_uint16_t cmd)
{
    int i;

    for (i=0; i<sizeof(cmd_data)/sizeof(struct scsi_cmd); i++)
    {
        if (cmd_data[i].cmd == cmd)
            return &cmd_data[i];
    }

    return RT_NULL;
}

static void _cb_len_calc(ufunction_t func, struct scsi_cmd* cmd,
                         ustorage_cbw_t cbw)
{
    struct mstorage *data;

    RT_ASSERT(func != RT_NULL);
    RT_ASSERT(cmd != RT_NULL);
    RT_ASSERT(cbw != RT_NULL);

    data = (struct mstorage*)func->user_data;
    if (cmd->cmd_len == 6)
    {
        switch (cmd->type)
        {
            case COUNT:
                data->cb_data_size = cbw->cb[4];
                break;
            case BLOCK_COUNT:
                data->cb_data_size = cbw->cb[4] * data->geometry.bytes_per_sector;
                break;
            case FIXED:
                data->cb_data_size = cmd->data_size;
                break;
            default:
                break;
        }
    }
    else if (cmd->cmd_len == 10)
    {
        switch (cmd->type)
        {
            case COUNT:
                data->cb_data_size = cbw->cb[7]<<8 | cbw->cb[8];
                break;
            case BLOCK_COUNT:
                data->cb_data_size = (cbw->cb[7]<<8 | cbw->cb[8]) *
                                     data->geometry.bytes_per_sector;
                break;
            case FIXED:
                data->cb_data_size = cmd->data_size;
                break;
            default:
                break;
        }
    }
}

static rt_bool_t _cbw_verify(ufunction_t func, struct scsi_cmd* cmd,
                             ustorage_cbw_t cbw)
{
    struct mstorage *data;

    RT_ASSERT(cmd != RT_NULL);
    RT_ASSERT(cbw != RT_NULL);
    RT_ASSERT(func != RT_NULL);

    data = (struct mstorage*)func->user_data;

    if (cbw->xfer_len > 0 && data->cb_data_size == 0)
    {
        rt_kprintf("xfer_len > 0 && data_size == 0\n");
        return RT_FALSE;
    }

    if (cbw->xfer_len == 0 && data->cb_data_size > 0)
    {
        rt_kprintf("xfer_len == 0 && data_size > 0");
        return RT_FALSE;
    }

    if (((cbw->dflags & USB_DIR_IN) && (cmd->dir == DIR_OUT)) ||
        (!(cbw->dflags & USB_DIR_IN) && (cmd->dir == DIR_IN)))
    {
        rt_kprintf("dir error\n");
        return RT_FALSE;
    }

    if (cbw->xfer_len > data->cb_data_size)
    {
        rt_kprintf("xfer_len > data_size\n");
        return RT_FALSE;
    }

    if (cbw->xfer_len < data->cb_data_size)
    {
        rt_kprintf("xfer_len < data_size\n");
        data->cb_data_size = cbw->xfer_len;
        data->csw_response.status = 1;
    }

    return RT_TRUE;
}

static rt_size_t _cbw_handler(ufunction_t func, struct scsi_cmd* cmd,
                              ustorage_cbw_t cbw)
{
    struct mstorage *data;

    RT_ASSERT(func != RT_NULL);
    RT_ASSERT(cbw != RT_NULL);
    RT_ASSERT(cmd->handler != RT_NULL);

    data = (struct mstorage*)func->user_data;
    data->processing = cmd;

    return cmd->handler(func, cbw);
}

/**
 * This function will handle mass storage bulk out endpoint request.
 *
 * @param func the usb function object.
 * @param size request size.
 *
 * @return RT_EOK.
 */
static rt_err_t _ep_out_handler(ufunction_t func, rt_size_t size)
{
    RT_ASSERT(func != RT_NULL);
    RT_ASSERT(func->device != RT_NULL);
    RT_DEBUG_LOG(RT_DEBUG_USB, ("_ep_out_handler %d\n", size));

#ifdef USB_HANDLER_USING_THREAD

    int result;
    struct udisk_event event;

    event.func         = func;
    event.size         = size;
    event.handler_type = EP_OUT_HANDLER;

    result = rt_mq_send(&g_udisk_mq, (void *)&event, sizeof(struct udisk_event));
    if (result == -RT_EFULL)
    {
        rt_kprintf("%s:%d rt_mq_send\n", __func__, __LINE__);
    }

    return RT_EOK;

#else
    struct mstorage *data;
    struct scsi_cmd* cmd;
    rt_size_t len;
    struct ustorage_cbw* cbw;

    data = (struct mstorage*)func->user_data;
    cbw = (struct ustorage_cbw*)data->ep_out->buffer;
    if (data->status == STAT_CBW)
    {
        /* dump cbw information */
        if (cbw->signature != CBW_SIGNATURE || size != SIZEOF_CBW)
        {
            goto exit;
        }

        data->csw_response.signature = CSW_SIGNATURE;
        data->csw_response.tag = cbw->tag;
        data->csw_response.data_reside = cbw->xfer_len;
        data->csw_response.status = 0;

        RT_DEBUG_LOG(RT_DEBUG_USB, ("ep_out reside %d\n", data->csw_response.data_reside));

        cmd = _find_cbw_command(cbw->cb[0]);
        if (cmd == RT_NULL)
        {
            rt_kprintf("can't find cbw command\n");
            goto exit;
        }

        _cb_len_calc(func, cmd, cbw);
        if (!_cbw_verify(func, cmd, cbw))
        {
            goto exit;
        }

        len = _cbw_handler(func, cmd, cbw);
        if (len == 0)
        {
            _send_status(func);
        }
        return RT_EOK;
    }
    else if (data->status == STAT_RECEIVE)
    {
        RT_DEBUG_LOG(RT_DEBUG_USB, ("\nwrite size %d block 0x%x oount 0x%x\n",
                                    size, data->block, data->size));

        data->size -= size;
        data->csw_response.data_reside -= size;

        rt_device_write(data->disk, data->block - data->offset, data->ep_out->buffer, data->count);
        _send_status(func);

        return RT_EOK;
    }

exit:
    if (data->csw_response.data_reside)
    {
        if (cbw->dflags & USB_DIR_IN)
        {
            rt_usbd_ep_set_stall(func->device, data->ep_in);
        }
        else
        {
            rt_usbd_ep_set_stall(func->device, data->ep_in);
            rt_usbd_ep_set_stall(func->device, data->ep_out);
        }
    }
    data->csw_response.status = 1;
    _send_status(func);

    return -RT_ERROR;
#endif
}


/**
 * This function will handle mass storage interface request.
 *
 * @param func the usb function object.
 * @param setup the setup request.
 *
 * @return RT_EOK on successful.
 */
static rt_err_t _function_enable(ufunction_t func);
static rt_err_t _interface_handler(ufunction_t func, ureq_t setup)
{
    rt_uint8_t lun[4] = {0};

    RT_ASSERT(func != RT_NULL);
    RT_ASSERT(func->device != RT_NULL);
    RT_ASSERT(setup != RT_NULL);

    RT_DEBUG_LOG(RT_DEBUG_USB, ("mstorage_interface_handler\n"));

    switch (setup->bRequest)
    {
        case USBREQ_GET_MAX_LUN:

            RT_DEBUG_LOG(RT_DEBUG_USB, ("USBREQ_GET_MAX_LUN\n"));

            if (setup->wValue || setup->wLength != 1)
            {
                rt_kprintf("%s:%d rt_usbd_ep0_set_stall\n", __func__, __LINE__);
                rt_usbd_ep0_set_stall(func->device);
            }
            else
            {
                rt_usbd_ep0_write(func->device, lun, 1);
            }
            break;
        case USBREQ_MASS_STORAGE_RESET:

            RT_DEBUG_LOG(RT_DEBUG_USB, ("USBREQ_MASS_STORAGE_RESET\n"));

            if (setup->wValue || setup->wLength != 0)
            {
                rt_usbd_ep0_set_stall(func->device);
            }
            else
            {
                rt_usbd_ep0_write(func->device, lun, 0);    /* ignore lun */
            }
            break;
        default:
            rt_kprintf("unknown interface request\n");
            break;
    }

    return RT_EOK;
}

/**
 * This function will run mass storage function, it will be called on handle set configuration request.
 *
 * @param func the usb function object.
 *
 * @return RT_EOK on successful.
 */
static rt_err_t _function_enable(ufunction_t func)
{
    struct mstorage *data;
    RT_ASSERT(func != RT_NULL);
    RT_DEBUG_LOG(RT_DEBUG_USB, ("Mass storage function enabled\n"));
    data = (struct mstorage*)func->user_data;

    if (data->disk != NULL)
        return 0;
    rt_kprintf("Mass storage function enabled\n");

    data->disk = rt_device_find(RT_USB_MSTORAGE_DISK_NAME);
    if (data->disk == RT_NULL)
    {
        rt_kprintf("no data->disk named %s\n", RT_USB_MSTORAGE_DISK_NAME);
        return -RT_ERROR;
    }

    if (rt_device_open(data->disk, RT_DEVICE_OFLAG_RDWR) != RT_EOK)
    {
        rt_kprintf("disk open error\n");
        return -RT_ERROR;
    }

    if (rt_device_control(data->disk, RT_DEVICE_CTRL_BLK_GETGEOME,
                          (void*)&data->geometry) != RT_EOK)
    {
        rt_kprintf("get disk info error\n");
        return -RT_ERROR;
    }

    if (rt_device_control(data->disk, RT_DEVICE_CTRL_BLKPARTGET, &data->offset)!= RT_EOK)
    {
        rt_kprintf("get disk offset error\n");
        return -RT_ERROR;
    }

    data->ep_in->buffer = (rt_uint8_t*)rt_malloc(data->geometry.bytes_per_sector * 256);
    if (data->ep_in->buffer == RT_NULL)
    {
        rt_kprintf("no memory\n");
        return -RT_ENOMEM;
    }
    data->ep_out->buffer = (rt_uint8_t*)rt_malloc(data->geometry.bytes_per_sector * 256);
    if (data->ep_out->buffer == RT_NULL)
    {
        rt_free(data->ep_in->buffer);
        rt_kprintf("no memory\n");
        return -RT_ENOMEM;
    }
    STOP_FLAG = 0;
    /* prepare to read CBW request */
    data->ep_out->request.buffer = data->ep_out->buffer;
    data->ep_out->request.size = SIZEOF_CBW;
    data->ep_out->request.req_type = UIO_REQUEST_READ_FULL;
    rt_usbd_io_request(func->device, data->ep_out, &data->ep_out->request);

    return RT_EOK;
}

/**
 * This function will stop mass storage function, it will be called on handle set configuration request.
 *
 * @param device the usb device object.
 *
 * @return RT_EOK on successful.
 */
static rt_err_t _function_disable(ufunction_t func)
{
    struct mstorage *data;
    RT_ASSERT(func != RT_NULL);

    RT_DEBUG_LOG(RT_DEBUG_USB, ("Mass storage function disabled\n"));

    data = (struct mstorage*)func->user_data;
    if (data->ep_in->buffer != RT_NULL)
    {
        rt_free(data->ep_in->buffer);
        data->ep_in->buffer = RT_NULL;
    }

    if (data->ep_out->buffer != RT_NULL)
    {
        rt_free(data->ep_out->buffer);
        data->ep_out->buffer = RT_NULL;
    }
    if (data->disk != RT_NULL)
    {
        rt_device_close(data->disk);
        data->disk = RT_NULL;
    }

    data->status = STAT_CBW;

    return RT_EOK;
}

#define UVC_COPY_DESCRIPTORS(mem, dst, src) \
        do { \
            const struct usb_descriptor_header * const *__src; \
            for (__src = src; *__src; ++__src) { \
                rt_memcpy(mem, *__src, (*__src)->bLength); \
                *dst++ = mem; \
                mem += (*__src)->bLength; \
            } \
        } while (0)

static void mstorage_copy_descriptors(ufunction_t f)
{
    const struct usb_descriptor_header * const *src;
    struct usb_descriptor_header **dst;
    struct usb_descriptor_header **hdr;
    unsigned int n_desc = 0;
    unsigned int bytes = 0;
    void *mem;
    int needBufLen;

    for (src = mstorage_desc; *src; ++src)
    {
        bytes += (*src)->bLength;
        n_desc++;
    }

    if (f->desc_hdr_mem)
    {
        rt_free(f->desc_hdr_mem);
        f->desc_hdr_mem = RT_NULL;
    }

    needBufLen = (n_desc + 1) * sizeof(*src) + bytes;

    mem = rt_malloc(needBufLen);
    if (mem == NULL)
        return;

    hdr = mem;
    dst = mem;
    mem += (n_desc + 1) * sizeof(*src);

    f->desc_hdr_mem = hdr;
    f->descriptors = mem;
    f->desc_size = bytes;

    /* Copy the descriptors. */
    UVC_COPY_DESCRIPTORS(mem, dst, mstorage_desc);
    *dst = NULL;
}



static rt_err_t
mstorage_set_alt(struct ufunction *f, unsigned int interface, unsigned int alt)
{
    struct udevice* device = f->device;
    struct mstorage *data = f->user_data;

    dcd_ep_disable(device->dcd, data->ep_in);
    dcd_ep_enable(device->dcd, data->ep_in);

    dcd_ep_disable(device->dcd, data->ep_out);
    dcd_ep_enable(device->dcd, data->ep_out);

    return 0;
}

#ifdef USB_HANDLER_USING_THREAD

void remove_udisk(void)
{
    struct mstorage *data;

    if (NULL == g_last_func)
        return;

    data = (struct mstorage*)g_last_func->user_data;

    rt_usbd_ep_set_stall(g_last_func->device, data->ep_in);
    rt_usbd_ep_set_stall(g_last_func->device, data->ep_out);
    _function_disable(g_last_func);

    REG_OTG_DCTL |= DCTL_SOFT_DISCONN;

    g_last_func = NULL;

    rt_kprintf("%s done\n", __FUNCTION__);
}

static rt_err_t ep_in_handler_process(ufunction_t func, rt_size_t size)
{
    struct mstorage *data;

    RT_ASSERT(func != RT_NULL);
    RT_ASSERT(func->device != RT_NULL);

    RT_DEBUG_LOG(RT_DEBUG_USB, ("_ep_in_handler\n"));

    data = (struct mstorage*)func->user_data;

    switch (data->status)
    {
        case STAT_CSW:
            if (data->ep_in->request.size != SIZEOF_CSW)
            {
                rt_kprintf("Size of csw command error\n");
                rt_usbd_ep_set_stall(func->device, data->ep_in);
            }
            else
            {
                RT_DEBUG_LOG(RT_DEBUG_USB, ("return to cbw status\n"));
                data->ep_out->request.buffer = data->ep_out->buffer;
                data->ep_out->request.size = SIZEOF_CBW;
                data->ep_out->request.req_type = UIO_REQUEST_READ_FULL;
                rt_usbd_io_request(func->device, data->ep_out, &data->ep_out->request);
                data->status = STAT_CBW;
            }
            break;
        case STAT_CMD:
            if (data->csw_response.data_reside == 0xFF)
            {
                data->csw_response.data_reside = 0;
            }
            else
            {
                data->csw_response.data_reside -= data->ep_in->request.size;
                if (data->csw_response.data_reside != 0)
                {
                    RT_DEBUG_LOG(RT_DEBUG_USB, ("data_reside %d, request %d\n",
                                                data->csw_response.data_reside, data->ep_in->request.size));
                    if (data->processing->dir == DIR_OUT)
                    {
                        rt_usbd_ep_set_stall(func->device, data->ep_out);
                    }
                    else
                    {
                        rt_usbd_ep_set_stall(func->device, data->ep_in);
                    }
                    data->csw_response.data_reside = 0;
                }
            }
            _send_status(func);
            break;
        case STAT_SEND:
            data->csw_response.data_reside -= data->ep_in->request.size;
            data->block += data->count;
            data->count = 0;
            _send_status(func);
            break;
    }
    return RT_EOK;
}

static rt_err_t ep_out_handler_process(ufunction_t func, rt_size_t size)
{
    struct mstorage *data;
    struct scsi_cmd *cmd;
    rt_size_t len;
    struct ustorage_cbw* cbw;

    RT_ASSERT(func != RT_NULL);
    RT_ASSERT(func->device != RT_NULL);

    RT_DEBUG_LOG(RT_DEBUG_USB, ("_ep_out_handler %d\n", size));

    data = (struct mstorage*)func->user_data;
    cbw = (struct ustorage_cbw*)data->ep_out->buffer;
    if (data->status == STAT_CBW)
    {
        /* dump cbw information */
        if (cbw->signature != CBW_SIGNATURE || size != SIZEOF_CBW)
        {
            rt_kprintf("%s:%d\n", __func__, __LINE__);
            goto exit;
        }

        data->csw_response.signature = CSW_SIGNATURE;
        data->csw_response.tag = cbw->tag;
        data->csw_response.data_reside = cbw->xfer_len;
        data->csw_response.status = 0;

        RT_DEBUG_LOG(RT_DEBUG_USB, ("ep_out reside %d\n", data->csw_response.data_reside));

        cmd = _find_cbw_command((rt_uint16_t)cbw->cb[0]);
        if (cmd == RT_NULL)
        {
            rt_kprintf("can't find cbw command: 0x%x len:%d\n", cbw->cb[0], cbw->cb_len);
            goto exit;
        }

        _cb_len_calc(func, cmd, cbw);
        if (!_cbw_verify(func, cmd, cbw))
        {
            rt_kprintf("%s:%d\n", __func__, __LINE__);
            goto exit;
        }

        len = _cbw_handler(func, cmd, cbw);
        if (len == 0)
        {
            _send_status(func);

            if (cmd->cmd == SCSI_START_STOP)
            {
                STOP_FLAG = 1;
                // _function_disable(func);
                // rt_usbd_ep_set_stall(func->device, data->ep_in);
                // rt_usbd_ep_set_stall(func->device, data->ep_out);
                // rt_thread_mdelay(100);
                // REG_OTG_DCTL |= DCTL_SOFT_DISCONN;
                // rt_kprintf("remove udisk done\n");
            }
        }
        return RT_EOK;
    }
    else if (data->status == STAT_RECEIVE)
    {
        RT_DEBUG_LOG(RT_DEBUG_USB, ("\nwrite size %d block 0x%x oount 0x%x\n",
                                    size, data->block, data->size));
        data->size -= size;
        data->csw_response.data_reside -= size;

        if (data->ep_out->buffer[0] == 0xF8 &&
            data->ep_out->buffer[3] == 0x0F &&
            data->ep_out->buffer[7] == 0x0F &&
            data->ep_out->buffer[11] == 0x0F &&
            g_fat32_flag == 1)
        {
            g_fat32_flag = 0;
            data->format = 1;
            rt_kprintf("+++++++++++++format fat32+++++++++++\n");
        }
        rt_device_write(data->disk, data->block - data->offset, data->ep_out->buffer, data->count);
        _send_status(func);
        return RT_EOK;
    }

exit:
    if (data->csw_response.data_reside)
    {
        if (cbw->dflags & USB_DIR_IN)
        {
            rt_usbd_ep_set_stall(func->device, data->ep_in);
        }
        else
        {
            rt_usbd_ep_set_stall(func->device, data->ep_in);
            rt_usbd_ep_set_stall(func->device, data->ep_out);
        }
    }
    data->csw_response.status = 1;
    _send_status(func);

    return -RT_ERROR;
}

static void udisk_ep_handler(void *param)
{
    int ret = 0;
    struct udisk_event event;

    while (1)
    {
        if (rt_mq_recv(&g_udisk_mq, &event, sizeof(struct udisk_event), RT_WAITING_FOREVER) == RT_EOK)
        {
            if (event.handler_type ==  EP_IN_HANDLER)
            {
                ret = ep_in_handler_process(event.func, event.size);
            }
            else if (event.handler_type ==  EP_OUT_HANDLER)
            {
                ret = ep_out_handler_process(event.func, event.size);
            }
            else
            {
                ret = RT_ERROR;
                rt_kprintf("%s:%d type %d error!!!\n", __func__, __LINE__, event.handler_type);
            }

            if (ret != RT_EOK)
            {
                rt_kprintf("%s:%d error, handler_type:%d!!!\n", __func__, __LINE__, event.handler_type);
            }
        }
        else
        {
            rt_thread_delay(100);
        }
    }
}

void ep_handler_thread_init(void)
{
    int ret;

    rt_mq_init(&g_udisk_mq, "udisk_mq", udisk_mq_pool, sizeof(struct udisk_event),
                        sizeof(udisk_mq_pool), RT_IPC_FLAG_FIFO);

    ret = rt_thread_init(&udisk_thread, "udisk_handler", udisk_ep_handler, RT_NULL,
                 &udisk_thread_stack[0], UDISK_STACK_SIZE, UDISK_THREAD_PREORITY, 1);

    if (ret == RT_EOK)
    {
        rt_thread_startup(&udisk_thread);
    }
    else
    {
        rt_kprintf("%s:%d error!!!\n", __func__, __LINE__);
    }
}
#endif


int mstorage_bind_config(uconfig_t c)
{
    struct mstorage *data;
    udevice_t device = c->device;
    ufunction_t f;

    int ret = 0;

#ifdef USB_HANDLER_USING_THREAD
    ep_handler_thread_init();       /* sd read/write can't int ISR */
#endif

    ret = usb_string_id(c, MSTORAGE_STR_ASSOCIATION);
    if (ret < 0)
        goto error;
    _mass_desc.iad_desc.iFunction = ret;
    _mass_desc.intf_desc.iInterface = ret;

    f = rt_usbd_function_new(device);
    f->enable = _function_enable;
    f->disable = _function_disable;
    f->setup = _interface_handler;
    f->set_alt = mstorage_set_alt;

    data = (struct mstorage*)rt_malloc(sizeof(struct mstorage));
    rt_memset(data, 0, sizeof(struct mstorage));
    f->user_data = (void*)data;

    ret = usb_interface_id(c, f);
    _mass_desc.iad_desc.bFirstInterface = ret;
    _mass_desc.intf_desc.bInterfaceNumber = ret;

    data->ep_out = usb_ep_autoconfig(f, &_mass_desc.ep_out_desc, _ep_out_handler);
    data->ep_in = usb_ep_autoconfig(f, &_mass_desc.ep_in_desc, _ep_in_handler);

	mstorage_copy_descriptors(f);

	rt_usbd_config_add_function(c, f);

#ifdef RT_USING_RAM_BLKDEV
	char *buf_ram = (char *)malloc(RAMDEV_SIZE);
	if (RT_NULL != buf_ram)
	{
		rt_memset(buf_ram, 0 ,RAMDEV_SIZE);
		int retdisk = rt_ramdev_create(RT_USB_MSTORAGE_DISK_NAME, buf_ram, RAMDEV_SIZE);
		if (0 != retdisk)
		{
			rt_kprintf("-----------rt_ramdev_create fail!\n");
            goto error;
		}

        if (dfs_mkfs("elm", RT_USB_MSTORAGE_DISK_NAME))
        {
            rt_kprintf("[ERROR] dfs_mkfs error\n");
            return -RT_ERROR;
        }
        ret = mkdir("/ramfs",0);
        if (ret)
            rt_kprintf("[ERROR] mkdir error %d\n", ret);
        ret = mount(RT_USB_MSTORAGE_DISK_NAME, "ramfs", "elm", 0, 0);
        if (ret)
        {
            rt_kprintf("[ERROR] mount error %d\n", ret);
            return -RT_ERROR;
        }
	}
#else
    rt_thread_delay(10);
#endif
    return 0;
error:
    rt_kprintf("error: mstorage bind config\n");
    return ret;
}

void fh_mstorage_init(void)
{
    rt_usb_device_init();
}
