include $(SDKROOT)/build/config.mk

LIBNAME := $(shell basename `pwd`)

# core
ifeq ($(CONFIG_ARCH_ARM_CORTEX_A7),y)
    CSRCS := $(wildcard common/*.c)
    CSRCS += $(wildcard cortex-a/*.c)
    CSRCS += $(wildcard common/*.S)
    CSRCS += $(wildcard cortex-a/*.S)
    CSRCS += $(wildcard *.S)
    CSRCS += $(wildcard *.c)
endif
ifeq ($(CONFIG_ARCH_ARM_ARM11), y)
    CSRCS := $(wildcard common/*.c)
    CSRCS += $(wildcard armv6/*.c)
    CSRCS += $(wildcard common/*.S)
    CSRCS += $(wildcard armv6/*.S)
    CSRCS += $(wildcard *.S)
    CSRCS += $(wildcard *.c)
endif

include $(SDKROOT)/build/lib.mk
