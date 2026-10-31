export SDKROOT := $(shell pwd)

-include $(SDKROOT)/build/config.mk

all: lib app

$(SYSCONFIG):.config
	$(QUIET)$(BUILD_MKDIR)
	$(QUIET)python $(TOOLS_DIR)/cfg2hdr.py .config $(OUT)/rtconfig/rtconfig.h

%_defconfig %_fastconfig:
	$(QUIET)if [ -e $(SDKROOT)/configs/$@ ]; then \
				cp $(SDKROOT)/configs/$@ .config; \
			else \
				echo "$@ not exists."; \
			fi
	$(QUIET)if [ ! -e $(OUT)/rtconfig ]; then mkdir -p $(OUT)/rtconfig; fi
	$(QUIET)python $(TOOLS_DIR)/cfg2hdr.py .config $(OUT)/rtconfig/rtconfig.h
	$(QUIET)./build/tools/fastb.sh

lib: $(SYSCONFIG)
	$(QUIET)for dir in $(LIBDIRS); do make -j $(MKTHRDS) -C $$dir all || exit 1; done
	$(QUIET)echo "============================Lib Done==================================="
	$(QUIET)python $(TOOLS_DIR)/createsysinc.py $(SYSCONFIG) $(OUT)/appconfig.h

mem_layout:
	$(QUIET)$(CC) -E -P -I$(OUT)/rtconfig -Iplatform $(SDKROOT)/platform/mem.ld.S -o $(OUT)/lib/mem.ld

fixscript:
	$(QUIET)./build/tools/fastb.sh
	# $(QUIET)if [ "$(CONFIG_FH_CONFIG_COMPRESS_LZO)"x != x ]; then \
	#             sed -i 's/libzlib/libunlzo/' build/baseos.ld; \
    #             sed -i 's/libunlzma/libunlzo/' build/baseos.ld; \
    #         fi
	# $(QUIET)if [ "$(CONFIG_FH_CONFIG_COMPRESS_GZIP)"x != x ]; then \
	#             sed -i 's/libunlzma/libzlib/' build/baseos.ld; \
    #             sed -i 's/libunlzo/libzlib/' build/baseos.ld; \
    #         fi
	# $(QUIET)if [ "$(CONFIG_FH_CONFIG_COMPRESS_LZMA)"x != x ]; then \
	#             sed -i 's/libunlzo/libunlzma/' build/baseos.ld; \
    #             sed -i 's/libzlib/libunlzma/' build/baseos.ld; \
    #         fi

app: mem_layout fixscript
	$(QUIET)if [ -d $(SAMPLE_DIR) ]; then \
	            export SDKROOT=`pwd`; \
	            export OS=RTT; \
	            make -C $(SAMPLE_DIR) OS=RTT SOC_LINK=$(SOC_LINK) QUIET=$(QUIET) all; \
	        else \
	            echo "$(SAMPLE_DIR) doesn't exists."; \
	        fi
	$(QUIET)if [ -e local_post_build.sh ]; then ./local_post_build.sh; fi

clean: clean_lib clean_app
	$(QUIET)$(RM) $(OUT)/lib/mem.ld

clean_app:
	@echo "Cleaning application."
	$(QUIET)if [ "$(SAMPLE_DIR)"x != x ] && [ -d $(SAMPLE_DIR) ]; then \
                export SDKROOT=`pwd`; \
                export OS=RTT; \
                make clean -s -C $(SAMPLE_DIR); \
            else \
                echo 'SAMPLE_DIR not defined!'; \
	        fi
	$(QUIET)$(RM) $(SENSFILE) $(HEX_OBJ_FILE)

clean_lib:
	@echo "Cleaning libs..."
	$(QUIET)for dir in $(LIBDIRS); do make clean -s -C $$dir; done

distclean: clean
	$(QUIET)$(RM) .config
	$(QUIET)$(RM) $(SYSCONFIG)
	$(QUIET)$(RM) $(OUT)/appconfig.h

include $(SDKROOT)/build/menuconfig.mk

.PHONY: clean clean_app clean_lib distclean lib app all menuconfig appconfig

env:
	@build/tools/makenv.sh test

# create Makefile.app template for application
appmake:
	@python3 build/tools/appmake.py "CFLAGS" $(CFLAGS)  $(SYSFLAGS) $(VIDEO_INC)
	@python3 build/tools/appmake.py "LDFLAGS" $(LDFLAGS)
	@python3 build/tools/appmake.py "LNKLIBS" $(LNKLIBS)
