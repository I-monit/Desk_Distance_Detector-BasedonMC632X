
OBJOUT := $(SDKROOT)/out/obj/$(dir $(subst $(SDKROOT),,$(shell pwd)))$(LIBNAME)

CUR_PATH := $(shell pwd)

LIBA := $(OUT)/lib/lib$(LIBNAME).a

C_SRCS := $(filter %.c,$(CSRCS))
CXX_SRCS := $(filter %.cpp,$(CSRCS))
ASM_SRCS := $(filter %.S,$(CSRCS))

C_OBJS := $(patsubst %.c,$(OBJOUT)/%.o,$(C_SRCS))
CXX_OBJS := $(patsubst %.cpp,$(OBJOUT)/%.o,$(CXX_SRCS))
ASM_OBJS := $(patsubst %.S,$(OBJOUT)/%.o,$(ASM_SRCS))

HDRDEPENDS := $(subst .o,.d,$(C_OBJS) $(ASM_OBJS) $(CXX_OBJS))
# HDRDEPENDS := $(subst .o,.d,$(C_OBJS)) $(subst .o,.d,$(ASM_OBJS)) $(subst .o,.d,$(CXX_OBJS))

-include $(HDRDEPENDS)

all: $(LIBA)

$(LIBA): $(C_OBJS) $(CXX_OBJS) $(ASM_OBJS)
	$(QUIET)if [ -e $(LIBA) ]; then rm -f $(LIBA); fi       # force create
	$(QUIET)$(BUILD_MKDIR)
	$(QUIET)$(AR) $(ARFLAGS) $@ $(C_OBJS) $(CXX_OBJS) $(ASM_OBJS)

clean: clean_obj

clean_obj:
	$(QUIET)$(RM) $(LIBA) $(C_OBJS) $(ASM_OBJS) $(CXX_OBJS) $(HDRDEPENDS)

$(C_OBJS): $(OBJOUT)/%.o: %.c
	$(QUIET)$(BUILD_MKDIR)
	$(QUIET)echo "CC "$<
	$(QUIET)$(CC) $(LIB_CFLAGS) -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -c $(CUR_PATH)/$< -o $@

$(CXX_OBJS): $(OBJOUT)/%.o: %.cpp
	$(QUIET)$(BUILD_MKDIR)
	$(QUIET)echo "CXX "$<
	$(QUIET)$(CXX) $(LIB_CXXFLAGS) -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -c $(CUR_PATH)/$< -o $@

$(ASM_OBJS): $(OBJOUT)/%.o: %.S
	$(QUIET)$(BUILD_MKDIR)
	$(QUIET)echo "AS "$<
	$(QUIET)$(CC) $(LIB_ASFLAGS) -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -c $(CUR_PATH)/$< -o $@

.PHONY: all clean clean_obj
