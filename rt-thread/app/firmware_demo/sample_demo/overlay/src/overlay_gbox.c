#include "overlay/include/overlay_gbox.h"

static char *model_name = MODEL_TAG_DEMO_OVERLAY_GBOX;

static FH_VOID checkParams(FH_SINT32 grp_id, FH_SINT32 chn_id, OSD_GBOX *gbox)
{
    FH_UINT32 chn_w = 0;
    FH_UINT32 chn_h = 0;

    chn_w = get_vpu_chn_w(grp_id, chn_id);
    chn_h = get_vpu_chn_h(grp_id, chn_id);

    gbox->x = CLIP(gbox->x, 0, chn_w);
    gbox->y = CLIP(gbox->y, 0, chn_h);
    gbox->w = CLIP(gbox->w, 0, chn_w);
    gbox->h = CLIP(gbox->h, 0, chn_h);

    if (gbox->x + gbox->w > chn_w)
    {
        gbox->w = chn_w - gbox->x;
    }

    if (gbox->y + gbox->h > chn_h)
    {
        gbox->h = chn_h - gbox->y;
    }
}

static FH_VOID getChnSize(FH_SINT32 grp_id, FH_SINT32 chn_id, FH_SINT32 *w, FH_SINT32 *h)
{
#ifdef UVC_ENABLE
    *w = CHN0_MAX_WIDTH;
    *h = CHN0_MAX_HEIGHT;
#else
    *w = get_vpu_chn_w(grp_id, chn_id);
    *h = get_vpu_chn_h(grp_id, chn_id);
#endif
}

FH_SINT32 sample_set_gbox(FH_SINT32 grp_id, FH_SINT32 chn_id, OSD_GBOX *gbox)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    FHT_OSD_Gbox_t gbox_cfg;
    FH_SINT32 w = 0, h = 0;

    memset(&gbox_cfg, 0, sizeof(gbox_cfg));

    SDK_CHECK_NULL_PTR(model_name, gbox);

    checkParams(grp_id, chn_id, gbox);
    getChnSize(grp_id, chn_id, &w, &h);

    /* 配置gbox 0 */
    gbox_cfg.enable = gbox->enable;  /* 使能gbox显示 */
    gbox_cfg.gboxId = gbox->gbox_id; /* gbox ID 为0 */
    gbox_cfg.max_frame_w = w;
    gbox_cfg.max_frame_h = h;

    if (gbox->enable)
    {
        gbox_cfg.rotate = 0;               /* 不旋转 */
        gbox_cfg.gboxLineWidth = 2;        /* gbox边框像素宽度为2*/
        gbox_cfg.area.fTopLeftX = gbox->x; /* gbox左上角起始点像素宽度位置 */
        gbox_cfg.area.fTopLeftY = gbox->y; /* gbox左上角起始点像素高度位置 */
        gbox_cfg.area.fWidth = gbox->w;    /* gbox宽度 */
        gbox_cfg.area.fHeigh = gbox->h;    /* gbox高度 */
        gbox_cfg.osdColor.fAlpha = gbox->color.fAlpha;
        gbox_cfg.osdColor.fRed = gbox->color.fRed;
        gbox_cfg.osdColor.fGreen = gbox->color.fGreen;
        gbox_cfg.osdColor.fBlue = gbox->color.fBlue;
    }
    else /*just work-around FHAdv_Osd_Ex_SetGbox,it will check fWidth and fHeight*/
    {
        gbox_cfg.gboxLineWidth = 2;
        gbox_cfg.area.fTopLeftX = 0;
        gbox_cfg.area.fTopLeftY = 0;
        gbox_cfg.area.fWidth = 16;
        gbox_cfg.area.fHeigh = 16;
        gbox_cfg.osdColor.fAlpha = gbox->color.fAlpha;
        gbox_cfg.osdColor.fRed = gbox->color.fRed;
        gbox_cfg.osdColor.fGreen = gbox->color.fGreen;
        gbox_cfg.osdColor.fBlue = gbox->color.fBlue;
    }

#ifdef FH_APP_OVERLAY_GBOX
    ret = FHAdv_Osd_Ex_SetGbox(grp_id, chn_id, &gbox_cfg);
    SDK_FUNC_ERROR(model_name, ret);
#endif

    return ret;

Exit:
    return FH_SDK_FAILED;
}
