#include "user_font.h"

void set_default_font_config(font_config_t *font_cfg){
    font_cfg->font_zise.u32Width = 16;
    font_cfg->font_zise.u32Height = 32;
}

int getCharCountInRow(int pixelsInRow, font_config_t *font_cfg) {
    int charCountInRow;
    charCountInRow = pixelsInRow / font_cfg->font_zise.u32Width;
    return charCountInRow;
}

int getCharCountInColumn(int pixelsInColumn, font_config_t *font_cfg) {
    int charCountInColumn;
    charCountInColumn = pixelsInColumn / font_cfg->font_zise.u32Height;
    return charCountInColumn;
}


