#ifndef _OSD_MAIN_H_
#define _OSD_MAIN_H_

typedef struct osd_menu_attr
{
    unsigned int osd_group_id;  /* 菜单所在group */
    unsigned int osd_channel_id;    /* 菜单所在通道 */
    unsigned int osd_font_size; /* 字体大小 */
    unsigned int row_space;     /* 行间距 */
    unsigned int pos_offset_x;  /* 起始点偏移 */
    unsigned int pos_offset_y;  /* 起始点偏移 */
    unsigned int font_color;    /* 字体颜色 */
    unsigned int edge_color;    /* 描边颜色 */
    unsigned int layerMaxWidth; /* 菜单最大宽度，仅第一次生效，请按最大幅面菜单所需大小配置 */
    unsigned int layerMaxHeight;/* 菜单最大高度，仅第一次生效，请按最大幅面菜单所需大小配置 */

} OSD_MENU_PARA_t;

void osd_menu_init(OSD_MENU_PARA_t *osd_para);
void osd_menu_paracfg(OSD_MENU_PARA_t *osd_para);
void osd_menu_handle(int event);
void osd_menu_exit(void);

#define OSD_MENU_LAYER_ID (1)   /* 菜单画在哪个layer上 */

#define OSD_DEF_GROUP_ID        (FH_APP_GRP_ID)    /* 默认的groupid */
#define OSD_DEF_CHANNEL         (0)  /* 默认显示通道 */
#define OSD_DEF_FONT_SIZE       (32) /* 默认字体大小，实际应根据幅面大小进行修改 */
#define OSD_DEF_RAW_SPACE       (10) /* 默认行间距 */
#define OSD_DEF_POS_OFFSET_X    (128) /* 默认菜单起始位置x偏移 */
#define OSD_DEF_POS_OFFSET_Y    (128) /* 默认菜单起始位置y偏移 */
#define OSD_DEF_FONT_COLOR      (0xffffffff) /* 默认字体颜色 */
#define OSD_DEF_EDGE_COLOR      (0xff000000) /* 默认字体勾边颜色 */
#define OSD_DEF_MAX_LAYER_W     (1024) /* 默认菜单最大宽度 */
#define OSD_DEF_MAX_LAYER_H     (768) /* 默认菜单最大高度 */
#endif /*_OSD_MAIN_H_*/
