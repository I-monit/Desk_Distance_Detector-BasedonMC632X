include $(SDKROOT)/build/config.mk

LIBNAME := $(shell basename `pwd`)

# core
CSRCS := $(wildcard dma.c)
CSRCS += $(wildcard dma_memcpy.c)

ifeq ($(CONFIG_FH_USING_DMA_MEM), y)
    CSRCS += $(wildcard dma_mem.c)
endif

ifeq ($(CONFIG_FH_USING_DMA), y)
    CSRCS += $(wildcard fh_dma.c)
endif

ifeq ($(CONFIG_FH_USING_AXI_DMA), y)
    CSRCS += $(wildcard fh_axi_dma.c)
endif

ifeq ($(CONFIG_FH_USING_MOL_DMA), y)
    CSRCS += $(wildcard mol_dma.c)
endif


include $(SDKROOT)/build/lib.mk
