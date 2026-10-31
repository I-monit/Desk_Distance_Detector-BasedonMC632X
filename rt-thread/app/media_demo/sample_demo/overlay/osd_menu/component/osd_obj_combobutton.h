
#ifndef _OSD_OBJ_COMBOBUTTON_H_
#define _OSD_OBJ_COMBOBUTTON_H_

#include "dsp/fh_common.h"
#include "component_base.h"
#include "../core/osd_obj.h"
#include "osd_obj_button.h"


typedef struct _atrribute_button {
    FH_UINT8     type;     // 按钮类型
    osd_obj_t *jumpto;
} atrribute_button_t;

typedef struct _combobutton {
    osd_obj_t base;
    int curr;
    atrribute_button_t    *array;     // 当前选项索引
    FH_SINT8 val_pos;  // value 位置
    FH_UINT8 max_num;  // 选项数量

    fGetValueCallback   getValueCallback;
    fDataChangeCallback dataChangeCallback;
    fClickCallback clickCallback;
} combobutton_t;


#define   to_combobutton(x) container_of((x), combobutton_t, base)

void combobutton_setValuePos(combobutton_t *this, int val);
void combobutton_setMaxNum(combobutton_t *this, int val);
void combobutton_setCurr(combobutton_t *this, int curr);
int  combobutton_getCurr(combobutton_t *this);
void combobutton_registerGetValueCallback(combobutton_t *this, fGetValueCallback cb);
void combobutton_registerDataChangeCallback(combobutton_t *this, fDataChangeCallback cb);
void combobutton_init(combobutton_t *this);
void combobutton_setArray(combobutton_t *self, int *type, osd_obj_t *(*jumpto), atrribute_button_t *array, int max_num);

#endif /*_OSD_OBJ_COMBOBUTTON_H_*/
