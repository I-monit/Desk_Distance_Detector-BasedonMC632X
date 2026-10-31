/**
 * Copyright (c) 2015-2019 Shanghai Fullhan Microelectronics Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 *
 * Change Logs:
 * Date           Author       Notes
 * 2019-04-12                  add license Apache-2.0
 */


#ifndef __TIMER_H__
#define __TIMER_H__

#include <rtdef.h>
#include <stdint.h>
#include <fh_arch.h>


typedef struct
{
    void *base;
    unsigned int rate;
    void (*irq_handler)(void);
    unsigned int reload_cnt;
} timer;

enum timer_mode
{
    TIMER_MODE_PERIODIC = 0,
    TIMER_MODE_ONESHOT  = 1,
};

int timer_init(timer *tim);

int timer_set_mode(timer *tim, enum timer_mode);

void timer_set_period(timer *tim, unsigned int period, unsigned int clock);

void timer_enable(timer *tim);

void timer_disable(timer *tim);

void timer_enable_irq(timer *tim);

void timer_disable_irq(timer *tim);

unsigned int timer_get_status(timer *tim);

unsigned int timer_get_eoi(timer *tim);

void timer_set_count(timer *tim, unsigned int count);

timer *get_timer(void);

void timer_set_irq_handler(timer *tim, void (*handler)(void));

uint64_t read_pts(void);
void rt_hw_timer_init(void);
void clocksource_pts_register(void);
void clockevent_timer0_register(void);
void clocksource_timer1_register(void);
#endif
