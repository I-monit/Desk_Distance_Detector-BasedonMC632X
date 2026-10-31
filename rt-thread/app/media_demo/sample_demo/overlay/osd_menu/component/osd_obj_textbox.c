/**
 * textbox
 */
#include "osd_obj_textbox.h"
#include "osd_obj_common.h"
#include "../mod/user_interface.h"
#include "../top/dirty_interface.h"

static void textbox_registerMethod(textbox_t *this);
static void obj_textbox_draw(osd_obj_t *obj);
static void textbox_texttoString(char *text, int len, char *string);

void textbox_init(textbox_t *this) {
    obj_init(&this->base);
    textbox_registerMethod(this);
    this->text = 0;
    this->mode = 0;
}

void textbox_setString(textbox_t *this, char *text) {
    this->text = text;
    textbox_texttoString(this->text, fh_strlen(this->text) + 1, this->string);
}

void textbox_setMode(textbox_t *this, int mode) {
    this->mode = mode;
}

static void textbox_registerMethod(textbox_t *this) {
    method_draw_register(&this->base, obj_textbox_draw);
}


static void obj_textbox_draw(osd_obj_t *obj) {
    textbox_t *this = to_textbox(obj);
    if(this->mode >= 2){
        printf("invalid mode!\n");
    };
    if (this->mode == TEXT_DEFAULT) {
        obj_set_title_string(obj);
    } else if (this->mode == TEXT_CHANGE) {
        string_clean();
        string_set_with_offset(0, this->string, fh_strlen(this->string));
    }
    obj_osd_draw(obj);
}

static void textbox_texttoString(char *text, int len, char *string) {
    memcpy(string, text, len);
    return;
}
