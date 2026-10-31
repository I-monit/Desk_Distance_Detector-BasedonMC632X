#include "osd_manager.h"
#include "osd_obj.h"
#include "../component/osd_obj_menu.h"
#include "osd_screen.h"
#include "../mod/user_interface.h"

typedef struct _osd_manager {
    osd_obj_t *   root_menu;
    FH_UINT8         menu_id;
    FH_UINT8 refresh;  // by wuzh
    FH_UINT8         reload;

} osd_manager_t;

osd_manager_t osd_manager;

void manager_initialize() {
    osd_manager_t *this = &osd_manager;

    this->root_menu = 0;
    this->refresh   = 0;
    this->reload    = 0;
}

int manager_needsRefresh() {
    osd_manager_t *this = &osd_manager;
    return this->refresh;
}

void manager_setRootMenu(osd_obj_t *menu) {
    osd_manager_t *this = &osd_manager;

    if (this->root_menu != menu) {
        this->root_menu = menu;
        this->reload    = 1;
        this->menu_id   = to_menu(menu)->id;
    }
}

void *manager_getRootMenu() {
    osd_manager_t *self = &osd_manager;
    return self->root_menu;
}

void manager_setRefresh(int refresh) {
    osd_manager.refresh = refresh;
}

void manager_doOperation(FH_UINT8 event) {
    osd_manager_t *this = &osd_manager;
    method_op(this->root_menu, event);
}

void manager_drawMenu() {
    osd_manager_t *this = &osd_manager;
    method_draw(this->root_menu);
}

