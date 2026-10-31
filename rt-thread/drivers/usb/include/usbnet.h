#ifndef __USBNET_H__
#define __USBNET_H__

/****************************************************************************
* #include section
*    add #include here if any
***************************************************************************/
#include <usb.h>
#include <rtdef.h>
#include <rtthread.h>
#include <rtdevice.h>
#include <rthw.h>
/* #include "usb_errno.h" */
#include <dma_mem.h>
#include <interrupt.h>
#include <sys/types.h>
#include <netif/ethernetif.h>
#include <mii.h>

/****************************************************************************
* #define section
*    add constant #define here if any
***************************************************************************/
#ifdef USB_NET_DEBUG
#define netdev_dbg(__dev, format, args...)            \
({                                \
    if (1)                            \
        rt_kprintf(format, ##args); \
    0;                            \
})
#else
#define netdev_dbg(__dev, format, args...)
#endif

#define kmalloc(x, y) rt_malloc(x)
#define kfree(x) rt_free(x)
#define vmalloc(x) rt_malloc(x)
#define vfree(x) rt_free(x)
#define memcpy(x, y, z) rt_memcpy(x, y, z)
#define strncpy(x, y, z) rt_strncpy(x, y, z)

#define USBNET_TX_RING_SIZE         20
#define USBNET_TX_BUFFER_SIZE         2048
#define USBNET_RX_BUFFER_SIZE         2048
#define USBNET_RX_RING_SIZE         20
#define USBNET_RX_URB_SIZE        USBNET_RX_RING_SIZE
/* means max rx eth packet in one urb rx buff..
maybe one urb have many short packet.*/
#define USBNET_TX_MESSAGEPOLL_SIZE    USBNET_TX_RING_SIZE
#define USBNET_PBUF_MALLOC_LEN 1600
/*here the thread send to the usb core should low than lwip thread...
 * cause data processed by the lwip should be faster than rev from usb..
 */
#define USBNET_RX_PRIORITY        RT_LWIP_ETHTHREAD_PRIORITY + 1
#define USBNET_TX_PRIORITY        RT_LWIP_ETHTHREAD_PRIORITY - 1
#define USBNET_ERR_PRIORITY        RT_LWIP_ETHTHREAD_PRIORITY + 2
/** 16 bits byte swap */
#define swap_byte_16(x) \
    ((rt_uint16_t)((((rt_uint16_t)(x) & 0x00ffU) << 8) | \
    (((rt_uint16_t)(x) & 0xff00U) >> 8)))

/** 32 bits byte swap */
#define swap_byte_32(x) \
    ((rt_uint32_t)((((rt_uint32_t)(x) & 0x000000ffUL) << 24) | \
    (((rt_uint32_t)(x) & 0x0000ff00UL) <<  8) | \
    (((rt_uint32_t)(x) & 0x00ff0000UL) >>  8) | \
    (((rt_uint32_t)(x) & 0xff000000UL) >> 24)))


#define USBNET_TX_ERR_MSGPOLL_SIZE    USBNET_TX_RING_SIZE
#define USBNET_RX_ERR_MSGPOLL_SIZE    USBNET_RX_RING_SIZE
#define USBNET_INT_ERR_MSGPOLL_SIZE    1

#if BYTE_ORDER == BIG_ENDIAN
/** Convert from 16 bit little endian format to CPU format */
#define le16_to_cpu(x) swap_byte_16(x)
/** Convert from 32 bit little endian format to CPU format */
#define le32_to_cpu(x) swap_byte_32(x)
/** Convert to 16 bit little endian format from CPU format */
#define cpu_to_le16(x) swap_byte_16(x)
/** Convert to 32 bit little endian format from CPU format */
#define cpu_to_le32(x) swap_byte_32(x)
/** Convert WFDCMD header to little endian format from CPU format */
#define endian_convert_request_header(x);               \
    do                                                   \
    {                                              \
        (x).cmd_code = cpu_to_le16((x).cmd_code);   \
        (x).size = cpu_to_le16((x).size);         \
        (x).seq_num = cpu_to_le16((x).seq_num);     \
        (x).result = cpu_to_le16((x).result);     \
    } while (0);

/** Convert WFDCMD header from little endian format to CPU format */
#define endian_convert_response_header(x);              \
    do                                                   \
    {                                               \
        (x).cmd_code = le16_to_cpu((x).cmd_code);   \
        (x).size = le16_to_cpu((x).size);         \
        (x).seq_num = le16_to_cpu((x).seq_num);     \
        (x).result = le16_to_cpu((x).result);     \
    } while (0);

#else /* BIG_ENDIAN */
/** Do nothing */
#define le16_to_cpu(x) x
/** Do nothing */
#define le32_to_cpu(x) x
/** Do nothing */
#define cpu_to_le16(x) x
/** Do nothing */
#define cpu_to_le32(x) x
/** Do nothing */
#define endian_convert_request_header(x)
/** Do nothing */
#define endian_convert_response_header(x)

#endif /* BIG_ENDIAN */


#define kmalloc_track_caller(size, flags) \
        kmalloc(size, flags)

#define ETHTOOL_FWVERS_LEN    32
#define ETHTOOL_BUSINFO_LEN    32
#define ETH_ALEN         6

#define    DEFAULT_FILTER    (USB_CDC_PACKET_TYPE_BROADCAST \
            |USB_CDC_PACKET_TYPE_ALL_MULTICAST \
            |USB_CDC_PACKET_TYPE_PROMISCUOUS \
            |USB_CDC_PACKET_TYPE_DIRECTED)

#define USB_NET_ASSERT(expr)\
do\
{\
    if (!(expr))\
    { \
        rt_kprintf("[usb_net] : Assertion failed! %s:line %d\n", \
        __func__, __LINE__); \
        while (1)   \
           ;       \
    }\
}\
while (0)

static inline void ethtool_cmd_speed_set(struct ethtool_cmd *ep,
                                         rt_uint32_t speed)
{

    ep->speed = (rt_uint16_t)speed;
    ep->speed_hi = (rt_uint16_t)(speed >> 16);
}

static inline rt_uint32_t ethtool_cmd_speed(const struct ethtool_cmd *ep)
{
    return (ep->speed_hi << 16) | ep->speed;
}


/****************************************************************************
* ADT section
*    add Abstract Data Type definition here
***************************************************************************/
typedef enum {
    GFP_KERNEL,
    GFP_ATOMIC,
} gfp_t;
typedef enum {
    IRQ_NONE,
    IRQ_HANDLED
} irqreturn_t;

struct  sk_buff_head
{
    struct list_head    *next, *prev;
    unsigned int         qlen;
};


struct mii_if_info
{
    int phy_id;
    int advertising;
    int phy_id_mask;
    int reg_num_mask;

    unsigned int full_duplex : 1;    /* is full duplex? */
    unsigned int force_media : 1;    /* is autoneg. disabled? */
    unsigned int supports_gmii : 1; /* are GMII registers supported? */

    struct usbnet *dev;
    int (*mdio_read)(struct usbnet *dev, int phy_id, int location);
    void (*mdio_write)(struct usbnet *dev, int phy_id, int location, int val);
};


struct ethtool_eeprom
{
    rt_uint32_t    cmd;
    rt_uint32_t    magic;
    rt_uint32_t    offset; /* in bytes */
    rt_uint32_t    len; /* in bytes */
    rt_uint8_t    data[0];
};


struct ethtool_drvinfo
{
    rt_uint32_t    cmd;
    char    driver[32];    /* driver short name, "tulip", "eepro100" */
    char    version[32];    /* driver version string */
    char    fw_version[ETHTOOL_FWVERS_LEN];    /* firmware version string */
    char    bus_info[ETHTOOL_BUSINFO_LEN];    /* Bus info for this IF. */
    /* For PCI devices, use pci_name(pci_dev). */
    char    reserved1[32];
    char    reserved2[12];
    /*
     * Some struct members below are filled in
     * using ops->get_sset_count().  Obtaining
     * this info from ethtool_drvinfo is now
     * deprecated; Use ETHTOOL_GSSET_INFO
     * instead.
     */
    rt_uint32_t    n_priv_flags;    /* number of flags valid in ETHTOOL_GPFLAGS */
    rt_uint32_t    n_stats;    /* number of u64's from ETHTOOL_GSTATS */
    rt_uint32_t    testinfo_len;
    rt_uint32_t    eedump_len;    /* Size of data from ETHTOOL_GEEPROM (bytes) */
    rt_uint32_t    regdump_len;    /* Size of data from ETHTOOL_GREGS (bytes) */
};


struct usbnet_self_buff_desc
{

#define USBNET_MEM_IDLE        0x55
#define USBNET_MEM_BUSY        0xaa
    unsigned int mem_flag;
    /* point to the mem. */
    rt_uint8_t *buff_add;
    /* this transfer error maybe i will use. */
    unsigned int status;
    unsigned int usb_in_active_len;
    /* bind to the host point... */
    struct usbnet *p_usbnet;
    void *urb;
};

struct usbnet_pubuf_list
{
    rt_list_t list;
    struct pbuf *p_buf;
};

struct usbnet_rx_node
{
    rt_list_t list;
    struct urb *urb;
    struct usbnet_self_buff_desc *p_buf_desc;
    /* add pbuf done list...
       this para is just a head list... may have several pbufs which should put them to the lwip..
    */
    struct usbnet_pubuf_list list_done_pbuf;
#define GET_ACTIVE_NODE_PBUFF   1
#define GET_NODE_DONE           2
    rt_uint32_t rx_state_mac;
};



struct usbnet
{
    /* housekeeping */
    struct net_device rtt_eth_interface;
    struct usb_device    *udev;
    struct usb_interface    *intf;
    struct driver_info    *driver_info;
    const char        *driver_name;
    void            *driver_priv;
    /* wait_queue_head_t    *wait; */
    struct rt_mutex        phy_mutex;
    /* unsigned char        suspend_count; */
    struct rt_semaphore    sem_rx_idle_list;
    struct rt_semaphore    sem_interrupt;
    struct rt_semaphore    sem_reconnet;
    struct rt_semaphore    sem_usbnet_tx;
    struct rt_mailbox      usb_net_tx_mb;
    rt_uint32_t usb_net_tx_mb_pool[USBNET_TX_MESSAGEPOLL_SIZE];
    rt_thread_t usb_net_rx;
    rt_thread_t usb_net_tx;
    rt_thread_t usb_net_int;

    /* i/o info: pipes etc */
    unsigned int in, out;
    struct usb_host_endpoint *status;
    unsigned int maxpacket;
    /* struct timer_list    delay; */

    unsigned long        data[5];
    rt_uint32_t        xid;
    rt_uint32_t        hard_mtu;    /* count any extra framing */
    rt_uint32_t        rx_urb_size;    /* size for rx urbs */
    struct mii_if_info    mii;

    /* self buf.... */
    /* first point to the allign buf.. */
    rt_uint8_t *tx_allign_raw_buff;
    rt_uint8_t *rx_allign_raw_buff;
    rt_uint8_t *tx_allign_buff;
    rt_uint8_t *rx_allign_buff;
    rt_uint8_t *interrupt_raw_buf;
    rt_uint8_t *interrupt_allign_buf;
    /* 2048 default.. */
    rt_uint32_t tx_allign_buff_size;
    rt_uint32_t rx_allign_buff_size;
    /* use  'tx_allign_buff + tx_allign_index * tx_allign_buff_size' to user to buf net data */
    /* data is 0,1,2,3,....size of rx or tx ,0 */
    rt_uint32_t tx_allign_index;
    rt_uint32_t rx_allign_index;
    struct usbnet_self_buff_desc *txbuff_desc;
    struct usbnet_self_buff_desc *rxbuff_desc;
    /* rx urb array... */
    struct urb *rx_urb_array[USBNET_RX_URB_SIZE];

    /* various kinds of pending driver work */
    /* only used to check link status. */
    struct urb        *interrupt;

    /* struct usb_anchor    deferred; */
    /* struct tasklet_struct    bh; */
    rt_uint8_t usbnet_mac_add[ETH_ALEN];
    struct rt_mutex        rx_idle_node_mutex;
    struct rt_mutex        rx_done_node_mutex;
    struct rt_mutex        rx_node_mutex;
    struct rt_mutex        tx_free_mutex;

    /* here just need a "list" .. but just look good for here :) */
    struct usbnet_rx_node rx_idle_node_head;
    struct usbnet_rx_node rx_done_node_head;
    rt_uint32_t link_state;
    /* add active node to process rx done list..*/
    struct usbnet_rx_node *active_node;
    #define USBNET_MAGIC        0xaaaa5555
    int usbnet_magic;
    #define USBNET_CONNECT_MASK 0xff
    #define USBNET_CONNECT      0xaa
    #define USBNET_DISCONNECT   0x55
    int usbnet_status;

    rt_thread_t usb_net_err_prc;
    struct rt_mailbox tx_err_mb;
    struct rt_mailbox rx_err_mb;
    struct rt_mailbox int_err_mb;
    rt_uint32_t tx_err_mb_pool[USBNET_TX_ERR_MSGPOLL_SIZE];
    rt_uint32_t rx_err_mb_pool[USBNET_RX_ERR_MSGPOLL_SIZE];
    rt_uint32_t int_err_mb_pool[USBNET_INT_ERR_MSGPOLL_SIZE];
    unsigned long		flags;
#		define EVENT_TX_HALT	0
#		define EVENT_RX_HALT	1
#		define EVENT_RX_MEMORY	2
#		define EVENT_STS_SPLIT	3
#		define EVENT_LINK_RESET	4
#		define EVENT_RX_PAUSED	5
#		define EVENT_DEV_ASLEEP 6
#		define EVENT_DEV_OPEN	7
#		define EVENT_DEVICE_REPORT_IDLE	8
#		define EVENT_NO_RUNTIME_PM	9
#		define EVENT_RX_KILL	10
#		define EVENT_LINK_CHANGE	11
#		define EVENT_SET_RX_MODE	12
#		define EVENT_NO_IP_ALIGN	13
};


static inline struct usb_driver *driver_of(struct usb_interface *intf)
{
    return intf->driver;
}

/* interface from the device/framing level "minidriver" to core */
struct driver_info
{
    char        *description;
    int        flags;
/* framing is CDC Ethernet, not writing ZLPs (hw issues), or optionally: */
#define FLAG_FRAMING_NC    0x0001        /* guard against device dropouts */
#define FLAG_FRAMING_GL    0x0002        /* genelink batches packets */
#define FLAG_FRAMING_Z    0x0004        /* zaurus adds a trailer */
#define FLAG_FRAMING_RN    0x0008        /* RNDIS batches, plus huge header */

#define FLAG_NO_SETINT    0x0010        /* device can't set_interface() */
#define FLAG_ETHER    0x0020        /* maybe use "eth%d" names */

#define FLAG_FRAMING_AX 0x0040        /* AX88772/178 packets */
#define FLAG_WLAN    0x0080        /* use "wlan%d" names */
#define FLAG_AVOID_UNLINK_URBS 0x0100    /* don't unlink urbs at usbnet_stop() */
#define FLAG_SEND_ZLP    0x0200        /* hw requires ZLPs are sent */
#define FLAG_WWAN    0x0400        /* use "wwan%d" names */

#define FLAG_LINK_INTR    0x0800        /* updates link (carrier) status */

#define FLAG_POINTTOPOINT 0x1000    /* possibly use "usb%d" names */

/*
 * Indicates to usbnet, that USB driver accumulates multiple IP packets.
 * Affects statistic (counters) and short packet handling.
 */
#define FLAG_MULTI_PACKET    0x2000
#define FLAG_RX_ASSEMBLE    0x4000    /* rx packets may span >1 frames */

    /* init device ... can sleep, or cause probe() failure */
    int (*bind)(struct usbnet *, struct usb_interface *);

    /* cleanup device ... can sleep, but can't fail */
    void (*unbind)(struct usbnet *, struct usb_interface *);

    /* reset device ... can sleep */
    int (*reset)(struct usbnet *);

    /* stop device ... can sleep */
    int (*stop)(struct usbnet *);

    /* see if peer is connected ... can sleep */
    int (*check_connect)(struct usbnet *);

    /* (dis)activate runtime power management */
    int (*manage_power)(struct usbnet *, int);

    /* for status polling */
    void (*status)(struct usbnet *, struct urb *);

    /* link reset handling, called from defer_kevent */
    int (*link_reset)(struct usbnet *);

    /* fixup rx packet (strip framing) */
    unsigned int (*rx_fixup)(struct usbnet *dev, unsigned char *rx_buf,
                unsigned int rx_buf_size, struct usbnet_rx_node *rx_node);

    /* fixup tx packet (add framing) */
    unsigned char *(*tx_fixup)(struct usbnet *dev, struct pbuf *p,
                unsigned char *tx_buf, unsigned int tx_buf_size,
                unsigned int *status, unsigned int *size);

    /* early initialization code, can sleep. This is for minidrivers
     * having 'subminidrivers' that need to do extra initialization
     * right after minidriver have initialized hardware. */
    int (*early_init)(struct usbnet *dev);

    /* called by minidriver when receiving indication */
    void (*indication)(struct usbnet *dev, void *ind, int indlen);

    /* for new devices, use the descriptor-reading code instead */
    int        in;        /* rx endpoint */
    int        out;        /* tx endpoint */
    /* add driver private info by zhangy 2018-11-3 to fix asix rx fixup lost data */
    void    *driver_priv;
    unsigned long    data;        /* Misc driver specific data */
};

/****************************************************************************
*  extern variable declaration section
***************************************************************************/

/****************************************************************************
*  section
*    add function prototype here if any
***************************************************************************/

int usbnet_probe(struct usb_interface *udev, const struct usb_device_id *prod);
void usbnet_disconnect(struct usb_interface *udev);
int usbnet_get_endpoints(struct usbnet *dev, struct usb_interface *intf);
int usbnet_put_one_pbuf(struct usbnet *p_usbnet, struct pbuf *p_buf, struct usbnet_pubuf_list *head_node);
struct pbuf *usbnet_get_one_pbuf(struct usbnet_pubuf_list *p_node);
unsigned int mii_check_media(struct mii_if_info *mii,
                              unsigned int ok_to_print,
                              unsigned int init_media);
int mii_ethtool_gset(struct mii_if_info *mii, struct ethtool_cmd *ecmd);
int usbnet_get_ethernet_addr(struct usbnet *dev, int iMACAddress);

/* Drivers that reuse some of the standard USB CDC infrastructure
 * (notably, using multiple interfaces according to the CDC
 * union descriptor) get some helper code.
 */
struct cdc_state
{
    struct usb_cdc_header_desc    *header;
    struct usb_cdc_union_desc    *u;
    struct usb_cdc_ether_desc    *ether;
    struct usb_interface        *control;
    struct usb_interface        *data;
};



/********************************End Of File********************************/
#endif /* USBNET_H_ */
