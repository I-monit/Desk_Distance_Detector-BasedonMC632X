#ifndef _CONFIGURE_H_
#define _CONFIGURE_H_

#include "../../component/osd_components.h"

typedef struct menu_configure {
    int autoOrder;
    int enable;
    int baseId;
    int screenPosX;  // 菜单相对屏幕位置
    int screenPosY;
    int width;
    int height;

    int compPosX;  // 控件水平偏移
    int valPosX;   // 控件数值水平位置偏移
} menu_conf_t;

void menu_configure(menu_t *this, menu_conf_t *param);
void scrollbar_configure(scrollbar_t *this, menu_conf_t *param);
void combobox_configure(combobox_t *this, menu_conf_t *param);
void button_configure(button_t *this, menu_conf_t *param);
void textbox_configure(textbox_t *this, menu_conf_t *param);
void combobutton_configure(combobutton_t *this, menu_conf_t *param);

extern menu_conf_t defaultConf;

#endif