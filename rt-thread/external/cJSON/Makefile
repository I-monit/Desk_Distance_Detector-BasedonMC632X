include $(SDKROOT)/build/config.mk

LIBNAME := $(shell basename `pwd`)

# core
ifeq ($(RT_CJSON_VERSION_RAW),y)
    CSRCS := $(wildcard raw/*.c)
    LOCAL_CFLAGS += -Iraw
else
    CSRCS := $(wildcard v1.7.13/*.c)
    LOCAL_CFLAGS += -Iv1.7.13
endif

include $(SDKROOT)/build/lib.mk
