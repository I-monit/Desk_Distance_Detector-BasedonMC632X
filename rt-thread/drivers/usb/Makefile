include $(SDKROOT)/build/config.mk

LIBNAME := $(shell basename `pwd`)

LOCAL_CFLAGS := -I./include/
LOCAL_CFLAGS += -I./host/
CSRCS :=

ifeq ($(CONFIG_RT_USING_UHC),y)
    CSRCS += $(wildcard fh_otg/fh_common_port/*.c)
    CSRCS += $(wildcard fh_otg/fh_otg/*.c)
    CSRCS += host/buffer.c host/config.c host/devices.c host/driver.c
    CSRCS += host/generic.c host/hcd.c host/hub.c host/message.c host/quirks.c
    CSRCS += host/urb.c host/usb.c

    ifeq ($(CONFIG_RT_USING_USB_MAC),y)
        CSRCS += host/net/usbnet.c
    endif

    ifeq ($(CONFIG_USB_MAC_USING_ASIX),y)
        CSRCS += host/net/asix.c
    endif
    ifeq ($(CONFIG_RT_USING_USB_RNDIS),y)
        CSRCS += host/net/cdc/rndis_host.c
    endif
    ifeq ($(CONFIG_RT_USING_USB_TEST),y)
        CSRCS += host/usbtest.c
    endif

    ifeq ($(CONFIG_RT_USING_USB_STORAGE),y)
        CSRCS += host/usb_storage.c
    endif

    ifeq ($(CONFIG_RT_USING_USB_SERIAL),y)
        CSRCS += host/serial/usb_serial.c
    endif

    ifeq ($(CONFIG_RT_USING_USB_CDC_ACM),y)
        CSRCS += host/serial/cdc-acm.c
    endif

    ifeq ($(CONFIG_RT_USING_USB_CDC_ACM),y)
        CSRCS += host/net/cdc/cdc-ether.c
    else ifeq ($(CONFIG_RT_USING_USB_RNDIS),y)
        CSRCS += host/net/cdc/cdc-ether.c
    else ifeq ($(CONFIG_RT_USING_USB_ECM),y)
        CSRCS += host/net/cdc/cdc-ether.c
    endif

    ifeq ($(CONFIG_RT_USING_USB_SERIAL),y)
        CSRCS += host/serial/usb_serial_skel.c
    else ifeq ($(CONFIG_RT_USING_USB_CDC_ACM),y)
        CSRCS += host/serial/usb_serial_skel.c
    endif

endif

ifeq ($(CONFIG_FH_RT_USB_DEVICE),y)
    CSRCS += $(wildcard device/udc/*.c)
    CSRCS += $(wildcard device/core/*.c)

    ifeq ($(CONFIG_FH_RT_USB_DEVICE_UVC),y)
        CSRCS += device/class/f_uvc.c device/class/libuvc_ctrl.c
        CSRCS += device/class/libuvc.c device/class/uvc_init.c device/class/uvc_vs_desc.c
    endif

    ifeq ($(CONFIG_FH_RT_USB_DEVICE_UAC),y)
        CSRCS += device/class/f_uac.c device/class/uac_init.c
    endif

    ifeq ($(CONFIG_FH_RT_USB_DEVICE_CDC),y)
        CSRCS += device/class/cdc_cmd_dbi.c device/class/cdc_vcom.c
    endif

    ifeq ($(CONFIG_FH_RT_USB_DEVICE_HID),y)
        CSRCS += device/class/hid.c
    endif

    ifeq ($(CONFIG_FH_RT_USB_DEVICE_MSTORAGE),y)
        CSRCS += device/class/mstorage.c device/class/ram_block_dev.c
    endif

    ifeq ($(CONFIG_FH_RT_USB_DEVICE_RNDIS),y)
        CSRCS += device/class/rndis.c
    endif
endif

ifeq ($(CONFIG_RT_USING_USB_DEVICE),y)

    ifeq ($(CONFIG_APP_CONFIG_UVC_UPDATE), y)
        CSRCS += $(wildcard usbdevice/udc/*.c)
        CSRCS += $(wildcard usbdevice/core/*.c)
        CSRCS += usbdevice/class/FlashBase.c usbdevice/class/update_common.c usbdevice/class/dfu.c

        ifeq ($(CONFIG_RT_USB_DEVICE_CDC),y)
            CSRCS += usbdevice/class/cdc_vcom.c usbdevice/class/update_com.c
        endif

        ifeq ($(CONFIG_RT_USB_DEVICE_HID),y)
            CSRCS += usbdevice/class/hid.c
        endif

        ifeq ($(CONFIG_RT_USB_DEVICE_MSTORAGE),y)
            CSRCS += usbdevice/class/mstorage.c usbdevice/class/decode_fat32.c usbdevice/class/update_fw.c
        endif
    else
        CSRCS += $(wildcard device_rtt/udc/*.c)
        CSRCS += $(wildcard device_rtt/core/*.c)

        ifeq ($(CONFIG_RT_USB_DEVICE_CDC),y)
            CSRCS += device_rtt/class/cdc_vcom.c
        endif

        ifeq ($(CONFIG_RT_USB_DEVICE_HID),y)
            CSRCS += device_rtt/class/hid.c
        endif

        ifeq ($(CONFIG_RT_USB_DEVICE_MSTORAGE),y)
            CSRCS += device_rtt/class/mstorage.c
        endif
    endif

    ifeq ($(CONFIG_RT_USB_DEVICE_ECM),y)
    	CSRCS += device_rtt/class/ecm.c
    endif

    ifeq ($(CONFIG_RT_USB_DEVICE_RNDIS),y)
    	CSRCS += device_rtt/class/rndis.c
    endif

    ifeq ($(CONFIG_RT_USB_DEVICE_WINUSB),y)
    	CSRCS += device_rtt/class/winusb.c
    endif
endif

include $(SDKROOT)/build/lib.mk
