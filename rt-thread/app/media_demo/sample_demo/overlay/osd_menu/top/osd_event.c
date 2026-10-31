/*****************************************************************************
*	Copyright (c) 2010-2017 Shanghai Fullhan Microelectronics Co., Ltd.
*						All Rights Reserved. Confidential.
******************************************************************************/

#include "osd_event.h"

event_cfg_t event_cfg_g;

void osdEvent_setEvent(event_cfg_t *cfg) {
    event_cfg_g.curr_event = cfg->curr_event;
}

enum EVENT_OUT osdEvent_getEvent() {
    return event_cfg_g.curr_event;
}

