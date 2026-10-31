#ifndef UBI_TYPES_H_
#define UBI_TYPES_H_

#include <sys/types.h>
#include <stdint.h>
#include <stdbool.h>
typedef unsigned gfp_t;
#ifndef __BIT_TYPES_DEFINED__
#define __BIT_TYPES_DEFINED__

typedef signed char __int8_t;
typedef unsigned char __uint8_t;
typedef signed short int __int16_t;
typedef unsigned short int __uint16_t;
typedef signed int __int32_t;
typedef unsigned int __uint32_t;
#if __WORDSIZE == 64
typedef signed long int __int64_t;
typedef unsigned long int __uint64_t;
#else
typedef signed long long int __int64_t;
typedef unsigned long long int __uint64_t;
#endif

#endif /* !(__BIT_TYPES_DEFINED__) */
#ifndef __linux__
typedef long loff_t;
#endif
typedef __int8_t __s8;
typedef __uint8_t __u8;
typedef __uint16_t __u16;
typedef __int16_t __s16;
typedef __uint32_t __u32;
typedef __int32_t __s32;
typedef __uint64_t __u64;
typedef __int64_t __s64;

typedef __u16 __le16;
typedef __u16 __be16;
typedef __u32 __le32;
typedef __u32 __be32;
#if defined(__GNUC__)
typedef __u64 __le64;
typedef __u64 __be64;
#endif
typedef __u16 __sum16;
typedef __u32 __wsum;

typedef unsigned long ulong;
typedef u_int8_t uchar;

typedef __u64 u64;
typedef __u32 u32;
typedef __u16 u16;
typedef __u8 u8;

#ifndef NULL
#define NULL 0
#endif
#ifdef ARCH_CPU_64BIT
typedef __s64 lbaint_t;
#define LBAFlength "ll"
#define BITS_PER_LONG 64
#else
typedef __u32 lbaint_t;
#define LBAFlength "l"
#define BITS_PER_LONG 32
#endif

typedef unsigned short umode_t;

#ifndef _SIZE_T
#define _SIZE_T
typedef ubase_t size_t;
#endif

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

#ifndef MTD_UBIVOLUME
#define MTD_UBIVOLUME 7
#endif

#endif
