#include "overlay/include/overlay_logo.h"
#include "overlay/include/logo_array.h"

static char *model_name = MODEL_TAG_DEMO_OVERLAY_LOGO;

FH_SINT32 sample_set_logo(FH_SINT32 grp_id)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    FHT_OSD_Logo_t logo_cfg;

    memset(&logo_cfg, 0, sizeof(logo_cfg));

    logo_cfg.enable = 1;                        /* 使能logo显示 */
    logo_cfg.rotate = 0;                        /* 不旋转 */
    logo_cfg.maxW = AUTO_GEN_PIC_WIDTH;         /* 最大logo像素宽度，用于第一次设置内存分配 */
    logo_cfg.maxH = AUTO_GEN_PIC_HEIGHT;        /* 最大logo像素高度，用于第一次设置内存分配 */
    logo_cfg.alpha = 255;                       /* 不透明 */
    logo_cfg.area.fTopLeftX = 0;                /* logo左上角起始点像素宽度位置 */
    logo_cfg.area.fTopLeftY = 64;               /* logo左上角起始点像素高度位置 */
    logo_cfg.area.fWidth = AUTO_GEN_PIC_WIDTH;  /* logo实际像素宽度 */
    logo_cfg.area.fHeigh = AUTO_GEN_PIC_HEIGHT; /* logo实际像素高度 */
    logo_cfg.pData = logo_data;                 /* logo数据 */

    ret = FHAdv_Osd_SetLogo(grp_id, &logo_cfg);
    SDK_FUNC_ERROR(model_name, ret);

    return ret;
}
