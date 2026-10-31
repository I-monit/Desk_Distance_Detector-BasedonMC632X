

#ifndef __DRV_PM_H__
#define __DRV_PM_H__

#include <rtdef.h>
#include <rthw.h>

struct pm_param
{
    unsigned int reg_coh_addr;
    unsigned int reg_cpu_addr;
    unsigned int arm_in_slp;
};

int fh_hw_pm_init(void *p);

/* void pm_sleep_register(void (*input)(int sleep_time_ms)); */

#endif
