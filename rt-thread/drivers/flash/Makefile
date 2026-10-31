include $(SDKROOT)/build/config.mk

LIBNAME := $(shell basename `pwd`)

# core
CSRCS := $(wildcard *.c)
CSRCS += $(wildcard sfud/src/*.c)
CSRCS += $(wildcard fal/src/*.c)

LOCAL_CFLAGS = -Ifal/inc

include $(SDKROOT)/build/lib.mk
