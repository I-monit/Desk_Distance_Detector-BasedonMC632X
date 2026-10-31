#include "user_interface.h"
#include "../mod/menu/menu_list.h"
#include "menu/configure.h"

void init_defaultConf() {
    defaultConf.screenPosX = 0;
    defaultConf.screenPosY = 0;
}

osd_obj_t *getSleepMenu()
{
    return &menuSleep.base;
}

osd_obj_t *getMainMenu()
{
    return &menuMain.base;
}

osd_obj_t *getInitMenu()
{
    return getSleepMenu();
}
