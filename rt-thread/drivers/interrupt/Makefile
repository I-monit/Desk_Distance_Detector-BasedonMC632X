include $(SDKROOT)/build/config.mk

LIBNAME := $(shell basename `pwd`)

# core
ifeq ($(CONFIG_ARCH_ARM_ARM11),y)
CSRCS := $(wildcard dw/*.c)
else
CSRCS := $(wildcard gic/*.c)
endif

include $(SDKROOT)/build/lib.mk
