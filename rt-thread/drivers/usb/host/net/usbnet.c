/*****************************************************************************
 *  Include Section
 *  add all #include here
 *****************************************************************************/
#include "usbnet.h"
#include "mmu.h"
#include "fh_def.h"
/*****************************************************************************
 * Define section
 * add all #define here
 *****************************************************************************/
#define USBNET_CACHE_INVALIDATE(addr, size) \
    mmu_invalidate_dcache((rt_uint32_t)addr, size)
#define USBNET_CACHE_WRITEBACK(addr, size) \
    mmu_clean_dcache((rt_uint32_t)addr, size)
#define USBNET_CACHE_WRITEBACK_INVALIDATE(addr, size) \
    mmu_clean_invalidated_dcache((rt_uint32_t)addr, size)

/* USBNET has BD's in cached memory - so need cache functions */
#define BD_CACHE_INVALIDATE(addr, size) \
    mmu_invalidate_dcache((rt_uint32_t)addr, size)
#define BD_CACHE_WRITEBACK(addr, size) mmu_clean_dcache((rt_uint32_t)addr, size)

extern void mmu_clean_invalidated_dcache(rt_uint32_t buffer, rt_uint32_t size);
#define BD_CACHE_WRITEBACK_INVALIDATE(addr, size) \
    mmu_clean_invalidated_dcache((rt_uint32_t)addr, size)

#define list_for_each_entry_safe(pos, n, head, member)                \
    for (pos = rt_list_entry((head)->next, typeof(*pos), member),     \
        n    = rt_list_entry(pos->member.next, typeof(*pos), member); \
         &pos->member != (head);                                      \
         pos = n, n = rt_list_entry(n->member.next, typeof(*n), member))


#define JEDEC_MFR(info)    ((info)->id[0])

#define USBNET_SET_STATUS(p_usbnet, status) p_usbnet->usbnet_status =  status
#define USBNET_GET_STATUS(p_usbnet) (p_usbnet->usbnet_status)
/****************************************************************************
 * ADT section
 *  add definition of user defined Data Type that only be used in this file here
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
static struct usbnet_rx_node *get_one_rx_node(struct usbnet_rx_node *p_node);
static int put_one_rx_node(struct usbnet_rx_node *insert_node, struct usbnet_rx_node *head_node);
static int usbnet_init_self_mem(struct usbnet *p_usbnet);
static struct usbnet_self_buff_desc *usbnet_get_rx_buf(struct usbnet *p_usbnet);
static struct usbnet_self_buff_desc *usbnet_get_tx_buf(struct usbnet *p_usbnet);
/* static void usbnet_put_rx_buf(struct usbnet_self_buff_desc *desc); */
static void usbnet_put_tx_buf(struct usbnet *p_usbnet, struct usbnet_self_buff_desc *desc);
static struct usbnet *private_from_net_to_usbnet(struct net_device *p_eth);
static int usbnet_check_rx_done_list(struct usbnet *p_usbnet);
static int usbnet_check_rx_idle_list(struct usbnet *p_usbnet);
static int usbnet_rx_idle_list_xfer_init(struct usbnet *p_usbnet);
static int usbnet_rx_xfer_data_to_usb(struct usbnet *p_usbnet);
static rt_err_t rtt_interface_usbnet_control(rt_device_t dev, int cmd, void *args);
/*****************************************************************************
 * Global variables section - Local
 * define global variables(will be refered only in this file) here,
 * static keyword should be used to limit scope of local variable to this file
 * e.g.
 *  static uint8_t ufoo;
 *****************************************************************************/

/* debug data; */
static struct usbnet *g_test_usbnet;
static int _rx_idle_times = 0;
static int _rx_done_times = 0;
static int call_eth_times = 0;

/* function body */
/*****************************************************************************
 * Description:
 *      add funtion description here
 * Parameters:
 *      description for each argument, new argument starts at new line
 * Return:
 *      what does this function returned?
 *****************************************************************************/
void dump_usb_rx_node_info(struct usbnet_rx_node *node)
{

    rt_list_t *p_list;
    struct usbnet_rx_node *idle_node;
    struct usbnet_rx_node *_idle_node;

    p_list = &node->list;
    list_for_each_entry_safe(idle_node, _idle_node, p_list, list)
    {
        rt_kprintf("node add is %x\n", (int) idle_node);
    }
}

void dump_usb_rx_desc(struct usbnet *p_usbnet)
{
    struct usbnet_self_buff_desc *p_desc;
    int i;

    rt_kprintf("%s\n", __func__);
    p_desc = p_usbnet->rxbuff_desc;
    for (i = 0; i < USBNET_RX_RING_SIZE; i++, p_desc++)
    {
        rt_kprintf("desc%d\tadd:%08x\n", i, (unsigned int)p_desc);
        rt_kprintf("flag:%08x  buff add:%08x\n", p_desc->mem_flag, p_desc->buff_add);
        rt_kprintf("\n");
    }
    rt_kprintf("%s : soft current desc is:%d\n", __func__, p_usbnet->rx_allign_index);
}

void dump_usb_tx_desc(struct usbnet *p_usbnet)
{
    struct usbnet_self_buff_desc *p_desc;
    int i;

    rt_kprintf("%s\n", __func__);
    p_desc = p_usbnet->txbuff_desc;
    for (i = 0; i < USBNET_TX_RING_SIZE; i++, p_desc++)
    {
        rt_kprintf("desc%d\tadd:%08x\n", i, (unsigned int)p_desc);
        rt_kprintf("flag:%08x  buff add:%08x\n", p_desc->mem_flag, p_desc->buff_add);
        rt_kprintf("\n");
    }
    rt_kprintf("%s : soft current desc is:%d\n", __func__, p_usbnet->tx_allign_index);
}

static void intr_complete(struct urb *urb)
{
    struct usbnet *p_usbnet = urb->context;
    rt_sem_release(&p_usbnet->sem_interrupt);
}

static int init_status(struct usbnet *dev, struct usb_interface *intf)
{
    void *buf = NULL;
    unsigned int pipe = 0;
    unsigned int maxp;
    unsigned int period;
    int ret;

    if (!dev->driver_info->status)
        return 0;

    pipe = usb_rcvintpipe(dev->udev,
    dev->status->desc.bEndpointAddress
    & USB_ENDPOINT_NUMBER_MASK);
    maxp = usb_maxpacket(dev->udev, pipe, 0);
    /* avoid 1 msec chatter:  min 8 msec poll rate */
    period = MAX((int)dev->status->desc.bInterval,
    (dev->udev->speed == USB_SPEED_HIGH) ? 7 : 3);

    if ((!dev->interrupt_raw_buf) && (!dev->interrupt))
    {
        buf = rt_malloc(maxp + CACHE_LINE_SIZE);
        if (!buf)
            return -ENOMEM;
        dev->interrupt = usb_alloc_urb(0, 0);
        if (!dev->interrupt)
        {
            rt_kprintf("malloc urb error.%d\n", __LINE__);
            rt_free(buf);
            return -ENOMEM;
        }
        dev->interrupt_raw_buf = (rt_uint8_t *)buf;
        dev->interrupt_allign_buf = (rt_uint8_t *) (((unsigned int)buf + CACHE_LINE_SIZE - 1)
                & (~(CACHE_LINE_SIZE - 1)));
    }

    usb_fill_int_urb(dev->interrupt, dev->udev, pipe,
        dev->interrupt_allign_buf, maxp, intr_complete, dev, period);
    ret = usb_submit_urb(dev->interrupt, 0);
    if (ret)
        rt_kprintf("usbnet init status urb submit failed %d\n", ret);
    return 0;
}

int usbnet_get_endpoints(struct usbnet *dev, struct usb_interface *intf)
{
    int                tmp;
    struct usb_host_interface    *alt = NULL;
    struct usb_host_endpoint    *in = NULL, *out = NULL;
    struct usb_host_endpoint    *status = NULL;

    for (tmp = 0; tmp < intf->num_altsetting; tmp++)
    {
        unsigned int ep;

        in = out = status = NULL;
        alt = intf->altsetting + tmp;

        /* take the first altsetting with in-bulk + out-bulk;
         * remember any status endpoint, just in case;
         * ignore other endpoints and altsettings.
         */
        for (ep = 0; ep < alt->desc.bNumEndpoints; ep++)
        {
            struct usb_host_endpoint    *e;
            int                intr = 0;

            e = alt->endpoint + ep;
            switch (e->desc.bmAttributes)
            {
            case USB_ENDPOINT_XFER_INT:
                if (!usb_endpoint_dir_in(&e->desc))
                    continue;
                intr = 1;
                /* FALLTHROUGH */
            case USB_ENDPOINT_XFER_BULK:
                break;
            default:
                continue;
            }
            if (usb_endpoint_dir_in(&e->desc))
            {
                if (!intr && !in)
                    in = e;
                else if (intr && !status)
                    status = e;
            }
            else
            {
                if (!out)
                    out = e;
            }
        }
        if (in && out)
            break;
    }
    if (!alt || !in || !out)
        return -EINVAL;

    if (alt->desc.bAlternateSetting != 0 ||
        !(dev->driver_info->flags & FLAG_NO_SETINT)) {
        tmp = usb_set_interface(dev->udev, alt->desc.bInterfaceNumber,
                alt->desc.bAlternateSetting);
        if (tmp < 0)
            return tmp;
    }

    dev->in = usb_rcvbulkpipe(dev->udev,
            in->desc.bEndpointAddress & USB_ENDPOINT_NUMBER_MASK);
    dev->out = usb_sndbulkpipe(dev->udev,
            out->desc.bEndpointAddress & USB_ENDPOINT_NUMBER_MASK);
    dev->status = status;
    return 0;
}

int usbnet_get_ethernet_addr(struct usbnet *dev, int iMACAddress)
{
    int 		tmp = -1, ret;
    unsigned char	buf [13];

    ret = usb_string(dev->udev, iMACAddress, (char *)buf, sizeof buf);
    if (ret == 12)
    {
        int i;
        char ch, number;
        unsigned char *str = buf;

        for (i = 0; i < 6; i++)
        {
            ch = *str++;
            if (ch < 0x20)
                break;
            ch |= 0x20;
            if ((ch < '0' || ch > '9') && (ch < 'a' || ch > 'f'))
                break;
            number = !(ch > '9') ? (ch - '0') : (ch - 'a' + 10);
            ch = *str++;
            ch |= 0x20;
            if ((ch < '0' || ch > '9') && (ch < 'a' || ch > 'f'))
                break;
            number = (number << 4) + (!(ch > '9') ? (ch - '0') : (ch - 'a' + 10));
            dev->usbnet_mac_add[i] = number;
        }
        tmp = i == 6 ? 1 : -1;
    }

    if (tmp < 0) {
        rt_kprintf("bad MAC string %d fetch, %d\n",
                iMACAddress, tmp);
        if (ret >= 0)
            ret = -EINVAL;
        return ret;
    }
    return 0;
}
static struct usbnet_rx_node *get_one_rx_node(struct usbnet_rx_node *p_node)
{
    struct usbnet_rx_node *node;

    register rt_base_t temp;

    temp = rt_hw_interrupt_disable();
    if (rt_list_isempty(&p_node->list))
    {
        rt_hw_interrupt_enable(temp);
        return RT_NULL;
    }
    node = rt_list_entry(p_node->list.next, struct usbnet_rx_node, list);
    rt_list_remove(&node->list);
    rt_hw_interrupt_enable(temp);

    return node;
}

static int put_one_rx_node(struct usbnet_rx_node *insert_node,
                struct usbnet_rx_node *head_node)
{
    register rt_base_t temp;

    temp = rt_hw_interrupt_disable();
    rt_list_init(&insert_node->list);
    rt_list_insert_before(&head_node->list, &insert_node->list);
    rt_hw_interrupt_enable(temp);
    return 0;
}

int usbnet_put_one_pbuf(struct usbnet *p_usbnet, struct pbuf *p_buf, struct usbnet_pubuf_list *head_node)
{
    register rt_base_t temp;
    struct usbnet_pubuf_list *insert_node;
    temp = rt_hw_interrupt_disable();
    insert_node = rt_malloc(sizeof(struct usbnet_pubuf_list));
    if (!insert_node)
    {
        rt_kprintf("[%s : %d] malloc failed ...\n");
        USB_NET_ASSERT(insert_node != RT_NULL);
    }
    rt_list_init(&insert_node->list);
    insert_node->p_buf = p_buf;
    rt_list_insert_before(&head_node->list, &insert_node->list);
    rt_hw_interrupt_enable(temp);
    return 0;
}

struct pbuf *usbnet_get_one_pbuf(struct usbnet_pubuf_list *p_node)
{
    struct usbnet_pubuf_list *node;
    register rt_base_t temp;
    struct pbuf *r_pbuf;
    temp = rt_hw_interrupt_disable();
    if (rt_list_isempty(&p_node->list))
    {
        rt_hw_interrupt_enable(temp);
        return RT_NULL;
    }
    node = rt_list_entry(p_node->list.next, struct usbnet_pubuf_list, list);
    rt_list_remove(&node->list);
    r_pbuf = node->p_buf;
    node->p_buf = RT_NULL;
    rt_free(node);
    rt_hw_interrupt_enable(temp);

    return r_pbuf;
}

void usbnet_free_self_mem(struct usbnet *p_usbnet)
{
/* TBD */
    rt_free(p_usbnet->tx_allign_raw_buff);
    rt_free(p_usbnet->rx_allign_raw_buff);
    rt_free(p_usbnet->txbuff_desc);
    rt_free(p_usbnet->rxbuff_desc);
    p_usbnet->tx_allign_raw_buff = RT_NULL;
    p_usbnet->rx_allign_raw_buff = RT_NULL;
    p_usbnet->tx_allign_buff = RT_NULL;
    p_usbnet->rx_allign_buff = RT_NULL;
    p_usbnet->txbuff_desc = RT_NULL;
    p_usbnet->rxbuff_desc = RT_NULL;

}

static int usbnet_init_self_mem(struct usbnet *p_usbnet)
{
    struct urb *urb;
    int i, j;
    rt_uint32_t t1;
    rt_uint32_t t2;
    rt_uint32_t t3;
    rt_uint32_t t4;

    struct usbnet_self_buff_desc *p_desc;
    struct usbnet_rx_node *node;
    int status = 0;

    if (USBNET_RX_URB_SIZE < USBNET_RX_RING_SIZE)
    {
        rt_kprintf("urb size must larger than rx ring size...\n");
        USB_NET_ASSERT(USBNET_RX_URB_SIZE > USBNET_RX_RING_SIZE);
    }
    /* t1 */
    t1 = (rt_uint32_t) rt_malloc(
    USBNET_TX_RING_SIZE * sizeof(struct usbnet_self_buff_desc));
    if (!t1)
    {
        status = -ENOMEM;
        goto out1;
    }
    rt_memset((void *) t1, 0,
    USBNET_TX_RING_SIZE * sizeof(struct usbnet_self_buff_desc));

    /* t2 */
    t2 = (rt_uint32_t) rt_malloc(
    USBNET_TX_RING_SIZE * USBNET_TX_BUFFER_SIZE + CACHE_LINE_SIZE);
    if (!t2)
    {
        rt_free((void *) t1);
        status = -ENOMEM;
        goto out1;
    }
    rt_memset((void *) t2, 0,
    USBNET_TX_RING_SIZE * USBNET_TX_BUFFER_SIZE + CACHE_LINE_SIZE);

    /* t3 */
    t3 = (rt_uint32_t) rt_malloc(
    USBNET_RX_RING_SIZE * sizeof(struct usbnet_self_buff_desc));

    if (!t3)
    {
        status = -ENOMEM;
        rt_free((void *) t1);
        rt_free((void *) t2);
        goto out1;
    }
    rt_memset((void *) t3, 0,
    USBNET_RX_RING_SIZE * sizeof(struct usbnet_self_buff_desc));

    /* t4 */
    t4 = (rt_uint32_t) rt_malloc(
    USBNET_RX_RING_SIZE * USBNET_RX_BUFFER_SIZE + CACHE_LINE_SIZE);
    if (!t4)
    {
        rt_free((void *) t1);
        rt_free((void *) t2);
        rt_free((void *) t3);
        status = -ENOMEM;
        goto out1;
    }

    rt_memset((void *) t4, 0,
    USBNET_RX_RING_SIZE * USBNET_RX_BUFFER_SIZE + CACHE_LINE_SIZE);

    p_usbnet->tx_allign_buff = (rt_uint8_t *) ((t2 + CACHE_LINE_SIZE - 1)
                    & (~(CACHE_LINE_SIZE - 1)));
    p_usbnet->tx_allign_raw_buff = (rt_uint8_t *) t2;

    p_usbnet->rx_allign_buff = (rt_uint8_t *) ((t4 + CACHE_LINE_SIZE - 1)
                    & (~(CACHE_LINE_SIZE - 1)));
    p_usbnet->rx_allign_raw_buff = (rt_uint8_t *) t4;

    p_usbnet->txbuff_desc = (struct usbnet_self_buff_desc *) t1;
    p_usbnet->rxbuff_desc = (struct usbnet_self_buff_desc *) t3;
    p_usbnet->rx_allign_buff_size = USBNET_RX_BUFFER_SIZE;
    p_usbnet->tx_allign_buff_size = USBNET_TX_BUFFER_SIZE;
    p_usbnet->rx_allign_index = 0;
    p_usbnet->tx_allign_index = 0;

    p_desc = p_usbnet->txbuff_desc;
    for (i = 0; i < USBNET_TX_RING_SIZE; i++, p_desc++)
    {
        p_desc->mem_flag = USBNET_MEM_IDLE;
        p_desc->buff_add = (rt_uint8_t *) (p_usbnet->tx_allign_buff
                        + i * USBNET_TX_BUFFER_SIZE);
        p_desc->status = 0;
        p_desc->p_usbnet = p_usbnet;
    }

    p_desc = p_usbnet->rxbuff_desc;
    for (i = 0; i < USBNET_RX_RING_SIZE; i++, p_desc++)
    {
        p_desc->mem_flag = USBNET_MEM_IDLE;
        p_desc->buff_add = (rt_uint8_t *) (p_usbnet->rx_allign_buff
                        + i * USBNET_RX_BUFFER_SIZE);
        p_desc->status = 0;
        p_desc->p_usbnet = p_usbnet;
    }

    rt_list_init(&p_usbnet->rx_idle_node_head.list);
    rt_list_init(&p_usbnet->rx_done_node_head.list);
    /* head list nerver use the para below... */
    p_usbnet->rx_idle_node_head.p_buf_desc = RT_NULL;
    p_usbnet->rx_idle_node_head.urb = RT_NULL;
    p_usbnet->rx_done_node_head.p_buf_desc = RT_NULL;
    p_usbnet->rx_done_node_head.urb = RT_NULL;

    rt_kprintf("rx idle head add :%x,done add: %x\n",
                    &p_usbnet->rx_idle_node_head,
                    &p_usbnet->rx_done_node_head);

    for (i = 0, j = 0; i < USBNET_RX_URB_SIZE; i++, j++)
    {
        urb = usb_alloc_urb(0, 0);
        if (!urb)
        {
            rt_kprintf("malloc urb failed...\n");
            USB_NET_ASSERT(urb != RT_NULL);
            goto out2;
        }

        /* malloc rx node... */
        node = rt_malloc(sizeof(struct usbnet_rx_node));
        if (!node)
        {
            USB_NET_ASSERT(node != RT_NULL);
            /* free current urb.. */
            usb_free_urb(urb);
            goto out2;
        }
        rt_memset(node, 0, sizeof(struct usbnet_rx_node));
        /* if i wanna free mem from usb core..just check the array below. */
        p_usbnet->rx_urb_array[i] = urb;
        rt_list_init(&node->list);
        rt_list_init(&node->list_done_pbuf.list);
        /* head list nerver use the para below... */
        node->list_done_pbuf.p_buf = RT_NULL;
        node->p_buf_desc = usbnet_get_rx_buf(p_usbnet);
        node->urb = urb;
        /* put one urb to the idle list.. */
        /* put all the rx node to the idle list.. */
        put_one_rx_node(node, &p_usbnet->rx_idle_node_head);
    }
    /* dump_usb_rx_node_info(&p_usbnet->rx_idle_node_head); */

    return 0;
out2:

    for (i = 0; i < j; i++)
    {
        urb = p_usbnet->rx_urb_array[i];
        usb_free_urb(urb);
        node = get_one_rx_node(&p_usbnet->rx_idle_node_head);
        if (!node)
            continue;
        rt_free(node);
    }
    usbnet_free_self_mem(p_usbnet);
    status = -ENOMEM;

out1:
    return status;

}


static struct usbnet_self_buff_desc *usbnet_get_rx_buf(struct usbnet *p_usbnet)
{
    struct usbnet_self_buff_desc *p_desc;

    p_desc = &p_usbnet->rxbuff_desc[p_usbnet->rx_allign_index];
    if (p_desc->mem_flag == USBNET_MEM_BUSY)
    {
        return RT_NULL;
    }
    p_desc->mem_flag  = USBNET_MEM_BUSY;
    p_usbnet->rx_allign_index++;
    p_usbnet->rx_allign_index %= USBNET_RX_RING_SIZE;
    return p_desc;
}


static struct usbnet_self_buff_desc *usbnet_get_tx_buf(struct usbnet *p_usbnet)
{
    struct usbnet_self_buff_desc *p_desc = RT_NULL;

    if (rt_sem_take(&p_usbnet->sem_usbnet_tx,
                        RT_WAITING_FOREVER) == RT_EOK)
    {
        p_desc = &p_usbnet->txbuff_desc[p_usbnet->tx_allign_index];
        if (p_desc->mem_flag == USBNET_MEM_BUSY)
        {
            return RT_NULL;
        }
        p_desc->mem_flag  = USBNET_MEM_BUSY;
        p_usbnet->tx_allign_index++;
        p_usbnet->tx_allign_index %= USBNET_TX_RING_SIZE;
    }
    return p_desc;
}

static void usbnet_put_tx_buf(struct usbnet *p_usbnet, struct usbnet_self_buff_desc *desc)
{
    struct usbnet_self_buff_desc *p_desc;

    p_desc = desc;
    p_desc->mem_flag = USBNET_MEM_IDLE;
    p_desc->urb = NULL;
    rt_sem_release(&p_usbnet->sem_usbnet_tx);
}

static struct usbnet *private_from_net_to_usbnet(struct net_device *p_eth)
{
    struct usbnet *p_usbnet;

    if (!p_eth)
    {
        return RT_NULL;
    }
    p_usbnet = rt_list_entry(p_eth, struct usbnet, rtt_eth_interface);
    return p_usbnet;
}

static rt_err_t rtt_interface_usbnet_init(rt_device_t dev)
{
    struct net_device *p_eth;
    struct usbnet *p_usbnet;

    p_eth = (struct net_device *)dev;
    p_usbnet = private_from_net_to_usbnet(p_eth);
    usbnet_rx_idle_list_xfer_init(p_usbnet);
    return RT_EOK;
}

void rtt_interface_usbnet_tx_callback(struct urb *urb)
{

    struct usbnet_self_buff_desc *p_desc;
    struct usbnet *p_usbnet;

    p_desc = urb->context;
    p_usbnet = p_desc->p_usbnet;
    /* do not free mem here...send one message to the tx thread..TBD */
/* usbnet_put_tx_buf(p_desc); */
/* usb_free_urb(urb); */
    /* rt_kprintf("call back urb add :0x%x\n",(int)urb); */
    rt_mb_send(&p_usbnet->usb_net_tx_mb, (rt_uint32_t)urb);
}



void rtt_interface_usbnet_rx_callback(struct urb *urb)
{
    struct usbnet *p_usbnet;
    struct usbnet_rx_node *rx_node;

    rx_node = (struct usbnet_rx_node *)urb->context;
    p_usbnet = rx_node->p_buf_desc->p_usbnet;
    rx_node->p_buf_desc->usb_in_active_len = urb->actual_length;
    rx_node->p_buf_desc->status = urb->status;
    _rx_done_times++;
    if (rx_node->p_buf_desc->status != 0)
    {
        put_one_rx_node(rx_node, &p_usbnet->rx_idle_node_head);
        usbnet_check_rx_idle_list(p_usbnet);
        return;
    }
    BD_CACHE_WRITEBACK_INVALIDATE((unsigned int)rx_node->p_buf_desc->buff_add, rx_node->p_buf_desc->usb_in_active_len);
    /* just put to done list.... */
    put_one_rx_node(rx_node, &p_usbnet->rx_done_node_head);
    usbnet_check_rx_done_list(p_usbnet);

}

static int usbnet_check_rx_done_list(struct usbnet *p_usbnet)
{

    int ret;

    if (rt_list_isempty(&p_usbnet->rx_done_node_head.list))
    {
        return -1;
    }
    /* wake up lwip to call rx to receive data.. */
    ret = net_device_ready(&(p_usbnet->rtt_eth_interface));
    if (ret != RT_EOK)
    {
        rt_kprintf("net_device_ready error:0x%x\n", ret);
        return -2;
    }
    return 0;
}


static int usbnet_rx_idle_list_xfer_init(struct usbnet *p_usbnet)
{

    int ret;

    do
    {
        ret = usbnet_rx_xfer_data_to_usb(p_usbnet);
    }while (ret == 0);
    return 0;
}

static int usbnet_check_rx_idle_list(struct usbnet *p_usbnet)
{
    rt_sem_release(&p_usbnet->sem_rx_idle_list);
    return 0;
}

static rt_err_t rtt_interface_usbnet_open(rt_device_t dev, uint16_t oflag)
{
    rt_kprintf("% s\n", __func__);
    return RT_EOK;
}

/* Close the interface */
static rt_err_t rtt_interface_usbnet_close(rt_device_t dev)
{
    rt_kprintf("% s\n", __func__);
    return RT_EOK;
}

/* Read */
static rt_size_t rtt_interface_usbnet_read(rt_device_t dev, rt_off_t pos, void *buffer, rt_size_t size)
{
    rt_kprintf("% s\n", __func__);
    rt_set_errno(RT_ENOSYS);
    return RT_EOK;
}

/* Write */
static rt_size_t rtt_interface_usbnet_write(rt_device_t dev, rt_off_t pos, const void *buffer, rt_size_t size)
{
    rt_kprintf("% s\n", __func__);
    rt_set_errno(RT_ENOSYS);
    return 0;
}

static rt_err_t rtt_interface_usbnet_control(rt_device_t dev, int cmd, void *args)
{

    rt_kprintf("% s\n", __func__);
    struct net_device *p_eth;
    struct usbnet *p_usbnet;

    p_eth = (struct net_device *) dev;
    p_usbnet = private_from_net_to_usbnet(p_eth);

    switch (cmd)
    {
    case NIOCTL_GADDR:
        /* get mac address */
        if (args)
        {
            rt_memcpy(args, p_usbnet->usbnet_mac_add, 6);
        }
        else
        {
            return -RT_ERROR;
        }

        break;

    default:
        break;
    }

    return RT_EOK;
}


static rt_err_t rtt_interface_usbnet_tx(rt_device_t dev, struct pbuf *p)
{
    struct net_device *p_eth;
    struct usbnet *p_usbnet;
    unsigned char *tx_buf;
    unsigned int status;
    unsigned int size;
    struct usbnet_self_buff_desc *p_desc;
    struct urb *urb;
    int retry = 3;
    int ret = 0;

    p_eth = (struct net_device *) dev;
    p_usbnet = private_from_net_to_usbnet(p_eth);
    rt_mutex_take(&(p_usbnet->tx_free_mutex), RT_WAITING_FOREVER);

    if (!p_usbnet->udev)
    {
        rt_mutex_release(&(p_usbnet->tx_free_mutex));
        return -1;
    }

    /* call the driver to check if data is too large to send... */
    /* add check p_buf if right.. */
    /* get buff to send.. */
    p_desc = usbnet_get_tx_buf(p_usbnet);
    if (!p_desc)
    {
        rt_kprintf("[%s-%d]: no tx buf\n", __func__, __LINE__);
        rt_mutex_release(&(p_usbnet->tx_free_mutex));
        return -1;
    }
    tx_buf = p_desc->buff_add;
    if (p_usbnet->driver_info->tx_fixup)
    {
        tx_buf = p_usbnet->driver_info->tx_fixup(p_usbnet, p, tx_buf,
                        p_usbnet->tx_allign_buff_size, &status, &size);
        if (status != 0)
        {
            rt_kprintf("driver process tx data error...\n");
            usbnet_put_tx_buf(p_usbnet, p_desc);
            rt_mutex_release(&(p_usbnet->tx_free_mutex));
            return -1;
        }
    }

    urb = usb_alloc_urb(0, 0);
    if (!urb)
    {
        rt_kprintf("%s no urb..\n", __func__);
        usbnet_put_tx_buf(p_usbnet, p_desc);
        rt_mutex_release(&(p_usbnet->tx_free_mutex));
        return -1;
    }
    p_desc->urb = urb;
    BD_CACHE_WRITEBACK_INVALIDATE((unsigned int)tx_buf, size);
    usb_fill_bulk_urb(urb, p_usbnet->udev, p_usbnet->out, tx_buf, size,
                    rtt_interface_usbnet_tx_callback, p_desc);

    while (retry-- > 0)
    {
        ret = usb_submit_urb(urb, 0);
        if (ret != -ENOMEM || !retry)
            break;
        rt_thread_delay(2);
    }
    if (ret != 0)
    {
        usbnet_put_tx_buf(p_usbnet, p_desc);
        usb_free_urb(urb);
    }

    rt_mutex_release(&(p_usbnet->tx_free_mutex));
    return RT_EOK;
}

static struct pbuf *rtt_interface_usbnet_rx(rt_device_t dev)
{
    struct pbuf *temp_pbuf = RT_NULL;
    struct net_device *p_eth;
    struct usbnet *p_usbnet;
    struct usbnet_rx_node *rx_done_node_head;
    struct usbnet_rx_node *rx_done_node;
    int driver_status;
    p_eth = (struct net_device *) dev;
    p_usbnet = private_from_net_to_usbnet(p_eth);
    rx_done_node_head = &p_usbnet->rx_done_node_head;
    /* if pre active node is none ..then get one node first.. */
    if (!p_usbnet->active_node)
    {
        /* get one node.. */
        if (rt_list_isempty(&rx_done_node_head->list))
        {
            return RT_NULL;
        }
        rx_done_node = get_one_rx_node(rx_done_node_head);
        if (rx_done_node == RT_NULL)
        {
            return RT_NULL;
        }
        p_usbnet->active_node = rx_done_node;
        /* parse the usb data */
        if (p_usbnet->driver_info->rx_fixup)
        {
            driver_status = p_usbnet->driver_info->rx_fixup(
                    p_usbnet,
                    p_usbnet->active_node->p_buf_desc->buff_add,
                    p_usbnet->active_node->p_buf_desc->usb_in_active_len,
                    p_usbnet->active_node);
            if (driver_status != 0)
            {
                put_one_rx_node(p_usbnet->active_node, &p_usbnet->rx_idle_node_head);
                p_usbnet->active_node = RT_NULL;
                usbnet_check_rx_idle_list(p_usbnet);
                return RT_NULL;
            }
        }
    }
    if (p_usbnet->active_node->rx_state_mac == GET_ACTIVE_NODE_PBUFF)
    {
        temp_pbuf = usbnet_get_one_pbuf(&p_usbnet->active_node->list_done_pbuf);
        if (!temp_pbuf)
        {
            p_usbnet->active_node->rx_state_mac = GET_NODE_DONE;
        }
    }
    if (p_usbnet->active_node->rx_state_mac == GET_NODE_DONE)
    {
        put_one_rx_node(p_usbnet->active_node, &p_usbnet->rx_idle_node_head);
        p_usbnet->active_node = RT_NULL;
        usbnet_check_rx_idle_list(p_usbnet);
    }
    return temp_pbuf;
}


static int usbnet_rx_xfer_data_to_usb(struct usbnet *p_usbnet)
{
    struct urb *urb;
    struct usbnet_rx_node *idle_node = RT_NULL;
    int ret;
    /* rt_mutex_take(&p_usbnet->rx_idle_node_mutex,RT_WAITING_FOREVER); */
    idle_node = get_one_rx_node(&p_usbnet->rx_idle_node_head);
    /* rt_mutex_release(&p_usbnet->rx_idle_node_mutex); */
    if (!idle_node)
    {
        return -1;
    }
    urb = idle_node->urb;
    usb_init_urb(urb);
    usb_fill_bulk_urb(urb, p_usbnet->udev, p_usbnet->in,
            idle_node->p_buf_desc->buff_add,
            p_usbnet->rx_allign_buff_size,
            rtt_interface_usbnet_rx_callback,
            idle_node);
    ret = usb_submit_urb(urb, 0);
    if (ret != 0)
        rt_mb_send(&p_usbnet->rx_err_mb, (rt_uint32_t)idle_node);

    _rx_idle_times++;
    return 0;
}

void usbnet_thread_check_rx_list(void *_p_usbnet)
{
    int ret;
    struct usbnet *p_usbnet = (struct usbnet *) _p_usbnet;

    while (1)
    {
        if (rt_sem_take(&p_usbnet->sem_rx_idle_list,
                        RT_WAITING_FOREVER) == RT_EOK) {
            do
            {
                ret = usbnet_rx_xfer_data_to_usb(p_usbnet);
            } while (ret == 0);
        } else
            rt_kprintf("%s---%d p_usbnet->sem_rx_idle_list error\n", __func__, __LINE__);
    }
}


void usbnet_tx_reconnet_prc(struct usbnet *_p_usbnet)
{
    struct usbnet *p_usbnet = (struct usbnet *) _p_usbnet;
    struct urb *urb;
    struct usbnet_self_buff_desc *p_desc;

    while (1)
    {
        if (rt_mb_recv(&p_usbnet->tx_err_mb, (rt_uint32_t *) &urb, 0) == RT_EOK)
        {
            p_desc = urb->context;
            usbnet_put_tx_buf(p_usbnet, p_desc);
            usb_free_urb(urb);
        }
        else
            break;
    }

}

void usbnet_rx_reconnet_prc(struct usbnet *_p_usbnet)
{
    struct usbnet *p_usbnet = (struct usbnet *) _p_usbnet;
    struct usbnet_rx_node *idle_node;

    while (1)
    {
        if (rt_mb_recv(&p_usbnet->rx_err_mb, (rt_uint32_t *) &idle_node, 0) == RT_EOK)
        {
            put_one_rx_node(idle_node, &p_usbnet->rx_idle_node_head);
            usbnet_check_rx_idle_list(p_usbnet);
        }
        else
            break;
    }


}

void usbnet_int_reconnet_prc(struct usbnet *_p_usbnet)
{
    struct usbnet *p_usbnet = (struct usbnet *) _p_usbnet;
    struct urb *urb;

    while (1)
    {
        if (rt_mb_recv(&p_usbnet->int_err_mb, (rt_uint32_t *) &urb, 0) == RT_EOK)
        {
            init_status(p_usbnet, p_usbnet->intf);
        }
        else
            break;
    }
}

void usbnet_thread_reconnet_prc(void *_p_usbnet)
{
    struct usbnet *p_usbnet = (struct usbnet *) _p_usbnet;

    while (1)
    {
        if (rt_sem_take(&p_usbnet->sem_reconnet,
                        RT_WAITING_FOREVER) == RT_EOK) {
            /* parse all the err */
            usbnet_tx_reconnet_prc(p_usbnet);
            usbnet_rx_reconnet_prc(p_usbnet);
            usbnet_int_reconnet_prc(p_usbnet);
        }
    }
}

void usbnet_thread_interrupt_packet_process(void *_p_usbnet)
{
    struct usbnet *p_usbnet = (struct usbnet *) _p_usbnet;
    struct usbnet *dev = p_usbnet;
    int status;
    struct urb *urb = p_usbnet->interrupt;

    while (1)
    {
        if (rt_sem_take(&p_usbnet->sem_interrupt, RT_WAITING_FOREVER) == RT_EOK)
        {
        	status = urb->status;
            switch (status)
            {
            /* success */
            case 0:
                BD_CACHE_WRITEBACK_INVALIDATE(urb->transfer_buffer, urb->transfer_buffer_length);
                dev->driver_info->status(dev, urb);
                break;

            /* software-driven interface shutdown */
            case -ENOENT:    /* urb killed */
            case -ESHUTDOWN:    /* hardware gone */
			case -ETIMEDOUT:
                rt_kprintf("intr shutdown, code %d\n", status);
				rt_mb_send(&p_usbnet->int_err_mb, (rt_uint32_t)urb);
                continue;

            /* NOTE:  not throttling like RX/TX, since this endpoint
            * already polls infrequently
            */
            default:
                rt_kprintf("intr status %d\n", status);
                break;
            }
            memset(urb->transfer_buffer, 0, urb->transfer_buffer_length);
            urb->status = 0;
            status = usb_submit_urb(urb, 0);
            if (status != 0)
            {
                rt_mb_send(&p_usbnet->int_err_mb, (rt_uint32_t)urb);
                rt_kprintf("intr resubmit --> %d\n", status);
            }

        }
    }
}

void usbnet_thread_check_tx_list(void *_p_usbnet)
{
    struct usbnet *p_usbnet = (struct usbnet *) _p_usbnet;
    struct urb *urb;
    struct usbnet_self_buff_desc *p_desc;

    while (1)
    {
        if (rt_mb_recv(&p_usbnet->usb_net_tx_mb, (rt_uint32_t *) &urb,
        RT_WAITING_FOREVER) == RT_EOK) {
            p_desc = urb->context;
            usbnet_put_tx_buf(p_usbnet, p_desc);
            usb_free_urb(urb);
        }

    }

}

/**
 *
 * add eth tools func
 *
 * **/
#ifdef RT_USING_ETHTOOL

static rt_uint32_t mii_get_an(struct mii_if_info *mii, rt_uint16_t addr)
{
    rt_uint32_t result = 0;
    int advert;

    advert = mii->mdio_read(mii->dev, mii->phy_id, addr);
    if (advert & LPA_LPACK)
        result |= ADVERTISED_Autoneg;
    if (advert & ADVERTISE_10HALF)
        result |= ADVERTISED_10baseT_Half;
    if (advert & ADVERTISE_10FULL)
        result |= ADVERTISED_10baseT_Full;
    if (advert & ADVERTISE_100HALF)
        result |= ADVERTISED_100baseT_Half;
    if (advert & ADVERTISE_100FULL)
        result |= ADVERTISED_100baseT_Full;
    if (advert & ADVERTISE_PAUSE_CAP)
        result |= ADVERTISED_Pause;
    if (advert & ADVERTISE_PAUSE_ASYM)
        result |= ADVERTISED_Asym_Pause;

    return result;
}

/**
 * mii_ethtool_gset - get settings that are specified in @ecmd
 * @mii: MII interface
 * @ecmd: requested ethtool_cmd
 *
 * The @ecmd parameter is expected to have been cleared before calling
 * mii_ethtool_gset().
 *
 * Returns 0 for success, negative on error.
 */
int mii_ethtool_gset(struct mii_if_info *mii, struct ethtool_cmd *ecmd)
{
    struct usbnet *dev = mii->dev;
    rt_uint16_t bmcr, bmsr, ctrl1000 = 0, stat1000 = 0;
    rt_uint32_t nego;

    ecmd->supported =
      (SUPPORTED_10baseT_Half | SUPPORTED_10baseT_Full |
       SUPPORTED_100baseT_Half | SUPPORTED_100baseT_Full |
       SUPPORTED_Autoneg | SUPPORTED_TP | SUPPORTED_MII);
    if (mii->supports_gmii)
        ecmd->supported |= SUPPORTED_1000baseT_Half | SUPPORTED_1000baseT_Full;

    /* only supports twisted-pair */
    ecmd->port = PORT_MII;

    /* only supports internal transceiver */
    ecmd->transceiver = XCVR_INTERNAL;

    /* this isn't fully supported at higher layers */
    ecmd->phy_address = mii->phy_id;
    ecmd->advertising = ADVERTISED_TP | ADVERTISED_MII;
    bmcr = mii->mdio_read(dev, mii->phy_id, MII_BMCR);
    bmsr = mii->mdio_read(dev, mii->phy_id, MII_BMSR);
    if (mii->supports_gmii)
    {
        ctrl1000 = mii->mdio_read(dev, mii->phy_id, MII_CTRL1000);
        stat1000 = mii->mdio_read(dev, mii->phy_id, MII_STAT1000);
    }

    if (bmcr & BMCR_ANENABLE)
    {
        ecmd->advertising |= ADVERTISED_Autoneg;
        ecmd->autoneg = AUTONEG_ENABLE;
        ecmd->advertising |= mii_get_an(mii, MII_ADVERTISE);
        if (ctrl1000 & ADVERTISE_1000HALF)
            ecmd->advertising |= ADVERTISED_1000baseT_Half;
        if (ctrl1000 & ADVERTISE_1000FULL)
            ecmd->advertising |= ADVERTISED_1000baseT_Full;

        if (bmsr & BMSR_ANEGCOMPLETE)
        {
            ecmd->lp_advertising = mii_get_an(mii, MII_LPA);
            if (stat1000 & LPA_1000HALF)
                ecmd->lp_advertising |=
              ADVERTISED_1000baseT_Half;
            if (stat1000 & LPA_1000FULL)
                ecmd->lp_advertising |=
              ADVERTISED_1000baseT_Full;
        } else
        {
            ecmd->lp_advertising = 0;
        }

        nego = ecmd->advertising & ecmd->lp_advertising;

        if (nego & (ADVERTISED_1000baseT_Full |
                    ADVERTISED_1000baseT_Half))
        {
            ethtool_cmd_speed_set(ecmd, SPEED_1000);
            ecmd->duplex = !!(nego & ADVERTISED_1000baseT_Full);
        } else if (nego & (ADVERTISED_100baseT_Full | ADVERTISED_100baseT_Half))
        {
            ethtool_cmd_speed_set(ecmd, SPEED_100);
            ecmd->duplex = !!(nego & ADVERTISED_100baseT_Full);
        } else
        {
            ethtool_cmd_speed_set(ecmd, SPEED_10);
            ecmd->duplex = !!(nego & ADVERTISED_10baseT_Full);
        }
    } else
    {
        ecmd->autoneg = AUTONEG_DISABLE;
        ethtool_cmd_speed_set(ecmd,
                              ((bmcr & BMCR_SPEED1000 &&
                                (bmcr & BMCR_SPEED100) == 0) ?
                               SPEED_1000 :
                               ((bmcr & BMCR_SPEED100) ?
                                SPEED_100 : SPEED_10)));
        ecmd->duplex = (bmcr & BMCR_FULLDPLX) ? DUPLEX_FULL : DUPLEX_HALF;
    }

    mii->full_duplex = ecmd->duplex;

    /* ignore maxtxpkt, maxrxpkt for now */

    return 0;
}


/**
 * mii_ethtool_sset - set settings that are specified in @ecmd
 * @mii: MII interface
 * @ecmd: requested ethtool_cmd
 *
 * Returns 0 for success, negative on error.
 */
int mii_ethtool_sset(struct mii_if_info *mii, struct ethtool_cmd *ecmd)
{
    struct usbnet *dev = mii->dev;
    rt_uint32_t speed = ethtool_cmd_speed(ecmd);

    if (speed != SPEED_10 &&
        speed != SPEED_100 &&
        speed != SPEED_1000)
    {
        rt_kprintf("ERR speed : %x\n",speed);
        return -EINVAL;
    }

    if (ecmd->duplex != DUPLEX_HALF && ecmd->duplex != DUPLEX_FULL)
    {
        rt_kprintf("ERR duplex : %x\n",ecmd->duplex);
        return -EINVAL;
    }

    if (ecmd->port != PORT_MII)
    {
        rt_kprintf("ERR port : %x\n",ecmd->port);
        return -EINVAL;
    }

    if (ecmd->transceiver != XCVR_INTERNAL)
    {
        rt_kprintf("ERR transceiver : %x\n",ecmd->transceiver);
        return -EINVAL;
    }

    if (ecmd->autoneg != AUTONEG_DISABLE && ecmd->autoneg != AUTONEG_ENABLE)
    {
        rt_kprintf("ERR autoneg : %x\n",ecmd->autoneg);
        return -EINVAL;
    }

    if ((speed == SPEED_1000) && (!mii->supports_gmii))
    {
        rt_kprintf(" speed set 1000M but dev not support\n");
        return -EINVAL;
    }
    rt_kprintf("usb net sset : %dMbps, %s-duplex, %s-autoneg\n",
            speed,
            ecmd->duplex ? "full" : "half",
            ecmd->autoneg ? "enable" : "disable");
    /* ignore supported, maxtxpkt, maxrxpkt */

    if (ecmd->autoneg == AUTONEG_ENABLE)
    {
        rt_uint32_t bmcr, advert, tmp;
        rt_uint32_t advert2 = 0, tmp2 = 0;

        if ((ecmd->advertising & (ADVERTISED_10baseT_Half |
                                  ADVERTISED_10baseT_Full |
                                  ADVERTISED_100baseT_Half |
                                  ADVERTISED_100baseT_Full |
                                  ADVERTISED_1000baseT_Half |
                                  ADVERTISED_1000baseT_Full)) == 0)
            return -EINVAL;

        /* advertise only what has been requested */
        advert = mii->mdio_read(dev, mii->phy_id, MII_ADVERTISE);
        tmp = advert & ~(ADVERTISE_ALL | ADVERTISE_100BASE4);
        if (mii->supports_gmii)
        {
            advert2 = mii->mdio_read(dev, mii->phy_id, MII_CTRL1000);
            tmp2 = advert2 & ~(ADVERTISE_1000HALF | ADVERTISE_1000FULL);
        }
        if (ecmd->advertising & ADVERTISED_10baseT_Half)
            tmp |= ADVERTISE_10HALF;
        if (ecmd->advertising & ADVERTISED_10baseT_Full)
            tmp |= ADVERTISE_10FULL;
        if (ecmd->advertising & ADVERTISED_100baseT_Half)
            tmp |= ADVERTISE_100HALF;
        if (ecmd->advertising & ADVERTISED_100baseT_Full)
            tmp |= ADVERTISE_100FULL;
        if (mii->supports_gmii)
        {
            if (ecmd->advertising & ADVERTISED_1000baseT_Half)
                tmp2 |= ADVERTISE_1000HALF;
            if (ecmd->advertising & ADVERTISED_1000baseT_Full)
                tmp2 |= ADVERTISE_1000FULL;
        }
        if (advert != tmp)
        {
            mii->mdio_write(dev, mii->phy_id, MII_ADVERTISE, tmp);
            mii->advertising = tmp;
        }
        if ((mii->supports_gmii) && (advert2 != tmp2))
            mii->mdio_write(dev, mii->phy_id, MII_CTRL1000, tmp2);

        /* turn on autonegotiation, and force a renegotiate */
        bmcr = mii->mdio_read(dev, mii->phy_id, MII_BMCR);
        bmcr |= (BMCR_ANENABLE | BMCR_ANRESTART);
        mii->mdio_write(dev, mii->phy_id, MII_BMCR, bmcr);
        mii->force_media = 0;
    } else
    {
        rt_uint32_t bmcr, tmp;

        /* turn off auto negotiation, set speed and duplexity */
        bmcr = mii->mdio_read(dev, mii->phy_id, MII_BMCR);
        tmp = bmcr & ~(BMCR_ANENABLE | BMCR_SPEED100 |
                       BMCR_SPEED1000 | BMCR_FULLDPLX);
        if (speed == SPEED_1000)
            tmp |= BMCR_SPEED1000;
        else if (speed == SPEED_100)
            tmp |= BMCR_SPEED100;
        if (ecmd->duplex == DUPLEX_FULL)
        {
            tmp |= BMCR_FULLDPLX;
            mii->full_duplex = 1;
        } else
            mii->full_duplex = 0;
        if (bmcr != tmp)
            mii->mdio_write(dev, mii->phy_id, MII_BMCR, tmp);

        mii->force_media = 1;
    }
    return 0;
}



/**
 * mii_check_media - check the MII interface for a duplex change
 * @mii: the MII interface
 * @ok_to_print: OK to print link up/down messages
 * @init_media: OK to save duplex mode in @mii
 *
 * Returns 1 if the duplex mode changed, 0 if not.
 * If the media type is forced, always returns 0.
 */

int mii_link_ok(struct mii_if_info *mii)
{
    /* first, a dummy read, needed to latch some MII phys */
    mii->mdio_read(mii->dev, mii->phy_id, MII_BMSR);
    if (mii->mdio_read(mii->dev, mii->phy_id, MII_BMSR) & BMSR_LSTATUS)
        return 1;
    return 0;
}

int netif_carrier_ok(struct usbnet *dev)
{
    return !!dev->link_state;
}

unsigned int mii_check_media(struct mii_if_info *mii,
                              unsigned int ok_to_print,
                              unsigned int init_media)
{
    unsigned int old_carrier, new_carrier;
    int advertise, lpa, media, duplex;
    int lpa2 = 0;

    /* if forced media, go no further */
    if (mii->force_media)
        return 0; /* duplex did not change */

    /* check current and old link status */
    old_carrier = netif_carrier_ok(mii->dev) ? 1 : 0;
    new_carrier = (unsigned int) mii_link_ok(mii);

    if ((!init_media) && (old_carrier == new_carrier))
        return 0; /* duplex did not change */

    /* no carrier, nothing much to do */
    if (!new_carrier)
    {
        /*netif_carrier_off(mii->dev);*/
        if (ok_to_print)
            rt_kprintf("link down\n");
        return 0; /* duplex did not change */
    }
    /* we have carrier, see who's on the other end*/

    /*netif_carrier_on(mii->dev);*/

    /* get MII advertise and LPA values */
    if ((!init_media) && (mii->advertising))
        advertise = mii->advertising;
    else
    {
        advertise = mii->mdio_read(mii->dev, mii->phy_id, MII_ADVERTISE);
        mii->advertising = advertise;
    }
    lpa = mii->mdio_read(mii->dev, mii->phy_id, MII_LPA);
    if (mii->supports_gmii)
        lpa2 = mii->mdio_read(mii->dev, mii->phy_id, MII_STAT1000);

    /* figure out media and duplex from advertise and LPA values */
    media = mii_nway_result(lpa & advertise);
    duplex = (media & ADVERTISE_FULL) ? 1 : 0;
    if (lpa2 & LPA_1000FULL)
        duplex = 1;

    if (ok_to_print)
        rt_kprintf("link up, %uMbps, %s-duplex, lpa 0x%04X\n",
                   lpa2 & (LPA_1000FULL | LPA_1000HALF) ? 1000 :
                   media & (ADVERTISE_100FULL | ADVERTISE_100HALF) ?
                   100 : 10,
                   duplex ? "full" : "half",
                   lpa);
    if ((init_media) || (mii->full_duplex != duplex))
    {
        mii->full_duplex = duplex;
        return 1; /* duplex changed */
    }

    return 0; /* duplex did not change */
}
unsigned int usbnet_get_link(struct net_device *net)
{
    /*just use soft net flag to keep uniformity*/
    return net->netif->flags & NETIF_FLAG_LINK_UP ? 1 : 0;
}

int usbnet_get_settings(struct net_device *net, struct ethtool_cmd *cmd)
{

    /*call the driver mdio driver read*/
    struct usbnet *dev = private_from_net_to_usbnet(net);

    if (!dev->mii.mdio_read)
    {
        rt_kprintf("%s: %s: has no mdio_read func..\n",
               __func__, net->parent.parent.name);
        return -EOPNOTSUPP;
    }
    return mii_ethtool_gset(&dev->mii, cmd);
}


int usbnet_set_settings(struct net_device *net, struct ethtool_cmd *cmd)
{

    struct usbnet *dev = private_from_net_to_usbnet(net);
    int retval;

    if (!dev->mii.mdio_write)
        return -EOPNOTSUPP;

    retval = mii_ethtool_sset(&dev->mii, cmd);
    if (retval != 0)
    {
        rt_kprintf("%s failed. err no : %x\n",__func__,retval);
        return retval;
    }
    /* link speed/duplex might have changed */
    if (dev->driver_info->link_reset)
        dev->driver_info->link_reset(dev);

    return 0;
}

static struct ethtool_ops usb_net_ethtool_ops = {
    .get_link = usbnet_get_link,
    .get_settings = usbnet_get_settings,
    .set_settings = usbnet_set_settings,
};

#endif

struct usbnet *usbnet_check_usbdev_reconnet(void)
{
    char usbnet_name[20] = { 0 };
    int list_id = 0;
    struct net_device *p_eth;
    struct usbnet *p_usbnet;

    while (1)
    {
        rt_sprintf(usbnet_name, "%s%d", "u", list_id++);
        p_usbnet = RT_NULL;
        p_eth = (struct net_device *)rt_device_find(usbnet_name);
        if (!p_eth)
            break;
        p_usbnet = private_from_net_to_usbnet(p_eth);
        if ((p_usbnet->usbnet_magic == USBNET_MAGIC) &&
        (USBNET_GET_STATUS(p_usbnet) & USBNET_DISCONNECT))
            break;
    }

    return p_usbnet;
}


void usbnet_reconnet_update_usbdev(struct usbnet *p_usbnet,
struct usb_interface *udev, struct usb_device_id *prod)
{
    register rt_base_t temp;
    /*close int, incase multi reconnet*/
    temp = rt_hw_interrupt_disable();
    USBNET_SET_STATUS(p_usbnet, USBNET_CONNECT);
#ifdef RT_USING_USB_ECM
    net_device_linkchange(&p_usbnet->rtt_eth_interface, 1);
#endif
    p_usbnet->udev = udev->usb_dev;
    p_usbnet->intf = udev;
    p_usbnet->driver_info = (struct driver_info *) prod->driver_info;
    p_usbnet->driver_name = p_usbnet->driver_info->description;
    p_usbnet->driver_info->bind(p_usbnet, udev);
    usb_set_intfdata(udev, p_usbnet);
    rt_hw_interrupt_enable(temp);
    /*wake the reconnet thread*/
    rt_sem_release(&p_usbnet->sem_reconnet);
}

static void usbnet_syn_signal_free(struct usbnet *p_usbnet)
{
    rt_mutex_detach(&p_usbnet->phy_mutex);
    rt_mutex_detach(&p_usbnet->rx_idle_node_mutex);
    rt_mutex_detach(&p_usbnet->rx_done_node_mutex);
    rt_mutex_detach(&p_usbnet->rx_node_mutex);
    rt_mutex_detach(&p_usbnet->tx_free_mutex);
    rt_sem_detach(&p_usbnet->sem_rx_idle_list);
    rt_sem_detach(&p_usbnet->sem_interrupt);
    rt_sem_detach(&p_usbnet->sem_reconnet);
    rt_sem_detach(&p_usbnet->sem_usbnet_tx);
    rt_mb_detach(&p_usbnet->usb_net_tx_mb);
    rt_mb_detach(&p_usbnet->tx_err_mb);
    rt_mb_detach(&p_usbnet->rx_err_mb);
    rt_mb_detach(&p_usbnet->int_err_mb);
}
int usbnet_probe(struct usb_interface *udev, const struct usb_device_id *prod)
{
    int status, i;
    struct usbnet *p_usbnet;
    struct net_device *p_eth;
    char usbnet_name[20] = { 0 };
    static int usbnet_id = 0;
    struct usbnet_rx_node *node;
    struct urb *urb;
    unsigned char flag = 0;

    p_usbnet = usbnet_check_usbdev_reconnet();
    if (p_usbnet)
    {
        rt_kprintf("reconnet coming......\n");
        usbnet_reconnet_update_usbdev(p_usbnet, udev, (struct usb_device_id *)prod);
        return 0;
    }

    p_usbnet = rt_malloc(sizeof(struct usbnet));
    rt_kprintf("%s probe go\n", __func__);
    if (!p_usbnet)
    {
        rt_kprintf("usbnet malloc no mem..\n");
        status = -ENOMEM;
        goto out1;
    }
    g_test_usbnet = p_usbnet;
    rt_memset(p_usbnet, 0, sizeof(struct usbnet));
    rt_mutex_init(&p_usbnet->phy_mutex, "usb mutex", RT_IPC_FLAG_FIFO);
    rt_mutex_init(&p_usbnet->rx_idle_node_mutex, "usb rx idle",
                    RT_IPC_FLAG_FIFO);
    rt_mutex_init(&p_usbnet->rx_done_node_mutex, "usb rx done",
                    RT_IPC_FLAG_FIFO);
    rt_mutex_init(&p_usbnet->rx_node_mutex, "usb rx node",
                    RT_IPC_FLAG_FIFO);
    rt_mutex_init(&p_usbnet->tx_free_mutex, "usb tx free",
                    RT_IPC_FLAG_FIFO);
    rt_sem_init(&p_usbnet->sem_rx_idle_list, "usb_rx_idle", 0,
                    RT_IPC_FLAG_FIFO);
    rt_sem_init(&p_usbnet->sem_interrupt, "usb_int_pkt", 0,
                    RT_IPC_FLAG_FIFO);
    rt_sem_init(&p_usbnet->sem_reconnet, "usb_reconnet", 0,
                    RT_IPC_FLAG_FIFO);
    rt_sem_init(&p_usbnet->sem_usbnet_tx, "usbnet_tx", USBNET_TX_RING_SIZE,
                    RT_IPC_FLAG_FIFO);

    rt_mb_init(&p_usbnet->usb_net_tx_mb, "usbnet_tx_poll",
                            &p_usbnet->usb_net_tx_mb_pool[0], sizeof(p_usbnet->usb_net_tx_mb_pool)/4,
                            RT_IPC_FLAG_FIFO);

    rt_mb_init(&p_usbnet->tx_err_mb, "usbnet_tx_err_poll",
                            &p_usbnet->tx_err_mb_pool[0], sizeof(p_usbnet->tx_err_mb_pool)/4,
                            RT_IPC_FLAG_FIFO);
    rt_mb_init(&p_usbnet->rx_err_mb, "usbnet_rx_err_poll",
                            &p_usbnet->rx_err_mb_pool[0], sizeof(p_usbnet->rx_err_mb_pool)/4,
                            RT_IPC_FLAG_FIFO);
    rt_mb_init(&p_usbnet->int_err_mb, "usbnet_int_err_poll",
                            &p_usbnet->int_err_mb_pool[0], sizeof(p_usbnet->int_err_mb_pool)/4,
                            RT_IPC_FLAG_FIFO);

    /* malloc tx and rx buff... */
    status = usbnet_init_self_mem(p_usbnet);
    USB_NET_ASSERT(status == 0);
    if (status != 0)
    {
        rt_kprintf("%s init mem error ...\n", __func__);
        goto out2;
    }

    /* bind interface.. */
    p_usbnet->udev = udev->usb_dev;
    p_usbnet->intf = udev;
    p_usbnet->driver_info = (struct driver_info *) prod->driver_info;
    p_usbnet->driver_name = p_usbnet->driver_info->description;
    /* call the bind func... */
    if (p_usbnet->driver_info->bind)
    {
        /* int    (*bind)(struct usbnet *, struct usb_interface *); */
        rt_kprintf("call usbnet driver:%s..\n", p_usbnet->driver_name);
        status = p_usbnet->driver_info->bind(p_usbnet, p_usbnet->intf);
        if (status != 0)
        {
            rt_kprintf("usbnet driver:%s bind failed..\n",
                            p_usbnet->driver_name);
            for (i = 0; i < USBNET_RX_URB_SIZE; i++)
            {
                urb = p_usbnet->rx_urb_array[i];
                usb_free_urb(urb);
                node = get_one_rx_node(&p_usbnet->rx_idle_node_head);
                if (!node)
                    continue;
                rt_free(node);
            }
            usbnet_free_self_mem(p_usbnet);
            goto out2;
        }

    }
    /* register net interface... */
    p_eth = &p_usbnet->rtt_eth_interface;
    p_eth->parent.type = RT_Device_Class_NetIf;
    p_eth->parent.init = rtt_interface_usbnet_init;
    p_eth->parent.open = rtt_interface_usbnet_open;
    p_eth->parent.close = rtt_interface_usbnet_close;
    p_eth->parent.read = rtt_interface_usbnet_read;
    p_eth->parent.write = rtt_interface_usbnet_write;
    p_eth->parent.control = rtt_interface_usbnet_control;
    /* data port... */
    p_eth->net.eth.eth_tx = rtt_interface_usbnet_tx;
    p_eth->net.eth.eth_rx = rtt_interface_usbnet_rx;
#ifdef RT_USING_ETHTOOL
    p_eth->net.eth.ethtool_ops = &usb_net_ethtool_ops;
#endif
    /* before register eth. run the rx thread... */
    /* TBD..name should add number in the tail. */
    p_usbnet->usb_net_rx = rt_thread_create("usb_net_rx",
                    usbnet_thread_check_rx_list, (void *) p_usbnet,
                    2048, USBNET_RX_PRIORITY, 10);
    USB_NET_ASSERT(p_usbnet->usb_net_rx != RT_NULL);
    if (p_usbnet->usb_net_rx != RT_NULL)
    {

        rt_thread_startup(p_usbnet->usb_net_rx);
    }



    p_usbnet->usb_net_tx = rt_thread_create("usb_net_tx",
            usbnet_thread_check_tx_list, (void *) p_usbnet,
                    2048, USBNET_TX_PRIORITY, 10);
    USB_NET_ASSERT(p_usbnet->usb_net_tx != RT_NULL);
    if (p_usbnet->usb_net_tx != RT_NULL)
    {

        rt_thread_startup(p_usbnet->usb_net_tx);
    }

    p_usbnet->usb_net_err_prc = rt_thread_create("usb_net_err_prc",
            usbnet_thread_reconnet_prc, (void *) p_usbnet,
                    2048, USBNET_ERR_PRIORITY, 10);
    USB_NET_ASSERT(p_usbnet->usb_net_err_prc != RT_NULL);
    if (p_usbnet->usb_net_err_prc != RT_NULL)
    {

        rt_thread_startup(p_usbnet->usb_net_err_prc);
    }



    p_usbnet->usb_net_int = rt_thread_create("usb_net_int",
                    usbnet_thread_interrupt_packet_process, (void *) p_usbnet,
                    2048, USBNET_RX_PRIORITY - 10, 10);
    USB_NET_ASSERT(p_usbnet->usb_net_int != RT_NULL);
    if (p_usbnet->usb_net_int != RT_NULL)
    {
        rt_thread_startup(p_usbnet->usb_net_int);
    }

    init_status(p_usbnet, udev);

    rt_sprintf(usbnet_name, "%s%d", "u", usbnet_id++);

    flag = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP;
#ifdef RT_LWIP_IGMP
    flag |= NETIF_FLAG_IGMP;
#endif
    status = net_device_init(&p_usbnet->rtt_eth_interface, usbnet_name, flag, NET_DEVICE_USBNET);
    usb_set_intfdata(udev, p_usbnet);
    USBNET_SET_STATUS(p_usbnet, USBNET_CONNECT);
    netif_set_default(p_usbnet->rtt_eth_interface.netif);
#ifdef RT_USING_USB_ECM
    net_device_linkchange(&p_usbnet->rtt_eth_interface, 1);
#endif
    p_usbnet->usbnet_magic = USBNET_MAGIC;
    USB_NET_ASSERT(status == 0);
    rt_kprintf("%s probe ok...\n", __func__);
    return status;

out2:
    usbnet_syn_signal_free(p_usbnet);
    usb_set_intfdata(udev, NULL);
    rt_free(p_usbnet);
out1:
    return status;

}


void usbnet_disconnect(struct usb_interface *intf)
{
    struct usbnet *p_usbnet;
    int i;
    rt_kprintf("% s\n", __func__);
    p_usbnet = usb_get_intfdata(intf);
    if (p_usbnet)
    {
        rt_mutex_take(&(p_usbnet->tx_free_mutex), RT_WAITING_FOREVER);
        p_usbnet->udev = NULL;
        USBNET_SET_STATUS(p_usbnet, USBNET_DISCONNECT);
        net_device_linkchange(&p_usbnet->rtt_eth_interface, 0);

        struct usbnet_self_buff_desc *p_desc;
        p_desc = p_usbnet->txbuff_desc;
        for (i = 0; i < USBNET_TX_RING_SIZE; i++, p_desc++)
        {
            if (p_desc->mem_flag == USBNET_MEM_BUSY)
            {
                usb_free_urb(p_desc->urb);
                usbnet_put_tx_buf(p_usbnet, p_desc);
            }
        }

        rt_mutex_release(&(p_usbnet->tx_free_mutex));

        usb_kill_urb(p_usbnet->interrupt);
        for (i = 0; i < USBNET_RX_URB_SIZE; i++)
            usb_kill_urb(p_usbnet->rx_urb_array[i]);
    }
}

int kick_usb_idle(void)
{
    int ret = 0;

    ret =  usbnet_check_rx_idle_list(g_test_usbnet);
    rt_kprintf("kick usb ret is %x\n", ret);
    return ret;
}

int kick_usb_done(void)
{
    int ret = 0;

    usbnet_check_rx_done_list(g_test_usbnet);
    rt_kprintf("kick usb ret is %x\n", ret);
    return ret;
}

int usb_idletimes(void)
{
    rt_kprintf("idle submit times: %x; done time :%x , eth times:0x%x\n",
                    _rx_idle_times, _rx_done_times, call_eth_times);
    rt_kprintf("idle info..\n");
    /* rt_mutex_take(&g_test_usbnet->rx_idle_node_mutex, RT_WAITING_FOREVER); */
    dump_usb_rx_node_info(&g_test_usbnet->rx_idle_node_head);
    /* rt_mutex_release(&g_test_usbnet->rx_idle_node_mutex); */
    rt_kprintf("done info..\n");
    /* rt_mutex_take(&g_test_usbnet->rx_done_node_mutex, RT_WAITING_FOREVER); */
    dump_usb_rx_node_info(&g_test_usbnet->rx_done_node_head);
    /* rt_mutex_release(&g_test_usbnet->rx_done_node_mutex); */
    return 0;
}

#ifdef RT_USING_FINSH
#include <finsh.h>
FINSH_FUNCTION_EXPORT(kick_usb_idle, kick_usb_idle());
FINSH_FUNCTION_EXPORT(kick_usb_done, kick_usb_done());
FINSH_FUNCTION_EXPORT(usb_idletimes, usb_idletimes());
#endif

