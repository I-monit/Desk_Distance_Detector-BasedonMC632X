#include "port_cpu.h"
/*#include "ubi_def.h"*/
/* #include <rtthread.h> */
#include "linux/compat.h"
#include "ubiport_common.h"
/* #include <pthread.h> */
/*
#define endia_trans_16(x) ((uint16_t)(              \
    (((uint16_t)(x) & (uint16_t)0x000000ff) <<  8) |        \
    (((uint16_t)(x) & (uint16_t)0x0000ff00) >>  8)   ))

#define endia_trans_32(x) ((uint32_t)(              \
    (((uint32_t)(x) & (uint32_t)0x000000ffUL) << 24) |        \
    (((uint32_t)(x) & (uint32_t)0x0000ff00UL) <<  8) |        \
    (((uint32_t)(x) & (uint32_t)0x00ff0000UL) >>  8) |        \
    (((uint32_t)(x) & (uint32_t)0xff000000UL) >> 24)))

#define endia_trans_64(x) ((uint64_t)(              \
    (((uint64_t)(x) & (uint64_t)0x00000000000000ffUL) << 56) |        \
    (((uint64_t)(x) & (uint64_t)0x000000000000ff00UL) << 40) |        \
    (((uint64_t)(x) & (uint64_t)0x0000000000ff0000UL) << 24) |        \
    (((uint64_t)(x) & (uint64_t)0x00000000ff000000UL) << 8) |        \
    (((uint64_t)(x) & (uint64_t)0x000000ff00000000UL) >>  8) |        \
    (((uint64_t)(x) & (uint64_t)0x0000ff0000000000UL) >>  24) |        \
    (((uint64_t)(x) & (uint64_t)0x00ff000000000000UL) >>  40) |        \
    (((uint64_t)(x) & (uint64_t)0xff00000000000000UL) >>  56)))


uint16_t le16_to_cpu(uint16_t x)
{
    if (GetEndianness() == 0)
        return x;
    else
        return endia_trans_16(x);
}
uint32_t le32_to_cpu(uint32_t x)
{
    if (GetEndianness() == 0)
        return x;
    else
        return endia_trans_32(x);
}
uint64_t le64_to_cpu(uint64_t x)
{
    if (GetEndianness() == 0)
        return x;
    else
        return endia_trans_64(x);
}
uint16_t cpu_to_le16(uint16_t x)
{
    if (GetEndianness() == 0)
        return x;
    else
        return endia_trans_16(x);
}
uint32_t cpu_to_le32(uint32_t x)
{
    if (GetEndianness() == 0)
        return x;
    else
        return endia_trans_32(x);
}
uint64_t cpu_to_le64(uint64_t x)
{
    if (GetEndianness() == 0)
        return x;
    else
        return endia_trans_64(x);
}


uint16_t be16_to_cpu(uint16_t x)
{
    if (GetEndianness() == 0)
    {
        uint16_t tmp1 = endia_trans_16(x); //error if just return.e.g. x=0x300,return 0x30003.
        return tmp1;
    } else return x;
}
uint32_t be32_to_cpu(uint32_t x)
{
    if (GetEndianness() == 0)
    {
        return endia_trans_32(x);
    }
    else return x;
}
uint64_t be64_to_cpu(uint64_t x)
{
    if (GetEndianness() == 0) return endia_trans_64(x);
    else return x;
}
uint16_t cpu_to_be16(uint16_t x)
{
    if (GetEndianness() == 0)
    {
        uint16_t tmp1 = endia_trans_16(x);
        return tmp1;
    } else return x;
}
uint32_t cpu_to_be32(uint32_t x)
{
    if (GetEndianness() == 0) return endia_trans_32(x);
    else return x;
}

uint64_t cpu_to_be64(uint64_t x)
{
    if (GetEndianness() == 0) return endia_trans_64(x);
    else return x;
}
*/
uint32_t __div64_32(uint64_t *n, uint32_t base)
{
/*    printf("%ld,%d\n", *n, base);
    printf("%d\n", base);*/
    /*
    uint32_t ret = *n % base;
    *n =  *n / base;
    return ret;*/
    uint64_t rem = *n;
    uint64_t b = base;
    uint64_t res, d = 1;
    uint32_t high = rem >> 32;

    /* Reduce the thing a bit first */
    res = 0;
    if (high >= base)
    {
        high /= base;
        res = (uint64_t)high << 32;
        rem -= (uint64_t)(high * base) << 32;
    }

    while ((int64_t)b > 0 && b < rem)
    {
        b = b + b;
        d = d + d;
    }

    do
    {
        if (rem >= b)
        {
            rem -= b;
            res += d;
        }
        b >>= 1;
        d >>= 1;
    } while (d);

    *n = res;
    return rem;
}
uint32_t __idiv64_32(uint64_t n, uint32_t base)
{
/*    printf("%ld,%d\n", n, base);*/
    uint64_t nn = n;
    __div64_32(&nn, base);
    return nn;
}
