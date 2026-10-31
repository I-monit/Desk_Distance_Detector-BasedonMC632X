#ifndef CRC32_H
#define CRC32_H

/* NOW, crc32 is moved to external as a single component */
/* #include <cyg/crc/crc.h>     */
#include <fh_crc32.h>

/* #define crc32(val, s, len) cyg_crc32_accumulate(val, (unsigned char *)s, len)    */
#define crc32(val, s, len) fh_crc32(val, (unsigned char *)s, len)

#endif
