#include <reent.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <rtthread.h>

#ifdef RT_USING_DFS
#include <dfs_posix.h>
#endif

#ifdef RT_USING_PTHREADS
#include <pthread.h>
#endif

/* Reentrant versions of system calls.  */


extern char *_strerror_r(struct _reent *ptr, int err, int v, int *p);
void _perror_r(struct _reent *ptr, const char *s)
{
    char *error;
    int dummy;
    rt_err_t err = rt_get_errno();

    if (s && *s)
    {
        rt_kprintf(s);
        rt_kprintf(": ");
    }
    error = _strerror_r(ptr, (err < 0 ? -err : err), 1, &dummy);
    if (error && *error)
        rt_kprintf(error);
    else
        rt_kprintf("Unknown error %d", err);

    rt_kprintf("\n");
}

void perror(const char *s)
{
    _perror_r(RT_NULL, s);
}
