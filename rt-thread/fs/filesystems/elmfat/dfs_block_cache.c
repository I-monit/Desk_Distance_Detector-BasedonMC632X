/**
 * Copyright (c) 2015-2019 Shanghai Fullhan Microelectronics Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-05-12     zhangy       the first version
 *
 */
/*
/*****************************************************************************
 *  Include Section
 *  add all #include here
 *****************************************************************************/
#include <rtthread.h>
#include "ffconf.h"
#include "ff.h"
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <dfs_fs.h>
#include <dfs_file.h>
#include "dfs_block_cache.h"
/*****************************************************************************
 * Define section
 * add all #define here
 *****************************************************************************/
#define DFS_SEC_SIZE    512

#define DFS_SEC_CACHE_NO    (DFS_SEC_CACHE_TYPE_128_NO + DFS_SEC_CACHE_TYPE_64_NO + \
                            DFS_SEC_CACHE_TYPE_32_NO + DFS_SEC_CACHE_TYPE_16_NO + \
                                DFS_SEC_CACHE_TYPE_8_NO + DFS_SEC_CACHE_TYPE_4_NO + \
                                DFS_SEC_CACHE_TYPE_2_NO + DFS_SEC_CACHE_TYPE_1_NO)


#define DFSBLK_CACHE_ASSERT(_expr) do { \
    if (!(_expr)) \
    { rt_kprintf("%s:%d:\n",  \
                      __FILE__, __LINE__);\
     while (1)\
      ;\
    } \
} while (0)

#ifndef min
#define min(x, y) (x < y ? x : y)
#endif
#ifndef max
#define max(x, y) (x < y ? y : x)
#endif
#define min_t(t, x, y) ((t)x < (t)y ? (t)x : (t)y)
#define WRITE_CNT_AREA_WEIGHT    5
#define TIME_ALIVE_AREA_WEIGHT    5
#define FOCUS_AREA_WEIGHT    5
#define NORMAL_AREA_DEFAULT_WEIGHT   0x0000ffff

#define ALIGN_UP(x, a)  (((x) + (a) - 1) & ~((a) - 1))
#define ALIGN_DOWN(x, a)    ((x) & ~(((a)) - 1))
/****************************************************************************
 * ADT section
 *  add definition of user defined Data Type that only be used in this file  here
 ***************************************************************************/
struct range_result
{
    int start;
    int end;
    int count;
};

struct s_sec_cache_node
{
#define SEC_DEFAULT_NO    0xdeadbeef
    rt_uint32_t st_lg_sec;
    rt_uint32_t st_ca_add;
    rt_uint32_t dirty_array[EACH_CACHE_NODE_SEC_SIZE / 32];
    rt_uint32_t dirty_no;
#define SEC_MAX_WCNT_NO        0x0000ffff
    rt_uint32_t write_cnt;
#define SEC_MAX_ALIVE_TIME        0x0000ffff
    rt_uint32_t time_alive_left;
    rt_uint32_t replace_weight;
#define TYPE_1_SEC        0
#define TYPE_2_SEC        1
#define TYPE_4_SEC        2
#define TYPE_8_SEC        3
#define TYPE_16_SEC        4
#define TYPE_32_SEC        5
#define TYPE_64_SEC        6
#define TYPE_128_SEC        7
    rt_uint32_t type;
#define SEC_DIRTY            0xaa
#define SEC_CLEAN            0x0
    rt_uint32_t dirty;
#define SEC_BUSY            0xaa
#define SEC_IDLE            0x0
    rt_uint32_t active;
    struct range_result dirty_info;
};

char *debug_type_array[] = {
    "_1_SEC",
    "_2_SEC",
    "_4_SEC",
    "_8_SEC",
    "_16_SEC",
    "_32_SEC",
    "_64_SEC",
    "_128_SEC",
};

struct s_blk_cache
{
    rt_uint8_t *cache_head;
    struct s_sec_cache_node sec_node_array[DFS_SEC_CACHE_NO];
    struct rt_thread *guard_thread;
    struct rt_semaphore sem_wake;
    struct rt_device *p_io_dev;
    rt_uint32_t total_alloc_sec;
    rt_uint32_t focus_st;
    rt_uint32_t focus_size;
    rt_size_t (*io_read)(rt_device_t dev, rt_off_t pos, void *buffer, rt_size_t size);
    rt_size_t (*io_write)(rt_device_t dev, rt_off_t pos, void *buffer, rt_size_t size);
};
/*****************************************************************************
 *  static fun;
 *****************************************************************************/

/*****************************************************************************
 *  extern fun;
 *****************************************************************************/

/*****************************************************************************
 * Global variables section - Local
 * define global variables(will be refered only in this file) here,
 * static keyword should be used to limit scope of local variable to this file
 * e.g.
 *  static uint8_t ufoo;
 *****************************************************************************/
static struct s_blk_cache *g_blk_cache = 0;
/*****************************************************************************
 * func below..
 *****************************************************************************/
static void dump_sec_cache_info(struct s_sec_cache_node *p)
{
    rt_kprintf("node add : \t\t%08x\n",(unsigned int)p);
    rt_kprintf("lg_sec_no : \t\t%08x\n",p->st_lg_sec);
    rt_kprintf("st_ca_add : \t\t%08x\n",p->st_ca_add);
    rt_kprintf("dirty_bit : \t\t%08x : %08x\n",p->dirty_array[0],p->dirty_array[1]);
    rt_kprintf("write_cnt : \t\t%08x\n",p->write_cnt);
    rt_kprintf("time_alive_left : \t%08x\n",p->time_alive_left);
    rt_kprintf("weight : \t\t%08x\n",p->replace_weight);
    rt_kprintf("[%s]\n",debug_type_array[p->type]);
    rt_kprintf("[%s]\n",(p->dirty == SEC_DIRTY) ? "DIRTY" : "CLEAN");
    rt_kprintf("[%s]\n",(p->active == SEC_BUSY) ? "BUSY" : "IDLE");
}
#if (0)
static void dump_all_cache_info(struct s_blk_cache *p)
{
    int i;
    rt_kprintf("------------->\n");
    rt_kprintf("cache_head : %08x\n",(int)p->cache_head);
    for (i = 0; i < DFS_SEC_CACHE_NO; i++)
        dump_sec_cache_info(&p->sec_node_array[i]);
    rt_kprintf("<-------------\n");
}
#endif

static void dump_runtime_cache_info(struct s_blk_cache *p)
{
    struct s_sec_cache_node *p_sec;
    int i;

    rt_kprintf("---------------------->\n");
    rt_kprintf("[CACHE ADD]\t[LOGIC SEC ST]\t[DIRTY SEC SZ]\t[WRITE CNT]\t[ALIVE TIME LEFT]\t[WIGHT]\t\t\t[DIRTY|CLEAN]\t[BUSY|IDLE]\n");
    for (i = 0, p_sec = p->sec_node_array; i < DFS_SEC_CACHE_NO; i++, p_sec++)
    {
        rt_kprintf("%08x\t",p_sec->st_ca_add);
        rt_kprintf("%08x\t",p_sec->st_lg_sec);
        rt_kprintf("%08x\t",p_sec->dirty_no);
        rt_kprintf("%08x\t",p_sec->write_cnt);
        rt_kprintf("%08x\t\t",p_sec->time_alive_left);
        rt_kprintf("%08x\t\t",p_sec->replace_weight);
        rt_kprintf("[%s]\t\t",(p_sec->dirty == SEC_DIRTY) ? "DIRTY" : "CLEAN");
        rt_kprintf("[%s]\n",(p_sec->active == SEC_BUSY) ? "BUSY" : "IDLE");
    }
    rt_kprintf("<----------------------\n");
}


static rt_uint32_t __cache_left_sec_size(rt_uint32_t sec_in, rt_uint32_t allign)
{
    return ALIGN_UP(sec_in + 1, allign) - sec_in;
}

static rt_uint32_t cache_left_sec_size(rt_uint32_t sec_in)
{
    return __cache_left_sec_size(sec_in, EACH_CACHE_NODE_SEC_SIZE);
}

static rt_uint32_t sec_2_cache_st(rt_uint32_t sec_in)
{
    return ALIGN_DOWN(sec_in, EACH_CACHE_NODE_SEC_SIZE);
}


static void find_continuous_ones(rt_uint32_t *array, rt_uint32_t length, struct range_result *p)
{
    rt_uint32_t *p_32;
    int multi32;
    rt_uint32_t i;
    rt_uint32_t count = 0;
    rt_uint32_t first_bit1;
    rt_uint32_t tmp;

    p->start = -1;
    p->end = -1;
    p->count = 0;

    for (i = 0, p_32 = array, multi32 = -1; i < length; i++, p_32++)
    {
        tmp = __rt_ffs(*p_32);
        /*ignore all 0*/
        if (tmp == 0)
            continue;
        /*if no value. rec first base*/
        if (multi32 == -1)
        {
            multi32 = i;
            p->start = multi32 * 32 + (tmp - 1);
            break;
        }
    }

    first_bit1 = (tmp - 1);
    /*get last and 1 cnt*/
    for (; i < length; i++, p_32++)
    {
        while (first_bit1 < 32)
        {
            if ((*p_32 & (1 << (first_bit1))) == 0)
                goto out;
            count++;
            first_bit1++;
        }
        first_bit1 = 0;
    }
out:
    p->count = count;
    if (count)
        p->end = i * 32 + first_bit1 - 1;
    return;
}

static void clear_continuous_ones(rt_uint32_t *array, rt_uint32_t length, struct range_result *p)
{
    rt_uint32_t *p_32;
    rt_uint32_t i;
    rt_uint32_t j;
    rt_uint32_t find_first_1 = 0;

    if (p->count <= 0)
        return;
    i = p->start / 32;
    p_32 = &array[i];

    for (; i < length; i++, p_32++)
    {
        for (j = 0; j < 32; j++)
        {
            if ((*p_32 & (1 << j)))
            {
                *p_32 &= ~(1 << j);
                find_first_1 = 1;
            }
            else
            {
                if (find_first_1)
                    goto out;
            }
        }
    }
out:
    p->count = 0;
}

static void sec_alive_plus(struct s_sec_cache_node *p_sec)
{
    (p_sec->time_alive_left >= SEC_MAX_ALIVE_TIME) ?  : p_sec->time_alive_left++;
}

static void sec_alive_reduce(struct s_sec_cache_node *p_sec)
{
    (p_sec->time_alive_left == 0) ?  : p_sec->time_alive_left--;
}

static rt_uint32_t sec_alive_get(struct s_sec_cache_node *p_sec)
{
    return p_sec->time_alive_left;
}

static void sec_alive_init(struct s_sec_cache_node *p_sec)
{
    p_sec->time_alive_left = SEC_MAX_ALIVE_TIME;
}

static void sec_type_set(struct s_sec_cache_node *p_sec, rt_uint32_t type)
{
    p_sec->type = type;
}


static rt_uint32_t sec_winsize_get(struct s_sec_cache_node *p_sec)
{
    return 1 << p_sec->type;
}

static void sec_dirty_no_set(struct s_sec_cache_node *p_sec, rt_uint32_t dirty_no)
{
    p_sec->dirty_no = dirty_no;
}

static void sec_cbuf_set(struct s_sec_cache_node *p_sec, rt_uint32_t st_ca_add)
{
    p_sec->st_ca_add = st_ca_add;
}


static rt_uint32_t sec_cbuf_get(struct s_sec_cache_node *p_sec)
{
    return p_sec->st_ca_add;
}

static void sec_lgno_set(struct s_sec_cache_node *p_sec, rt_uint32_t st_lg_sec)
{
    p_sec->st_lg_sec = st_lg_sec;
}

static rt_uint32_t sec_lgno_get(struct s_sec_cache_node *p_sec)
{
    return p_sec->st_lg_sec;
}

static void sec_dirty_set(struct s_sec_cache_node *p_sec)
{
    p_sec->dirty = SEC_DIRTY;
}

static void sec_dirty_clean(struct s_sec_cache_node *p_sec)
{
    p_sec->dirty = SEC_CLEAN;
}

static rt_uint32_t sec_dirty_get(struct s_sec_cache_node *p_sec)
{
    return p_sec->dirty;
}

static void sec_busy_set(struct s_sec_cache_node *p_sec)
{
    p_sec->active = SEC_BUSY;
}
static void sec_busy_clean(struct s_sec_cache_node *p_sec)
{
    p_sec->active = SEC_IDLE;
}

static rt_uint32_t sec_busy_get(struct s_sec_cache_node *p_sec)
{
    return p_sec->active;
}

static void sec_wcnt_init(struct s_sec_cache_node *p_sec)
{
    p_sec->write_cnt = 0;
}

static void sec_wcnt_plus(struct s_sec_cache_node *p_sec)
{
    (p_sec->write_cnt >= SEC_MAX_WCNT_NO) ?  : p_sec->write_cnt++;
}

static rt_uint32_t sec_wcnt_get(struct s_sec_cache_node *p_sec)
{
    return p_sec->write_cnt;
}

static void sec_weight_set(struct s_sec_cache_node *p_sec, rt_uint32_t weight)
{
    p_sec->replace_weight = weight;
}


/*RT_TRUE is over lap or RT_FALSE*/
static int is_sec_overlap(rt_uint32_t cache_sec_st, rt_uint32_t cache_sec_size,
rt_uint32_t usr_sec_st, rt_uint32_t usr_sec_size)
{
    rt_uint32_t cache_sec_end;
    rt_uint32_t usr_sec_end;

    cache_sec_end = cache_sec_st + cache_sec_size;
    usr_sec_end = usr_sec_st + usr_sec_size;

    if ((cache_sec_st >= usr_sec_end) || (cache_sec_end <= usr_sec_st))
        return RT_FALSE;
    else
        return RT_TRUE;
}

static int set_continuous_ones(struct s_sec_cache_node *p_sec, rt_uint32_t sec_st, rt_uint32_t sec_size)
{
    rt_uint32_t *p_32;
    rt_uint32_t i;
    rt_uint32_t j;
    rt_uint32_t cnt;
    rt_uint32_t tmp_dirty_size;
    rt_uint32_t ret_dirty_size;
    rt_uint32_t tmp_index;
    rt_uint32_t len = sizeof(p_sec->dirty_array) / sizeof(rt_uint32_t);

    if (sec_busy_get(p_sec) == SEC_IDLE || sec_dirty_get(p_sec) == SEC_CLEAN)
    {
        rt_kprintf("%s :: node idle or clean.....\n",__func__);
        dump_sec_cache_info(p_sec);
        return 0;
    }

    if (sec_2_cache_st(sec_st) != sec_lgno_get(p_sec))
    {
        rt_kprintf("%s :: sec get in [%x] not allign.\n",__func__,sec_st);
        dump_sec_cache_info(p_sec);
        return 0;
    }

    tmp_index = sec_st - sec_lgno_get(p_sec);
    tmp_dirty_size = min(cache_left_sec_size(sec_st), sec_size);
    ret_dirty_size = tmp_dirty_size;
    i = tmp_index / 32;
    p_32 = &p_sec->dirty_array[i];

    for (; i < len; i++, p_32++)
    {
        for (j = 0, cnt = 0; j < min(tmp_dirty_size, 32); j++)
        {
            *p_32 |= 1 << (tmp_index % 32);
            tmp_index++;
            cnt++;
            if (((tmp_index - 1) % 32) == 31)
            {
                break;
            }
        }
        tmp_dirty_size -= cnt;
        if (!tmp_dirty_size)
            goto out;
    }
out:
    return ret_dirty_size;
}

static void check_overlap(struct s_blk_cache *p)
{
    int  ret;
    int i;
    int j;
    struct s_sec_cache_node *p_sec = 0;
    struct s_sec_cache_node *p_cmpsec = 0;

    for (i = 0, p_sec = p->sec_node_array; i < DFS_SEC_CACHE_NO; i++, p_sec++)
    {
        if (sec_busy_get(p_sec) == SEC_IDLE)
            continue;
        for (j = 0, p_cmpsec = p->sec_node_array; j < DFS_SEC_CACHE_NO; j++, p_cmpsec++)
        {
            if (sec_busy_get(p_cmpsec) == SEC_IDLE)
                continue;
            if (p_cmpsec == p_sec)
                continue;

            ret = is_sec_overlap(sec_lgno_get(p_sec), sec_winsize_get(p_sec),
            sec_lgno_get(p_cmpsec), sec_winsize_get(p_cmpsec));
            if (ret == RT_TRUE)
            {
                rt_kprintf("ERRRRR::::::::::::::::.....%x : %x\n",sec_lgno_get(p_sec),sec_lgno_get(p_cmpsec));
                dump_sec_cache_info(p_sec);
                dump_sec_cache_info(p_cmpsec);
                return;
            }
        }
    }
}



static void all_dirty_desc_alive_reduce(struct s_blk_cache *p)
{
    struct s_sec_cache_node *p_sec = 0;
    int i;

    for (i = 0, p_sec = p->sec_node_array; i < DFS_SEC_CACHE_NO; i++, p_sec++)
    {
        if ((sec_busy_get(p_sec) == SEC_BUSY) && (sec_dirty_get(p_sec) == SEC_DIRTY))
        {
            sec_alive_reduce(p_sec);
        }
    }
}

static void blk_cache_thread(void *param)
{

    struct s_blk_cache *blkcache_obj = 0;


    while (1)
    {
        blkcache_obj = (struct s_blk_cache *)param;
        rt_thread_sleep(100);
        all_dirty_desc_alive_reduce(blkcache_obj);
    }
}

rt_uint8_t *__cache_node_init(struct s_sec_cache_node *p_sec, rt_uint32_t type,
rt_uint32_t node_size, rt_uint8_t *_addr_head)
{
    int i;
    rt_uint8_t *update_cache;
    rt_uint8_t *addr_head = _addr_head;

    for (i = 0; i < node_size; i++, p_sec++)
    {
        sec_cbuf_set(p_sec, addr_head + i * (1 << type) * DFS_SEC_SIZE);
        sec_type_set(p_sec, type);
    }
    update_cache = addr_head + node_size * (1 << type) * DFS_SEC_SIZE;
    return update_cache;
}

void cache_node_init(struct s_blk_cache *p)
{
    int i;
    unsigned char *cache_head;
    struct s_sec_cache_node *p_sec = 0;

    p_sec = p->sec_node_array;
    for (i = 0; i < DFS_SEC_CACHE_NO; i++, p_sec++)
    {
        sec_alive_init(p_sec);
        sec_lgno_set(p_sec, SEC_DEFAULT_NO);
        sec_dirty_no_set(p_sec, 0);
        sec_dirty_clean(p_sec);
        sec_busy_clean(p_sec);
        sec_weight_set(p_sec, 0);
    }

    cache_head = p->cache_head;
    p_sec = p->sec_node_array;

    cache_head = __cache_node_init(p_sec, TYPE_128_SEC, DFS_SEC_CACHE_TYPE_128_NO, cache_head);
    p_sec += DFS_SEC_CACHE_TYPE_128_NO;

    cache_head = __cache_node_init(p_sec, TYPE_64_SEC, DFS_SEC_CACHE_TYPE_64_NO, cache_head);
    p_sec += DFS_SEC_CACHE_TYPE_64_NO;

    cache_head = __cache_node_init(p_sec, TYPE_32_SEC, DFS_SEC_CACHE_TYPE_32_NO, cache_head);
    p_sec += DFS_SEC_CACHE_TYPE_32_NO;

    cache_head = __cache_node_init(p_sec, TYPE_16_SEC, DFS_SEC_CACHE_TYPE_16_NO, cache_head);
    p_sec += DFS_SEC_CACHE_TYPE_16_NO;

    cache_head = __cache_node_init(p_sec, TYPE_8_SEC, DFS_SEC_CACHE_TYPE_8_NO, cache_head);
    p_sec += DFS_SEC_CACHE_TYPE_8_NO;

    cache_head = __cache_node_init(p_sec, TYPE_4_SEC, DFS_SEC_CACHE_TYPE_4_NO, cache_head);
    p_sec += DFS_SEC_CACHE_TYPE_4_NO;

    cache_head = __cache_node_init(p_sec, TYPE_2_SEC, DFS_SEC_CACHE_TYPE_2_NO, cache_head);
    p_sec += DFS_SEC_CACHE_TYPE_2_NO;

    __cache_node_init(p_sec, TYPE_1_SEC, DFS_SEC_CACHE_TYPE_1_NO, cache_head);

}

struct s_blk_cache *blk_sw_init(void)
{

    struct s_blk_cache *ret_obj = 0;
    rt_uint32_t alloc_sec;

    ret_obj = rt_malloc(sizeof(struct s_blk_cache));
    if (!ret_obj)
    {
        rt_kprintf("%s :: no mem alloc obj ...\n",__func__);
        return 0;
    }
    rt_memset(ret_obj, 0, sizeof(struct s_blk_cache));

    alloc_sec = DFS_SEC_CACHE_TYPE_128_NO * 128 + DFS_SEC_CACHE_TYPE_64_NO * 64 +
    DFS_SEC_CACHE_TYPE_32_NO * 32 + DFS_SEC_CACHE_TYPE_16_NO * 16 +
    DFS_SEC_CACHE_TYPE_8_NO * 8 + DFS_SEC_CACHE_TYPE_4_NO * 4 +
    DFS_SEC_CACHE_TYPE_2_NO * 2 + DFS_SEC_CACHE_TYPE_1_NO * 1;
    ret_obj->cache_head = (rt_uint8_t *)rt_malloc(alloc_sec * DFS_SEC_SIZE);

    if (!ret_obj->cache_head)
    {
        rt_kprintf("%s :: no mem alloc cache buff ...\n",__func__);
        rt_free(ret_obj);
        return 0;
    }
    ret_obj->total_alloc_sec = alloc_sec;
    rt_kprintf("%s :: max alloc sec is %d\n",__func__,alloc_sec);
    ret_obj->guard_thread = rt_thread_create("blk_cache",
    blk_cache_thread, (void *)ret_obj, GUARD_THREAD_STACK_SIZE, GUARD_THREAD_PREORITY, 10);
    rt_sem_init(&ret_obj->sem_wake, "blk_cache", 0, RT_IPC_FLAG_FIFO);

    cache_node_init(ret_obj);
    return ret_obj;
}

static struct s_sec_cache_node *check_if_hit_cache(struct s_blk_cache *p,
rt_uint32_t sec_st, rt_uint32_t sec_size,
rt_uint32_t *ret_cache_buf, rt_uint32_t *ret_cache_sec_size)
{
    struct s_sec_cache_node *p_sec = 0;
    struct s_sec_cache_node *p_ret_sec = 0;
    int i;

    for (i = 0, p_sec = p->sec_node_array; i < DFS_SEC_CACHE_NO; i++, p_sec++)
    {
        if (sec_busy_get(p_sec) == SEC_BUSY)
        {
            if (is_sec_overlap(sec_lgno_get(p_sec), sec_winsize_get(p_sec), sec_st, cache_left_sec_size(sec_st)) == RT_TRUE)
            {
                *ret_cache_sec_size = min(cache_left_sec_size(sec_st), sec_size);
                *ret_cache_buf = sec_cbuf_get(p_sec) + ((sec_st - sec_lgno_get(p_sec)) * DFS_SEC_SIZE);
                p_ret_sec = p_sec;
                break;
            }
        }
    }
    return p_ret_sec;
}


static int flush_single_node(struct s_blk_cache *p, struct s_sec_cache_node *p_dirty)
{
    rt_size_t result;

    if (sec_dirty_get(p_dirty) == SEC_CLEAN)
    {
        rt_kprintf("node get in is clean....\n");
        return 0;
    }

    do
    {
        find_continuous_ones(p_dirty->dirty_array, sizeof(p_dirty->dirty_array) / sizeof(rt_uint32_t), &p_dirty->dirty_info);
        if (p_dirty->dirty_info.count == 0)
            break;

        result = p->io_write(p->p_io_dev, sec_lgno_get(p_dirty) + p_dirty->dirty_info.start,
        sec_cbuf_get(p_dirty) + p_dirty->dirty_info.start * DFS_SEC_SIZE, p_dirty->dirty_info.count);
        if (result != p_dirty->dirty_info.count)
        {
            rt_kprintf("%s :: flush cache to IO ERR........\n",__func__);
            rt_memset(p_dirty->dirty_array, 0, sizeof(p_dirty->dirty_array));
            sec_dirty_clean(p_dirty);
            sec_wcnt_init(p_dirty);
            sec_busy_clean(p_dirty);
            return -1;
        }
        clear_continuous_ones(p_dirty->dirty_array, sizeof(p_dirty->dirty_array) / sizeof(rt_uint32_t), &p_dirty->dirty_info);
    } while (1);


    sec_dirty_clean(p_dirty);
    sec_wcnt_init(p_dirty);

    return 0;
}

#if (0)
static int flush_bro_node(struct s_blk_cache *p, struct s_sec_cache_node *p_dirty)
{
    rt_size_t result;
    rt_uint32_t flush_no = 0;
    struct s_sec_cache_node *p_st_node = p_dirty;
    struct s_sec_cache_node *p_end_node = p_dirty;
    struct s_sec_cache_node *p_cur_sec = p_dirty;
    struct s_sec_cache_node *p_pre_sec = p_dirty - 1;
    struct s_sec_cache_node *p_next_sec = p_dirty + 1;


    if (sec_dirty_get(p_dirty) == SEC_CLEAN)
    {
        rt_kprintf("node get in is clean....\n");
        return 0;
    }

    if (p_dirty != &p->sec_node_array[0])
    {
        while (1)
        {
            if (sec_dirty_get(p_pre_sec) == SEC_CLEAN)
                break;
            if ((sec_lgno_get(p_cur_sec) - sec_winsize_get(p_cur_sec)) != sec_lgno_get(p_pre_sec))
                break;
            p_st_node = p_pre_sec;
            if (p_pre_sec == &p->sec_node_array[0])
                break;
            p_pre_sec--;
            p_cur_sec--;
        }
    }
    p_cur_sec = p_dirty;

    if (p_dirty != &p->sec_node_array[DFS_SEC_CACHE_NO - 1])
    {
        while (1)
        {
            if (sec_dirty_get(p_next_sec) == SEC_CLEAN)
                break;
            if ((sec_lgno_get(p_cur_sec) + sec_winsize_get(p_cur_sec)) != sec_lgno_get(p_next_sec))
                break;
            p_end_node = p_next_sec;
            if (p_next_sec == &p->sec_node_array[DFS_SEC_CACHE_NO - 1])
                break;
            p_cur_sec++;
            p_next_sec++;
        }
    }

    if (p_st_node == p_end_node)
    {
        sec_dirty_clean(p_st_node);
        sec_wcnt_init(p_st_node);
        flush_no = sec_winsize_get(p_st_node);
    }
    else
    {
        for (p_cur_sec = p_st_node; p_cur_sec != p_end_node + 1; p_cur_sec++)
        {
            sec_dirty_clean(p_cur_sec);
            sec_wcnt_init(p_cur_sec);
            flush_no += sec_winsize_get(p_cur_sec);
        }
    }

    result = p->io_write(p->p_io_dev, p_st_node->st_lg_sec, p_st_node->st_ca_add, flush_no);
    if (result != flush_no)
    {
        rt_kprintf("%s :: flush cache to IO ERR........\n",__func__);
        return -1;
    }
    return 0;
}
#endif

static struct s_sec_cache_node *get_sec_desc_multi_with_weight(struct s_blk_cache *p, rt_uint32_t sec_size, int *p_ret_err)
{
    struct rt_thread *thread;

    int result;
    int i;
    struct s_sec_cache_node *p_first_null_sec = 0;
    struct s_sec_cache_node *p_first_clean_sec = 0;
    struct s_sec_cache_node *p_flush_sec = 0;
    struct s_sec_cache_node *p_sec = 0;
    rt_uint32_t wcnt_max_weight = SEC_MAX_WCNT_NO + 1;
    rt_uint32_t talive_max_weight = SEC_MAX_ALIVE_TIME + 1;
    rt_uint32_t normal_area_weight = NORMAL_AREA_DEFAULT_WEIGHT + 1;
    rt_uint32_t pre_weight = 0;
    rt_uint32_t cur_weight = 0;

    thread = rt_thread_self();
    *p_ret_err = 0;
    if (sec_size == 0)
    {
        rt_kprintf("[%s] :: R/W sec size [%d] ERR..\n",thread->name, sec_size);
        DFSBLK_CACHE_ASSERT(0);
    }

    if (sec_size > p->total_alloc_sec)
    {
        rt_kprintf("[%s] :: R/W sec size [%d] > total buf sec[%d]\n",
        thread->name, sec_size, p->total_alloc_sec);
    }

    for (i = 0, p_sec = p->sec_node_array; i < DFS_SEC_CACHE_NO; i++, p_sec++)
    {
        if (sec_busy_get(p_sec) == SEC_IDLE)
        {
            p_first_null_sec = p_sec;
            break;
        }
        if ((!p_first_clean_sec) && (sec_busy_get(p_sec) == SEC_BUSY) && (sec_dirty_get(p_sec) == SEC_CLEAN))
        {
            p_first_clean_sec = p_sec;
            break;
        }

        if ((!p_flush_sec) && (sec_busy_get(p_sec) == SEC_BUSY) && (sec_dirty_get(p_sec) == SEC_DIRTY))
            p_flush_sec = p_sec;

        if ((p_flush_sec) && (sec_busy_get(p_sec) == SEC_BUSY) && (sec_dirty_get(p_sec) == SEC_DIRTY))
        {
            cur_weight = (wcnt_max_weight - sec_wcnt_get(p_sec)) * WRITE_CNT_AREA_WEIGHT;
            cur_weight += ((talive_max_weight - sec_alive_get(p_sec)) * TIME_ALIVE_AREA_WEIGHT);
            /*if not overlap focus area...*/
            if (is_sec_overlap(p->focus_st, p->focus_size, sec_lgno_get(p_sec), sec_winsize_get(p_sec)) == RT_FALSE)
                cur_weight += normal_area_weight;
            sec_weight_set(p_sec, cur_weight);
            if (cur_weight > pre_weight)
            {
                pre_weight = cur_weight;
                p_flush_sec = p_sec;
            }
        }
    }

    if (p_first_null_sec)
        return p_first_null_sec;

    if (p_first_clean_sec)
        return p_first_clean_sec;

    result = flush_single_node(p, p_flush_sec);

    if (result != 0)
    {
        *p_ret_err = result;
        rt_kprintf("%s :: flush_single_node ERR........\n",__func__);
    }

    return p_flush_sec;
}



int api_blk_cache_write(void *_p, rt_uint32_t sec_lg_no, const void *usr_buf, rt_uint32_t sec_size)
{

    struct s_sec_cache_node *p_sec = 0;
    rt_uint32_t cache_mem;
    rt_uint32_t tmp_sec_size;
    rt_uint32_t cache_sec_win;
    rt_uint32_t io_size;
    rt_uint8_t *tmp_usr_buf = (rt_uint8_t *)usr_buf;
    struct s_blk_cache *p = (struct s_blk_cache *)_p;
    int io_err = 0;

    check_overlap(p);
    do
    {
        p_sec = check_if_hit_cache(p, sec_lg_no, sec_size, &cache_mem, &cache_sec_win);
        if (p_sec)
        {
            tmp_sec_size = min(cache_sec_win, sec_size);
            rt_memcpy(cache_mem, tmp_usr_buf, tmp_sec_size * DFS_SEC_SIZE);
            sec_alive_plus(p_sec);
            sec_wcnt_plus(p_sec);
            sec_dirty_set(p_sec);
            set_continuous_ones(p_sec, sec_lg_no, tmp_sec_size);
        }
        else
        {
            p_sec = get_sec_desc_multi_with_weight(p, sec_size, &io_err);
            if (io_err)
                return -1;
            DFSBLK_CACHE_ASSERT(p_sec);
            sec_alive_init(p_sec);
            sec_busy_set(p_sec);
            sec_lgno_set(p_sec, sec_2_cache_st(sec_lg_no));
            sec_wcnt_init(p_sec);
            sec_wcnt_plus(p_sec);
            tmp_sec_size = min(cache_left_sec_size(sec_lg_no), sec_size);
            io_size = p->io_read(p->p_io_dev, p_sec->st_lg_sec, p_sec->st_ca_add, sec_winsize_get(p_sec));
            if (io_size != sec_winsize_get(p_sec))
            {
                rt_memset(p_sec->dirty_array, 0, sizeof(p_sec->dirty_array));
                sec_busy_clean(p_sec);
                return -1;
            }
            sec_dirty_set(p_sec);
            rt_memcpy(sec_cbuf_get(p_sec) + ((sec_lg_no - sec_lgno_get(p_sec)) * DFS_SEC_SIZE), tmp_usr_buf, tmp_sec_size * DFS_SEC_SIZE);
            set_continuous_ones(p_sec, sec_lg_no, tmp_sec_size);
        }
        tmp_usr_buf += (tmp_sec_size * DFS_SEC_SIZE);
        sec_lg_no += tmp_sec_size;
        sec_size -= tmp_sec_size;
    } while (sec_size);

    return 0;

}


int api_blk_cache_read(void *_p, rt_uint32_t sec_lg_no, void *usr_buf, rt_uint32_t sec_size)
{
    struct s_sec_cache_node *p_sec = 0;
    rt_uint32_t cache_mem;
    rt_uint32_t tmp_sec_size;
    rt_uint32_t cache_sec_win;
    rt_uint32_t io_size;
    rt_uint8_t *tmp_usr_buf = (rt_uint8_t *)usr_buf;
    int io_err = 0;
    struct s_blk_cache *p = (struct s_blk_cache *)_p;

    check_overlap(p);
    do
    {
        p_sec = check_if_hit_cache(p, sec_lg_no, sec_size, &cache_mem, &cache_sec_win);
        if (p_sec)
        {
            tmp_sec_size = min(cache_sec_win, sec_size);
            rt_memcpy(tmp_usr_buf, cache_mem, tmp_sec_size * DFS_SEC_SIZE);
            sec_alive_plus(p_sec);
        }
        else
        {
            p_sec = get_sec_desc_multi_with_weight(p, sec_size, &io_err);
            if (io_err)
                return -1;
            DFSBLK_CACHE_ASSERT(p_sec);
            sec_alive_init(p_sec);
            sec_busy_set(p_sec);
            sec_lgno_set(p_sec, sec_2_cache_st(sec_lg_no));
            tmp_sec_size = min(cache_left_sec_size(sec_lg_no), sec_size);
            io_size = p->io_read(p->p_io_dev, p_sec->st_lg_sec, p_sec->st_ca_add, sec_winsize_get(p_sec));
            if (io_size != sec_winsize_get(p_sec))
            {
                rt_memset(p_sec->dirty_array, 0, sizeof(p_sec->dirty_array));
                sec_busy_clean(p_sec);
                return -1;
            }
            rt_memcpy(tmp_usr_buf, sec_cbuf_get(p_sec) + ((sec_lg_no - sec_lgno_get(p_sec)) * DFS_SEC_SIZE), tmp_sec_size * DFS_SEC_SIZE);
        }
        tmp_usr_buf += (tmp_sec_size * DFS_SEC_SIZE);
        sec_lg_no += tmp_sec_size;
        sec_size -= tmp_sec_size;
    } while (sec_size);

    return 0;

}

void api_flush_all_blk_cache(void *dfs_handle)
{
    int i;
    int ret;
    struct s_blk_cache *p = (struct s_blk_cache *)dfs_handle;
    struct s_sec_cache_node *p_sec;

    for (i = 0, p_sec = p->sec_node_array; i < DFS_SEC_CACHE_NO; i++, p_sec++)
    {
        if ((sec_busy_get(p_sec) == SEC_BUSY) && (sec_dirty_get(p_sec) == SEC_DIRTY))
        {
            ret = mmcsd_wait_cd_changed(0);
            if (ret == MMCSD_HOST_UNPLUGED)
            {
                rt_kprintf("%s :: bypass all flush dirty info...\n",__func__);
                return;
            }
            flush_single_node(p, p_sec);
        }
    }

}

void *api_dfs_get_cache_handle(void)
{
    return (void *)g_blk_cache;
}

int api_dfs_bind_hw_io_handle(void *dfs_handle, void *io_handle)
{
    struct s_blk_cache *p_blk = (struct s_blk_cache *)dfs_handle;

    p_blk->p_io_dev = io_handle;
    p_blk->io_read = rt_device_read;
    p_blk->io_write = rt_device_write;

    if (p_blk->guard_thread != RT_NULL)
    {
        if ((p_blk->guard_thread->stat & RT_THREAD_STAT_MASK) == RT_THREAD_INIT)
            rt_thread_startup(p_blk->guard_thread);
    }
    return 0;
}


int api_dfs_block_cache_init(void)
{
    if (!g_blk_cache)
    {
        g_blk_cache = blk_sw_init();
        if (!g_blk_cache)
            rt_kprintf("%s :: init err....\n",__func__);
    }

    return 0;
}

void api_dfs_focus_on_cache_area(void *dfs_handle, rt_uint32_t st_lg_no, rt_uint32_t end_lg_no)
{
    struct s_blk_cache *p_blk = (struct s_blk_cache *)dfs_handle;

    if (!p_blk)
    {
        rt_kprintf("%s :: null handle in......\n",__func__);
        return;
    }

    if (end_lg_no <= st_lg_no)
        DFSBLK_CACHE_ASSERT(0);

    p_blk->focus_st = st_lg_no;
    p_blk->focus_size = end_lg_no - st_lg_no;
}


#include <rttshell.h>
static void runtime_blkcache(int argc, char *argv[])
{
    rt_kprintf("dump runtime info...\n");
    dump_runtime_cache_info(g_blk_cache);
}

SHELL_CMD_EXPORT(runtime_blkcache, runtime_blkcache);
