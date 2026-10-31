include $(SDKROOT)/build/config.mk

LIBNAME := $(shell basename `pwd`)

LOCAL_CFLAGS :=
# core
CSRCS := $(wildcard src/*.c)
CSRCS += $(wildcard filesystems/devfs/*.c)
ifeq ($(CONFIG_RT_USING_DFS_JFFS2), y)
    CSRCS += $(wildcard filesystems/jffs2/*.c)
    CSRCS += $(wildcard filesystems/jffs2/kernel/*.c)
    CSRCS += $(wildcard filesystems/jffs2/src/*.c)
    LOCAL_CFLAGS += -Ifilesystems/jffs2/include
    LOCAL_CFLAGS += -Ifilesystems/jffs2/cyg
    LOCAL_CFLAGS += -Ifilesystems/jffs2/kernel
    # include zlib directory directly.
    # LOCAL_CFLAGS += -Ifilesystems/jffs2/cyg/compress
endif

ifeq ($(CONFIG_RT_USING_DFS_RAMFS),y)
    CSRCS += $(wildcard filesystems/ramfs/*.c)
endif

ifeq ($(CONFIG_RT_USING_DFS_ROMFS),y)
    CSRCS += $(wildcard filesystems/romfs/dfs_romfs.c)
    ifneq ($(CONFIG_DFS_ROMFS_ROOT),y)
        CSRCS += $(wildcard filesystems/romfs/romfs.c)
    endif
endif

ifeq ($(CONFIG_RT_USING_DFS_NFS),y)
    CSRCS += $(wildcard filesystems/nfs/*.c)
    CSRCS += $(wildcard filesystems/nfs/rpc/*.c)
endif

ifeq ($(CONFIG_RT_USING_DFS_UFFS),y)
    CSRCS += $(wildcard filesystems/uffs/*.c)
    CSRCS += $(wildcard filesystems/uffs/src/uffs/*.c)
endif

ifeq ($(CONFIG_RT_USING_DFS_ELMFAT),y)
    CSRCS += $(wildcard filesystems/elmfat/*.c)
    CSRCS := $(filter-out filesystems/elmfat/dfs_block_cache.c,$(CSRCS))
    ifeq ($(CONFIG_RT_DFS_ELM_BLK_CACHE),y)
        CSRCS += filesystems/elmfat/dfs_block_cache.c
    endif
    ifeq ($(CONFIG_RT_DFS_ELM_CODE_PAGE_FILE),y)
        CSRCS += filesystems/elmfat/option/ccfile.c
    else
        ifeq ($(CONFIG_RT_DFS_ELM_CODE_PAGE), 932)
            CSRCS += filesystems/elmfat/option/cc932.c
        else
            ifeq ($(CONFIG_RT_DFS_ELM_CODE_PAGE), 936)
                CSRCS += filesystems/elmfat/option/cc936.c
            else
                ifeq ($(CONFIG_RT_DFS_ELM_CODE_PAGE), 949)
                    CSRCS += filesystems/elmfat/option/cc949.c
                else
                    ifeq ($(CONFIG_RT_DFS_ELM_CODE_PAGE), 950)
                        CSRCS += filesystems/elmfat/option/cc950.c
                    else
                        CSRCS += filesystems/elmfat/option/ccsbcs.c
                    endif
                endif
            endif
        endif
    endif
endif

ifeq ($(CONFIG_RT_USING_NFS),y)
    CSRCS += $(wildcard filesystems/nfs/*.c)
    CSRCS += $(wildcard filesystems/nfs/rpc/*.c)
endif

ifeq ($(CONFIG_RT_USING_UFFS),y)
    CSRCS += $(wildcard filesystems/uffs/*.c)
endif

ifeq ($(CONFIG_RT_USING_RAMFS),y)
    CSRCS += $(wildcard filesystems/ramfs/*.c)
endif

ifeq ($(CONFIG_RT_USING_ROMFS),y)
    CSRCS += $(wildcard filesystems/romfs/*.c)
endif

ifeq ($(CONFIG_RT_USING_DFS_YAFFS2),y)
    CSRCS += $(wildcard filesystems/yaffs2/*.c)
    LOCAL_CFLAGS += -Ifilesystems/yaffs2/
endif

ifeq ($(CONFIG_RT_USING_MTD_UBI),y)
    CSRCS += $(wildcard filesystems/ubifs/port/*.c)
    CSRCS += $(wildcard filesystems/ubifs/lib/*.c)
    CSRCS += $(wildcard filesystems/ubifs/lib/lzo/*.c)
    CSRCS += $(wildcard filesystems/ubifs/ubi/*.c)
    LOCAL_CFLAGS += -Ifilesystems/ubifs/include/
    LOCAL_CFLAGS += -Ifilesystems/ubifs/include/port/
    LOCAL_CFLAGS += -Ifilesystems/ubifs/lib/lzo/
    LOCAL_CFLAGS += -Ifilesystems/ubifs/lib/zlib/
    LOCAL_CFLAGS += -Ifilesystems/ubifs/ubi/
    LOCAL_CFLAGS += -Ifilesystems/ubifs/kernel/
    LOCAL_CFLAGS += -Ifilesystems/ubifs/kernel/
    LOCAL_CFLAGS += -Ifilesystems/ubifs/
    LOCAL_CFLAGS += -Ifilesystems/ubifs/ubifs
    LOCAL_CFLAGS += -I../drivers/nand/inc/
    LOCAL_CFLAGS += -I../platform/include/
    LOCAL_CFLAGS += -D__RT_UBI__
    LOCAL_CFLAGS += -D__UBOOT__
    LOCAL_CFLAGS += -D__KERNEL__
    LOCAL_CFLAGS += -D__RT_UBIPORT__
    LOCAL_CFLAGS += -DCPU_32BIT
    LOCAL_CFLAGS += -D__FULLHAN___
    ifeq ($(CONFIG_UBIFS_FS),y)
        CSRCS += $(wildcard filesystems/ubifs/*.c)
        CSRCS += $(wildcard filesystems/ubifs/ubifs/*.c)
    endif
    ifeq ($(CONFIG_UBIFS_TEST),y)
        CSRCS += $(wildcard filesystems/ubifs/test/*.c)
    endif
endif

include $(SDKROOT)/build/lib.mk
