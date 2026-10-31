#include <rtthread.h>
#include <api_wifi/generalWifi.h>
#include <gpio.h>
#include <platform_def.h>

#ifdef WIFI_USING_CYPRESS
int fh_wifi_power_set(FH_Wifi_Power_Mode_e mode)
{
    switch (mode)
    {
    case FHEN_WIFI_POWER_MODE_CLOSE:

    rt_kprintf("[WIFI]%s-%d\n", __func__, __LINE__);
    gpio_request(WIFI_ENABLE_GPIO);
    gpio_direction_output(WIFI_ENABLE_GPIO, 0);
    gpio_release(WIFI_ENABLE_GPIO);
    rt_thread_delay(15);

        break;
    default:
        break;
    }

    return 0;
}

void fh_wifi_sdio_reset(void)
{
#ifdef HW_WIFI_POWER_GPIO
    gpio_request(HW_WIFI_POWER_GPIO);
    gpio_direction_output(HW_WIFI_POWER_GPIO, 1);
    rt_thread_delay(15);
    gpio_direction_output(HW_WIFI_POWER_GPIO, 0);
    gpio_release(HW_WIFI_POWER_GPIO);
#endif
    gpio_request(WIFI_ENABLE_GPIO);
    gpio_direction_output(WIFI_ENABLE_GPIO, 0);
    rt_thread_delay(15);
    gpio_direction_output(WIFI_ENABLE_GPIO, 1);
    gpio_release(WIFI_ENABLE_GPIO);
}
#endif /* WIFI_USING_CYPRESS */
