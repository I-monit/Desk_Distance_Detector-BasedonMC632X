#include <stdlib.h>
#include <rtconfig.h>
#include <rtthread.h>
#include <rthw.h>

extern char *__real__dtoa_r(struct _reent *ptr, double _d,
    int mode, int ndigits, int *decpt, int *sign, char **rve);

char *__wrap__dtoa_r(struct _reent *ptr,
    double _d,
    int mode,
    int ndigits,
    int *decpt,
    int *sign,
    char **rve)
{
    unsigned long lvl;
    char *ret = NULL;

    lvl = rt_hw_interrupt_disable();
    ret = __real__dtoa_r(ptr, _d, mode, ndigits, decpt, sign, rve);
    rt_hw_interrupt_enable(lvl);

    return ret;
}
