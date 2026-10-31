#include "media_proc.h"

#ifdef __LINUX_OS__
static int write_proc(char *path, char *s)
{
    int fd;

    fd = open(path, O_RDWR);
    if (fd < 0)
    {
        printf("Error: open %s!\n", path);
        return -1;
    }

    write(fd, s, strlen(s));
    close(fd);
    return 0;
}

static int vpu_write_proc(char *s)
{
    return write_proc("/proc/driver/vpu", s);
}

#if !defined CONFIG_ARCH_MC632X
static int enc_write_proc(char *s)
{
    return write_proc("/proc/driver/enc", s);
}

static int jpeg_write_proc(char *s)
{
    return write_proc("/proc/driver/jpeg", s);
}
#endif

#ifdef BGM_SUPPORT
static int bgm_write_proc(char *s)
{
    return write_proc("/proc/driver/bgm", s);
}
#endif

static int isp_write_proc(char *s)
{
    return write_proc("/proc/driver/isp", s);
}

static int mipi_write_proc(char *s)
{
    return write_proc("/proc/driver/mipi", s);
}

static int media_trace_write_proc(char *s)
{
    return write_proc("/proc/driver/trace", s);
}
#else
extern int vpu_write_proc(char *s);
#if !defined CONFIG_ARCH_MC632X
extern int enc_write_proc(char *s);
extern int jpeg_write_proc(char *s);
#endif
#ifdef BGM_SUPPORT
extern int bgm_write_proc(char *s);
#endif
extern int isp_write_proc(char *s);
extern int mipi_write_proc(char *s);
extern int media_trace_write_proc(char *);
#endif

void write_media_proc(char *mod, char *config)
{
    // printf("[Write Proc]\"echo %s > %s\"\n", config, mod);
    if (strcmp(mod, "/proc/driver/vpu") == 0)
    {
        vpu_write_proc(config);
    }
    if (strcmp(mod, "/proc/driver/enc") == 0)
    {
#if !defined CONFIG_ARCH_MC632X
        enc_write_proc(config);
#else
        printf("not supported on this platform!\n");
#endif
    }
    if (strcmp(mod, "/proc/driver/jpeg") == 0)
    {
#if !defined CONFIG_ARCH_MC632X
        jpeg_write_proc(config);
#else
        printf("not supported on this platform!\n");
#endif
    }
    if (strcmp(mod, "/proc/driver/bgm") == 0)
    {
#ifdef BGM_SUPPORT
        bgm_write_proc(config);
#else
        printf("not supported on this platform!\n");
#endif
    }
    if (strcmp(mod, "/proc/driver/isp") == 0)
    {
        isp_write_proc(config);
    }
    if (strcmp(mod, "/proc/driver/mipi") == 0)
    {
        mipi_write_proc(config);
    }
    if (strcmp(mod, "/proc/driver/trace") == 0)
    {
        media_trace_write_proc(config);
    }
}
