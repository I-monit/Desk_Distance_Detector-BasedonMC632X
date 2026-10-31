
#include "../core/osd_manager.h"
#include "../mod/user_interface.h"

void set_textbox(char *str) {
    // obj_textbox_set_text(&obj_zoom_dis.base, str);
}

int osd_isSleeping() {
    return manager_getRootMenu() == (void *) getSleepMenu();
}