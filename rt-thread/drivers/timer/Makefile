include $(SDKROOT)/build/config.mk

LIBNAME := $(shell basename `pwd`)

# core
ifeq ($(CONFIG_RT_USING_ARCH_TIMER),y)
CSRCS := $(wildcard arch-timer.c)
endif

ifeq ($(CONFIG_RT_USING_MOL_TIMER),y)
CSRCS += $(wildcard mol_timer.c)
endif

ifeq ($(CONFIG_RT_USING_DW_TIMER),y)
CSRCS += $(wildcard fh_timer.c)
endif

include $(SDKROOT)/build/lib.mk
