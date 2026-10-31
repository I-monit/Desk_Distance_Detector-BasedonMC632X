#ifndef _FH_OSAL_TASKLET_H
#define _FH_OSAL_TASKLET_H

typedef struct fh_osal_tasklet_struct {
	void *tasklet_struct;
}fh_osal_tasklet_struct_t;

void fh_osal_tasklet_init(fh_osal_tasklet_struct_t *t,
				void (*func)(unsigned long), unsigned long data);
void fh_osal_tasklet_enable(fh_osal_tasklet_struct_t *t);
void fh_osal_tasklet_schedule(fh_osal_tasklet_struct_t *t);
void fh_osal_tasklet_hi_schedule(fh_osal_tasklet_struct_t *t);
void fh_osal_tasklet_kill(fh_osal_tasklet_struct_t *t);

#endif