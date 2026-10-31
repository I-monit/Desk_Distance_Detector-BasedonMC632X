#ifndef _FH_OSAL_COMPLETION_H
#define _FH_OSAL_COMPLETION_H

typedef struct fh_osal_completion {
	void *completion;
}fh_osal_completion_t;

int fh_osal_init_completion(fh_osal_completion_t *completion);
int fh_osal_reinit_completion(fh_osal_completion_t *completion);
int fh_osal_complete(fh_osal_completion_t *completion);
int fh_osal_complete_all(fh_osal_completion_t *completion);
void fh_osal_completion_destroy(fh_osal_completion_t *completion);
long fh_osal_wait_for_completion_interruptible_timeout(fh_osal_completion_t *completion,
													unsigned long timeout_ms);
long fh_osal_wait_for_completion_timeout(fh_osal_completion_t *completion,
													unsigned long timeout_ms);
#endif