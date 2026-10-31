#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <rtthread.h>
#include "drivers/mtd_nor.h"
#include "drivers/mtd_nand.h"

#include "flash_load.h"
#include "fh_def.h"
#include "fh_arch.h"
#include "fh_chip.h"
#include "fh_clock.h"
#include "platform_def.h"
#include "timer.h"      /* for read_pts()   */
#include "mmu.h"
#include "delay.h"

#ifdef FH_USING_ZLIB
#include <zlib.h>
#endif

#define MEM_ALLOC_ALIGN         (128)

/* for 2-wire flash load, PRE_LOAD_SIZE must be at least 20% of the total load size */
#if defined(FH_FAST_BOOT) && !defined(CTL_QUAD_WIRE_SUPPORT) && !defined(CTL_DUAL_WIRE_SUPPORT)
#error "2/4 wire spi flash mode MUST be configed while FASTBOOT Enabled."
#endif

#ifdef FH_ENABLE_SELF_LOAD_CODE     /* stage load defined */

/* #define FL_DEBUG */

#ifdef FL_DEBUG
#define FL_Info                 rt_kprintf
#else
#define FL_Info(...)
#endif
#define FL_Error                rt_kprintf

extern unsigned char __stage_30_start, __stage_30_end;
extern unsigned long *__stage_10_start;

extern void mmu_clean_dcache(rt_uint32_t addr, rt_uint32_t size);
extern void mmu_invalidate_dcache(rt_uint32_t addr, rt_uint32_t size);

extern int unlzo(unsigned char *buf, int len, void *f1, void *f2,
                 unsigned char *obuf, int *pos, unsigned int *olen, void *e);
extern int unlzma(unsigned char *buf, int len, void *f1, void *f2,
                unsigned char *obuf, int *pos, unsigned int *olen, void *e);

struct tag_codeheader
{
    unsigned int magic;
    unsigned int hdrlen;
    unsigned int szcode;
    unsigned int compressed;
};

struct tag_codeloader
{
    unsigned int  mtd_num;      /* mtd number of code in flash */
    unsigned int  compressed;   /* image compress format */
    unsigned int  ldsize;       /* image size to load */
    unsigned int  mtd_offset;   /* image offset in flash, byte */
    unsigned int  ram_addr;     /* ram address for code to load into */
    unsigned int  ram_size;     /* ram size for code */
    unsigned int  ram_buffer;   /* temp buffer for decompressor */
    unsigned int  buf_headlen;  /* header length in ram_buffer */
    unsigned int  loaded_size;  /* code size already loaded. */
    unsigned int  code_size;    /* code size after decompress */
};

static int g_sys_loader = 1;        /* patch for decompressor */
static struct tag_codeloader g_loader_info;

#ifdef CONFIG_STARTUP_TIMECOST
static unsigned int g_stc_load[3];      /* ARC/APP/NBG */
static unsigned int g_stc_loadsize[3];
static unsigned int g_stc_decompress[3];
static unsigned int g_stc_decwait = 0;

void print_loadcode_timecost(void)
{
    rt_kprintf("\n");
    rt_kprintf("      Flash Load time cost:\n");

    if (g_stc_load[CODE_ARM_SEG2] == 0)
    {
        rt_kprintf("            User Code cost:     [TFTP]\n");
    }
    else
    {
        rt_kprintf("            User Code cost: % 10d,", g_stc_load[CODE_ARM_SEG2]);
        rt_kprintf("    speed: %d(KB/s)\n", g_stc_loadsize[CODE_ARM_SEG2] * 1000 / g_stc_load[CODE_ARM_SEG2]);
        rt_kprintf("           decompress cost: % 10d\n", g_stc_decompress[CODE_ARM_SEG2]);
    }
#ifdef FH_USING_VBUS
    rt_kprintf("            VBUS Code cost: % 10d,", g_stc_load[CODE_ARC]);
    rt_kprintf("    speed: %d(KB/s)\n", g_stc_loadsize[CODE_ARC] * 1000 / g_stc_load[CODE_ARC]);
    rt_kprintf("           decompress cost: % 10d\n", g_stc_decompress[CODE_ARC]);
#endif
#if (defined(CONFIG_ARCH_FH865x) || defined(CONFIG_ARCH_FH8636_FH8852V20X) || defined (CONFIG_ARCH_FH885xV310) || \
    defined(CONFIG_ARCH_FH8852V301_FH8662V100))
    rt_kprintf("             NBG Code cost: % 10d,", g_stc_load[CODE_NBG_FILE]);
    rt_kprintf("    speed: %d(KB/s)\n", g_stc_loadsize[CODE_NBG_FILE] * 1000 / g_stc_load[CODE_NBG_FILE]);
    rt_kprintf("           decompress cost: % 10d\n", g_stc_decompress[CODE_NBG_FILE]);
#endif
}
#endif

void gunzip_report(char *szbuf)
{
    unsigned int v_start = (unsigned int)&__stage_30_start;

    FL_Info("\n\n");
    FL_Info(szbuf);
    FL_Info("\n\n ------------ System Halted.\n");

    write_reg(v_start, 0);       /* mark code loaded. */
    while (1)
        rt_thread_delay(1000);
}

int pull_datum_size(unsigned int ptr, unsigned int size)
{
    unsigned int ptr_end;
    unsigned int cur_size;
#ifdef CONFIG_STARTUP_TIMECOST
    unsigned int pts1, pts2;
#endif
    struct tag_codeloader *ldinfo = &g_loader_info;

    if (g_sys_loader == 0)      /* normal use, do nothing */
        return 0;

    ptr_end = ptr + size;
    /* judge if post the ends of the stream */
    if (ptr_end > ldinfo->ram_buffer + ldinfo->ldsize)
        ptr_end = ldinfo->ram_buffer + ldinfo->ldsize;

    /* calculate the loaded size */
    cur_size = ldinfo->ram_buffer + ldinfo->loaded_size;
#ifdef CONFIG_STARTUP_TIMECOST
    pts1 = read_pts();
#endif
    while (cur_size < ptr_end)      /* if no stream available, wait */
    {
        fh_udelay(1000);
        cur_size = ldinfo->ram_buffer + ldinfo->loaded_size;
    }

#ifdef CONFIG_STARTUP_TIMECOST
    pts2 = read_pts();
    g_stc_decwait += pts2 - pts1;
#endif
    /* invalidate stream buffer */
    mmu_invalidate_dcache(ptr, cur_size - ptr);
    return cur_size - ptr;
}

#ifdef FH_USING_ZLIB
static void *gmalloc(void *q, unsigned int m, unsigned int n)
{
    return rt_calloc(m, n);
}

static void gfree(void *q, void *p)
{
    rt_free(p);
}

static int fh_unzip(unsigned char *bufIn, int inLen, unsigned char *bufOut, unsigned int *outLen)
{
    int ret;
    z_stream strm;

    strm.zalloc = gmalloc;
    strm.zfree  = gfree;
    strm.opaque = NULL;
    strm.next_in = (unsigned char *)g_loader_info.ram_buffer;
    strm.avail_in = g_loader_info.loaded_size;
    strm.next_out = (unsigned char *)g_loader_info.ram_addr;
    strm.avail_out = g_loader_info.ram_size;

    ret = inflateInit2(&strm, 31);
    if (ret != Z_OK)
    {
        return -1;
    }
    while (1)
    {
        ret = inflate(&strm, Z_NO_FLUSH);
        if (ret != Z_OK && ret != Z_STREAM_END)
        {
            rt_kprintf("decompress stream failed: ret = %d\n", ret);
            while (1)
                rt_thread_delay(10000);
        }
        else if (ret == Z_STREAM_END)
            break;
    }
    g_loader_info.code_size = (unsigned int)strm.next_out - g_loader_info.ram_addr;
    inflateEnd(&strm);
    return 0;
}
#endif

static void copy_mem_code(void)
{
    while (g_loader_info.loaded_size != g_loader_info.ldsize)
    {
        fh_udelay(1000);
    }
    rt_memcpy((void *)g_loader_info.ram_addr, (void *)g_loader_info.ram_buffer, g_loader_info.ldsize);
}

static int fbv_decompress(int method, unsigned char *buf, int len,
                          unsigned char *obuf, unsigned int *olen)
{
    int ret;

    switch (method)
    {
    case CRYPTO_NONE: /* no compress */
        *olen = len;
        copy_mem_code();
        ret = 0;
        break;
    case CRYPTO_LZO: /* LZO */
#ifdef FH_USING_LZOP
        ret = unlzo((unsigned char *)g_loader_info.ram_buffer, g_loader_info.ldsize, RT_NULL, RT_NULL,
                    (unsigned char *)g_loader_info.ram_addr, RT_NULL, &g_loader_info.code_size, gunzip_report);
#else
        rt_kprintf("LZO format not supported.\n");
        ret = -ENOSYS;
#endif
        break;
    case CRYPTO_GZIP: /* GZIP */
#ifdef FH_USING_ZLIB
        ret = fh_unzip((unsigned char *)g_loader_info.ram_buffer, g_loader_info.ldsize,
                     (unsigned char *)g_loader_info.ram_addr, &g_loader_info.code_size);
#else
        rt_kprintf("GZIP format not supported.\n");
        ret = -ENOSYS;
#endif
        break;
    default:    /* not supported */
        rt_kprintf("un-recognized compress method detected: %d\n", method);
        ret = -ENOSYS;
        break;
    }

    return ret;
}

void load_flash_code(int idx, unsigned int offset, unsigned char *ram_addr, unsigned int len)
{
    char mtdname[20];
    rt_device_t flash_dev;

    rt_sprintf(mtdname, "mtd%d", idx);
    flash_dev = rt_device_find(mtdname);
    if (flash_dev == RT_NULL)
    {
        rt_kprintf("device(%s) not found to load code.\n", mtdname);
        return;
    }
    mmu_invalidate_dcache((unsigned int)ram_addr, len);

#ifdef RT_USING_FAL_SFUD_ADAPT
    rt_mtd_nor_read(RT_MTD_NOR_DEVICE(flash_dev), offset, ram_addr, len);
#elif defined FH_USING_NAND_FLASH
    rt_mtd_nand_read(RT_MTD_NAND_DEVICE(flash_dev), offset, ram_addr, len);
#endif
}

extern void *rt_memalign(rt_size_t boundary, rt_size_t size);
int get_code_part_info(void)
{
    int part_idx;
    struct tag_codeheader *hdrbuf = RT_NULL;
    unsigned int v_ramst = (unsigned int)&__stage_10_start;
    unsigned int v_size1 = (unsigned int)&__stage_30_start;
    unsigned int sfl_offset = 0;

    /* 第二段代码有偏移 */
    part_idx = g_loader_info.mtd_num;
    if (part_idx == __get_code_part_idx(CODE_ARM_SEG2))
        sfl_offset = v_size1 - v_ramst + 0x1000;

    hdrbuf = (struct tag_codeheader *)rt_memalign(MEM_ALLOC_ALIGN, FLASH_LOAD_SIZE);
    if (hdrbuf == RT_NULL)
        return -RT_ENOMEM;
    load_flash_code(part_idx, sfl_offset, (unsigned char *)hdrbuf, FLASH_LOAD_SIZE);

    if (hdrbuf->magic != HDR_MAGIC_NUMBER)
    {
        FL_Error("magic wrong: %#x, should be: %#x.\n", hdrbuf->magic, HDR_MAGIC_NUMBER);
        rt_free(hdrbuf);
        return -5;                                  /* not a valid segment */
    }
    if (hdrbuf->hdrlen > FLASH_LOAD_SIZE)
    {
        FL_Error("Too big header size 0x%08x[%d].\n", hdrbuf->hdrlen, part_idx);
        rt_free(hdrbuf);
        return -6;                                  /* invalid segment header */
    }
    FL_Info("part %d: offset 0x%08x, size 0x%08x, crypt: %d\n", part_idx,
            sfl_offset, hdrbuf->szcode, hdrbuf->compressed);

    g_loader_info.buf_headlen = hdrbuf->hdrlen;
    g_loader_info.compressed  = hdrbuf->compressed;
    g_loader_info.mtd_offset  = sfl_offset;                 /* 4K aligned */
    g_loader_info.ldsize      = hdrbuf->szcode;

    rt_free(hdrbuf);
    return 0;
}

static rt_sem_t __g_dec_lock = RT_NULL;
static void init_dec_sync(void)
{
    __g_dec_lock = rt_sem_create("fbv_dec", 0, RT_IPC_FLAG_FIFO);
}

static void wait_decompress_done(void)
{
    rt_sem_take(__g_dec_lock, RT_WAITING_FOREVER);
    rt_sem_delete(__g_dec_lock);
}

static void notify_dec_end(void)
{
    rt_sem_release(__g_dec_lock);
}

static void fbv_dec_thrd(void *param)
{
#ifdef CONFIG_STARTUP_TIMECOST
    unsigned int ptsend;

    g_stc_decwait = read_pts();
#endif
    fbv_decompress(g_loader_info.compressed, (unsigned char *)g_loader_info.ram_buffer, g_loader_info.ldsize,
                   (unsigned char *)g_loader_info.ram_addr, &g_loader_info.ram_size);
#ifdef CONFIG_STARTUP_TIMECOST
    ptsend = read_pts();
    g_stc_decwait = ptsend - g_stc_decwait;
#endif
    mmu_clean_dcache(g_loader_info.ram_addr, g_loader_info.code_size);

    notify_dec_end();
}

static void startup_decompress(void)
{
    static int tidx = 0;
    char tname[20];
    rt_thread_t fbv_tid;
    int pri = RT_THREAD_PRIORITY_MAX - 10;      /* use a low priority */

    init_dec_sync();
    rt_sprintf(tname, "fdec_%d", tidx++);
    fbv_tid = rt_thread_create(tname, fbv_dec_thrd, RT_NULL, 0x4000, pri, 10);
    if (fbv_tid != RT_NULL)
        rt_thread_startup(fbv_tid);
}

int load_code(int type, unsigned char *codbase, unsigned int *csize)
{
    int ret, i, blk_64K, sztail;
    int part_idx = 0, preldcnt = 0;
    unsigned char *st2buf, *ramaddr;
    unsigned int ldsize, preldsize, offset;
    const char *sztype[3] = { "ARC", "SEG2", "NBG" };
#ifdef CONFIG_STARTUP_TIMECOST
    unsigned int pts1, pts2;
#endif

    part_idx = __get_code_part_idx(type);
    if (part_idx < 0)
    {
        FL_Error("un-recognized code type: %d\n", type);
        return -ENXIO;
    }

#ifdef CONFIG_STARTUP_TIMECOST
    pts1 = read_pts();
#endif
    rt_memset(&g_loader_info, 0, sizeof(g_loader_info));
    g_loader_info.mtd_num = part_idx;                   /* mtd block */
    g_loader_info.ram_addr = (unsigned int)codbase;     /* destination address */
    g_loader_info.ram_size   = (unsigned int)*csize;    /* destination buffer size */
    ret = get_code_part_info();         /* get code size/compress info */
    if (ret != 0)
    {
        FL_Error("no code found for %s\n", sztype[type]);
        return -ENXIO;
    }
    if (g_loader_info.ldsize == 0)
    {
        FL_Error("code size is 0.\n");
        return 0;
    }

    ldsize = FL_ALIGNUP(g_loader_info.ldsize + g_loader_info.buf_headlen);
    st2buf = (unsigned char *)rt_memalign(MEM_ALLOC_ALIGN, ldsize);
    if (st2buf == RT_NULL)
    {
        FL_Error("malloc code memory failed.\n");
        return  -ENOMEM;
    }
    g_loader_info.ram_buffer = (unsigned int)(st2buf + g_loader_info.buf_headlen);      /* tmp buffer */

    preldsize = (0x10000 - (g_loader_info.mtd_offset & 0xffff)) & 0xffff;       /* alignment for 64K */
    if (preldsize > g_loader_info.ldsize)       /* if user code is tooooo small */
    {
        preldsize = g_loader_info.ldsize + g_loader_info.buf_headlen;
    }
    if (preldsize > 0)
    {
        load_flash_code(part_idx, g_loader_info.mtd_offset, st2buf, preldsize);
        g_loader_info.loaded_size = preldsize - g_loader_info.buf_headlen;
    }
    else
    {
        g_loader_info.loaded_size = 0;
    }

    blk_64K = (g_loader_info.ldsize + g_loader_info.buf_headlen - preldsize) >> 16;
    sztail  = (g_loader_info.ldsize + g_loader_info.buf_headlen - preldsize) & 0xffff;

    if (blk_64K > 4)
        preldcnt = 4;       /* load MAX 256KB for initial decompress */
    else
        preldcnt = blk_64K;
    load_flash_code(part_idx, g_loader_info.mtd_offset + preldsize,
                   (rt_uint8_t *)(st2buf + preldsize), preldcnt << 16);
    g_loader_info.loaded_size += preldcnt << 16;

    startup_decompress();         /* startup decompress thread */

    for (i = preldcnt; i < blk_64K; i++)                /* go on to load remaining codes */
    {
        offset = g_loader_info.mtd_offset + preldsize + (i << 16);
        ramaddr = (rt_uint8_t *)(st2buf + preldsize + (i << 16));
        load_flash_code(part_idx, offset, ramaddr, 0x10000);
        g_loader_info.loaded_size += 0x10000;
    }

    if (sztail > 0)         /* load last block less than 64KB */
    {
        offset = g_loader_info.mtd_offset + preldsize + (i << 16);
        ramaddr = (rt_uint8_t *)(st2buf + preldsize + (i << 16));
        load_flash_code(part_idx, offset, ramaddr, sztail);
        g_loader_info.loaded_size += sztail;
    }
#ifdef CONFIG_STARTUP_TIMECOST
    pts2 = read_pts();
    g_stc_loadsize[type] = g_loader_info.ldsize;
#endif
    /* wait decompress done */
    wait_decompress_done();

#ifdef CONFIG_STARTUP_TIMECOST
    g_stc_load[type] = pts2 - pts1;
    g_stc_decompress[type] = g_stc_decwait;
    g_stc_decwait = 0;
#endif
    rt_free(st2buf);
    if (ret != 0)
    {
        FL_Error("decompress code failed: %d\n", ret);
        return -RT_EIO;
    }

    if (g_loader_info.code_size > g_loader_info.ram_size)
    {
        FL_Error("given buffer is too small, code size: %d, buffer size: %d\n",
                    g_loader_info.code_size, g_loader_info.ram_size);
        return -RT_EFULL;
    }

    FL_Info("user app code loaded!.................\n");

    return 0;
}

#ifdef FH_USING_VBUS
#include <xbus_loader.h>
static unsigned int arc_clock_get(void)
{
    unsigned int clkv = 0;
    struct clk *arc_clk;

    arc_clk = clk_get(NULL, "arc_clk");
    if (arc_clk)
    {
        clkv = clk_get_rate(arc_clk);
    }

    return clkv;
}

static int read_firmware(unsigned int address)
{
    unsigned int codsize = FH_ARCOS_SIZE;

    if (load_code(CODE_ARC, (unsigned char *)address, &codsize) == 0)
    {
        *((unsigned int *)(address+(256+32-4))) = arc_clock_get();
        rt_kprintf("xbus: load arc @%08x,size=%d\n", address, codsize);
        return codsize;
    }

    rt_kprintf("xbus: load arc @%08x failed!\n", address);
    return -1;
}

static int load_arc_with_xbus(void)
{
    unsigned int arc_base = FH_DDR_END - FH_ARCOS_SIZE;

    return xbus_loader_init(XBUS_FW_LOAD_POLICY_ALWAYS, arc_base, read_firmware, 8, 10/*10 seconds timeout*/);
}
#endif

int load_nbg_file(unsigned char *nn_buf, unsigned int nn_size)
{
    return load_code(CODE_NBG_FILE, nn_buf, &nn_size);
}

void load_seg30(void)
{
    unsigned char *v_start = (unsigned char *)&__stage_30_start;
    unsigned char *v_end   = (unsigned char *)&__stage_30_end;
    unsigned int realsize  = ((int)v_end - (int)v_start);
    unsigned int codsize = realsize;

    load_code(CODE_ARM_SEG2, v_start, &codsize);
    if (codsize != realsize)
    {
        rt_kprintf("Code Size NOT match!!!\ndecompress size: %d, want: %d\n", codsize, realsize);

        while(1);
    }
}

int seg30_loaded(void)
{
    unsigned int v_start = (unsigned int)&__stage_30_start;
    unsigned int v_end_m = (unsigned int)&__stage_30_end - 4;
    unsigned int ms = read_reg(v_start);
    unsigned int me = read_reg(v_end_m);

    if ((ms == CODE_MAGIC_NUM) && (me == CODE_MAGIC_NUM))
        return 1;
    return 0;
}

void mark_loaded(void)
{
    unsigned int v_start = (unsigned int)&__stage_30_start;
    unsigned int v_end_m = (unsigned int)&__stage_30_end - 4;

    write_reg(v_start, 0);
    write_reg(v_start - 4, 0);
    write_reg(v_end_m, 0);

    mmu_clean_dcache(v_start, 4);
    mmu_clean_dcache(v_end_m - 28, 4);
}

void load_application(void)
{
#ifdef FH_USING_VBUS
    load_arc_with_xbus();
#endif

    if (seg30_loaded() == 0)
        load_seg30();
    mark_loaded();
    g_sys_loader = 0;
}
#endif
