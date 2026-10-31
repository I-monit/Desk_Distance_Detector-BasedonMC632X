#include "osd_obj_combobox.h"
#include "dsp/fh_common.h"
#include "../mock/key_event.h"
#include "osd_obj_common.h"
#include "../mod/user_interface.h"
#include "../top/dirty_interface.h"

#define COMBOBOX_WIDTH_MAX (32)

static void combobox_draw_value(combobox_t *self);
static void obj_combobox_draw(osd_obj_t *obj);
static void combobox_method_init(combobox_t *self);
static void combobox_left(combobox_t *self);
static void combobox_right(combobox_t *self);
static void obj_op_combobox(osd_obj_t *obj, FH_UINT8 event);

static void combobox_draw_value(combobox_t *self) {
    osd_obj_t *obj = &self->base;

    char *text;
    char *string;
    char  buf[COMBOBOX_WIDTH_MAX];
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

    // draw right arrow
    string = ">";
    buf[len++] = string[0];

    // fill screen
    string_set_with_offset(self->val_pos, buf, len);
}

static void obj_combobox_draw(osd_obj_t *obj) {
    combobox_t *c = to_combobox(obj);

    // draw text
    obj_set_title_string(obj);

    // get value
    if (c->getValueCallback)
        c->getValueCallback(c);  // get scrollbar cur num

    // draw value
    combobox_draw_value(c);

    obj_osd_draw(obj);
}



static void combobox_method_init(combobox_t *self) {
    method_op_register(&self->base, obj_op_combobox);
    method_draw_register(&self->base, obj_combobox_draw);
}

static void obj_op_combobox(osd_obj_t *obj, FH_UINT8 event){
    combobox_t *c = to_combobox(obj);
    if (obj_isFocused(obj)){
        switch (event) {
            case EVENT_UP_CLICK:
                obj_focus_pre(obj);
                break;
            case EVENT_DOWN_CLICK:
                obj_focus_next(obj);
                break;
            case EVENT_LEFT_CLICK:
                combobox_left(c);
                break;
            case EVENT_RIGHT_CLICK:
                combobox_right(c);
                break;
            default:
                break;
        }
    }
}

static void combobox_left(combobox_t *self) {
    self->curr--;
    if (self->curr < 0)
        self->curr = self->max_num - 1;
    if (self->dataChangeCallback)
        self->dataChangeCallback(self);
}

static void combobox_right(combobox_t *self) {
    self->curr++;
    if (self->curr > self->max_num - 1)
        self->curr = 0;
    if (self->dataChangeCallback)
        self->dataChangeCallback(self);
}

void combobox_setValuePos(combobox_t *self, int val) {
    self->val_pos = val;
}

void combobox_setMaxNum(combobox_t *self, int val) {
    self->max_num = val;
}

void combobox_setCurr(combobox_t *self, int curr) {
    self->curr = curr;
}

int combobox_getCurr(combobox_t *self) {
    return self->curr;
}

void combobox_registerGetValueCallback(combobox_t *self, fGetValueCallback cb) {
    self->getValueCallback = cb;
}

void combobox_registerDataChangeCallback(combobox_t *self, fDataChangeCallback cb) {
    self->dataChangeCallback = cb;
}

void combobox_init(combobox_t *self) {
    obj_init(&self->base);

    self->val_pos = 0;
    self->curr    = 0;
    self->max_num = 0;

    self->getValueCallback   = 0;
    self->dataChangeCallback = 0;

    combobox_method_init(self);
}
