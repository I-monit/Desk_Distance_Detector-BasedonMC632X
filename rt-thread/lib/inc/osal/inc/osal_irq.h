#ifndef _FH_OSAL_IRQ_H
#define _FH_OSAL_IRQ_H

#define FH_OSAL_IRQF_THREADED		0x00080000
#define FH_OSAL_IRQF_SHARED		0x00000080
#define FH_OSAL_IRQF_PROBE_SHARED	0x00000100
#define __FH_OSAL_IRQF_TIMER		0x00000200
#define FH_OSAL_IRQF_PERCPU		0x00000400
#define FH_OSAL_IRQF_NOBALANCING	0x00000800
#define FH_OSAL_IRQF_IRQPOLL		0x00001000
#define FH_OSAL_IRQF_ONESHOT		0x00002000
#define FH_OSAL_IRQF_NO_SUSPEND	0x00004000
#define FH_OSAL_IRQF_FORCE_RESUME	0x00008000
#define FH_OSAL_IRQF_NO_THREAD		0x00010000
#define FH_OSAL_IRQF_EARLY_RESUME	0x00020000
#define FH_OSAL_IRQF_COND_SUSPEND	0x00040000

#define FH_OSAL_IRQF_TIMER		(__FH_OSAL_IRQF_TIMER | \
						FH_OSAL_IRQF_NO_SUSPEND | FH_OSAL_IRQF_NO_THREAD)

enum fh_osal_irqreturn {
	FH_OSAL_IRQ_NONE = (0 << 0),
	FH_OSAL_IRQ_HANDLED = (1 << 0),
	FH_OSAL_IRQ_WAKE_THREAD = (1 << 1),
};

typedef int (*fh_osal_irq_handler_t)(int, void *);

int fh_osal_request_irq(unsigned int irq, fh_osal_irq_handler_t handler, 
		fh_osal_irq_handler_t thread_fn, unsigned long flags, 
		const char *name, void *dev);
int fh_osal_irq_create_mapping(void *host, unsigned long hwirq);
void fh_osal_irq_dispose_mapping(unsigned int virq);
unsigned int fh_osal_irq_of_parse_and_map(void *dev, int index);
void fh_osal_free_irq(unsigned int irq, void *dev);
int fh_osal_in_interrupt(void);
int fh_osal_enable_irq_wake(unsigned int irq);
int fh_osal_disable_irq_wake(unsigned int irq);

#endif
