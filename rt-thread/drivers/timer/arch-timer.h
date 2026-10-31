#ifndef __ARCH_TIMER__
#define __ARCH_TIMER__

enum arch_timer_reg
{
    ARCH_TIMER_REG_CTRL,
    ARCH_TIMER_REG_TVAL,
};

#define ARCH_TIMER_CTRL_ENABLE      (1 << 0)
#define ARCH_TIMER_CTRL_IT_MASK     (1 << 1)
#define ARCH_TIMER_CTRL_IT_STAT     (1 << 2)

#define ARCH_TIMER_PHYS_ACCESS      0
#define ARCH_TIMER_VIRT_ACCESS      1

#define ARCH_TIMER_USR_PCT_ACCESS_EN    (1 << 0) /* physical counter */
#define ARCH_TIMER_USR_VCT_ACCESS_EN    (1 << 1) /* virtual counter */
#define ARCH_TIMER_VIRT_EVT_EN      (1 << 2)
#define ARCH_TIMER_EVT_TRIGGER_SHIFT    (4)
#define ARCH_TIMER_EVT_TRIGGER_MASK (0xF << ARCH_TIMER_EVT_TRIGGER_SHIFT)
#define ARCH_TIMER_USR_VT_ACCESS_EN (1 << 8) /* virtual timer registers */
#define ARCH_TIMER_USR_PT_ACCESS_EN (1 << 9) /* physical timer registers */

void pts_init(void);
void arm_arch_timer_init(void);
unsigned long long get_ticks(void);
void udelay(unsigned int us);

#endif
