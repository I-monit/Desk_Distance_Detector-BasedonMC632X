/*
 * Copyright (c) 2006-2019, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author        Notes
 * 2019-05-06      songyh          first version
 */

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h> // gettimeofday()
#include <sys/prctl.h>
#include <time.h>
#include <stdlib.h>
#include <pthread.h>
#include <rttshell.h>
#define TEST_FILE "/mnt/test.dat"
#define CHUNK_SIZE (32*1024)
#define TEST_LOOP_CNT 1024

/** @fn double get_interval(IN const struct timeval start_time, IN const struct timeval end_time);
*  @brief 获取两个timeval类型start_time和end_time的间隔，以微妙为单位返回相差的绝对值
*  @      （两个输入参数颠倒后也能正常计算）
*  @param start_time  开始timeval值（跟timeval 可以颠倒）
*  @param end_time    结束timeval值（跟timeval 可以颠倒）
*  @return 间隔值 ：成功
*/

double get_interval(const struct timeval *start_time, const struct timeval *end_time)
{
    double delay_time = 0.0; // 存放临时时间间隔值

    if (end_time->tv_sec >= start_time->tv_sec)
    {
        delay_time = (end_time->tv_sec - start_time->tv_sec)*1000000;
        delay_time += end_time->tv_usec - start_time->tv_usec;

		// 防止在输入值颠倒的情况下，而两个时间值的秒相同，则需要取绝对值
        delay_time = (delay_time < 0) ? (0 - delay_time): (delay_time);
    }
    else
    {
        delay_time = (start_time->tv_sec - end_time->tv_sec)*1000000;
        delay_time += start_time->tv_usec - end_time->tv_usec;
    }

    return delay_time;
}
void *sdcard_demo_proc(void *param)
{
    int fd;
    int index, length;
    int round;
    struct timeval tv_begin,tv_end; // 用于存放读取的开始时间以及截至时间
    double read_speed;
    double write_speed;
    char *write_buf;
    char *read_buf;
    int stop_test = 0;
    double time_spend = 0.0;

    prctl(PR_SET_NAME, "sdcard_demo");
    write_buf = malloc(CHUNK_SIZE);
    if (write_buf == NULL)
    {
        printf("[sdcard_demo] malloc write buffer failed\n");
        return NULL;
    }
    read_buf = malloc(CHUNK_SIZE);
    if (read_buf == NULL)
    {
        printf("[sdcard_demo] malloc read buffer failed\n");
        free(write_buf);
        return NULL;
    }

    round = 1;

    while (1)
    {
        fd = open(TEST_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0);
        if (fd < 0)
        {
            printf("[sdcard_demo] open file for write failed\n");
            sleep(1);
            continue;
        }

        for (index = 0; index < CHUNK_SIZE; index++)
            write_buf[index] = index % 0xFF;

        gettimeofday(&tv_begin, NULL);
        for (index = 0; index < TEST_LOOP_CNT; index++)
        {
            length = write(fd, write_buf, CHUNK_SIZE);
            if (length != CHUNK_SIZE)
            {
                printf("[sdcard_demo] write data failed\n");
                close(fd);
                stop_test = 1;
                break;
            }
        }
        if (stop_test)
            break;
        gettimeofday(&tv_end, NULL);
        time_spend = get_interval(&tv_begin, &tv_end);
				if (time_spend != 0)
				{
					write_speed = CHUNK_SIZE*TEST_LOOP_CNT / time_spend;
					printf("\n\t[sdcard_demo] Write speed:  %.2f(MB/S)\n\n", write_speed);
				}
        close(fd);

        fd = open(TEST_FILE, O_RDONLY, 0);
        if (fd < 0)
        {
            printf("[sdcard_demo] open file for read failed\n");
            break;
        }

        gettimeofday(&tv_begin, NULL);
        for (index = 0; index < TEST_LOOP_CNT; index++)
        {
            length = read(fd, read_buf, CHUNK_SIZE);
            if (length != CHUNK_SIZE)
            {
                printf("[sdcard_demo] read file failed\n");
                close(fd);
                stop_test = 1;
                break;
            }
        }
        if (stop_test)
            break;
        gettimeofday(&tv_end, NULL);;
        for (index = 0; index < CHUNK_SIZE; index++)
        {
            if (read_buf[index] != write_buf[index])
            {
                printf("[sdcard_demo] data check failed, read_buf[%d]0x%x != write_buf[%d]0x%x\n", index, read_buf[index], index, write_buf[index]);
                close(fd);
                stop_test = 1;
                break;
            }
        }
        if (stop_test)
            break;
        time_spend = get_interval(&tv_begin, &tv_end);
				if (time_spend != 0)
				{
					read_speed = CHUNK_SIZE*TEST_LOOP_CNT / time_spend;
					printf("\n\t[sdcard_demo] Read speed:  %.2f(MB/S)\n\n", read_speed);
				}

        printf("[sdcard_demo] test round %d ", round++);
        close(fd);
    }

    free(write_buf);
    free(read_buf);
    return NULL;
}

void sdcard_demo_init(void)
{
    pthread_attr_t attr;
    pthread_t thrd;
    struct sched_param param;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_attr_setstacksize(&attr, 10240);

    param.sched_priority = 80;
    pthread_attr_setschedparam(&attr, &param);

    if (pthread_create(&thrd, &attr, sdcard_demo_proc, NULL) != 0)
    {
        printf("[sdcard_demo] Create sdcard_demo thread failed!\n");
    }
}
