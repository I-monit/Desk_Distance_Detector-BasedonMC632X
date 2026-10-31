#include "osd_obj_scrollbar.h"
#include "../mock/key_event.h"
#include "osd_obj_common.h"
#include "../mod/user_interface.h"
#include "../top/dirty_interface.h"

static void scrollbar_left(scrollbar_t *this);
static void scrollbar_right(scrollbar_t *this);
static void obj_op_scrollbar(osd_obj_t *obj, FH_UINT8 event);

static void draw_value_type_0(scrollbar_t *this, int value) {
    char *string;
    char buf[SCROLLBAR_MAX_LEN];
    char num_str[NUM_MAX_LEN];
    int  len;
    int  startpos = this->val_pos;
    int  i;

    for (i = 0; i < SCROLLBAR_MAX_LEN; i++) {
        string = " ";
        buf[i] = string[0];
    }

    string = "<";
    buf[0] = string[0];

    sprintf(num_str, "%d", value);
    len = fh_strlen(num_str);
    printf("value:%d, num_str:%s, len %d\n", value, num_str, len);
    memcpy(&buf[1], num_str, len);
    string = ">";
    buf[len+1] = string[0];

    string_set_with_offset(startpos, buf, len + 2);
}

static void obj_scrollbar_draw(osd_obj_t *obj) {
    scrollbar_t *this = to_scrollbar(obj);

    // draw title
    obj_set_title_string(obj);

    // get value
    if (this->getValueCallback)
        this->getValueCallback(this);  // get scrollbar cur num

    // draw value
    draw_value_type_0(this, this->cur);

    obj_osd_draw(obj);
}



static void scrollbar_method_init(scrollbar_t *this) {
    method_op_register(&this->base, obj_op_scrollbar);
    method_draw_register(&this->base, obj_scrollbar_draw);
}

static void obj_op_scrollbar(osd_obj_t *obj, FH_UINT8 event) {
    scrollbar_t *c = to_scrollbar(obj);
    if (obj_isFocused(obj)){
        switch (event) {
            case EVENT_UP_CLICK:
                obj_focus_pre(obj);
                break;
            case EVENT_DOWN_CLICK:
                obj_focus_next(obj);
                break;
            case EVENT_LEFT_CLICK:
                scrollbar_left(c);
                break;
            case EVENT_RIGHT_CLICK:
                scrollbar_right(c);
                break;
            default:
                break;
        }
    }
}

static void scrollbar_left(scrollbar_t *this) {
    if (this->cur >= (this->step + this->min))
        this->cur = this->cur - this->step;
    else
        this->cur = this->max + (this->cur - this->min) - this->step + 1;

    if (this->dataChangeCallback)
        this->dataChangeCallback(this);
}

static void scrollbar_right(scrollbar_t *this) {
    if (this->cur <= (this->max - this->step))
        this->cur = this->cur + this->step;
    else
        this->cur = this->min + (this->max - this->cur) + this->step - 1;

    if (this->dataChangeCallback)
        this->dataChangeCallback(this);
}

void scrollbar_setValuePos(scrollbar_t *this, int val) {
    this->val_pos = val;
}

void scrollbar_setMin(scrollbar_t *this, int val) {
    this->min = val;
}

int scrollbar_getMin(scrollbar_t *this) {
    return this->min;
}

void scrollbar_setMax(scrollbar_t *this, int val) {
    this->max = val;
}

int scrollbar_getMax(scrollbar_t *this) {
    return this->max;
}

void scrollbar_setCurr(scrollbar_t *this, int val) {
    this->cur = val;
}

int scrollbar_getCurr(scrollbar_t *this) {
    return this->cur;
}

void scrollbar_setStep(scrollbar_t *this, int val) {
    this->step = val;
}

void scrollbar_registerGetValueCallback(scrollbar_t *this, fGetValueCallback cb) {
    this->getValueCallback = cb;
}

void scrollbar_registerDataChangeCallback(scrollbar_t *this, fDataChangeCallback cb) {
    this->dataChangeCallback = cb;
}

void scrollbar_init(scrollbar_t *this) {
    obj_init(&this->base);
    this->val_pos = 0;
    this->min     = 0;
    this->max     = 0;
    this->cur     = 0;
    this->step    = 0;

    this->getValueCallback   = 0;
    this->dataChangeCallback = 0;

    scrollbar_method_init(this);
}
