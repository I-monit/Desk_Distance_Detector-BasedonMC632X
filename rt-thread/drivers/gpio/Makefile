include $(SDKROOT)/build/config.mk

LIBNAME := $(shell basename `pwd`)

# core
CSRCS := gpiolib.c

ifeq ($(CONFIG_RT_USING_MOL_GPIO), y)
    CSRCS += $(wildcard mol_gpio.c)
else
    CSRCS += $(wildcard gpio.c)
endif

include $(SDKROOT)/build/lib.mk
