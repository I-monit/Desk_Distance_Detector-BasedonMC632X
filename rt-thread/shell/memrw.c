#include <rtthread.h>
#ifdef RT_USING_FINSH
#include "finsh.h"
#endif

static unsigned int fh_hw_readreg(unsigned int addr)
{
    volatile unsigned int *paddr = (volatile unsigned int *)addr;

    return paddr[0];
}

long disp_memory(unsigned int addr, unsigned int length)
{
    unsigned int cnt = length >> 2;

    int i;
    /* unsigned int *pmem = (unsigned int *)addr; */

    rt_kprintf("\r\nMemory info:[%#x,+%#x]\n", addr, length);
    for (i = 0; i < cnt; i++)
    {
        unsigned int v[4];
        int j;
        unsigned char *pb = (unsigned char *)v;

        v[0] = fh_hw_readreg(i * 16 + addr);
        v[1] = fh_hw_readreg(i * 16 + 4 + addr);
        v[2] = fh_hw_readreg(i * 16 + 8 + addr);
        v[3] = fh_hw_readreg(i * 16 + 12 + addr);
        rt_kprintf("%#x: %08x %08x %08x %08x ", addr + i * 16, v[0], v[1], v[2], v[3]);
        for (j = 0; j < 16; j++)
        {
            if (pb[j] > 31 && pb[j] < 127)
            {
                rt_kprintf("%c", pb[j]);
            }
            else
            {
                rt_kprintf(".");
            }
        }
        rt_kprintf("\n");
    }
    if (length & 3)
    {
        int j, cnt;
        int v[4];
        unsigned char *pb = (unsigned char *)v;

        cnt = length & 3;
        rt_kprintf("%#x:", addr + i * 16);
        for (j = 0; j < cnt; j++)
        {
            v[j] = fh_hw_readreg(i * 16 + addr + j * 4);
            rt_kprintf(" %08x", v[j]);
        }
        for (j = 0; j < (4-cnt); j++)
            rt_kprintf("         ");
        rt_kprintf(" ");
        for (j = 0; j < 4 * cnt; j++)
        {
            if (pb[j] > 31 && pb[j] < 127)
                rt_kprintf("%c", pb[j]);
            else
                rt_kprintf(".");
        }
        rt_kprintf("\n");
    }

    /*
     *
     *
     * fh_hw_readreg(i * 4 + 0), pmem[i * 4 + 1], pmem[i * 4 + 2], pmem[i * 4 + 3]
     *
     *
     *
     *
     *
     */

    rt_kprintf("\r\n");

    return 0;
}

static void fh_hw_writereg(unsigned int addr, unsigned int val)
{
    volatile unsigned int *paddr = (volatile unsigned int *)addr;

    paddr[0] = val;
}

long set_memory(unsigned int addr, unsigned int val)
{
    fh_hw_writereg(addr, val);
    return 0;
}
/* #endif */

#ifdef RT_USING_FINSH
extern rt_uint32_t __vsymtab_start;
extern rt_uint32_t __vsymtab_end;
void show_symval(char *name)
{
    struct finsh_sysvar *temp;

    if (name == RT_NULL)
    {
        rt_kprintf("Input Globe Value name\n");
        return;
    }
    temp = (struct finsh_sysvar *)&__vsymtab_start;
    for ( ; (rt_uint32_t)temp <= (rt_uint32_t)(&__vsymtab_end); temp++)
    {
        if (!rt_strcmp(temp->name, name))
        {
            disp_memory((rt_uint32_t)temp->var, temp->type);
            return;
        }
    }
    rt_kprintf("No search for %s\n", name);
}
#endif

static void mem_usage(void)
{
    rt_kprintf("memory read/write operation.\n");
    rt_kprintf("Usage:\n");
    rt_kprintf("  memrw -h:            show this help\n");
    rt_kprintf("  memrw -r addr [len]  show memory contents\n");
    rt_kprintf("  memrw -w addr val    write val to memory\n");
}

static int getHex(char ch)
{
    if (ch >= '0' && ch <= '9')
        return ch - '0';
    if (ch >= 'A' && ch <= 'F')
        return 10 + ch - 'A';
    if (ch >= 'a' && ch <= 'f')
        return 10 + ch - 'a';
    return -1;
}

static unsigned int conv_addr(char *buf)
{
    if ((strncmp(buf, "0x", 2) == 0) || (strncmp(buf, "0X", 2) == 0))
    {
        unsigned int addr = 0;
        int len = rt_strlen(buf);
        int i = 0;

        while (i < len - 2)
        {
            int cHex = getHex(buf[i + 2]);

            if (cHex < 0)
                return 0;
            addr = (addr << 4) + cHex;
            i++;
        }
        return addr;
    }
    else
    {
        return rt_atoi(buf);
    }
}

void memrw(int argc, char *argv[])
{
    if (argc < 2)
    {
        mem_usage();
        return;
    }
    if (strcmp(argv[1], "-h") == 0)
    {
        mem_usage();
        return;
    }
    if (strcmp(argv[1], "-r") == 0)
    {
        if (argc == 3)      /* mem -r addr */
        {
            unsigned int addr = conv_addr(argv[2]);

            if (addr == 0)
            {
                rt_kprintf("Invalid arguments!\n");
                return;
            }

            disp_memory(addr, 0x40);
            return;
        }
        if (argc == 4)      /* mem -r addr len */
        {
            unsigned int addr = conv_addr(argv[2]);
            unsigned int alen = conv_addr(argv[3]);

            if (addr == 0)
            {
                rt_kprintf("Invalid address.\n");
                return;
            }
            if (alen == 0)
            {
                rt_kprintf("Invalid length.\n");
                return;
            }

            disp_memory(addr, alen);
            return;
        }
    }
    if (strcmp(argv[1], "-w") == 0)
    {
        if (argc != 4)      /* MUST be mem -w addr val */
        {
            mem_usage();
            return;
        }
        unsigned int addr = conv_addr(argv[2]);
        unsigned int val  = conv_addr(argv[3]);

        if (addr == 0)
        {
            rt_kprintf("Invalid input arguments.\n");
            return;
        }

        set_memory(addr, val);
        return;
    }

    mem_usage();

}

#ifdef RT_USING_FINSH
#ifdef FINSH_USING_MSH
MSH_CMD_EXPORT(memrw, memory read/write operation)
#else
FINSH_FUNCTION_EXPORT(disp_memory, display memory info)
FINSH_FUNCTION_EXPORT(set_memory, set memory/register value)
FINSH_FUNCTION_EXPORT(show_symval, show global val)
#endif
#endif
