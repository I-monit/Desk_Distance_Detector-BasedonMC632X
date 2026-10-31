/**
 * Copyright (c) 2015-2019 Shanghai Fullhan Microelectronics Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020-07-13     songyh    the first version
 *
 */
#ifndef __PWM_H__
#define __PWM_H__

#ifndef NSEC_PER_SEC
#define NSEC_PER_SEC (1000000000ULL)
#endif

#define ENABLE_PWM                      0
#define DISABLE_PWM                     1
#define SET_PWM_CONFIG                  2
#define GET_PWM_CONFIG                  3
#define SET_PWM_DUTY_CYCLE_PERCENT      4
#define SET_PWM_ENABLE                  5
#define SET_PWM_STOP_CTRL               6
#define ENABLE_MUL_PWM                  7
#define ENABLE_FINSHALL_INTR            8
#define ENABLE_FINSHONCE_INTR           9
#define DISABLE_FINSHALL_INTR           10
#define DISABLE_FINSHONCE_INTR          11

struct fh_pwm_config
{
    unsigned int period_ns;
    unsigned int duty_ns;
#define  FH_PWM_PULSE_LIMIT         (0x0)
#define  FH_PWM_PULSE_NOLIMIT       (0x1)
    unsigned int pulses;
    unsigned int pulse_num;
#define FH_PWM_STOPLVL_LOW          (0x0)
#define FH_PWM_STOPLVL_HIGH         (0x3)
#define FH_PWM_STOPLVL_KEEP         (0x1)
    unsigned int stop_module;
#define  FH_PWM_STOPCTRL_INS        (0x1)
#define  FH_PWM_STOPCTRL_DELAY      (0x0)
    unsigned int stop_ctrl;
    unsigned int delay_ns;
    unsigned int phase_ns;
    unsigned int percent;
    unsigned int shadow_enable;
    unsigned int finish_once;
    unsigned int finish_all;
};

struct fh_pwm_status
{
    unsigned int done_cnt;
    unsigned int total_cnt;
    unsigned int busy;
    unsigned int error;
};

struct fh_pwm_chip_data
{
    int id;
    struct fh_pwm_config config;
    struct fh_pwm_status status;
    void (*finishall_callback)(struct fh_pwm_chip_data *data);
    void (*finishonce_callback)(struct fh_pwm_chip_data *data);
};

void rt_hw_pwm_init(void);

#endif
