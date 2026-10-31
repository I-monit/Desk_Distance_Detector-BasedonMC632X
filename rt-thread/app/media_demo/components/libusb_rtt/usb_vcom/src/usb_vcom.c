#include "libusb_rtt/usb_vcom/include/usb_vcom.h"

#ifdef FH_APP_CDC_COOLVIEW
static pthread_t g_thread_cdc_mode;
#endif
static pthread_t g_thread_cdc_update;

#ifdef FH_APP_CDC_COOLVIEW
void *cdc_mode_proc(void *arg)
{
    int flag = FH_CDC_VCOM_MODE;
    int old_flag = FH_CDC_COOLVIEW_MODE;

    prctl(PR_SET_NAME, "cdc_mode_proc");
    sleep(1);
    while (1)
    {
        flag = fh_cdc_mode_get();
        if (old_flag != flag)
        {
            old_flag = flag;
            if (flag == FH_CDC_COOLVIEW_MODE)
            {
                printf("sample_common_start_coolview!\n");
                sample_common_start_coolview();
            }
            else
            {
                printf("sample_common_stop_coolview!\n");
                sample_common_stop_coolview();
            }
        }
        else
            usleep(10000);
    }
}
#endif

void *cdc_update_proc(void *arg)
{
    int size = 64;
    int flag = 0;
    uint8_t buf[64] = {0x99, 0x00, 0x04, 0xe9, 0x88, 0x63, 0x6f, 0x6d,
                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

    prctl(PR_SET_NAME, "cdc_update_proc");
    while (1)
    {
        flag = fh_cdc_mode_get();
        if (flag != FH_CDC_UPDATE_MODE)
        {
            usleep(10 * 1000);
            continue;
        }
        usb_update_check(buf, size);
        usleep(100 * 1000);
    }
}

int cdc_init(void)
{
    pthread_attr_t attr;
    struct sched_param param;

    fh_cdc_init();

#ifdef FH_APP_CDC_COOLVIEW
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_attr_setstacksize(&attr, 4 * 1024);
    param.sched_priority = 30;
    pthread_attr_setschedparam(&attr, &param);

    if (pthread_create(&g_thread_cdc_mode, &attr, cdc_mode_proc, NULL) != 0)
    {
        printf("Error: Create cdc_mode_proc thread failed!\n");
        return -1;
    }
#endif

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_attr_setstacksize(&attr, 2 * 1024);
    param.sched_priority = 130;
    pthread_attr_setschedparam(&attr, &param);
    if (pthread_create(&g_thread_cdc_update, &attr, cdc_update_proc, NULL) != 0)
    {
        printf("Error: Create cdc_update_proc thread failed!\n");
        return -1;
    }
    return 0;
}
