#ifndef _FH_OSAL_TASK_H
#define _FH_OSAL_TASK_H

typedef struct fh_osal_task{
	void *task_struct;
}fh_osal_task_t;
typedef int (*threadfn_t)(void *data);
fh_osal_task_t *fh_osal_kthread_create(threadfn_t thread,void *data, char *name);
int fh_osal_kthread_should_stop(void);
void fh_osal_kthread_destroy(fh_osal_task_t *task);
void fh_osal_kthread_bind(fh_osal_task_t *task, unsigned int cpu);

pid_t fh_osal_task_get_pid(fh_osal_task_t *task);
void *fh_osal_task_get_mm(fh_osal_task_t *task);
pid_t fh_osal_task_get_tgid(fh_osal_task_t *task);
pid_t fh_osal_get_current_pid(void);
pid_t fh_osal_get_current_tid(void);
pid_t fh_osal_get_current_tgid(void);
char *fh_osal_get_current_taskname(void);

void fh_osal_yield(void);

#endif
