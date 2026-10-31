/**
 * Copyright (c) 2015-2019 Shanghai Fullhan Microelectronics Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 *
 * Change Logs:
 * Date           Author       Notes
 * 2019-04-12     zhangy       the first version
 */

/*****************************************************************************
 *  Include Section
 *  add all #include here
 *****************************************************************************/
#include "algorithm_core.h"
#include "rtdebug.h"
/*****************************************************************************
 * Define section
 * add all #define here
 *****************************************************************************/
#ifdef ALGO_DEBUG
#define ALGO_DBG(fmt, args...)     \
    do                                  \
    {                                   \
        rt_kprintf("[ALGO]: ");   \
        rt_kprintf(fmt, ## args);       \
    }                                   \
    while (0)
#else
#define ALGO_DBG(fmt, args...)  do { } while (0)
#endif

#define ALGO_ERROR(fmt, args...)     \
    do                                  \
    {                                   \
        rt_kprintf("[ALGO ERROR]: ");   \
        rt_kprintf(fmt, ## args);       \
    }                                   \
    while (0)

#define _ALGO_CONTAINER_LIST_INIT(c, name)     {.type = c, .class_name = name,\
     .list_head.next = &(rt_algo_container[c].list_head), \
     .list_head.prev = &(rt_algo_container[c].list_head)}

#define list_for_each_entry_safe(pos, n, head, member)          \
    for (pos = rt_list_entry((head)->next, typeof(*pos), member),   \
        n = rt_list_entry(pos->member.next, typeof(*pos), member);  \
         &pos->member != (head);                    \
         pos = n, n = rt_list_entry(n->member.next, typeof(*n), member))

/* #define DES_KEY_SIZE      8 */
/* #define DES_EXPKEY_WORDS  32 */
/* #define DES_BLOCK_SIZE        8 */
/*  */
/* #define DES3_EDE_KEY_SIZE     (3 * DES_KEY_SIZE) */
/* #define DES3_EDE_EXPKEY_WORDS (3 * DES_EXPKEY_WORDS) */
/* #define DES3_EDE_BLOCK_SIZE       DES_BLOCK_SIZE */

/****************************************************************************
 * ADT section
 *  add definition of user defined Data Type that only be used in this file  here
 ***************************************************************************/

/******************************************************************************
 * Function prototype section
 * add prototypes for all functions called by this file,execepting those
 * declared in header file
 *****************************************************************************/

/*****************************************************************************
 * Global variables section - Exported
 * add declaration of global variables that will be exported here
 * e.g.
 *  int8_t foo;
 ****************************************************************************/

/*****************************************************************************

 *  static fun;
 *****************************************************************************/

/*****************************************************************************
 * Global variables section - Local
 * define global variables(will be refered only in this file) here,
 * static keyword should be used to limit scope of local variable to this file
 * e.g.
 *  static uint8_t ufoo;
 *****************************************************************************/
/* here is a loose list to save diff algo basic class.. */
struct rt_algo_information rt_algo_container[RT_Algo_Class_Unknown] = {

        _ALGO_CONTAINER_LIST_INIT(RT_Algo_Class_Crypto, "crypto"),
        #ifdef RT_USING_REV

#endif

                };

/* function body */
/*****************************************************************************
 * Description:
 *      add funtion description here
 * Parameters:
 *      description for each argument, new argument starts at new line
 * Return:
 *      what does this function returned?
 *****************************************************************************/

static rt_err_t rt_dev_algo_init(struct rt_device *dev)
{

    return RT_EOK;
}

static rt_err_t rt_dev_algo_open(struct rt_device *dev, rt_uint16_t oflag)
{
    return RT_EOK;
}
/* struct rt_algo_information rt_algo_container[RT_Algo_Class_Unknown] */
static rt_err_t rt_dev_algo_close(struct rt_device *dev)
{

    return RT_EOK;
}

static rt_err_t rt_dev_algo_control(struct rt_device *dev,
        int cmd,
        void *args)
{
#if (0)
    struct rt_dma_device *dma;

    RT_ASSERT(dev != RT_NULL);
    dma = (struct rt_dma_device *)dev;

    /* args is the private data for the soc!! */
    return (dma->ops->control(dma, cmd, args));
#endif
    return (RT_EOK);
}

#ifdef RT_USING_DEVICE_OPS
const static struct rt_device_ops algorithm_ops = {
    .init    = rt_dev_algo_init,
    .open    = rt_dev_algo_open,
    .close   = rt_dev_algo_close,
    .read    = RT_NULL,
    .write   = RT_NULL,
    .control = rt_dev_algo_control
};
#endif

rt_err_t rt_algo_init(void)

{

    rt_uint32_t ret, i;
    struct rt_device *device;

    for (i = 0; i < RT_Algo_Class_Unknown; i++)
    {
        device = &(rt_algo_container[i].parent);
        device->type = RT_Device_Class_Miscellaneous;
        device->rx_indicate = RT_NULL;
        device->tx_complete = RT_NULL;

#ifdef RT_USING_DEVICE_OPS
        device->ops       = &algorithm_ops;
#else
        device->init = rt_dev_algo_init;
        device->open = rt_dev_algo_open;
        device->close = rt_dev_algo_close;
        device->read = RT_NULL;
        device->write = RT_NULL;
        device->control = rt_dev_algo_control;
#endif


        device->user_data = RT_NULL;

        /* register a character device */
        ret = rt_device_register(device, rt_algo_container[i].class_name, RT_IPC_FLAG_FIFO);
        if (ret != RT_EOK)
        {
            break;
        }

        /* return ret; */
    }
    if (i != RT_Algo_Class_Unknown)
    {
        /* add unregister algo device... */

        ALGO_ERROR("register error..\n");
    }

    return ret;
}

#ifdef RT_USING_CRYPTO
static rt_err_t rt_algo_crypto_register(void *obj, const char *obj_name, void *priv)
{
    struct rt_crypto_obj *crypto_obj;
    struct rt_crypto_obj *check_crypto_obj;

    crypto_obj = (struct rt_crypto_obj *) obj;

    /* first check if the name has been register already.. */
    struct rt_list_node *node;

    if ((obj_name == RT_NULL))
        return -RT_EEMPTY;

    for (node = rt_algo_container[RT_Algo_Class_Crypto].list_head.next;
            node != &(rt_algo_container[RT_Algo_Class_Crypto].list_head);
            node = node->next)
    {
        check_crypto_obj = rt_list_entry(node, struct rt_crypto_obj, list);
        if (rt_strncmp(check_crypto_obj->name, obj_name, RT_NAME_MAX) == 0)
        {
            ALGO_ERROR("crypto has the same name algo..\n");
            return -RT_ERROR;

        }
    }

    rt_list_init(&crypto_obj->list);
    crypto_obj->driver_private = priv;
    rt_strncpy(crypto_obj->name, obj_name, RT_NAME_MAX);
    /* here insert "before" and test is "after" will make the list like a fifo... */
    rt_list_insert_before(&rt_algo_container[RT_Algo_Class_Crypto].list_head, &crypto_obj->list);
    return RT_EOK;
}

static struct rt_crypto_obj *rt_algo_crypto_find(char *name)
{

    struct rt_crypto_obj *object;
    struct rt_list_node *node;
    struct rt_algo_information *information = RT_NULL;

    if ((name == RT_NULL))
        return RT_NULL;
    rt_enter_critical();

    /* try to find object */
    if (information == RT_NULL)
        information = &rt_algo_container[RT_Algo_Class_Crypto];
    for (node = information->list_head.next;
            node != &(information->list_head);
            node = node->next)
    {
        object = rt_list_entry(node, struct rt_crypto_obj, list);
        if (rt_strncmp(object->name, name, RT_NAME_MAX) == 0)
        {
            /* leave critical */
            rt_exit_critical();

            return object;
        }
    }

    /* leave critical */
    rt_exit_critical();

    return RT_NULL;

}

static void rt_algo_crypto_check_para(struct rt_crypto_obj *obj_p, struct rt_crypto_request *request_p)
{

    RT_ASSERT(obj_p != RT_NULL);
    RT_ASSERT(request_p != RT_NULL);
    if (obj_p->flag == RT_TRUE)
        RT_ASSERT(request_p->iv != RT_NULL);
    RT_ASSERT(obj_p->max_key_size >= request_p->key_size);
    RT_ASSERT(obj_p->min_key_size <= request_p->key_size);

    RT_ASSERT(((rt_uint32_t)request_p->data_src % obj_p->src_allign_size) == 0);
    RT_ASSERT(((rt_uint32_t)request_p->data_dst % obj_p->dst_allign_size) == 0);
    RT_ASSERT(((rt_uint32_t)request_p->data_size % obj_p->block_size) == 0);

}

rt_err_t rt_algo_crypto_setkey(struct rt_crypto_obj *obj_p, struct rt_crypto_request *request_p)
{

    /* para check... */
    rt_algo_crypto_check_para(obj_p, request_p);
    if (obj_p->ops.set_key)
    {
        obj_p->ops.set_key(obj_p, request_p);
    }

    return RT_EOK;
}

rt_err_t rt_algo_crypto_encrypt(struct rt_crypto_obj *obj_p, struct rt_crypto_request *request_p)
{

    /* para check... */
    rt_algo_crypto_check_para(obj_p, request_p);

    if (obj_p->ops.encrypt)
    {
        obj_p->ops.encrypt(obj_p, request_p);
    }
    return RT_EOK;
}

rt_err_t rt_algo_crypto_decrypt(struct rt_crypto_obj *obj_p, struct rt_crypto_request *request_p)
{

    /* para check... */
    rt_algo_crypto_check_para(obj_p, request_p);

    if (obj_p->ops.decrypt)
    {
        obj_p->ops.decrypt(obj_p, request_p);
    }

    return RT_EOK;
}

#endif

rt_err_t rt_algo_register(rt_uint8_t type, void *obj, const char *obj_name, void *priv)
{

    rt_err_t ret;

    if ((obj_name == RT_NULL) || (type >= RT_Algo_Class_Unknown) || (obj == RT_NULL))
        return -RT_ERROR;

    switch (type)
    {
#ifdef RT_USING_CRYPTO
    case RT_Algo_Class_Crypto:
        ALGO_DBG("'Crypto' register name is %s\n", obj_name);
        ret = rt_algo_crypto_register(obj, obj_name, priv);
        break;
#endif
    default:
        ret = -RT_EEMPTY;
        ALGO_ERROR("please check the type..\n");
    }

    return ret;
}

rt_err_t rt_algo_unregister(rt_uint8_t type, void *obj)
{

    return RT_EOK;

}

void *rt_algo_obj_find(rt_uint8_t type, char *name)
{
    void *obj;
    /* struct rt_crypto_obj *crypto_obj; */
    /* struct rt_list_node *node; */
   /* struct rt_algo_information *information = RT_NULL; */

    if ((name == RT_NULL) || (type >= RT_Algo_Class_Unknown))
        return RT_NULL;

    switch (type)
    {
#ifdef RT_USING_CRYPTO
    case RT_Algo_Class_Crypto:
        ALGO_DBG("'Crypto' find name is %s\n", name);
        obj = rt_algo_crypto_find(name);
        break;
#endif
    default:
        obj = RT_NULL;
        break;

    }
    return obj;
}

