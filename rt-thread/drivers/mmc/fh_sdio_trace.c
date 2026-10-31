#ifdef SDIO_TRACE_ENABLE

#include "timer.h"

extern void get_call_stack(unsigned int *callerfn, int num);

#define SDIO_CALL_STACK_SIZE (6)
struct trace_node
{
    int       nlen; /*node ocuppied length*/
    int       clen; /*content real length*/

    char    b_cmd53;
    char    b_write;
    char    reserved[2];

    unsigned int addr;
    unsigned int fn;
    unsigned int bcnt;
    unsigned int bsize;

    unsigned int pts;

    unsigned int callfn[SDIO_CALL_STACK_SIZE];

    unsigned char data[0];
};

static int g_sdio_trace_buf[1024*1024/4];
static int g_sdio_buf_pos = 0;

int sdio_add_trace_node(int b_cmd53, int b_write,
        unsigned int addr, unsigned int fn,
        unsigned int bcnt,  unsigned int bsize,
        unsigned char *buf, int len)
{
    int nlen;
    struct trace_node *n;
    rt_base_t temp;

    nlen = ((len + 3) & (~3));

    temp = rt_hw_interrupt_disable();

    if (g_sdio_buf_pos + sizeof(struct trace_node) + nlen > sizeof(g_sdio_trace_buf))
    {
        rt_hw_interrupt_enable(temp);
        rt_kprintf("+++g_sdio_trace_buf overflow!\n");
        return -1;
    }

    n = (struct trace_node *)((char *)g_sdio_trace_buf + g_sdio_buf_pos);
    n->nlen = nlen;
    n->clen = len;
    n->b_cmd53 = b_cmd53;
    n->b_write = b_write;
    n->addr = addr;
    n->fn = fn;
    n->bcnt = bcnt;
    n->bsize = bsize;
    n->pts = read_pts();
    get_call_stack(n->callfn, SDIO_CALL_STACK_SIZE);
    memcpy(n->data, buf, len);

    g_sdio_buf_pos += (sizeof(struct trace_node) + nlen);

    rt_hw_interrupt_enable(temp);

    return 0;
}

void sdio_dump_trace_info(void)
{
    struct trace_node *n;
    int pos = 0;
    int nlen;
    int clen;
    int i;

    while (pos < g_sdio_buf_pos)
    {
        n = (struct trace_node *)((char *)g_sdio_trace_buf + pos);
        nlen = n->nlen;
        clen = n->clen;

        if (n->b_cmd53) /*CMD53*/
        {
            if (n->b_write)
            {
                rt_kprintf("\n++CMD 53 write:");
            }
            else
            {
                rt_kprintf("\n++CMD 53 read:");
            }

            rt_kprintf("pts=%08x,fn=%d,addr=0x%08x,bcnt=%d,bsize=%d", n->pts, n->fn, n->addr, n->bcnt, n->bsize);

            for (i = 0; i < clen; i++)
            {
                if (i % 32 == 0)
                {
                    rt_kprintf("\n");
                }

                rt_kprintf("%02x ", n->data[i]);
            }
        }
        else /*CMD52*/
        {
            if (n->b_write)
            {
                rt_kprintf("\n++CMD 52 write:");
            }
            else
            {
                rt_kprintf("\n++CMD 52 read:");
            }

            rt_kprintf("pts=%08x,fn=%d,addr=0x%08x,data=%02x", n->pts, n->fn, n->addr, n->data[0]);
        }

        rt_kprintf("\n        ");
        for (i = 0; i < SDIO_CALL_STACK_SIZE; i++)
        {
            rt_kprintf(" <-%08x", n->callfn[i]);
        }

        rt_kprintf("\n");

        pos += (sizeof(struct trace_node) + nlen);
    }
}

#endif /*SDIO_TRACE_ENABLE*/
