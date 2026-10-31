include $(SDKROOT)/build/config.mk

LIBNAME := $(shell basename `pwd`)


LOCAL_CFLAGS = -I./
LOCAL_CFLAGS += -Isrtcore/
LOCAL_CFLAGS += -Ihaicrypt/
LOCAL_CXXFLAGS = -I./
LOCAL_CXXFLAGS += -Isrtcore/
LOCAL_CXXFLAGS += -Ihaicrypt/
CFLAGS += -DENABLE_LOGGING=1 -DENABLE_HEAVY_LOGGING=1
CFLAGS += -DHAI_ENABLE_SRT=1 -DHAI_PATCH=1 -DHAVE_INET_PTON=1
CFLAGS += -DSRT_ENABLE_BINDTODEVICE -DSRT_ENABLE_CLOSE_SYNCH -DSRT_VERSION=\"1.4.2\"

CSRCS += \
	srtcore/srt_compat.c \
	srtcore/api.cpp \
	srtcore/buffer.cpp \
	srtcore/cache.cpp \
	srtcore/channel.cpp \
	srtcore/common.cpp \
	srtcore/congctl.cpp \
	srtcore/core.cpp \
	srtcore/crypto.cpp \
	srtcore/epoll.cpp \
	srtcore/fec.cpp \
	srtcore/handshake.cpp \
	srtcore/list.cpp \
	srtcore/logger_default.cpp \
	srtcore/logger_defs.cpp \
	srtcore/md5.cpp \
	srtcore/packet.cpp \
	srtcore/packetfilter.cpp \
	srtcore/queue.cpp \
	srtcore/srt_c_api.cpp \
	srtcore/sync.cpp \
	srtcore/sync_posix.cpp \
	srtcore/window.cpp

ifeq ($(CONFIG_FH_ENABLE_ENCRYPTION), y)
CFLAGS += -DSRT_ENABLE_ENCRYPTION=1 -DUSE_MBEDTLS
CSRCS += \
	haicrypt/haicrypt_log.cpp \
	haicrypt/cryspr.c \
	haicrypt/hcrypt.c \
	haicrypt/hcrypt_ctx_rx.c \
	haicrypt/hcrypt_ctx_tx.c \
	haicrypt/hcrypt_rx.c \
	haicrypt/hcrypt_sa.c \
	haicrypt/hcrypt_tx.c \
	haicrypt/hcrypt_xpt_srt.c \
	haicrypt/cryspr-mbedtls.c

endif

ifeq ($(CONFIG_FH_EXPERIMENTAL_BONDING), y)
CFLAGS += -DENABLE_EXPERIMENTAL_BONDING
CSRCS += srtcore/group.cpp
endif

# core
# CSRCS := $(wildcard *.cpp)

include $(SDKROOT)/build/lib.mk