
#include "stdio.h"
#include "osd_queue.h"
#include "dsp/fh_common.h"

FH_SINT32 osd_queue_clear(OSD_QUEUE *queue){
    queue->queue_cnt = 0;
    return RETURN_OK;
}


FH_SINT32 osd_queue_query(OSD_QUEUE *queue, FH_UINT32 *ele){
    if(queue->queue_cnt > 0){
        ele = &queue->queue_ele_list[queue->queue_cnt-1];
        return RETURN_OK;
    }
    return -1;
}

FH_SINT32 osd_queue_get(OSD_QUEUE *queue, FH_UINT32 *ele){
    if(queue->queue_cnt > 0){
        *ele = queue->queue_ele_list[--queue->queue_cnt];
        // printf("queue_get, clean ele:%d, queue_cnt:%d ## \n", ele, queue->queue_cnt);
        return RETURN_OK;
    }
    return -1;
}

FH_SINT32 osd_queue_put(OSD_QUEUE *queue, FH_UINT32 *ele){
    if(queue->queue_cnt > MAX_OSD_NUM_ON_SCREEM){
        printf("osd queue full\n");
        return -1;
    }
    queue->queue_ele_list[queue->queue_cnt++] = *ele;
    // printf("queue_put, queue_cnt:%d, ele:%d $$ ", queue->queue_cnt - 1, queue->queue_ele_list[queue->queue_cnt - 1]);
    return RETURN_OK;
}
