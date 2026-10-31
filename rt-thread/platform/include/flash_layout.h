#ifndef __FLASH_LAYOUT_H__
#define __FLASH_LAYOUT_H__

/*
 *  flash 分区信息说明：
 *
 *  1. 最多支持15个分区
 *  2. 分区名称最大支持15个字符
 *  3. 必须定义应用代码存放分区，分区类型为PART_APPLICATION(2)
 *  4. 如果支持VBUS，请定义分区类型为PART_ARCFIRM(3)
 *  5. 其它用户自定义分区，名称自由定义，但不得超过15字节长度
 *  6. 如需自动挂载根分区，请定义根分区属性PART_ROOT
 *  7. 分区名称不能相同，否则，分区创建会出异常
 *
 */

#define MAX_FLASH_PART_NAME     16

#define MAX_FLASH_PART_NUM      16

enum _part_type
{
    PART_USER = 0,      /* user defined partitoin */
    PART_RESERVED,      /* sys defined partition */
    PART_APPLICATION,   /* application partition */
    PART_ARCFIRM,       /* arc firmware partition */
    PART_NN_MODEL,      /* NN model partition */

    PART_TMAX = 0x0f,   /* MAX part type */

    PART_ROOT = 0x40,   /* root partition flag, should be ORed with above partition type */
    PART_LAST = 0x80    /* deprecated now */
};

enum _part_erase_size
{
    BLOCK_64K = 0,
    BLOCK_4K,
    BLOCK_128K,
};

enum _flash_type
{
    FLASH_NOR = 0,
    FLASH_NAND,
    FLASH_NR,
};

struct _tag_flash_layout_info
{
    char part_name[MAX_FLASH_PART_NAME];
    unsigned int part_size;
    unsigned char erase_size;
    unsigned char part_type;
    unsigned char resved[10];
};

extern struct _tag_flash_layout_info g_flash_layout_info[MAX_FLASH_PART_NUM];
extern struct _tag_flash_layout_info g_flash_layout_nand_info[MAX_FLASH_PART_NUM];

#endif
