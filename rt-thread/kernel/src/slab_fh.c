#include <rthw.h>
#include <rtthread.h>
#include <platform_def.h>
#define RT_MEM_STATS

#define MM_PAGE_BITS    (12) /*可以直接修改为13,表示8KB页面*/
#define MM_PAGE_SIZE    (1<<MM_PAGE_BITS)
#define MM_PAGE_MASK    (MM_PAGE_SIZE - 1)
#define MM_ALIGN_4      (2) /*1<<2等于4,即4对齐*/

#define ZONE_RELEASE_THRESH (8) /*可以修改为0,这样就没有cache page*/
#define NZONES              (sizeof(g_zone_sizes) / sizeof(g_zone_sizes[0]))
static rt_uint32_t g_zone_sizes[] = {
#if MM_PAGE_SIZE == 4096
    64,   /* 64   * 64 = 4096 */ /*最小64，因为结构体page_man中的trunk_bitmap最多表示64个bit!!!!!!*/
    96,   /* 96   * 42 = 4032 */
#endif
    128,  /* 128  * 32 = 4096 */
    192,  /* 192  * 21 = 4032 */
    256,  /* 256  * 16 = 4096 */
    292,  /* 292  * 14 = 4088 */
    340,  /* 340  * 12 = 4080 */
    408,  /* 408  * 10 = 4080 */
    512,  /* 512  * 8  = 4096 */
    584,  /* 584  * 7  = 4088 */
    680,  /* 680  * 6  = 4080 */
    816,  /* 816  * 5  = 4080 */
    1024, /* 1024 * 4  = 4096 */
    1364, /* 1364 * 3  = 4092 */
#if MM_PAGE_SIZE == 8192
    1636,
    2048,
    2728,
#endif
    MM_PAGE_SIZE / 2, /* 2048 * 2  = 4096 */ /*必须保证一个page至少2个trunk!!!!!!!!!!!!*/
};

typedef rt_uint32_t rt_addr_t;

#define END_FLAG 0xffff
struct page_allocator
{
    rt_uint16_t page; /*number of free pages*/
    rt_uint16_t next; /*index of next free,END_FLAG means end*/
};

#define PAGE_TYPE_FREE  (0x22)
#define PAGE_TYPE_PAGE  (0x33)
#define PAGE_TYPE_ZONE  (0x44)
#define PAGE_TYPE_CACHE (0x55)
struct page_man
{
    rt_uint32_t page_type:7;    /*for little endian, bit0-6   of uint32*/
    rt_uint32_t zone_index:5;   /*for little endian, bit7-11  of uint32*/
    rt_uint32_t page_prev:20;   /*for little endian, bit20-31 of uint32*/

    rt_uint32_t free_trunks:12; /*for little endian, bit0-6   of uint32*/
    rt_uint32_t page_next:20;   /*for little endian, bit12-31 of uint32*/

    rt_uint64_t trunk_bitmap;
};

struct mem_pool
{
    rt_uint32_t            id;
    rt_uint32_t            inited;

    rt_uint32_t            page_num;
    rt_addr_t              heap_start;
    rt_addr_t              heap_end; /*not include itself*/

    struct page_allocator *page_alloctor;
    rt_uint16_t            alloctor_list;

    struct page_man       *pgman_array;
    int                    zone_alloc_list[NZONES];
    int                    zone_cache_list;
    int                    zone_cache_count;

#ifdef RT_USING_MM_TRACE
    rt_list_t              trace_list;
    int                    trace_count;
    int                    trace_count_max;
#endif

#ifdef RT_MEM_STATS
    int                    used_mem;
    int                    max_mem;
    int                    block_used_bytes;
    int                    block_used_bytes_max;
    int                    zone_num[NZONES];
    int                    zone_num_max[NZONES];
    int                    zone_used_bytes[NZONES];
    int                    zone_used_bytes_max[NZONES];
#endif
};

#include "slab_config.h"

#ifdef RT_USING_MM_TRACE
/***************************************************************************
 * mm_trace_head + PAD(optional) + alloc_len + mm_trace_tail + PAD(optional)
 ***************************************************************************/
#define MM_TRACE_MAGIC1        (0x339DBFAC)
#define MM_TRACE_MAGIC2        (0x339DBAFC)
#define MM_TRACE_BARRIER_LEN   (2)
#define MM_TRACE_BARRIER_CHAR  (0xFDFDFDFD)
#define MM_TRACE_PAD_CHAR      (0xABABABAB)
#define MM_TRACE_FN_NUM        (4)
struct mm_trace_head
{
    rt_uint32_t magic1;
    rt_uint32_t magic2;
    rt_uint32_t trace_index; /*don't change, it must be the last member!!!*/
};

struct mm_trace_tail
{
    rt_uint32_t barrier[MM_TRACE_BARRIER_LEN];
    rt_uint32_t jiffies;
    rt_uint32_t caller_fn_addr[MM_TRACE_FN_NUM];
    rt_uint8_t  thname[RT_NAME_MAX];
};

/*#define GET_TRACE_INDEX(trace) ((trace)->trace_index)*/
#define GET_TRACE_INDEX(trace) (((rt_uint32_t)((void *)(trace) - (void *)g_mm_trace_array))/sizeof(struct mm_trace_node))
struct mm_trace_node
{
    rt_list_t list; /*don't change, must be the first member!!!*/
    struct mm_trace_head *phdr;
    rt_uint32_t alloc_len:28;
    rt_uint32_t alloc_type:4;
    rt_uint16_t headsz;
    rt_uint16_t trailing_len;
};

static char *g_last_print_addr;

static struct mm_trace_node g_mm_trace_array[MM_TRACE_NODE_MAX];
static rt_list_t  g_mm_trace_free_list;
#endif

#define addr_2_pgidx(pool, addr)  (((rt_addr_t)(addr) - pool->heap_start) >> MM_PAGE_BITS)
#define pgidx_2_addr(pool, pgidx) ((void *)(((rt_addr_t)(pool->heap_start + ((pgidx) << MM_PAGE_BITS)))))
#define addr_2_man(pool, addr) &pool->pgman_array[(((rt_addr_t)(addr) - pool->heap_start) >> MM_PAGE_BITS)]

#define BUG(val) do{ \
    rt_kprintf("#FHMM:failed@%08x,line %d!\n", (rt_uint32_t)(val), __LINE__); \
    *((rt_uint32_t *)0) = 0; \
}while (0)

#define BUG2(val, val2) do{ \
    rt_kprintf("#FHMM:failed@%08x-%08x,line %d!\n", (rt_uint32_t)(val), (rt_uint32_t)(val2), __LINE__); \
    *((rt_uint32_t *)0) = 0; \
}while (0)

#define BUG3(val, val2, val3) do{ \
    rt_kprintf("#FHMM:failed@%08x-%08x-%08x,line %d!\n", (rt_uint32_t)(val), (rt_uint32_t)(val2), (rt_uint32_t)(val3), __LINE__); \
    *((rt_uint32_t *)0) = 0; \
}while (0)

#define MM_POOL_NUM (sizeof(g_mem_pool_config)/sizeof(g_mem_pool_config[0]))
static struct mem_pool g_mem_pool[MM_POOL_NUM];
static rt_addr_t       g_heap_start;
static rt_addr_t       g_heap_end;

void list_mem(void);
extern struct rt_thread *rt_current_thread;

void call_stack(rt_uint32_t *fp, rt_uint32_t *callerfn, int num)
{
    extern unsigned char __bss_start;
    int i;

    for (i = 0; i < num; i++)
    {
        /*FIXME, perhaps it will cause cache coherent problem with device driver...*/
        if (((rt_addr_t)fp & (sizeof(rt_addr_t) - 1)) != 0 ||
                (void *)fp < (void *)&__bss_start ||
                (rt_addr_t)fp >= FH_RTT_OS_MEM_END)
            break;

        if (callerfn)
        {
            callerfn[i] = *fp - 0x0c;
        }
        else
        {
            rt_kprintf("<--%08x", *fp - 0x0c);
        }
        fp = (rt_uint32_t *)(*(fp - 3));
    }

    if (callerfn)
    {
        for (; i < num; i++)
        {
            callerfn[i] = 0;
        }
    }
}

void print_call_stack(char *prefix, int num)
{
    rt_uint32_t *fpreg;
    asm ("mov %0, fp":"=r" (fpreg));

    if (prefix)
        rt_kprintf("%s:\n", prefix);
    call_stack(fpreg, RT_NULL, num);
    rt_kprintf("\n");
}

void fhmm_memset(rt_uint32_t *data, rt_uint32_t word, rt_uint32_t len/*in bytes*/)
{
    len >>= 2;

    while (len >= 8)
    {
        data[0] = word;
        data[1] = word;
        data[2] = word;
        data[3] = word;
        data[4] = word;
        data[5] = word;
        data[6] = word;
        data[7] = word;
        data     += 8;
        len -= 8;
    }

    while (len-- > 0)
        *(data++) = word;
}

static void fhmm_disp_mem(rt_uint32_t *data, int len)
{
    int i;
    rt_uint8_t ch;
    while (len > 0)
    {
        rt_kprintf("%08x: %08x %08x %08x %08x ", data, data[0], data[1], data[2], data[3]);
        for (i=0; i<16; i++)
        {
            ch = *((rt_uint8_t *)data + i);
            if (ch >= 0x20 && ch <= 0x7e)
            {
                rt_kprintf("%c", ch);
            }
            else
            {
                rt_kprintf(".");
            }
        }
        rt_kprintf("\n");
        len -= 16;
        data += 4;
    }
}

static rt_uint32_t zoneindex(rt_uint32_t *bytes, rt_uint32_t align)
{
    rt_uint32_t zi;
    rt_uint32_t zonesize;
    rt_uint32_t n = *bytes;

    for (zi = 0; zi < NZONES; zi++)
    {
        zonesize = g_zone_sizes[zi];
        if (n <= zonesize && (zonesize & (align - 1)) == 0)
        {
            *bytes = zonesize;
            break;
        }
    }

    return zi;
}

#ifdef RT_USING_MM_TRACE
#ifdef RT_USING_MEM_FOOT
#define FOOTPRINT_CALLER_NUM (4)
#define FOOTPRINT_MAX        ((MEM_FOOT_MM_SIZE)/sizeof(struct mm_footprint_node))
struct mm_footprint_node
{
    void       *addr;
    rt_uint32_t size;
    rt_uint32_t jiffies;
    rt_uint32_t caller_fn[FOOTPRINT_CALLER_NUM];
    rt_uint8_t  thname[RT_NAME_MAX];
};
static int    g_footprint_pos;
static struct mm_footprint_node g_footprint_array[FOOTPRINT_MAX];
static void footprint_add(rt_uint32_t *fp, void *addr, rt_uint32_t size)
{
    int pos = g_footprint_pos;
    struct mm_footprint_node *node = &g_footprint_array[pos];

    node->addr = addr;
    node->size = size;
    node->jiffies = rt_tick_get();
    call_stack(fp, node->caller_fn, FOOTPRINT_CALLER_NUM);
    node->thname[0] = 0;
    if (rt_current_thread)
        rt_memcpy(node->thname, rt_current_thread->name, RT_NAME_MAX);
    node->thname[RT_NAME_MAX - 1] = 0;

    if (++pos >= FOOTPRINT_MAX)
        pos = 0;
    g_footprint_pos = pos;
}

static void footprint_dump(void *addr, rt_uint32_t size)
{
    int i, j;
    int pos;
    int nodesz;
    struct mm_footprint_node *node;

    pos = g_footprint_pos - 1;
    for (i=0; i<FOOTPRINT_MAX; i++)
    {
        if  (pos < 0)
            pos = FOOTPRINT_MAX - 1;
        node = &g_footprint_array[pos];
        nodesz = node->size & (~1);
        if ((node->addr <= addr && node->addr + nodesz > addr) ||
                (node->addr >= addr && addr + size > node->addr))
        {
            if ((node->size & 1) == 0)
            {
                rt_kprintf("foot-malloc,");
            }
            else
            {
                rt_kprintf("foot-free,");
            }
            rt_kprintf("addr:%p,size:%08x,tick=%08x,%s ",
                    node->addr, nodesz, node->jiffies, node->thname);
            for (j=0; j<FOOTPRINT_CALLER_NUM; j++)
            {
                rt_kprintf("<-%08x", node->caller_fn[j]);
            }
            rt_kprintf("\n");
        }
        pos--;
    }
}
#endif

static void disp_trace_node(struct mm_trace_node *trace)
{
    rt_uint32_t ptr_offset;
    rt_uint32_t tail_offset;
    rt_uint32_t trailing_offset;
    rt_uint32_t trunksz;
    char *ss;
    char *se;
    struct mm_trace_head *phdr = trace->phdr;

    ptr_offset  = trace->headsz;
    tail_offset = ptr_offset + trace->alloc_len;
    trailing_offset = tail_offset + sizeof(struct mm_trace_tail);
    trunksz = trailing_offset + trace->trailing_len;

    rt_kprintf("#FHMM:bad memory node %p-%p-%p-%p-%p-%d\n", phdr,
            (void *)phdr + ptr_offset,
            (void *)phdr + tail_offset,
            (void *)phdr + trailing_offset,
            (void *)phdr + trunksz,
            trace->alloc_type);

    fhmm_disp_mem((rt_uint32_t *)trace, sizeof(*trace));

    ss = (char *)phdr - 16*1024;
    se = (char *)phdr + trunksz + 16*1024;

    if (ss < (char *)g_heap_start)
        ss = (char *)g_heap_start;
    if (se > (char *)g_heap_end)
        se = (char *)g_heap_end;

    if (g_last_print_addr)
    {
        if (ss < g_last_print_addr)
            ss = g_last_print_addr;
        if (se < g_last_print_addr)
            se = g_last_print_addr;
        g_last_print_addr = se;
    }

    fhmm_disp_mem((rt_uint32_t *)ss, se - ss);
}

static int check_trace_node(struct mm_trace_node *trace)
{
    int i;
    rt_uint32_t  trace_index;
    void        *ptr;
    struct mm_trace_head *phdr;
    struct mm_trace_tail *tail;

    trace_index = GET_TRACE_INDEX(trace);
    phdr        = trace->phdr;
    ptr         = (void *)phdr + trace->headsz;
    if (phdr->magic1 != MM_TRACE_MAGIC1 ||
            phdr->magic2 != MM_TRACE_MAGIC2 ||
            phdr->trace_index != trace_index ||
            *((rt_uint32_t *)ptr - 1) != trace_index)
    {
        goto Error;
    }

    tail = (struct mm_trace_tail *)(ptr + trace->alloc_len);
    for (i = 0; i < MM_TRACE_BARRIER_LEN; i++)
    {
        if (tail->barrier[i] != MM_TRACE_BARRIER_CHAR)
        {
            goto Error;
        }
    }

    if (1)
    {
        rt_uint32_t *pad = (rt_uint32_t *)(phdr + 1);
        rt_uint32_t *end = (rt_uint32_t *)((void *)ptr - 4);
        while (pad < end)
        {
            if (*(pad++) != MM_TRACE_PAD_CHAR)
            {
                goto Error;
            }
        }
    }

    if (1)
    {
        rt_uint32_t *pad = (rt_uint32_t *)(tail + 1);
        rt_uint32_t len = trace->trailing_len >> 2;
        while (len-- > 0)
        {
            if (*(pad++) != MM_TRACE_PAD_CHAR)
            {
                goto Error;
            }
        }
    }

    return 0;

Error:
    return 1;
}

static int check_pool(struct mem_pool *pool, int abort)
{
    int ret = 0;
    rt_base_t level;
    rt_list_t *node;
    rt_list_t *head;

    if (!pool->inited)
        return 0;

    level = rt_hw_interrupt_disable();
    head = &pool->trace_list;
    for (node = head->next; node != head; node = node->next)
    {
        if (check_trace_node((struct mm_trace_node *)node) != 0)
        {
            if (abort)
            {
                BUG(node);
            }
            ret = 1;
            break;
        }
    }
    rt_hw_interrupt_enable(level);
    return ret;
}

static int scan_pool(struct mem_pool *pool)
{
    int       count = 0;
    int       badcnt = 0;
    rt_base_t level;
    rt_list_t *node;
    rt_list_t *head;
    struct mm_trace_node *currtrace;
    struct mm_trace_head *prevphdr;
    struct mm_trace_head *phdr;

    if (!pool->inited)
        return 0;

    head = &pool->trace_list;
    prevphdr = RT_NULL;

    rt_kprintf("#begin scan pool%d...\n", pool->id);

    level = rt_hw_interrupt_disable();
    g_last_print_addr = (char *)g_heap_start;
    while (1)
    {
        /*找一个地址最小的节点*/
        currtrace = RT_NULL;
        for (node = head->next; node != head; node = node->next)
        {
            phdr = ((struct mm_trace_node *)node)->phdr;
            if (phdr > prevphdr && (!currtrace || phdr < currtrace->phdr))
            {
                currtrace = (struct mm_trace_node *)node;
            }
        }

        if (!currtrace)
            break;

        count++;
        if (check_trace_node(currtrace) != 0)
        {
            badcnt++;
            disp_trace_node(currtrace);
#ifdef RT_USING_MEM_FOOT
            footprint_dump(currtrace->phdr, currtrace->headsz +
                    currtrace->alloc_len + sizeof(struct mm_trace_tail) + currtrace->trailing_len);
#endif
        }

        prevphdr = currtrace->phdr;
    }
    g_last_print_addr = RT_NULL;
    rt_hw_interrupt_enable(level);

    rt_kprintf("#end scan pool,total %d,bad %d\n", count, badcnt);
    return badcnt;
}

static void dump_pool(struct mem_pool *pool)
{
    int i;
    int seq = 0;
    rt_base_t level;
    rt_list_t   *node;
    rt_list_t   *head;
    struct mm_trace_node *trace;
    struct mm_trace_head *phdr;
    struct mm_trace_tail *tail;

    if (!pool->inited)
        return;

    rt_kprintf("\n#dump for pool %d\n", pool->id);

    level = rt_hw_interrupt_disable();

    head = &pool->trace_list;
    for (node = head->next; node != head; node = node->next)
    {
        trace = (struct mm_trace_node *)node;
        phdr = trace->phdr;
        tail = (struct mm_trace_tail *)((void *)phdr + trace->headsz + trace->alloc_len);

        if (check_trace_node(trace) != 0)
        {
            rt_kprintf("#FHMM:bad memory node %p\n", phdr);
        }
        rt_kprintf("node-%08x(%p): pts=%08x,magic1=%08x,magic2=%08x,caller_fn=%08x",
                seq, phdr, tail->jiffies, phdr->magic1, phdr->magic2, tail->caller_fn_addr[0]);
        for (i=1; i<MM_TRACE_FN_NUM; i++)
        {
            rt_kprintf("<-%08x", tail->caller_fn_addr[i]);
        }
        rt_kprintf(",alloc_len=%d,type=%d,%s\n", trace->alloc_len, trace->alloc_type, tail->thname);
        seq++;
    }

    rt_hw_interrupt_enable(level);
}

#endif /*RT_USING_MM_TRACE*/

static void *rt_page_alloc_nolock(struct mem_pool *pool, rt_uint32_t npages)
{
    rt_uint16_t *prev;
    rt_uint32_t  prevend = -1;
    rt_uint32_t  index;
    rt_uint32_t  nextindex;
    rt_uint32_t  page_num = pool->page_num;
    struct page_allocator *b;
    struct page_allocator *n;

    if (npages == 0)
        return RT_NULL;

    for (prev = &pool->alloctor_list; (index = *prev) < page_num; prev = &(b->next))
    {
        b = &pool->page_alloctor[index];
        if ((int)index <= (int)prevend || b->page == 0)
        {
            BUG(prev);
        }

        if (b->page < npages)
        {
            prevend = index + b->page;
            continue;
        }

        if (b->page > page_num - index)
        {
            BUG(b);
        }

        nextindex = b->next;
        if (nextindex < page_num)
        {
            if (index + b->page >= nextindex)
            {
                BUG(b);
            }
        }
        else if (nextindex != END_FLAG)
        {
            BUG(b);
        }

        if (b->page > npages) /* splite pages */
        {
            n       = b + npages;
            n->next = nextindex;
            n->page = b->page - npages;
            *prev   = index + npages;
        }
        else /* this node fit, remove this node */
        {
            *prev = nextindex;
        }
        goto done;
    }

    if (index != END_FLAG)
    {
        BUG(index);
    }
    return RT_NULL;

done:
    return (void *)pgidx_2_addr(pool, index);
}

/*
 * caller必须保证addr和npages两个参数的正确性
 */
static void rt_page_free_nolock(struct mem_pool *pool, void *addr, rt_uint32_t npages)
{
    rt_uint32_t  bend = -1;
    rt_uint16_t *prev;
    rt_uint32_t  index;
    rt_uint32_t  freindex;
    rt_uint32_t  nextindex;
    rt_uint32_t  freend;
    rt_uint32_t  page_num = pool->page_num;
    struct page_allocator *b;
    struct page_allocator *next;
    struct page_allocator *fre;

    freindex = addr_2_pgidx(pool, addr);
    if (((rt_addr_t)addr & MM_PAGE_MASK) != 0 ||
            addr < (void *)pool->heap_start ||
            addr >= (void *)pool->heap_end ||
            npages > page_num - freindex)
    {
        BUG(addr);
    }

    freend = freindex + npages;
    fre    = &pool->page_alloctor[freindex];
    for (prev = &pool->alloctor_list; (index = *prev) < page_num; prev = &(b->next))
    {
        b = &pool->page_alloctor[index];
        if ((int)index <= (int)bend || b->page == 0)
        {
            BUG(prev);
        }

        bend = index + b->page;
        if (bend < freindex)
        {
            continue;
        }

        if (bend > page_num)
        {
            BUG(b);
        }

        if (freindex == bend)
        {
            /*santanic checks...*/
            nextindex = b->next;
            next = &pool->page_alloctor[nextindex];
            if (nextindex < page_num)
            {
                if (next->page == 0 || next->page > page_num - nextindex || freend > nextindex)
                {
                    rt_kprintf("fre=%d,pages=%d,next=%p\n", freindex, npages, next);
                    BUG(b);
                }
            }
            else if (nextindex != END_FLAG)
            {
                BUG(b);
            }

            b->page += npages;
            if (freend == nextindex)
            {
                b->page += next->page;
                b->next  = next->next;
            }
            goto _return;
        }

        if (freend < index)
            break;

        if (freend == index)
        {
            fre->page = b->page + npages;
            fre->next = b->next;
            *prev = freindex;
            goto _return;
        }

        BUG(b);
    }

    if (index >= page_num && index != END_FLAG)
    {
        BUG(index);
    }

    fre->page = npages;
    fre->next = index;
    *prev = freindex;

_return:
    return;
}

static void pool_init(struct mem_pool *pool, void *begin_addr, void *end_addr)
{
    int         line;
    rt_uint32_t i;
    rt_uint32_t npages;
    rt_uint32_t allocator_size;
    rt_uint32_t man_size;
    rt_uint32_t manage_size;
    rt_addr_t   heap_start;
    rt_addr_t   heap_end; /* not include itself */
    struct page_allocator *allocator;
    struct page_man *pgm;

    if (pool->inited)
        return;

    /* align begin and end addr to page */
    heap_start = RT_ALIGN((rt_addr_t)begin_addr, MM_PAGE_SIZE);
    heap_end   = RT_ALIGN_DOWN((rt_addr_t)end_addr, MM_PAGE_SIZE);
    if (heap_start >= heap_end || heap_start < (rt_addr_t)begin_addr)
    {
        line = __LINE__;
        goto Error;
    }

    npages = (heap_end - heap_start) >> MM_PAGE_BITS;
    allocator_size = npages * sizeof(struct page_allocator);
    man_size       = npages * sizeof(struct page_man);
    manage_size    = RT_ALIGN(allocator_size + man_size, MM_PAGE_SIZE);
    if (manage_size + 4 * MM_PAGE_SIZE > heap_end - heap_start)
    {
        line = __LINE__;
        goto Error;
    }

#ifdef RT_USING_MM_TRACE
    fhmm_memset((rt_uint32_t *)heap_start, MM_TRACE_PAD_CHAR, MM_PAGE_SIZE);
    heap_start += MM_PAGE_SIZE; /*reserved one page for barrier....*/
#endif
    allocator   = (struct page_allocator *)heap_start;
    pgm         = (struct page_man *)(heap_start + allocator_size);
    heap_start += manage_size;
    npages      = (heap_end - heap_start) >> MM_PAGE_BITS;

    pool->page_num   = npages;
    pool->heap_start = heap_start;
    pool->heap_end   = heap_end;
    pool->page_alloctor = allocator;
    pool->alloctor_list = END_FLAG;
    pool->pgman_array   = pgm;
    for (i=0; i<NZONES; i++)
    {
        pool->zone_alloc_list[i] = -1;
    }
    pool->zone_cache_list = -1;
    pool->zone_cache_count = 0;

#ifdef RT_USING_MM_TRACE
    rt_list_init(&pool->trace_list);
    pool->trace_count = 0;
    pool->trace_count_max = 0;
#endif

    for (i = 0; i < npages; i++)
    {
        pgm->page_type = PAGE_TYPE_FREE;
        pgm++;
    }
#ifdef RT_USING_MM_TRACE
    fhmm_memset((rt_uint32_t *)allocator, 0, allocator_size);
    fhmm_memset((rt_uint32_t *)pgm, MM_TRACE_PAD_CHAR, heap_start - (rt_addr_t)pgm);
#endif

    /* init pages allocator */
    rt_page_free_nolock(pool, (void *)heap_start, npages);

    pool->inited = 1;
    return;
Error:
    BUG(line);
}

void rt_system_heap_init(void *begin_addr, void *end_addr/*not include itself*/)
{
    int         line;
    int         i;
    rt_uint32_t sz;
    rt_uint32_t poolid;
    rt_uint32_t totalsz;
    rt_uint32_t zonesize;
    rt_uint32_t presz = 0;
    rt_addr_t   heap_start;
    rt_addr_t   heap_end;
    struct page_man *pgm;

    heap_start = RT_ALIGN((rt_addr_t)begin_addr, MM_PAGE_SIZE);
    heap_end   = RT_ALIGN_DOWN((rt_addr_t)end_addr, MM_PAGE_SIZE);
    if (heap_start >= heap_end || heap_start < (rt_addr_t)begin_addr)
    {
        line = __LINE__;
        goto Error;
    }
    totalsz = heap_end - heap_start;
    g_heap_start = heap_start;
    g_heap_end   = heap_end;

    if (sizeof(g_mem_pool_config) < sizeof(rt_uint32_t))
    {
        line = __LINE__;
        goto Error;
    }

#ifdef RT_USING_MM_TRACE
#ifdef RT_USING_MEM_FOOT
    if (sizeof(g_mem_pool_config) != sizeof(g_mem_pool_foot))
    {
        line = __LINE__;
        goto Error;
    }
#endif
    if ((sizeof(struct mm_trace_head) & 3) != 0 ||
            (sizeof(struct mm_trace_tail) & 3) != 0 ||
            (sizeof(struct mm_trace_node) & 3) != 0) /*长度4对齐*/
    {
        line = __LINE__;
        goto Error;
    }

    if (1) /*初始化空闲trace list*/
    {
        struct mm_trace_node *trace = &g_mm_trace_array[0];
        rt_list_init(&g_mm_trace_free_list);
        for (i=0; i<MM_TRACE_NODE_MAX; i++)
        {
            trace->phdr = RT_NULL;
            trace->headsz = 0;
            rt_list_insert_before(&g_mm_trace_free_list, &trace->list);
            trace++;
        }
    }
#endif

    /* 检查zone_size正确性 */
    for (i = 0; i < NZONES; i++)
    {
        zonesize = g_zone_sizes[i];
        if (zonesize <= presz || (zonesize & (sizeof(rt_addr_t) - 1)) != 0)
        {
            line = __LINE__;
            goto Error;
        }
        presz = zonesize;
    }
    if (NZONES <= 0 ||
            zonesize > MM_PAGE_SIZE / 2 ||
            g_zone_sizes[0] * (sizeof(pgm->trunk_bitmap) * 8) < MM_PAGE_SIZE)
    {
        line = __LINE__;
        goto Error;
    }

    /*pool 0使用剩余内存,计算其大小*/
    sz = 0;
    for (i=1; i<MM_POOL_NUM; i++)
    {
        sz += RT_ALIGN(g_mem_pool_config[i], MM_PAGE_SIZE);
    }
    if (sz >= totalsz)
    {
        line = __LINE__;
        goto Error;
    }

    /*初始化各个pool*/
    g_mem_pool_config[0] = totalsz - sz;
    for (i=0; i<MM_POOL_NUM; i++)
    {
        g_mem_pool[i].id = i;
        sz = RT_ALIGN(g_mem_pool_config[i], MM_PAGE_SIZE);
        if (sz != 0)
        {
            pool_init(&g_mem_pool[i], (void *)heap_start, (void *)heap_start + sz);
            heap_start += sz;
        }
    }

    /*检查每种type所用pool的正确性*/
    for (i=0; i<FHMEM_TYPE_MAX; i++)
    {
        poolid = g_mem_pool_map[i];
        if (poolid >= MM_POOL_NUM || !g_mem_pool_config[poolid])
        {
            line = __LINE__;
            goto Error;
        }
    }

    return;
Error:
    BUG(line);
}

static int verify_pglist(struct mem_pool *pool, rt_uint32_t pgidx, rt_uint32_t type, rt_uint32_t zi)
{
    rt_uint32_t      prev;
    rt_uint32_t      next;
    rt_uint32_t      page_num = pool->page_num;
    struct page_man *pgman_array = pool->pgman_array;
    struct page_man *pgm;

    if (pgidx >= page_num)
    {
        BUG(pgidx);
    }

    pgm    = &pgman_array[pgidx];
    prev = pgm->page_prev;
    next = pgm->page_next;
    if (prev >= page_num ||
            next >= page_num ||
            pgman_array[prev].page_next != pgidx ||
            pgman_array[next].page_prev != pgidx ||
            pgm->page_type != type ||
            pgm->zone_index != zi)
    {
        BUG(pgidx);
    }

    return 0;
}

static void *alloc_trunk(struct mem_pool *pool, rt_uint32_t zi)
{
    int pgidx;
    rt_uint32_t prev;
    rt_uint32_t next;
    rt_uint8_t *bitmap;
    struct page_man *pgm;
    void *trunk = RT_NULL;
    struct page_man *pgman_array = pool->pgman_array;
    rt_uint32_t trunks_per_page = MM_PAGE_SIZE / g_zone_sizes[zi];

    /* take one trunk from alloc list */
    pgidx = pool->zone_alloc_list[zi];
    if (pgidx >= 0)
    {
        rt_uint32_t x = 0;
        rt_uint32_t y = 0;
        rt_uint32_t free_trunks;
        rt_uint32_t trunk_index;
        rt_uint8_t  mask;

        verify_pglist(pool, (rt_uint32_t)pgidx, PAGE_TYPE_ZONE, zi);
        pgm           = &pgman_array[pgidx];
        prev        = pgm->page_prev;
        next        = pgm->page_next;
        free_trunks = pgm->free_trunks;
        bitmap      = (rt_uint8_t *)(&pgm->trunk_bitmap);

        /* find one trunk which is not used */
        for (; x < sizeof(pgm->trunk_bitmap); x++)
        {
            mask = bitmap[x];
            if (mask != 0xff)
            {
                for (; y < 8; y++)
                {
                    if (!(mask & (1<<y)))
                        break;
                }
                break;
            }
        }
        trunk_index = (x << 3) + y;
        if (trunk_index >= trunks_per_page || free_trunks == 0 || free_trunks >= trunks_per_page)
        {
            BUG(pgidx);
        }

        /* remove the page from alloc list if no available trunk */
        if (--free_trunks == 0)
        {
            pgm->page_prev = pgidx;
            pgm->page_next = pgidx;

            if (pgidx == next)
            {
                pool->zone_alloc_list[zi] = -1;
            }
            else
            {
                pgman_array[prev].page_next = next;
                pgman_array[next].page_prev = prev;
                pool->zone_alloc_list[zi] = next;
            }
        }

        /* update page info*/
        bitmap[x] |= (1<<y);
        pgm->free_trunks = free_trunks;

        trunk = pgidx_2_addr(pool, pgidx) + trunk_index * g_zone_sizes[zi];
        goto done;
    }

    /* take one trunk from cache list or alloc one new page */
    pgidx = pool->zone_cache_list;
    if (pgidx >= 0)
    {
        /* take from cache list */
        verify_pglist(pool, (rt_uint32_t)pgidx, PAGE_TYPE_CACHE, 0); /* in cache list, zi fixed to zero */
        pgm = &pgman_array[pgidx];
        prev  = pgm->page_prev;
        next  = pgm->page_next;
        trunk = pgidx_2_addr(pool, pgidx);

        /* remove this page from cache list */
        if (pgidx == next)
        {
            pool->zone_cache_list = -1;
        }
        else
        {
            pgman_array[prev].page_next = next;
            pgman_array[next].page_prev = prev;
            pool->zone_cache_list = next;
        }
        pool->zone_cache_count--;
    }
    else
    {
        /* alloc one new page */
        pool->zone_cache_count = 0;
        trunk = rt_page_alloc_nolock(pool, 1);
        if (!trunk)
            goto Exit;

        pgidx = addr_2_pgidx(pool, trunk);
        pgm = &pgman_array[pgidx];
        if (pgm->page_type != PAGE_TYPE_FREE)
        {
            BUG(pgidx);
        }
    }

    /* link into alloc list */
    pgm->page_type    = PAGE_TYPE_ZONE;
    pgm->zone_index   = zi;
    pgm->free_trunks  = trunks_per_page - 1;
    pgm->trunk_bitmap = 0;
    *((rt_uint8_t *)&pgm->trunk_bitmap) = 1;
    pgm->page_prev    = pgidx;
    pgm->page_next    = pgidx;
    pool->zone_alloc_list[zi] = pgidx;

#ifdef RT_MEM_STATS
    if (++(pool->zone_num[zi]) > pool->zone_num_max[zi])
        pool->zone_num_max[zi] = pool->zone_num[zi];
#endif

done:
#ifdef RT_MEM_STATS
    pool->zone_used_bytes[zi] += g_zone_sizes[zi];
    if (pool->zone_used_bytes[zi] > pool->zone_used_bytes_max[zi])
        pool->zone_used_bytes_max[zi] = pool->zone_used_bytes[zi];
    pool->used_mem += g_zone_sizes[zi];
    if (pool->used_mem > pool->max_mem)
        pool->max_mem = pool->used_mem;
#endif
Exit:
    return trunk;
}

static void *malloc_r(struct mem_pool *pool,
#ifdef RT_USING_MM_TRACE
        rt_uint32_t *ptrunksz,
#endif
        rt_uint32_t size,
        rt_uint32_t align_bits)
{
    rt_uint32_t  zi;
    rt_uint32_t  align = 1 << align_bits;
    struct page_man *pgm;
    void *trunk = RT_NULL;

    /*
     * Handle large allocations directly.  There should not be very many of
     * these so performance is not a big issue.
     */
    if (size > g_zone_sizes[NZONES-1] || align_bits == MM_PAGE_BITS)
    {
        size = RT_ALIGN(size, MM_PAGE_SIZE);
        trunk = rt_page_alloc_nolock(pool, size >> MM_PAGE_BITS);
        if (!trunk)
            goto fail;

        pgm = addr_2_man(pool, trunk);
        if (pgm->page_type != PAGE_TYPE_FREE)
        {
            BUG(pgm);
        }
        pgm->page_type = PAGE_TYPE_PAGE;
        pgm->page_prev = size >> MM_PAGE_BITS;

#ifdef RT_MEM_STATS
        pool->block_used_bytes += size;
        if (pool->block_used_bytes > pool->block_used_bytes_max)
            pool->block_used_bytes_max = pool->block_used_bytes;
        pool->used_mem += size;
        if (pool->used_mem > pool->max_mem)
            pool->max_mem = pool->used_mem;
#endif
        goto done;
    }

    zi = zoneindex(&size, align);
    if (zi >= NZONES)
    {
        rt_kprintf("#FHMM:not support align%d\n", align);
        return RT_NULL;
    }

    trunk = alloc_trunk(pool, zi);
    if (!trunk)
        goto fail;

done:
#ifdef RT_USING_MM_TRACE
    *ptrunksz = size;
#endif
    return trunk;

fail:
    list_mem();
    rt_kprintf("#FHMM:out of memory,size=%d,align=%d!\n", size, align);
    return RT_NULL;
}

static void free_r(struct mem_pool *pool, void *ptr)
{
    rt_uint32_t pgidx;
    void       *freeptr;
    struct page_man *pgm;
    struct page_man *pgman_array = pool->pgman_array;

    pgidx = addr_2_pgidx(pool, ptr);
    pgm = &pgman_array[pgidx];
    freeptr = ptr;

    if (pgm->page_type == PAGE_TYPE_PAGE)
    {
        rt_uint32_t npages = pgm->page_prev;

        if (((rt_addr_t)freeptr & MM_PAGE_MASK) != 0 ||
                npages == 0 ||
                npages > pool->page_num - pgidx)
        {
            BUG(ptr);
        }

        rt_page_free_nolock(pool, freeptr, npages);
        pgm->page_type = PAGE_TYPE_FREE;
#ifdef RT_MEM_STATS
        pool->block_used_bytes -= (npages << MM_PAGE_BITS);
        pool->used_mem -= (npages << MM_PAGE_BITS);
#endif
    }
    else if (pgm->page_type == PAGE_TYPE_ZONE)
    {
        struct page_man *hm;
        int         head;

        rt_uint32_t offset      = (rt_addr_t)freeptr & MM_PAGE_MASK;
        rt_uint8_t *bitmap      = (rt_uint8_t *)&pgm->trunk_bitmap;
        rt_uint32_t zi          = pgm->zone_index;
        rt_uint32_t free_trunks = pgm->free_trunks;
        rt_uint32_t trunkidx;
        rt_uint32_t trunks_per_page;

        if (zi >= NZONES)
        {
            BUG(ptr);
        }

        trunkidx = offset / g_zone_sizes[zi];
        trunks_per_page = MM_PAGE_SIZE / g_zone_sizes[zi];

        if (free_trunks >= trunks_per_page ||
                trunkidx * g_zone_sizes[zi] != offset ||
                !(bitmap[trunkidx>>3] & (1<<(trunkidx&7))))
        {
            BUG(ptr);
        }

        /*put into alloc list*/
        /*之前page内所有trunk都被分配,此前已从alloc list去掉,所以需要重新加入*/
        if (free_trunks == 0)
        {
            if (!(pgm->page_prev == pgidx && pgm->page_next == pgidx))
            {
                BUG(ptr);
            }

            head = pool->zone_alloc_list[zi];
            if (head < 0)
            {
                pool->zone_alloc_list[zi] = pgidx;
            }
            else
            {
                verify_pglist(pool, head, PAGE_TYPE_ZONE, zi);
                hm = &pgman_array[head];
                pgm->page_prev = head;
                pgm->page_next = hm->page_next;
                pgman_array[hm->page_next].page_prev = pgidx;
                hm->page_next = pgidx;
            }
        }
        else if (free_trunks + 1 == trunks_per_page) /*remove from alloc list,and insert into cache list*/
        {
            rt_uint32_t prev = pgm->page_prev;
            rt_uint32_t next = pgm->page_next;

            head = pool->zone_alloc_list[zi];
            if (head < 0 ||
                    prev >= pool->page_num ||
                    next >= pool->page_num ||
                    pgman_array[prev].page_next != pgidx ||
                    pgman_array[next].page_prev != pgidx)
            {
                rt_kprintf("#FHMM:%08x\n", head);
                BUG(ptr);
            }

            /* remove from alloc list */
            if (pgidx == next)
            {
                if (head != pgidx)
                {
                    rt_kprintf("#FHMM:%08x\n", head);
                    BUG(ptr);
                }
                pool->zone_alloc_list[zi] = -1;
            }
            else
            {
                pgman_array[prev].page_next = next;
                pgman_array[next].page_prev = prev;
                if (head == pgidx)
                {
                    pool->zone_alloc_list[zi] = next;
                }
            }
#ifdef RT_MEM_STATS
            pool->zone_num[zi]--;
#endif

            /* insert into cache list */
            if (pool->zone_cache_count < ZONE_RELEASE_THRESH)
            {
                head = pool->zone_cache_list;
                if (head >= 0)
                {
                    verify_pglist(pool, head, PAGE_TYPE_CACHE, 0);
                    hm = &pgman_array[head];
                    pgm->page_prev = hm->page_prev;
                    pgm->page_next = head;
                    pgman_array[hm->page_prev].page_next = pgidx;
                    hm->page_prev = pgidx;
                }
                else
                {
                    pgm->page_prev = pgidx;
                    pgm->page_next = pgidx;
                    pool->zone_cache_list = pgidx;
                }
                pgm->zone_index = 0;
                pgm->page_type = PAGE_TYPE_CACHE;
                pool->zone_cache_count++;
            }
            else
            {
                rt_page_free_nolock(pool, (void *)RT_ALIGN_DOWN((rt_addr_t)freeptr, MM_PAGE_SIZE), 1);
                pgm->page_type = PAGE_TYPE_FREE;
            }
        }

        pgm->free_trunks = free_trunks + 1;
        bitmap[trunkidx>>3] &= ~(1<<(trunkidx&7));
#ifdef RT_MEM_STATS
        pool->zone_used_bytes[zi] -= g_zone_sizes[zi];
        pool->used_mem -= g_zone_sizes[zi];
#endif
    }
    else
    {
        BUG(ptr);
    }
}

#ifdef RT_USING_MM_TRACE
static void *malloc_r2(struct mem_pool *pool, rt_uint32_t size, rt_uint32_t align_bits, rt_uint32_t *fp, rt_uint32_t module_id)
{
    rt_uint32_t  headsz;
    rt_uint32_t  trunksz;
    struct mm_trace_node *trace;
    struct mm_trace_head *phdr;
    struct mm_trace_tail *tail;
    void                 *ptr;

    headsz = RT_ALIGN(sizeof(struct mm_trace_head), (1<<align_bits));

    phdr = (struct mm_trace_head *)malloc_r(pool, &trunksz, headsz + size + sizeof(struct mm_trace_tail), align_bits);
    if (!phdr)
    {
        return RT_NULL;
    }

    /*take one trace from free list*/
    if (g_mm_trace_free_list.next == &g_mm_trace_free_list)
    {
        BUG(0);
    }
    trace = (struct mm_trace_node *)g_mm_trace_free_list.next;
    rt_list_remove(&trace->list);

    fhmm_memset((rt_uint32_t *)phdr, MM_TRACE_PAD_CHAR, trunksz);

    ptr               = (void *)phdr + headsz;
    phdr->magic1      = MM_TRACE_MAGIC1;
    phdr->magic2      = MM_TRACE_MAGIC2;
    *((rt_uint32_t *)ptr - 1) = phdr->trace_index = GET_TRACE_INDEX(trace);

    tail = (struct mm_trace_tail *)(ptr + size);
    fhmm_memset(tail->barrier, MM_TRACE_BARRIER_CHAR, sizeof(tail->barrier));
    tail->jiffies = rt_tick_get();
    call_stack(fp, tail->caller_fn_addr, MM_TRACE_FN_NUM);
    tail->thname[0] = 0;
    if (rt_current_thread)
        rt_memcpy(tail->thname, rt_current_thread->name, RT_NAME_MAX);
    tail->thname[RT_NAME_MAX - 1] = 0;

    /*insert into trace list tail*/
    trace->phdr = phdr;
    trace->alloc_len = size;
    trace->alloc_type = module_id;
    trace->headsz = headsz;
    trace->trailing_len = trunksz - headsz - size - sizeof(*tail);
    rt_list_insert_before(&pool->trace_list, &trace->list);
    pool->trace_count++;
    if (pool->trace_count > pool->trace_count_max)
        pool->trace_count_max = pool->trace_count;

#ifdef RT_USING_MEM_FOOT
    if (g_mem_pool_foot[pool->id])
        footprint_add(fp, phdr, trunksz);
#endif

    return ptr;
}

static struct mm_trace_node *find_trace(struct mem_pool *pool, void *ptr)
{
    rt_list_t   *node;
    rt_list_t   *head;
    struct mm_trace_node *trace;

    head = &pool->trace_list;
    for (node = head->next; node != head; node = node->next)
    {
        trace = (struct mm_trace_node *)node;
        if ((void *)trace->phdr +  trace->headsz == ptr)
        {
            return trace;
        }
    }

    return RT_NULL;
}

static void free_r2(struct mem_pool *pool, void *ptr, rt_uint32_t module_id)
{
    rt_uint32_t trace_index;
    struct mm_trace_node *trace = RT_NULL;
    struct mm_trace_head *phdr;
    rt_uint32_t *fpreg;
    asm ("mov %0, fp":"=r" (fpreg));

    trace_index = *((rt_uint32_t *)ptr - 1);
    if (trace_index < MM_TRACE_NODE_MAX)
    {
        trace = &g_mm_trace_array[trace_index];
        if ((void *)trace->phdr + trace->headsz == ptr)
        {
            goto okay;
        }
    }

    trace = find_trace(pool, ptr);
    if (!trace)
    {
        rt_kprintf("#FHMM:invalid pointer %p!\n", ptr);
        fhmm_disp_mem((rt_uint32_t *)(ptr - 2*MM_PAGE_SIZE), 4*MM_PAGE_SIZE);
#ifdef RT_USING_MEM_FOOT
        footprint_dump(ptr, MM_PAGE_SIZE/2/*don't know the size, just use it*/);
#endif
    }
    BUG(ptr);

okay:
    phdr = trace->phdr;
    if (check_trace_node(trace) != 0)
    {
        BUG2(ptr, trace);
    }

    if (trace->alloc_type != module_id)
    {
        rt_kprintf("#FHMM:duplicate free(%d-%d,%p)!\n", trace->alloc_type, module_id, ptr);
        disp_trace_node(trace);
        BUG(phdr);
    }

#ifdef RT_USING_MEM_FOOT
    if (g_mem_pool_foot[pool->id])
        footprint_add(fpreg, phdr, 1 + trace->headsz + trace->alloc_len + trace->trailing_len + sizeof(struct mm_trace_tail));
#endif

    if (1)
    {
        struct mm_trace_tail *tail;
        tail = (struct mm_trace_tail *)((void *)phdr + trace->headsz + trace->alloc_len);
        tail->jiffies = rt_tick_get();
        call_stack(fpreg, tail->caller_fn_addr, MM_TRACE_FN_NUM);
        tail->thname[0] = 0;
        if (rt_current_thread)
            rt_memcpy(tail->thname, rt_current_thread->name, RT_NAME_MAX);
        tail->thname[RT_NAME_MAX - 1] = 0;
    }

    pool->trace_count--;
    rt_list_remove(&trace->list);
    trace->phdr = RT_NULL;
    trace->headsz = 0;
    rt_list_insert_before(&g_mm_trace_free_list, &trace->list);
    phdr->magic1 = phdr->magic2 = 0;

    free_r(pool, phdr);
}
#endif

/******************************************************************
 **********Begin API implementation********************************
 ******************************************************************
 ******************************************************************/
void *fhkmalloc(rt_uint32_t module_id, rt_uint32_t size, rt_uint32_t align_bits)
{
    rt_base_t level;
    void *ptr;
    struct mem_pool *pool;
#ifdef RT_USING_MM_TRACE
    rt_uint32_t *fpreg;
    asm ("mov %0, fp":"=r" (fpreg));
#endif

    pool = &g_mem_pool[g_mem_pool_map[module_id]];
    if (module_id >= FHMEM_TYPE_MAX || !pool->inited || align_bits > MM_PAGE_BITS)
    {
        BUG2(module_id, align_bits);
    }

    if (size <= 0 || size > (pool->page_num << MM_PAGE_BITS))
        return RT_NULL;

    size = RT_ALIGN(size, 4);
    if (align_bits < MM_ALIGN_4)
        align_bits = MM_ALIGN_4; /*最小4对齐*/

    level = rt_hw_interrupt_disable();
#ifdef RT_USING_MM_TRACE
    ptr = malloc_r2(pool, size, align_bits, fpreg, module_id);
#else
    ptr = malloc_r (pool, size, align_bits);
#endif
    rt_hw_interrupt_enable(level);

    return ptr;
}

void fhkfree(rt_uint32_t module_id, void *ptr)
{
    rt_base_t level;
    struct mem_pool *pool;
    if (!ptr)
        return;

    pool = &g_mem_pool[g_mem_pool_map[module_id]];
    if (module_id >= FHMEM_TYPE_MAX || !pool->inited)
    {
        BUG(module_id);
    }

    if (((rt_addr_t)ptr & (sizeof(rt_addr_t) - 1)) != 0 ||
            (rt_addr_t)ptr < pool->heap_start ||
            (rt_addr_t)ptr >= pool->heap_end)
    {
        BUG2(module_id, ptr);
    }

    level = rt_hw_interrupt_disable();
#ifdef RT_USING_MM_TRACE
    free_r2(pool, ptr, module_id);
#else
    free_r(pool, ptr);
#endif
    rt_hw_interrupt_enable(level);
}

void *rt_malloc(rt_size_t size)
{
    return fhkmalloc(FHMEM_TYPE_KERNEL, size, MM_ALIGN_4);
}

void rt_free(void *ptr)
{
    fhkfree(FHMEM_TYPE_KERNEL, ptr);
}

void *rt_memalign(rt_size_t boundary, rt_size_t size)
{
    rt_uint32_t align_bits;

    if (boundary <= 2)
        boundary = 4;

    for (align_bits = 0; align_bits <= MM_PAGE_BITS; align_bits++)
    {
        if (boundary == (1 << align_bits))
        {
            return fhkmalloc(FHMEM_TYPE_KERNEL, size, align_bits);
        }
    }

    BUG(boundary);
    return RT_NULL;
}

void *rt_realloc(void *ptr, rt_size_t size)
{
    void *nptr;

    if (!ptr)
    {
        return rt_malloc(size);
    }

    if (!size)
    {
        rt_free(ptr);
        return RT_NULL;
    }

    nptr = rt_malloc(size);
    if (nptr)
    {
        rt_memcpy(nptr, ptr, size);
        rt_free(ptr);
    }

    return nptr;
}

void *rt_calloc(rt_size_t count, rt_size_t size)
{
    void *p;
    rt_uint64_t len = count * size;

    if (len > 1*1024*1024*1024)
        return RT_NULL;

    size = len;
    p = rt_malloc(size);
    if (p)
        rt_memset(p, 0, size);

    return p;
}

#ifdef RT_MEM_STATS
void rt_memory_info(rt_uint32_t *total, rt_uint32_t *used, rt_uint32_t *max_used)
{
    int i;
    struct mem_pool *pool;
    rt_uint32_t tot = 0;
    rt_uint32_t use = 0;
    rt_uint32_t max = 0;

    for (i=0; i<MM_POOL_NUM; i++)
    {
        pool = &g_mem_pool[i];
        if (pool->inited)
        {
            tot += (pool->heap_end - pool->heap_start);
            use += pool->used_mem;
            max += pool->max_mem;
        }
    }

    if (total != RT_NULL)
        *total = tot;
    if (used  != RT_NULL)
        *used = use;
    if (max_used != RT_NULL)
        *max_used = max;
}

static void list_mem_pool(struct mem_pool *pool)
{
    int i;
    rt_base_t level;
    int zones = 0;
    int used_bytes = 0;
    int mzone_num[NZONES];
    int mzone_num_max[NZONES];
    int mzone_used_bytes[NZONES];
    int mzone_used_bytes_max[NZONES];

    if (!pool->inited)
        return;

    rt_kprintf("\npool(%d)\ntotal memory: %d[%08x,%08x]\n", pool->id,
            pool->heap_end - pool->heap_start, pool->heap_start, pool->heap_end);
    rt_kprintf("used memory : %d\n", pool->used_mem);
    rt_kprintf("maximum allocated memory: %d\n", pool->max_mem);
#ifdef RT_USING_MM_TRACE
    rt_kprintf("trace node: %d,max %d\n", pool->trace_count, pool->trace_count_max);
#endif

    level = rt_hw_interrupt_disable();
    rt_memcpy(mzone_num, pool->zone_num, sizeof(pool->zone_num));
    rt_memcpy(mzone_num_max, pool->zone_num_max, sizeof(pool->zone_num_max));
    rt_memcpy(mzone_used_bytes, pool->zone_used_bytes, sizeof(pool->zone_used_bytes));
    rt_memcpy(mzone_used_bytes_max, pool->zone_used_bytes_max, sizeof(pool->zone_used_bytes_max));
    rt_hw_interrupt_enable(level);

    rt_kprintf("\nblock_used_bytes:     %d.", pool->block_used_bytes);
    rt_kprintf("\nblock_used_bytes_max: %d.\n\n", pool->block_used_bytes_max);

    rt_kprintf("zone_index  chunk_size  zones  used_bytes  max_zones  max_used_bytes\n");
    for (i = 0; i < NZONES; i++)
    {
        zones += mzone_num[i];
        used_bytes += mzone_used_bytes[i];
        rt_kprintf("%2d          %4d        %4d   %7d     %4d       %7d\n",
                i,
                g_zone_sizes[i],
                mzone_num[i],
                mzone_used_bytes[i],
                mzone_num_max[i],
                mzone_used_bytes_max[i]);
    }

    rt_kprintf("\ntotal %d zones, used_bytes=%d.\n", zones, used_bytes);
}

void list_mem(void)
{
    int i;

    for (i=0; i<MM_POOL_NUM; i++)
    {
        list_mem_pool(&g_mem_pool[i]);
    }
}
#endif /*RT_MEM_STATS*/

static void dump_allocator(struct mem_pool *pool)
{
    rt_uint32_t  prevend = -1;
    rt_uint32_t  index;
    rt_uint32_t  page_num = pool->page_num;
    struct page_allocator *b;

    rt_kprintf("#dump allocator(%d)\n", pool->id);
    fhmm_disp_mem((rt_uint32_t *)&pool->page_alloctor[0], sizeof(struct page_allocator) * page_num);
    for (index = pool->alloctor_list; index < page_num; index = b->next)
    {
        b = &pool->page_alloctor[index];
        rt_kprintf("%d-%p: %08x\n", index, b, *((rt_uint32_t *)b));
        if ((int)index <= (int)prevend || b->page == 0 || b->page > page_num - index)
        {
            rt_kprintf("BAD\n");
            break;
        }
        prevend = index + b->page;
    }
    if (index >= page_num && index != END_FLAG)
    {
        rt_kprintf("BAD end\n");
    }
}

static void dump_pgman(struct mem_pool *pool)
{
    rt_uint32_t i;
    rt_uint32_t *data;
    struct page_man *pgm = pool->pgman_array;

    rt_kprintf("#dump pgman(%d)\n", pool->id);
    for (i=0; i < pool->page_num; i++)
    {
        data = (rt_uint32_t *)pgm;
        rt_kprintf("%08x: %08x %08x %08x %08x T%02x %d\n", data, data[0], data[1], data[2], data[3], pgm->page_type, i);
        pgm++;
    }
}

static void export_pool(struct mem_pool *pool)
{
    if (!pool->inited)
        return;

#ifdef RT_MEM_STATS
    list_mem_pool(pool);
#endif

    dump_allocator(pool);
    dump_pgman(pool);

#ifdef RT_USING_MM_TRACE
    scan_pool(pool);
#endif
}

void rt_mm_export(void)
{
    int i;
    for (i=0; i<MM_POOL_NUM; i++)
    {
        export_pool(&g_mem_pool[i]);
    }
}

#ifdef RT_USING_MM_TRACE
static int rt_mm_check(int abort)
{
    int i;
    for (i=0; i<MM_POOL_NUM; i++)
    {
        if (check_pool(&g_mem_pool[i], abort) != 0)
            return 1;
    }
    return 0;
}

void ptck(char *file, int line)
{
    if (rt_mm_check(0) != 0)
    {
        rt_kprintf("#FHMM:%s line %d, ptck error!!\n", file, line);
    }
}

#ifdef RT_USING_FINSH
#include <finsh.h>
static int st_mm_checking = 0;
static rt_thread_t mm_check_tid;
static void rt_mm_check_main(void *parameter)
{
    while (1)
    {
        if (st_mm_checking)
        {
            if (rt_mm_check(1) == 0)
            {
                rt_kprintf("rt_mm_check:PASS\n");
            }
        }
        rt_thread_delay(2 * RT_TICK_PER_SECOND);
    }
}

void cmd_rt_mm_check(int argc, char **argv)
{
    if (st_mm_checking == 0)
    {
        st_mm_checking = 1;
        if (mm_check_tid == RT_NULL)
        {
            mm_check_tid = rt_thread_create("mm_check", rt_mm_check_main, (void *)0, 1024, 0xfe/*priority*/, 5);
            if (mm_check_tid != RT_NULL)
                rt_thread_startup(mm_check_tid);
        }
    }
    else
    {
        st_mm_checking = 0;
    }
}

void cmd_rt_mm_dump(int argc, char **argv)
{
    int i;

    for (i=0; i<MM_POOL_NUM; i++)
    {
        dump_pool(&g_mem_pool[i]);
    }
}

#ifdef FINSH_USING_MSH
FINSH_FUNCTION_EXPORT_ALIAS(cmd_rt_mm_check, __cmd_rt_mm_check, start memory check);
FINSH_FUNCTION_EXPORT_ALIAS(cmd_rt_mm_dump, __cmd_rt_mm_dump, dump rt_malloc memory);
#endif
#endif /*RT_USING_FINSH*/
#endif /*RT_USING_MM_TRACE*/

#ifdef RT_USING_MM_TRACE
#ifdef RT_USING_MEM_ASAN_CHECK
static void address_check(void *p, unsigned int size)
{
    rt_base_t level;
    struct page_man *pgm;
    struct mm_trace_head *phdr;
    struct mm_trace_node *trace;
    struct mem_pool *pool = &g_mem_pool[ASAN_CHECK_POOL_ID];

    if ((rt_addr_t)p >= pool->heap_start && (rt_addr_t)p < pool->heap_end)
    {
        level = rt_hw_interrupt_disable();
        pgm = addr_2_man(pool, p);
        if (pgm->page_type == PAGE_TYPE_ZONE)
        {
            rt_uint32_t offset      = (rt_addr_t)p & MM_PAGE_MASK;
            rt_uint8_t *bitmap      = (rt_uint8_t *)&pgm->trunk_bitmap;
            rt_uint32_t zi          = pgm->zone_index;
            rt_uint32_t trunkidx;

            if (zi >= NZONES)
            {
                BUG(p);
            }

            trunkidx = offset / g_zone_sizes[zi];
            offset -= trunkidx * g_zone_sizes[zi];

            if (!(bitmap[trunkidx>>3] & (1<<(trunkidx&7))))
            {
                BUG(p);
            }

            phdr = (struct mm_trace_head *)(p - offset);
            trace = &g_mm_trace_array[phdr->trace_index];
            if (phdr->trace_index >= MM_TRACE_NODE_MAX || trace->phdr != phdr)
            {
                BUG3(phdr, phdr->trace_index, trace->phdr);
            }

            if (!(size <= MM_PAGE_SIZE && offset >= trace->headsz && offset + size <= trace->headsz + trace->alloc_len))
            {
                rt_kprintf("#FHMM:p=%p,phdr=%p,headsz=%d,alloc_len=%d,offset=%d,idx=%d,size=%d,zi=%d!\n",
                        p, phdr, trace->headsz, trace->alloc_len, offset, trunkidx, size, zi);
                BUG(p);
            }
        }
        rt_hw_interrupt_enable(level);
    }
}

void __asan_init(void)
{
}

void __asan_load1_noabort(void *p)
{
}

void __asan_load2_noabort(void *p)
{
}

void __asan_load4_noabort(void *p)
{
}

void __asan_load8_noabort(void *p)
{
}

void __asan_loadN_noabort(void *p, unsigned int size)
{
}

void __asan_store1_noabort(void *p)
{
    address_check(p, 1);
}

void __asan_store2_noabort(void *p)
{
    address_check(p, 2);
}

void __asan_store4_noabort(void *p)
{
    address_check(p, 4);
}

void __asan_store8_noabort(void *p)
{
    address_check(p, 8);
}

void __asan_storeN_noabort(void *p, unsigned int size)
{
    address_check(p, size);
}

void __asan_handle_no_return(void)
{
}

void *__wrap_memset(void *addr, int c, size_t len)
{
    __asan_storeN_noabort(addr, len);
    return rt_memset(addr, c, len);
}

void *__wrap_memmove(void *dest, const void *src, size_t len)
{
    __asan_storeN_noabort(dest, len);
    return rt_memmove(dest, src, len);
}

void *__wrap_memcpy(void *dest, const void *src, size_t len)
{
    __asan_storeN_noabort(dest, len);
    return rt_memcpy(dest, src, len);
}

char *__wrap_strcat(char *s, const char *append)
{
    if ((s == NULL) || (append == NULL))
    {
        return s;
    }

    char *end = s;
    size_t len = rt_strlen(append);
    for (; *end != '\0'; ++end)
    {
    }

    __asan_storeN_noabort(end, len + 1);

    rt_memcpy(end, append, len+1);

    return s;
}

char *__wrap_strcpy(char *dest, const char *src)
{
    if (!dest)
        return dest;

    if (!src)
    {
        address_check(dest, 1);
        *dest = 0;
        return dest;
    }

    size_t len = rt_strlen(src);
    __asan_storeN_noabort(dest, len + 1);

    rt_memcpy(dest, src, len+1);

    return dest;
}

char *__wrap_strncat(char *dest, const char *src, size_t n)
{
    if ((dest == NULL) || (src == NULL))
    {
        return dest;
    }

    char *end = dest;
    size_t len = rt_strlen(src);
    size_t size = len > n ? n : len;
    for (; *end != '\0'; ++end)
    {
    }

    __asan_storeN_noabort(end, size + 1);

    rt_memcpy(end, src, size);
    end += size;
    *end = 0;

    return dest;
}

char *__wrap_strncpy(char *dest, const char *src, size_t n)
{
    if (!dest)
        return dest;

    if (!src)
    {
        address_check(dest, 1);
        *dest = 0;
        return dest;
    }

    size_t len = rt_strlen(src) + 1;
    if (len > n)
    {
        len = n;
    }

    __asan_storeN_noabort(dest, len);

    rt_memcpy(dest, src, len);

    return dest;
}
#endif
#endif

#include "slab_customer.c"
