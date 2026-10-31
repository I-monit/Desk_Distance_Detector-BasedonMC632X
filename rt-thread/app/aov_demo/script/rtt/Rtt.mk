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
# CFLAGS += -Wall -Werror

A7_CPU_FLAGS = -mcpu=cortex-a7
A7_VFP_FLAGS = -mfloat-abi=hard -mfpu=neon-vfpv4

ARMV6_CPU_FLAGS = -mcpu=arm1176jzf-s
ARMV6_VFP_FLAGS = -mfloat-abi=softfp -mfpu=vfp

ifeq ($(CHIP_ID), FH8626V300)
	ARM_CORTEXA7_VFP ?= 0
else
	ARM_CORTEXA7_VFP ?= 1
endif
ifeq ($(DEBUG),1)
CFLAGS += -g
endif

#LIBS = -lsensor_rtt -lmipi_rtt -lrtt_isp -lisp_rtt -lispcore_rtt -ldbi_rtt -lm
#LIBS += -lmedia_process_rtt -ldsp_rtt
#LIBS += -ladvapi_osd -ladvapi_isp -ladvapi_smartir -ladvapi_md -ladvapi -lrpc_customer
LIBS = -lrpc_customer

ifeq ($(CHIP_ID), MC632X)
	LIBS += -lveu_armrtt_xbus
endif

ifeq ($(CHIP_ID), FH8626V300)
	LIBS += -ldsprpc_leader_rtt
endif

#sample_demo start
SRCS += $(wildcard *.c)
SRCS += $(wildcard libpes/src/*.c)
SRCS += $(wildcard librecord/src/*.c)

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
INCS += -I$(SDKROOT)/lib/$(CHIP_ID_LOWER)/inc/bsp
INCS += -I$(SDKROOT)/lib/$(CHIP_ID_LOWER)/inc/dsp
INCS += -I$(SDKROOT)/lib/$(CHIP_ID_LOWER)/inc/isp
INCS += -I$(SDKROOT)/lib/$(CHIP_ID_LOWER)/inc/isp_ext
INCS += -I$(SDKROOT)/lib/$(CHIP_ID_LOWER)/inc/vicap
INCS += -Ilibpes/include
INCS += -Ilibrecord/include

ifeq ($(ARM_CORTEXA7_VFP), 1)
LDFLAGS += $(A7_CPU_FLAGS) $(A7_VFP_FLAGS)
CFLAGS += $(A7_CPU_FLAGS) $(A7_VFP_FLAGS)
else
LDFLAGS += $(ARMV6_CPU_FLAGS) $(ARMV6_VFP_FLAGS)
CFLAGS += $(ARMV6_CPU_FLAGS) $(ARMV6_VFP_FLAGS)
endif

CFLAGS += $(INCS)

SAMPNAME := $(shell basename `pwd`)
SAMP_SRCS := $(SRCS)
LOCAL_CFLAGS := $(CFLAGS)
LOCAL_LIBS := $(LIBS)

include $(SDKROOT)/build/apps.mk

all:

clean:
	rm -f $(BIN)
	find . -name \*.o -exec rm -rf {} \;
	find . -name \*.d -exec rm -rf {} \;

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

.PHONY: clean

