#include "osd_obj_combobutton.h"
#include "dsp/fh_common.h"
#include "../mock/key_event.h"
#include "osd_obj_common.h"
#include "../mod/user_interface.h"
#include "../core/osd_log.h"
#include "../core/osd_manager.h"
#include "../top/dirty_interface.h"

#define COMBOBUTTON_WIDTH_MAX (32)

static void combobutton_draw_value(combobutton_t *self);
static void obj_combobutton_draw(osd_obj_t *obj);
static void combobutton_method_init(combobutton_t *self);
static void combobutton_left(combobutton_t *self);
static void combobutton_right(combobutton_t *self);
static void combobutton_enter(combobutton_t *this);
static void obj_op_combobutton(osd_obj_t *obj, FH_UINT8 event);


static void combobutton_draw_value(combobutton_t *self) {
    osd_obj_t *obj = &self->base;
    atrribute_button_t *array = self->array;
    int curr = self->curr;

    char *text;
    char *string;
    char  buf[COMBOBUTTON_WIDTH_MAX];
    int   textlen;
    int   len = 0;

    // draw left arrow
    string = "<";
    buf[len++] = string[0];

    // draw text
    text    = getString(obj_getId(obj) + self->curr + 1);
    textlen = fh_strlen(text);
    memcpy(&buf[len], text, textlen);
    len += textlen;

    //draw enter
    if (array[curr].type == TO_SUBMENU){
        string = ";";
        buf[len++] = string[0];
    }

    // draw right arrow
    string = ">";
    buf[len++] = string[0];

    // fill screen
    string_set_with_offset(self->val_pos, buf, len);
}

static void obj_combobutton_draw(osd_obj_t *obj) {
    combobutton_t *c = to_combobutton(obj);

    // draw text
    obj_set_title_string(obj);

    // get value
    if (c->getValueCallback)
        c->getValueCallback(c);  // get scrollbar cur num

    // draw value
    combobutton_draw_value(c);

    obj_osd_draw(obj);
}

static void combobutton_method_init(combobutton_t *self) {
    method_op_register(&self->base, obj_op_combobutton);
    method_draw_register(&self->base, obj_combobutton_draw);
}


static void obj_op_combobutton(osd_obj_t *obj, FH_UINT8 event){
    combobutton_t *c = to_combobutton(obj);
    if (obj_isFocused(obj)){
        switch (event) {
            case EVENT_UP_CLICK:
                obj_focus_pre(obj);
                break;
            case EVENT_DOWN_CLICK:
                obj_focus_next(obj);
                break;
            case EVENT_ENTER_CLICK:
                combobutton_enter(c);
                break;
            case EVENT_LEFT_CLICK:
                combobutton_left(c);
                break;
            case EVENT_RIGHT_CLICK:
                combobutton_right(c);
                break;
            default:
                break;
        }
    }
}

static void combobutton_enter(combobutton_t *this) {
    OSD_LOG("[combobutton] enter, id: %d\n", obj_getId(&this->base));
    int curr = this->curr;
    atrribute_button_t *array = this->array;

    if (array[curr].jumpto != 0)
        manager_setRootMenu(array[curr].jumpto);

    if (this->clickCallback)
        this->clickCallback(this);
}

static void combobutton_left(combobutton_t *self) {
    self->curr--;
    if (self->curr < 0)
        self->curr = self->max_num - 1;
    if (self->dataChangeCallback)
        self->dataChangeCallback(self);
}

static void combobutton_right(combobutton_t *self) {
    self->curr++;
    if (self->curr > self->max_num - 1)
        self->curr = 0;
    if (self->dataChangeCallback)
        self->dataChangeCallback(self);
}

void combobutton_setValuePos(combobutton_t *self, int val) {
    self->val_pos = val;
}

void combobutton_setMaxNum(combobutton_t *self, int val) {
    self->max_num = val;
}

void combobutton_setCurr(combobutton_t *self, int curr) {
    self->curr = curr;
}

int combobutton_getCurr(combobutton_t *self) {
    return self->curr;
}

void combobutton_registerGetValueCallback(combobutton_t *self, fGetValueCallback cb) {
    self->getValueCallback = cb;
}

void combobutton_registerDataChangeCallback(combobutton_t *self, fDataChangeCallback cb) {
    self->dataChangeCallback = cb;
}

void combobutton_init(combobutton_t *self) {
    obj_init(&self->base);

    self->val_pos = 0;
    self->curr    = 0;
    self->max_num = 0;

    self->getValueCallback   = 0;
    self->dataChangeCallback = 0;

    combobutton_method_init(self);
}


void combobutton_setArray(combobutton_t *self, int *type, osd_obj_t *(*jumpto), atrribute_button_t *array, int max_num) {
    int i;
    for(i=0; i<max_num; i++) {
        array[i].jumpto = jumpto[i];
        array[i].type = type[i];
    }
    self->array = array;
}
