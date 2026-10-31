
#include <rthw.h>
#include <rtthread.h>
#include <rtservice.h>
#include <rtdevice.h>
#include "../core/usbdevice.h"
#include "ch9.h"
#include <string.h>
#include "audio.h"
#include "uac_init.h"

#define USB_GADGET_DELAYED_STATUS       0x7fff    /* Impossibly large value */

#define UAC_ERR(msg...)     /* rt_kprintf("UAC, " msg) */
#define UAC_DBG(msg...)     /* rt_kprintf("UAC, " msg) */
#define UAC_INFO(msg...)     /* rt_kprintf("UAC, " msg) */

#define UAC_STR_ASSOCIATION_IDX        0
#define UAC_STR_CONTROL_IDX            1
#define UAC_STR_STREAMING_IDX        2

struct rt_messagequeue uac_event_mq;
static rt_uint8_t uac_eventmq_pool[(sizeof(struct uac_ctrl_event) * 8 + sizeof(void *))*8];

static unsigned int gLastDirOut = 0;
static unsigned int gLastEvtLen = 0;
static struct udevice *g_uacdevice;
struct uac_audio *g_audio;

#define    UAC_STR_CONTROL "Audio Control"
#define    UAC_STR_STREAMING "Audio Streaming"

#ifdef ENABLE_SPEAKER_DESC
#define UAC_IF_COUNT                3
#define UAC_COLLECTION_NUM          2
#define AC_HEADER_TOTAL_LENGTH     0x46
#else
#define UAC_IF_COUNT                2
#define UAC_COLLECTION_NUM          1
#define AC_HEADER_TOTAL_LENGTH      0x26
#endif

static struct usb_interface_assoc_descriptor audio_iad_desc = {
    .bLength            =   0x08,
    .bDescriptorType    =   USB_DT_INTERFACE_ASSOCIATION,
    .bFirstInterface    =   2,
    .bInterfaceCount    =   UAC_IF_COUNT,
    .bFunctionClass        =   USB_CLASS_AUDIO,
    .bFunctionSubClass    =   USB_SUBCLASS_AUDIOSTREAMING,
    .bFunctionProtocol    =   0x00,
    .iFunction            =   0,
};

static struct usb_interface_descriptor ac_intf_desc  = {
    .bLength            =   USB_DT_INTERFACE_SIZE,
    .bDescriptorType    =   USB_DT_INTERFACE,
    .bInterfaceNumber    =   2,
    .bAlternateSetting    =   0,
    .bNumEndpoints        =   0,
    .bInterfaceClass    =   USB_CLASS_AUDIO,
    .bInterfaceSubClass    =   USB_SUBCLASS_AUDIOCONTROL,
    .bInterfaceProtocol    =   0x00,
    .iInterface            =   0,
};

DECLARE_UAC_AC_HEADER_DESCRIPTOR(2);

/* B.3.2  Class-Specific AC Interface Descriptor */
static struct uac1_ac_header_descriptor_2 ac_header_desc = {
    .bLength            =   UAC_DT_AC_HEADER_SIZE(UAC_COLLECTION_NUM),
    .bDescriptorType    =   USB_DT_CS_INTERFACE,
    .bDescriptorSubtype =   UAC_HEADER,
    .bcdADC             =   cpu_to_le16(0x0100),
    .wTotalLength       =   cpu_to_le16(AC_HEADER_TOTAL_LENGTH),
    .bInCollection      =   UAC_COLLECTION_NUM,
    .baInterfaceNr[0]   =   3,
    .baInterfaceNr[1]   =   4,

};



#define INPUT_TERMINAL_ID    1

static struct uac_input_terminal_descriptor audio_mic_it_desc = {
    .bLength            =    UAC_DT_INPUT_TERMINAL_SIZE,
    .bDescriptorType    =    USB_DT_CS_INTERFACE,
    .bDescriptorSubtype =    UAC_INPUT_TERMINAL,
    .bTerminalID        =    INPUT_TERMINAL_ID,
    .wTerminalType      =    UAC_INPUT_TERMINAL_MICROPHONE,
    .bAssocTerminal     =    0,
    .bNrChannels        =   AUDIO_CHN_NUM_MIC,
    .wChannelConfig     =    0x01 | AUDIO_CHN_NUM_MIC,
};


#define OUTPUT_TERMINAL_ID  3
#define FEATURE_UNIT_ID        5

static struct uac1_output_terminal_descriptor  audio_mic_ot_desc = {

    .bLength            =   UAC_DT_OUTPUT_TERMINAL_SIZE,
    .bDescriptorType    =   USB_DT_CS_INTERFACE,
    .bDescriptorSubtype =     UAC_OUTPUT_TERMINAL,
    .bTerminalID        =   OUTPUT_TERMINAL_ID,
    .wTerminalType      =   UAC_TERMINAL_STREAMING,
    .bAssocTerminal     =   0,
    .bSourceID          =   FEATURE_UNIT_ID,
    .iTerminal          =   0,
};

DECLARE_UAC_FEATURE_UNIT_DESCRIPTOR(0);
static struct uac_feature_unit_descriptor_0 audio_mic_fu_desc = {
    .bLength            =   0x09,
    .bDescriptorType    =   USB_DT_CS_INTERFACE,
    .bDescriptorSubtype    =   UAC_FEATURE_UNIT,
    .bUnitID            =   FEATURE_UNIT_ID,
    .bSourceID            =   INPUT_TERMINAL_ID,
    .bControlSize        =   2,
    .bmaControls[0]        =   (UAC_FU_MUTE | UAC_FU_VOLUME),
};



#define SPEAKER_IT_ID       4
#define SPEAKER_FU_ID       6
#define SPEAKER_OT_ID       8

#ifdef ENABLE_SPEAKER_DESC
static struct uac_input_terminal_descriptor audio_spk_it_desc = {
    .bLength            =    UAC_DT_INPUT_TERMINAL_SIZE,
    .bDescriptorType    =    USB_DT_CS_INTERFACE,
    .bDescriptorSubtype =    UAC_INPUT_TERMINAL,
    .bTerminalID        =    SPEAKER_IT_ID,
    .wTerminalType      =    UAC_TERMINAL_STREAMING,
    .bAssocTerminal     =    0,
    .bNrChannels        =   AUDIO_CHN_NUM_SPK,
    .wChannelConfig     =    0x01 | AUDIO_CHN_NUM_SPK,
};



static struct uac_feature_unit_descriptor_0 audio_spk_fu_desc = {
    .bLength            =   0x09,
    .bDescriptorType    =   USB_DT_CS_INTERFACE,
    .bDescriptorSubtype    =   UAC_FEATURE_UNIT,
    .bUnitID            =   SPEAKER_FU_ID,
    .bSourceID            =   SPEAKER_IT_ID,
    .bControlSize        =   2,
    .bmaControls[0]        =   (UAC_FU_MUTE | UAC_FU_VOLUME),
};


static struct uac1_output_terminal_descriptor  audio_spk_ot_desc = {

    .bLength            =   UAC_DT_OUTPUT_TERMINAL_SIZE,
    .bDescriptorType    =   USB_DT_CS_INTERFACE,
    .bDescriptorSubtype =     UAC_OUTPUT_TERMINAL,
    .bTerminalID        =   SPEAKER_OT_ID,
    .wTerminalType      =   UAC_OUTPUT_TERMINAL_SPEAKER,
    .bAssocTerminal     =   0,
    .bSourceID          =   SPEAKER_FU_ID,
    .iTerminal          =   0,
};

#endif


/* Standard AS Interface Descriptor */
static struct usb_interface_descriptor as_mic_if_alt_0_desc = {
    .bLength            =    USB_DT_INTERFACE_SIZE,
    .bDescriptorType    =    USB_DT_INTERFACE,
    .bInterfaceNumber   =   3,
    .bAlternateSetting  =    0,
    .bNumEndpoints      =    0,
    .bInterfaceClass    =    USB_CLASS_AUDIO,
    .bInterfaceSubClass =    USB_SUBCLASS_AUDIOSTREAMING,
};

static struct usb_interface_descriptor as_mic_if_alt_1_desc = {
    .bLength            =    USB_DT_INTERFACE_SIZE,
    .bDescriptorType    =    USB_DT_INTERFACE,
    .bInterfaceNumber   =   3,
    .bAlternateSetting  =    1,
    .bNumEndpoints      =    1,
    .bInterfaceClass    =    USB_CLASS_AUDIO,
    .bInterfaceSubClass =    USB_SUBCLASS_AUDIOSTREAMING,
};


/* B.4.2  Class-Specific AS Interface Descriptor */
static struct uac1_as_header_descriptor as_mic_header_desc = {
    .bLength            =    UAC_DT_AS_HEADER_SIZE,
    .bDescriptorType    =    USB_DT_CS_INTERFACE,
    .bDescriptorSubtype =    UAC_AS_GENERAL,
    .bTerminalLink      =    OUTPUT_TERMINAL_ID,
    .bDelay             =    0x01,
    .wFormatTag         =    UAC_FORMAT_TYPE_I_PCM,
};

#define SAMPLE_FREQ_8K     (8000)
#define SAMPLE_FREQ_16K     (16000)
#define SAMPLE_FREQ_22K     (22050)
#define SAMPLE_FREQ_32K     (32000)
#define SAMPLE_FREQ_44K     (44100)
#define SAMPLE_FREQ_48K     (48000)
DECLARE_UAC_FORMAT_TYPE_I_DISCRETE_DESC(6);

static struct uac_format_type_i_discrete_descriptor_6 as_mic_type_i_desc = {
    .bLength            =    UAC_FORMAT_TYPE_I_DISCRETE_DESC_SIZE(4),
    .bDescriptorType    =    USB_DT_CS_INTERFACE,
    .bDescriptorSubtype =    UAC_FORMAT_TYPE,
    .bFormatType        =    UAC_FORMAT_TYPE_I,
    .bNrChannels        =   AUDIO_CHN_NUM_MIC,
    .bSubframeSize      =    2,
    .bBitResolution     =    16,
    .bSamFreqType       =    4,
    .tSamFreq[0][0]     =   (SAMPLE_FREQ_8K & 0xff),
    .tSamFreq[0][1]     =   (SAMPLE_FREQ_8K >> 8) & 0xff,
    .tSamFreq[0][2]     =   0,
    .tSamFreq[1][0]     =   (SAMPLE_FREQ_16K & 0xff),
    .tSamFreq[1][1]     =   (SAMPLE_FREQ_16K >> 8) & 0xff,
    .tSamFreq[1][2]     =   0,
    .tSamFreq[2][0]     =   (SAMPLE_FREQ_44K & 0xff),
    .tSamFreq[2][1]     =   (SAMPLE_FREQ_44K >> 8) & 0xff,
    .tSamFreq[2][2]     =   0,
    .tSamFreq[3][0]     =   (SAMPLE_FREQ_48K & 0xff),
    .tSamFreq[3][1]     =   (SAMPLE_FREQ_48K >> 8) & 0xff,
    .tSamFreq[3][2]     =   0,
};


static struct usb_endpoint_descriptor audio_mic_streaming_ep = {
    .bLength            =   USB_DT_ENDPOINT_AUDIO_SIZE,
    .bDescriptorType    =   USB_DT_ENDPOINT,
    .bEndpointAddress    =   USB_DIR_IN,
    .bmAttributes        =   USB_ENDPOINT_XFER_ISOC | USB_ENDPOINT_SYNC_ASYNC,/* USB_ENDPOINT_XFER_ISOC, */
    .wMaxPacketSize        =   cpu_to_le16(256),
    .bInterval            =   4,
};


/* Class-specific AS ISO OUT Endpoint Descriptor */
static struct uac_iso_endpoint_descriptor as_mic_iso_out_desc = {
    .bLength            =    UAC_ISO_ENDPOINT_DESC_SIZE,
    .bDescriptorType    =    USB_DT_CS_ENDPOINT,
    .bDescriptorSubtype =    UAC_EP_GENERAL,
    .bmAttributes       =     1,
    .bLockDelayUnits    =    0,
    .wLockDelay         =    0,
};


#ifdef ENABLE_SPEAKER_DESC

/* Standard AS Interface Descriptor */
static struct usb_interface_descriptor as_spk_if_alt_0_desc = {
    .bLength            =    USB_DT_INTERFACE_SIZE,
    .bDescriptorType    =    USB_DT_INTERFACE,
    .bInterfaceNumber   =   4,
    .bAlternateSetting  =    0,
    .bNumEndpoints      =    0,
    .bInterfaceClass    =    USB_CLASS_AUDIO,
    .bInterfaceSubClass =    USB_SUBCLASS_AUDIOSTREAMING,
};

static struct usb_interface_descriptor as_spk_if_alt_1_desc = {
    .bLength            =    USB_DT_INTERFACE_SIZE,
    .bDescriptorType    =    USB_DT_INTERFACE,
    .bInterfaceNumber   =   4,
    .bAlternateSetting  =    1,
    .bNumEndpoints      =    1,
    .bInterfaceClass    =    USB_CLASS_AUDIO,
    .bInterfaceSubClass =    USB_SUBCLASS_AUDIOSTREAMING,
};


/* B.4.2  Class-Specific AS Interface Descriptor */
static struct uac1_as_header_descriptor as_spk_header_desc = {
    .bLength            =    UAC_DT_AS_HEADER_SIZE,
    .bDescriptorType    =    USB_DT_CS_INTERFACE,
    .bDescriptorSubtype =    UAC_AS_GENERAL,
    .bTerminalLink      =    SPEAKER_IT_ID,
    .bDelay             =    0x01,
    .wFormatTag         =    UAC_FORMAT_TYPE_I_PCM,
};


static struct uac_format_type_i_discrete_descriptor_6 as_spk_type_i_desc = {
    .bLength            =    UAC_FORMAT_TYPE_I_DISCRETE_DESC_SIZE(4),
    .bDescriptorType    =    USB_DT_CS_INTERFACE,
    .bDescriptorSubtype =    UAC_FORMAT_TYPE,
    .bFormatType        =    UAC_FORMAT_TYPE_I,
    .bNrChannels        =   AUDIO_CHN_NUM_SPK,
    .bSubframeSize      =    2,
    .bBitResolution     =    16,
    .bSamFreqType       =    4,
    .tSamFreq[0][0]     =   (SAMPLE_FREQ_8K & 0xff),
    .tSamFreq[0][1]     =   (SAMPLE_FREQ_8K >> 8) & 0xff,
    .tSamFreq[0][2]     =   0,
    .tSamFreq[1][0]     =   (SAMPLE_FREQ_16K & 0xff),
    .tSamFreq[1][1]     =   (SAMPLE_FREQ_16K >> 8) & 0xff,
    .tSamFreq[1][2]     =   0,
    .tSamFreq[2][0]     =   (SAMPLE_FREQ_44K & 0xff),
    .tSamFreq[2][1]     =   (SAMPLE_FREQ_44K >> 8) & 0xff,
    .tSamFreq[2][2]     =   0,
    .tSamFreq[3][0]     =   (SAMPLE_FREQ_48K & 0xff),
    .tSamFreq[3][1]     =   (SAMPLE_FREQ_48K >> 8) & 0xff,
    .tSamFreq[3][2]     =   0,
};


static struct usb_endpoint_descriptor audio_spk_streaming_ep = {
    .bLength            =   USB_DT_ENDPOINT_AUDIO_SIZE,
    .bDescriptorType    =   USB_DT_ENDPOINT,
    .bEndpointAddress    =   USB_DIR_OUT,
    .bmAttributes        =   USB_ENDPOINT_XFER_ISOC | USB_ENDPOINT_SYNC_ADAPTIVE,
    .wMaxPacketSize        =   cpu_to_le16(256),
    .bInterval            =   4,
};


/* Class-specific AS ISO OUT Endpoint Descriptor */
static struct uac_iso_endpoint_descriptor as_spk_iso_out_desc = {
    .bLength            =    UAC_ISO_ENDPOINT_DESC_SIZE,
    .bDescriptorType    =    USB_DT_CS_ENDPOINT,
    .bDescriptorSubtype =    UAC_EP_GENERAL,
    .bmAttributes       =     1,
    .bLockDelayUnits    =    1,
    .wLockDelay         =    1,
};

#endif



static struct usb_descriptor_header *f_audio_descs[]  = {
    (struct usb_descriptor_header *)&audio_iad_desc,
    (struct usb_descriptor_header *)&ac_intf_desc,
    (struct usb_descriptor_header *)&ac_header_desc,

    (struct usb_descriptor_header *)&audio_mic_it_desc,
    (struct usb_descriptor_header *)&audio_mic_ot_desc,
    (struct usb_descriptor_header *)&audio_mic_fu_desc,

#ifdef ENABLE_SPEAKER_DESC
    (struct usb_descriptor_header *)&audio_spk_it_desc,
    (struct usb_descriptor_header *)&audio_spk_ot_desc,
    (struct usb_descriptor_header *)&audio_spk_fu_desc,
#endif


    (struct usb_descriptor_header *)&as_mic_if_alt_0_desc,
    (struct usb_descriptor_header *)&as_mic_if_alt_1_desc,
    (struct usb_descriptor_header *)&as_mic_header_desc,
    (struct usb_descriptor_header *)&as_mic_type_i_desc,
    (struct usb_descriptor_header *)&audio_mic_streaming_ep,
    (struct usb_descriptor_header *)&as_mic_iso_out_desc,

#ifdef ENABLE_SPEAKER_DESC
    (struct usb_descriptor_header *)&as_spk_if_alt_0_desc,
    (struct usb_descriptor_header *)&as_spk_if_alt_1_desc,
    (struct usb_descriptor_header *)&as_spk_header_desc,
    (struct usb_descriptor_header *)&as_spk_type_i_desc,
    (struct usb_descriptor_header *)&audio_spk_streaming_ep,
    (struct usb_descriptor_header *)&as_spk_iso_out_desc,
#endif

    NULL,
};

static unsigned char cmd_data[16];

struct uac_request_data
{
    unsigned int length;
    __u8 data[68];
};

static void uac_send_response(struct uac_request_data *resp)
{
    unsigned int len  = resp->length;

    if (gLastDirOut == 0)
    {
        if (resp->length > 0)
        {
            if (gLastEvtLen < resp->length)
            {
                len = gLastEvtLen;
            }

            rt_usbd_ep0_write(g_uacdevice, resp->data, len);
        }
        else
        {
            dcd_ep0_send_status(g_uacdevice->dcd);
        }
    }
}

static rt_err_t uac_ep0_complete(udevice_t device, rt_size_t size)
{
    unsigned char *pData = NULL;
    unsigned int current_rate = 0;
    unsigned int mic_volume;
    static int last_sample_rate = 0;
    static int last_spk_sample_rate = 0;
    struct uac_ctrl_event event;
    int result;

    if (g_audio->set_cmd)
    {
        pData = cmd_data;
        if (g_audio->set_cmd == UAC_FU_MUTE && size == 1)
        {
            switch (g_audio->set_id)
            {
            case FEATURE_UNIT_ID:
                g_audio->mute_set = pData[0];
                event.func_num = UAC_CALLBACK_MIC_MUTE;
                event.parm1 = pData[0];
                result = rt_mq_send(&uac_event_mq, (void *)&event, sizeof(event));
                if (result == -RT_EFULL)
                {
                    rt_kprintf("uac callback message send failed\n");
                }
                break;

            case SPEAKER_FU_ID:
                g_audio->spkmute_set = pData[0];
                event.func_num = UAC_CALLBACK_SPK_MUTE;
                event.parm1 = pData[0];
                result = rt_mq_send(&uac_event_mq, (void *)&event, sizeof(event));
                if (result == -RT_EFULL)
                {
                    rt_kprintf("uac callback message send failed\n");
                }
                break;
            }
        } else if (g_audio->set_cmd == UAC_FU_VOLUME && size == 2)
        {
            switch (g_audio->set_id)
            {
            case FEATURE_UNIT_ID:
                mic_volume = pData[0] | pData[1] << 8;
                g_audio->vol_set = mic_volume * 100 / UAC_MIC_MAX_GAIN;
                if (g_audio->mute_set)
                    break;
                event.func_num = UAC_CALLBACK_MIC_VOLUME;
                event.parm1 = g_audio->vol_set;
                result = rt_mq_send(&uac_event_mq, (void *)&event, sizeof(event));
                if (result == -RT_EFULL)
                {
                    rt_kprintf("uac callback message send failed\n");
                }
                break;
            case SPEAKER_FU_ID:
                g_audio->spkvol_set = pData[0] | pData[1] << 8;
                if (g_audio->spkmute_set)
                    break;
                event.func_num = UAC_CALLBACK_SPK_VOLUME;
                event.parm1 = g_audio->spkvol_set;
                result = rt_mq_send(&uac_event_mq, (void *)&event, sizeof(event));
                if (result == -RT_EFULL)
                {
                    rt_kprintf("uac callback message send failed\n");
                }
                break;
            }
        } else if (audio_mic_streaming_ep.bEndpointAddress == g_audio->set_cmd && size == 3)
        {
            current_rate = (pData[0]) | (pData[1]<<8) | (pData[2]<<16);
            if (last_sample_rate != current_rate)
            {
                last_sample_rate = current_rate;
                g_audio->mic_sample = current_rate;
                fh_uac_micsample_change(current_rate);
                switch (current_rate)
                {
                case SAMPLE_FREQ_8K:
                    audio_mic_streaming_ep.wMaxPacketSize = 16 * AUDIO_CHN_NUM_MIC;
                    break;
                case SAMPLE_FREQ_16K:
                    audio_mic_streaming_ep.wMaxPacketSize = 32 * AUDIO_CHN_NUM_MIC;
                    break;
                case SAMPLE_FREQ_22K:
                    audio_mic_streaming_ep.wMaxPacketSize = 46 * AUDIO_CHN_NUM_MIC;
                    break;
                case SAMPLE_FREQ_32K:
                    audio_mic_streaming_ep.wMaxPacketSize = 64 * AUDIO_CHN_NUM_MIC;
                    break;
                case SAMPLE_FREQ_44K:
                    audio_mic_streaming_ep.wMaxPacketSize = 90 * AUDIO_CHN_NUM_MIC;
                    break;
                case SAMPLE_FREQ_48K:
                    audio_mic_streaming_ep.wMaxPacketSize = 96 * AUDIO_CHN_NUM_MIC;
                    break;
                default:
                    rt_kprintf("UAC: uac_ep0_complete()-micsample error %d\n", current_rate);
                }
                event.func_num = UAC_CALLBACK_MIC_SAMPLE;
                event.parm1 = current_rate;
                result = rt_mq_send(&uac_event_mq, (void *)&event, sizeof(event));
                if (result == -RT_EFULL)
                {
                    rt_kprintf("uac callback message send failed\n");
                }
                // dcd_ep_enable(device->dcd, g_audio->ep);
            }
        }
#ifdef ENABLE_SPEAKER_DESC
        else if (audio_spk_streaming_ep.bEndpointAddress == g_audio->set_cmd && size == 3)
        {
            current_rate = (pData[0]) | (pData[1]<<8) | (pData[2]<<16);
            if (last_spk_sample_rate != current_rate)
            {
                last_spk_sample_rate = current_rate;
                g_audio->spk_sample = current_rate;
                switch (current_rate)
                {
                case SAMPLE_FREQ_8K:
                    audio_spk_streaming_ep.wMaxPacketSize = 16 * AUDIO_CHN_NUM_SPK;
                    break;
                case SAMPLE_FREQ_16K:
                    audio_spk_streaming_ep.wMaxPacketSize = 32 * AUDIO_CHN_NUM_SPK;
                    break;
                case SAMPLE_FREQ_22K:
                    audio_spk_streaming_ep.wMaxPacketSize = 46 * AUDIO_CHN_NUM_SPK;
                    break;
                case SAMPLE_FREQ_32K:
                    audio_spk_streaming_ep.wMaxPacketSize = 64 * AUDIO_CHN_NUM_SPK;
                    break;
                case SAMPLE_FREQ_44K:
                    audio_spk_streaming_ep.wMaxPacketSize = 90 * AUDIO_CHN_NUM_SPK;
                    break;
                case SAMPLE_FREQ_48K:
                    audio_spk_streaming_ep.wMaxPacketSize = 96 * AUDIO_CHN_NUM_SPK;
                    break;
                default:
                    rt_kprintf("UAC: uac_ep0_complete()-spksample error %d\n", current_rate);
                }
                event.func_num = UAC_CALLBACK_SPK_SAMPLE;
                event.parm1 = current_rate;
                result = rt_mq_send(&uac_event_mq, (void *)&event, sizeof(event));
                if (result == -RT_EFULL)
                {
                    rt_kprintf("uac callback message send failed\n");
                }
            }
        }
#endif
    }
    g_audio->set_cmd = 0;
    dcd_ep0_send_status(device->dcd);
    return RT_EOK;
}

static rt_err_t
uac_function_setup(ufunction_t f, ureq_t ctrl)
{
    struct uac_request_data resp;

    resp.length = 0;

    int value = -EOPNOTSUPP;
    int cs, id;

    g_audio->set_cmd = 0;
    g_audio->set_id = 0;

    gLastDirOut = !(ctrl->bRequestType & USB_DIR_IN);
    gLastEvtLen = ctrl->wLength;

    if ((ctrl->bRequestType & USB_RECIP_MASK) != USB_RECIP_INTERFACE)
    {
        cs = ctrl->wValue >> 8;
        id = ctrl->wIndex & 0xff;
        if (id == audio_mic_streaming_ep.bEndpointAddress) {
            switch (cs) {
            case SAMPLING_FREQ_CONTROL:
                value = 3;
                switch (ctrl->bRequest) {
                case UAC_SET_CUR:
                    g_audio->set_cmd = ctrl->wIndex;
                    break;
                case UAC_GET_CUR:
                    resp.data[0] = g_audio->mic_sample & 0xff;
                    resp.data[1] = g_audio->mic_sample >> 8 & 0xff;
                    resp.data[2] = g_audio->mic_sample >> 16 & 0xff;
                    break;
                default:
                    rt_kprintf("UAC:%s ctrl->bRequest TODO...\n", __func__);
                }
                break;
            default:
                rt_kprintf("UAC:%s TODO...\n", __func__);
            }
        }
#ifdef ENABLE_SPEAKER_DESC
        else if (id == audio_spk_streaming_ep.bEndpointAddress)
        {
            switch (cs) {
            case SAMPLING_FREQ_CONTROL:
                value = 3;
                switch (ctrl->bRequest) {
                case UAC_SET_CUR:
                    g_audio->set_cmd = ctrl->wIndex;
                    break;
                case UAC_GET_CUR:
                    resp.data[0] = g_audio->spk_sample & 0xff;
                    resp.data[1] = g_audio->spk_sample >> 8 & 0xff;
                    resp.data[2] = g_audio->spk_sample >> 16 & 0xff;
                    break;
                default:
                    rt_kprintf("UAC:%s ctrl->bRequest TODO...\n", __func__);
                }
                break;
            default:
                rt_kprintf("UAC:%s TODO...\n", __func__);
            }
        }
#endif
        else
            rt_kprintf("UAC:%s speaker ep control TODO...\n", __func__);
    }
    else
    {
        cs = ctrl->wValue >> 8;
        id = ctrl->wIndex >> 8;
        UAC_INFO("id = %d, cs = %d\n", id, cs);
        /* pData = req->buf; */
        if (FEATURE_UNIT_ID == id || SPEAKER_FU_ID == id)
        {
            switch (cs)
            {
            case UAC_FU_MUTE:
                switch (ctrl->bRequest)
                {
                case UAC_SET_CUR:
                    g_audio->set_cmd = cs;
                    g_audio->set_id = id;
                    break;

                case UAC_GET_CUR:
                    if (FEATURE_UNIT_ID == id)
                    {
                        value = 1;
                        resp.data[0] = g_audio->mute_set;
                    }
                    if (SPEAKER_FU_ID == id)
                    {
                        value = 1;
                        resp.data[0] = g_audio->spkmute_set;
                    }
                    break;
                }
                break;
            case UAC_FU_VOLUME:
                value = 2;
                switch (ctrl->bRequest)
                {
                case UAC_SET_CUR:
                    g_audio->set_cmd = cs;
                    g_audio->set_id = id;
                    break;

                case UAC_GET_CUR:
                    if (FEATURE_UNIT_ID == id)
                    {
                        resp.data[0] = g_audio->vol_set & 0xff;
                        resp.data[1] = (g_audio->vol_set >> 8) & 0xff;
                    }
                    if (SPEAKER_FU_ID == id)
                    {
                        resp.data[0] = g_audio->spkvol_set & 0xff;
                        resp.data[1] = (g_audio->spkvol_set >> 8) & 0xff;
                    }
                    break;

                case UAC_GET_MIN:
                    if (FEATURE_UNIT_ID == id)
                    {
                        resp.data[0] = 0x00;
                        resp.data[1] = 0x00;
                    }
                    if (SPEAKER_FU_ID == id)
                    {
                        resp.data[0] = 0x00;
                        resp.data[1] = 0x00;
                    }
                    break;

                case UAC_GET_MAX:
                    if (FEATURE_UNIT_ID == id)
                    {
                        resp.data[0] = UAC_MIC_MAX_GAIN & 0xff;
                        resp.data[1] = (UAC_MIC_MAX_GAIN >> 8) & 0xff;
                    }
                    if (SPEAKER_FU_ID == id)
                    {
                        resp.data[0] = 0x64;
                        resp.data[1] = 0x00;
                    }
                    break;

                case UAC_GET_RES:
                    resp.data[0] = 0x01;
                    resp.data[1] = 0x00;
                    break;
                default:
                    rt_kprintf("UAC：%s-%d TODO ctrl->bRequest....\n", __func__, __LINE__);
                    break;
                }
                break;
            default:
                rt_kprintf("UAC：%s-%d TODO Feature Unit Control Selectors....\n", __func__, __LINE__);
            }
        } else
            rt_kprintf("UAC：%s-%d TODO Unit Control....\n", __func__, __LINE__);
    }

    if (value >= 0 && value != USB_GADGET_DELAYED_STATUS)
    {
        resp.length = value;
        uac_send_response(&resp);
    }

    if (gLastDirOut)
    {
        if (ctrl->wLength > 0)
        {
            rt_memset(cmd_data, 0, 14);
            rt_usbd_ep0_read(f->device, cmd_data,  ctrl->wLength, uac_ep0_complete);
            return 0;
        }
    }

    return 0;
}

static rt_err_t
uac_function_get_alt(ufunction_t f, unsigned int interface)
{
    if (interface == g_audio->as_mic_intf)
        return g_audio->mic_alt;
    else if (interface == g_audio->as_spk_intf)
        return g_audio->spk_alt;
    else
        return -EINVAL;
}

static rt_err_t
uac_function_set_alt(ufunction_t f, unsigned int interface, unsigned int alt)
{

    UAC_INFO("set_alt, intf = %d, alt = %d\n", interface, alt);
    if (g_audio->as_mic_intf == interface)
    {
        g_audio->mic_alt = alt;
        if (alt)
        {
            fh_uac_micstatus_set(UAC_STREAM_ON);
            dcd_ep_enable(f->device->dcd, g_audio->ep);
        }
        else
        {
            fh_uac_micstatus_set(UAC_STREAM_OFF);
            dcd_ep_disable(f->device->dcd, g_audio->ep);
            fh_uac_micbuf_reinit();
        }
    }
#ifdef ENABLE_SPEAKER_DESC
    else if (g_audio->as_spk_intf == interface)
    {
        g_audio->spk_alt = alt;
        if (alt)
        {
            fh_uac_spkstatus_set(UAC_STREAM_ON);
            dcd_ep_enable(f->device->dcd, g_audio->out_ep);
            uac_speaker_pump(f->device, g_audio->out_ep);
        }
        else
        {
            fh_uac_spkstatus_set(UAC_STREAM_OFF);
            dcd_ep_disable(f->device->dcd, g_audio->out_ep);
            fh_uac_spkbuf_reinit();
        }
    }
#endif

    return 0;
}

#define UVC_COPY_DESCRIPTOR(mem, dst, desc) \
    do \
    {\
        memcpy(mem, desc, (desc)->bLength); \
        *(dst)++ = mem; \
        mem += (desc)->bLength; \
    } while (0)

#define UVC_COPY_DESCRIPTORS(mem, dst, src) \
    do \
    {\
        const struct usb_descriptor_header * const *__src; \
        for (__src = src; *__src; ++__src) \
        {\
            memcpy(mem, *__src, (*__src)->bLength); \
            *dst++ = mem; \
            mem += (*__src)->bLength; \
        } \
    } while (0)

static struct usb_descriptor_header **
uac_copy_descriptors(struct ufunction *f)
{
    const struct usb_descriptor_header * const *src;
    struct usb_descriptor_header **dst;
    struct usb_descriptor_header **hdr;
    unsigned int n_desc = 0;
    unsigned int bytes  = 0;
    void *mem;

    int audio_desc_num = 0;

    if (f->desc_hdr_mem)
    {
        rt_free(f->desc_hdr_mem);
        f->desc_hdr_mem = RT_NULL;
    }

    for (src = (const struct usb_descriptor_header **)f_audio_descs; *src; ++src)
    {
        bytes += (*src)->bLength;
        n_desc++;
    }

    audio_desc_num = (n_desc + 1) * sizeof(*src) + bytes;
    mem = rt_malloc(audio_desc_num+20);

    hdr = mem;
    dst = mem;
    mem += (n_desc + 1) * sizeof(*src);

    f->desc_hdr_mem = hdr;
    f->descriptors = mem;
    f->desc_size = bytes;
    f->other_descriptors = mem;
    f->other_desc_size = bytes;

    UVC_COPY_DESCRIPTORS(mem, dst, (const struct usb_descriptor_header * const *)f_audio_descs);
    *dst = (struct usb_descriptor_header *)NULL;
    return hdr;

}

rt_err_t uac_ep_in_handler(ufunction_t func, rt_size_t size)
{
    RT_ASSERT(func != RT_NULL);
    RT_ASSERT(func->device != RT_NULL);

    struct uendpoint *ep;

    ep = g_audio->ep;
    uac_audio_complete(func, ep);

    return RT_EOK;
}

#ifdef ENABLE_SPEAKER_DESC
rt_err_t uac_ep_out_handler(ufunction_t func, rt_size_t size)
{
    RT_ASSERT(func != RT_NULL);
    RT_ASSERT(func->device != RT_NULL);
    struct uendpoint *ep;

    ep = g_audio->out_ep;
    uac_speaker_complete(func, ep, size);

    return RT_EOK;
}
#endif

static rt_err_t
uac_function_bind(struct uconfig *c, struct ufunction *f)
{
    uep_t ep;
    int ret = -EINVAL;

    /* Allocate endpoints. */
    ep = usb_ep_autoconfig(f, (uep_desc_t)&audio_mic_streaming_ep, uac_ep_in_handler);
    if (!ep)
    {
        UAC_INFO("Unable to allocate control EP\n");
        goto error;
    }
    UAC_DBG("bind ep=%x\n", ep->ep_desc->bEndpointAddress);
    UAC_DBG("bind ep=%x\n", ep->ep_desc->bEndpointAddress);

    g_audio->ep = ep;

    /* Allocate interface IDs. */
    ret = usb_interface_id(c, f);
    if (ret < 0)
        goto error;

    UAC_INFO("control_intf = %d\n", ret);
    audio_iad_desc.bFirstInterface = ret;
    ac_intf_desc.bInterfaceNumber  = ret;
    g_audio->ac_intf = ret;

    ret = usb_interface_id(c, f);
    if (ret < 0)
        goto error;

    UAC_INFO("streaming_intf = %d\n", ret);
    as_mic_if_alt_0_desc.bInterfaceNumber = ret;
    as_mic_if_alt_1_desc.bInterfaceNumber = ret;
    g_audio->as_mic_intf = ret;
    ac_header_desc.baInterfaceNr[0] = ret;

#ifdef ENABLE_SPEAKER_DESC
    ep = usb_ep_autoconfig(f, (uep_desc_t)&audio_spk_streaming_ep, uac_ep_out_handler);
    UAC_DBG("audio_in_ep_id  = %s\n", ep->name);
    if (!ep)
    {
        UAC_ERR("Unable to allocate audio EP\n");
        goto error;
    }
    g_audio->out_ep = ep;
    ret = usb_interface_id(c, f);
    if (ret < 0)
        goto error;

    UAC_INFO("streaming_intf2 = %d\n", ret);
    as_spk_if_alt_0_desc.bInterfaceNumber = ret;
    as_spk_if_alt_1_desc.bInterfaceNumber = ret;
    g_audio->as_spk_intf = ret;
    ac_header_desc.baInterfaceNr[1] = ret;
    /* fh_uac_spkbuf_init(); */
 #endif
    g_audio->vol_set = UAC_MIC_DEFAULT_VOLUME * UAC_MIC_MAX_GAIN / 100;
    g_audio->spkvol_set = UAC_SPK_DEFAULT_VOLUME;
    /* Copy descriptors. */
    uac_copy_descriptors(f);

    g_uacdevice = c->device;

    return 0;

error:
    rt_kprintf("error: uac function bind\n");
    return ret;
}

char *uac_interface_string;
int  uac_bind_config(struct uconfig *c)
{
    int ret = 0;
    ufunction_t  func;

    /* Allocate string descriptor numbers. */
    ret = usb_string_id(c, (const char *)uac_interface_string);
    if (ret < 0)
        goto error;
    audio_iad_desc.iFunction = ret;
    ac_intf_desc.iInterface = ret;

    ret = usb_string_id(c, UAC_STR_STREAMING);
    if (ret < 0)
        goto error;
    as_mic_if_alt_0_desc.iInterface = ret;
    as_mic_if_alt_1_desc.iInterface = ret;

#ifdef ENABLE_SPEAKER_DESC
    as_spk_if_alt_0_desc.iInterface = ret;
    as_spk_if_alt_1_desc.iInterface = ret;
#endif

    g_audio = rt_calloc(sizeof(*g_audio), 1);

    func = rt_usbd_function_new(c->device);
    /* Register the function. */
    func->get_alt = uac_function_get_alt;
    func->set_alt = uac_function_set_alt;
    func->setup = uac_function_setup;

    g_audio->func = func;

    uac_function_bind(c, func);

    rt_usbd_config_add_function(c, func);

    rt_mq_init(&uac_event_mq, "cdc_check_mq", uac_eventmq_pool, sizeof(struct uac_ctrl_event),
                    sizeof(uac_eventmq_pool), RT_IPC_FLAG_FIFO);
    return 0;

error:
    rt_kprintf("error: uac bind config\n");
    return ret;
}

