/*
 * DMA memory management for framework level HCD code (hc_driver)
 *
 * This implementation plugs in through generic "usb_bus" level methods,
 * and should work with all USB controllers, regardles of bus type.
 */

#include <quirks.h>
#include <hcd.h>
#include <rtthread.h>
#include <rtdef.h>
#include <rtdevice.h>
#include <sys/types.h>
#include "usb_errno.h"
#include <dma_mem.h>

/*
 * DMA-Coherent Buffers
 */

/* FIXME tune these based on pool statistics ... */

#define USB_DMA_BUFFER_DEPTH       8
static const size_t    pool_max[USB_DMA_BUFFER_LENGTH] = {
    /* platforms without dma-friendly caches might need to
     * prevent cacheline sharing...
     */
    32,
    128,
    512,
    1024
    /* bigger --> allocate pages */
};



struct usb_dma_list_node
{
    rt_uint32_t flag;  /* 1,using 0,not in sing */
    void *dma_addr;
    rt_list_t node;
};


/* SETUP primitives */

/**
 * hcd_buffer_create - initialize buffer pools
 * @hcd: the bus whose buffer pools are to be initialized
 * Context: !in_interrupt()
 *
 * Call this as part of initializing a host controller that uses the dma
 * memory allocators.  It initializes some pools of dma-coherent memory that
 * will be shared by all drivers using that controller, or returns a negative
 * errno value on error.
 *
 * Call hcd_buffer_destroy() to clean up after using those pools.
 */
int usb_dma_buffer_create(struct usb_bus *bus)
{
    int i, j, size;
    struct usb_dma_list_node *local_node = RT_NULL;

    for (i = 0; i < USB_DMA_BUFFER_LENGTH; i++)
    {
        size = pool_max[i];
        if (!size)
            continue;
        rt_list_init(&(bus->dma_addr[i]));
        for (j = 0; j < USB_DMA_BUFFER_DEPTH; j++)
        {
            local_node = rt_malloc(sizeof(struct usb_dma_list_node));
            if (local_node == RT_NULL)
                return -1;
            rt_memset(local_node, 0, sizeof(struct usb_dma_list_node));

            local_node->dma_addr = fh_dma_mem_malloc(size);
            if (local_node->dma_addr == RT_NULL)
                return -1;
            rt_list_insert_after(&(bus->dma_addr[i]), &(local_node->node));
        }

    }
    return 0;
}


/* sometimes alloc/free could use kmalloc with GFP_DMA, for
 * better sharing and to leverage mm/slab.c intelligence.
 */

void *usb_dma_buffer_alloc(rt_uint32_t size, struct usb_bus *bus)
{
    int            i;
    struct usb_dma_list_node *local_node = RT_NULL;

    for (i = 0; i < USB_DMA_BUFFER_LENGTH; i++)
    {
        if (size <= pool_max[i])
        {
            list_for_each_entry(local_node, &(bus->dma_addr[i]), node)
            {
                    if (!(local_node->flag))
                    {
                        local_node->flag = 1;
                        return local_node->dma_addr;
                    }
            }
        }
    }
    rt_kprintf("%s dma malloc error", __func__);
    return RT_NULL;
}

int usb_dma_buffer_free(void *addr, struct usb_bus *bus)
{
    int            i;
    struct usb_dma_list_node *local_node = RT_NULL;

    if (!addr)
        return -1;

    for (i = 0; i < USB_DMA_BUFFER_LENGTH; i++)
    {
        list_for_each_entry(local_node, &(bus->dma_addr[i]), node)
        {
            if (local_node->dma_addr == addr)
            {
                local_node->flag = 0;
                return 0;
            }
        }
    }
    rt_kprintf("%s dma free error", __func__);
    return -1;

}
