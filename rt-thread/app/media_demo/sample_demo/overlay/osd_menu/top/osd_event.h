/*****************************************************************************
*	Copyright (c) 2010-2017 Shanghai Fullhan Microelectronics Co., Ltd.
*						All Rights Reserved. Confidential.
******************************************************************************/
#ifndef _OSD_EVENT_H_
#define _OSD_EVENT_H_

#include "../mock/key_event.h"
#include "dsp/fh_common.h"

typedef struct _event_cfg {
    enum EVENT_OUT curr_event;
} event_cfg_t;

void osdEvent_initialize(event_cfg_t *cfg);
// enum EVENT_OUT osdEvent_getOutput();

#endif  // _OSD_EVENT_H_
