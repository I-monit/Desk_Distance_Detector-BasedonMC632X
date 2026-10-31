include $(SDKROOT)/build/config.mk

LIBNAME := $(shell basename `pwd`)

LOCAL_CFLAGS=-O3 -D_LARGEFILE64_SOURCE=1 -DHAVE_HIDDEN

CSRCS := adler32.c crc32.c deflate.c infback.c inffast.c inflate.c inftrees.c trees.c zutil.c
CSRCS += compress.c uncompr.c gzclose.c gzlib.c gzread.c gzwrite.c

include $(SDKROOT)/build/lib.mk
