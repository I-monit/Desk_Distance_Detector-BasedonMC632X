include $(SDKROOT)/build/config.mk

LIBNAME := $(shell basename `pwd`)

CSRCS := $(wildcard mtd.c)

ifeq ($(CONFIG_RT_USING_MTD_NOR),y)
    CSRCS += $(wildcard mtd_nor.c)
endif

ifeq ($(CONFIG_RT_USING_MTD_NAND),y)
    CSRCS += $(wildcard mtd_nand.c)
endif

LOCAL_CFLAGS := -I./


include $(SDKROOT)/build/lib.mk
