#include "osd_menu_api.h"
#include "../core/osd_manager.h"
#include "../mod/menu/menu_sleep.h"

void setTextboxAeStatus(char *str) {
    textbox_setString(&textboxAeStatus, str);
    manager_setRefresh(1);
}
