#include "rtconfig.h"
#include "platform_def.h"

OUTPUT_FORMAT("elf32-littlearm", "elf32-littlearm", "elf32-littlearm")
OUTPUT_ARCH(arm)

BASEADDR = FH_DDR_START;
