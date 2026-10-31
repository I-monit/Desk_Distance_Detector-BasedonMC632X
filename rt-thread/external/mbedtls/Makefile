include $(SDKROOT)/build/config.mk

LIBNAME := $(shell basename `pwd`)

# core
CSRCS := $(wildcard mbedtls/*.c)
LOCAL_CFLAGS := -I./include/

include $(SDKROOT)/build/lib.mk
