
#include "string.h"
#include "stdio.h"
#include "../top/dirty_interface.h"
#include "osd_draw.h"
#include "../mod/user_string.h"
#include "osd_screen.h"
#include "osd_log.h"

OSD_DRAW osd_draw_g;

void string_clean(){
    memset(&osd_draw_g.osd_string, 0, MAX_CHAR_NUM_PER_OSD);
    osd_draw_g.osd_curr_char_num = 0;
}

void string_set_with_offset(int offset, char *string, int len) {
    char *string_p;
    char *string_space;
    FH_SINT32 space_num;
    // printf("offset:%d, len:%d, osd_curr_char_num:%d, string:%s\n", offset, len, osd_draw_g.osd_curr_char_num, string);
    if((osd_draw_g.osd_curr_char_num + len) > MAX_CHAR_NUM_PER_OSD){
        printf("string len overflow!\n");
        return;
    }
    if(offset > MAX_CHAR_NUM_PER_OSD){
        printf("offset len overflow!\n");
        return;
    }

    string_p = osd_draw_g.osd_string + osd_draw_g.osd_curr_char_num;
    space_num = offset - osd_draw_g.osd_curr_char_num;
    if((space_num) < 0){
        printf("offset too small!\n");
    }else if((space_num) > 0){
        string_space = " ";
        memset(string_p, string_space[0], space_num);
    }
    string_p += space_num;
    osd_draw_g.osd_curr_char_num += space_num;
    memcpy(string_p, string, len);
    osd_draw_g.osd_curr_char_num += len;
}

void osd_draw_with_gosd(FH_UINT32 component_offset_x, FH_UINT32 component_offset_y, FH_UINT8 component_id){
    FH_UINT32 gosd_id, draw_offset_x, draw_offset_y ;
    gosd_id = (osd_draw_g.menu_id << 8) | component_id;
    draw_offset_x = osd_draw_g.menu_offset_x + component_offset_x;
    draw_offset_y = osd_draw_g.menu_offset_y + component_offset_y;

    // screen_queue_put(&gosd_id);
    OSD_LOG("menu_id:%d, component_id:%d, ", osd_draw_g.menu_id, component_id);
    OSD_LOG("menu_offset_x:%d, component_offset_x:%d\n", osd_draw_g.menu_offset_x, component_offset_x);
    OSD_LOG("osd_menu_config, draw_offset_x:%d, draw_offset_y:%d, gosd_id:%d, highlight_en:%d, osd_string:%s\n\n", draw_offset_x, draw_offset_y, gosd_id, highlight_en, osd_draw_g.osd_string);

    osd_menu_config(draw_offset_x*(g_osd_para.osd_font_size/2), draw_offset_y*(g_osd_para.osd_font_size + g_osd_para.row_space), gosd_id, osd_draw_g.osd_string);
}

void set_draw_menu_offset(FH_UINT8 menu_offset_x, FH_UINT8 menu_offset_y){
    osd_draw_g.menu_offset_x = menu_offset_x;
    osd_draw_g.menu_offset_y = menu_offset_y;
}

void set_draw_menu_id(FH_UINT8 menu_id){
    osd_draw_g.menu_id = menu_id;
}

char *getString(int id)
{
    char *string_menu_base = osd_string_index[osd_draw_g.menu_id];
    char *string = string_menu_base + MAX_STR_LEN * id;
    return string;
}