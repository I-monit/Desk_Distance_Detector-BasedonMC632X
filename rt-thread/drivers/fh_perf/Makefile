include $(SDKROOT)/build/config.mk

LIBNAME := $(shell basename `pwd`)

ifeq ($(CONFIG_FH_PERF_V2), y)
    CSRCS := fh_perf_mon_v2.c
endif
ifeq ($(CONFIG_FH_PERF_V1_0), y)
    CSRCS := fh_perf_mon.c
endif
ifeq ($(CONFIG_FH_PERF_V1_1), y)
    CSRCS := fh_perf_mon.c
endif

include $(SDKROOT)/build/lib.mk
