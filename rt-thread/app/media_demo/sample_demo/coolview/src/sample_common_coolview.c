#include "coolview/include/sample_common_coolview.h"

static char *model_name = MODEL_TAG_COOLVIEW;

#ifdef FH_APP_CDC_COOLVIEW
static struct dbi_uart_config g_dbi_uart_conf;
#else
static struct dbi_tcp_config g_dbi_tcp_conf;
#endif
static FH_SINT32 g_coolview_is_running;
static FH_SINT32 g_stop_coolview;
static pthread_t g_thread;

FH_SINT32 sample_common_start_coolview(FH_VOID)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    pthread_attr_t attr;
    struct sched_param param;
    if (g_coolview_is_running)
    {
        SDK_PRT(model_name, "coolview is running!\n");
        return -1;
    }

    g_stop_coolview = 0;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
#ifdef __RTTHREAD_OS__
    param.sched_priority = 130;
    pthread_attr_setstacksize(&attr, 8 * 1024);
#endif
    pthread_attr_setschedparam(&attr, &param);
#ifdef FH_APP_CDC_COOLVIEW
#ifdef __RTTHREAD_OS__
    g_dbi_uart_conf.index = 4; /* vcom */
#else
    g_dbi_uart_conf.index = 0; /* ttyGS0 */
#endif
    g_dbi_uart_conf.cancel = &g_stop_coolview;
    g_dbi_uart_conf.is_running = &g_coolview_is_running;
    ret = pthread_create(&g_thread, &attr, (FH_VOID * (*)(FH_VOID *)) uart_dbi_thread, &g_dbi_uart_conf);
#else
    g_dbi_tcp_conf.port = 8888;
    g_dbi_tcp_conf.cancel = &g_stop_coolview;
    g_dbi_tcp_conf.is_running = &g_coolview_is_running;
    ret = pthread_create(&g_thread, &attr, (FH_VOID * (*)(FH_VOID *)) libtcp_dbi_thread, &g_dbi_tcp_conf);
#endif
    SDK_FUNC_ERROR(model_name, ret);

    return ret;
}

FH_SINT32 sample_common_stop_coolview(FH_VOID)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;

    if (g_coolview_is_running)
    {
        /*wait for coolview thread to exit*/
        g_stop_coolview = 1;

#ifdef __RTTHREAD_OS__
        ret = tcp_dbi_destroy();
        SDK_FUNC_ERROR(model_name, ret);
#endif

#ifdef FH_APP_CDC_COOLVIEW
        pthread_cancel(g_thread);
#else
        while (g_stop_coolview)
        {
            usleep(20 * 1000);
        }
#endif
        g_coolview_is_running = 0;
    }

    return ret;
}