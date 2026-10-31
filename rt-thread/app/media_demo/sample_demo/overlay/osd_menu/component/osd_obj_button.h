
#ifndef _OSD_OBJ_BUTTON_H_
#define _OSD_OBJ_BUTTON_H_

#include "dsp/fh_common.h"
#include "component_base.h"
#include "../core/osd_obj.h"

enum BUTTON_TYPE {
    TO_SUBMENU = 0,  // 包含小箭头，不重置focus
    NORMAL     = 1
};

typedef struct _button {
    osd_obj_t base;

    FH_UINT8     type;     // 按钮类型
    FH_UINT8     val_pos;  // 回车符显示位置
    osd_obj_t *jumpto;   // 跳转到菜单对象

    fClickCallback clickCallback;
} button_t;

#define to_button(x) container_of((x), button_t, base)

void button_init(button_t *this);
void button_setJumpTo(button_t *this, osd_obj_t *jumpto);
void button_setType(button_t *this, FH_UINT16 type);
void button_setValuePos(button_t *this, int pos);
void button_registerClickCallback(button_t *this, fClickCallback cb);

#endif  // _OSD_OBJ_BUTTON_H_
