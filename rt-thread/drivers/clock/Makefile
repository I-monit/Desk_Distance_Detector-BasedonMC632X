include $(SDKROOT)/build/config.mk

LIBNAME := $(shell basename `pwd`)

# core
ifeq ($(CONFIG_CONFIG_ARCH_FH8626V100)$(CONFIG_CONFIG_ARCH_FH8636_FH8852V20X)$(CONFIG_CONFIG_ARCH_FH865x)$(CONFIG_CONFIG_ARCH_FH885xV200)$(CONFIG_CONFIG_ARCH_FH885xV300)$(CONFIG_CONFIG_ARCH_FH885xV310), y)
    CSRCS :=  clock.c fh_clock.c
else ifeq ($(CONFIG_CONFIG_ARCH_MC632X), y)
    CSRCS :=  clock.c mol_clock.c
else
    CSRCS :=  clock.c fh_clockv2.c
endif

include $(SDKROOT)/build/lib.mk
