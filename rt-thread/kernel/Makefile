include $(SDKROOT)/build/config.mk

LIBNAME := $(shell basename `pwd`)

LOCAL_CFLAGS :=
# core
CSRCS := 
CSRCS += src/block.c src/clock.c src/idle.c src/ipc.c src/irq.c src/irqdesc.c
CSRCS += src/kservice.c src/object.c src/scheduler.c src/thread.c src/timer.c
CSRCS += src/cpu.c

ifeq ($(CONFIG_RT_USING_COMPONENTS_INIT),y)
    CSRCS += src/components.c
endif
ifeq ($(CONFIG_RT_ENABLE_RUNNING_LOG),y)
    CSRCS += src/runlog.c
endif

ifeq ($(CONFIG_RT_USING_HEAP),y)
    CSRCS += src/mem.c src/slab_fh.c
endif
ifeq ($(CONFIG_RT_USING_MEMHEAP),y)
    CSRCS += src/memheap.c
endif
ifeq ($(CONFIG_RT_USING_DEVICE),y)
    CSRCS += src/device.c
endif
ifeq ($(CONFIG_RT_USING_MEMPOOL),y)
    CSRCS += src/mempool.c
endif
ifeq ($(CONFIG_RT_USING_NEWLIB),y)
    # extension
    CSRCS += $(wildcard utilities/*.c)
    # time
    CSRCS += $(wildcard libc/time/*.c)
    # libc
    CSRCS += $(wildcard libc/compilers/common/*.c)
    CSRCS += $(wildcard libc/compilers/newlib/*.c)
endif
# aio
ifeq ($(CONFIG_RT_USING_POSIX_AIO),y)
    CSRCS += $(wildcard libc/aio/*.c)
endif
ifeq ($(CONFIG_RT_USING_MODULE),y)
    CSRCS += $(wildcard libc/libdl/*.c)
    CSRCS += $(wildcard libc/libdl/arch/*.c)
endif
ifeq ($(CONFIG_RT_USING_POSIX_MMAP),y)
    CSRCS += $(wildcard libc/mmap/*.c)
endif
ifeq ($(CONFIG_RT_USING_SIGNALS),y)
    CSRCS += $(wildcard libc/signal/*.c)
    CSRCS += $(wildcard src/signal.c)
endif

ifeq ($(CONFIG_RT_USING_POSIX_TERMIOS),y)
    CSRCS += $(wildcard libc/termios/*.c)
endif

ifeq ($(CONFIG_RT_USING_TIMEKEEPING),y)
    CSRCS += $(wildcard libc/time/hrtime/*.c)
    LOCAL_CFLAGS += -Ilibc/time/hrtime
endif

ifeq ($(CONFIG_RT_USING_CPLUSPLUS),y)
    CSRCS += $(wildcard cplusplus/*.cpp)
    CSRCS += $(wildcard cplusplus/*.c)
endif

ifeq ($(CONFIG_RT_USING_PM),y)
    CSRCS += $(wildcard pm/*.c)
endif

include $(SDKROOT)/build/lib.mk
