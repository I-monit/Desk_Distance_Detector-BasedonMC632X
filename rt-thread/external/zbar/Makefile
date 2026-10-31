include $(SDKROOT)/build/config.mk

LIBNAME := $(shell basename `pwd`)

# core
# CSRCS := $(wildcard src/*.c)
CSRCS := src/config.c src/decoder.c src/error.c src/image.c src/img_scanner.c
CSRCS += src/refcnt.c src/scanner.c src/symbol.c
CSRCS += src/decoder/qr_finder.c
CSRCS += src/processor/lock.c src/processor/null.c src/processor/posix.c
CSRCS += src/qrcode/bch15_5.c src/qrcode/binarize.c src/qrcode/isaac.c
CSRCS += src/qrcode/qrdec.c src/qrcode/qrdectxt.c src/qrcode/rs.c src/qrcode/util.c

LOCAL_CFLAGS += -Iinc/zbar -Isrc/

include $(SDKROOT)/build/lib.mk
