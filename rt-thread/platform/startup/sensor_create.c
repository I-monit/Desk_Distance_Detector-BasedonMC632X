#include <rtthread.h>
#include "isp/isp_api.h"

#if !defined CONFIG_ARCH_FH8626V100 && !defined CONFIG_ARCH_FH8862 && \
    !defined CONFIG_ARCH_FH885xV310 && !defined CONFIG_ARCH_FH885xV300 && \
    !defined CONFIG_ARCH_FH8636_FH8852V20X && !defined CONFIG_ARCH_FH8852V301_FH8662V100 && \
    !defined CONFIG_ARCH_MC632X && !defined CONFIG_ARCH_FH885xV500

extern int _sensor_drv_start;
extern int _sensor_drv_end;
struct isp_sensor_if *Sensor_Create(void)
{
    unsigned int _snstart = (unsigned int)&_sensor_drv_start;
    unsigned int _snsend = (unsigned int)&_sensor_drv_end;
    struct isp_sensor_if *psdrv = (struct isp_sensor_if *)_snstart;

    int i;
    int drv_cnt = (_snsend - _snstart) / sizeof(struct isp_sensor_if);

    /* rt_kprintf("sensor count: %d\n", drv_cnt);   */
    for (i = 0; i < drv_cnt; i++)
    {
        /* rt_kprintf("sensor name: %s\n", psdrv->name);    */
        if (psdrv->is_sensor_connect())
        {
            return psdrv;
        }
        else
        {
            /* rt_kprintf("do NOT match.\n");   */
        }

        psdrv++;
    }

    rt_kprintf("\nSensor Can't be recognized or supported!\n");

    return RT_NULL;
}
#endif
