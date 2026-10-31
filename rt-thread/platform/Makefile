include $(SDKROOT)/build/config.mk

# LIBNAME := board
LIBNAME := $(shell basename `pwd`)

# core
CSRCS := $(wildcard $(SOC_NAME)/*.c)
CSRCS += $(wildcard loader/*.c)
CSRCS += $(wildcard startup/*.c)

CSRCS += $(wildcard $(SOC_NAME)/$(BOARD_NAME)/*.c)

ifeq ($(CONFIG_FH_FAST_BOOT), y)
ifeq ($(CONFIG_CONFIG_CHIP_FH8852V201), y)
    CSRCS += $(wildcard fh8852v201/fastboot/*.c)
endif
ifeq ($(CONFIG_CONFIG_CHIP_FH8852V301), y)
    CSRCS += $(wildcard fh8852v301/fastboot/*.c)
endif
ifeq ($(CONFIG_CONFIG_CHIP_FH8662V100), y)
    CSRCS += $(wildcard fh8662v100/fastboot/*.c)
endif
ifeq ($(CONFIG_CONFIG_CHIP_FH8852V310), y)
    CSRCS += $(wildcard fh8852v310/fastboot/*.c)
endif
ifeq ($(CONFIG_CONFIG_CHIP_FH8856V310), y)
    CSRCS += $(wildcard fh8856v310/fastboot/*.c)
endif
ifeq ($(CONFIG_CONFIG_CHIP_FH8858V310), y)
    CSRCS += $(wildcard fh8858v310/fastboot/*.c)
endif
ifeq ($(CONFIG_CONFIG_CHIP_MC632X), y)
    CSRCS += $(wildcard mc632x/fastboot/*.c)
endif
endif

CSRCS += $(wildcard common/board_info.c) $(wildcard common/calibrate.c) \
$(wildcard common/chkenv.c) $(wildcard common/fh_chipid.c) \
$(wildcard common/fh_pmu.c) $(wildcard common/media_proc.c) \
$(wildcard common/trap.c) $(wildcard common/timer.c)

ifeq ($(CONFIG_CONFIG_ARCH_MC632X), y)
	CSRCS += $(wildcard common/mol_pinctrl.c)
else
	CSRCS += $(wildcard common/pinctrl.c)
endif

include $(SDKROOT)/build/lib.mk
