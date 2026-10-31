include $(SDKROOT)/build/config.mk

LIBNAME := $(shell basename `pwd`)

# core
CSRCS := $(wildcard finsh*.c)
CSRCS += $(wildcard cmd.c)
CSRCS += $(wildcard shell.c)
CSRCS += $(wildcard symbol.c)
CSRCS += $(wildcard media_proc.c)
CSRCS += $(wildcard memrw.c)

ifeq ($(CONFIG_FINSH_USING_MSH), y)
    CSRCS += $(wildcard msh*.c)
endif

include $(SDKROOT)/build/lib.mk
