include $(SDKROOT)/build/config.mk

LIBNAME := $(shell basename `pwd`)

LOCAL_CFLAGS := 
# core
CSRCS := prctl.c adapter.c find.c top.c rmdir.c
ifeq ($(CONFIG_RT_USING_DFS), y)
    CSRCS += ad_fs.c
endif

ifeq ($(CONFIG_RT_USING_TIMEKEEPING), y)
    CSRCS += tk_sleep.c
else
    CSRCS += rt_sleep.c
endif

ifeq ($(CONFIG_RT_USING_LWIP), y)
    CSRCS += ad_net.c
endif

include $(SDKROOT)/build/lib.mk
