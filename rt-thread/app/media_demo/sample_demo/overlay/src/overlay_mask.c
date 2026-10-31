#include "overlay/include/overlay_mask.h"

static char *model_name = MODEL_TAG_DEMO_OVERLAY_MASK;

FH_SINT32 sample_set_mask(FH_SINT32 grp_id)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    FHT_OSD_Mask_t mask_cfg;

    memset(&mask_cfg, 0, sizeof(mask_cfg));

    /*配置mask 0*/
    mask_cfg.enable = 1;           /* 使能mask显示 */
    mask_cfg.maskId = 0;           /* mask ID 为0 */
    mask_cfg.rotate = 0;           /* 不旋转 */
    mask_cfg.area.fTopLeftX = 16;  /* mask左上角起始点像素宽度位置 */
    mask_cfg.area.fTopLeftY = 128; /* mask左上角起始点像素高度位置 */
    mask_cfg.area.fWidth = 64;     /* mask宽度 */
    mask_cfg.area.fHeigh = 64;     /* mask高度 */
    mask_cfg.osdColor.fAlpha = 255;
    mask_cfg.osdColor.fRed = 0;
    mask_cfg.osdColor.fGreen = 255;
    mask_cfg.osdColor.fBlue = 0;

    ret = FHAdv_Osd_SetMask(grp_id, &mask_cfg);
    SDK_FUNC_ERROR(model_name, ret);

    /*配置mask 1*/
    mask_cfg.enable = 1;           /* 使能mask显示 */
    mask_cfg.maskId = 1;           /* mask ID 为1 */
    mask_cfg.rotate = 0;           /* 不旋转 */
    mask_cfg.area.fTopLeftX = 90;  /* mask左上角起始点像素宽度位置 */
    mask_cfg.area.fTopLeftY = 128; /* mask左上角起始点像素高度位置 */
    mask_cfg.area.fWidth = 64;     /* mask宽度 */
    mask_cfg.area.fHeigh = 64;     /* mask高度 */

    ret = FHAdv_Osd_SetMask(grp_id, &mask_cfg);
    SDK_FUNC_ERROR(model_name, ret);

    /* 清除mask 1 */
    mask_cfg.enable = 0; /* 去使能mask显示 */
    mask_cfg.maskId = 1; /* mask ID 为1 */
    ret = FHAdv_Osd_SetMask(grp_id, &mask_cfg);
    SDK_FUNC_ERROR(model_name, ret);

    return ret;
}
