#include "dirty_interface.h"

osd_para_ext_t para_g;

int getOutputResolutionX() {
    return para_g.pic_size.u32Width;
}

int getOutputResolutionY() {
    return para_g.pic_size.u32Height;
}

int getAutoExitTimer(void) {
    int auto_exit_timer = para_g.auto_exit_timer;
    if(auto_exit_timer == 0)
        auto_exit_timer = 0x7fffffff;
    return auto_exit_timer;
}

int fh_strlen(const char *s) {
    const char *sc;

    for (sc = s; *sc != '\0'; ++sc) /* nothing */
        ;

    return sc - s;
}

