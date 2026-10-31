#include <rtthread.h>
#include <finsh.h>

#ifdef FH_ENABLE_VIDEO
extern int media_read_proc();
extern int media_mem_proc();
extern int vpu_read_proc();
extern int vpu_write_proc(char *s);
extern int jpeg_read_proc();
extern int jpeg_write_proc(char *s);
#if 0
extern int bgm_read_proc(void);
extern int bgm_write_proc(char *s);
#endif
extern int vise_read_proc(void);
/*extern void cmm_mem_proc(int index);*/
extern int isp_read_proc(void);
extern int isp_write_proc();
extern int mipi_read_proc(void);
extern int mipi_write_proc(char *s);
extern int nna_read_proc(void);
extern int nna_write_proc(char *s);
extern int media_trace_read_proc(void);
extern int media_trace_write_proc(char *);
#if defined CONFIG_ARCH_MC632X
extern int venc_read_proc(void);
extern int Vicap_ReadProc(void);
extern int Vicap_WriteProc(char *s);
extern int DRVI_VO_ProcRead(void);
extern int jpeg_hld_read_proc(void);
#else
extern int vicap_read_proc(void);
extern int vicap_write_proc(char *s);
extern int enc_read_proc(void);
extern int enc_write_proc(char *s);
#endif

#ifdef FINSH_USING_MSH
/* int cmd_cmm_mem_proc(int argc, char *argv[])
   {
   int index;
   if (argc < 2)
   {
   rt_kprintf("Usage: cmm_mem_proc index\n");
   return 0;
   }
   index = rt_atoi(argv[1]);
   cmm_mem_proc(index);
   return 0;
   }*/

static void print_usage()
{
    rt_kprintf("Usage:\n");
    rt_kprintf("  media_info [-r] (isp|vicap|vpu|enc|jpeg|bgm|vise|media|trace|mem|mipi|nna|vou) read media module status info.\n");
    rt_kprintf("  media_info -w (isp|vicap|vpu|enc|jpeg|bgm|mipi|trace) set_string  write media module config string.\n");
    rt_kprintf("  media_info [-h]                                       show this help\n");
}

static int read_media_info(char *mod)
{
    int ret = 0;

    if (strcmp(mod, "isp") == 0)
    {
        isp_read_proc();
    }
    if (strcmp(mod, "vicap") == 0)
    {
#if defined CONFIG_ARCH_MC632X
    Vicap_ReadProc();
#else
    vicap_read_proc();
#endif
    }
    if (strcmp(mod, "vpu") == 0)
    {
        vpu_read_proc();
    }
    if (strcmp(mod, "enc") == 0)
    {
#if defined CONFIG_ARCH_MC632X
        venc_read_proc();
#else
        enc_read_proc();
#endif
    }
    if (strcmp(mod, "jpeg") == 0)
    {
#if !defined CONFIG_ARCH_MC632X
        jpeg_read_proc();
#else
        jpeg_hld_read_proc();
#endif
    }
    if (strcmp(mod, "bgm") == 0)
    {
#ifdef BGM_SUPPORT
        bgm_read_proc();
#else
        rt_kprintf("not supported on this platform!\n");
        ret = -1;
#endif
    }
    if (strcmp(mod, "vise") == 0)
    {
        vise_read_proc();
    }
    if (strcmp(mod, "media") == 0)
    {
        media_read_proc();
    }
    if (strcmp(mod, "trace") == 0)
    {
        media_trace_read_proc();
    }
    if (strcmp(mod, "mem") == 0)
    {
        media_mem_proc();
    }
    if (strcmp(mod, "mipi") == 0)
    {
        mipi_read_proc();
    }
    if (strcmp(mod, "nna") == 0)
    {
        nna_read_proc();
    }
    if  (strcmp(mod, "vou") == 0)
    {
#if defined CONFIG_ARCH_MC632X
       DRVI_VO_ProcRead();
#else
       rt_kprintf("not supported on this platform!\n");
#endif
    }
    return ret;
}

int write_media_info(char *mod, char *config)
{
    int ret = 0;

    if (strcmp(mod, "isp") == 0)
    {
        isp_write_proc(config);
    }
    if (strcmp(mod, "vicap") == 0)
    {
#if defined CONFIG_ARCH_MC632X
    Vicap_WriteProc(config);
#else
    vicap_write_proc(config);
#endif
    }
    if (strcmp(mod, "vpu") == 0)
    {
        vpu_write_proc(config);
    }
    if (strcmp(mod, "enc") == 0)
    {
#if defined CONFIG_ARCH_MC632X
        rt_kprintf("not supported on this platform!\n");
        ret = -1;
#if 0
        venc_writ_proc(config);
#endif
#else
        enc_write_proc(config);
#endif
    }
    if (strcmp(mod, "jpeg") == 0)
    {
#if !defined CONFIG_ARCH_MC632X
        jpeg_write_proc(config);
#else
        rt_kprintf("not supported on this platform!\n");
        ret = -1;
#endif
    }
    if (strcmp(mod, "bgm") == 0)
    {
#ifdef BGM_SUPPORT
        bgm_write_proc(config);
#else
        rt_kprintf("not supported on this platform!\n");
        ret = -1;
#endif
    }
    if (strcmp(mod, "trace") == 0)
    {
        media_trace_write_proc(config);
    }
    if (strcmp(mod, "mipi") == 0)
    {
        mipi_write_proc(config);
    }
    if (strcmp(mod, "nna") == 0)
    {
        nna_write_proc(config);
        return 0;
    }

    return ret;
}

int cmd_media_info(int argc, char *argv[])
{
    if (argc < 2)
    {
        print_usage();
        return 0;
    }
    if (strcmp(argv[1], "-h") == 0)
    {
        print_usage();
        return 0;
    }
    if (strcmp(argv[1], "-r") == 0)
    {
        if (argc != 3)
        {
            print_usage();
            return 0;
        }

        if (read_media_info(argv[2]) == 0)
            return 0;

        print_usage();
        return 0;
    }

    if (argv[1][0] != '-')
    {
        if (read_media_info(argv[1]) == 0)
            return 0;

        print_usage();
        return 0;
    }

    if (strcmp(argv[1], "-w") == 0)
    {
        if (argc != 4)
        {
            print_usage();
            return 0;
        }
        if (write_media_info(argv[2], argv[3]) == 0)
            return 0;
    }

    print_usage();

    return 0;
}
/*FINSH_FUNCTION_EXPORT_ALIAS(cmd_cmm_mem_proc, __cmd_cmm_mem_proc, get cmm memory status information);*/
FINSH_FUNCTION_EXPORT_ALIAS(cmd_media_info, __cmd_media_info, read / write media status information);
#else
FINSH_FUNCTION_EXPORT(isp_read_proc, read proc info);
FINSH_FUNCTION_EXPORT(isp_write_proc, write proc info);

FINSH_FUNCTION_EXPORT(media_mem_proc, media mem use info.e.g
                      : media_mem_proc());
FINSH_FUNCTION_EXPORT(jpeg_write_proc, write jpeg proc info);
FINSH_FUNCTION_EXPORT(jpeg_read_proc, read jpeg proc info);
FINSH_FUNCTION_EXPORT(media_read_proc, get media process proc info);
FINSH_FUNCTION_EXPORT(enc_write_proc, write enc proc info);
FINSH_FUNCTION_EXPORT(enc_read_proc, read enc proc info);
FINSH_FUNCTION_EXPORT(vpu_write_proc, write vpu proc info);
FINSH_FUNCTION_EXPORT(vpu_read_proc, read vpu proc info);
FINSH_FUNCTION_EXPORT(vise_read_proc, read vise proc info);
#endif
#endif
