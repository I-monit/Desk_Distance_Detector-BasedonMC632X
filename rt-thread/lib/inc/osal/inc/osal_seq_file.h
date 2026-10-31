#ifndef _FH_OSAL_SEQ_FILE_H
#define _FH_OSAL_SEQ_FILE_H

typedef struct fh_osal_seqops {
	void * (*start) (void *m, loff_t *pos);
	void (*stop) (void *m, void *v);
	void * (*next) (void *m, void *v, loff_t *pos);
	int (*show) (void *m, void *v);
}fh_osal_seqops_t;

typedef struct fh_osal_seqcontext{
	void * seq_context;
}fh_osal_seqcontext_t;

int fh_osal_seq_register(const fh_osal_seqops_t *seqops_t, fh_osal_seqcontext_t *seqcontext);

void fh_osal_seq_printf(void *m, const char *fmt, ...);
void fh_osal_seq_puts(void *m, const char *s);
int fh_osal_seq_open(void *file, fh_osal_seqcontext_t *seqcontext);
ssize_t fh_osal_seq_read(void *, char __user *, size_t, loff_t *);
loff_t fh_osal_seq_lseek(void *, loff_t, int);
int fh_osal_seq_release(void *, void *);
int fh_osal_seq_write(void *seq, const void *data, size_t len);
int fh_osal_single_open(void *file, int (*show)(void *m, void *v), void *data);
int fh_osal_single_open_size(void *file, int (*show)(void *m, void *v), void *data, size_t size);
int fh_osal_single_release(void *inode, void *file);
void fh_osal_seq_destroy(fh_osal_seqcontext_t *seqcontext);


#endif
