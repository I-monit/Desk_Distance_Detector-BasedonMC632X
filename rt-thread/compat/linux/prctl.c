#include <rtthread.h>
#include <sys/prctl.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>

#define PRCTL_SET_NAME_MAX 16
int prctl(int option, ...)
{
    va_list args;
    char *name;
    rt_thread_t thread;

    va_start(args, option);
    name = va_arg(args, char *);
    va_end(args);

    switch (option)
    {
    case PR_SET_NAME:
        thread = rt_thread_self();
        memset(thread->name, 0, RT_NAME_MAX);
        strncpy(thread->name, name, RT_NAME_MAX - 1);
        break;
    case PR_GET_NAME:
        thread = rt_thread_self();
        strcpy(name, thread->name);
        break;
    default:
        rt_kprintf("unsupported prctl option: %d\n", option);
        errno = EINVAL;
        return -1;
    }

    return 0;
}
