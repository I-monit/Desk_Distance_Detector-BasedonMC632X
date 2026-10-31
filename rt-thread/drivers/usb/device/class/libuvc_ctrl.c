
#include "types/type_def.h"
#include <string.h>
#include "libuvc.h"
#include "uvc_init.h"

extern struct rt_messagequeue g_uvc_event_mq;
extern struct uvc_stream_ops *g_uvc_ops;
extern struct uvc_stream_info *stream_info[2];

int uvc_flash_ctrl(uint8_t cs, struct uvc_request_data *data);
void uvc_cdc_coolview_status(struct uvc_request_data *data);

static void
uvc_h264_extension_data(struct uvc_stream_info *dev, uint8_t cs, struct uvc_request_data *data);

#define DBG  rt_kprintf

uint32_t uvc_camera_terminal_supported(uint8_t cs)
{
    uint32_t controls = CAMERA_TERMINAL_CONTROLS;
    uint32_t ret = 0;

    switch(cs)
    {
    case UVC_CT_SCANNING_MODE_CONTROL:
        ret = controls & BIT(0);
        break;
    case UVC_CT_AE_MODE_CONTROL:
        ret = controls & BIT(1);
        break;
    case UVC_CT_AE_PRIORITY_CONTROL:
        ret = controls & BIT(2);
        break;
    case UVC_CT_EXPOSURE_TIME_ABSOLUTE_CONTROL:
        ret = controls & BIT(3);
        break;
    case UVC_CT_EXPOSURE_TIME_RELATIVE_CONTROL:
        ret = controls & BIT(4);
        break;
    case UVC_CT_FOCUS_ABSOLUTE_CONTROL:
        ret = controls & BIT(5);
        break;
    case UVC_CT_FOCUS_RELATIVE_CONTROL:
        ret = controls & BIT(6);
        break;
    case UVC_CT_FOCUS_AUTO_CONTROL:
        ret = controls & BIT(17);
        break;
    case UVC_CT_IRIS_ABSOLUTE_CONTROL:
        ret = controls & BIT(7);
        break;
    case UVC_CT_IRIS_RELATIVE_CONTROL:
        ret = controls & BIT(8);
        break;
    case UVC_CT_ZOOM_ABSOLUTE_CONTROL:
        ret = controls & BIT(9);
        break;
    case UVC_CT_ZOOM_RELATIVE_CONTROL:
        ret = controls & BIT(10);
        break;
    case UVC_CT_PANTILT_ABSOLUTE_CONTROL:
        ret = controls & BIT(11);
        break;
    case UVC_CT_PANTILT_RELATIVE_CONTROL:
        ret = controls & BIT(12);
        break;
    case UVC_CT_ROLL_ABSOLUTE_CONTROL:
        ret = controls & BIT(13);
        break;
    case UVC_CT_ROLL_RELATIVE_CONTROL:
        ret = controls & BIT(14);
        break;
    case UVC_CT_PRIVACY_CONTROL:
        ret = controls & BIT(18);
        break;
    default:
        ret = 0;
        break;
    }
    return ret;
}

uint32_t uvc_processing_unit_supported(uint8_t cs)
{
    uint32_t controls = PROCESSING_UNIT_CONTROLS;
    uint32_t ret = 0;

    switch(cs)
    {
    case UVC_PU_BACKLIGHT_COMPENSATION_CONTROL:
        ret = controls & BIT(8);
        break;
    case UVC_PU_BRIGHTNESS_CONTROL:
        ret = controls & BIT(0);
        break;
    case UVC_PU_CONTRAST_CONTROL:
        ret = controls & BIT(1);
        break;
    case UVC_PU_GAIN_CONTROL:
        ret = controls & BIT(9);
        break;
    case UVC_PU_POWER_LINE_FREQUENCY_CONTROL:
        ret = controls & BIT(10);
        break;
    case UVC_PU_HUE_CONTROL:
        ret = controls & BIT(2);
        break;
    case UVC_PU_SATURATION_CONTROL:
        ret = controls & BIT(3);
        break;
    case UVC_PU_SHARPNESS_CONTROL:
        ret = controls & BIT(4);
        break;
    case UVC_PU_GAMMA_CONTROL:
        ret = controls & BIT(5);
        break;
    case UVC_PU_WHITE_BALANCE_TEMPERATURE_CONTROL:
        ret = controls & BIT(6);
        break;
    case UVC_PU_WHITE_BALANCE_TEMPERATURE_AUTO_CONTROL:
        ret = controls & BIT(12);
        break;
    case UVC_PU_WHITE_BALANCE_COMPONENT_CONTROL:
        ret = controls & BIT(7);
        break;
    case UVC_PU_WHITE_BALANCE_COMPONENT_AUTO_CONTROL:
        ret = controls & BIT(13);
        break;
    case UVC_PU_DIGITAL_MULTIPLIER_CONTROL:
        ret = controls & BIT(14);
        break;
    case UVC_PU_DIGITAL_MULTIPLIER_LIMIT_CONTROL:
        ret = controls & BIT(15);
        break;
    case UVC_PU_HUE_AUTO_CONTROL:
        ret = controls & BIT(11);
        break;
    case UVC_PU_ANALOG_VIDEO_STANDARD_CONTROL:
        ret = controls & BIT(16);
        break;
    case UVC_PU_ANALOG_LOCK_STATUS_CONTROL:
        ret = controls & BIT(17);
        break;
    default:
        ret = 0;
        break;
    }
    return ret;
}

uint32_t uvc_msxu_unit_supported(uint8_t cs)
{
    uint32_t controls = MSXU_EXTENSION_UNIT_CONTROLS;

    return controls & BIT(cs - 1);
}

static void ct_scanning_mode_setup(struct uvc_request_data *r)
{
    UVC_DBG("# %s\n", __func__);
}

static void ct_scanning_mode_data(struct uvc_request_data *r)
{
    UVC_DBG("# %s\n", __func__);
}

extern struct uvc_control_ops uvc_control_ct[];
extern struct uvc_control_ops uvc_control_pu[];
extern struct uvc_control_ops uvc_msxu_face[];

static void ct_ae_mode_setup(struct uvc_request_data *r)
{
    r->data[0] = uvc_control_ct[UVC_CT_AE_MODE_CONTROL].val.cur;
}

static void ct_ae_mode_data(struct uvc_request_data *r)
{
    struct uvc_ctrl_event uvc_event;
    int result;
    unsigned char val = r->data[0];

    if (val == uvc_control_ct[UVC_CT_AE_MODE_CONTROL].val.cur)
        return;

    uvc_control_ct[UVC_CT_AE_MODE_CONTROL].val.cur = val;

    uvc_event.func_num = UVC_PARAM_FUNC_SETAEMODE;
    uvc_event.parm1 = val;

    result = rt_mq_send(&g_uvc_event_mq, (void *)&uvc_event, sizeof(uvc_event));
    if (result  != RT_EOK)
    {
        rt_kprintf("uvc message send failed\n");
    }
}

static void ct_ae_priority_setup(struct uvc_request_data *r)
{
    r->data[0] = uvc_control_ct[UVC_CT_AE_PRIORITY_CONTROL].val.cur;
}

static void ct_ae_priority_data(struct uvc_request_data *r)
{
    struct uvc_ctrl_event uvc_event;
    int result;

    uvc_control_ct[UVC_CT_AE_PRIORITY_CONTROL].val.cur = r->data[0];

    uvc_event.func_num = UVC_PARAM_FUNC_AE_PRIORITY;
    uvc_event.parm1 = r->data[0];

    result = rt_mq_send(&g_uvc_event_mq, (void *)&uvc_event, sizeof(uvc_event));
    if (result  != RT_EOK)
    {
        rt_kprintf("uvc message send failed\n");
    }
}

static void ct_exposure_absolute_setup(struct uvc_request_data *r)
{
    UVC_DBG("# %s\n", __func__);
    uint32_t exposure_time = 0;

    exposure_time = uvc_control_ct[UVC_CT_EXPOSURE_TIME_ABSOLUTE_CONTROL].val.cur;
    r->data[0] = exposure_time & 0xff;
    r->data[1] = (exposure_time>>8) & 0xff;
    r->data[2] = (exposure_time>>16) & 0xff;
    r->data[3] = (exposure_time>>24) & 0xff;
}

static void ct_exposure_absolute_data(struct uvc_request_data *r)
{
    UVC_DBG("# %s\n", __func__);
    struct uvc_ctrl_event uvc_event;
    int result;
    uint32_t exposure_time = 0;

    exposure_time = (r->data[0])|(r->data[1]<<8)|(r->data[2]<<16)|(r->data[3]<<24);
    uvc_control_ct[UVC_CT_EXPOSURE_TIME_ABSOLUTE_CONTROL].val.cur = exposure_time;

    uvc_event.func_num = UVC_PARAM_FUNC_SETEXPOSURE;
    uvc_event.parm1 = exposure_time;

    result = rt_mq_send(&g_uvc_event_mq, (void *)&uvc_event, sizeof(uvc_event));
    if (result != RT_EOK)
    {
        rt_kprintf("uvc message send failed\n");
    }
}

static void ct_exposure_relative_setup(struct uvc_request_data *r)
{
    UVC_DBG("# %s\n", __func__);
}

static void ct_exposure_relative_data(struct uvc_request_data *r)
{
    UVC_DBG("# %s\n", __func__);
}

static void ct_focus_absolute_setup(struct uvc_request_data *r)
{
    int val;
    val = uvc_control_ct[UVC_CT_FOCUS_ABSOLUTE_CONTROL].val.cur;
    r->data[0] = val & 0xff;
    r->data[1] = (val >> 8) & 0xff;
}

static void ct_focus_absolute_data(struct uvc_request_data *r)
{
    struct uvc_ctrl_event uvc_event;
    int result;
    unsigned int val = r->data[0] | (r->data[1] << 8);

    uvc_control_ct[UVC_CT_FOCUS_ABSOLUTE_CONTROL].val.cur = val;

    uvc_event.func_num = UVC_PARAM_FUNC_FOCUSABS;
    uvc_event.parm1 = val;

    result = rt_mq_send(&g_uvc_event_mq, (void *)&uvc_event, sizeof(uvc_event));
    if (result  != RT_EOK)
    {
        rt_kprintf("uvc message send failed\n");
    }
}

static void ct_focus_relative_setup(struct uvc_request_data *r)
{
    UVC_DBG("# %s\n", __func__);
}

static void ct_focus_relative_data(struct uvc_request_data *r)
{
    UVC_DBG("# %s\n", __func__);
}

static void ct_focus_auto_setup(struct uvc_request_data *r)
{
    r->data[0] = uvc_control_ct[UVC_CT_FOCUS_AUTO_CONTROL].val.cur;
}

static void ct_focus_auto_data(struct uvc_request_data *r)
{
    struct uvc_ctrl_event uvc_event;
    int result;
    unsigned char val = r->data[0];

    uvc_control_ct[UVC_CT_FOCUS_AUTO_CONTROL].val.cur = val;

    uvc_event.func_num = UVC_PARAM_FUNC_FOCUSMODE;
    uvc_event.parm1 = val;

    result = rt_mq_send(&g_uvc_event_mq, (void *)&uvc_event, sizeof(uvc_event));
    if (result  != RT_EOK)
    {
        rt_kprintf("uvc message send failed\n");
    }
}

static void ct_iris_absolute_setup(struct uvc_request_data *r)
{
    r->data[0] = uvc_control_ct[UVC_CT_IRIS_ABSOLUTE_CONTROL].val.cur & 0xff;
    r->data[1] = (uvc_control_ct[UVC_CT_IRIS_ABSOLUTE_CONTROL].val.cur >> 8) & 0xff;
}

static void ct_iris_absolute_data(struct uvc_request_data *r)
{
    struct uvc_ctrl_event uvc_event;
    int result;
    unsigned char val = r->data[0] | r->data[1] << 8;

    uvc_control_ct[UVC_CT_IRIS_ABSOLUTE_CONTROL].val.cur = val;

    uvc_event.func_num = UVC_PARAM_FUNC_SETIRIS;
    uvc_event.parm1 = val;

    result = rt_mq_send(&g_uvc_event_mq, (void *)&uvc_event, sizeof(uvc_event));
    if (result  != RT_EOK)
    {
        rt_kprintf("uvc message send failed\n");
    }
}

static void ct_iris_relative_setup(struct uvc_request_data *r)
{
    UVC_DBG("# %s\n", __func__);
}

static void ct_iris_relative_data(struct uvc_request_data *r)
{
    UVC_DBG("# %s\n", __func__);
}

static void ct_zoom_absolute_setup(struct uvc_request_data *r)
{
    uint32_t zoom = 0;

    zoom = uvc_control_ct[UVC_CT_ZOOM_ABSOLUTE_CONTROL].val.cur;
    r->data[0] = zoom & 0xff;
    r->data[1] = (zoom>>8) & 0xff;
    r->data[2] = (zoom>>16) & 0xff;
    r->data[3] = (zoom>>24) & 0xff;
}

static void ct_zoom_absolute_data(struct uvc_request_data *r)
{
    struct uvc_ctrl_event uvc_event;
    int result;
    unsigned int val = r->data[0] | (r->data[1]<<8);

    uvc_control_ct[UVC_CT_ZOOM_ABSOLUTE_CONTROL].val.cur = val;

    uvc_event.func_num = UVC_PARAM_FUNC_ZOOMABS;
    uvc_event.parm1 = val;

    result = rt_mq_send(&g_uvc_event_mq, (void *)&uvc_event, sizeof(uvc_event));
    if (result  != RT_EOK)
    {
        rt_kprintf("uvc message send failed\n");
    }
}

static void ct_zoom_relative_setup(struct uvc_request_data *r)
{
    UVC_DBG("# %s\n", __func__);
}

static void ct_zoom_relative_data(struct uvc_request_data *r)
{
    UVC_DBG("# %s\n", __func__);
}

void pantilt2Buf(uint8_t *buf, uint32_t val)
{
    int32_t pan, tilt;

    pan = ((signed char)(val & 0xff)) * 3600;
    tilt = ((signed char)((val>>8) & 0xff)) * 3600;

    *buf = (pan & 0xff);
    *(buf+1) = ((pan >> 8) & 0xff);
    *(buf+2) = ((pan >> 16) & 0xff);
    *(buf+3) = ((pan >> 24) & 0xff);

    *(buf+4) = (tilt & 0xff);
    *(buf+5) = ((tilt>>8) & 0xff);
    *(buf+6) = ((tilt>>16) & 0xff);
    *(buf+7) = ((tilt>>24) & 0xff);
}


int8_t buf2pantilt(uint8_t *buf, unsigned char is_pan)
{
    int32_t pan_32, tilt_32;
    int8_t pan_8, tilt_8;

    pan_32 = (*buf) | (*(buf + 1) << 8) | (*(buf + 2) << 16) | (*(buf + 3) << 24);
    tilt_32 = (*buf + 4) | (*(buf + 5) << 8) | (*(buf + 6) << 16) | (*(buf + 7) << 24);

    if (is_pan)
    {
        pan_8 = (int8_t)((pan_32/3600) & 0xff);
        return pan_8;
    }
    else
    {
        tilt_8 = (int8_t)((tilt_32/3600) & 0xff);
        return tilt_8;
    }
}

#define pantilt2u16(pan,tilt) ((uint8_t)pan|((uint8_t)tilt)<<8)

static void ct_pantilt_absolute_setup(struct uvc_request_data *r)
{
    pantilt2Buf(r->data, uvc_control_ct[UVC_CT_PANTILT_ABSOLUTE_CONTROL].val.cur);
}

static void ct_pantilt_absolute_data(struct uvc_request_data *r)
{
    struct uvc_ctrl_event uvc_event;
    int result;
    int8_t pan,titl;

    pan = buf2pantilt(r->data, 1);
    titl = buf2pantilt(r->data, 0);

    uvc_control_ct[UVC_CT_PANTILT_ABSOLUTE_CONTROL].val.cur = pantilt2u16(pan,titl);

    uvc_event.func_num = UVC_PARAM_FUNC_PANTILTABS;
    uvc_event.parm1 = pan;
    uvc_event.parm2 = titl;

    result = rt_mq_send(&g_uvc_event_mq, (void *)&uvc_event, sizeof(uvc_event));
    if (result  != RT_EOK)
    {
        rt_kprintf("uvc message send failed\n");
    }
}

static void ct_pantilt_relative_setup(struct uvc_request_data *r)
{
    UVC_DBG("# %s\n", __func__);
}

static void ct_pantilt_relative_data(struct uvc_request_data *r)
{
    UVC_DBG("# %s\n", __func__);
}

static void ct_roll_absolute_setup(struct uvc_request_data *r)
{
    r->data[0] = uvc_control_ct[UVC_CT_ROLL_ABSOLUTE_CONTROL].val.cur & 0xff;
    r->data[1] = (uvc_control_ct[UVC_CT_ROLL_ABSOLUTE_CONTROL].val.cur >> 8) & 0xff;
}

static void ct_roll_absolute_data(struct uvc_request_data *r)
{
    struct uvc_ctrl_event uvc_event;
    int result;
    short val = r->data[0] | (r->data[1] << 8);

    uvc_control_ct[UVC_CT_ROLL_ABSOLUTE_CONTROL].val.cur = val;

    uvc_event.func_num = UVC_PARAM_FUNC_ROLLABS;
    uvc_event.parm1 = val;

    result = rt_mq_send(&g_uvc_event_mq, (void *)&uvc_event, sizeof(uvc_event));
    if (result  != RT_EOK)
    {
        rt_kprintf("uvc message send failed\n");
    }
}

static void ct_roll_relative_setup(struct uvc_request_data *r)
{
    UVC_DBG("# %s\n", __func__);
}

static void ct_roll_relative_data(struct uvc_request_data *r)
{
    UVC_DBG("# %s\n", __func__);
}

static void ct_privacy_setup(struct uvc_request_data *r)
{
    UVC_DBG("# %s\n", __func__);
}

static void ct_privacy_data(struct uvc_request_data *r)
{
    UVC_DBG("# %s\n", __func__);
}

static void ct_focus_simple_setup(struct uvc_request_data *r)
{
    UVC_DBG("# %s\n", __func__);
}

static void ct_focus_simple_data(struct uvc_request_data *r)
{
    UVC_DBG("# %s\n", __func__);
}

static void ct_window_setup(struct uvc_request_data *r)
{
    UVC_DBG("# %s\n", __func__);
}

static void ct_window_data(struct uvc_request_data *r)
{
    UVC_DBG("# %s\n", __func__);
}

static void ct_region_of_interest_setup(struct uvc_request_data *r)
{
    UVC_DBG("# %s\n", __func__);
}

static void ct_region_of_interest_data(struct uvc_request_data *r)
{
    ;
}

struct uvc_control_ops uvc_control_ct[] = {
    /* TODO item's length longer than 4 should be specific */
    /* len, max, min, cur, def, res, setup, data */
    /* UVC_CT_CONTROL_UNDEFINED */
    { 0, { 0, 0, 0, 0, 0}, NULL, NULL },
    /* UVC_CT_SCANNING_MODE_CONTROL */
    { 1, { 255, 0, 128, 128, 1 }, ct_scanning_mode_setup, ct_scanning_mode_data},
    /* UVC_CT_AE_MODE_CONTROL */
    { 1, {8 , 1, AEMODE_DEF, AEMODE_DEF, AEMODE_BITMAP},
                                                        ct_ae_mode_setup, ct_ae_mode_data},
    /* UVC_CT_AE_PRIORITY_CONTROL */
    { 1, { 1, 0, 0, 0, 1 }, ct_ae_priority_setup, ct_ae_priority_data},
    /* UVC_CT_EXPOSURE_TIME_ABSOLUTE_CONTROL */
    { 4, {EXPOSURE_MAX, EXPOSURE_MIN, EXPOSURE_DEF, EXPOSURE_DEF, 1 },
                                    ct_exposure_absolute_setup, ct_exposure_absolute_data},
    /* UVC_CT_EXPOSURE_TIME_RELATIVE_CONTROL */
    { 1, { 255, 0, 128, 128, 1 }, ct_exposure_relative_setup, ct_exposure_relative_data},
    /* UVC_CT_FOCUS_ABSOLUTE_CONTROL */
    { 2, { FOCUS_MAX, FOCUS_MIN, FOCUS_DEF, FOCUS_MAX, 5 },
                                        ct_focus_absolute_setup, ct_focus_absolute_data},
    /* UVC_CT_FOCUS_RELATIVE_CONTROL */
    { 2, { 255, 0, 128, 128, 1 }, ct_focus_relative_setup, ct_focus_relative_data},
    /* UVC_CT_FOCUS_AUTO_CONTROL */
    { 1, { 1, 0, FOCUSMODE_DEF, FOCUSMODE_DEF, 1 }, ct_focus_auto_setup, ct_focus_auto_data},
    /* UVC_CT_IRIS_ABSOLUTE_CONTROL */
    { 2, { IRIS_MAX, IRIS_MIN, IRIS_DEF, IRIS_DEF, 1 }, ct_iris_absolute_setup, ct_iris_absolute_data},
    /* UVC_CT_IRIS_RELATIVE_CONTROL */
    { 1, { 255, 0, 128, 128, 1 }, ct_iris_relative_setup, ct_iris_relative_data},
    /* UVC_CT_ZOOM_ABSOLUTE_CONTROL */
    { 2, { ZOOMABS_MAX, ZOOMABS_MIN, 100, ZOOMABS_DEF, 1 },
                                    ct_zoom_absolute_setup, ct_zoom_absolute_data},
    /* UVC_CT_ZOOM_RELATIVE_CONTROL */
    { 2, { 0x1f4, 0x64, 0x64, 0x64, 1 }, ct_zoom_relative_setup, ct_zoom_relative_data},
    /* UVC_CT_PANTILT_ABSOLUTE_CONTROL */
    { 8, { pantilt2u16(PANABS_MAX, TILTABS_MAX), pantilt2u16(PANABS_MIN, TILTABS_MIN),
            pantilt2u16(PANABS_DEF, TILTABS_DEF), pantilt2u16(PANABS_DEF, TILTABS_DEF),
            pantilt2u16(1, 1)}, ct_pantilt_absolute_setup, ct_pantilt_absolute_data},
    /* UVC_CT_PANTILT_RELATIVE_CONTROL */
    { 4, { 255, 0, 128, 128, 1 }, ct_pantilt_relative_setup, ct_pantilt_relative_data},
    /* UVC_CT_ROLL_ABSOLUTE_CONTROL */
    { 2, { ROLLABS_MAX, ROLLABS_MIN, ROLLABS_DEF, ROLLABS_DEF, 1 },
                                    ct_roll_absolute_setup, ct_roll_absolute_data},
    /* UVC_CT_ROLL_RELATIVE_CONTROL */
    { 2, { 255, 0, 128, 128, 1 }, ct_roll_relative_setup, ct_roll_relative_data},
    /* UVC_CT_PRIVACY_CONTROL */
    { 1, { 255, 0, 128, 128, 1 }, ct_privacy_setup, ct_privacy_data},
    /* UVC_CT_FOCUS_SIMPLE_CONTROL */
    { 1, { 255, 0, 128, 128, 1 }, ct_focus_simple_setup, ct_focus_simple_data},
    /* UVC_CT_WINDOW_CONTROL */
    { 1, { 255, 0, 128, 128, 1 }, ct_window_setup, ct_window_data},
    /* UVC_CT_REGION_OF_INTEREST_CONTROL */
    { 10, { 255, 0, 128, 128, 1 }, ct_region_of_interest_setup, ct_region_of_interest_data},
};

static void pu_backlight_setup(struct uvc_request_data *r)
{
    r->data[0] = uvc_control_pu[UVC_PU_BACKLIGHT_COMPENSATION_CONTROL].val.cur & 0xff;
    r->data[1] = (uvc_control_pu[UVC_PU_BACKLIGHT_COMPENSATION_CONTROL].val.cur >> 8) & 0xff;
}

static void pu_backlight_data(struct uvc_request_data *r)
{
    unsigned char val = r->data[0] | r->data[1] << 8;

    uvc_control_pu[UVC_PU_BACKLIGHT_COMPENSATION_CONTROL].val.cur = val;

    struct uvc_ctrl_event uvc_event;
    int result;

    uvc_event.func_num = UVC_PARAM_FUNC_SETBACKLIGHT;
    uvc_event.parm1 = val;
    result = rt_mq_send(&g_uvc_event_mq, (void *)&uvc_event, sizeof(uvc_event));
    if (result  != RT_EOK)
    {
        rt_kprintf("uvc message send failed\n");
    }
}

static void pu_brightness_setup(struct uvc_request_data *r)
{
    r->data[0] = uvc_control_pu[UVC_PU_BRIGHTNESS_CONTROL].val.cur & 0xff;
    r->data[1] = (uvc_control_pu[UVC_PU_BRIGHTNESS_CONTROL].val.cur >> 8) & 0xff;
}

static void pu_brightness_data(struct uvc_request_data *r)
{
    unsigned char val = r->data[0] | r->data[1] << 8;

    uvc_control_pu[UVC_PU_BRIGHTNESS_CONTROL].val.cur = val;

    struct uvc_ctrl_event uvc_event;
    int result;

    uvc_event.func_num = UVC_PARAM_FUNC_SETBRIGHTNESS;
    uvc_event.parm1 = val;
    result = rt_mq_send(&g_uvc_event_mq, (void *)&uvc_event, sizeof(uvc_event));
    if (result  != RT_EOK)
    {
        rt_kprintf("uvc message send failed\n");
    }
}

static void pu_contrast_setup(struct uvc_request_data *r)
{
    r->data[0] = uvc_control_pu[UVC_PU_CONTRAST_CONTROL].val.cur & 0xff;
    r->data[1] = (uvc_control_pu[UVC_PU_CONTRAST_CONTROL].val.cur >> 8) & 0xff;
}

static void pu_contrast_data(struct uvc_request_data *r)
{

    unsigned char val = r->data[0] | r->data[1] << 8;

    uvc_control_pu[UVC_PU_CONTRAST_CONTROL].val.cur = val;

    struct uvc_ctrl_event uvc_event;
    int result;

    uvc_event.func_num = UVC_PARAM_FUNC_SETCONTRAST;
    uvc_event.parm1 = val;
    result = rt_mq_send(&g_uvc_event_mq, (void *)&uvc_event, sizeof(uvc_event));
    if (result  != RT_EOK)
    {
        rt_kprintf("uvc message send failed\n");
    }
}

static void pu_gain_setup(struct uvc_request_data *r)
{
    UVC_DBG("# %s\n", __func__);
    r->data[0] = (uvc_control_pu[UVC_PU_GAIN_CONTROL].val.cur) & 0xff;
    r->data[1] = (uvc_control_pu[UVC_PU_GAIN_CONTROL].val.cur >> 8) & 0xff;
}

static void pu_gain_data(struct uvc_request_data *r)
{
    UVC_DBG("# %s\n", __func__);
    struct uvc_ctrl_event uvc_event;
    int result;
    int value = r->data[0] | r->data[1] << 8;

    uvc_control_pu[UVC_PU_GAIN_CONTROL].val.cur = value;

    uvc_event.func_num = UVC_PARAM_FUNC_SETGAIN;
    uvc_event.parm1 = value;

    result = rt_mq_send(&g_uvc_event_mq, (void *)&uvc_event, sizeof(uvc_event));
    if (result  != RT_EOK)
    {
        rt_kprintf("uvc message send failed\n");
    }
}

static void pu_power_freq_setup(struct uvc_request_data *r)
{
    r->data[0] = uvc_control_pu[UVC_PU_POWER_LINE_FREQUENCY_CONTROL].val.cur&0xff;
}

static void pu_power_freq_data(struct uvc_request_data *r)
{
    struct uvc_ctrl_event uvc_event;
    int result;

    uvc_control_pu[UVC_PU_POWER_LINE_FREQUENCY_CONTROL].val.cur = r->data[0]&0xff;

    uvc_event.func_num = UVC_PARAM_FUNC_POWERLINE;
    uvc_event.parm1 = r->data[0]&0xff;

    result = rt_mq_send(&g_uvc_event_mq, (void *)&uvc_event, sizeof(uvc_event));
    if (result  != RT_EOK)
    {
        rt_kprintf("uvc message send failed\n");
    }
}

static void pu_hue_setup(struct uvc_request_data *r)
{
    r->data[0] = (uvc_control_pu[UVC_PU_HUE_CONTROL].val.cur) & 0xff;
    r->data[1] = (uvc_control_pu[UVC_PU_HUE_CONTROL].val.cur >> 8) & 0xff;
}

static void pu_hue_data(struct uvc_request_data *r)
{
    unsigned char val = r->data[0] | r->data[1] << 8;
    rt_err_t result;
    struct uvc_ctrl_event uvc_event;

    uvc_control_pu[UVC_PU_HUE_CONTROL].val.cur = val;

    uvc_event.func_num = UVC_PARAM_FUNC_HUEGAIN;
    uvc_event.parm1 = val;

    result = rt_mq_send(&g_uvc_event_mq, (void *)&uvc_event, sizeof(uvc_event));
    if (result  != RT_EOK)
    {
        rt_kprintf("uvc message send failed\n");
    }
}

static void pu_saturation_setup(struct uvc_request_data *r)
{
    r->data[0] = (uvc_control_pu[UVC_PU_SATURATION_CONTROL].val.cur) & 0xff;
    r->data[1] = (uvc_control_pu[UVC_PU_SATURATION_CONTROL].val.cur >> 8) & 0xff;
}


static void pu_saturation_data(struct uvc_request_data *r)
{
    unsigned char val = r->data[0] | r->data[1] << 8;
    rt_err_t result;
    struct uvc_ctrl_event uvc_event;

    uvc_control_pu[UVC_PU_SATURATION_CONTROL].val.cur = val;

    uvc_event.func_num = UVC_PARAM_FUNC_SETSATURATION;
    uvc_event.parm1 = val;

    result = rt_mq_send(&g_uvc_event_mq, (void *)&uvc_event, sizeof(uvc_event));
    if (result  != RT_EOK)
    {
        rt_kprintf("uvc message send failed\n");
    }
}

static void pu_sharpness_setup(struct uvc_request_data *r)
{
    r->data[0] = (uvc_control_pu[UVC_PU_SHARPNESS_CONTROL].val.cur) & 0xff;
    r->data[1] = (uvc_control_pu[UVC_PU_SHARPNESS_CONTROL].val.cur >> 8) & 0xff;
}

static void pu_sharpness_data(struct uvc_request_data *r)
{
    unsigned char val = r->data[0] | r->data[1] << 8;
    rt_err_t result;
    struct uvc_ctrl_event uvc_event;

    UVC_DBG("# %s %d\n", __func__,val);

    uvc_control_pu[UVC_PU_SHARPNESS_CONTROL].val.cur = val;

    uvc_event.func_num = UVC_PARAM_FUNC_SETSHARPENESS;
    uvc_event.parm1 = val; /* 0 manual, 1 mapping */

    result = rt_mq_send(&g_uvc_event_mq, (void *)&uvc_event, sizeof(uvc_event));
    if (result  != RT_EOK)
    {
        rt_kprintf("uvc message send failed\n");
    }
}

static void pu_gamma_setup(struct uvc_request_data *r)
{
    UVC_DBG("# %s\n", __func__);
    r->data[0] = (uvc_control_pu[UVC_PU_GAMMA_CONTROL].val.cur) & 0xff;
    r->data[1] = (uvc_control_pu[UVC_PU_GAMMA_CONTROL].val.cur >> 8) & 0xff;
}

#define abs(s) ((s) < 0 ? -(s):(s))

static void pu_gamma_data(struct uvc_request_data *r)
{
    uint8_t gamma;
    rt_err_t result;
    struct uvc_ctrl_event uvc_event;

    gamma = r->data[0] | r->data[1] << 8;

    uvc_control_pu[UVC_PU_GAMMA_CONTROL].val.cur = gamma;

    uvc_event.func_num = UVC_PARAM_FUNC_SETGAMMACFG;
    uvc_event.parm1 = (uint32_t)gamma;

    result = rt_mq_send(&g_uvc_event_mq, (void *)&uvc_event, sizeof(uvc_event));
    if (result  != RT_EOK)
    {
        rt_kprintf("uvc message send failed\n");
    }
}

static void pu_wb_temperature_setup(struct uvc_request_data *r)
{
    UVC_DBG("# %s\n", __func__);
    r->data[0] = (uvc_control_pu[UVC_PU_WHITE_BALANCE_TEMPERATURE_CONTROL].val.cur) & 0xff;
    r->data[1] = (uvc_control_pu[UVC_PU_WHITE_BALANCE_TEMPERATURE_CONTROL].val.cur >> 8) & 0xff;
    r->data[2] = (uvc_control_pu[UVC_PU_WHITE_BALANCE_TEMPERATURE_CONTROL].val.cur >> 16) & 0xff;
    r->data[3] = (uvc_control_pu[UVC_PU_WHITE_BALANCE_TEMPERATURE_CONTROL].val.cur >> 24) & 0xff;
}

static void pu_wb_temperature_data(struct uvc_request_data *r)
{
    UVC_DBG("# %s\n", __func__);
    uint32_t temp;
    rt_err_t result;
    struct uvc_ctrl_event uvc_event;

    temp = r->data[0] | (r->data[1]<<8) | (r->data[2]<<16) | (r->data[3]<<24);

    uvc_control_pu[UVC_PU_WHITE_BALANCE_TEMPERATURE_CONTROL].val.cur = temp;

    uvc_event.func_num = UVC_PARAM_FUNC_SETAWBGAIN;
    uvc_event.parm1 = temp;

    result = rt_mq_send(&g_uvc_event_mq, (void *)&uvc_event, sizeof(uvc_event));
    if (result  != RT_EOK)
    {
        rt_kprintf("uvc message send failed\n");
    }
}

static void pu_wb_temperature_auto_setup(struct uvc_request_data *r)
{
    UVC_DBG("# %s\n", __func__);
    r->data[0] = (uvc_control_pu[UVC_PU_WHITE_BALANCE_TEMPERATURE_AUTO_CONTROL].val.cur) & 0xff;
}

static void pu_wb_temperature_auto_data(struct uvc_request_data *r)
{
    rt_err_t result;
    struct uvc_ctrl_event uvc_event;

    uvc_control_pu[UVC_PU_WHITE_BALANCE_TEMPERATURE_AUTO_CONTROL].val.cur = r->data[0];

    uvc_event.func_num = UVC_PARAM_FUNC_SETAWBMODE;
    uvc_event.parm1 = r->data[0];
    result = rt_mq_send(&g_uvc_event_mq, (void *)&uvc_event, sizeof(uvc_event));
    if (result  != RT_EOK)
    {
        rt_kprintf("uvc message send failed\n");
    }
}

static void pu_wb_component_setup(struct uvc_request_data *r)
{
    UVC_DBG("# %s\n", __func__);
}

static void pu_wb_component_data(struct uvc_request_data *r)
{
    UVC_DBG("# %s\n", __func__);
}

static void pu_wb_component_auto_setup(struct uvc_request_data *r)
{
    UVC_DBG("# %s\n", __func__);
}

static void pu_wb_component_auto_data(struct uvc_request_data *r)
{
    UVC_DBG("# %s\n", __func__);
}

static void pu_digital_multiplier_setup(struct uvc_request_data *r)
{
    UVC_DBG("# %s\n", __func__);
}

static void pu_digital_multiplier_data(struct uvc_request_data *r)
{
    UVC_DBG("# %s\n", __func__);
}

static void pu_digital_limit_setup(struct uvc_request_data *r)
{
    UVC_DBG("# %s\n", __func__);
}

static void pu_digital_limit_data(struct uvc_request_data *r)
{
    UVC_DBG("# %s\n", __func__);
}

static void pu_hue_auto_setup(struct uvc_request_data *r)
{
    r->data[0] = (uvc_control_pu[UVC_PU_HUE_AUTO_CONTROL].val.cur) & 0xff;
}

static void pu_hue_auto_data(struct uvc_request_data *r)
{
    rt_err_t result;
    struct uvc_ctrl_event uvc_event;

    uvc_control_pu[UVC_PU_HUE_AUTO_CONTROL].val.cur = r->data[0];

    uvc_event.func_num = UVC_PARAM_FUNC_HUEAUTO;
    uvc_event.parm1 = r->data[0];
    result = rt_mq_send(&g_uvc_event_mq, (void *)&uvc_event, sizeof(uvc_event));
    if (result  != RT_EOK)
    {
        rt_kprintf("uvc message send failed\n");
    }
}

static void pu_analog_video_setup(struct uvc_request_data *r)
{
    UVC_DBG("# %s\n", __func__);
}

static void pu_analog_video_data(struct uvc_request_data *r)
{
    UVC_DBG("# %s\n", __func__);
}

static void pu_analog_lock_setup(struct uvc_request_data *r)
{
    UVC_DBG("# %s\n", __func__);
}

static void pu_analog_lock_data(struct uvc_request_data *r)
{
    UVC_DBG("# %s\n", __func__);
}

struct uvc_control_ops uvc_control_pu[] = {
    /* len, max, min, cur, def, res, setup, data */
    /* 0 UVC_PU_CONTROL_UNDEFINED */
    { 0, { 0, 0, 0, 0, 0}, NULL, NULL },
    /* 1 UVC_PU_BACKLIGHT_COMPENSATION_CONTROL */
    { 2, {BACKLIGHT_MAX, BACKLIGHT_MIN, BACKLIGHT_DEF, BACKLIGHT_DEF, 1 },
                                            pu_backlight_setup,           pu_backlight_data},
    /* 2 UVC_PU_BRIGHTNESS_CONTROL */
    { 2, {BIGHTNESS_MAX, BIGHTNESS_MIN, BIGHTNESS_DEF, BIGHTNESS_DEF, 1},
                                            pu_brightness_setup,          pu_brightness_data},
    /* 3 UVC_PU_CONTRAST_CONTROL */
    { 2, {CONTRAST_MAX, CONTRAST_MIN, CONTRAST_DEF, CONTRAST_DEF, 1 },
                                                        pu_contrast_setup, pu_contrast_data},
    /* 4 UVC_PU_GAIN_CONTROL */
    { 2, {GAIN_MAX, GAIN_MIN, GAIN_DEF, GAIN_DEF, 1}, pu_gain_setup, pu_gain_data},
    /* 5 UVC_PU_POWER_LINE_FREQUENCY_CONTROL */
    { 1, { 2, 1, 1, 2, 1 }, pu_power_freq_setup,          pu_power_freq_data},
    /* 6 UVC_PU_HUE_CONTROL */
    { 2, {HUEGAIN_MAX, HUEGAIN_MIN, HUEGAIN_DEF, HUEGAIN_DEF, 1 },
                                                    pu_hue_setup,               pu_hue_data},
    /* 7 UVC_PU_SATURATION_CONTROL */
    { 2, {SATURATION_MAX, SATURATION_MIN, SATURATION_DEF, SATURATION_DEF, 1 },
                                            pu_saturation_setup,          pu_saturation_data},
    /* 8 UVC_PU_SHARPNESS_CONTROL */
    { 2, {SHARPENESS_MAX, SHARPENESS_MIN, SHARPENESS_DEF, SHARPENESS_DEF, 1 },
                                            pu_sharpness_setup,           pu_sharpness_data},
    /* 9 UVC_PU_GAMMA_CONTROL */
    { 2, {GAMMACFG_MAX, GAMMACFG_MIN, GAMMACFG_DEF, GAMMACFG_DEF, 1 },
                                                pu_gamma_setup,               pu_gamma_data},
    /* 10 UVC_PU_WHITE_BALANCE_TEMPERATURE_CONTROL */
    { 2, {AWBGAIN_MAX, AWBGAIN_MIN, AWBGAIN_DEF, AWBGAIN_DEF, 10 },
                                            pu_wb_temperature_setup, pu_wb_temperature_data},
    /* 11 UVC_PU_WHITE_BALANCE_TEMPERATURE_AUTO_CONTROL */
    { 1, { 1, 0, AWBMODE_DEF, AWBMODE_DEF, 1 }, pu_wb_temperature_auto_setup,       pu_wb_temperature_auto_data},
    /* 12 UVC_PU_WHITE_BALANCE_COMPONENT_CONTROL */
    { 4, { 255, 0, 128, 128, 1 }, pu_wb_component_setup,        pu_wb_component_data},
    /* 13 UVC_PU_WHITE_BALANCE_COMPONENT_AUTO_CONTROL */
    { 1, { 255, 0, 128, 128, 1 }, pu_wb_component_auto_setup,   pu_wb_component_auto_data},
    /* 14 UVC_PU_DIGITAL_MULTIPLIER_CONTROL */
    { 2, { 255, 0, 128, 128, 1 }, pu_digital_multiplier_setup,  pu_digital_multiplier_data},
    /* 15 UVC_PU_DIGITAL_MULTIPLIER_LIMIT_CONTROL */
    { 2, { 255, 0, 128, 128, 1 }, pu_digital_limit_setup,       pu_digital_limit_data},
    /* 16 UVC_PU_HUE_AUTO_CONTROL */
    { 1, { 1, 0, HUEAUTO_DEF, HUEAUTO_DEF, 1 }, pu_hue_auto_setup,            pu_hue_auto_data},
    /* 17 UVC_PU_ANALOG_VIDEO_STANDARD_CONTROL */
    { 1, { 255, 0, 128, 128, 1 }, pu_analog_video_setup,        pu_analog_video_data},
    /* 18 UVC_PU_ANALOG_LOCK_STATUS_CONTROL */
    { 1, { 255, 0, 128, 128, 1 }, pu_analog_lock_setup,         pu_analog_lock_data},
};
static void msxu_face_authentication(struct uvc_request_data *r)
{
    r->data[0] = (uvc_msxu_face[MSXU_CONTROL_FACE_AUTHENTICATION].val.cur) & 0xff;
    r->data[1] = (uvc_msxu_face[MSXU_CONTROL_FACE_AUTHENTICATION].val.cur >> 8) & 0xff;
    r->data[2] = (uvc_msxu_face[MSXU_CONTROL_FACE_AUTHENTICATION].val.cur >> 16) & 0xff;
}
static void msxu_face_data(struct uvc_request_data *r)
{
    UVC_DBG("# %s\n", __func__);
}
static void msxu_metadata_getdata(struct uvc_request_data *r)
{
    r->data[0] = (uvc_msxu_face[MSXU_CONTROL_METADATA].val.cur) & 0xff;
}
static void msxu_metadata_setdata(struct uvc_request_data *r)
{
    UVC_DBG("# %s\n", __func__);
}


struct uvc_control_ops uvc_msxu_face[] = {
    /* len, max, min, cur, def, res, setup, data */
    /* 0 MSXU_CONTROL_UNDEFINED */
    { 0, { 0, 0, 0, 0, 0}, NULL, NULL },
    /* 1 MSXU_CONTROL_FOCES */
    { 0, { 0, 0, 0, 0, 0}, NULL, NULL },
    /* 2 MSXU_CONTROL_EXPOSUER */
    { 0, { 0, 0, 0, 0, 0}, NULL, NULL },
    /* 3 MSXU_CONTROL_EVCOMPENSATION */
    { 0, { 0, 0, 0, 0, 0}, NULL, NULL },
    /* 4 MSXU_CONTROL_WHITEBALANCE */
    { 0, { 0, 0, 0, 0, 0}, NULL, NULL },
    /* 5 RESERVED */
    { 0, { 0, 0, 0, 0, 0}, NULL, NULL },
    /* 6 MSXU_CONTROL_FACE_AUTHENTICATION */
    { 9, { 0x020201, 0x020200, 0x020201, 0x020201, 0x020200}, msxu_face_authentication, msxu_face_data},
    /* 7 MSXU_CONTROL_EXTRINSICS */
    { 0, { 0, 0, 0, 0, 0}, NULL, NULL },
    /* 8 MSXU_CONTROL_INTRINSICS */
    { 0, { 0, 0, 0, 0, 0}, NULL, NULL },
    /* 9 MSXU_CONTROL_METADATA */
    { 4, { 0x10, 0, 0x10, 0, 1}, msxu_metadata_getdata, msxu_metadata_setdata},
    /* 10 MSXU_CONTROL_TORCH */
    { 0, { 0, 0, 0, 0, 0}, NULL, NULL },
    /* 11 MSXU_CONTROL_DIGITALWINDOW */
    { 0, { 0, 0, 0, 0, 0}, NULL, NULL },
    /* 12 MSXU_CONTROL_DIGITALWINDOW_CONFIG */
    { 0, { 0, 0, 0, 0, 0}, NULL, NULL },
    /* 13 MSXU_CONTROL_VIDEO_HDR */
    { 0, { 0, 0, 0, 0, 0}, NULL, NULL },
};

static void
uvc_interface_setup(struct uvc_stream_info *dev, uint8_t req, uint8_t cs,
               struct uvc_request_data *resp)
{
    if (dev->error_code)
    {
        resp->data[0] = dev->error_code;
        resp->length = 1;
        dev->error_code = UVC_REQ_ERROR_NO;
    }
}

static void
uvc_camera_terminal_setup(struct uvc_stream_info *dev, uint8_t req, uint8_t cs,
               struct uvc_request_data *resp)
{
    UVC_DBG("# %s req %d\n", __func__, req);

    if (cs > (sizeof(uvc_control_ct)/sizeof(uvc_control_ct[0])))
    {
        resp->length = -1;
        dev->error_code = UVC_REQ_ERROR_INVALID_CONTROL;
        return;
    }

    resp->length = uvc_control_ct[cs].len;

    if (uvc_camera_terminal_supported(cs) == 0)
    {
        resp->length = -1;
        dev->error_code = UVC_REQ_ERROR_INVALID_CONTROL;
        return;
    }

    if (cs == UVC_CT_EXPOSURE_TIME_ABSOLUTE_CONTROL)
    {
        if (req == UVC_GET_INFO)
        {
            if (uvc_control_ct[UVC_CT_AE_MODE_CONTROL].val.cur == 2 ||
                uvc_control_ct[UVC_CT_AE_MODE_CONTROL].val.cur == 8)
            {
                resp->data[0] = 0x0f; /* disable due to auto mode */
            }
            else
            {
               resp->data[0] = 0x0b;
            }
            return;
        }
        else if (req == UVC_SET_CUR)
        {
            if (uvc_control_ct[UVC_CT_AE_MODE_CONTROL].val.cur == 2 ||
                uvc_control_ct[UVC_CT_AE_MODE_CONTROL].val.cur == 8)
            {
                resp->length = -1;
                dev->error_code = UVC_REQ_ERROR_INVALID_REQ;
                return;
            }
        }
    }

    if (cs == UVC_CT_IRIS_ABSOLUTE_CONTROL)
    {
        if (req == UVC_GET_INFO)
        {
            if (uvc_control_ct[UVC_CT_AE_MODE_CONTROL].val.cur == 2 ||
                uvc_control_ct[UVC_CT_AE_MODE_CONTROL].val.cur ==4)
            {
                resp->data[0] = 0x0f; /* disable due to auto mode */
            }
            else
            {
               resp->data[0] = 0x0b;
            }
            return;
        }
        else if (req == UVC_SET_CUR)
        {
            if (uvc_control_ct[UVC_CT_AE_MODE_CONTROL].val.cur == 2 ||
                uvc_control_ct[UVC_CT_AE_MODE_CONTROL].val.cur == 4 ||
                uvc_control_ct[UVC_CT_AE_MODE_CONTROL].val.cur == 8)
            {
                resp->length = -1;
                dev->error_code = UVC_REQ_ERROR_INVALID_REQ;
                return;
            }
        }
    }

    if (cs == UVC_CT_FOCUS_ABSOLUTE_CONTROL)
    {
        if (req == UVC_GET_INFO)
        {
            if (uvc_control_ct[UVC_CT_FOCUS_AUTO_CONTROL].val.cur == 1)
            {
                resp->data[0] = 0x0f; /* disable due to auto mode */
            }
            else
            {
               resp->data[0] = 0x0b;
            }
            return;
        }
        else if (req == UVC_SET_CUR)
        {
            if (uvc_control_ct[UVC_CT_FOCUS_AUTO_CONTROL].val.cur == 1)
            {
                resp->length = -1;
                dev->error_code = UVC_REQ_ERROR_INVALID_REQ;
                return;
            }
        }
    }

    switch (req)
    {
    case UVC_SET_CUR:
        dev->context.cs = cs;
        dev->context.id = UVC_CTRL_CAMERAL_TERMINAL_ID;
        dev->context.intf = UVC_INTF_CONTROL;
        break;
    case UVC_GET_LEN:
        resp->data[0] = resp->length;
        break;

    case UVC_GET_CUR:
        /* if the value could be changed only in data phase,*/
        /* give the cur value directly */
        (*uvc_control_ct[cs].fsetup)(resp);
        break;
    case UVC_GET_MIN:
        /* TODO diff type */
        if (cs == UVC_CT_PANTILT_ABSOLUTE_CONTROL)
            pantilt2Buf(resp->data, uvc_control_ct[cs].val.min);
        else
        {
            resp->data[0] = uvc_control_ct[cs].val.min & 0xff;
            resp->data[1] = (uvc_control_ct[cs].val.min>>8) & 0xff;
            resp->data[2] = (uvc_control_ct[cs].val.min>>16) & 0xff;
            resp->data[3] = (uvc_control_ct[cs].val.min>>24) & 0xff;
        }
        break;
    case UVC_GET_MAX:
        if (cs == UVC_CT_PANTILT_ABSOLUTE_CONTROL)
            pantilt2Buf(resp->data, uvc_control_ct[cs].val.max);
        else
        {
            resp->data[0] = uvc_control_ct[cs].val.max & 0xff;
            resp->data[1] = (uvc_control_ct[cs].val.max>>8) & 0xff;
            resp->data[2] = (uvc_control_ct[cs].val.max>>16) & 0xff;
            resp->data[3] = (uvc_control_ct[cs].val.max>>24) & 0xff;
        }
        break;
    case UVC_GET_DEF:
        if (cs == UVC_CT_PANTILT_ABSOLUTE_CONTROL)
            pantilt2Buf(resp->data, uvc_control_ct[cs].val.def);
        else
        {
            resp->data[0] = uvc_control_ct[cs].val.def;
            resp->data[1] = (uvc_control_ct[cs].val.def>>8) & 0xff;
            resp->data[2] = (uvc_control_ct[cs].val.def>>16) & 0xff;
            resp->data[3] = (uvc_control_ct[cs].val.def>>24) & 0xff;
        }
        break;
    case UVC_GET_RES:
        if (cs == UVC_CT_PANTILT_ABSOLUTE_CONTROL)
            pantilt2Buf(resp->data, uvc_control_ct[cs].val.res);
        else
        {
            resp->data[0] = uvc_control_ct[cs].val.res & 0xff;
            resp->data[1] = (uvc_control_ct[cs].val.res>>8) & 0xff;
            resp->data[2] = (uvc_control_ct[cs].val.res>>16) & 0xff;
            resp->data[3] = (uvc_control_ct[cs].val.res>>24) & 0xff;
        }
        break;
    case UVC_GET_INFO:
        resp->data[0] = 3; /* support get and set req */
        break;
    default:
        DBG("# unkown req: %d\n", __func__, req);
        break;
    }
}

static void
uvc_processing_unit_setup(struct uvc_stream_info *dev, uint8_t req, uint8_t cs,
               struct uvc_request_data *resp)
{
    UVC_DBG("# %s\n", __func__);
    if (cs > (sizeof(uvc_control_pu) - 1))
    {
        return;
    }

    resp->length = uvc_control_pu[cs].len;

    if (uvc_processing_unit_supported(cs) == 0)
    {
        resp->length = -1;
        dev->error_code = UVC_REQ_ERROR_INVALID_CONTROL;
        return;
    }
    if (cs == UVC_PU_WHITE_BALANCE_TEMPERATURE_CONTROL)
    {
        if (req == UVC_GET_INFO)
        {
            if (uvc_control_pu[UVC_PU_WHITE_BALANCE_TEMPERATURE_AUTO_CONTROL].val.cur == 0x01)
            {
                resp->data[0] = 0x0f;
            }
            else
            {
                resp->data[0] = 0x0b;
            }
            return;
        }
    }
    if (cs == UVC_PU_WHITE_BALANCE_TEMPERATURE_AUTO_CONTROL)
    {
        if (req == UVC_GET_MIN || req == UVC_GET_MAX || req == UVC_GET_RES)
        {
            resp->length = -1;
            dev->error_code = UVC_REQ_ERROR_INVALID_REQ;
            return;
        }
    }

    switch (req)
    {
    case UVC_SET_CUR:
        dev->context.cs = cs;
        dev->context.id = UVC_CTRL_PROCESSING_UNIT_ID;
        dev->context.intf = UVC_INTF_CONTROL;
        break;
    case UVC_GET_LEN:
        resp->data[0] = resp->length;
        break;

    case UVC_GET_CUR:
        /* if the value could be changed only in data phase,*/
        /* give the cur value directly */
        (*uvc_control_pu[cs].fsetup)(resp);
        break;
    case UVC_GET_MIN:
        /* TODO diff type */
        resp->data[0] = uvc_control_pu[cs].val.min & 0xff;
        resp->data[1] = (uvc_control_pu[cs].val.min>>8) & 0xff;
        resp->data[2] = (uvc_control_pu[cs].val.min>>16) & 0xff;
        resp->data[3] = (uvc_control_pu[cs].val.min>>24) & 0xff;
        break;
    case UVC_GET_MAX:
        resp->data[0] = uvc_control_pu[cs].val.max & 0xff;
        resp->data[1] = (uvc_control_pu[cs].val.max>>8) & 0xff;
        resp->data[2] = (uvc_control_pu[cs].val.max>>16) & 0xff;
        resp->data[3] = (uvc_control_pu[cs].val.max>>24) & 0xff;
        break;
    case UVC_GET_DEF:
        resp->data[0] = uvc_control_pu[cs].val.def & 0xff;
        resp->data[1] = (uvc_control_pu[cs].val.def>>8) & 0xff;
        resp->data[2] = (uvc_control_pu[cs].val.def>>16) & 0xff;
        resp->data[3] = (uvc_control_pu[cs].val.def>>24) & 0xff;
        break;
    case UVC_GET_RES:
        resp->data[0] = uvc_control_pu[cs].val.res & 0xff;
        resp->data[1] = (uvc_control_pu[cs].val.res>>8) & 0xff;
        resp->data[2] = (uvc_control_pu[cs].val.res>>16) & 0xff;
        resp->data[3] = (uvc_control_pu[cs].val.res>>24) & 0xff;
        break;
    case UVC_GET_INFO:
        resp->data[0] = 3;
        break;
    default:
        DBG("# unkown req: %d\n", __func__, req);
        break;
    }
}

static void
uvc_h264_extension_setup(struct uvc_stream_info *dev, uint8_t req, uint8_t cs,
               struct uvc_request_data *resp)
{
    if (!g_uvc_ops || !g_uvc_ops->uvc_extern_intr)
    {
        resp->length = -1;
        dev->error_code = UVC_REQ_ERROR_INVALID_UNIT;
        return;
    }

    if (req == UVC_SET_CUR)
    {
        dev->context.cs = cs;
        dev->context.id = UVC_CTRL_H264_EXTENSION_UNIT_ID;
        dev->context.intf = UVC_INTF_CONTROL;
        return;
    }
    if (g_uvc_ops->uvc_extern_intr)
        g_uvc_ops->uvc_extern_intr(req, cs, resp);
}

static void
uvc_msxu_face_extension_setup(struct uvc_stream_info *dev, uint8_t req, uint8_t cs,
               struct uvc_request_data *resp)
{
    UVC_DBG("# %s\n", __func__);
    resp->length = uvc_msxu_face[cs].len;
    if (uvc_msxu_unit_supported(cs) == 0)
    {
        resp->length = -1;
        dev->error_code = UVC_REQ_ERROR_INVALID_CONTROL;
        return;
    }
    switch (req)
    {
    case UVC_SET_CUR:
        dev->context.cs = cs;
        dev->context.id = UVC_CTRL_MSXU_FACE_EXTENSION_UNIT_ID;
        dev->context.intf = UVC_INTF_CONTROL;
        break;
    case UVC_GET_LEN:
        resp->data[0] = resp->length;
        break;

    case UVC_GET_CUR:
        /* if the value could be changed only in data phase,*/
        /* give the cur value directly */
        if (uvc_msxu_face[cs].fsetup)
            (*uvc_msxu_face[cs].fsetup)(resp);
        break;
    case UVC_GET_MIN:
        /* TODO diff type */
        resp->data[0] = uvc_msxu_face[cs].val.min & 0xff;
        resp->data[1] = (uvc_msxu_face[cs].val.min>>8) & 0xff;
        resp->data[2] = (uvc_msxu_face[cs].val.min>>16) & 0xff;
        break;
    case UVC_GET_MAX:
        resp->data[0] = uvc_msxu_face[cs].val.max & 0xff;
        resp->data[1] = (uvc_msxu_face[cs].val.max>>8) & 0xff;
        resp->data[2] = (uvc_msxu_face[cs].val.max>>16) & 0xff;
        break;
    case UVC_GET_DEF:
        resp->data[0] = uvc_msxu_face[cs].val.def & 0xff;
        resp->data[1] = (uvc_msxu_face[cs].val.def>>8) & 0xff;
        resp->data[2] = (uvc_msxu_face[cs].val.def>>16) & 0xff;
        break;
    case UVC_GET_RES:
        resp->data[0] = uvc_msxu_face[cs].val.res & 0xff;
        resp->data[1] = (uvc_msxu_face[cs].val.res>>8) & 0xff;
        resp->data[2] = (uvc_msxu_face[cs].val.res>>16) & 0xff;
        break;
    case UVC_GET_INFO:
        if (cs == MSXU_CONTROL_FACE_AUTHENTICATION)
            resp->data[0] = 3;
        else if (cs == MSXU_CONTROL_METADATA)
            resp->data[0] = 1;
        break;
    default:
        DBG("# unkown req: %d\n", __func__, req);
        break;
    }
}
void
uvc_events_process_vc_setup(struct uvc_stream_info *dev, uint8_t req, uint8_t cs, uint8_t id,
               struct uvc_request_data *resp)
{

    switch (id)
    {
    case UVC_CTRL_INTERFACE_ID:
        uvc_interface_setup(dev, req, cs, resp);
        break;
    case UVC_CTRL_CAMERAL_TERMINAL_ID:
        uvc_camera_terminal_setup(dev, req, cs, resp);
        break;
    case UVC_CTRL_PROCESSING_UNIT_ID:
        uvc_processing_unit_setup(dev, req, cs, resp);
        break;
    case UVC_CTRL_OUTPUT_TERMINAL_ID:
        break;
    case UVC_CTRL_H264_EXTENSION_UNIT_ID:
        uvc_h264_extension_setup(dev, req, cs, resp);
        break;
    case UVC_CTRL_MSXU_FACE_EXTENSION_UNIT_ID:
        uvc_msxu_face_extension_setup(dev, req, cs, resp);
        break;
    default:
        DBG("# %s can not support termianl or UNIT id %d\n", __func__, id);
        break;
    }
}

static void
uvc_interface_data(struct uvc_stream_info *dev, uint8_t cs,
    struct uvc_request_data *data)
{
    ;
}

static void
uvc_camera_terminal_data(struct uvc_stream_info *dev, uint8_t cs,
    struct uvc_request_data *data)
{
    int i, val = 0;

    UVC_DBG("# %s\n", __func__);

    if (data->length != uvc_control_ct[cs].len)
    {
        UVC_DBG("# %s(cs %02x) data length %d is no valid\n",
            __func__, cs, data->length);
        return;
    }

    for (i=0; i < data->length; i++)
        val |= data->data[i] << (i * 8);

    if (cs == UVC_CT_PANTILT_ABSOLUTE_CONTROL)
    {
        int8_t pan,tilt;
        int8_t pan_max,tilt_max;
        int8_t pan_min,tilt_min;

        pan = buf2pantilt(data->data, 1);
        tilt = buf2pantilt(data->data, 0);
        pan_min = (int8_t)(uvc_control_ct[UVC_CT_PANTILT_ABSOLUTE_CONTROL].val.min&0xff);
        pan_max = (int8_t)(uvc_control_ct[UVC_CT_PANTILT_ABSOLUTE_CONTROL].val.max&0xff);
        tilt_max = (int8_t)(uvc_control_ct[UVC_CT_PANTILT_ABSOLUTE_CONTROL].val.max>>8&0xff);
        tilt_min = (int8_t)(uvc_control_ct[UVC_CT_PANTILT_ABSOLUTE_CONTROL].val.min>>8&0xff);

        if (pan > pan_max || pan < pan_min || tilt > tilt_max || tilt < tilt_min)
        {
            dev->error_code = UVC_REQ_ERROR_OUT_OF_RANGE;
            return;
        }
    } else if (val > uvc_control_ct[cs].val.max || val < uvc_control_ct[cs].val.min)
    {
        dev->error_code = UVC_REQ_ERROR_OUT_OF_RANGE;
        return;
    }
    if (cs == UVC_CT_AE_MODE_CONTROL &&
        val != 1 && val != 2 && val != 4 && val != 8)
    {
        dev->error_code = UVC_REQ_ERROR_OUT_OF_RANGE;
        return;
    }

    (*uvc_control_ct[cs].fdata)(data);
}

static void
uvc_processing_unit_data(struct uvc_stream_info *dev, uint8_t cs,
    struct uvc_request_data *data)
{
    int i, val = 0;

    if (data->length != uvc_control_pu[cs].len)
    {
        rt_kprintf("# %s(cs %02x) data length %d is no valid\n",
            __func__, cs, data->length);
        return;
    }

    for (i = 0; i < data->length; i++)
        val |= (data->data[i]) << (i * 8);
    if (val > uvc_control_pu[cs].val.max || val < uvc_control_pu[cs].val.min)
    {
        dev->error_code = UVC_REQ_ERROR_OUT_OF_RANGE;
        return;
    }
    if (cs == UVC_PU_WHITE_BALANCE_TEMPERATURE_CONTROL &&
        uvc_control_pu[UVC_PU_WHITE_BALANCE_TEMPERATURE_AUTO_CONTROL].val.cur == 0x01)
    {
        dev->error_code =  UVC_REQ_ERROR_INVALID_CONTROL;
        return;
    }
    (*uvc_control_pu[cs].fdata)(data);
}

static void
uvc_output_terminal_data(struct uvc_stream_info *dev, uint8_t cs, struct uvc_request_data *data)
{
    UVC_DBG("# %s\n", __func__);
}

static void
uvc_msxu_extension_data(struct uvc_stream_info *dev, uint8_t cs,
    struct uvc_request_data *data)
{
    if (data->length != uvc_msxu_face[cs].len)
    {
        rt_kprintf("# %s(cs %02x) data length %d is no valid\n",
            __func__, cs, data->length);
        return;
    }
    if (uvc_msxu_face[cs].fdata)
        (*uvc_msxu_face[cs].fdata)(data);
}
void uvc_events_process_vc_data(struct uvc_stream_info *dev, struct uvc_request_data *data)
{
    int8_t cs = dev->context.cs;
    int8_t id = dev->context.id;

    UVC_DBG("# %s(id:%02x cs %02x)\n",
        __func__, id, cs);

    switch (id)
    {
    case UVC_CTRL_INTERFACE_ID:
        uvc_interface_data(dev, cs, data);
        break;
    case UVC_CTRL_CAMERAL_TERMINAL_ID:
        uvc_camera_terminal_data(dev, cs, data);
        break;
    case UVC_CTRL_PROCESSING_UNIT_ID:
        uvc_processing_unit_data(dev, cs, data);
        break;
    case UVC_CTRL_OUTPUT_TERMINAL_ID:
        uvc_output_terminal_data(dev, cs, data);
        break;
    case UVC_CTRL_H264_EXTENSION_UNIT_ID:
        uvc_h264_extension_data(dev, cs, data);
        break;
    case UVC_CTRL_MSXU_FACE_EXTENSION_UNIT_ID:
        uvc_msxu_extension_data(dev, cs, data);
        break;
    default:
        DBG("# %s can not support termianl or UNIT id %d\n",
                __func__, id);
        break;
    }
}
static void
uvc_h264_extension_data(struct uvc_stream_info *dev, uint8_t cs, struct uvc_request_data *data)
{
    if (g_uvc_ops->uvc_extern_data)
        g_uvc_ops->uvc_extern_data(data->data,
                                    dev->context.size,
                                    cs);
}

void uvc_uspara_load(void)
{
    int i;
    int *g_uvc_uspara_buf;

    if (g_uvc_ops && g_uvc_ops->uvc_uspara)
        g_uvc_uspara_buf = g_uvc_ops->uvc_uspara();
    else
        return;

    for (i = 0; i < USER_PARAM_SUPPORT_CNT; i++)
    {
        if (g_uvc_uspara_buf[i] != USER_PARAM_NO_CHANGE_FLAG)
        {
            switch (i)
            {
            case USER_PARAM_GAIN:
                uvc_control_pu[UVC_PU_GAIN_CONTROL].val.cur = g_uvc_uspara_buf[i];
                break;
            case USER_PARAM_BRIGHTNESS:
                uvc_control_pu[UVC_PU_BRIGHTNESS_CONTROL].val.cur = g_uvc_uspara_buf[i];
                break;
            case USER_PARAM_CONTRAST:
                uvc_control_pu[UVC_PU_CONTRAST_CONTROL].val.cur = g_uvc_uspara_buf[i];
                break;
            case USER_PARAM_SATURATION:
                uvc_control_pu[UVC_PU_SATURATION_CONTROL].val.cur = g_uvc_uspara_buf[i];
                break;
            case USER_PARAM_SHARPENESS:
                uvc_control_pu[UVC_PU_SHARPNESS_CONTROL].val.cur = g_uvc_uspara_buf[i];
                break;
            case USER_PARAM_AWBGAIN:
                uvc_control_pu[UVC_PU_WHITE_BALANCE_TEMPERATURE_CONTROL].val.cur =
                                                                        g_uvc_uspara_buf[i];
                break;
            case USER_PARAM_AWBMODE:
                uvc_control_pu[UVC_PU_WHITE_BALANCE_TEMPERATURE_AUTO_CONTROL].val.cur =
                                                                        g_uvc_uspara_buf[i];
                break;
            case USER_PARAM_BACKLIGHT:
                uvc_control_pu[UVC_PU_BACKLIGHT_COMPENSATION_CONTROL].val.cur = g_uvc_uspara_buf[i];
                break;
            case USER_PARAM_HUEAUTO:
                uvc_control_pu[UVC_PU_HUE_AUTO_CONTROL].val.cur = g_uvc_uspara_buf[i];
                break;
            case USER_PARAM_HUEGAIN:
                uvc_control_pu[UVC_PU_HUE_CONTROL].val.cur = g_uvc_uspara_buf[i];
                break;
            case USER_PARAM_POWERLINE:
                uvc_control_pu[UVC_PU_POWER_LINE_FREQUENCY_CONTROL].val.cur = g_uvc_uspara_buf[i];
                break;
            case USER_PARAM_AEMODE:
                uvc_control_ct[UVC_CT_AE_MODE_CONTROL].val.cur = g_uvc_uspara_buf[i];
                break;
            case USER_PARAM_EXPOSURE:
                uvc_control_ct[UVC_CT_EXPOSURE_TIME_ABSOLUTE_CONTROL].val.cur =
                                                                g_uvc_uspara_buf[i];
                break;
            case USER_PARAM_AE_PRI:
                uvc_control_ct[UVC_CT_AE_PRIORITY_CONTROL].val.cur = g_uvc_uspara_buf[i];
                break;
            case USER_PARAM_GAMMACFG:
                uvc_control_pu[UVC_PU_GAMMA_CONTROL].val.cur = g_uvc_uspara_buf[i];
                break;
            case USER_PARAM_FOCUSMODE:
                uvc_control_ct[UVC_CT_FOCUS_AUTO_CONTROL].val.cur = g_uvc_uspara_buf[i];
                break;
            case USER_PARAM_FOCUSABS:
                uvc_control_ct[UVC_CT_FOCUS_ABSOLUTE_CONTROL].val.cur = g_uvc_uspara_buf[i];
                break;
            case USER_PARAM_ZOOMABS:
                uvc_control_ct[UVC_CT_ZOOM_ABSOLUTE_CONTROL].val.cur = g_uvc_uspara_buf[i];
                break;
            case USER_PARAM_PANABS:
                uvc_control_ct[UVC_CT_PANTILT_ABSOLUTE_CONTROL].val.cur |= g_uvc_uspara_buf[i];
                break;
            case USER_PARAM_TILTABS:
                uvc_control_ct[UVC_CT_PANTILT_ABSOLUTE_CONTROL].val.cur |= g_uvc_uspara_buf[i] << 8;
                break;
            case USER_PARAM_ROLLABS:
                uvc_control_ct[UVC_CT_ROLL_ABSOLUTE_CONTROL].val.cur = g_uvc_uspara_buf[i];
                break;
            case USER_PARAM_IRIS:
                uvc_control_ct[UVC_CT_IRIS_ABSOLUTE_CONTROL].val.cur = g_uvc_uspara_buf[i];
                break;
            default:
                break;
            }
        }
    }
}


void uvc_para_init(void)
{
    static int uvc_para_init = 0;

    if (uvc_para_init)
        return;

    uvc_para_init = 1;

    if (!g_uvc_ops)
        return;

    if (g_uvc_ops->SetAEMode)
        g_uvc_ops->SetAEMode(uvc_control_ct[UVC_CT_AE_MODE_CONTROL].val.cur);

    if (g_uvc_ops->SetExposure)
        g_uvc_ops->SetExposure(uvc_control_ct[UVC_CT_EXPOSURE_TIME_ABSOLUTE_CONTROL].val.cur);

    if (g_uvc_ops->SetGain)
        g_uvc_ops->SetGain(uvc_control_pu[UVC_PU_GAIN_CONTROL].val.cur);

    if (g_uvc_ops->SetBrightness)
        g_uvc_ops->SetBrightness(uvc_control_pu[UVC_PU_BRIGHTNESS_CONTROL].val.cur);

    if (g_uvc_ops->SetContrast)
        g_uvc_ops->SetContrast(uvc_control_pu[UVC_PU_CONTRAST_CONTROL].val.cur);

    if (g_uvc_ops->SetSaturation)
        g_uvc_ops->SetSaturation(uvc_control_pu[UVC_PU_SATURATION_CONTROL].val.cur);

    if (g_uvc_ops->SetSharpeness)
        g_uvc_ops->SetSharpeness(uvc_control_pu[UVC_PU_SHARPNESS_CONTROL].val.cur);

    if (g_uvc_ops->SetGammaCfg)
        g_uvc_ops->SetGammaCfg(uvc_control_pu[UVC_PU_GAMMA_CONTROL].val.cur);

    if (g_uvc_ops->SetAWBGain)
        g_uvc_ops->SetAWBGain(uvc_control_pu[UVC_PU_WHITE_BALANCE_TEMPERATURE_CONTROL].val.cur);

    if (g_uvc_ops->SetAwbMode)
        g_uvc_ops->SetAwbMode(uvc_control_pu[UVC_PU_WHITE_BALANCE_TEMPERATURE_AUTO_CONTROL].val.cur);

    if (g_uvc_ops->SetBacklight)
        g_uvc_ops->SetBacklight(uvc_control_pu[UVC_PU_BACKLIGHT_COMPENSATION_CONTROL].val.cur);

    if (g_uvc_ops->SetHueAuto)
        g_uvc_ops->SetHueAuto(uvc_control_pu[UVC_PU_HUE_AUTO_CONTROL].val.cur);

    if (g_uvc_ops->SetHueGain)
        g_uvc_ops->SetHueGain(uvc_control_pu[UVC_PU_HUE_CONTROL].val.cur);

    if (g_uvc_ops->SetPowerLine)
        g_uvc_ops->SetPowerLine(uvc_control_pu[UVC_PU_POWER_LINE_FREQUENCY_CONTROL].val.cur);

    if (g_uvc_ops->SetAe_Priority)
        g_uvc_ops->SetAe_Priority(uvc_control_ct[UVC_CT_AE_PRIORITY_CONTROL].val.cur);

    if (g_uvc_ops->SetFocusMode)
        g_uvc_ops->SetFocusMode(uvc_control_ct[UVC_CT_FOCUS_AUTO_CONTROL].val.cur);

    if (g_uvc_ops->SetFocusAbs)
        g_uvc_ops->SetFocusAbs(uvc_control_ct[UVC_CT_FOCUS_ABSOLUTE_CONTROL].val.cur);

    if (g_uvc_ops->SetZoomAbs)
        g_uvc_ops->SetZoomAbs(uvc_control_ct[UVC_CT_ZOOM_ABSOLUTE_CONTROL].val.cur);

    if (g_uvc_ops->SetPanTiltAbs)
        g_uvc_ops->SetPanTiltAbs(uvc_control_ct[UVC_CT_PANTILT_ABSOLUTE_CONTROL].val.cur & 0xff,
                            (uvc_control_ct[UVC_CT_PANTILT_ABSOLUTE_CONTROL].val.cur >> 8) & 0xff);

    if (g_uvc_ops->SetRollAbs)
        g_uvc_ops->SetRollAbs(uvc_control_ct[UVC_CT_ROLL_ABSOLUTE_CONTROL].val.cur);
}

