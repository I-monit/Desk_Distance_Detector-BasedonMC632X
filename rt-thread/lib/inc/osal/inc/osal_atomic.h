#ifndef _FH_OSAL_ATOMIC_H
#define _FH_OSAL_ATOMIC_H

typedef struct fh_osal_atomic {
	void *atomic;
}fh_osal_atomic_t;

int  fh_osal_atomic_init(fh_osal_atomic_t *atomic);
void fh_osal_atomic_destroy(fh_osal_atomic_t *atomic);
int  fh_osal_atomic_read(fh_osal_atomic_t *v);
void fh_osal_atomic_set(fh_osal_atomic_t *v, int i);
int  fh_osal_atomic_inc_return(fh_osal_atomic_t *v);
int  fh_osal_atomic_dec_return(fh_osal_atomic_t *v);

int fh_osal_atomic_add_return(int i, fh_osal_atomic_t *atomic);
int fh_osal_atomic_sub_return(int i, fh_osal_atomic_t *atomic);

static inline void fh_osal_atomic_add(int i, fh_osal_atomic_t *v)
{
	fh_osal_atomic_add_return(i, v);
}

static inline void fh_osal_atomic_sub(int i, fh_osal_atomic_t *v)
{
	fh_osal_atomic_sub_return(i, v);
}

static inline void fh_osal_atomic_inc(fh_osal_atomic_t *v)
{
	fh_osal_atomic_inc_return(v);
}

static inline void fh_osal_atomic_dec(fh_osal_atomic_t *v)
{
	fh_osal_atomic_dec_return(v);
}

#endif
