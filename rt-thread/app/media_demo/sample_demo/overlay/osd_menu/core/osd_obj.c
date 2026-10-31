#include "stdio.h"
#include "osd_obj.h"
#include "../top/osd_event.h"
#include "osd_log.h"
/** llist operation */
osd_obj_t *obj_getNext(osd_obj_t *this) {
    if (node_isTail(&this->node))
        return OBJ_NULL;
    else
        return to_osd_obj(node_getNext(&this->node));
}

osd_obj_t *obj_getPrev(osd_obj_t *this) {
    if (node_isHead(&this->node))
        return OBJ_NULL;
    else
        return to_osd_obj(node_getPrev(&this->node));
}

osd_obj_t *obj_getChild(osd_obj_t *this) {
    if (node_hasChild(&this->node))
        return to_osd_obj(node_getChild(&this->node));
    else
        return OBJ_NULL;
}

osd_obj_t *obj_getParent(osd_obj_t *this) {
    if (node_getParent(&this->node))
        return to_osd_obj(node_getParent(&this->node));
    else
        return OBJ_NULL;
}

void obj_setNext(osd_obj_t *this, osd_obj_t *next) {
    return node_setNext(&this->node, &next->node);
}

void obj_setPrev(osd_obj_t *this, osd_obj_t *prev) {
    return node_setPrev(&this->node, &prev->node);
}

void obj_setChild(osd_obj_t *this, osd_obj_t *child) {
    return node_setChild(&this->node, &child->node);
}
void obj_setParent(osd_obj_t *this, osd_obj_t *parent) {
    return node_setParent(&this->node, &parent->node);
}

int obj_isHead(osd_obj_t *this) {
    return node_isHead(&this->node);
}

int obj_isTail(osd_obj_t *this) {
    return node_isTail(&this->node);
}

int obj_hasChild(osd_obj_t *this) {
    return node_hasChild(&this->node);
}

osd_obj_t *obj_getHead(osd_obj_t *this) {
    if (node_getHead(&this->node))
        return to_osd_obj(node_getHead(&this->node));
    else
        return OBJ_NULL;
}

osd_obj_t *obj_getTail(osd_obj_t *this) {
    if (node_getTail(&this->node))
        return to_osd_obj(node_getTail(&this->node));
    else
        return OBJ_NULL;
}

void obj_append(osd_obj_t *this, osd_obj_t *next) {
    return node_append(&this->node, &next->node);
}

void obj_insert(osd_obj_t *this, osd_obj_t *prev) {
    return node_insert(&this->node, &prev->node);
}

void obj_adopt(osd_obj_t *this, osd_obj_t *child) {
    return node_adopt(&this->node, &child->node);
}

void obj_delete(osd_obj_t *this) {
    return node_delete(&this->node);
}

//////////////////////////////////////////////////focus
void focus_reset(osd_obj_t *this) {
    osd_obj_t *c;
    c = find_focused(this);
    if (c)
        obj_setFocused(c, 0);
}

void obj_focus_next(osd_obj_t *this) {
    osd_obj_t *c = this;
    FH_UINT16     count;

    obj_setFocused(this, 0);
    for (count = 0; count < OBJ_MAX_NUM; count++) {
        if (obj_isTail(c))
            c = obj_getHead(c);
        else
            c = obj_getNext(c);

        if (obj_isEnable(c) && obj_isSelectable(c)) {
            obj_setFocused(c, 1);
            break;
        }
    }
}
void obj_focus_index(osd_obj_t *this, FH_UINT16 index_in) {
    FH_UINT16     cnt;
    osd_obj_t *c = this;

    for (cnt = 0; cnt < index_in; cnt++) {
        if (obj_isTail(c) == 0)  // not foot
        {
            c = obj_getNext(c);
            obj_focus_next(c);
        } else
            break;
    }
}
void obj_focus_pre(osd_obj_t *this) {
    osd_obj_t *c = this;
    FH_UINT16     count;

    obj_setFocused(this, 0);
    for (count = 0; count < OBJ_MAX_NUM; count++) {
        if (obj_isHead(c))
            c = obj_getTail(c);
        else
            c = obj_getPrev(c);

        if (obj_isEnable(c) && obj_isSelectable(c)) {
            obj_setFocused(c, 1);
            break;
        }
    }
}

osd_obj_t *find_focused(osd_obj_t *this)  // get focused ,if no fucused return 0
{
    osd_obj_t *c;
    FH_UINT16     count;
    c = obj_getHead(this);
    for (count = 0; count < OBJ_MAX_NUM; count++) {
        if ((obj_isSelectable(c) == 1) && (obj_isFocused(c) == 1)) {
            return c;
        }
        if (obj_isTail(c)) {
            // obj_setFocused(obj_getHead(this),1);
            return 0;
        } else {
            c = obj_getNext(c);
        }
    }
    return OBJ_NULL;
}
osd_obj_t *find_selectable(osd_obj_t *this)  // get selectable ,if no selectable return 0
{
    osd_obj_t *c;
    FH_UINT16     count;
    c = obj_getHead(this);
    for (count = 0; count < OBJ_MAX_NUM; count++) {
        if (obj_isSelectable(c) == 1) {
            return c;
        }
        if (obj_isTail(c)) {
            // obj_setFocused(obj_getHead(this),1);
            return 0;
        } else {
            c = obj_getNext(c);
        }
    }
    return OBJ_NULL;
}
//////////////////////////////////////////////////obj value
// state
void obj_setEnable(osd_obj_t *this, FH_UINT8 en) {
    this->enable = en;
}
FH_UINT8 obj_isEnable(osd_obj_t *this) {
    return this->enable;
}
void obj_setId(osd_obj_t *this, FH_UINT8 id) {
    this->id = id;
}
FH_UINT8 obj_getId(osd_obj_t *this) {
    return this->id;
}
void obj_setOffsetX(osd_obj_t *this, FH_UINT8 x) {
    this->offset_x = x;
}
FH_UINT8 obj_getOffsetX(osd_obj_t *this) {
    return this->offset_x;
}
void obj_setOffsetY(osd_obj_t *this, FH_UINT8 y) {
    this->offset_y = y;
}
FH_UINT8 obj_getOffsetY(osd_obj_t *this) {
    return this->offset_y;
}
void obj_setSelectable(osd_obj_t *this, FH_UINT8 selectable) {
    this->selectable = selectable;
}
FH_UINT8 obj_isSelectable(osd_obj_t *this) {
    return this->selectable;
}
void obj_setVisible(osd_obj_t *this, FH_UINT8 visiable) {
    this->visible = visiable;
}
FH_UINT8 obj_isVisible(osd_obj_t *this) {
    return this->visible;
}
void obj_setFocused(osd_obj_t *this, FH_UINT8 focused) {
    this->focused = focused;
}
FH_UINT8 obj_isFocused(osd_obj_t *this) {
    return this->focused;
}

/////////////////////////////method
void method_op(osd_obj_t *this, FH_UINT8 event) {
    if (this->obj_method.op)
        this->obj_method.op(this, event);
    else
        printf("obj_method.op == NULL!\n");
}
void method_op_register(osd_obj_t *this, osd_op op) {
    this->obj_method.op = op;
}
void method_draw(osd_obj_t *this) {
    if (this->obj_method.draw){
        this->obj_method.draw(this);
    }
    else{
        printf("obj_method.draw == NULL!\n");
    }
}
void method_draw_register(osd_obj_t *this, osd_draw draw) {
    this->obj_method.draw = draw;
}

void obj_init(osd_obj_t *this) {
    node_init(&this->node);

    this->enable     = 1;
    this->id         = 0;
    this->offset_x   = 0;
    this->offset_y   = 0;
    this->selectable = 1;
    this->visible    = 1;
    this->focused    = 0;

    this->obj_method.draw                      = 0;
    this->obj_method.op                        = 0;
}
