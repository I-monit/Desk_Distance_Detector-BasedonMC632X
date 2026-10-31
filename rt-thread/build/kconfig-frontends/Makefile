TARGET := kconfig-mconf

INCDIR := -Ilibs/parser -Ilibs

CFLAGS := -DCURSES_LOC=\"ncurses.h\" -DHAVE_CONFIG_H -DROOTMENU=\"Configuration\"
CFLAGS += -DCONFIG_=\"CONFIG_\" -DKBUILD_NO_NLS -D"GPERF_LEN_TYPE=unsigned int"
CFLAGS += $(INCDIR)

LDFLAGS := -lncurses -Wl,-Map=kconf.map

CSRCS := $(wildcard libs/lxdialog/*.c)
CSRCS += $(wildcard frontends/mconf/*.c)
CSRCS += libs/parser/yconf.c

COBJS := $(CSRCS:.c=.o)

all: $(TARGET)

$(TARGET):$(COBJS)
	$(CC) -o $@ $(COBJS) $(LDFLAGS)
	# $(CC) $(COBJS) $(LDFLAGS) -o $@

$(COBJS):%.o:%.c
	@echo "CC "$<
	$(CC) -c $< $(CFLAGS) -o $@

clean:
	@rm -f $(COBJS) $(TARGET)

.PHONY: all clean
