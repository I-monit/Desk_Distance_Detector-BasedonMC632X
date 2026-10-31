#include <rtdevice.h>
#include <rthw.h>
#include <fh_arch.h>
#include <fh_def.h>

#include "gic.h"
#include "irqdesc.h"
#ifdef RT_USING_PM
#include <pm.h>
#endif

#define INTC_DEV_NAME "intc_dev"

struct gic_chip_data
{
    unsigned int irq_offset;
    unsigned int dist_base;
    unsigned int cpu_base;
};

static struct gic_chip_data gic_data[MAX_GIC_NR];
static unsigned int gic_cpu_base_addr = 0;

void rt_hw_ipi_handler_install(int ipi_vector, rt_isr_handler_t ipi_isr_handler)
{
    rt_hw_interrupt_install(ipi_vector, ipi_isr_handler, 0, "IPI_HANDLER");
}

static void gic_dist_init(struct gic_chip_data *gic,
    unsigned int irq_start)
{
    unsigned int gic_irqs, i;
    unsigned int base = gic->dist_base;
    unsigned int cpumask = 1 << 0;

    cpumask = 1 << 0;
    cpumask |= cpumask << 8;
    cpumask |= cpumask << 16;

    SET_REG(base + GIC_DIST_CTRL, 0);
    /*
     * Find out how many interrupts are supported.
     * The GIC only supports up to 1020 interrupt sources.
     */
    gic_irqs = GET_REG(base + GIC_DIST_CTR) & 0x1f;
    gic_irqs = (gic_irqs + 1) * 32;
    if (gic_irqs > 1020)
        gic_irqs = 1020;
    /*
     * Set all global interrupts to be level triggered, active low.
     */
    for (i = 32; i < gic_irqs; i += 16)
        SET_REG(base + GIC_DIST_CONFIG + i * 4 / 16, 0);

    /*
     * Set all global interrupts to this CPU only.
     */
    for (i = 32; i < gic_irqs; i += 4)
        SET_REG(base + GIC_DIST_TARGET + i * 4 / 4, cpumask);

    /*
     * Set priority on all global interrupts.
     */
    for (i = 32; i < gic_irqs; i += 4)
        SET_REG(base + GIC_DIST_PRI + i * 4 / 4, 0xa0a0a0a0);

    /*
     * Disable all interrupts.  Leave the PPI and SGIs alone
     * as these enables are banked registers.
     */
    for (i = 32; i < gic_irqs; i += 32)
        SET_REG(base + GIC_DIST_ENABLE_SET + i * 4 / 32, 0x0);

    SET_REG(base + GIC_DIST_CTRL, 1);
}

void gic_rebasetoGrp1(void)
{
    int i = 0;

    for (i = 0; i < 96/32; i++)
        SET_REG(GICD_BASE + GIC_DIST_GRP  + i * 4, 0xffffffff);
}

static void  gic_cpu_init(struct gic_chip_data *gic)
{
    unsigned int dist_base = gic->dist_base;
    unsigned int base = gic->cpu_base;
    int i;

    /*
     * Deal with the banked PPI and SGI interrupts - disable all
     * PPI interrupts, ensure all SGI interrupts are enabled.
     */
    SET_REG(dist_base + GIC_DIST_ENABLE_CLEAR, 0xffff0000);
    SET_REG(dist_base + GIC_DIST_ENABLE_SET, 0x0000ffff);

    /*
     * Set priority on PPI and SGI interrupts
     */
    for (i = 0; i < 32; i += 4)
        SET_REG(dist_base + GIC_DIST_PRI + i * 4 / 4, 0xa0a0a0a0);

    SET_REG(base + GIC_CPU_PRIMASK, 0xff);
    SET_REG(base + GIC_CPU_CTRL, 0x17);
}

static inline unsigned int gic_irq(unsigned int irq)
{
    return irq - gic_data[0].irq_offset;
}

static void gic_mask_irq(unsigned int irq)
{
    unsigned int mask = 1 << (irq % 32);

    SET_REG(gic_data[0].dist_base + GIC_DIST_ENABLE_CLEAR + (gic_irq(irq) / 32) * 4, mask);
}

static void gic_unmask_irq(unsigned int irq)
{
    unsigned int mask = 1 << (irq % 32);

    SET_REG(gic_data[0].dist_base + GIC_DIST_ENABLE_SET + (gic_irq(irq) / 32) * 4, mask);
}

static void gic_eoi_irq(unsigned int irq)
{
    SET_REG(gic_data[0].cpu_base + GIC_CPU_EOI, gic_irq(irq));
}

static void gic_handle_irq(void)
{
    unsigned int  irqstat, irqnr;
    unsigned int cpu_base = GICC_BASE;
    do
    {
        irqnr = 0x3ff;
        int k = 0;
        while (irqnr == 0x3ff)
        {
            irqstat = GET_REG(cpu_base + GIC_CPU_INTACK);
            irqnr = irqstat & GICC_IAR_INT_ID_MASK;
            k++;
        }
        generic_handle_irq(irqnr);
        break;
    } while (1);

    gic_eoi_irq(irqnr);
}

void raise_soft_irq(int irq, int cpu_mask)
{
    SET_REG(GICD_BASE+GIC_DIST_SOFTINT, (cpu_mask << 16) | irq);
}

void rt_hw_ipi_send(int ipi_vector, unsigned int cpu_mask)
{
    raise_soft_irq(ipi_vector, cpu_mask);
}

struct irq_chip gic_chip = {
    .name       = "arm-gic",
    .irq_mask   = gic_mask_irq,
    .irq_unmask = gic_unmask_irq,
    .irq_eoi    = gic_eoi_irq,
};

#ifdef RT_USING_PM
extern int rt_hw_irq_suspend(void);
extern void rt_hw_irq_resume(void);
int interrupt_suspend(const struct rt_device *device, rt_uint8_t mode)
{
    rt_hw_irq_suspend();
    return RT_EOK;
}

void interrupt_resume(const struct rt_device *device, rt_uint8_t mode)
{
    rt_hw_irq_resume();
}

struct rt_device_pm_ops intc_pm_ops = {
    .suspend_noirq = interrupt_suspend,
    .resume_noirq = interrupt_resume
};
#endif

void gic_init(void)
{
    struct gic_chip_data *gic;
    int idx = 0;
    unsigned int dist_base = GICD_BASE;
    unsigned int cpu_base = GICC_BASE;
    unsigned int gic_nr = 0;
    unsigned int irq_start = 0;
    gic = &gic_data[0];
    gic->dist_base = dist_base;
    gic->cpu_base = cpu_base;
    gic->irq_offset = 0 ;

    if (gic_nr == 0)
        gic_cpu_base_addr = cpu_base;

    gic_dist_init(gic, irq_start);
    gic_cpu_init(gic);
    for (idx = 0; idx < NR_IRQS; idx++)
        irq_set_chip(idx, &gic_chip);

    set_handle_irq(gic_handle_irq);
}

#ifdef RT_USING_PM
static struct rt_device interrupt_device;
#endif
void intc_init(void)
{
    gic_init();

#ifdef RT_USING_PM
    rt_memcpy(interrupt_device.parent.name, INTC_DEV_NAME, rt_strlen(INTC_DEV_NAME));
    rt_pm_device_register(&interrupt_device, &intc_pm_ops);
#endif
}

void gic_init_smp(void)
{
    struct gic_chip_data *gic;

    gic = &gic_data[0];

    gic_cpu_init(gic);
}
