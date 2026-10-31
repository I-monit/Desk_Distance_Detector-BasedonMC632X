
#ifndef _OSD_OBJ_SCROLLBAR_H_
#define _OSD_OBJ_SCROLLBAR_H_

#include "dsp/fh_common.h"
#include "component_base.h"
#include "../core/osd_obj.h"

/**
 * scrollbar type 0
 *      TITLE         ◀ VAL ▶
 */

#define SCROLLBAR_MAX_LEN 32
#define NUM_MAX_LEN 10

struct _scrollbar;

typedef void (*scrollbar_cur_get)(struct _scrollbar *scrollbar);
// typedef void (*fDataChangeCallback)(struct _scrollbar *scrollbar);

typedef struct _scrollbar {
    osd_obj_t base;

    FH_UINT8 val_pos;  // value 位置

    FH_UINT8 min;  // 寄存器最小值
    FH_UINT8 max;  //寄存器最大值
    FH_SINT8 cur;  // 寄存器当前值
    FH_UINT8 step;

    fGetValueCallback   getValueCallback;
    fDataChangeCallback dataChangeCallback;
} scrollbar_t;

#define to_scrollbar(x) container_of((x), scrollbar_t, base)

void scrollbar_setValuePos(scrollbar_t *self, int val);
void scrollbar_setMin(scrollbar_t *self, int val);
int scrollbar_getMin(scrollbar_t *self);
void scrollbar_setMax(scrollbar_t *self, int val);
int scrollbar_getMax(scrollbar_t *self);
void scrollbar_setCurr(scrollbar_t *self, int val);
int scrollbar_getCurr(scrollbar_t *self);
void scrollbar_setStep(scrollbar_t *self, int val);
void scrollbar_registerGetValueCallback(scrollbar_t *self, fGetValueCallback cb);
void scrollbar_registerDataChangeCallback(scrollbar_t *self, fDataChangeCallback cb);
void scrollbar_init(scrollbar_t *self);

#endif /*_OSD_OBJ_SCROLLBAR_H_*/
