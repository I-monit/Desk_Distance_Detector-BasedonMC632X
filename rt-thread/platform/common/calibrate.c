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

#include "fh_def.h"
#include <rtthread.h>
#include <delay.h>

/* #define FH_CALIBRATE_DEBUG */

#if defined(FH_CALIBRATE_DEBUG) /* && defined(RT_DEBUG) */
#define FH_CALIBRATE_PRINT(fmt, args...)    \
    do                                      \
    {                                       \
     /*   rt_kprintf("FH_CALIBRATE_PRINT: ");*/ \
        rt_kprintf(fmt, ##args);            \
    } while (0)
#else
#define FH_CALIBRATE_PRINT(fmt, args...) \
    do                                   \
    {                                    \
    } while (0)
#endif

unsigned long lpj_fine = 0;
/* cpu clk with 1.0 DMIPS(armv6 typical value) loop has 2 instructions */
#ifdef FH_FAST_BOOT

#if defined(CONFIG_CHIP_MC632X)
#define PRESET_LPJ      (3000000)
#endif
#if defined(CONFIG_CHIP_FH8626V100)
#define PRESET_LPJ      (3000000) /* default as 600M */
#else
#if defined(CONFIG_CHIP_FH8656) || defined(CONFIG_CHIP_FH8658)
#define PRESET_LPJ      (9936896)   /* 996M */
#endif
#ifdef CONFIG_CHIP_FH8852V201
#define PRESET_LPJ      (7905280)   /* 792M */
#endif
#ifdef CONFIG_CHIP_FH8852V301
#define PRESET_LPJ      (7905280)   /* 792M */
#endif
#ifdef CONFIG_CHIP_FH8662V100
#define PRESET_LPJ      (7905280)   /* 792M */
#endif
#ifdef CONFIG_CHIP_FH8626V200
#define PRESET_LPJ      (7905280)   /* 792M */
#endif
#ifdef CONFIG_CHIP_FH8652
#define PRESET_LPJ      (7905280)   /* 792M */
#endif
#if defined(CONFIG_CHIP_FH8858V200)
#define PRESET_LPJ      (4000000)   /* TBD, assume 800M */
#endif
#if defined(CONFIG_CHIP_FH8852V310) || defined(CONFIG_CHIP_FH8856V310) || defined(CONFIG_CHIP_FH8858V310)
#define PRESET_LPJ      (8953856)   /* TBD, assume 800M */
#endif
#if defined(CONFIG_CHIP_FH8852V200) || defined(CONFIG_CHIP_FH8852V210) \
|| defined(CONFIG_CHIP_FH8856V210) || defined(CONFIG_CHIP_FH8856V200)
#define PRESET_LPJ      (3475456)   /* 696M */
#endif
#endif
unsigned long preset_lpj = PRESET_LPJ;
unsigned long loops_per_jiffy = PRESET_LPJ;
#else
unsigned long preset_lpj = 0; /* try caculate */
unsigned long loops_per_jiffy = (1 << 12);
#endif
static unsigned long calibrate_delay_converge(void);

/*
 * This is the number of bits of precision for the loops_per_jiffy.  Each
 * time we refine our estimate after the first takes 1.5/HZ seconds, so try
 * to start with a good estimate.
 * For the boot cpu we can skip the delay calibration and assign it a value
 * calculated based on the timer frequency.
 * For the rest of the CPUs we cannot assume that the timer frequency is same as
 * the cpu frequency, hence do the calibration for those.
 */
#define LPS_PREC 8

static unsigned long calibrate_delay_converge(void)
{
    /* First stage - slowly accelerate to find initial bounds */
    unsigned long lpj, lpj_base, ticks, loopadd, loopadd_base, chop_limit;
    int trials = 0, band = 0, trial_in_band = 0;

    lpj = (1 << 12);

    /* wait for "start of" clock tick */
    ticks = rt_tick_get();
    while (ticks == rt_tick_get())
        ; /* nothing */
    /* Go .. */
    ticks = rt_tick_get();
    do
    {
        if (++trial_in_band == (1 << band))
        {
            ++band;
            trial_in_band = 0;
        }
        __delay(lpj * band);
        trials += band;
    } while (ticks == rt_tick_get());
    /*
     * We overshot, so retreat to a clear underestimate. Then estimate
     * the largest likely undershoot. This defines our chop bounds.
     */
    trials -= band;
    loopadd_base = lpj * band;
    lpj_base     = lpj * trials;

/* FH_CALIBRATE_PRINT("trials: %d, loopadd_base: %lu, lpj_base: %lu\n", trials, */
/* loopadd_base, lpj_base); */

recalibrate :
    lpj     = lpj_base;
    loopadd = loopadd_base;

    /*
     * Do a binary approximation to get lpj set to
     * equal one clock (up to LPS_PREC bits)
     */
    chop_limit = lpj >> LPS_PREC;
    while (loopadd > chop_limit)
    {
        lpj += loopadd;
        ticks = rt_tick_get();
        while (ticks == rt_tick_get())
            ; /* nothing */
        ticks = rt_tick_get();
        __delay(lpj);
        if (rt_tick_get() != ticks) /* longer than 1 tick */
            lpj -= loopadd;
        loopadd >>= 1;
    }
    /*
     * If we incremented every single time possible, presume we've
     * massively underestimated initially, and retry with a higher
     * start, and larger range. (Only seen on x86_64, due to SMIs)
     */
    if (lpj + loopadd * 2 == lpj_base + loopadd_base * 2)
    {
        lpj_base = lpj;
        loopadd_base <<= 2;
        goto recalibrate;
    }

    return lpj;
}

void calibrate_delay(void)
{
    unsigned long lpj;
    static rt_bool_t printed = RT_FALSE;

    if (preset_lpj)
    {
        lpj = preset_lpj;
        if (!printed)
            FH_CALIBRATE_PRINT(
                "Calibrating delay loop (skipped) preset value.. ");
    }
    else if ((!printed) && lpj_fine)
    {
        lpj = lpj_fine;
        FH_CALIBRATE_PRINT(
            "Calibrating delay loop (skipped), value calculated using timer frequency.. ");
    }
    else
    {
        if (!printed)
            FH_CALIBRATE_PRINT("Calibrating delay loop... ");
        lpj = calibrate_delay_converge();
    }

    if (!printed)
        FH_CALIBRATE_PRINT("%lu.%02lu BogoMIPS (lpj=%lu)\n",
                           lpj / (500000 / RT_TICK_PER_SECOND),
                           (lpj / (5000 / RT_TICK_PER_SECOND)) % 100, lpj);

    loops_per_jiffy = lpj;
    printed         = RT_TRUE;
}
