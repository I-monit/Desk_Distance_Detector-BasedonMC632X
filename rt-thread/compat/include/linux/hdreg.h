/*
 *  * Copyright (c) 2015-2019 Shanghai Fullhan Microelectronics Co., Ltd.
 *  *
 *  * SPDX-License-Identifier: Apache-2.0
 *  *
 *  * Change Logs:
 *  * Date           Author       Notes
 *  * 2019-01-18      {fullhan}   the first version
 *  *
 */
#ifndef __HDREG_H
#define __HDREG_H

#ifdef __cplusplus
extern "C" {
#endif

#define HDIO_GETGEO     (97)   /* get device geometry */

struct hd_geometry
{
      unsigned char heads;
      unsigned char sectors;
      unsigned short cylinders;
      unsigned long start;
};


#ifdef __cplusplus
}
#endif

#endif
