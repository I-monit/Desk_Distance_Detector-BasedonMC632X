#ifndef _OSD_SCREEN_H_
#define _OSD_SCREEN_H_
#include "dsp/fh_common.h"
#include "osd_queue.h"
#include "../mod/user_font.h"

typedef struct _osd_screen {
  FH_UINT8 width;
  FH_UINT8 height;
  font_config_t font_config;
  OSD_QUEUE osd_id_queue;
} screen_t;

void screen_setWidth(int width);
void screen_setHeight(int height);
void screen_initilize();
void screen_clean();
void screen_queue_put(FH_UINT32 *ele);

#endif  // !_OSD_SCREEN_H_