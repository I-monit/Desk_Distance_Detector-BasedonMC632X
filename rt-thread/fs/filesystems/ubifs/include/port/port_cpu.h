#ifndef __PORT_CPU_H__
#define __PORT_CPU_H__
#include <ubi_compiler.h>
#include <ubi_types.h>

#ifdef CPU_32BIT
#define BITS_PER_LONG 32
#endif
#ifdef CPU_64BIT
#define BITS_PER_LONG 64
#endif

#define BITS_PER_LONG_LONG 64

#define BIT(nr)         (1UL << (nr))
#define BIT_ULL(nr)     (1ULL << (nr))
#define BIT_MASK(nr)        (1UL << ((nr) % BITS_PER_LONG))
#define BIT_WORD(nr)        ((nr) / BITS_PER_LONG)
#define BIT_ULL_MASK(nr)    (1ULL << ((nr) % BITS_PER_LONG_LONG))
#define BIT_ULL_WORD(nr)    ((nr) / BITS_PER_LONG_LONG)
#define BITS_PER_BYTE       8
/* #define BITS_TO_LONGS(nr)   DIV_ROUND_UP(nr, BITS_PER_BYTE * sizeof(long)) */
#ifdef __FULLHAN___
   #include <machine/endian.h>
   #if _BYTE_ORDER == _LITTLE_ENDIAN
      #include "linux/byteorder/little_endian.h"
  #else
      #include "linux/byteorder/big_endian.h"
  #endif
#else
   #include <endian.h> /* x86 */
   #if __BYTE_ORDER == __LITTLE_ENDIAN
      #include "linux/byteorder/little_endian.h"
  #else
      #include "linux/byteorder/big_endian.h"
  #endif
#endif
#include "linux/byteorder/generic.h"
/*u_int16_t le16_to_cpu(u_int16_t x);
u_int32_t le32_to_cpu(u_int32_t x);
u_int64_t le64_to_cpu(u_int64_t x);

u_int16_t cpu_to_le16(u_int16_t x);
u_int32_t cpu_to_le32(u_int32_t x);
u_int64_t cpu_to_le64(u_int64_t x);

u_int16_t be16_to_cpu(u_int16_t x);
u_int32_t be32_to_cpu(u_int32_t x);
u_int64_t be64_to_cpu(u_int64_t x);

u_int16_t cpu_to_be16(u_int16_t x);
u_int32_t cpu_to_be32(u_int32_t x);
u_int64_t cpu_to_be64(u_int64_t x);

little endian
#define __cpu_to_le32(x) ((__force __le32)(__u32)(x))
#define __le32_to_cpu(x) ((__force __u32)(__le32)(x))

static inline int GetEndianness()
{
#ifdef __LITTLE_ENDIAN
    return 0;
#elif defined(__BIG_ENDIAN)
    return 1;
#else
    static  unsigned char  gEndian = 0xF;
    if (gEndian != 0xF)
    {
        return gEndian;
    }
    short s = 0x0110;
    char *p = (char *)&s;
    if (p[0] == 0x10) gEndian = 0; // 小端格式
    else gEndian = 1; // 大端格式
    return gEndian;
#endif
}*/

/*
static inline int test_bit(int nr, const void * addr)
{
    return ((unsigned char *) addr)[nr >> 3] & (1U << (nr & 7));
}*/
static inline int test_bit(unsigned int nr, const unsigned long *addr)
{
    return ((1UL << (nr % BITS_PER_LONG)) &
            (((unsigned long *)addr)[nr / BITS_PER_LONG])) != 0;
}
/**
 * __set_bit - Set a bit in memory
 * @nr: the bit to set
 * @addr: the address to start counting from
 *
 * Unlike set_bit(), this function is non-atomic and may be reordered.
 * If it's called on the same region of memory simultaneously, the effect
 * may be that only one operation succeeds.
 */
static inline void __set_bit(int nr, volatile unsigned long *addr)
{
    unsigned long mask = BIT_MASK(nr);
    unsigned long *p = ((unsigned long *)addr) + BIT_WORD(nr);

    *p  |= mask;
}

static inline void __clear_bit(int nr, volatile unsigned long *addr)
{
    unsigned long mask = BIT_MASK(nr);
    unsigned long *p = ((unsigned long *)addr) + BIT_WORD(nr);

    *p &= ~mask;
}
static inline int test_and_set_bit(int nr, volatile void *addr)
{
    unsigned long mask = BIT_MASK(nr);
    unsigned long *p = ((unsigned long *)addr) + BIT_WORD(nr);
    unsigned long old = *p;

    *p = old | mask;
    return (old & mask) != 0;
}

static inline void set_bit(int nr, unsigned long *addr)
{
    addr[nr / BITS_PER_LONG] |= 1UL << (nr % BITS_PER_LONG);
}

static inline void clear_bit(int nr, unsigned long *addr)
{
    addr[nr / BITS_PER_LONG] &= ~(1UL << (nr % BITS_PER_LONG));
}


/**
 * fls - find last (most-significant) bit set
 * @x: the word to search
 *
 * This is defined the same way as ffs.
 * Note fls(0) = 0, fls(1) = 1, fls(0x80000000) = 32.
 */

static inline int fls(int x)
{
    int r = 32;

    if (!x)
        return 0;
    if (!(x & 0xffff0000u))
    {
        x <<= 16;
        r -= 16;
    }
    if (!(x & 0xff000000u))
    {
        x <<= 8;
        r -= 8;
    }
    if (!(x & 0xf0000000u))
    {
        x <<= 4;
        r -= 4;
    }
    if (!(x & 0xc0000000u))
    {
        x <<= 2;
        r -= 2;
    }
    if (!(x & 0x80000000u))
    {
        x <<= 1;
        r -= 1;
    }
    return r;
}

u_int32_t  __div64_32(u_int64_t *n, u_int32_t base);
u_int32_t  __idiv64_32(u_int64_t n, u_int32_t base);
#ifdef ARCH_CPU_64BIT

#define do_div(n, base) ({                    \
    u_int32_t __base = (base);                \
    u_int32_t __rem;                        \
    __rem = ((u_int64_t)(n)) % __base;            \
    (n) = ((u_int64_t)(n)) / __base;                \
    __rem;                            \
 })
#define div_u64(n, base) do_div(n, base)
static inline u_int64_t div_u64_rem(u_int64_t dividend, u_int32_t divisor, u_int32_t *remainder)
{
    *remainder = dividend % divisor;
    return dividend / divisor;
}
#else
#define do_div(n, base) __div64_32(&n, base)
#define div_u64(n, base) __idiv64_32(n, base)
#ifndef div_u64_rem
static inline u_int64_t div_u64_rem(u_int64_t dividend, u_int32_t divisor, u_int32_t *remainder)
{
    *remainder = do_div(dividend, divisor);
    return dividend;
}
#endif
#endif

#endif
