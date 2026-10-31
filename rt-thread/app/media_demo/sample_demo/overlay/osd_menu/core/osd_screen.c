#include "osd_screen.h"
#include "../mod/user_interface.h"
#include "../top/dirty_interface.h"
#include "osd_queue.h"

static screen_t osdScreen;

void screen_setWidth(int width) {
    screen_t *this = &osdScreen;
    this->width = width;
}

void screen_setHeight(int height) {
    screen_t *this = &osdScreen;
    this->height = height;
}

void screen_initilize() {
    screen_t *this = &osdScreen;
    // this->width   = getCharCountInRow(getOutputResolutionX(), &osdScreen.font_config);
    // this->height  = getCharCountInColumn(getOutputResolutionY(), &osdScreen.font_config);
    set_default_font_config(&osdScreen.font_config);
    osd_queue_clear(&this->osd_id_queue);
}

#define CLEAN_ID 255    //gosd clean整个幅面时要求传入255
void screen_clean() {
    osd_menu_clean(CLEAN_ID);
}

void screen_queue_put(FH_UINT32 *ele){
    screen_t *this = &osdScreen;
    osd_queue_put(&this->osd_id_queue, ele);
}
