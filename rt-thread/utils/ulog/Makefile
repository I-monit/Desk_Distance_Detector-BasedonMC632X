include $(SDKROOT)/build/config.mk

LIBNAME := $(shell basename `pwd`)

# core
CSRCS := $(wildcard *.c)

ifeq ($(CONFIG_ULOG_BACKEND_USING_CONSOLE), y)
    CSRCS += backend/console_be.c
endif

ifeq ($(CONFIG_ULOG_BACKEND_USING_FS), y)
    CSRCS += backend/fs_be.c
endif

ifeq ($(CONFIG_ULOG_USING_SYSLOG), y)
    CSRCS += $(wildcard syslog/*.c)
endif

include $(SDKROOT)/build/lib.mk
