include $(SDKROOT)/build/config.mk

LIBNAME := $(shell basename `pwd`)

# core
CSRCS := $(wildcard mDNSCore/*.c)
CSRCS += $(wildcard mDNSRtthread/*.c)
CSRCS += $(wildcard mDNSShared/*.c)

LOCAL_CFLAGS := -ImDNSCore -ImDNSRtthread -ImDNSShared
LOCAL_CFLAGS += -D_GNU_SOURCE -DNOT_HAVE_SA_LEN -fno-strict-aliasing

include $(SDKROOT)/build/lib.mk
