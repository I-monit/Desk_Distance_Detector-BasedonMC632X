#include "osd_obj_menu.h"
#include "osd_obj_common.h"
#include "../core/osd_log.h"
#include "../core/osd_screen.h"
#include "../top/osd_event.h"
#include "../core/osd_obj.h"
#include "../top/dirty_interface.h"

static void menu_reorderChildren(menu_t *this) {

    osd_obj_t *child;

    if (!obj_hasChild(&this->base))
        return;

    // get head child
    child = obj_getHead(obj_getChild(&this->base));

    // for each child, do draw
    int y = 1;
    while (1) {
        if (obj_isEnable(child) && obj_isVisible(child))
            obj_setOffsetY(child, y++);

        if (obj_isTail(child))
            break;

        child = obj_getNext(child);
    }
}

static void menu_draw_children(menu_t *m) {
    osd_obj_t *child;
    if (!obj_hasChild(&m->base))
        return;
    // get head child
    child = obj_getHead(obj_getChild(&m->base));
    // for each child, do draw
    while (1) {
        if (obj_isEnable(child) && obj_isVisible(child))
            method_draw(child);
        if (obj_isTail(child)){
            osd_menu_draw();
            break;
        }
        child = obj_getNext(child);
    }
}

static void obj_menu_draw(osd_obj_t *obj) {
    menu_t *self = to_menu(obj);

    OSD_LOG("[draw] menuID: %d\n", self->id);

    if (self->drawCallback)
        self->drawCallback(self);
    set_draw_menu_id(self->id);
    set_draw_menu_offset(self->offset_x, self->offset_y);
    if (obj_isEnable(obj) && obj_isVisible(obj)) {
        obj_set_title_string(obj);
        obj_osd_draw(obj);
        if (self->autoOrder){
            menu_reorderChildren(self);
        }
        menu_draw_children(self);
    }
}

static void obj_menu_op(osd_obj_t *obj, FH_UINT8 event) {
    //	obj_event_handle(obj);
    // get the focused child obj , and op
    osd_obj_t *focused;
    osd_obj_t *child;
    osd_obj_t *selectable;

    if (obj_hasChild(obj)) {
        child   = obj_getChild(obj);
        focused = find_focused(child);
        if (focused == 0) {
            selectable = find_selectable(child);
            if (selectable) {
                OSD_LOG("[op] ID1: %d\n", obj_getId(selectable));
                obj_setFocused(selectable, 1);
                method_op(selectable, event);
            } else {
                // OSD_LOG("[op]No obj to handle event: %d\n", osdEvent_getOutput());
            }
        } else {
            OSD_LOG("[op] ID2: %d\n", obj_getId(focused));
            method_op(focused, event);
        }
    }
}

static void menu_method_init(menu_t *this) {
    method_op_register(&this->base, obj_menu_op);
    method_draw_register(&this->base, obj_menu_draw);
}

void menu_setAutoOrder(menu_t *this, int autoOrder) {
    this->autoOrder = autoOrder;
}

void menu_setOffsetX(menu_t *this, FH_UINT8 x) {
    this->offset_x = x;
}

void menu_setOffsetY(menu_t *this, FH_UINT8 y) {
    this->offset_y = y;
}

void menu_setId(menu_t *this, int id) {
    this->id = id;
}

void menu_init(menu_t *this) {
    obj_init(&this->base);

    this->id        = 0;
    this->autoOrder = 0;

    menu_method_init(this);
}

void menu_registerDrawCallback(menu_t *self, fDrawCallback cb) {
    self->drawCallback = cb;
}
