
#ifndef _OSD_OBJ_COMBOBOX_H_
#define _OSD_OBJ_COMBOBOX_H_

#include "dsp/fh_common.h"
#include "component_base.h"
#include "../core/osd_obj.h"

typedef struct _combobox {
    osd_obj_t base;


    FH_UINT8 val_pos;  // value 位置
    int    curr;     // 当前选项索引
    FH_UINT8 max_num;  // 选项数量

    fGetValueCallback   getValueCallback;
    fDataChangeCallback dataChangeCallback;
} combobox_t;

#define to_combobox(x) container_of((x), combobox_t, base)

void combobox_setValuePos(combobox_t *this, int val);
void combobox_setMaxNum(combobox_t *this, int val);
void combobox_setCurr(combobox_t *this, int curr);
int  combobox_getCurr(combobox_t *this);
void combobox_registerGetValueCallback(combobox_t *this, fGetValueCallback cb);
void combobox_registerDataChangeCallback(combobox_t *this, fDataChangeCallback cb);
void combobox_init(combobox_t *this);

#endif /*_OSD_OBJ_COMBOBOX_H_*/
