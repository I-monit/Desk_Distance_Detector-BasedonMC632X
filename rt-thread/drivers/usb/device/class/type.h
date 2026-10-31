#ifndef __TYPE_H__
#define __TYPE_H__

typedef unsigned char __u8;
typedef unsigned short __u16;
typedef unsigned int __u32;

#define __bitwise

typedef __u16 __bitwise __le16;
typedef __u32 __bitwise __le32;


# define __force

#define __cpu_to_le32(x) ((__force __le32)(__u32)(x))
#define __cpu_to_le16(x) ((__force __le16)(__u16)(x))

#define  cpu_to_le16  __cpu_to_le16
#define  cpu_to_le32  __cpu_to_le32

#define  le16_to_cpu(x) x

#endif
