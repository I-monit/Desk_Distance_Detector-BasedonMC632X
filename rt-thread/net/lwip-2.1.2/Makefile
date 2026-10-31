include $(SDKROOT)/build/config.mk

LIBNAME := $(shell basename `pwd`)

LOCAL_CFLAGS :=
# core
CSRCS := $(wildcard src/arch/*.c)
CSRCS += $(wildcard src/api/*.c)
CSRCS += $(wildcard src/netif/*.c)
IPV4_SRCS := $(wildcard src/core/ipv4/*.c)
IPV4S := $(filter-out src/core/ipv4/dhcp.c,$(IPV4_SRCS))
CSRCS += $(filter-out src/core/ipv4/dhcp_server.c,$(IPV4S))
CORES := $(wildcard src/core/*.c)
CSRCS += $(filter-out src/core/mem.c,$(CORES))
CSRCS += $(wildcard src/apps/utils/ping.c)
CSRCS += $(wildcard src/apps/utils/net_tools.c)

ifeq ($(CONFIG_RT_USING_TFTP), y)
    CSRCS += $(wildcard src/apps/utils/tftp_client.c)
endif

ifeq ($(CONFIG_RT_USING_TELNET), y)
    CSRCS += $(wildcard src/apps/utils/telnet.c)
endif

ifeq ($(CONFIG_RT_LWIP_DHCP), y)
    CSRCS += $(wildcard src/core/ipv4/dhcp.c)
endif

ifeq ($(CONFIG_RT_USING_DHCPD), y)
    CSRCS += $(wildcard src/core/ipv4/dhcp_server.c)
endif

ifeq ($(CONFIG_RT_LWIP_SNMP), y)
    CSRCS += $(wildcard src/apps/snmp/*.c)
    LOCAL_CFLAGS += -Isrc/apps/snmp
endif

ifeq ($(CONFIG_RT_LWIP_PPP), y)
    CSRCS += $(wildcard src/netif/ppp/*.c)
    CSRCS += $(wildcard src/netif/ppp/polarssl/*.c)
    LOCAL_CFLAGS += -Isrc/netif/ppp
endif

ifeq ($(CONFIG_RT_USING_LWIP_IPV6), y)
    CSRCS += $(wildcard src/core/ipv6/*.c)
    CSRCS += $(wildcard src/apps/utils/ping6.c)
endif

include $(SDKROOT)/build/lib.mk
