#ifndef _FH_OSAL_WORKQUEUE_H
#define _FH_OSAL_WORKQUEUE_H

struct wq_node{
	struct fh_osal_work_struct *fh_osal_work;
	struct work_struct *work;
	struct fh_osal_list_head node;
};

typedef struct fh_osal_work_struct {
	void *work;
	void (*func)(struct fh_osal_work_struct *work);
}fh_osal_work_struct_t;
typedef void (*fh_osal_work_func_t)(struct fh_osal_work_struct *work);

int fh_osal_init_work(struct fh_osal_work_struct *work, fh_osal_work_func_t func);
int fh_osal_schedule_work(struct fh_osal_work_struct *work);
void fh_osal_destroy_work(struct fh_osal_work_struct *work);

#endif