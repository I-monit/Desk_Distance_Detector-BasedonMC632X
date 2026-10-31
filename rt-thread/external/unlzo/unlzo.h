#ifndef __UNLZO_H__
#define __UNLZO_H__

#include <rtthread.h>

/* #define malloc rt_malloc */
/* #define free   rt_free */
/* #define memcpy rt_memcpy */

int unlzo(unsigned char *inbuf, int len,
    int (*fill)(void*, unsigned int),
    int (*flush)(void*, unsigned int),
    unsigned char *output,
    int *pos, unsigned int *outputlen,
    void (*error)(char *x));
#endif
