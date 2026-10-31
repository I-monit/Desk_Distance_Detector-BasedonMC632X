#include <rtthread.h>
#include <rtservice.h>
#include <rtdevice.h>
#include <string.h>
#include "cdc_vcom.h"
#if defined(RT_USING_POSIX)
#include <dfs_posix.h>
#include <dfs_poll.h>
#include <libc.h>
static int dev_old_flag;
#endif

static rt_thread_t thread_cdc_keyboard;

struct rt_messagequeue cdc_check_mq;
static rt_uint8_t cdc_checkmq_pool[(sizeof(int) + sizeof(void *))*8];

static int cdc_mode_flag = 0;
static int cdc_mode_init = 0;

void fh_cdc_mode_set(int flag)
{
    cdc_mode_flag = flag;
}

int fh_cdc_mode_get(void)
{
    return cdc_mode_flag;
}


/* client close */
static void disable_cdc_cmd(void)
{
    if (fh_cdc_mode_get() == FH_CDC_CMD_MODE)
    {
        /* set console */
        rt_console_set_device(RT_CONSOLE_DEVICE_NAME);
        /* set finsh device */
#if defined(RT_USING_POSIX)
        fcntl(libc_stdio_get_console(), F_SETFL, (void *) (dev_old_flag | O_NONBLOCK));
        libc_stdio_set_console(RT_CONSOLE_DEVICE_NAME, O_RDWR);
#else
        finsh_set_device(RT_CONSOLE_DEVICE_NAME);
#endif /* RT_USING_POSIX */
        rt_thread_t tid = rt_thread_find(FINSH_THREAD_NAME);

        if (tid)
        {
            rt_thread_resume(tid);
            rt_schedule();
        }
        fh_cdc_mode_set(FH_CDC_VCOM_MODE);
    }
}

static void enable_cdc_cmd(void)
{
    /* process the new connection */
    /* set console */
    rt_console_set_device("vcom");
    /* set finsh device */
#if defined(RT_USING_POSIX)
    /* backup flag */
    dev_old_flag = fcntl(libc_stdio_get_console(), F_GETFL, (void *) RT_NULL);
    /* add non-block flag */
    fcntl(libc_stdio_get_console(), F_SETFL, (void *) (dev_old_flag | O_NONBLOCK));
    libc_stdio_set_console("vcom", O_RDWR);
    rt_thread_t tid = rt_thread_find(FINSH_THREAD_NAME);

    if (tid)
    {
        rt_thread_resume(tid);
        rt_schedule();
    }
#else
    /* set finsh device */
    finsh_set_device("vcom");
#endif /* RT_USING_POSIX */
}

int coolview_mode = 0;
void cdc_keyboard_proc(void *arg)
{
    int mode;
    int ret;
    rt_device_t dev;

    dev = (rt_device_t)rt_device_find("vcom");
    if (!dev)
    {
        rt_kprintf("rt_device_find(%s) fail\n", "vcom");
        return;
    }
    ret = rt_device_open(dev, RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_INT_RX | RT_DEVICE_FLAG_INT_TX);
    if (ret != 0)
    {
        rt_kprintf("rt_device->open(%s) fail with(%d)\n", "vcom", ret);
        return;
    }
    fh_cdc_mode_set(FH_CDC_VCOM_MODE);
    rt_kprintf("[Vcom] Switch to vcom mode\n");
    while (1)
    {
        if (rt_mq_recv(&cdc_check_mq, &mode, sizeof(int), RT_WAITING_FOREVER) == RT_EOK)
        {
            rt_kprintf("[Vcom] fh_cdc_mode_get %d\n", fh_cdc_mode_get());
            coolview_mode = 0;
            if (fh_cdc_mode_get() == FH_CDC_VCOM_MODE)
            {
                ret = rt_device_close(dev);
                if (ret != 0)
                {
                    rt_kprintf("rt_device_close(%s) fail with(%d)\n", "vcom", ret);
                    return;
                }
            }
            if (mode == FH_CDC_CMD_MODE)
            {
                rt_kprintf("[Vcom] Switch to console mode\n");
                fh_cdc_mode_set(FH_CDC_CMD_MODE);
                enable_cdc_cmd();
            }
            else if (mode == FH_CDC_COOLVIEW_MODE)
            {
                disable_cdc_cmd();
                rt_kprintf("[Vcom] Switch to coolview mode\n");
                fh_cdc_mode_set(FH_CDC_COOLVIEW_MODE);
                coolview_mode = 2;
            }
            else if (mode == FH_CDC_UPDATE_MODE)
            {
                rt_kprintf("[Vcom] Switch to update mode\n");
                fh_cdc_mode_set(FH_CDC_UPDATE_MODE);
            }
            else if (mode == FH_CDC_VCOM_MODE)
            {
                disable_cdc_cmd();
                rt_kprintf("[Vcom] Switch to vcom mode\n");
                fh_cdc_mode_set(FH_CDC_VCOM_MODE);
                ret = rt_device_open(dev, RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_INT_RX | RT_DEVICE_FLAG_INT_TX);
                if (ret != 0)
                {
                    rt_kprintf("rt_device->open(%s) fail with(%d)\n", "vcom", ret);
                    return;
                }
            }
        } else
            rt_thread_mdelay(1);
    }
}

void cdc_keyboard_change(int mode)
{
    int ret;

    if (!cdc_mode_init)
        return;

    ret = rt_mq_send(&cdc_check_mq, (void *)&mode, sizeof(int));
    if (ret == -RT_EFULL)
    {
        rt_kprintf("cdc keyboard check message send failed\n");
    }
}

void cdc_shell_check_init(void)
{
    rt_mq_init(&cdc_check_mq, "cdc_check_mq", cdc_checkmq_pool, sizeof(int),
                                    sizeof(cdc_checkmq_pool), RT_IPC_FLAG_FIFO);
    thread_cdc_keyboard = rt_thread_create("cdc_keyboard_check", cdc_keyboard_proc,
                   NULL, 4 * 1024, 30, 1);
    rt_thread_startup(thread_cdc_keyboard);

    cdc_mode_init = 1;
}

#ifdef FINSH_USING_MSH
static int switch_cdc(int argc, char *argv[])
{
    if (argc != 2) {
        goto error_pr;
    }


    int mode = atoi(argv[1]);
    if (mode > 3)
        goto error_pr;

    cdc_keyboard_change(mode);
    return 0;
error_pr:
    printf("Usage: %s <mode_number>\n", argv[0]);
    printf("Mode numbers:\n");
    printf("  0 - COOLVIEW mode\n");
    printf("  1 - CMD mode\n");
    printf("  2 - UPDATE mode\n");
    printf("  3 - VCOM mode\n");
    return 1;
}
MSH_CMD_EXPORT(switch_cdc, change cdc mode for coolview-0 console-1 update-2 vocm-3);
#else
#include <finsh.h>
static int switch_cdc(int mode)
{
    if (mode > 3)
        goto error_pr;

    cdc_keyboard_change(mode);
    return 0;
error_pr:
    printf("Usage: switch_cdc <mode_number>\n");
    printf("Mode numbers:\n");
    printf("  0 - COOLVIEW mode\n");
    printf("  1 - CMD mode\n");
    printf("  2 - UPDATE mode\n");
    printf("  3 - VCOM mode\n");
    return 1;
}
FINSH_FUNCTION_EXPORT(switch_cdc, change cdc mode for coolview-0 console-1 update-2 vocm-3);
#endif

