/*
 * 使用方法：
 * 1,修改文件shell/Makefile,加入以下这一行，
 *   CSRCS += $(wildcard acw_dump.c)
 * 2,修改文件build/extern.ld,加入以下这一行，
 *   EXTERN(acw_dump)
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <pthread.h>

#include "types/bufCtrl.h"
#include "dsp/fh_audio_mpi.h"

#define TP_NUM_MAX       (32)      /*max 32 testpoint*/
#define TP_SIZE_MAX      (64*1024) /*one testpoint size in bytes, it must be multiple of _CACHE_LINE_SIZE__ */
#define VMM_MEM_SIZE_MAX (0x100000)/*max vmm size in bytes */
#define VMM_MEM_ALIGN    (4096)    /*vmm memory align */

#define RING_BUF_READY_FLAG (0x3344991B)

#define _CACHE_LINE_SIZE__ (128)
struct ring_buf
{
    volatile unsigned int  rd;
    volatile unsigned char reserved1[_CACHE_LINE_SIZE__ - sizeof(unsigned int)];

    volatile unsigned int  wr;
    volatile unsigned char reserved2[_CACHE_LINE_SIZE__ - sizeof(unsigned int)];

    volatile unsigned int  size;
    volatile unsigned char reserved3[_CACHE_LINE_SIZE__ - sizeof(unsigned int)];

    volatile unsigned int  overflow;
    volatile unsigned char reserved4[_CACHE_LINE_SIZE__ - sizeof(unsigned int)];

    volatile unsigned int  ready_flag;
    volatile unsigned char reserved5[_CACHE_LINE_SIZE__ - sizeof(unsigned int)];

    unsigned char  buf[0]; /*保证buf刚好在640字节偏移的地方,这样ring_buf_write函数的效率比较高*/
};
typedef struct ring_buf *RING_BUFF_HANDLE;


struct testpoint
{
    int enable;

    int fd;  /*fd for save pcm data*/
    int last_sync_tim;

    RING_BUFF_HANDLE handle;
    unsigned int phy_mem_addr;
    unsigned int phy_mem_size;
};

static unsigned int g_vmm_phy_addr;
static unsigned int g_vmm_size;
static int          g_tp_size;
static int          g_tp_support;
static struct testpoint g_testpoint[TP_NUM_MAX];
static char         g_save_dir[256];
static volatile int g_print_ctrl = 0;

static volatile int g_exit_req = 0;
static volatile int g_running  = 0;

static int get_hex(char *p, unsigned int *val)
{
    int i;
    unsigned int hex = 0;
    unsigned char chr;

    if (!p || p[0] != '0' || p[1] != 'x')
        return -1;

    p += 2;
    for (i=0; i<8; i++)
    {
        chr = p[i];
        if (!chr)
            break;

        if (chr >= '0' && chr <= '9')
        {
            hex <<= 4;
            hex |= (chr - '0');
        }
        else if (chr >= 'A' && chr <= 'F')
        {
            hex <<= 4;
            hex |= (chr - 'A' + 10);
        }
        else if (chr >= 'a' && chr <= 'f')
        {
            hex <<= 4;
            hex |= (chr - 'a' + 10);
        }
        else
        {
            return -1;
        }
    }

    if (p[i] != 0)
        return -1;

    *val = hex;

    return 0;
}

static int alloc_memory_from_vmm(unsigned int *phyaddr, char **viraddr, unsigned int request_size)
{
    int ret;
    MEM_DESC mem;
    static unsigned int alloced_paddr;
    static void         *alloced_vaddr;
    static unsigned int alloced_size;

    if (alloced_size)
    {
        if (request_size > alloced_size)
        {
            printf("acw_dump: not enough memory,alloced_size=0x%08x!\n", alloced_size);
            return -1;
        }

        *phyaddr = alloced_paddr;
        *viraddr = alloced_vaddr;
        return 0;
    }

    ret = buffer_malloc_withname(&mem, request_size, VMM_MEM_ALIGN,  "acw_dump");
    if (ret)
    {
        printf("acw_dump: buffer_malloc_withname failed!\n");
        return -1;
    }

    alloced_paddr = mem.base;
    alloced_vaddr = mem.vbase;
    alloced_size  = request_size;

    *phyaddr = alloced_paddr;
    *viraddr = alloced_vaddr;

    return 0;
}

static void parse_param(char *p, unsigned int *mask)
{
    int ret;
    unsigned int val;

    if (!p)
        return;

    ret = get_hex(p, &val);
    if (ret == 0)
    {
        *mask = val;
    }
    else
    {
        strncpy(g_save_dir, p, sizeof(g_save_dir));
        g_save_dir[sizeof(g_save_dir) - 1] = 0;
    }
}

static int init_test_point(int argc, char *argv[])
{
    int i;
    int ret;
    char *mem_virt = NULL;
    unsigned int mem_phy  = 0;
    unsigned int mem_size = 0;
    unsigned int usrmask  = 0xffffffff;
    int tpnum   = 0;
    int tpsize  = 0;
    int support = 0;
    FH_AC_EXT_IOCTL_CMD ext;

    g_save_dir[0] = 0;
    if (argc >= 2)
        parse_param(argv[1], &usrmask);
    if (argc >= 3)
        parse_param(argv[2], &usrmask);

    ext.cmd = FH_AC_EXT_CMD_TP_GET_TP_NUM;
    support = FH_AC_Ext_Ioctl(&ext);
    if (support <= 0 || support > TP_NUM_MAX)
    {
        printf("acw_dump: internal error, invalid testpoints!\n");
        return -1;
    }

    printf("support %d testpoints\n", support);

    memset(g_testpoint, 0, sizeof(g_testpoint));
    for (i=0; i<TP_NUM_MAX; i++)
    {
        g_testpoint[i].enable = 0;
        g_testpoint[i].fd     = -1;

        if (((1<<i) & usrmask) && (i < support))
        {
            g_testpoint[i].enable = 1;
            tpnum++;
        }
    }

    if (tpnum <= 0)
    {
        printf("acw_dump: invalid user_mask 0x%08x\n", usrmask);
        return -1;
    }

    mem_size = tpnum * TP_SIZE_MAX;
    if (mem_size > VMM_MEM_SIZE_MAX)
    {
        mem_size = VMM_MEM_SIZE_MAX;
    }

    tpsize = (mem_size / tpnum) & (~(_CACHE_LINE_SIZE__ - 1));
    mem_size = tpsize * tpnum;

    printf("tpnum=%d,tpsize=0x%x,mem_size=0x%x\n", tpnum, tpsize, mem_size);

    ret = alloc_memory_from_vmm(&mem_phy, &mem_virt, mem_size);
    if (ret != 0 || !mem_phy)
    {
        printf("Error: not enough VMM memory,need %d bytes.\n", mem_size);
        return -1;
    }

    g_vmm_phy_addr = mem_phy;
    g_vmm_size     = mem_size;
    g_tp_size      = tpsize;
    g_tp_support   = support;

    for (i=0; i<TP_NUM_MAX; i++)
    {
        if (!g_testpoint[i].enable)
            continue;

        g_testpoint[i].handle = (RING_BUFF_HANDLE)mem_virt;
        g_testpoint[i].phy_mem_addr = mem_phy;
        g_testpoint[i].phy_mem_size = tpsize;
        printf("testpoint %d [0x%08x-0x%08x)\n", i, mem_phy, mem_phy + tpsize);

        mem_virt += tpsize;
        mem_phy  += tpsize;
    }

    return 0;
}

static int clear_tp(void)
{
    int ret;
    FH_AC_EXT_IOCTL_CMD ext;

    ext.cmd = FH_AC_EXT_CMD_TP_CLR;
    ret = FH_AC_Ext_Ioctl(&ext);
    if (ret)
    {
        printf("Error: Clear testpoint failed, ret=0x%08x!\n", ret);
    }

    return ret;
}

static int install_test_point(void)
{
    int i;
    unsigned int mask = 0;
    FH_SINT32 ret;
    char fname[256];
    FH_AC_EXT_IOCTL_CMD ext;

    ret = clear_tp();
    if (ret != 0)
        return ret;

    for (i=0; i<TP_NUM_MAX; i++)
    {
        if (i < g_tp_support)
        {
            sprintf(fname, "%s/acw.tp%d.pcm", g_save_dir, i);
            unlink(fname);
        }

        if (g_testpoint[i].enable)
        {
            mask |= (1<<i);
            memset(g_testpoint[i].handle, 0, g_testpoint[i].phy_mem_size);
        }
    }

    ext.cmd = FH_AC_EXT_CMD_TP_RESET;
    ext.args[0] = mask;
    ext.args[1] = g_vmm_phy_addr;
    ext.args[2] = g_tp_size;
    ret = FH_AC_Ext_Ioctl(&ext);
    if (ret)
    {
        printf("Error: FH_AC_EXT_CMD_TP_RESET failed, ret=0x%08x!\n", ret);
        return ret;
    }

    printf("install testpoint success,effective mask %x!\n", mask);
    return 0;
}

static int get_test_point_data(RING_BUFF_HANDLE h, int fd)
{
    int rd;
    int wr;
    int size;
    int ret;
    int magic;
    int read_bytes;

    rd = h->rd;
    wr = h->wr;
    size = h->size;
    magic = h->ready_flag;
    if (magic != RING_BUF_READY_FLAG || rd == wr)
    {
        return 0;
    }

    if (rd > wr)
    {
        read_bytes = size - rd;
    }
    else
    {
        read_bytes = wr - rd;
    }

Retry:
    ret = write(fd, h->buf + rd, read_bytes);
    if (ret != read_bytes)
    {
        if (g_print_ctrl)
        {
            printf("Error: write file failed for audio testpoint,write_len=%d,ret=%d.!\n",read_bytes, ret);
            g_print_ctrl = 0;
        }
    }

    rd += read_bytes;
    if (rd >= size)
        rd -= size;

    if (rd < wr)
    {
        read_bytes = wr - rd;
        goto Retry;
    }

    h->rd = rd;
    return 1;
}

static int capture_test_point(void)
{
    int i;
    int ret = 0;
    char fname[256];

    for (i=0; i<TP_NUM_MAX; i++)
    {
        if (!g_testpoint[i].enable)
            continue;

        if (g_testpoint[i].fd < 0)
        {
            sprintf(fname, "%s/acw.tp%d.pcm", g_save_dir, i);
            g_testpoint[i].fd = open(fname, O_RDWR | O_CREAT | O_TRUNC);
            if (g_testpoint[i].fd < 0)
            {
                if (g_print_ctrl)
                {
                    printf("Error: create file %s failed!\n", fname);
                    g_print_ctrl = 0;
                }
                continue;
            }
            g_testpoint[i].last_sync_tim = time(0);
            /*fchmod(g_testpoint[i].fd, 0644);*/
        }

        ret |= get_test_point_data(g_testpoint[i].handle, g_testpoint[i].fd);
    }

    return ret;
}

static int sync_test_point(void)
{
    int syn = 0;
    int i;
    int now = time(0);
    int diff;

    for (i=0; i<TP_NUM_MAX; i++)
    {
        if (!g_testpoint[i].enable)
            continue;

        if (g_testpoint[i].fd < 0)
            continue;

        diff = now - g_testpoint[i].last_sync_tim;
        if (diff >= 10)
        {
            /*fdatasync(g_testpoint[i].fd);*/
            g_testpoint[i].last_sync_tim = now;
            syn++;
        }
    }

    return syn;
}

static int close_test_point(void)
{
    int i;
    int overflow[TP_NUM_MAX];

    for (i=0; i<TP_NUM_MAX; i++)
    {
        overflow[i] = 0;
        if (!g_testpoint[i].enable)
            continue;

        if (g_testpoint[i].fd < 0)
            continue;

        overflow[i] = g_testpoint[i].handle->overflow;
    }

    for (i=0; i<TP_NUM_MAX; i++)
    {
        if (g_testpoint[i].fd < 0)
            continue;

        if (overflow[i])
        {
            printf("testpoint %d overflow(%d)!\n", i, overflow[i]);
        }
        close(g_testpoint[i].fd);
        g_testpoint[i].fd = -1;
    }

    clear_tp();

    return 0;
}

static void *dump_proc(void *arg)
{
    while (!g_exit_req)
    {
        if (capture_test_point())
            continue;

        if (sync_test_point())
            continue;

        usleep(10*1000);
    }

    close_test_point();

    printf("Finished!\n");
    g_running  = 0;
    g_exit_req = 0;

    return (void *)0;
}

int acw_dump(int argc, char *argv[])
{
    int ret;
    pthread_attr_t attr;
    pthread_t      thd;

    if (g_running)
    {
        g_exit_req = 1;
        while (g_exit_req)
        {
            usleep(20*1000);
        }
        return 0;
    }

    g_print_ctrl = 1;

    ret = FH_AC_Init();
    if (ret != 0)
    {
        printf("Error: FH_AC_Init failed, ret=%08x!\n", ret);
        return ret;
    }

    g_running  = 1;
    g_exit_req = 0;

    ret = init_test_point(argc, argv);
    if (ret != 0)
    {
        g_running  = 0;
        g_exit_req = 0;
        return ret;
    }

    ret = install_test_point();
    if (ret != 0)
    {
        g_running  = 0;
        g_exit_req = 0;
        return ret;
    }

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_attr_setstacksize(&attr, 8 * 1024);
    pthread_create(&thd, &attr, dump_proc, NULL);

    return 0;
}

#include <rttshell.h>
SHELL_CMD_EXPORT(acw_dump, dump audio pcm data);
