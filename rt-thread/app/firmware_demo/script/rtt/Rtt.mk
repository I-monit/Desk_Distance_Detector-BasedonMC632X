export ROOT := $(shell pwd)
export SDKROOT := $(shell pwd)/../..

CONFIG_PATH = .config
ifneq (.config, $(wildcard .config))
CONFIG_PATH = .def.config
endif

-include $(CONFIG_PATH)
-include $(SDKROOT)/build/config.mk

export CONFIG_ARCH_$(CHIP_ID) = y
CFLAGS += -D__RTTHREAD_OS__
CFLAGS += -Wall -Werror

A7_CPU_FLAGS = -mcpu=cortex-a7
A7_VFP_FLAGS = -mfloat-abi=hard -mfpu=neon-vfpv4

ARM_CORTEXA7_VFP ?= 1

ifeq ($(DEBUG),1)
CFLAGS += -g
endif

LIBS = -ldbi_rtt -lm -ladvapi_osd -ladvapi_isp -ladvapi_smartir -ladvapi_md -ladvapi

#sample_demo start
SRCS = $(wildcard startup/demo_entry_rtt.c)
SRCS += $(wildcard vlcview/*.c)

#kernel include
INCS = -I./
INCS += -I$(SDKROOT)/kernel/include/

#driver include src
INCS = -I./
INCS += -I$(SDKROOT)/lib/inc
INCS += -I$(SDKROOT)/lib/inc/advapi
INCS += -I$(SDKROOT)/lib/inc/mpp
INCS += -I$(SDKROOT)/lib/inc/types
INCS += -I$(SDKROOT)/lib/$(CHIP_ID_LOWER)/inc
INCS += -I$(SDKROOT)/lib/$(CHIP_ID_LOWER)/inc/dsp
INCS += -I$(SDKROOT)/lib/$(CHIP_ID_LOWER)/inc/dsp_ext
INCS += -I$(SDKROOT)/lib/$(CHIP_ID_LOWER)/inc/isp
INCS += -I$(SDKROOT)/lib/$(CHIP_ID_LOWER)/inc/isp_ext
INCS += -I$(SDKROOT)/lib/$(CHIP_ID_LOWER)/inc/vicap

#components include src
COMPONENTS_DIR = components
INCS += -I$(COMPONENTS_DIR)

# RTSP 出流 (DMC 容器 + librtsp)
SRCS += $(wildcard components/libdmc/src/libdmc.c)
SRCS += $(wildcard components/librtsp/src/rtsp.c)
SRCS += $(wildcard components/libdmc/src/libdmc_rtsp.c)
CFLAGS += -DFH_RTSP_ENABLE

# 适配层 (libsample_common: 文件读取/毫秒时钟等基础工具)
SRCS += $(wildcard components/libsample_common/src/*.c)

#sample_demo include src
SAMPLE_DEMO_DIR = sample_demo
INCS += -I$(SAMPLE_DEMO_DIR)
SRCS += $(wildcard $(SAMPLE_DEMO_DIR)/*.c)
SRCS += $(wildcard $(SAMPLE_DEMO_DIR)/stream/src/*.c)

#sample_common include src
SAMPLE_COMMON_DIR = sample_common
CHIP_DIR = $(SAMPLE_COMMON_DIR)/$(CHIP_ID)
DSP_DIR = $(CHIP_DIR)/dsp
ISP_DIR = $(CHIP_DIR)/isp
DSP_MODEL_DIR = $(shell find $(DSP_DIR) -maxdepth 1 -type d)
ISP_MODEL_DIR = $(shell find $(ISP_DIR) -maxdepth 1 -type d)
DSP_INC = $(foreach dir,$(DSP_MODEL_DIR),-I$(dir)/include)
ISP_INC = $(foreach dir,$(ISP_MODEL_DIR),-I$(dir)/include)
DSP_SRC = $(foreach dir,$(DSP_MODEL_DIR),$(wildcard $(dir)/src/*.c))
ISP_SRC = $(foreach dir,$(ISP_MODEL_DIR),$(wildcard $(dir)/src/*.c))
INCS += -I$(SAMPLE_COMMON_DIR) -I$(CHIP_DIR) -I$(DSP_DIR) -I$(ISP_DIR) $(DSP_INC) $(ISP_INC)
SRCS += $(wildcard $(SAMPLE_COMMON_DIR)/*.c) $(wildcard $(CHIP_DIR)/*.c) $(wildcard $(DSP_DIR)/*.c) $(wildcard $(ISP_DIR)/*.c) $(DSP_SRC) $(ISP_SRC)

export FH_SLT_TEST = y
ifeq ($(FH_SLT_TEST), y)
INCS += -I$(SAMPLE_COMMON_DIR)/slt_test/include
SRCS += $(wildcard $(SAMPLE_COMMON_DIR)/slt_test/src/*.c)
endif

-include script/rtt/chip/$(CHIP_ID)/media.mk

# LVGL 板端页面 (192x480 -> venc ch1 -> rtsp 562) 已移除: 板端 UI 由 PC 端 Qt_GUI 取代

ifeq ($(ARM_CORTEXA7_VFP), 1)
LDFLAGS += $(A7_CPU_FLAGS) $(A7_VFP_FLAGS)
CFLAGS += $(A7_CPU_FLAGS) $(A7_VFP_FLAGS)
endif

CFLAGS += $(INCS)

# Snapshot SRCS/CFLAGS/LIBS for apps.mk (必须在 apps.mk include 之前)
SAMP_SRCS := $(SRCS)
LOCAL_CFLAGS := $(CFLAGS)
LOCAL_LIBS := $(LIBS)

SAMPNAME := $(shell basename `pwd`)

include $(SDKROOT)/build/apps.mk

all:

sensor_hex:
	@chmod +x script/rtt/sensor_hex_rtt.py
	@script/rtt/sensor_hex_rtt.py config.h $(CHIP_ID) $(SDKROOT)
	@chmod 644 script/rtt/sensor_hex_rtt.py

clean:
	rm -f $(BIN)
	find . -name \*.o -exec rm -rf {} \;
	find . -name \*.d -exec rm -rf {} \;

clean_hex:
	@if [ -e sample_common/all_sensor_array.h ]; then rm -f sample_common/all_sensor_array.h; fi
	@if [ -e sample_common/all_sensor_hex.bin ]; then rm -f sample_common/all_sensor_hex.bin; fi

distclean: clean clean_hex
	-rm config.h
	-rm .config
	-rm .config.old

%_defconfig:
	@if [ -e script/rtt/chip/$(CHIP_ID)/defconfig/$@ ];then \
		cp script/rtt/chip/$(CHIP_ID)/defconfig/$@ .config; \
	else \
		@echo "$@ not exists"; \
	fi
	@python script/rtt/menu_anno.py .config config.h $(CHIP_ID)
	@rm -f .config.old .config.cmd

menuconfig:
	@chmod +x script/rtt/mconf
	@script/rtt/mconf script/rtt/Kconfig
	@python script/rtt/menu_anno.py .config config.h $(CHIP_ID)
	@rm -f .config.old .config.cmd
	@chmod +x script/rtt/sensor_hex_rtt.py
	@script/rtt/sensor_hex_rtt.py config.h $(CHIP_ID) $(SDKROOT)
	@chmod 644 script/rtt/sensor_hex_rtt.py

# (snapshot 在文件上半部 apps.mk include 之前)

.PHONY: clean
