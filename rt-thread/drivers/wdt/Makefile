include $(SDKROOT)/build/config.mk

LIBNAME := $(shell basename `pwd`)

LOCAL_CFLAGS := -I./

ifeq ($(CONFIG_CONFIG_ARCH_MC632X),y)
	CSRCS += $(wildcard ./mol_wdt.c)
	CSRCS += $(wildcard ./wdt_mol.c)
else
	CSRCS += $(wildcard ./fh_wdt.c)
	CSRCS += $(wildcard ./wdt.c)
endif

CSRCS += $(wildcard ./watchdog.c)
include $(SDKROOT)/build/lib.mk
