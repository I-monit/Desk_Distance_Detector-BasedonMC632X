include $(SDKROOT)/build/config.mk

LIBNAME := $(shell basename `pwd`)


LOCAL_CFLAGS := -I./

ifeq ($(CONFIG_CONFIG_ARCH_FH8626V100),y)
    CSRCS += $(wildcard ./fh_sadc_v2.c)
endif

ifeq ($(CONFIG_CONFIG_ARCH_FH885xV200),y)
    CSRCS += $(wildcard ./fh_sadc_v3.c)
endif

ifeq ($(CONFIG_CONFIG_ARCH_FH865x), y)
    CSRCS :=  $(wildcard ./fh_sadc_v3.c)
endif

ifeq ($(CONFIG_CONFIG_ARCH_FH8636_FH8852V20X), y)
    CSRCS :=  $(wildcard ./fh_sadc_v3.c)
endif

ifeq ($(CONFIG_CONFIG_ARCH_FH8852V301_FH8662V100), y)
    CSRCS :=  $(wildcard ./fh_sadc_v3.c)
endif

ifeq ($(CONFIG_CONFIG_ARCH_FH885xV300),y)
    CSRCS += $(wildcard ./fh_sadc_v3.c)
endif

ifeq ($(CONFIG_CONFIG_ARCH_FH885xV310),y)
    CSRCS += $(wildcard ./fh_sadc_v3.c)
endif

ifeq ($(CONFIG_CONFIG_ARCH_FH8862),y)
    CSRCS += $(wildcard ./fh_sadc_v3.c)
endif

ifeq ($(CONFIG_CONFIG_ARCH_MC632X),y)
    CSRCS += $(wildcard ./fh_sadc_v3.c)
endif

ifeq ($(CONFIG_CONFIG_ARCH_FH885xV500),y)
    CSRCS += $(wildcard ./fh_sadc_v3.c)
endif

ifeq ($(CONFIG_CONFIG_ARCH_FH8626V300),y)
    CSRCS += $(wildcard ./fh_sadc_v3.c)
endif

include $(SDKROOT)/build/lib.mk
