#ifndef _OSD_MANAGER_H_
#define _OSD_MANAGER_H_

#include "osd_obj.h"


void manager_initialize();
int  manager_needsRefresh();

void manager_setRootMenu(osd_obj_t *menu);
void *manager_getRootMenu();
void manager_setRefresh(int refresh);
void manager_doOperation(FH_UINT8 event);
void manager_drawMenu();

#endif
