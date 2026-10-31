include $(SDKROOT)/build/config.mk

LIBNAME := $(shell basename `pwd`)

# core
CSRCS := miniwget.c minixml.c igd_desc_parse.c minisoap.c \
         miniupnpc.c upnpreplyparse.c upnpcommands.c upnperrors.c \
         connecthostport.c portlistingparse.c receivedata.c \
         upnpdev.c minissdpc.c upnpc.c

include $(SDKROOT)/build/lib.mk
