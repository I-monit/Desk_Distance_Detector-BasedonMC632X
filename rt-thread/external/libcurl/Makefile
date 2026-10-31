include $(SDKROOT)/build/config.mk

LIBNAME := $(shell basename `pwd`)

# core
CSRCS := $(wildcard *.c)
CSRCS += $(wildcard vtls/*.c)
CSRCS += $(wildcard vauth/*.c)

LOCAL_CFLAGS := -I./


include $(SDKROOT)/build/lib.mk
