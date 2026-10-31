#ifndef _OSD_QUEUE_H_
#define _OSD_QUEUE_H_
#include "dsp/fh_common.h"

#define MAX_OSD_NUM_ON_SCREEM 128

typedef struct{
   FH_UINT32 queue_cnt;
   FH_UINT32 queue_ele_list[MAX_OSD_NUM_ON_SCREEM];
}OSD_QUEUE;

FH_SINT32 osd_queue_clear(OSD_QUEUE *queue);
FH_SINT32 osd_queue_query(OSD_QUEUE *queue, FH_UINT32 *ele);
FH_SINT32 osd_queue_get(OSD_QUEUE *queue, FH_UINT32 *ele);
FH_SINT32 osd_queue_put(OSD_QUEUE *queue, FH_UINT32 *ele);
#endif  // !_OSD_QUEUE_H_
