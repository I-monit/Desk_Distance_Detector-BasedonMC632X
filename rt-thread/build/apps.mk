APPOUT ?= $(shell pwd)/out

TARGET := $(APPOUT)/bin/$(SAMPNAME).elf
TARGET_BIN = $(TARGET:.elf=.bin)
TARGET_DIS = $(TARGET:.elf=.dis)
TARGET_MAP = $(TARGET:.elf=.map)
TARGET_IMG = $(TARGET:.elf=.img)

SAMPC_SRCS := $(filter %.c,$(SAMP_SRCS))
SAMPX_SRCS := $(filter %.cpp,$(SAMP_SRCS))
SAMPS_SRCS := $(filter %.S,$(SAMP_SRCS))

SAMPC_OBJS := $(patsubst %.c,$(APPOUT)/%.o,$(SAMPC_SRCS))
SAMPX_OBJS := $(patsubst %.cpp,$(APPOUT)/%.o,$(SAMPX_SRCS))
SAMPS_OBJS := $(patsubst %.S,$(APPOUT)/%.o,$(SAMPS_SRCS))

SAMP_OBJS := $(SAMPC_OBJS) $(SAMPX_OBJS) $(SAMPS_OBJS)
SAMP_DEPS := $(SAMP_OBJS:.o=.d)

LDFLAGS += -Wl,-Map=$(TARGET_MAP)

-include $(SAMP_DEPS)

QUIET?=@

all:app

app: $(TARGET)
	$(QUIET)$(OBJDUMP) -d $(TARGET) > $(TARGET_DIS)
	$(QUIET)$(OBJCOPY) -O binary $(TARGET) $(TARGET_BIN)
	@echo "===========================Image Info=================================="
	$(QUIET)$(SIZE) $(TARGET)
	@$(NM) -S --size-sort -r $(TARGET) > $(TARGET:.elf=.size)
	@echo "generating flash image...."
	@$(SDKROOT)/build/tools/gengz.sh $(APPOUT)/bin/$(SAMPNAME)
	# @$(CP) $(TARGET_BIN) $(TFTPROOT)
	@echo "============================All Done==================================="

$(TARGET):$(SAMP_OBJS) # $(DEPLIBS) $(HEX_OBJ_FILE)
	$(QUIET)$(BUILD_MKDIR)
	$(QUIET)for libnm in $(DEPLIBS); do if [ ! -e $$libnm ]; then echo "please build libs first!"; exit 1;fi; done || exit 1
	@echo "Linking....."
	$(QUIET)$(CC) $(LDFLAGS) $(SAMP_OBJS) -Wl,--start-group $(LNKLIBS) $(LOCAL_LIBS) -Wl,--end-group -o $(TARGET)
	@echo "============================App Done==================================="

$(SAMPX_OBJS):$(APPOUT)/%.o:%.cpp
	$(QUIET)$(BUILD_MKDIR)
	$(QUIET)echo "CXX "$<
	$(QUIET)$(CXX) $(APP_CFLAGS) -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -c $< -o $@ || exit 1

$(SAMPC_OBJS):$(APPOUT)/%.o:%.c
	$(QUIET)$(BUILD_MKDIR)
	$(QUIET)echo "CC "$<
	$(QUIET)$(CC) $(APP_CFLAGS) -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -c $< -o $@ || exit 1

$(SAMPS_OBJS):$(APPOUT)/%.o:%.S
	$(QUIET)$(BUILD_MKDIR)
	$(QUIET)echo "ASSEMBLE "$<
	$(QUIET)$(CC) $(APP_CFLAGS) -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -c $< -o $@ || exit 1

clean: clean_obj

clean_obj:
	$(QUIET)$(RM) $(TARGET) $(TARGET_BIN) $(TARGET_DIS) $(TARGET_MAP) $(SAMP_OBJS) $(SAMP_DEPS)
	$(QUIET)$(RM) $(TARGET_IMG) $(TARGET:.elf=.size)

.PHONY: clean app all menuconfig clean_obj
