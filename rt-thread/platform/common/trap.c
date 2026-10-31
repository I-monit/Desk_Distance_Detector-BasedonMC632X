/**
 * Copyright (c) 2015-2019 Shanghai Fullhan Microelectronics Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 *
 * Change Logs:
 * Date           Author       Notes
 * 2019-04-12                  add license Apache-2.0
 */


#include <string.h>
#include <rthw.h>
#ifdef ARCH_ARM_CORTEX_A7
#include <armv7.h>
#else
#include <armv6.h>
#endif

#include "fh_pmu.h"
#include "fh_def.h"
#include "fh_chip.h"
#include "fh_arch.h"
#include "irqdesc.h"
#include "fh_ictl.h"
#include "platform_def.h"
/**
 * @addtogroup FH81
 */
/*@{*/

extern int __rt_ffs(int value);
/* extern struct rt_thread *rt_current_thread;  */
#ifdef RT_USING_FINSH
extern long list_thread(void);
extern void list_irq(void);
#endif
#define IRQ_STATUS 0x2

extern void (*handle_arch_irq)(void);
extern long disp_memory(unsigned int addr, unsigned int length);
extern long set_memory(unsigned int addr, unsigned int val);

/* RTM_EXPORT(rt_sem_init); */
#ifdef USING_ISR_HOOK

void *g_isr_in_para = 0;
void *g_isr_out_para = 0;
void (*g_isr_in_hook)(void *para, int irq)  = 0;
void (*g_isr_out_hook)(void *para, int irq) = 0;

void isr_hook_in_set(void (*isr_in_hook)(void *para, int irq), void *para)
{
    g_isr_in_hook = isr_in_hook;
    g_isr_in_para = para;
}
RTM_EXPORT(isr_hook_in_set);

void isr_hook_out_set(void (*isr_out_hook)(void *para, int irq), void *para)
{
    g_isr_out_hook = isr_out_hook;
    g_isr_out_para = para;
}
RTM_EXPORT(isr_hook_out_set);

#endif

void rt_stack_backtrace(rt_uint32_t thread_stack_addr, rt_uint32_t thread_stack_size, rt_uint32_t thread_sp)
{
    rt_uint32_t sp;
    rt_uint32_t column = 0;

    sp = thread_stack_addr+thread_stack_size;
    rt_kprintf("Stacktrace:(0x%08x to 0x%08x)\n", thread_sp, sp);
    rt_kprintf("[<0x%08x>] ", sp);
    for (; sp >= thread_sp; sp = sp-4)
    {
        if (column == 8)
        {
            rt_kprintf("\n[<0x%08x>] ", sp);
            column = 0;
        }
        rt_kprintf("%08x ", *(rt_uint32_t *)sp);
        column++;
    }
    rt_kprintf("\n");
}

#ifdef RT_USING_VFP
static inline void rt_excpt_vfpprint(void)
{
    /* check fpexc state*/
    unsigned int value;

    asm volatile("fmrx %0,fpexc":"=r" (value));
    rt_kprintf("fpexc is %x\r\n",value);

    if ((value&0x80000000) != 0)
    {
        rt_kprintf("VFP exception found \r\n");
        rt_kprintf("fpexc is %x\r\n",value);

        if ((value& (1<<3)) != 0)
            rt_kprintf("VFP underflow \r\n");

        if ((value& (1<<2)) != 0)
            rt_kprintf("VFP overflow \r\n");

        if ((value& (1<<7)) != 0)
            rt_kprintf("VFP input exception \r\n");

        if ((value& (1<<0)) != 0)
            rt_kprintf("VFP invaild operation \r\n");

    }
}
#endif


static inline void rt_excpt_cp15print(void)
{
    rt_uint32_t SR;
    rt_uint32_t AR;

    asm ("mrc p15, 0, %0, c5, c0, 0":"=r"(SR));
    asm ("mrc p15, 0, %0, c6, c0, 0":"=r"(AR));
    rt_kprintf("DFSR 0x%x DFAR 0x%x\n",SR,AR);
    asm ("mrc p15, 0, %0, c5, c0, 1":"=r"(SR));
    asm ("mrc p15, 0, %0, c6, c0, 2":"=r"(AR));
    rt_kprintf("IFSR 0x%x IFAR 0x%x\n",SR,AR);

}

static inline void rt_excpt_handler(struct rt_hw_register *regs)
{

    if (((regs->cpsr) & 0xF) == IRQ_STATUS)
    {
        rt_kprintf("crashed in ISR\n");
#ifdef RT_USING_FINSH
        list_irq();
#endif
        rt_kprintf("Interrupt Controller Number:%d\n", irq_in_process());
    }
    else
    {
        rt_kprintf("thread - %s stack:\n", rt_thread_self()->name);
        rt_stack_backtrace((rt_uint32_t)rt_thread_self()->stack_addr, (rt_uint32_t)rt_thread_self()->stack_size, (rt_uint32_t)rt_thread_self()->sp);
    }

#ifdef RT_ENABLE_RUNNING_LOG
    extern void runlog_export(void);
    runlog_export();
#endif

#ifdef RT_USING_BACKTRACE
    rt_hw_backtrace((rt_uint32_t *)regs->fp, (rt_uint32_t)rt_thread_self()->entry);
#endif

#ifdef RT_USING_FINSH
    list_thread();      /* for FAST boot, list_thread may fail, move to last */
#endif
}



/**
 * this function will show registers of CPU
 *
 * @param regs the registers point
 */

void rt_hw_show_register(struct rt_hw_register *regs)
{
    rt_kprintf("Execption:\n");
    rt_kprintf("r00:0x%08x r01:0x%08x r02:0x%08x r03:0x%08x\n", regs->r0,
               regs->r1, regs->r2, regs->r3);
    rt_kprintf("r04:0x%08x r05:0x%08x r06:0x%08x r07:0x%08x\n", regs->r4,
               regs->r5, regs->r6, regs->r7);
    rt_kprintf("r08:0x%08x r09:0x%08x r10:0x%08x\n", regs->r8, regs->r9,
               regs->r10);
    rt_kprintf("fp :0x%08x ip :0x%08x\n", regs->fp, regs->ip);
    rt_kprintf("sp :0x%08x lr :0x%08x pc :0x%08x\n", regs->sp, regs->lr,
               regs->pc);
#ifdef RT_USING_SMP
    rt_kprintf("cpsr:0x%08x, cpu: %x\n", regs->cpsr, rt_hw_cpu_id());
#else
    rt_kprintf("cpsr:0x%08x\n", regs->cpsr);
#endif

    if( regs->pc >= FH_DDR_START+0x20 && regs->pc < FH_DDR_END )
    {
        rt_kprintf("PC Context contents:[-32, 0]\n");
        rt_kprintf("0x00: %08x %08x %08x %08x\n", read_reg(regs->pc - 0x20),
                read_reg(regs->pc - 0x1c),
                read_reg(regs->pc - 0x18),
                read_reg(regs->pc - 0x14));
        rt_kprintf("0x10: %08x %08x %08x %08x\n", read_reg(regs->pc - 0x10),
                read_reg(regs->pc - 0x0c),
                read_reg(regs->pc - 0x08),
                read_reg(regs->pc - 0x04));
        rt_kprintf(" *************** End of PC Context ***************************\n");
    }
}

/**
 * When ARM7TDMI comes across an instruction which it cannot handle,
 * it takes the undefined instruction trap.
 *
 * @param regs system registers
 *
 * @note never invoke this function in application
 */
#ifdef ARCH_ARM_CORTEX_A7
void rt_hw_trap_undef(struct rt_hw_register *regs)
#else
void rt_hw_trap_udef(struct rt_hw_register *regs)
#endif
{

    rt_hw_show_register(regs);
    rt_kprintf("undefined instruction \n");
#ifdef RT_USING_VFP
    rt_excpt_vfpprint();
#endif
    rt_excpt_cp15print();
    rt_excpt_handler(regs);

    rt_hw_cpu_shutdown();
}

/**
 * The software interrupt instruction (SWI) is used for entering
 * Supervisor mode, usually to request a particular supervisor
 * function.
 *
 * @param regs system registers
 *
 * @note never invoke this function in application
 */
void rt_hw_trap_swi(struct rt_hw_register *regs)
{
    rt_hw_show_register(regs);

    rt_kprintf("software interrupt\n");
    rt_hw_cpu_shutdown();
}

/**
 * An abort indicates that the current memory access cannot be completed,
 * which occurs during an instruction prefetch.
 *
 * @param regs system registers
 *
 * @note never invoke this function in application
 */
void rt_hw_trap_pabt(struct rt_hw_register *regs)
{

    rt_hw_show_register(regs);
    rt_kprintf("prefetch abort\n");

    rt_excpt_cp15print();
    rt_excpt_handler(regs);

    rt_hw_cpu_shutdown();
}

/**
 * An abort indicates that the current memory access cannot be completed,
 * which occurs during a data access.
 *
 * @param regs system registers
 *
 * @note never invoke this function in application
 */
extern void rt_mm_export(void);
void rt_hw_trap_dabt(struct rt_hw_register *regs)
{

    rt_hw_show_register(regs);

    rt_kprintf("data abort\n");

    rt_excpt_cp15print();
    rt_excpt_handler(regs);

    rt_mm_export();

    rt_mm_export();

    rt_hw_cpu_shutdown();
}

/**
 * Normally, system will never reach here
 *
 * @param regs system registers
 *
 * @note never invoke this function in application
 */
void rt_hw_trap_resv(struct rt_hw_register *regs)
{
    rt_kprintf("not used\n");
    rt_hw_show_register(regs);
    rt_hw_cpu_shutdown();
}

void rt_hw_trap_irq(void)
{
    handle_arch_irq();
}

void rt_hw_trap_fiq(void) { rt_kprintf("fast interrupt request\n"); }

#ifdef RT_USING_BACKTRACE
void show_func_callchain(void)
{
    unsigned int cpsr;
    unsigned int fp, pc, i;
    rt_uint32_t irqstat_l, irqstat_h, irq = 0;

    asm("mrs %0, cpsr":"=r"(cpsr));
    asm("mov %0, fp":"=r"(fp));

    if ((cpsr & 0xf) == IRQ_STATUS)
    {
#ifdef ARCH_ARM_CORTEX_A7
        rt_kprintf("ASSERT in irq!!!\n");
#else
        fh_intc *p = (fh_intc *)INTC_REG_BASE;

        irqstat_l = p->IRQ_FINALSTATUS_L;
        irqstat_h = p->IRQ_FINALSTATUS_H;
        if (irqstat_l)
        {
            irq = __rt_ffs(irqstat_l) - 1;
        }
        else if (irqstat_h)
        {
            irq = __rt_ffs(irqstat_h) - 1 + 32;
        }
        rt_kprintf("ASSERT in irq : %d\n", irq);
#endif
        rt_kprintf("FUNCTION: [<0x%x>]\n", *(unsigned int *)fp - 0xC);
        for (i = 2; i > 0; i--)
        {
            fp = *(unsigned int *)(fp - 3 * 4);
            pc = *(unsigned int *)fp;
            rt_kprintf("[<0x%x>]\n", pc - 0xC);
        }
    }
    else
        rt_hw_backtrace((rt_uint32_t *)fp, (rt_uint32_t)rt_thread_self()->entry);
}
#else
void show_func_callchain(void)
{
    return;
}
#endif

void machine_reset(void) { fh_pmu_restart(); }

unsigned char uart0_read(void)
{
    unsigned char data;

    if ((GET_REG(UART0_REG_BASE+0x14) &0x01) == 0x01)
    {
        data = GET_REG(UART0_REG_BASE+0x00) & 0xff;
        return data;
    }
    return 0;
}
unsigned char uart1_read(void)
{
    unsigned char data;

    if ((GET_REG(UART1_REG_BASE+0x14) &0x01) == 0x01)
    {
        data = GET_REG(UART1_REG_BASE+0x00) & 0xff;
        return data;
    }
    return 0;
}

int rt_htoi(char *buff)
{
    int begin;
    int value = 0;

    if (buff[0] == '0' && buff[1] == 'x')
    {
        begin = 2;
    }
    else
        begin = 0;

    for (; (buff[begin] >= '0' && buff[begin] <= '9') || (buff[begin] >= 'a' && buff[begin] <= 'z'); ++begin)
    {
        if (buff[begin] > '9')
            value = 16 * value + (10 + buff[begin] - 'a');
        else
            value = 16 * value + (buff[begin] - '0');

    }

    return value;
}


unsigned int fh_parse_value(char *buff)
{

    unsigned int data;

    data = rt_htoi(buff);
    return data;
}

int fh_get_debug_mode(void)
{
    unsigned int uart_base;
    unsigned int data;

    if (strchr((const char *)RT_CONSOLE_DEVICE_NAME, '1') != NULL)
    {
        uart_base = UART1_REG_BASE;
    }
    else
    {
        uart_base = UART0_REG_BASE;
    }

    while (1)
    {
        while ((GET_REG(uart_base + 0x14) & 0x01) == 0x01)
        {
            data = GET_REG(uart_base + 0x00) & 0xff;
            if (data == 'w' || data == 'W')
                return 0;
            else if (data == 'r' || data == 'R')
                return 1;
            else
                return -RT_ERROR;
        }
    }

}


unsigned int fh_get_chr(void)
{
    unsigned char internal_buffer[11];
    unsigned char loop = 0;
    unsigned char data;
    unsigned int uart_base;
    unsigned int value;

    if (strchr((const char *)RT_CONSOLE_DEVICE_NAME, '1') != NULL)
    {
        uart_base = UART1_REG_BASE;
    }
    else
    {
        uart_base = UART0_REG_BASE;
    }

    while (1)
    {
        while ((GET_REG(uart_base + 0x14) & 0x01) == 0x01)
        {
            data = GET_REG(uart_base + 0x00) & 0xff;
            /*Get Next Param*/
            if (data == ' ' || data == '\r' || data == '\n')
            {
                internal_buffer[loop] = '\0';
                value = fh_parse_value((char *)internal_buffer);
                return value;
            }
            else
            {
                internal_buffer[loop] = data;
                loop++;
                if (loop >= sizeof(internal_buffer))
                {
                    rt_kprintf("Address or Length Error!\n");
                    return 0;
                }
            }
        }
    }

}

void machine_shutdown(void)
{
#ifdef RT_DEBUG
    char chr;
    unsigned int base, length;
    unsigned char (*uartread)(void);

    if (strchr((const char *)RT_CONSOLE_DEVICE_NAME, '1') != NULL)
    {
        uartread = uart1_read;
    }
    else
    {
        uartread = uart0_read;
    }

    while (1)
    {
        chr = uartread();

        switch (chr)
        {
        /* Ctrl + c to reset */
        case 0x03:
            machine_reset();
        /* Ctrl + d to Debug Mode */
        case 0x04:
            rt_kprintf("Debug Mode\n");
            if (fh_get_debug_mode() == 1)
            {
                rt_kprintf("Read Mode\n");
                base = fh_get_chr();
                length = fh_get_chr();
                disp_memory(base, length);
            }
            else if (fh_get_debug_mode() == 0)
            {
                rt_kprintf("Write Mode\n");
                base = fh_get_chr();
                length = fh_get_chr();
                rt_kprintf("Set Address : 0x%x at 0x%x\n", base, length);
                set_memory(base, length);
            }
        }
    }
#else
    while (1)
        ;
#endif
}
/*@}*/
