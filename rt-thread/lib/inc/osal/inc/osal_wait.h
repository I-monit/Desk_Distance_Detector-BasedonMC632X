#ifndef _FH_OSAL_WAIT_H
#define _FH_OSAL_WAIT_H

typedef struct fh_osal_wait{
	void *wait;
}fh_osal_wait_t;

typedef int (*fh_osal_wait_cond_func_t)(void *param);

int fh_osal_wait_init(fh_osal_wait_t * wait);
void fh_osal_wakeup(fh_osal_wait_t * wait);
void fh_osal_wake_up_interruptible(fh_osal_wait_t * wait);
int fh_osal_wait_interruptible(fh_osal_wait_t * wait, fh_osal_wait_cond_func_t func, void* param);
int fh_osal_wait_uninterruptible(fh_osal_wait_t * wait, fh_osal_wait_cond_func_t func, void* param);
int fh_osal_wait_timeout_interruptible(fh_osal_wait_t * wait, fh_osal_wait_cond_func_t func, void* param, unsigned long ms);
int fh_osal_wait_timeout_uninterruptible(fh_osal_wait_t * wait, fh_osal_wait_cond_func_t func, void* param, unsigned long ms);
int fh_osal_easy_condition(void *param);
void fh_osal_wait_destroy(fh_osal_wait_t * wait);
long fh_osal_schedule_timeout(long timeout);

#define fh_osal_wait_event_interruptible(wait, func, param)    \
({                                  \
    int __ret = 0;                          \
                                          \
    for (;;){                          \
        if(func(param)){                       \
            __ret = 0;                  \
            break;                    \
        }\
        __ret = fh_osal_wait_interruptible(wait, (func), param);   \
        if(__ret < 0)                 \
            break;           \
    }                                    \
    __ret;                                   \
})

#define fh_osal_wait_event(wait, func, param)    \
({                                  \
    int __ret = 0;                          \
                                          \
    for (;;){                          \
        if(func(param)){                       \
            __ret = 0;                  \
            break;                    \
        }\
        __ret = fh_osal_wait_uninterruptible(wait, (func), param);   \
        if(__ret < 0)                 \
            break;           \
    }                                    \
    __ret;                                   \
})

#define fh_osal_wait_event_interruptible_timeout(wait, func, param, timeout)             \
({                                  \
    int __ret = timeout;                          \
                                        \
    if ((func(param)) && !timeout) \
    { \
    __ret = 1; \
    } \
                                          \
    for (;;) {                          \
        if (func(param))                       \
        {\
            __ret = fh_osal_msecs_to_jiffies(__ret);     \
            break;                    \
        }\
        __ret = fh_osal_wait_timeout_interruptible(wait, (func), param, __ret);   \
        if(!__ret || __ret == -ERESTARTSYS)   \
            break;                    \
    }                                   \
    __ret;                                   \
})

#define fh_osal_wait_event_timeout(wait, func, param, timeout)             \
		({									\
			int __ret = timeout;						  \
												\
			if ((func(param)) && !timeout) \
			{ \
			__ret = 1; \
			} \
												  \
			for (;;) {							\
				if (func(param))					   \
				{\
					__ret = fh_osal_msecs_to_jiffies(__ret);	  \
					break;					  \
				}\
				__ret = fh_osal_wait_timeout_uninterruptible(wait, (func), param, __ret);	 \
				if(!__ret || __ret == -ERESTARTSYS)   \
					break;					  \
			}									\
			__ret;									 \
		})

#endif