#include <rtconfig.h>

#define ALIGN		.align 4
#ifndef ENTRY
#define ENTRY(name) \
  .globl name; \
  ALIGN; \
  name:
#endif

#ifndef END
#define END(name) \
  .size name, .-name
#endif

#ifndef ENDPROC
#define ENDPROC(name) \
  .type name, %function; \
  END(name)
#endif
.text

.global rt_hw_interrupt_disable
.global rt_hw_interrupt_enable
.global __real_strtod
.global __real_strtof
//.global __wrapC__dtoa_r



ENTRY(__wrap_strtod)
 push    {r2-r10, lr}


 mov     r5, r0
 mov     r6, r1
 bl      rt_hw_interrupt_disable
 mov     r4, r0
 mov     r1, r6
 mov     r0, r5
 bl      __real_strtod
 mov     r5, r0
 mov     r0, r4
 bl      rt_hw_interrupt_enable
 mov     r0,r5

 pop     {r2-r10, pc}
ENDPROC(__wrap_strtod)

ENTRY(__wrap_strtof)
 push    {r1-r10, lr}


 mov     r5, r0
 mov     r6, r1
 bl      rt_hw_interrupt_disable
 mov     r4, r0
 mov     r1, r6
 mov     r0, r5
 bl      __real_strtof
 mov     r5, r0
 mov     r0, r4
 bl      rt_hw_interrupt_enable
 mov     r0,r5

 pop     {r1-r10, pc}
ENDPROC(__wrap_strtof)
