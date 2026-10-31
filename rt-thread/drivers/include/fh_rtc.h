/**
 * Copyright (c) 2015-2019 Shanghai Fullhan Microelectronics Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 *
 * Change Logs:
 * Date           Author       Notes
 * 2019-03-21     xuww         add license Apache-2.0
 */
#ifndef __FH_RTC_H__
#define __FH_RTC_H__

#include <rtdevice.h>
#include <drivers/alarm.h>

/*
 * Registers offset
 */
#define  FH_RTC_INT_STATUS          0x0
#define  FH_RTC_INT_EN              0x4
#define  FH_RTC_DEBUG0              0x8
#define  FH_RTC_DEBUG1              0xC
#define  FH_RTC_DEBUG2              0x10
#define  FH_RTC_CMD                 0x14
#define  FH_RTC_RD_DATA             0x18
#define  FH_RTC_WR_DATA             0x1C

#define  FH_RTC_CMD_COUNTER            (0<<4)
#define  FH_RTC_CMD_OFFSET             (1<<4)
#define  FH_RTC_CMD_ALARM_CFG          (2<<4)
#define  FH_RTC_CMD_TEMP_INFO          (0x3<<4)
#define  FH_RTC_CMD_TEMP_CFG           (0x4<<4)
#define  FH_RTC_CMD_ANA_CFG            (0x5<<4)
#define  FH_RTC_CMD_INT_STATUS         (0x6<<4)
#define  FH_RTC_CMD_INT_EN             (0x7<<4)
#define  FH_RTC_CMD_DEBUG              (0x8<<4)
#define  FH_RTC_CMD_OFFSET_LUT         (0x9<<4)

#define RTC_SCU_CLK     0
#define RTC_OSC_CLK     1

#define  RTC_READ 1
#define  RTC_WRITE 2
#define  RTC_TEMP 3

#define  FH_RTC_INT_STATUS_RX_CRC_ERR   (1<<0)
#define  FH_RTC_INT_STATUS_RX_COM_ERR   (1<<1)
#define  FH_RTC_INT_STATUS_RX_LEN_ERR   (1<<2)
#define  FH_RTC_INT_STATUS_CNT_THL      (1<<3)
#define  FH_RTC_INT_STATUS_CNT_THH      (1<<4)
#define  FH_RTC_INT_STATUS_CORE_IDLE    (1<<5)
#define  FH_RTC_INT_STATUS_CORE         (1<<6)
#define  FH_RTC_INT_STATUS_WRAPPER_BUSY (1<<8)
#define  FH_RTC_INT_STATUS_CORE_BUSY    (1<<16)

#define  FH_RTC_INT_RX_CRC_ERR_EN        (1<<0)
#define  FH_RTC_INT_RX_COM_ERR_EN        (1<<1)
#define  FH_RTC_INT_RX_LEN_ERR_EN        (1<<2)
#define  FH_RTC_INT_CNT_THL_ERR_EN       (1<<3)
#define  FH_RTC_INT_CNT_THH_ERR_EN       (1<<4)
#define  FH_RTC_INT_CORE_IDLE_ERR_EN     (1<<5)
#define  FH_RTC_INT_CORE_INT_ERR_EN      (1<<6)
#define  FH_RTC_INT_RX_CRC_ERR_MASK      (1<<16)
#define  FH_RTC_INT_RX_COM_ERR_MASK      (1<<17)
#define  FH_RTC_INT_RX_LEN_ERR_MASK      (1<<18)
#define  FH_RTC_INT_CNT_THL_ERR_MASK     (1<<19)
#define  FH_RTC_INT_CNT_THH_ERR_MASK     (1<<20)
#define  FH_RTC_INT_CORE_IDLE_ERR_MASK   (1<<21)
#define  FH_RTC_INT_CORE_INT_ERR_MASK    (1<<22)
#define  FH_RTC_INT_CORE_INT_ERR_MASK_COV   0xffbfffff
#define  FH_RTC_INT_CORE_INT_STATUS_COV     0xffffff3f

#define FH_RTC_CORE_INT_EN_SEC_INT (0x1<<0)
#define FH_RTC_CORE_INT_EN_MIN_INT (0x1<<1)
#define FH_RTC_CORE_INT_EN_HOU_INT (0x1<<2)
#define FH_RTC_CORE_INT_EN_DAY_INT (0x1<<3)
#define FH_RTC_CORE_INT_EN_ALM_INT (0x1<<4)
#define FH_RTC_CORE_INT_EN_POW_INT (0x1<<5)
#define FH_RTC_CORE_INT_EN_ALM_OUT_INT (0x1<<15)

#define FH_RTC_CORE_INT_EN_SEC_MAS (0x1<<16)
#define FH_RTC_CORE_INT_EN_MIN_MAS (0x1<<17)
#define FH_RTC_CORE_INT_EN_HOU_MAS (0x1<<18)
#define FH_RTC_CORE_INT_EN_DAY_MAS (0x1<<19)
#define FH_RTC_CORE_INT_EN_ALM_MAS (0x1<<20)
#define FH_RTC_CORE_INT_EN_POE_MAS (0x1<<21)
#define FH_RTC_CORE_INT_EN_ALM_OUT_MAS (0x1<<31)


#define  OFFSET_EN                     (1<<0)
#define  OFFSET_ATUTO                  (1<<1)
#define  OFFSET_IDX                    (1<<2)
#define  OFFSET_BK_EN                  (1<<8)
#define  OFFSET_BK_AUTO                (1<<9)
#define  OFFSET_BK_IDX                 (1<<10)
#define  OFFSET_CURRENT                (1<<16)
#define  CLK_OSC_SEL                   (1<<28)
#define  LP_MODE                       (1<<31)


#define SEC_BIT_START       0
#define SEC_VAL_MASK        0x3f

#define MIN_BIT_START       6
#define MIN_VAL_MASK        0xfc0

#define HOUR_BIT_START      12
#define HOUR_VAL_MASK       0x1f000

#define DAY_BIT_START       17
#define DAY_VAL_MASK        0xfffe0000

#define FH_RTC_ISR_SEC_POS          (1<<0)
#define FH_RTC_ISR_MIN_POS          (1<<1)
#define FH_RTC_ISR_HOUR_POS         (1<<2)
#define FH_RTC_ISR_DAY_POS          (1<<3)
#define FH_RTC_ISR_ALARM_POS        (1<<4)
#define FH_RTC_ISR_POWERFAIL_POS    (1<<5)
#define FH_RTC_ISR_RX_CRC_ERR_INT   (1<<6)
#define FH_RTC_ISR_RX_COM_ERR_INT   (1<<7)
#define FH_RTC_LEN_ERR_INT          (1<<8)
#define FH_RTC_ISR_ALM_OUT_INT      (1<<31)


#define FH_GET_RTC_SEC(val)      ((val & SEC_VAL_MASK) >> SEC_BIT_START)
#define FH_GET_RTC_MIN(val)      ((val & MIN_VAL_MASK) >> MIN_BIT_START)
#define FH_GET_RTC_HOUR(val)     ((val & HOUR_VAL_MASK) >> HOUR_BIT_START)
#define FH_GET_RTC_DAY(val)      ((val & DAY_VAL_MASK) >> DAY_BIT_START)

#define ELAPSED_LEAP_YEARS(y)    (((y -1)/4)-((y-1)/100)+((y+299)/400)-17)

#define RTC_UIE_ON     0x15
#define RTC_UIE_OFF    0x16
#define RTC_PIE_ON     0x17
#define RTC_PIE_OFF    0x18

#define INT_ON 1
#define INT_OFF 0
struct fh_rtc_platform_data
{
    unsigned int clock_in;
    char *clk_name;
    char *dev_name;
    unsigned int base_year;
    unsigned int base_month;
    unsigned int base_day;
    int sadc_channel;
};
enum
{
    init_done = 1,
    initing = 0
};
struct fh_rtc_adjust_parm
{
    int lut_coef;
    int lut_offset;
    int tsensor_offset;
    int tsensor_coef;
    int tsensor_step;
    int tsensor_cp;
    int tsensor_cp_default_out;
    int *tsensor_lut;
};
struct fh_rtc_obj
{
    int id;
    unsigned int irq;
    unsigned int base;
    struct fh_rtc_adjust_parm *adjust_parm;
};
struct fh_rtc_controller
{
    unsigned int regs;
    unsigned int irq;
    unsigned int paddr;
    unsigned int base_year;
    unsigned int base_month;
    unsigned int base_day;
    struct rtc_device *rtc;
    struct clk *clk;
    int sadc_channel;
    struct rt_workqueue *wq;
    struct rt_work *self_adjust;
    struct rt_completion rtc_completion;
    struct rt_completion rtc_uie_completion;
    struct fh_rtc_adjust_parm *adjust_parm;
    int tsensor_delta_val;
};

unsigned int fh_rtc_get_sync_status(struct fh_rtc_obj *rtc);
int fh_rtc_gettime_nosync(struct fh_rtc_obj *rtc, unsigned int *time);
int fh_rtc_gettime_sync(struct fh_rtc_obj *rtc, unsigned int *time);
int fh_rtc_exam_magic(struct fh_rtc_obj *rtc);
int fh_rtc_settime(struct fh_rtc_obj *rtc, unsigned int *time);
int fh_rtc_set_alarm(struct fh_rtc_obj *rtc, struct rt_rtc_wkalarm *args);
int fh_adjust_rtc(void *param);
void fh_rtc_pie(struct fh_rtc_obj *rtc, int enable);
void fh_rtc_uie(struct fh_rtc_obj *rtc, int enable);
void fh_core_interrupt_handler(int irq, void *param);
void rtc_int_func(void *param);

#ifdef TEMP_ADJUST
void rtc_adjust(void);
int rtc_get_temp(void);
#endif

void rt_hw_rtc_init(void);
#endif /* FH_RTC_H_ */

