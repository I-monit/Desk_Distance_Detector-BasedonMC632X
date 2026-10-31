include $(SDKROOT)/build/config.mk

LIBNAME := $(shell basename `pwd`)

# core
CSRCS := fh_spi.c qspi_core.c spi_core.c spi_dev.c ssi.c

ifeq ($(CONFIG_RT_USING_SPISLAVE),y)
    CSRCS += spi_slave.c
endif

include $(SDKROOT)/build/lib.mk
