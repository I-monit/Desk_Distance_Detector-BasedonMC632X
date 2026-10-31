/**
 * Copyright (c) 2015-2019 Shanghai Fullhan Microelectronics Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 *
 * Change Logs:
 * Date           Author       Notes
 * 2019-04-15     songyh    the first version
 *
 */

/*****************************************************************************
 *  Include Section
 *  add all #include here
 *  those macro below should define....
 *  or the dsp or isp lib will not be supported
 *****************************************************************************/
// #include <rtconfig_kernel.h>
#include <rtconfig.h>
// #include <rtconfig_driver.h>
/* #pragma comment ( linker, "/EXPORT: MyExportFunction = _MyExportFunctio" ) */
/* #pragma message */
/* #warning RT_NAME_MAX=32 */
/* #error .... */

/* #define yu_error(macro) #error  ##macro */

/**************************
 *
 *
 * define value below.
 *
 *
 **************************/
#if RT_NAME_MAX != 16
#error "define RT_NAME_MAX 16"
#endif

#if RT_TICK_PER_SECOND != 100
#warning "RT_TICK_PER_SECOND = 100"
#endif

#if RT_ALIGN_SIZE != 4
#error "define RT_ALIGN_SIZE 4"
#endif

/**************************
 *
 *
 * should define below..
 *
 *
 **************************/
#ifndef RT_USING_SEMAPHORE
#error need define "RT_USING_SEMAPHORE"
#endif

#ifndef RT_USING_MUTEX
#error need define "RT_USING_MUTEX"
#endif

#ifndef RT_USING_EVENT
#error need define "RT_USING_EVENT"
#endif

#ifndef RT_USING_MAILBOX
#error need define "RT_USING_MAILBOX"
#endif

#ifndef RT_USING_MESSAGEQUEUE
#warning need define "RT_USING_MESSAGEQUEUE"
#endif

#ifndef RT_USING_MEMPOOL
#warning need define "RT_USING_MEMPOOL"
#endif

#ifndef RT_USING_HEAP
#error need define "RT_USING_HEAP"
#endif

#ifndef RT_USING_MEMHEAP
#warning need define "RT_USING_MEMHEAP"
#endif

#ifndef RT_USING_DEVICE
#error need define "RT_USING_DEVICE"
#endif

#ifndef RT_USING_CONSOLE
#error need define "RT_USING_CONSOLE"
#endif

#ifndef RT_USING_FINSH
// #error need define "RT_USING_FINSH"
#endif

// #ifndef RT_USING_I2C
// #error need define "RT_USING_I2C"
// #endif

// #ifndef RT_USING_GPIO
// #error need define "RT_USING_GPIO"
// #endif
