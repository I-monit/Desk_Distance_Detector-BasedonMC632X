include $(SDKROOT)/build/config.mk

LIBNAME := $(shell basename `pwd`)

# core
ifeq ($(CONFIG_FH_EFUSE_V1), y)
CSRCS := $(wildcard fh_efuse.c)
endif

ifeq ($(CONFIG_FH_EFUSE_V2), y)
CSRCS := $(wildcard fh_efuse_v2.c)
endif

include $(SDKROOT)/build/lib.mk
