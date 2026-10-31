#include <rthw.h>
#include <rtthread.h>
#include <stdint.h>

#include "mmu.h"
#include "fh_chip.h"
#include "timer.h"
#include "arch-timer.h"
#include "fh_def.h"
#ifdef RT_USING_PM
#include <pm.h>
#endif

#define ARM_TIMER_SECURE_IRQ        (13+16)
#define ARM_TIMER_NONSECURE_IRQ     (14+16)
#define ARM_TIMER_VIRT_IRQ          (11+16)

#define ARM_TIMER_FREQ              24000000
#define ARM_TIMER_RELOAD_CNT        (ARM_TIMER_FREQ/RT_TICK_PER_SECOND - 1)


#define TIMER_DEV_NAME "timer_dev"

#ifndef dsb
#define dsb()  asm volatile ("dsb\n\t")
#endif
#ifndef isb
#define isb()  asm volatile ("isb\n\t")
#endif

static int g_arm_timer_access = ARCH_TIMER_PHYS_ACCESS;

static void arch_timer_reg_write_cp15(int access, enum arch_timer_reg reg, unsigned int val)
{
    if (access == ARCH_TIMER_PHYS_ACCESS)
    {
        switch (reg)
        {
        case ARCH_TIMER_REG_CTRL:
            asm volatile("mcr p15, 0, %0, c14, c2, 1" : : "r" (val));
            break;
        case ARCH_TIMER_REG_TVAL:
            asm volatile("mcr p15, 0, %0, c14, c2, 0" : : "r" (val));
            break;
        }
    }
    else if (access == ARCH_TIMER_VIRT_ACCESS)
    {
        switch (reg)
        {
        case ARCH_TIMER_REG_CTRL:
            asm volatile("mcr p15, 0, %0, c14, c3, 1" : : "r" (val));
            break;
        case ARCH_TIMER_REG_TVAL:
            asm volatile("mcr p15, 0, %0, c14, c3, 0" : : "r" (val));
            break;
        }
    }

    isb();
}

unsigned int arch_timer_reg_read_cp15(int access, enum arch_timer_reg reg)
{
    unsigned int val = 0;

    if (access == ARCH_TIMER_PHYS_ACCESS)
    {
        switch (reg)
        {
        case ARCH_TIMER_REG_CTRL:
            asm volatile("mrc p15, 0, %0, c14, c2, 1" : "=r" (val));
            break;
        case ARCH_TIMER_REG_TVAL:
            asm volatile("mrc p15, 0, %0, c14, c2, 0" : "=r" (val));
            break;
        }
    } else if (access == ARCH_TIMER_VIRT_ACCESS)
    {
        switch (reg)
        {
        case ARCH_TIMER_REG_CTRL:
            asm volatile("mrc p15, 0, %0, c14, c3, 1" : "=r" (val));
            break;
        case ARCH_TIMER_REG_TVAL:
            asm volatile("mrc p15, 0, %0, c14, c3, 0" : "=r" (val));
            break;
        }
    }

    return val;
}

static inline void arch_timer_set_cntfrq(unsigned int val)
{
    asm volatile("mcr p15, 0, %0, c14, c0, 0" : : "r" (val));
}

static inline unsigned int arch_timer_get_cntfrq(void)
{
    unsigned int val;
    asm volatile("mrc p15, 0, %0, c14, c0, 0" : "=r" (val));
    return val;
}

static inline unsigned long long arch_counter_get_cntpct(void)
{
    unsigned long long cval;

    isb();
    asm volatile("mrrc p15, 0, %Q0, %R0, c14" : "=r" (cval));
    return cval;
}

static inline unsigned long long arch_counter_get_cntvct(void)
{
    unsigned long long cval;

    isb();
    asm volatile("mrrc p15, 1, %Q0, %R0, c14" : "=r" (cval));
    return cval;
}

static inline unsigned int arch_timer_get_cntkctl(void)
{
    unsigned int cntkctl;
    asm volatile("mrc p15, 0, %0, c14, c1, 0" : "=r" (cntkctl));
    return cntkctl;
}

static inline void arch_timer_set_cntkctl(unsigned int cntkctl)
{
    asm volatile("mcr p15, 0, %0, c14, c1, 0" : : "r" (cntkctl));
}

static void arch_counter_set_user_access(void)
{
    unsigned int cntkctl = arch_timer_get_cntkctl();

    /* Disable user access to the timers and the physical counter */
    /* Also disable virtual event stream */
    cntkctl &= ~(ARCH_TIMER_USR_PT_ACCESS_EN
            | ARCH_TIMER_USR_VT_ACCESS_EN
            | ARCH_TIMER_VIRT_EVT_EN
            | ARCH_TIMER_EVT_TRIGGER_MASK
            | ARCH_TIMER_USR_PCT_ACCESS_EN);

    /* Enable user access to the virtual counter */
    cntkctl |= ARCH_TIMER_USR_VCT_ACCESS_EN;

    /* Set the divider and enable virtual event stream */
    cntkctl |= ((11) << ARCH_TIMER_EVT_TRIGGER_SHIFT)
            | ARCH_TIMER_VIRT_EVT_EN;

    arch_timer_set_cntkctl(cntkctl);
}

static void set_next_event(const int access, unsigned long evt)
{
    unsigned long ctrl;
    ctrl = arch_timer_reg_read_cp15(access, ARCH_TIMER_REG_CTRL);
    ctrl |= ARCH_TIMER_CTRL_ENABLE;
    ctrl &= ~ARCH_TIMER_CTRL_IT_MASK;
    arch_timer_reg_write_cp15(access, ARCH_TIMER_REG_TVAL, evt);
    arch_timer_reg_write_cp15(access, ARCH_TIMER_REG_CTRL, ctrl);
}

#ifdef ENABLE_SMP_DEBUG
static unsigned int g_tmr_cnter1 = 0;
static unsigned int g_tmr_cnter2 = 0;
#endif

static timer g_timer = {.rate = ARM_TIMER_FREQ, .reload_cnt = ARM_TIMER_RELOAD_CNT};

timer *get_timer(void)
{
    return &g_timer;
}
static void timerhandler(int irq, void *arg)
{
    int ctrl = arch_timer_reg_read_cp15(g_arm_timer_access, ARCH_TIMER_REG_CTRL);
    timer *tim = (timer *)arg;

    if (ctrl & ARCH_TIMER_CTRL_IT_STAT)
    {
        ctrl |= ARCH_TIMER_CTRL_IT_MASK;
        arch_timer_reg_write_cp15(g_arm_timer_access, ARCH_TIMER_REG_CTRL, ctrl);

        set_next_event(g_arm_timer_access, tim->reload_cnt);
        /* when timekeeping is enabled */
        if (tim->irq_handler)
            tim->irq_handler();
        else
            rt_tick_increase();

#ifdef ENABLE_SMP_DEBUG
        if ((rt_hw_cpu_id() & 3) == 0)
        {
            g_tmr_cnter1 += 1;
            if (g_tmr_cnter1 == 200)
            {
                rt_kprintf("ARCH TMR1 counts 200.\n");
                g_tmr_cnter1 = 0;
            }
        }
        if ((rt_hw_cpu_id() & 3) == 1)
        {
            g_tmr_cnter2 += 1;
            if (g_tmr_cnter2 == 300)
            {
                rt_kprintf("ARCH TMR2 counts 300.\n");
                g_tmr_cnter2 = 0;
            }
        }
#endif
        return;
    }
}

int timer_set_mode(timer *tim, enum timer_mode mode)
{
    return 0;
}


void timer_enable(timer *tim)
{
    unsigned long ctrl;

    ctrl = arch_timer_reg_read_cp15(g_arm_timer_access, ARCH_TIMER_REG_CTRL);
    ctrl |= ARCH_TIMER_CTRL_ENABLE;
    arch_timer_reg_write_cp15(g_arm_timer_access, ARCH_TIMER_REG_CTRL, ctrl);
}

void timer_disable(timer *tim)
{
    unsigned long ctrl;

    ctrl = arch_timer_reg_read_cp15(g_arm_timer_access, ARCH_TIMER_REG_CTRL);
    ctrl &= ~ARCH_TIMER_CTRL_ENABLE;
    arch_timer_reg_write_cp15(g_arm_timer_access, ARCH_TIMER_REG_CTRL, ctrl);
}

void timer_enable_irq(timer *tim)
{
}

void timer_disable_irq(timer *tim)
{
}

void timer_set_irq_handler(timer *tim, void (*handler)(void))
{
    tim->irq_handler = handler;
}

void timer_set_count(timer *tim, unsigned int count)
{
    tim->reload_cnt = count-1;
    set_next_event(g_arm_timer_access, count-1);
}

#ifdef RT_USING_PM
static int timer_ctrl;

static int timer_suspend(const struct rt_device *device, rt_uint8_t mode)
{
    timer_ctrl = arch_timer_reg_read_cp15(g_arm_timer_access, ARCH_TIMER_REG_CTRL);

    return RT_EOK;
}

static void timer_resume(const struct rt_device *device, rt_uint8_t mode)
{
    timer_disable(&g_timer);
    arch_timer_set_cntfrq(ARM_TIMER_FREQ);
    set_next_event(g_arm_timer_access, g_timer.reload_cnt);
    arch_timer_reg_write_cp15(g_arm_timer_access, ARCH_TIMER_REG_CTRL, timer_ctrl);
}

struct rt_device_pm_ops arch_timer_pm_ops = {
    .suspend_noirq = timer_suspend,
    .resume_noirq = timer_resume
};
#endif

static struct rt_device timer_device;
void rt_hw_timer_init(void)
{
    unsigned int cpuid;

    cpuid = rt_hw_cpu_id() & 3;
    arch_timer_set_cntfrq(ARM_TIMER_FREQ);
    arch_counter_set_user_access();

    if (cpuid == 0)
    {
        rt_hw_interrupt_install(ARM_TIMER_SECURE_IRQ, timerhandler, &g_timer, "tick_s");
        /* rt_hw_interrupt_install(ARM_TIMER_NONSECURE_IRQ, timerhandler, RT_NULL, "tick_ns");  */
    }
    /* enable the IRQ   */
    rt_hw_interrupt_umask(ARM_TIMER_SECURE_IRQ);
    /* rt_hw_interrupt_umask(ARM_TIMER_NONSECURE_IRQ);  */
    set_next_event(g_arm_timer_access, ARM_TIMER_RELOAD_CNT);

#ifdef RT_USING_PM
    rt_memcpy(timer_device.parent.name, TIMER_DEV_NAME, rt_strlen(TIMER_DEV_NAME));
    rt_pm_device_register(&timer_device, &arch_timer_pm_ops);
#endif
}
INIT_BOARD_EXPORT(rt_hw_timer_init);
