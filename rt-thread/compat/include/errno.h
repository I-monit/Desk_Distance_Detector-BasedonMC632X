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
#ifndef __ERRNO_H_
#define __ERRNO_H_

#include <sys/errno.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef errno
#undef errno
#endif

extern int *_rt_errno(void);
#define errno (*_rt_errno())

#ifdef _AEABI_PORTABLE
  extern _CONST int __aeabi_EDOM;
# define EDOM (__aeabi_EDOM)
#endif

#ifdef _AEABI_PORTABLE
  extern _CONST int __aeabi_ERANGE;
# define ERANGE (__aeabi_ERANGE);
#endif

#ifndef _AEABI_PORTABLE
#define ENOSYS 88    /* Function not implemented */
#endif /* !_AEABI_PORTABLE */

#ifdef _AEABI_PORTABLE
  extern _CONST int __aeabi_EILSEQ;
# define EILSEQ (__aeabi_EILSEQ)
#endif /* !_AEABI_PORTABLE */

#ifdef __cplusplus
}
#endif

#endif
