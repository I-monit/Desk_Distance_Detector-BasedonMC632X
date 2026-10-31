 menuconfig:
	mconf Kconfig
	@python $(shell pwd)/script/menu_anno.py .config fastconfig.h $(CHIP_ID)
	@rm -f .config.old .config.cmd
	@chmod +x $(shell pwd)/script/sensor_hex_rtt.py
	@$(shell pwd)/script/sensor_hex_rtt.py fastconfig.h $(CHIP_ID) $(SDKROOT)
	@chmod 644 $(shell pwd)/script/sensor_hex_rtt.py

.PHONY: clean
