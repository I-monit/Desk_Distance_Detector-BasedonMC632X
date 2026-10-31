include $(SDKROOT)/build/config.mk

SAMPNAME := $(shell basename `pwd`)

SAMP_SRCS := $(wildcard aes/*.c)
SAMP_SRCS += $(wildcard spi/*.c)
SAMP_SRCS += $(wildcard i2c/*.c)
SAMP_SRCS += $(wildcard rtc/*.c)
SAMP_SRCS += $(wildcard sadc/*.c)
SAMP_SRCS += $(wildcard sdio/*.c)
SAMP_SRCS += $(wildcard startup/*.c)
SAMP_SRCS += $(wildcard pwm/*.c)
SAMP_SRCS += $(wildcard gpio/*.c)
SAMP_SRCS += $(wildcard uart/*.c)

include $(SDKROOT)/build/apps.mk
