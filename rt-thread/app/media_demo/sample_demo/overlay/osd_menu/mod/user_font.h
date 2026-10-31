#ifndef _USER_FONT_H_
#define _USER_FONT_H_
#include "dsp/fh_common.h"

typedef struct _font_config {
    FH_SIZE font_zise;
} font_config_t;

void set_default_font_config(font_config_t *font_cfg);
int getCharCountInRow(int pixelsInRow, font_config_t *font_cfg);
int getCharCountInColumn(int pixelsInColumn, font_config_t *font_cfg);

#endif