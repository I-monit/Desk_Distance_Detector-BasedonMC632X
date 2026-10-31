#ifndef _OSD_OBJ_H_
#define _OSD_OBJ_H_

#include "dsp/fh_common.h"
#include "osd_llist.h"

struct _osd_obj;

#define OBJ_NULL (void *) 0
#define OBJ_MAX_NUM 0xffff

#define osd_offsetof(type, member) (unsigned int) &(((type *) 0)->member)
#define container_of(ptr, type, member) ((type *) (ptr) -osd_offsetof(type, member))

#define to_osd_obj(x) container_of((x), osd_obj_t, node)

typedef void (*osd_draw)(struct _osd_obj *obj);
typedef void (*osd_op)(struct _osd_obj *obj, FH_UINT8 event);
typedef void (*osd_event_op)(struct _osd_obj *obj);


typedef struct _obj_method {
    osd_draw draw;
    //	osd_status_get status_get; //get data/status for drawing
    osd_op     op;
} obj_method_t;

typedef struct _osd_obj {
    node_t node;  //继承 osd 链表结构

    // member
    FH_UINT8 enable;  // if disable, obj cannot be selected ,draw,&op and other BIZ
    FH_UINT8 id;
    FH_UINT8 offset_x;
    FH_UINT8 offset_y;
    FH_UINT8 selectable;  // if selectable = 0, obj cannot be focused
    FH_UINT8 visible;     // if visible = 0, obj cannot be draw
    FH_UINT8 focused;

    // method
    obj_method_t obj_method;
} osd_obj_t;

/** 链表操作 */
osd_obj_t *obj_getNext(osd_obj_t *obj);
osd_obj_t *obj_getPrev(osd_obj_t *obj);
osd_obj_t *obj_getChild(osd_obj_t *obj);
osd_obj_t *obj_getParent(osd_obj_t *obj);
void obj_setNext(osd_obj_t *obj, osd_obj_t *next);
void obj_setPrev(osd_obj_t *obj, osd_obj_t *pre);
void obj_setChild(osd_obj_t *obj, osd_obj_t *child);
void obj_setParent(osd_obj_t *obj, osd_obj_t *parent);

int obj_isHead(osd_obj_t *obj);
int obj_isTail(osd_obj_t *obj);
int obj_hasChild(osd_obj_t *obj);
osd_obj_t *obj_getHead(osd_obj_t *obj);
osd_obj_t *obj_getTail(osd_obj_t *obj);
void obj_append(osd_obj_t *obj, osd_obj_t *next);
void obj_insert(osd_obj_t *obj, osd_obj_t *prev);
void obj_adopt(osd_obj_t *obj, osd_obj_t *child);
void obj_delete(osd_obj_t *obj);

/** member access (成员变量访问) */
void obj_setEnable(osd_obj_t *obj, FH_UINT8 en);
void obj_setId(osd_obj_t *obj, FH_UINT8 id);
void obj_setOffsetX(osd_obj_t *obj, FH_UINT8 x);
void obj_setOffsetY(osd_obj_t *obj, FH_UINT8 y);
void obj_setSelectable(osd_obj_t *obj, FH_UINT8 selectable);
void obj_setVisible(osd_obj_t *obj, FH_UINT8 visible);
void obj_setFocused(osd_obj_t *obj, FH_UINT8 focused);

FH_UINT8 obj_isEnable(osd_obj_t *obj);
FH_UINT8 obj_getId(osd_obj_t *obj);
FH_UINT8 obj_getOffsetX(osd_obj_t *obj);
FH_UINT8 obj_getOffsetY(osd_obj_t *obj);
FH_UINT8 obj_isSelectable(osd_obj_t *obj);
FH_UINT8 obj_isVisible(osd_obj_t *obj);
FH_UINT8 obj_isFocused(osd_obj_t *obj);

/** 清除输入对象所在层的选中对象的 focused 标志 */
void focus_reset(osd_obj_t *cobj);
/** 聚焦到下一个对象 */
void obj_focus_next(osd_obj_t *cur);
/** 聚焦到上一个对象 */
void obj_focus_pre(osd_obj_t *cur);
/** 聚焦输入对象所在层的第n个对象 */
void obj_focus_index(osd_obj_t *cur_obj, FH_UINT16 n);
/** 返回输入对象所在层的聚焦对象 */
osd_obj_t *find_focused(osd_obj_t *cobj);
/** 返回输入对象所在层的第一个可选对象 */
osd_obj_t *find_selectable(osd_obj_t *cobj);

/** method */
void method_op(osd_obj_t *obj, FH_UINT8 event);
void method_draw(osd_obj_t *obj_op);
void obj_registerEventHandler(osd_obj_t *obj, osd_event_op event_op_in, FH_UINT8 event);
FH_UINT32 obj_registerEventHandler_check(osd_obj_t *obj, FH_UINT8 event);
void method_draw_register(osd_obj_t *obj_op, osd_draw draw);
void method_op_register(osd_obj_t *obj_op, osd_op op);

void obj_init(osd_obj_t *obj);

#endif /*_OSD_OBJ_H_*/
