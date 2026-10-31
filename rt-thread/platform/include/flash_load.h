#ifndef __FLASH_INFO_H__
#define __FLASH_INFO_H__

#define FLASH_LOAD_SIZE          (0x1000)

#define FL_ALIGNUP(a)            (((a)+FLASH_LOAD_SIZE-1) & ~(FLASH_LOAD_SIZE-1))

#define HDR_MAGIC_NUMBER         (0xa7b4c9f8)
#define CODE_MAGIC_NUM           (0x7a3bc9fd)
#define CODE_MAGIC_NUM2          (0x3a7b5e9f)

enum
{
    CRYPTO_NONE = 0,
    CRYPTO_LZO,
    CRYPTO_GZIP,
    CRYPTO_LZMA,
    CRYPTO_NOSUPPORT
};

enum
{
    CODE_ARC = 0,     /* ARC firmware */
    CODE_ARM_SEG2,    /* multi-seg code, part of main program */
    CODE_NBG_FILE,
    CODE_ROOTFS,
};

extern int __get_code_part_idx(int code);
int get_code_part_info(void);
void load_flash_code(int idx, unsigned int offset, unsigned char *ram_addr, unsigned int len);

#endif
