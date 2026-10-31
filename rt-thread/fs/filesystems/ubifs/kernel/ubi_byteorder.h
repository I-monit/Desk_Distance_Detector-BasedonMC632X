#ifndef __UBI__BYTEODER__H_
#define __UBI__BYTEODER__H_
/* byte order for different platform */

#ifdef __linux__
#include <asm/byteorder.h>
#include <endian.h>
#if __BYTE_ORDER == __LITTLE_ENDIAN
#define _UBI_LITTLE_ENDIAN
#elif __BYTE_ORDER == __BIG_ENDIAN
#define _UBI_BIG_ENDIAN
#endif

#else
#include <machine/endian.h> /* fullhan */
#if _BYTE_ORDER == _LITTLE_ENDIAN
#define _UBI_LITTLE_ENDIAN
#else
#define _UBI_BIG_ENDIAN
#endif
#endif

#endif
