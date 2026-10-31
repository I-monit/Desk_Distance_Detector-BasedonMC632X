
#ifndef _OSD_OBJ_TEXTBOX_H_
#define _OSD_OBJ_TEXTBOX_H_

#include "dsp/fh_common.h"
#include "../core/osd_obj.h"

enum TEXT_TYPE { TEXT_DEFAULT = 0, TEXT_CHANGE = 1 };

typedef struct _textbox {
    osd_obj_t base;

    char *text;
    char  string[32];
    FH_UINT8 mode;
} textbox_t;

#define to_textbox(x) container_of((x), textbox_t, base)

void textbox_init(textbox_t *this);
void textbox_setString(textbox_t *this, char *string);
void textbox_setMode(textbox_t *this, int mode);

#endif /*_OSD_OBJ_TEXTBOX_H_*/
