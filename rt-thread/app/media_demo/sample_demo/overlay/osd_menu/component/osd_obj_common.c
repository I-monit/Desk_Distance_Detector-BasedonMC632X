
#include "osd_obj_common.h"
#include "../mod/user_interface.h"
#include "../top/dirty_interface.h"

/** 叶节点通用处理 */
void obj_set_title_string(osd_obj_t *obj) {
    char *title;
    char *string;
    char head_lable;
    string_clean();
    if(obj_isFocused(obj)){
        string = "@";
    }else{
        string = " ";
    }
    head_lable = string[0];
    string_set_with_offset(0, &head_lable, 1);

    title = getString(obj_getId(obj));
    string_set_with_offset(1, title, fh_strlen(title));
}

void obj_osd_draw(osd_obj_t *obj){
    osd_draw_with_gosd(obj_getOffsetX(obj), obj_getOffsetY(obj), obj_getId(obj));
}
