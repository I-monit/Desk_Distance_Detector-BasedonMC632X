#ifndef _FH_OSAL_TIMER_H
#define _FH_OSAL_TIMER_H

typedef struct fh_osal_timer{
	void *timer;
	void (*function)(void *param);
	void *data;
}fh_osal_timer_t;

typedef struct fh_osal_timeval{
	  long tv_sec;
	  long tv_usec;
}fh_osal_timeval_t;

typedef struct fh_osal_rtc_time {
	int tm_sec;
	int tm_min;
	int tm_hour;
	int tm_mday;
	int tm_mon;
	int tm_year;
	int tm_wday;
	int tm_yday;
	int tm_isdst;
} fh_osal_rtc_time_t;

int fh_osal_timer_init(fh_osal_timer_t *timer);
int fh_osal_set_timer(fh_osal_timer_t *timer, unsigned long interval);//ms
int fh_osal_del_timer(fh_osal_timer_t *timer);
int fh_osal_del_timer_sync(fh_osal_timer_t *timer);
u64 fh_osal_local_clock(void);

unsigned long fh_osal_msleep(unsigned int msecs);
unsigned long fh_osal_msleep_interruptible(unsigned int msecs);
void fh_osal_usleep_range(unsigned long min, unsigned long max);
void fh_osal_udelay(unsigned int usecs);
void fh_osal_mdelay(unsigned int msecs);


unsigned int fh_osal_get_tickcount(void);
unsigned long long fh_osal_sched_clock(void);
void fh_osal_gettimeofday(fh_osal_timeval_t *tv);
void fh_osal_rtc_time_to_tm(unsigned long time, fh_osal_rtc_time_t *tm);
void fh_osal_rtc_tm_to_time(fh_osal_rtc_time_t *tm ,unsigned long *time);
int fh_osal_rtc_valid_tm(struct fh_osal_rtc_time *tm);
void fh_osal_getjiffies(unsigned long *pjiffies);
unsigned int fh_osal_msecs_to_jiffies(const unsigned int m);
unsigned int fh_osal_jiffies_to_msecs(const unsigned long n);
u64 fh_osal_local_clock(void);

#endif
