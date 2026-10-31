#ifndef _FH_OSAL_STRING_MEM_H
#define _FH_OSAL_STRING_MEM_H

#define FH_OSAL_KERN_SOH	"\001"		/* ASCII Start Of Header */
#define FH_OSAL_KERN_SOH_ASCII	'\001'

#define FH_OSAL_KERN_EMERG	FH_OSAL_KERN_SOH "0"	/* system is unusable */
#define FH_OSAL_KERN_ALERT	FH_OSAL_KERN_SOH "1"	/* action must be taken immediately */
#define FH_OSAL_KERN_CRIT	FH_OSAL_KERN_SOH "2"	/* critical conditions */
#define FH_OSAL_KERN_ERR	FH_OSAL_KERN_SOH "3"	/* error conditions */
#define FH_OSAL_KERN_WARNING	FH_OSAL_KERN_SOH "4"	/* warning conditions */
#define FH_OSAL_KERN_NOTICE	FH_OSAL_KERN_SOH "5"	/* normal but significant condition */
#define FH_OSAL_KERN_INFO	FH_OSAL_KERN_SOH "6"	/* informational */
#define FH_OSAL_KERN_DEBUG	FH_OSAL_KERN_SOH "7"	/* debug-level messages */

#define FH_OSAL_KERN_DEFAULT	FH_OSAL_KERN_SOH "d"	/* the default kernel loglevel */

/*
 * Annotation for a "continued" line of log printout (only done after a
 * line that had no enclosing \n). Only to be used by core/arch code
 * during early bootup (a continued line is not SMP-safe otherwise).
 */
#define FH_OSAL_KERN_CONT	FH_OSAL_KERN_SOH "c"

#define FH_OSAL_ADNBND (sizeof (int) - 1)
#define fh_osal_bnd(X, bnd) (((sizeof (X)) + (bnd)) & (~(bnd)))

#if defined(__linux__)
#define fh_osal_printk_once(x, ...)	({	\
	static bool __printk_once;	\
		\
	if (!__printk_once) { \
		__printk_once = true;	\
		printk(x, ##__VA_ARGS__);\
	}	\
})
#endif

#define FH_OSAL_ADNBND (sizeof (int) - 1)
#define fh_osal_bnd(X, bnd) (((sizeof (X)) + (bnd)) & (~(bnd)))

//string api
char * fh_osal_strcpy(char *s1,const char *s2);
char * fh_osal_strncpy(char *s1,const char *s2, int size);
int fh_osal_strlcpy(char *s1, const char *s2, int size);
char * fh_osal_strcat(char *s1, const char *s2);
char * fh_osal_strncat(char *s1, const char *s2, int size);
int fh_osal_strlcat(char *s1, const char *s2, int size);
int fh_osal_strcmp(const char *s1,const char *s2);
int fh_osal_strncmp(const char *s1,const char *s2,int size);
int fh_osal_strcasecmp(const char *s1, const char *s2);
int fh_osal_strncasecmp(const char *s1, const char *s2, int n);
char * fh_osal_strchr(const char *s,int n);
char * fh_osal_strnchr(const char *s , int count, int c);
char * fh_osal_strrchr(const char *s,int c);
char * fh_osal_skip_spaces(const char *s);
char *fh_osal_strim(char *s);
char *fh_osal_strstrip(char *str);
char * fh_osal_strstr(const char *s1, const char *s2);
char * fh_osal_strnstr(const char *s1, const char *s2, int n);
int fh_osal_strlen(const char *s);
int fh_osal_strnlen(const char *s,int size);
char * fh_osal_strpbrk(const char *s1,const char *s2);
char * fh_osal_strsep(char **s,const char *ct);
int fh_osal_strspn(const char *s,const char *accept);
int fh_osal_strcspn(const char *s,const char *reject);
void * fh_osal_memset(void *str,int c,int count);
void * fh_osal_memcpy(void *s1,const void *s2,int count);
void * fh_osal_memmove(void *s1,const void *s2,int count);
void * fh_osal_memscan(void *addr, int c, int size);
int fh_osal_memcmp(const void *cs,const void *ct,int count);
void * fh_osal_memchr(const void *s,int c,int n);
void * fh_osal_memchr_inv(const void *s, int c, int n);

char *fh_osal_itoa(int n, char * buf);
char *fh_osal_uitoa(unsigned int n, char * buf);
char *fh_osal_lluitoa(long long unsigned int n, char * buf);
char *fh_osal_llitoa(long long int n, char * buf);
char *fh_osal_uitoxa(unsigned int n, char * buf);

unsigned long long fh_osal_strtoull(const char *cp, char **endp, unsigned int base);
unsigned long fh_osal_strtoul(const char *cp, char **endp, unsigned int base);
long fh_osal_strtol(const char *cp, char **endp, unsigned int base);
long long fh_osal_strtoll(const char *cp, char **endp, unsigned int base);
unsigned long fh_osal_simple_strtoul(const char *cp, char **endp, unsigned int base);

#if defined(__linux__)
__printf(3, 4) int fh_osal_snprintf(char *buf, int size, const char *fmt, ...);
__printf(3, 4) int fh_osal_scnprintf(char *buf, int size, const char *fmt, ...);
__printf(2, 3) int fh_osal_sprintf(char *buf, const char *fmt, ...);
__printf(3, 0) int fh_osal_vsnprintf(char *str, int size, const char *fmt, va_list args);
__scanf(2, 3) int fh_osal_sscanf(const char *buf, const char *fmt, ...);

__printf(1, 2) int fh_osal_printk(const char *fmt, ...);
void fh_osal_panic(const char *fmt, const char * file, const char * fun, int line, const char *);
#else
int fh_osal_snprintf(char *buf, int size, const char *fmt, ...);
int fh_osal_scnprintf(char *buf, int size, const char *fmt, ...);
int fh_osal_sprintf(char *buf, const char *fmt, ...);
int fh_osal_vsnprintf(char *str, int size, const char *fmt, va_list args);
int fh_osal_sscanf(const char *buf, const char *fmt, ...);
int fh_osal_printk(const char *fmt, ...);
void fh_osal_panic(const char *fmt, const char * file, const char * fun, int line, const char *);
#endif

//addr translate
void *fh_osal_request_mem_region(unsigned int start, unsigned int n, const char *name);
void *fh_osal_ioremap(unsigned long phys_addr, int size);
void *fh_osal_ioremap_nocache(unsigned long phys_addr, int size);
void *fh_osal_ioremap_cached(unsigned long phys_addr, int size);
void fh_osal_iounmap(void *addr);
void *fh_osal_of_iomap(void* node, int index);
unsigned long long fh_osal_memparse(const char *ptr, char **retptr);
void *fh_osal_ioaddress(int addr);

unsigned int fh_osal_readl(volatile unsigned int *x);
void fh_osal_writel(unsigned int v, volatile unsigned int *x);
unsigned char fh_osal_readb(volatile unsigned char *x);
void fh_osal_writeb(unsigned char v, volatile unsigned char *x);


#define NULL_STRING "NULL"

unsigned long fh_osal_copy_from_user(void *to, const void *from, unsigned long n);
unsigned long fh_osal_copy_to_user(void *to, const void *from, unsigned long n);

int fh_osal_access_ok(int type, const void *addr, unsigned long size);

//barrier
void fh_osal_mb(void);
void fh_osal_rmb(void);
void fh_osal_wmb(void);
void fh_osal_smp_mb(void);
void fh_osal_smp_rmb(void);
void fh_osal_smp_wmb(void);
void fh_osal_isb(void);
void fh_osal_dsb(void);
void fh_osal_dmb(void);

#endif
