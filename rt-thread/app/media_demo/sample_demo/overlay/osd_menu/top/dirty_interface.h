#ifndef _DIRTY_INTERFACE_H_
#define _DIRTY_INTERFACE_H_

/**
 * 访问非OSD相关模块的数据，需通过此接口
 * OSD内部模块将通过此模块对外进行访问
 */

#include "dsp/fh_common.h"
#include "../osd_main.h"
typedef struct osd_para_ext {
    FH_SIZE pic_size;
    FH_SIZE font_size;
    FH_UINT32 auto_exit_timer;
} osd_para_ext_t;

int getOutputResolutionX();
int getOutputResolutionY();
int getAutoExitTimer(void);
int osd_menu_config(FH_UINT32 x, FH_UINT32 y, FH_UINT32 id, FH_CHAR *string);
int osd_menu_draw(void);
int osd_menu_clean(FH_UINT32 id);
int fh_strlen(const char *s);

extern OSD_MENU_PARA_t g_osd_para;
#endif  // !_DIRTY_INTERFACE_H_