include $(SDKROOT)/build/config.mk

LIBNAME := $(shell basename `pwd`)

CSRCS := rtc.c

ifneq ($(CONFIG_RT_USING_ALARM),)
CSRCS += alarm.c
endif

CSRCS += fh_rtc.c fh_rtc_v2.c

LOCAL_CFLAGS := -I./


include $(SDKROOT)/build/lib.mk
