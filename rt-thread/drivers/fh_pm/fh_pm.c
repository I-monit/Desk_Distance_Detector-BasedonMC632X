
#include <rtdef.h>
#include <rthw.h>
#include <pm.h>
#include "fh_pm.h"
#include "fh_def.h"

#ifdef RT_USING_PM

static struct pm_param pm_param;
int fh_suspend_sleep(unsigned long arg)
{
    if (pm_param.reg_cpu_addr != 0)
    {
        SET_REG(pm_param.reg_cpu_addr, (unsigned int)cpu_resume);
    }
    SET_REG(pm_param.reg_coh_addr, pm_param.arm_in_slp);
#ifdef ARCH_ARM_CORTEX_A
    asm volatile("wfi");
#else
    asm volatile("mcr p15, 0, r1, c7, c0, 4");
#endif
    return 0;
}

static void sleep(struct rt_pm *pm, uint8_t mode)
{
    switch (mode)
    {
    case PM_SLEEP_MODE_NONE:
        break;

    case PM_SLEEP_MODE_IDLE:
        break;

    case PM_SLEEP_MODE_LIGHT:
        break;

    case PM_SLEEP_MODE_DEEP:
        cpu_suspend(0, fh_suspend_sleep);
        break;

    case PM_SLEEP_MODE_STANDBY:
        break;

    case PM_SLEEP_MODE_SHUTDOWN:
        break;

    default:
        RT_ASSERT(0);
        break;
    }

}

static void run(struct rt_pm *pm, uint8_t mode)
{
    switch (mode)
    {
    case PM_RUN_MODE_HIGH_SPEED:
    case PM_RUN_MODE_NORMAL_SPEED:
        break;
    case PM_RUN_MODE_MEDIUM_SPEED:
        break;
    case PM_RUN_MODE_LOW_SPEED:
        break;
    default:
        break;

    }
}

static void pm_timer_start(struct rt_pm *pm, rt_uint32_t timeout)
{
    RT_ASSERT(pm != RT_NULL);
}

/**
 * This function stop the timer of pm
 *
 * @param pm Pointer to power manage structure
 */
static void pm_timer_stop(struct rt_pm *pm)
{
    RT_ASSERT(pm != RT_NULL);
}

/**
 * This function calculate how many OS Ticks that MCU have suspended
 *
 * @param pm Pointer to power manage structure
 *
 * @return OS Ticks
 */
static rt_tick_t pm_timer_get_tick(struct rt_pm *pm)
{
    rt_uint32_t timer_tick = 0;

    /* timer_tick = rt_tick_from_millisecond(rt_syst_diff); */

    return timer_tick;
}

/**
 * This function initialize the power manager
 */
int fh_hw_pm_init(void *p)
{
    static const struct rt_pm_ops _ops = {
        sleep,
        run,
        pm_timer_start,
        pm_timer_stop,
        pm_timer_get_tick
    };

    rt_uint8_t timer_mask = 0;

    /* initialize timer mask */
    timer_mask = 1UL << PM_SLEEP_MODE_DEEP;

    /* initialize system pm module */
    rt_system_pm_init(&_ops, timer_mask, RT_NULL);
    rt_memcpy(&pm_param, p, sizeof(pm_param));
    rt_pm_release_all(PM_SLEEP_MODE_NONE);

    return 0;
}

extern void dfs_suspend(void);
extern void dfs_resume(void);
void fh_aov_arm_sleep(void)
{
    register rt_base_t temp;

    /*must call before dis int*/

    rt_kprintf("[%s-%d]: ARM will slp...\n", __func__, __LINE__);
    dfs_suspend();

    /* disable interrupt */
    temp = rt_hw_interrupt_disable();
    rt_enter_critical();
    rt_pm_request(PM_SLEEP_MODE_DEEP);
    rt_pm_release(PM_SLEEP_MODE_NONE);

extern void rt_system_power_manager(void);
    rt_system_power_manager();

    rt_kprintf("[%s-%d]: ARM wake up...\n", __func__, __LINE__);
    rt_pm_request(PM_SLEEP_MODE_NONE);
    rt_pm_release(PM_SLEEP_MODE_DEEP);

    /*must call before en int*/
    dfs_resume();

    rt_exit_critical();
    /* enable interrupt */
    rt_hw_interrupt_enable(temp);
}

#endif
