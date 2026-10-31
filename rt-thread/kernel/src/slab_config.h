#ifndef __slab_config_h__
#define __slab_config_h__

/*don't modify!!!*/
enum {
    FHMEM_TYPE_KERNEL  =  0,
    FHMEM_TYPE_NET     =  1,
    FHMEM_TYPE_JFFS2   =  2,
    FHMEM_TYPE_APP     =  3,
    FHMEM_TYPE_RESERV1 =  4,
    FHMEM_TYPE_RESERV2 =  5,
    FHMEM_TYPE_RESERV3 =  6,
    FHMEM_TYPE_MAX     =  7
};

/*#define RT_USING_MEM_ASAN_CHECK*/  /*是否启用ASAN检查,开启RT_USING_MM_TRACE后有效*/
#ifdef RT_USING_MEM_ASAN_CHECK
#define ASAN_CHECK_POOL_ID  (0) /*检查哪个pool id, just check one pool*/
#endif

/*you can modoify the following array, you can add more pool...*/
static rt_uint32_t g_mem_pool_config[] = {
    0,  /*pool 0 memory size in bytes,使用剩余内存,配置为0即可*/
    /*1*1024*1024,*/ /*pool 1 memory size in bytes*/
    /*2*1024*1024*/  /*pool 2 memory size in bytes*/
};

#define MM_TRACE_NODE_MAX (12000) /*开启RT_USING_MM_TRACE后有效*/

/*#define RT_USING_MEM_FOOT*/     /*控制是否启用跟踪功能,开启RT_USING_MM_TRACE后有效*/
#ifdef RT_USING_MEM_FOOT
#define MEM_FOOT_MM_SIZE (2*1024*1024) /*用于跟踪的内存大小*/
/*
 * 其元素个数必须和g_mem_pool_config保持一致,表示是否跟踪某个pool的malloc/free历史,用于
 * 分析野指针类问题.
 * 1: 跟踪对应pool;  0:不跟踪对应pool
 */
static rt_uint32_t g_mem_pool_foot[] = {
    0,
};
#endif

/*you can modoify the following array*/
static rt_uint32_t g_mem_pool_map[FHMEM_TYPE_MAX] = {
    0, /*pool id of FHMEM_TYPE_KERNEL*/
    0, /*pool id of FHMEM_TYPE_NET*/
    0, /*pool id of FHMEM_TYPE_JFFS2*/
    0, /*pool id of FHMEM_TYPE_APP*/
    0, /*pool id of FHMEM_TYPE_RESERV1*/
    0, /*pool id of FHMEM_TYPE_RESERV2*/
    0, /*pool id of FHMEM_TYPE_RESERV3*/
};

#endif /*__slab_config_h__*/
