#include <stdio.h>
#include "sys/time.h"
#include <rttshell.h>

void rtc_demo_init(void)
{
	struct timeval curr_time;
	struct tm tm_curr;
	time_t tp;

    time(&tp);
    localtime_r(&tp, &tm_curr);
    printf("[rtc_demo]now is: %d-%d-%d %d:%d:%d\n", tm_curr.tm_year + 1900, tm_curr.tm_mon + 1, tm_curr.tm_mday, tm_curr.tm_hour, tm_curr.tm_min, tm_curr.tm_sec);

    tm_curr.tm_hour += 1;
    tp = mktime(&tm_curr);

    curr_time.tv_sec = tp;
    settimeofday(&curr_time, NULL);
    gettimeofday(&curr_time, NULL);

    printf("[rtc_demo]time_curr: %d s %d us\n", (int)curr_time.tv_sec, (int)curr_time.tv_usec);

    struct timespec real_time;
    real_time.tv_sec = tp;
    clock_settime(CLOCK_REALTIME, &real_time);
    clock_gettime(CLOCK_REALTIME, &real_time);
    printf("[rtc_demo]time_curr: %d s %d ns\n", (int)real_time.tv_sec, (int)real_time.tv_nsec);
}
