#ifndef __GIC_H__
#define __GIC_H__

#define GIC_CPU_CTRL            0x00
#define GIC_CPU_PRIMASK         0x04
#define GIC_CPU_BINPOINT        0x08
#define GIC_CPU_INTACK          0x0c
#define GIC_CPU_EOI             0x10
#define GIC_CPU_RUNNINGPRI      0x14
#define GIC_CPU_HIGHPRI         0x18

#define GIC_DIST_CTRL           0x000
#define GIC_DIST_CTR            0x004
#define GIC_DIST_GRP            0x080
#define GIC_DIST_ENABLE_SET     0x100
#define GIC_DIST_ENABLE_CLEAR   0x180
#define GIC_DIST_PENDING_SET    0x200
#define GIC_DIST_PENDING_CLEAR  0x280
#define GIC_DIST_ACTIVE_BIT     0x300
#define GIC_DIST_PRI            0x400
#define GIC_DIST_TARGET         0x800
#define GIC_DIST_CONFIG         0xc00
#define GIC_DIST_NSACR          0xe00
#define GIC_DIST_SOFTINT        0xf00

#define GICC_IAR_INT_ID_MASK    0x3ff

#define MAX_GIC_NR              1

#define NR_IRQS                 (128 + 32)

#endif
