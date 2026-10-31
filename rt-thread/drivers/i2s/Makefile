include $(SDKROOT)/build/config.mk

LIBNAME := $(shell basename `pwd`)

ifeq ($(CONFIG_FH_USING_MOL_I2S), y)
    CSRCS += $(wildcard mol_i2s.c)
else
    CSRCS += $(wildcard i2s.c)
endif

LOCAL_CFLAGS := -I./

include $(SDKROOT)/build/lib.mk
