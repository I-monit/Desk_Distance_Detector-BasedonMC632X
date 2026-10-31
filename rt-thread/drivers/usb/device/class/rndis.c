/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author            Notes
 * 2012-12-24     heyuanjie87       first version
 * 2013-04-13     aozima            update ethernet driver.
 * 2013-04-26     aozima            align the desc to 4byte.
 * 2013-05-08     aozima            pad a dummy when send MAX_PKT_SIZE.
 * 2013-05-09     aozima            add delay linkup feature.
 * 2013-07-09     aozima            support respone chain list.
 * 2013-07-18     aozima            re-initial respone chain list when RNDIS restart.
 * 2017-11-25     ZYH               fix it and add OS descriptor
 */

#include <rtdevice.h>
#include "../core/usbdevice.h"
#include "../core/usbcommon.h"
#include "cdc.h"
#include "rndis.h"
#include "ndis.h"

//#define RNDIS_DEBUG
//#define RNDIS_DELAY_LINK_UP
#define RNDIS_STR_ASSOCIATION "fullhan rndis"

#ifdef  RNDIS_DEBUG
#define RNDIS_PRINTF                rt_kprintf("[RNDIS] "); rt_kprintf
#else
#define RNDIS_PRINTF(...)
#endif /* RNDIS_DEBUG */

/* RT-Thread LWIP ethernet interface */
#include <netif/ethernetif.h>


struct rt_rndis_response
{
    struct rt_list_node list;
    const void * buffer;

};


#define MAX_ADDR_LEN    6
struct rt_rndis_eth
{
    /* inherit from ethernet device */
    struct net_device parent;
    struct ufunction *func;
    /* interface address info */
    rt_uint8_t  host_addr[MAX_ADDR_LEN];
    rt_uint8_t  dev_addr[MAX_ADDR_LEN];

#ifdef RNDIS_DELAY_LINK_UP
    struct rt_timer timer;
#endif /* RNDIS_DELAY_LINK_UP */

    ALIGN(4)
    rt_uint8_t rx_pool[512];
    ALIGN(4)
    rt_uint8_t tx_pool[512];

    rt_uint32_t cmd_pool[2];
    ALIGN(4)
    // char rx_buffer[sizeof(struct rndis_packet_msg) + USB_ETH_MTU + 14];
    struct pbuf* rx_buffer;
    rt_size_t rx_offset;
    rt_size_t rx_length;
    rt_bool_t rx_flag;
    rt_bool_t rx_frist;

    ALIGN(4)
    char tx_buffer[2][sizeof(struct rndis_packet_msg) + USB_ETH_MTU + 14];
    struct rt_semaphore tx_buffer_free;

    struct rt_list_node response_list;
    rt_bool_t  need_notify;
    struct cdc_eps eps;
    rt_uint32_t cur_id;
};
typedef struct rt_rndis_eth * rt_rndis_eth_t;
static rt_uint32_t oid_packet_filter = 0x0000000;

/* communcation interface descriptor */
ALIGN(4)
static struct ucdc_comm_descriptor _comm_desc =
{
#ifdef FH_RT_USB_DEVICE_COMPOSITE
    /* Interface Association Descriptor */
    {
        USB_DESC_LENGTH_IAD,
        USB_DESC_TYPE_IAD,
        USB_DYNAMIC,
        0x02,
        USB_CDC_CLASS_COMM,
        USB_CDC_SUBCLASS_ACM,
        USB_CDC_PROTOCOL_VENDOR,
        0x00,
    },
#endif
    /* Interface Descriptor */
    {
        USB_DESC_LENGTH_INTERFACE,
        USB_DESC_TYPE_INTERFACE,
        USB_DYNAMIC,
        0x00,
        0x01,
        USB_CDC_CLASS_COMM,
        USB_CDC_SUBCLASS_ACM,
        USB_CDC_PROTOCOL_VENDOR,
        0x00,
    },
    /* Header Functional Descriptor */
    {
        0x05,
        USB_CDC_CS_INTERFACE,
        USB_CDC_SCS_HEADER,
        0x0110,
    },
    /* Call Management Functional Descriptor */
    {
        0x05,
        USB_CDC_CS_INTERFACE,
        USB_CDC_SCS_CALL_MGMT,
        0x00,
        USB_DYNAMIC,
    },
    /* Abstract Control Management Functional Descriptor */
    {
        0x04,
        USB_CDC_CS_INTERFACE,
        USB_CDC_SCS_ACM,
        0x00,
    },
    /* Union Functional Descriptor */
    {
        0x05,
        USB_CDC_CS_INTERFACE,
        USB_CDC_SCS_UNION,
        USB_DYNAMIC,
        USB_DYNAMIC,
    },
    /* Endpoint Descriptor */
    {
        USB_DESC_LENGTH_ENDPOINT,
        USB_DESC_TYPE_ENDPOINT,
        USB_DIR_IN | USB_DYNAMIC,
        USB_EP_ATTR_INT,
        0x08,
        0x09,
    },
};

/* data interface descriptor */
ALIGN(4)
static struct ucdc_data_descriptor _data_desc =
{
    /* interface descriptor */
    {
        USB_DESC_LENGTH_INTERFACE,
        USB_DESC_TYPE_INTERFACE,
        USB_DYNAMIC,
        0x00,
        0x02,
        USB_CDC_CLASS_DATA,
        0x00,
        0x00,
        0x00,
    },
    /* endpoint, bulk out */
    {
        USB_DESC_LENGTH_ENDPOINT,
        USB_DESC_TYPE_ENDPOINT,
        USB_DIR_OUT | USB_DYNAMIC,
        USB_EP_ATTR_BULK,
        512,
        0x00,
    },
    /* endpoint, bulk in */
    {
        USB_DESC_LENGTH_ENDPOINT,
        USB_DESC_TYPE_ENDPOINT,
        USB_DYNAMIC | USB_DIR_IN,
        USB_EP_ATTR_BULK,
        512,
        0x00,
    },
};

static const struct usb_descriptor_header * const rndis_desc[] = {
    (struct usb_descriptor_header *) &_comm_desc.iad_desc,
    (struct usb_descriptor_header *) &_comm_desc.intf_desc,
    (struct usb_descriptor_header *) &_comm_desc.hdr_desc,
    (struct usb_descriptor_header *) &_comm_desc.call_mgmt_desc,
    (struct usb_descriptor_header *) &_comm_desc.acm_desc,
    (struct usb_descriptor_header *) &_comm_desc.union_desc,
    (struct usb_descriptor_header *) &_comm_desc.ep_desc,
    (struct usb_descriptor_header *) &_data_desc.intf_desc,
    (struct usb_descriptor_header *) &_data_desc.ep_in_desc,
    (struct usb_descriptor_header *) &_data_desc.ep_out_desc,
    NULL,
};


ALIGN(4)
struct usb_os_function_comp_id_descriptor rndis_func_comp_id_desc =
{
    .bFirstInterfaceNumber = USB_DYNAMIC,
    .reserved1          = 0x01,
    .compatibleID       = {'R', 'N', 'D', 'I', 'S', 0x00, 0x00, 0x00},
    .subCompatibleID    = {'5', '1', '6', '2', '0', '0', '1', 0x00},
    .reserved2          = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
};

/* supported OIDs */
ALIGN(4)
const static rt_uint32_t oid_supported_list[] =
{
    /* General OIDs */
    OID_GEN_SUPPORTED_LIST,
    OID_GEN_HARDWARE_STATUS,
    OID_GEN_MEDIA_SUPPORTED,
    OID_GEN_MEDIA_IN_USE,
    OID_GEN_MAXIMUM_FRAME_SIZE,
    OID_GEN_LINK_SPEED,
    OID_GEN_TRANSMIT_BLOCK_SIZE,
    OID_GEN_RECEIVE_BLOCK_SIZE,
    OID_GEN_VENDOR_ID,
    OID_GEN_VENDOR_DESCRIPTION,
    OID_GEN_VENDOR_DRIVER_VERSION,
    OID_GEN_CURRENT_PACKET_FILTER,
    OID_GEN_MAXIMUM_TOTAL_SIZE,
    OID_GEN_MEDIA_CONNECT_STATUS,

    OID_GEN_PHYSICAL_MEDIUM,

    /* General Statistic OIDs */
    OID_GEN_XMIT_OK,
    OID_GEN_RCV_OK,
    OID_GEN_XMIT_ERROR,
    OID_GEN_RCV_ERROR,
    OID_GEN_RCV_NO_BUFFER,

    /* Please configure us */
    OID_GEN_RNDIS_CONFIG_PARAMETER,

    /* 802.3 OIDs */
    OID_802_3_PERMANENT_ADDRESS,
    OID_802_3_CURRENT_ADDRESS,
    OID_802_3_MULTICAST_LIST,
    OID_802_3_MAXIMUM_LIST_SIZE,

    /* 802.3 Statistic OIDs */
    OID_802_3_RCV_ERROR_ALIGNMENT,
    OID_802_3_XMIT_ONE_COLLISION,
    OID_802_3_XMIT_MORE_COLLISIONS,

    OID_802_3_MAC_OPTIONS,
};

static rt_uint8_t rndis_message_buffer[RNDIS_MESSAGE_BUFFER_SIZE];

static void _rndis_response_available(ufunction_t func)
{
    rt_rndis_eth_t device = (rt_rndis_eth_t)func->user_data;
    rt_uint32_t * data;

    if(device->need_notify == RT_TRUE)
    {
        device->need_notify = RT_FALSE;
        data = (rt_uint32_t *)device->eps.ep_cmd->buffer;
        data[0] = RESPONSE_AVAILABLE;
        data[1] = 0;
        device->eps.ep_cmd->request.buffer = device->eps.ep_cmd->buffer;
        device->eps.ep_cmd->request.size = 8;
        device->eps.ep_cmd->request.req_type = UIO_REQUEST_WRITE;
        rt_usbd_io_request(func->device, device->eps.ep_cmd, &device->eps.ep_cmd->request);
    }
}

static rt_err_t _rndis_init_response(ufunction_t func, rndis_init_msg_t msg)
{
    rndis_init_cmplt_t resp;
    struct rt_rndis_response * response;

    response = rt_malloc(sizeof(struct rt_rndis_response));
    resp = rt_malloc(sizeof(struct rndis_init_cmplt));

    if( (response == RT_NULL) || (resp == RT_NULL) )
    {
        RNDIS_PRINTF("%d: no memory!\r\n", __LINE__);

        if(response != RT_NULL)
            rt_free(response);

        if(resp != RT_NULL)
            rt_free(resp);

        return -RT_ENOMEM;
    }

    resp->RequestId = msg->RequestId;
    resp->MessageType = REMOTE_NDIS_INITIALIZE_CMPLT;
    resp->MessageLength = sizeof(struct rndis_init_cmplt);
    resp->MajorVersion = RNDIS_MAJOR_VERSION;
    resp->MinorVersion = RNDIS_MINOR_VERSION;
    resp->Status = RNDIS_STATUS_SUCCESS;
    resp->DeviceFlags = RNDIS_DF_CONNECTIONLESS;
    resp->Medium = RNDIS_MEDIUM_802_3;
    resp->MaxPacketsPerTransfer = 1;
    resp->MaxTransferSize = USB_ETH_MTU + 58;   /* Space for 1280 IP buffer, Ethernet Header,
                                                RNDIS messages */
    resp->PacketAlignmentFactor = 0;
    resp->AfListOffset = 0;
    resp->AfListSize = 0;

    response->buffer = resp;

    {
        rt_base_t level = rt_hw_interrupt_disable();
        rt_list_insert_before(&((rt_rndis_eth_t)func->user_data)->response_list, &response->list);
        rt_hw_interrupt_enable(level);
    }


    return RT_EOK;
}

static rndis_query_cmplt_t _create_resp(rt_size_t size)
{
    rndis_query_cmplt_t resp;

    resp = rt_malloc(sizeof(struct rndis_query_cmplt) + size);

    if(resp == RT_NULL)
    {
        RNDIS_PRINTF("%d: no memory!\r\n", __LINE__);
        return RT_NULL;
    }

    resp->InformationBufferLength = size;

    return resp;
}

static void _copy_resp(rndis_query_cmplt_t resp, const void * buffer)
{
    char * resp_buffer = (char *)resp + sizeof(struct rndis_query_cmplt);
    rt_memcpy(resp_buffer, buffer, resp->InformationBufferLength);
}

static void _set_resp(rndis_query_cmplt_t resp, rt_uint32_t value)
{
    rt_uint32_t * response = (rt_uint32_t *)((char *)resp + sizeof(struct rndis_query_cmplt));
    *response = value;
}

static rt_err_t _rndis_query_response(ufunction_t func,rndis_query_msg_t msg)
{
    rndis_query_cmplt_t resp = RT_NULL;
    struct rt_rndis_response * response;
    rt_err_t ret = RT_EOK;

    switch (msg->Oid)
    {
        /*
         * general OIDs
         */
    case OID_GEN_SUPPORTED_LIST:
        resp = _create_resp(sizeof(oid_supported_list));
        if(resp == RT_NULL) break;
        _copy_resp(resp, oid_supported_list);
        break;

    case OID_GEN_PHYSICAL_MEDIUM:
        resp = _create_resp(4);
        if(resp == RT_NULL) break;
        _set_resp(resp, NDIS_MEDIUM_802_3);
        break;

    case OID_GEN_MAXIMUM_FRAME_SIZE:
    case OID_GEN_TRANSMIT_BLOCK_SIZE:
    case OID_GEN_RECEIVE_BLOCK_SIZE:
        resp = _create_resp(4);
        if(resp == RT_NULL) break;
        _set_resp(resp, USB_ETH_MTU);
        break;

    case OID_GEN_MAXIMUM_TOTAL_SIZE:
        resp = _create_resp(4);
        if(resp == RT_NULL) break;
        _set_resp(resp, USB_ETH_MTU + RNDIS_MESSAGE_BUFFER_SIZE);
        break;

    case OID_GEN_LINK_SPEED:
        resp = _create_resp(4);
        if(resp == RT_NULL) break;
        _set_resp(resp, func->device->dcd->device_is_hs ? (480UL * 1000 *1000) : (12UL * 1000 * 1000) / 100);
        break;

    case OID_GEN_MEDIA_CONNECT_STATUS:
        /* link_status */
        resp = _create_resp(4);
        if(resp == RT_NULL) break;

#ifdef  RNDIS_DELAY_LINK_UP
        if(((rt_rndis_eth_t)func->user_data)->parent.link_status)
        {
            _set_resp(resp, NDIS_MEDIA_STATE_CONNECTED);
        }
        else
        {
            _set_resp(resp, NDIS_MEDIA_STATE_DISCONNECTED);
        }
#else
        _set_resp(resp, NDIS_MEDIA_STATE_CONNECTED);
#endif /* RNDIS_DELAY_LINK_UP */
        break;

    case OID_GEN_VENDOR_ID:
        resp = _create_resp(4);
        if(resp == RT_NULL) break;
        _set_resp(resp, 0x12345678);    /* only for test */
        break;

    case OID_GEN_VENDOR_DESCRIPTION:
    {
        const char vendor_desc[] = "RT-Thread RNDIS";

        resp = _create_resp(sizeof(vendor_desc));
        if(resp == RT_NULL) break;
        _copy_resp(resp, vendor_desc);
    }
    break;

    case OID_GEN_VENDOR_DRIVER_VERSION:
        resp = _create_resp(4);
        if(resp == RT_NULL) break;
        _set_resp(resp, 0x0000200);
        break;

        /* statistics OIDs */
    case OID_GEN_XMIT_OK:
    case OID_GEN_RCV_OK:
        resp = _create_resp(4);
        if(resp == RT_NULL) break;
        _set_resp(resp, 1);
        break;

    case OID_GEN_XMIT_ERROR:
    case OID_GEN_RCV_ERROR:
    case OID_GEN_RCV_NO_BUFFER:
        resp = _create_resp(4);
        if(resp == RT_NULL) break;
        _set_resp(resp, 0);
        break;

        /*
         * ieee802.3 OIDs
         */
    case OID_802_3_MAXIMUM_LIST_SIZE:
        resp = _create_resp(4);
        if(resp == RT_NULL) break;
        _set_resp(resp, 1);
        break;

    case OID_802_3_PERMANENT_ADDRESS:
    case OID_802_3_CURRENT_ADDRESS:
        resp = _create_resp(sizeof(((rt_rndis_eth_t)func->user_data)->host_addr));
        if(resp == RT_NULL) break;
        _copy_resp(resp, ((rt_rndis_eth_t)func->user_data)->host_addr);
        break;

    case OID_802_3_MULTICAST_LIST:
        resp = _create_resp(4);
        if(resp == RT_NULL) break;
        _set_resp(resp, 0xE000000);
        break;

    case OID_802_3_MAC_OPTIONS:
        resp = _create_resp(4);
        if(resp == RT_NULL) break;
        _set_resp(resp, 0);
        break;

    default:
        RNDIS_PRINTF("OID %X\n", msg->Oid);
        ret = -RT_ERROR;
        break;
    }

    response = rt_malloc(sizeof(struct rt_rndis_response));
    if( (response == RT_NULL) || (resp == RT_NULL) )
    {
        RNDIS_PRINTF("%d: no memory!\r\n", __LINE__);

        if(response != RT_NULL)
            rt_free(response);

        if(resp != RT_NULL)
            rt_free(resp);

        return -RT_ENOMEM;
    }

    resp->RequestId = msg->RequestId;
    resp->MessageType = REMOTE_NDIS_QUERY_CMPLT;
    resp->InformationBufferOffset = 16;

    resp->Status = RNDIS_STATUS_SUCCESS;
    resp->MessageLength = sizeof(struct rndis_query_cmplt) + resp->InformationBufferLength;

    response->buffer = resp;

    {
        rt_base_t level = rt_hw_interrupt_disable();
        rt_list_insert_before(&((rt_rndis_eth_t)func->user_data)->response_list, &response->list);
        rt_hw_interrupt_enable(level);
    }

    return ret;
}

static rt_err_t _rndis_set_response(ufunction_t func,rndis_set_msg_t msg)
{
    rndis_set_cmplt_t resp;
    struct rt_rndis_response * response;

    response = rt_malloc(sizeof(struct rt_rndis_response));
    resp = rt_malloc(sizeof(struct rndis_set_cmplt));

    if( (response == RT_NULL) || (resp == RT_NULL) )
    {
        RNDIS_PRINTF("%d: no memory!\r\n", __LINE__);

        if(response != RT_NULL)
            rt_free(response);

        if(resp != RT_NULL)
            rt_free(resp);

        return -RT_ENOMEM;
    }

    resp->RequestId = msg->RequestId;
    resp->MessageType = REMOTE_NDIS_SET_CMPLT;
    resp->MessageLength = sizeof(struct rndis_set_cmplt);

    switch (msg->Oid)
    {
    case OID_GEN_CURRENT_PACKET_FILTER:
        oid_packet_filter = *((rt_uint32_t *)((rt_uint8_t *)&(msg->RequestId) + \
                                              msg->InformationBufferOffset));
        oid_packet_filter = oid_packet_filter;
        RNDIS_PRINTF("OID_GEN_CURRENT_PACKET_FILTER\r\n");

#ifdef  RNDIS_DELAY_LINK_UP
        /* link up. */
        rt_timer_start(&((rt_rndis_eth_t)func->user_data)->timer);
#else
        // eth_device_linkchange(&((rt_rndis_eth_t)func->user_data)->parent, RT_TRUE);
        net_device_linkchange(&((rt_rndis_eth_t)func->user_data)->parent, 1);
#endif /* RNDIS_DELAY_LINK_UP */
        break;

    case OID_802_3_MULTICAST_LIST:
        break;

    default:
        resp->Status = RNDIS_STATUS_FAILURE;
        return RT_EOK;
    }

    resp->Status = RNDIS_STATUS_SUCCESS;

    response->buffer = resp;

    {
        rt_base_t level = rt_hw_interrupt_disable();
        rt_list_insert_before(&((rt_rndis_eth_t)func->user_data)->response_list, &response->list);
        rt_hw_interrupt_enable(level);
    }

    return RT_EOK;
}

static rt_err_t _rndis_keepalive_response(ufunction_t func,rndis_keepalive_msg_t msg)
{
    rndis_keepalive_cmplt_t resp;
    struct rt_rndis_response * response;

    response = rt_malloc(sizeof(struct rt_rndis_response));
    resp = rt_malloc(sizeof(struct rndis_keepalive_cmplt));

    if( (response == RT_NULL) || (resp == RT_NULL) )
    {
        RNDIS_PRINTF("%d: no memory!\r\n", __LINE__);

        if(response != RT_NULL)
            rt_free(response);

        if(resp != RT_NULL)
            rt_free(resp);

        return -RT_ENOMEM;
    }

    resp->MessageType = REMOTE_NDIS_KEEPALIVE_CMPLT;
    resp->MessageLength = sizeof(struct rndis_keepalive_cmplt);
    resp->Status = RNDIS_STATUS_SUCCESS;

    response->buffer = resp;

    {
        rt_base_t level = rt_hw_interrupt_disable();
        rt_list_insert_before(&((rt_rndis_eth_t)func->user_data)->response_list, &response->list);
        rt_hw_interrupt_enable(level);
    }

    return RT_EOK;
}

static rt_err_t _rndis_msg_parser(ufunction_t func, rt_uint8_t *msg)
{
    rt_err_t ret = -RT_ERROR;

    switch (((rndis_gen_msg_t) msg)->MessageType)
    {
    case REMOTE_NDIS_INITIALIZE_MSG:
        ret = _rndis_init_response(func, (rndis_init_msg_t) msg);
        break;

    case REMOTE_NDIS_HALT_MSG:
        RNDIS_PRINTF("halt\n");
        /* link down. */
        // eth_device_linkchange(&((rt_rndis_eth_t)func->user_data)->parent, RT_FALSE);
        net_device_linkchange(&((rt_rndis_eth_t)func->user_data)->parent, 0);

        break;

    case REMOTE_NDIS_QUERY_MSG:
        ret = _rndis_query_response(func,(rndis_query_msg_t) msg);
        break;

    case REMOTE_NDIS_SET_MSG:
        ret = _rndis_set_response(func,(rndis_set_msg_t) msg);
        RNDIS_PRINTF("set\n");
        break;

    case REMOTE_NDIS_RESET_MSG:
        RNDIS_PRINTF("reset\n");
        break;

    case REMOTE_NDIS_KEEPALIVE_MSG:
        ret = _rndis_keepalive_response(func,(rndis_keepalive_msg_t) msg);
        break;

    default:
        RNDIS_PRINTF("msg %X\n", ((rndis_gen_msg_t) msg)->MessageType);
        ret = -RT_ERROR;
        break;
    }

    if (ret == RT_EOK)
        _rndis_response_available(func);

    return ret;
}

static ufunction_t function = RT_NULL;
static rt_err_t send_encapsulated_command_done(udevice_t device, rt_size_t size)
{
    if(function != RT_NULL)
    {
        dcd_ep0_send_status(device->dcd);
        _rndis_msg_parser(function, rndis_message_buffer);
        function = RT_NULL;
    }
    return RT_EOK;
}
//#error here have bug ep 0x82 send failed
static rt_err_t _rndis_send_encapsulated_command(ufunction_t func, ureq_t setup)
{
    RT_ASSERT(setup->wLength <= sizeof(rndis_message_buffer));
    function = func;
    rt_usbd_ep0_read(func->device,rndis_message_buffer,setup->wLength,send_encapsulated_command_done);

    return RT_EOK;
}

static rt_err_t _rndis_get_encapsulated_response(ufunction_t func, ureq_t setup)
{
    rndis_gen_msg_t msg;
    struct rt_rndis_response * response;

    if(rt_list_isempty(&((rt_rndis_eth_t)func->user_data)->response_list))
    {
        RNDIS_PRINTF("response_list is empty!\r\n");
        ((rt_rndis_eth_t)func->user_data)->need_notify = RT_TRUE;
        return RT_EOK;
    }

    response = (struct rt_rndis_response *)((rt_rndis_eth_t)func->user_data)->response_list.next;

    msg = (rndis_gen_msg_t)response->buffer;
    rt_usbd_ep0_write(func->device, (void*)msg, msg->MessageLength);

    {
        rt_base_t level = rt_hw_interrupt_disable();
        rt_list_remove(&response->list);
        rt_hw_interrupt_enable(level);
    }

    rt_free((void *)response->buffer);
    rt_free(response);

    if(!rt_list_isempty(&((rt_rndis_eth_t)func->user_data)->response_list))
    {
        rt_uint32_t * data;

        RNDIS_PRINTF("auto append next response!\r\n");
        data = (rt_uint32_t *)((rt_rndis_eth_t)func->user_data)->eps.ep_cmd->buffer;
        data[0] = RESPONSE_AVAILABLE;
        data[1] = 0;
        ((rt_rndis_eth_t)func->user_data)->eps.ep_cmd->request.buffer = ((rt_rndis_eth_t)func->user_data)->eps.ep_cmd->buffer;
        ((rt_rndis_eth_t)func->user_data)->eps.ep_cmd->request.size = 8;
        ((rt_rndis_eth_t)func->user_data)->eps.ep_cmd->request.req_type = UIO_REQUEST_WRITE;
        rt_usbd_io_request(func->device, ((rt_rndis_eth_t)func->user_data)->eps.ep_cmd, &((rt_rndis_eth_t)func->user_data)->eps.ep_cmd->request);
    }
    else
    {
        ((rt_rndis_eth_t)func->user_data)->need_notify = RT_TRUE;
    }

    return RT_EOK;
}

#ifdef  RNDIS_DELAY_LINK_UP
/**
 * This function will set rndis connect status.
 *
 * @param device the usb device object.
 * @param status the connect status.
 *
 * @return RT_EOK on successful.
 */
static rt_err_t _rndis_indicate_status_msg(ufunction_t func, rt_uint32_t status)
{
    rndis_indicate_status_msg_t resp;
    struct rt_rndis_response * response;

    response = rt_malloc(sizeof(struct rt_rndis_response));
    resp = rt_malloc(sizeof(struct rndis_indicate_status_msg));

    if( (response == RT_NULL) || (resp == RT_NULL) )
    {
        RNDIS_PRINTF("%d: no memory!\r\n", __LINE__);

        if(response != RT_NULL)
            rt_free(response);

        if(resp != RT_NULL)
            rt_free(resp);

        return -RT_ENOMEM;
    }

    resp->MessageType = REMOTE_NDIS_INDICATE_STATUS_MSG;
    resp->MessageLength = 20; /* sizeof(struct rndis_indicate_status_msg) */
    resp->Status = status;
    resp->StatusBufferLength = 0;
    resp->StatusBufferOffset = 0;

    response->buffer = resp;
    {
        rt_base_t level = rt_hw_interrupt_disable();
        rt_list_insert_before(&((rt_rndis_eth_t)func->user_data)->response_list, &response->list);
        rt_hw_interrupt_enable(level);
    }

    _rndis_response_available(func);

    return RT_EOK;
}
#endif /* RNDIS_DELAY_LINK_UP */

/**
 * This function will handle rndis interface request.
 *
 * @param device the usb device object.
 * @param setup the setup request.
 *
 * @return RT_EOK on successful.
 */
static rt_err_t _interface_handler(ufunction_t func, ureq_t setup)
{
    RT_ASSERT(func != RT_NULL);
    RT_ASSERT(setup != RT_NULL);

    switch(setup->bRequest)
    {
    case CDC_SEND_ENCAPSULATED_COMMAND:
        _rndis_send_encapsulated_command(func, setup);
        break;

    case CDC_GET_ENCAPSULATED_RESPONSE:
        _rndis_get_encapsulated_response(func, setup);
        break;

    default:
        RNDIS_PRINTF("unkown setup->request!\r\n");
        return -1;
    }

    return RT_EOK;
}

/**
 * This function will handle rndis bulk in endpoint request.
 *
 * @param device the usb device object.
 * @param size request size.
 *
 * @return RT_EOK.
 */

static rt_err_t _ep_in_handler(ufunction_t func, rt_size_t size)
{

    rt_sem_release(&((rt_rndis_eth_t)func->user_data)->tx_buffer_free);
    return RT_EOK;
}

/**
 * This function will handle RNDIS bulk out endpoint request.
 *
 * @param device the usb device object.
 * @param size request size.
 *
 * @return RT_EOK.
 */
static rt_err_t _ep_out_handler(ufunction_t func, rt_size_t size)
{
    cdc_eps_t eps;
    char* data = RT_NULL;

    eps = (cdc_eps_t)&((rt_rndis_eth_t)func->user_data)->eps;
    data = (char*)eps->ep_out->buffer;

    if(((rt_rndis_eth_t)func->user_data)->rx_frist == RT_TRUE)
    {
        rndis_packet_msg_t msg = (rndis_packet_msg_t)data;
        ((rt_rndis_eth_t)func->user_data)->rx_length = msg->DataLength;
        ((rt_rndis_eth_t)func->user_data)->rx_offset = 0;

        if (size >= 44)
        {
            data += sizeof(struct rndis_packet_msg);
            size -= sizeof(struct rndis_packet_msg);
            ((rt_rndis_eth_t)func->user_data)->rx_frist = RT_FALSE;
            rt_memcpy(&((rt_rndis_eth_t)func->user_data)->rx_buffer->payload[((rt_rndis_eth_t)func->user_data)->rx_offset], data, size);
            ((rt_rndis_eth_t)func->user_data)->rx_offset += size;
        }
    }
    else
    {
        rt_memcpy(&((rt_rndis_eth_t)func->user_data)->rx_buffer->payload[((rt_rndis_eth_t)func->user_data)->rx_offset], data, size);
        ((rt_rndis_eth_t)func->user_data)->rx_offset += size;
    }

    if(((rt_rndis_eth_t)func->user_data)->rx_offset >= ((rt_rndis_eth_t)func->user_data)->rx_length)
    {
        ((rt_rndis_eth_t)func->user_data)->rx_frist = RT_TRUE;
        ((rt_rndis_eth_t)func->user_data)->rx_flag = RT_TRUE;
        // eth_device_ready(&(((rt_rndis_eth_t)func->user_data)->parent));
        ((rt_rndis_eth_t)func->user_data)->rx_buffer->tot_len = ((rt_rndis_eth_t)func->user_data)->rx_buffer->len = ((rt_rndis_eth_t)func->user_data)->rx_offset;
        net_device_ready(&(((rt_rndis_eth_t)func->user_data)->parent));
    }
    else
    {
        eps->ep_out->request.buffer = eps->ep_out->buffer;
        eps->ep_out->request.size = EP_MAXPACKET(eps->ep_out);
        eps->ep_out->request.req_type = UIO_REQUEST_READ_BEST;
        rt_usbd_io_request(func->device, eps->ep_out, &eps->ep_out->request);
    }

    return RT_EOK;
}

/**
 * This function will handle RNDIS interrupt in endpoint request.
 *
 * @param device the usb device object.
 * @param size request size.
 *
 * @return RT_EOK.
 */
static rt_err_t _ep_cmd_handler(ufunction_t func, rt_size_t size)
{
//    _rndis_response_available(func);
    return RT_EOK;
}

/**
 * This function will run cdc class, it will be called on handle set configuration request.
 *
 * @param device the usb device object.
 *
 * @return RT_EOK on successful.
 */
static rt_err_t _function_enable(ufunction_t func)
{
    cdc_eps_t eps;

    eps = (cdc_eps_t)&((rt_rndis_eth_t)func->user_data)->eps;
    eps->ep_in->buffer  = ((rt_rndis_eth_t)func->user_data)->tx_pool;
    eps->ep_out->buffer = ((rt_rndis_eth_t)func->user_data)->rx_pool;
    eps->ep_cmd->buffer = (rt_uint8_t*)((rt_rndis_eth_t)func->user_data)->cmd_pool;

    eps->ep_out->request.buffer = eps->ep_out->buffer;
    eps->ep_out->request.size = EP_MAXPACKET(eps->ep_out);
    eps->ep_out->request.req_type = UIO_REQUEST_READ_BEST;
    rt_usbd_io_request(func->device, eps->ep_out, &eps->ep_out->request);

    ((rt_rndis_eth_t)func->user_data)->rx_flag = RT_FALSE;
    ((rt_rndis_eth_t)func->user_data)->rx_frist = RT_TRUE;

#ifdef  RNDIS_DELAY_LINK_UP
    /* stop link up timer. */
    rt_timer_stop(&((rt_rndis_eth_t)func->user_data)->timer);
#endif /* RNDIS_DELAY_LINK_UP */

    /* clean resp chain list. */
    {
        struct rt_rndis_response * response;
        rt_base_t level = rt_hw_interrupt_disable();

        while(!rt_list_isempty(&((rt_rndis_eth_t)func->user_data)->response_list))
        {
            response = (struct rt_rndis_response *)((rt_rndis_eth_t)func->user_data)->response_list.next;

            rt_list_remove(&response->list);
            rt_free((void *)response->buffer);
            rt_free(response);
        }

        ((rt_rndis_eth_t)func->user_data)->need_notify = RT_TRUE;
        rt_hw_interrupt_enable(level);
    }

    return RT_EOK;
}

/**
 * This function will stop cdc class, it will be called on handle set configuration request.
 *
 * @param device the usb device object.
 *
 * @return RT_EOK on successful.
 */
static rt_err_t _function_disable(ufunction_t func)
{
    RNDIS_PRINTF("plugged out\n");

#ifdef  RNDIS_DELAY_LINK_UP
    /* stop link up timer. */
    rt_timer_stop(&((rt_rndis_eth_t)func->user_data)->timer);
#endif /* RNDIS_DELAY_LINK_UP */

    /* clean resp chain list. */
    {
        struct rt_rndis_response * response;
        rt_base_t level = rt_hw_interrupt_disable();

        while(!rt_list_isempty(&((rt_rndis_eth_t)func->user_data)->response_list))
        {
            response = (struct rt_rndis_response *)((rt_rndis_eth_t)func->user_data)->response_list.next;
            RNDIS_PRINTF("remove resp chain list!\r\n");

            rt_list_remove(&response->list);
            rt_free((void *)response->buffer);
            rt_free(response);
        }

        ((rt_rndis_eth_t)func->user_data)->need_notify = RT_TRUE;
        rt_hw_interrupt_enable(level);
    }

    /* link down. */
    // eth_device_linkchange(&((rt_rndis_eth_t)func->user_data)->parent, RT_FALSE);
    net_device_linkchange(&((rt_rndis_eth_t)func->user_data)->parent, 0);
    return RT_EOK;
}

static rt_err_t
rndis_set_alt(struct ufunction *f, unsigned int interface, unsigned int alt)
{
    struct udevice* device = f->device;
    rt_rndis_eth_t data = f->user_data;

    dcd_ep_disable(device->dcd, data->eps.ep_cmd);
    dcd_ep_enable(device->dcd, data->eps.ep_cmd);

    dcd_ep_disable(device->dcd, data->eps.ep_in);
    dcd_ep_enable(device->dcd, data->eps.ep_in);

    dcd_ep_disable(device->dcd, data->eps.ep_out);
    dcd_ep_enable(device->dcd, data->eps.ep_out);

    return 0;
}

#ifdef RT_USING_LWIP
/* initialize the interface */
static rt_err_t rt_rndis_eth_init(rt_device_t dev)
{
    return RT_EOK;
}

static rt_err_t rt_rndis_eth_open(rt_device_t dev, rt_uint16_t oflag)
{
    return RT_EOK;
}

static rt_err_t rt_rndis_eth_close(rt_device_t dev)
{
    return RT_EOK;
}

static rt_size_t rt_rndis_eth_read(rt_device_t dev, rt_off_t pos, void* buffer, rt_size_t size)
{
    rt_set_errno(RT_ENOSYS);
    return 0;
}

static rt_size_t rt_rndis_eth_write (rt_device_t dev, rt_off_t pos, const void* buffer, rt_size_t size)
{
    rt_set_errno(RT_ENOSYS);
    return 0;
}
static rt_err_t rt_rndis_eth_control(rt_device_t dev, int cmd, void *args)
{
    rt_rndis_eth_t rndis_eth_dev = (rt_rndis_eth_t)dev;
    switch(cmd)
    {
    case NIOCTL_GADDR:
        /* get mac address */
        if(args) rt_memcpy(args, rndis_eth_dev->dev_addr, MAX_ADDR_LEN);
        else return -RT_ERROR;
        break;

    default :
        break;
    }

    return RT_EOK;
}

/* ethernet device interface */


/* reception packet. */
struct pbuf *rt_rndis_eth_rx(rt_device_t dev)
{
    struct pbuf* p = RT_NULL;
    rt_uint32_t offset = 0;
    rt_rndis_eth_t device = (rt_rndis_eth_t)dev;

    if(device->rx_flag == RT_FALSE)
    {
        return p;
    }
    p = device->rx_buffer;
    device->rx_buffer = pbuf_alloc(PBUF_RAW, 2048, PBUF_RAM);
    if (device->rx_buffer == RT_NULL)
    {
        rt_kprintf("[rndis error] pbuf_alloc = null\n");
        RT_ASSERT(device->rx_buffer);
    }

    {
        device->rx_flag = RT_FALSE;
        device->eps.ep_out->request.buffer = device->eps.ep_out->buffer;
        device->eps.ep_out->request.size = EP_MAXPACKET(device->eps.ep_out);
        device->eps.ep_out->request.req_type = UIO_REQUEST_READ_BEST;
        rt_usbd_io_request(device->func->device, device->eps.ep_out, &device->eps.ep_out->request);
    }
    return p;
}

/* transmit packet. */
rt_err_t rt_rndis_eth_tx(rt_device_t dev, struct pbuf* p)
{
    struct pbuf* q;
    char * buffer;
    rt_err_t result = RT_EOK;
    rt_rndis_eth_t device = (rt_rndis_eth_t)dev;

    if (!((device->parent.netif->flags & NETIF_FLAG_LINK_UP)))
    {
        RNDIS_PRINTF("linkdown, drop pkg\r\n");
        return RT_EOK;
    }

    RT_ASSERT(p->tot_len < sizeof(device->tx_buffer[0]));
    if(p->tot_len > sizeof(device->tx_buffer[0]))
    {
        RNDIS_PRINTF("RNDIS MTU is:%d, but the send packet size is %d\r\n",
                     sizeof(device->tx_buffer[0]), p->tot_len);
        p->tot_len = sizeof(device->tx_buffer[0]);
    }


    buffer = (char *)&device->tx_buffer[device->cur_id] + sizeof(struct rndis_packet_msg);
    for (q = p; q != NULL; q = q->next)
    {
        rt_memcpy(buffer, q->payload, q->len);
        buffer += q->len;
        // rt_kprintf("q->len %d\n", q->len);
    }

    /* wait for buffer free. */
    result = rt_sem_take(&device->tx_buffer_free, RT_WAITING_FOREVER);
    if(result != RT_EOK)
    {
        return result;
    }

    {
        rndis_packet_msg_t msg;

        msg = (rndis_packet_msg_t)&device->tx_buffer[device->cur_id];

        msg->MessageType = REMOTE_NDIS_PACKET_MSG;
        msg->DataOffset = sizeof(struct rndis_packet_msg) - 8;
        msg->DataLength = p->tot_len;
        msg->OOBDataLength = 0;
        msg->OOBDataOffset = 0;
        msg->NumOOBDataElements = 0;
        msg->PerPacketInfoOffset = 0;
        msg->PerPacketInfoLength = 0;
        msg->VcHandle = 0;
        msg->Reserved = 0;
        msg->MessageLength = sizeof(struct rndis_packet_msg) + p->tot_len;

        device->eps.ep_in->request.buffer = (void *)&device->tx_buffer[device->cur_id];
        device->eps.ep_in->request.size = msg->MessageLength;
        device->eps.ep_in->request.req_type = UIO_REQUEST_WRITE;
        rt_usbd_io_request(device->func->device, device->eps.ep_in, &device->eps.ep_in->request);
    }
    device->cur_id = 1 - device->cur_id;
    return result;
}

#ifdef RT_USING_DEVICE_OPS
const static struct rt_device_ops rndis_device_ops =
{
    rt_rndis_eth_init,
    rt_rndis_eth_open,
    rt_rndis_eth_close,
    rt_rndis_eth_read,
    rt_rndis_eth_write,
    rt_rndis_eth_control
};
#endif

#endif /* RT_USING_LWIP */

#ifdef  RNDIS_DELAY_LINK_UP
/* the delay linkup timer handler. */
static void timer_timeout(void* parameter)
{
    RNDIS_PRINTF("delay link up!\r\n");
    _rndis_indicate_status_msg(((rt_rndis_eth_t)parameter)->parent.parent.user_data,
                               RNDIS_STATUS_MEDIA_CONNECT);
    // eth_device_linkchange(&((rt_rndis_eth_t)parameter)->parent, RT_TRUE);
    net_device_linkchange(&((rt_rndis_eth_t)parameter)->parent, 0);
}
#endif /* RNDIS_DELAY_LINK_UP */

#define UVC_COPY_DESCRIPTORS(mem, dst, src) \
        do { \
            const struct usb_descriptor_header * const *__src; \
            for (__src = src; *__src; ++__src) { \
                rt_memcpy(mem, *__src, (*__src)->bLength); \
                *dst++ = mem; \
                mem += (*__src)->bLength; \
            } \
        } while (0)

static void rndis_copy_descriptors(ufunction_t f)
{
    const struct usb_descriptor_header * const *src;
    struct usb_descriptor_header **dst;
    struct usb_descriptor_header **hdr;
    unsigned int n_desc = 0;
    unsigned int bytes = 0;
    void *mem;
    int needBufLen;

    for (src = rndis_desc; *src; ++src)
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
    UVC_COPY_DESCRIPTORS(mem, dst, rndis_desc);
    *dst = NULL;
}

/**
 * This function will create a cdc rndis class instance.
 *
 * @param device the usb device object.
 *
 * @return RT_EOK on successful.
 */

int rndis_bind_config(struct uconfig * c)
{
    int ret = 0;
    ufunction_t func;
    rt_rndis_eth_t _rndis;
    udevice_t device = c->device;
    cdc_eps_t eps;

    /* parameter check */
    RT_ASSERT(device != RT_NULL);

    ret = usb_string_id(c, RNDIS_STR_ASSOCIATION);
    if (ret < 0)
        goto error;
    _comm_desc.iad_desc.iFunction = ret;
    _comm_desc.intf_desc.iInterface = ret;

    /* create a cdc class */
    func = rt_usbd_function_new(device);

    _rndis= rt_malloc(sizeof(struct rt_rndis_eth));
    rt_memset(_rndis, 0, sizeof(struct rt_rndis_eth));

    func->enable = _function_enable;
    func->disable = _function_disable;
    func->setup = _interface_handler;
    func->set_alt = rndis_set_alt;

    func->user_data = _rndis;

    _rndis->func = func;

    /* create a cdc class endpoints collection */
    eps = &_rndis->eps;
    ret = usb_interface_id(c, func);
    _comm_desc.iad_desc.bFirstInterface = ret;
    _comm_desc.intf_desc.bInterfaceNumber = ret;

    ret = usb_interface_id(c, func);
    _data_desc.intf_desc.bInterfaceNumber = ret;

    _comm_desc.call_mgmt_desc.data_interface = _data_desc.intf_desc.bInterfaceNumber;
    _comm_desc.union_desc.slave_interface0 = _data_desc.intf_desc.bInterfaceNumber;
    _comm_desc.union_desc.master_interface = _comm_desc.intf_desc.bInterfaceNumber;

    // rndis_func_comp_id_desc.bFirstInterfaceNumber = _comm_desc.intf_desc.bInterfaceNumber; /* NOT Sure */

    eps->ep_cmd = usb_ep_autoconfig(func, &_comm_desc.ep_desc, _ep_cmd_handler);
    eps->ep_in = usb_ep_autoconfig(func, &_data_desc.ep_in_desc, _ep_in_handler);
    eps->ep_out = usb_ep_autoconfig(func, &_data_desc.ep_out_desc, _ep_out_handler);

    rndis_copy_descriptors(func);

    rt_usbd_os_comp_id_desc_add_os_func_comp_id_desc(device->os_comp_id_desc, &rndis_func_comp_id_desc);

    rt_usbd_config_add_function(c, func);
#ifdef RT_USING_LWIP

    rt_list_init(&_rndis->response_list);
    _rndis->need_notify = RT_TRUE;

    rt_sem_init(&_rndis->tx_buffer_free, "ue_tx", 1, RT_IPC_FLAG_FIFO);

#ifdef  RNDIS_DELAY_LINK_UP
    rt_timer_init(&_rndis->timer,
                  "RNDIS",
                  timer_timeout,
                  _rndis,
                  RT_TICK_PER_SECOND * 2,
                  RT_TIMER_FLAG_ONE_SHOT);
#endif  /* RNDIS_DELAY_LINK_UP */

    /* OUI 00-00-00, only for test. */
    _rndis->dev_addr[0]                 = 0x34;
    _rndis->dev_addr[1]                 = 0x97;
    _rndis->dev_addr[2]                 = 0xF6;
    /* generate random MAC. */
    _rndis->dev_addr[3]                 = 0x94;//*(const rt_uint8_t *)(0x1fff7a10);
    _rndis->dev_addr[4]                 = 0xEA;//*(const rt_uint8_t *)(0x1fff7a14);
    _rndis->dev_addr[5]                 = 0x12;//(const rt_uint8_t *)(0x1fff7a18);
    /* OUI 00-00-00, only for test. */
    _rndis->host_addr[0]                = 0x34;
    _rndis->host_addr[1]                = 0x97;
    _rndis->host_addr[2]                = 0xF6;
    /* generate random MAC. */
    _rndis->host_addr[3]                = 0x94;//*(const rt_uint8_t *)(0x0FE081F0);
    _rndis->host_addr[4]                = 0xEA;//*(const rt_uint8_t *)(0x0FE081F1);
    _rndis->host_addr[5]                = 0x13;//*(const rt_uint8_t *)(0x0FE081F2);

    _rndis->parent.parent.type = RT_Device_Class_NetIf;
    _rndis->parent.parent.init = rt_rndis_eth_init;
    _rndis->parent.parent.open = rt_rndis_eth_open;
    _rndis->parent.parent.close = rt_rndis_eth_close;
    _rndis->parent.parent.read = rt_rndis_eth_read;
    _rndis->parent.parent.write = rt_rndis_eth_write;
    _rndis->parent.parent.control = rt_rndis_eth_control;
    /* data port... */
    _rndis->parent.net.eth.eth_tx = rt_rndis_eth_tx;
    _rndis->parent.net.eth.eth_rx = rt_rndis_eth_rx;
#if defined(RT_USING_ETHTOOL) && defined(RT_USING_DEVICE_OPS)
    _rndis->parent.net.eth.ethtool_ops = &rndis_device_ops;
#endif
    unsigned char flag = 0;
    flag = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP;
#ifdef RT_LWIP_IGMP
    flag |= NETIF_FLAG_IGMP;
#endif
    /* register eth device */
    // eth_device_init(&((rt_rndis_eth_t)cdc->user_data)->parent, "u0");
    _rndis->rx_buffer = pbuf_alloc(PBUF_RAW, 2048, PBUF_RAM);
    if (_rndis->rx_buffer == RT_NULL)
    {
        rt_kprintf("[rndis error] pbuf_alloc = null\n");
        RT_ASSERT(_rndis->rx_buffer);
    }
    net_device_init(&_rndis->parent, "u0", flag, NET_DEVICE_USBNET);
#endif /* RT_USING_LWIP */
    return 0;
error:
    rt_kprintf("error: hid bind config\n");
    return ret;
}

void fh_rndis_init(void)
{
    rt_usb_device_init();
}
