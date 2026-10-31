include $(SDKROOT)/build/config.mk

LIBNAME := $(shell basename `pwd`)

CSRCS := $(wildcard dm9051/*.c)
CSRCS += $(wildcard enc28j60/*.c)

LOCAL_CFLAGS := -I./


include $(SDKROOT)/build/lib.mk
