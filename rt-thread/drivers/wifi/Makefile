include $(SDKROOT)/build/config.mk

LIBNAME := $(shell basename `pwd`)

LOCAL_CFLAGS := -I./
LOCAL_CFLAGS += -I./pub/
LOCAL_CFLAGS += -I./pub/api_wifi/

CSRCS :=
CSRCS += $(wildcard pub/*.c)
CSRCS += $(wildcard pub/api_wifi/*.c)

include $(SDKROOT)/build/lib.mk