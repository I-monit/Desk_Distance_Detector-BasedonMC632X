#include "osd_obj_button.h"
#include "dsp/fh_common.h"
#include "../mock/key_event.h"
#include "../core/osd_log.h"
#include "../core/osd_manager.h"
#include "osd_obj_common.h"
#include "../mod/user_interface.h"

static void button_draw_enter(button_t *this);
static void obj_button_draw(osd_obj_t *obj);
static void button_method_init(button_t *this);
static void button_enter(button_t *this);
static void obj_op_button(osd_obj_t *obj, FH_UINT8 event);

static void button_draw_enter(button_t *this) {
    char *string;
    char enter_lable;
    string = ";";
    enter_lable = string[0];

    if (this->type == TO_SUBMENU)
        string_set_with_offset(this->val_pos, &enter_lable, 1);
}

static void obj_button_draw(osd_obj_t *obj) {
    button_t *this = to_button(obj);

    obj_set_title_string(obj);
    button_draw_enter(this);
    obj_osd_draw(obj);
}

static void button_method_init(button_t *this) {
    method_op_register(&this->base, obj_op_button);
    method_draw_register(&this->base, obj_button_draw);
}

static void obj_op_button(osd_obj_t *obj, FH_UINT8 event){
    button_t *c = to_button(obj);
    if (obj_isFocused(obj)){
        switch (event) {
            case EVENT_UP_CLICK:
                obj_focus_pre(obj);
                break;
            case EVENT_DOWN_CLICK:
                obj_focus_next(obj);
                break;
            case EVENT_ENTER_CLICK:
                button_enter(c);
                break;
            default:
                break;
        }
    }
}

static void button_enter(button_t *this) {
    OSD_LOG("[button] enter, id: %d\n", obj_getId(&this->base));
    if (this->type == NORMAL) {
        focus_reset(&this->base);
        obj_setFocused(obj_getHead(&this->base), 1);
    }

    if (this->jumpto != 0)
        manager_setRootMenu(this->jumpto);

    if (this->clickCallback)
        this->clickCallback(this);
}

void button_setJumpTo(button_t *this, osd_obj_t *jumpto) {
    if (jumpto != 0)
        this->jumpto = jumpto;
}

void button_setType(button_t *this, FH_UINT16 type) {
    this->type = type;
}

void button_setValuePos(button_t *this, int pos) {
    this->val_pos = pos;
}

void button_init(button_t *this) {
    obj_init(&this->base);
    this->jumpto = 0;
    button_method_init(this);
}

void button_registerClickCallback(button_t *this, fClickCallback cb) {
    this->clickCallback = cb;
}
