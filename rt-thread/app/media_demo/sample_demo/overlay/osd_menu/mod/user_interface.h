#ifndef _USER_INTERFACE_H_
#define _USER_INTERFACE_H_

// #define ARABEN

#include "../core/osd_obj.h"

void init_defaultConf();
osd_obj_t *getSleepMenu();
osd_obj_t *getMainMenu();
osd_obj_t *getInitMenu();

#endif  // !_USER_INTERFACE_H_
