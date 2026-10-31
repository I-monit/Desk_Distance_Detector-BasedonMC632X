#ifndef _FH_OSAL_EVENT_H
#define _FH_OSAL_EVENT_H

typedef struct fh_osal_event{
	void *event;
}fh_osal_event_t;

int fh_osal_event_init(fh_osal_event_t *event, const char *name, rt_uint8_t flag);
int fh_osal_event_destory(fh_osal_event_t *event);
int fh_osal_event_send(fh_osal_event_t *event, rt_uint32_t set);
int fh_osal_event_recv(fh_osal_event_t *event, rt_uint32_t set, rt_uint8_t option, rt_int32_t timeout, rt_uint32_t *recved);



#endif