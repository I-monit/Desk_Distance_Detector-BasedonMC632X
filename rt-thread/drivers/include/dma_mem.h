/**
 * Copyright (c) 2015-2019 Shanghai Fullhan Microelectronics Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 *
 * Change Logs:
 * Date           Author       Notes
 * 2019-01-24     tangyh    the first version
 *
 */

#ifndef __DMA_MEM_H__
#define __DMA_MEM_H__

#include <rtthread.h>
/****************************************************************************
* #include section
*    add #include here if any
***************************************************************************/


/****************************************************************************
* ADT section
*    add Abstract Data Type definition here
***************************************************************************/

/****************************************************************************
*  extern variable declaration section
***************************************************************************/

/****************************************************************************
*  section
*    add function prototype here if any
***************************************************************************/
rt_err_t fh_dma_mem_init(rt_uint32_t *mem_start, rt_uint32_t size);
void *fh_dma_mem_malloc(rt_uint32_t size);
void *fh_dma_mem_malloc_align(rt_uint32_t size, rt_uint32_t align, char *name);
void fh_dma_mem_free(void *ptr);
void fh_dma_mem_uninit(void);
/********************************End Of File********************************/


#endif
