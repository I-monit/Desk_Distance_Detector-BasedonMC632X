-include $(SDKROOT)/build/rtt_config.mk

export CROSS_COMPILE=arm-fullhanv2-eabi-

QUIET ?= @
RM = -rm -rf
CP = cp -a

ifeq ($(V),1)
    QUIET =
endif
CC = $(CROSS_COMPILE)gcc
AS = $(CROSS_COMPILE)as
AR = $(CROSS_COMPILE)ar
LD = $(CROSS_COMPILE)ld
CXX = $(CROSS_COMPILE)g++
NM  = $(CROSS_COMPILE)nm
OBJCOPY = $(CROSS_COMPILE)objcopy
OBJDUMP = $(CROSS_COMPILE)objdump
SIZE    = $(CROSS_COMPILE)size

MKTHRDS := $(shell cat /proc/cpuinfo | grep processor | wc -l)
BUILD_MKDIR = if [ ! -d $(dir $@) ]; then mkdir -p $(dir $@); fi

ARFLAGS := cr

LIB_CFLAGS  = $(CFLAGS)
LIB_CFLAGS += $(INCFLAGS)
LIB_CFLAGS += $(LOCAL_CFLAGS)

LIB_CXXFLAGS  = $(CFLAGS)
LIB_CXXFLAGS += $(INCFLAGS)
LIB_CXXFLAGS += $(LOCAL_CXXFLAGS)

LIB_ASFLAGS  = $(ASFLAGS)
LIB_ASFLAGS += $(CFLAGS)
LIB_ASFLAGS += $(INCFLAGS)
LIB_ASFLAGS += $(LOCAL_ASFLAGS)

APP_CFLAGS  = $(CFLAGS)
APP_CFLAGS += $(SYSFLAGS) $(VIDEO_INC)
APP_CFLAGS += $(LOCAL_CFLAGS)
APP_CFLAGS += -include $(OUT)/appconfig.h

LDFLAGS += $(LOCAL_LINKFLAGS)
