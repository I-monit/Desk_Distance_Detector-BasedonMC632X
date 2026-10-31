OS ?= LINUX

ifeq ($(OS), RTT)
make_file = ./script/rtt/Rtt.mk
endif

all:
	@make -f $(make_file) all

clean:
	@make -f $(make_file) clean

distclean:
	@make -f $(make_file) distclean

menuconfig:
	@make -f $(make_file) menuconfig

%_defconfig:
	@make -f $(make_file) $@
