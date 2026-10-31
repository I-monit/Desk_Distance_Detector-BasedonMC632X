#ifndef _FH_OSAL_TOOL_H
#define _FH_OSAL_TOOL_H

#define FH_OSAL_SCHED_NORMAL		0
#define FH_OSAL_SCHED_FIFO		1
#define FH_OSAL_SCHED_RR		2

#define FH_OSAL_MAX_USER_RT_PRIO	100
#define FH_OSAL_MAX_RT_PRIO		FH_OSAL_MAX_USER_RT_PRIO

bool __must_check FH_OSAL_IS_ERR(__force const void *ptr);
unsigned int fh_osal_do_div(unsigned int n, unsigned int base);
unsigned long long fh_osal_do_div64(unsigned long long n, unsigned int base);
unsigned long long fh_osal_do_div_rem64(unsigned long long n, unsigned int base);
long long fh_osal_do_div64_s64(long long dividend, long long divisor);
unsigned long long fh_osal_do_div64_u64(unsigned long long dividend, unsigned long long divisor);
void fh_osal_fh_setscheduler(int policy, int priority);
unsigned int fh_osal_get_hz(void);
void fh_osal_dump_stack(void);
int fh_osal_on_each_cpu(void (*func) (void *info), void *info, int wait);


#define FH_OSAL_WARN_ON(condition) do {\
	if(unlikely((condition) != 0)){\
		fh_osal_printk("Badness in %s at %s:%d\n", __FUNCTION__, __FILE__, __LINE__);\
		fh_osal_dump_stack();\
	}\
}while(0)

#define fh_osal_min(x,y) ({ \
	typeof(x) _x = (x);	\
	typeof(y) _y = (y);	\
	(void) (&_x == &_y);	\
	_x < _y ? _x : _y; })

#define fh_osal_max(x,y) ({ \
	typeof(x) _x = (x);	\
	typeof(y) _y = (y);	\
	(void) (&_x == &_y);	\
	_x > _y ? _x : _y; })

#define FH_OSAL_DIV_ROUND_UP_ULL(ll,d) \
	({ unsigned long long _tmp = (ll)+(d)-1; fh_osal_do_div64(_tmp, d); })


#endif
