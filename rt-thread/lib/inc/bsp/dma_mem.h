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
#include <stdint.h>
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
int32_t fh_dma_mem_init(uint32_t *mem_start, uint32_t size);
void *fh_dma_mem_malloc(uint32_t size);
void *fh_dma_mem_malloc_align(uint32_t size, uint32_t align, char *name);
void fh_dma_mem_free(void *ptr);
void fh_dma_mem_uninit(void);
/********************************End Of File********************************/


#endif
