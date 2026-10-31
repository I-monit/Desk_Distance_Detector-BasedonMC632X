#include "configure.h"

menu_conf_t defaultConf = {
    .autoOrder  = 1,
    .enable     = 1,
    .baseId     = 0,
    .screenPosX = 2,
    .screenPosY = 1,
    .compPosX   = 1,
    .valPosX    = 20,
};

static void obj_configure(osd_obj_t *this, menu_conf_t *param) {
    obj_setEnable(this, 1);
    obj_setOffsetX(this, param->compPosX);
    obj_setOffsetY(this, 0);
    obj_setSelectable(this, 1);
    obj_setVisible(this, 1);
    obj_setFocused(this, 0);
}

void menu_configure(menu_t *this, menu_conf_t *param) {
    menu_init(this);
    obj_configure(&this->base, param);
    menu_setAutoOrder(this, param->autoOrder);
    menu_setOffsetX(this, param->screenPosX);
    menu_setOffsetY(this, param->screenPosY);
    obj_setSelectable(&this->base, 0);
}

void scrollbar_configure(scrollbar_t *this, menu_conf_t *param) {
    scrollbar_init(this);
    scrollbar_setValuePos(this, param->valPosX);
    obj_configure(&this->base, param);
}

void combobox_configure(combobox_t *this, menu_conf_t *param) {
    combobox_init(this);
    combobox_setValuePos(this, param->valPosX);
    obj_configure(&this->base, param);
}

void button_configure(button_t *this, menu_conf_t *param) {
    button_init(this);
    button_setValuePos(this, param->valPosX);
    obj_configure(&this->base, param);
}

void textbox_configure(textbox_t *this, menu_conf_t *param) {
    textbox_init(this);
    obj_configure(&this->base, param);
}

void combobutton_configure(combobutton_t *this, menu_conf_t *param) {
    combobutton_init(this);
    combobutton_setValuePos(this, param->valPosX);
    obj_configure(&this->base, param);
}
