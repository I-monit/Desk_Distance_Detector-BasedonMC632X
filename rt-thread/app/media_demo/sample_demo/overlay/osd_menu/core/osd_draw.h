#ifndef _OSD_DRAW_H_
#define _OSD_DRAW_H_

#include "osd_obj.h"

#define MAX_CHAR_NUM_PER_OSD 127
typedef struct {
  FH_UINT8 menu_offset_x;
  FH_UINT8 menu_offset_y;
  FH_UINT8 menu_id;
  FH_UINT32 osd_curr_char_num;
  FH_CHAR osd_string[MAX_CHAR_NUM_PER_OSD];
} OSD_DRAW;

void string_clean();
void string_set_with_offset(int offset, char *string, int len);
void osd_draw_with_gosd(FH_UINT32 component_offset_x, FH_UINT32 component_offset_y, FH_UINT8 component_id);
void set_draw_menu_offset(FH_UINT8 menu_offset_x, FH_UINT8 menu_offset_y);
void set_draw_menu_id(FH_UINT8 menu_id);
char *getString(int id);

#endif  // !_OSD_DRAW_H_