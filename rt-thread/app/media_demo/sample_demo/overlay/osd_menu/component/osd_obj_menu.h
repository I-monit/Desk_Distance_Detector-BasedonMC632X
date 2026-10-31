
#ifndef _OSD_OBJ_MENU_H_
#define _OSD_OBJ_MENU_H_

#include "dsp/fh_common.h"
#include "component_base.h"
#include "../core/osd_obj.h"

// MENU
typedef struct _menu {
    osd_obj_t base;

    FH_UINT8 id;         // 菜单编号(不同于控件编号)
    FH_UINT8 offset_x;   //菜单偏移
    FH_UINT8 offset_y;
    FH_UINT8 autoOrder;  // 1: 控件显示位置由菜单决定 0:控件显示位置由自身决定

    fDrawCallback drawCallback;
} menu_t;

#define to_menu(x) container_of((x), menu_t, base)

void menu_setAutoOrder(menu_t *self, int autoOrder);
void menu_setOffsetX(menu_t *this, FH_UINT8 x);
void menu_setOffsetY(menu_t *this, FH_UINT8 y);
void menu_setId(menu_t *this, int id);
void menu_init(menu_t *self);
void menu_registerDrawCallback(menu_t *self, fDrawCallback cb);

#endif /*_OSD_OBJ_MENU_H_*/
