
#ifndef __DELAY_H__
#define __DELAY_H__

extern void __bad_udelay(void);
extern void __delay(int loops);
extern void __udelay(int loops);
extern void __const_udelay(unsigned int loops);
void fh_udelay(int n);

#define MAX_UDELAY_MS 2

#define SYS_TICK_HZ     (100)

#define udelay(n)                                                          \
    (__builtin_constant_p(n)                                               \
         ? ((n) > (MAX_UDELAY_MS * 1000)                                   \
                ? __bad_udelay()                                           \
                : __const_udelay((n) *                                     \
                                 ((2199023U * SYS_TICK_HZ) >> 11)))        \
         : __udelay(n))

void rt_udelay(int n);

#endif /* _HEAD_DEALY_H_ */
