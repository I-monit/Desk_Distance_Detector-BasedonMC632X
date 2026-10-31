#include <usb.h>
#include <rtdef.h>
#include <rtthread.h>
#include <rtdevice.h>
#include <rthw.h>
/* #include "usb_errno.h" */
#include <dma_mem.h>
#include <interrupt.h>
#include <sys/types.h>
#include "usbnet.h"
#include "asix.h"
#include <mii.h>

#define DRIVER_VERSION "14-Jun-2006"
static const char driver_name[] = "asix";

/* ASIX AX8817X based USB 2.0 Ethernet Devices */

#define AX_CMD_SET_SW_MII        0x06
#define AX_CMD_READ_MII_REG        0x07
#define AX_CMD_WRITE_MII_REG        0x08
#define AX_CMD_SET_HW_MII        0x0a
#define AX_CMD_READ_EEPROM        0x0b
#define AX_CMD_WRITE_EEPROM        0x0c
#define AX_CMD_WRITE_ENABLE        0x0d
#define AX_CMD_WRITE_DISABLE        0x0e
#define AX_CMD_READ_RX_CTL        0x0f
#define AX_CMD_WRITE_RX_CTL        0x10
#define AX_CMD_READ_IPG012        0x11
#define AX_CMD_WRITE_IPG0        0x12
#define AX_CMD_WRITE_IPG1        0x13
#define AX_CMD_READ_NODE_ID        0x13
#define AX_CMD_WRITE_NODE_ID        0x14
#define AX_CMD_WRITE_IPG2        0x14
#define AX_CMD_WRITE_MULTI_FILTER    0x16
#define AX88172_CMD_READ_NODE_ID    0x17
#define AX_CMD_READ_PHY_ID        0x19
#define AX_CMD_READ_MEDIUM_STATUS    0x1a
#define AX_CMD_WRITE_MEDIUM_MODE    0x1b
#define AX_CMD_READ_MONITOR_MODE    0x1c
#define AX_CMD_WRITE_MONITOR_MODE    0x1d
#define AX_CMD_READ_GPIOS        0x1e
#define AX_CMD_WRITE_GPIOS        0x1f
#define AX_CMD_SW_RESET            0x20
#define AX_CMD_SW_PHY_STATUS        0x21
#define AX_CMD_SW_PHY_SELECT        0x22

#define AX_MONITOR_MODE            0x01
#define AX_MONITOR_LINK            0x02
#define AX_MONITOR_MAGIC        0x04
#define AX_MONITOR_HSFS            0x10

/* AX88172 Medium Status Register values */
#define AX88172_MEDIUM_FD        0x02
#define AX88172_MEDIUM_TX        0x04
#define AX88172_MEDIUM_FC        0x10
#define AX88172_MEDIUM_DEFAULT \
        (AX88172_MEDIUM_FD | AX88172_MEDIUM_TX | AX88172_MEDIUM_FC)

#define AX_MCAST_FILTER_SIZE        8
#define AX_MAX_MCAST            64

#define AX_SWRESET_CLEAR        0x00
#define AX_SWRESET_RR            0x01
#define AX_SWRESET_RT            0x02
#define AX_SWRESET_PRTE            0x04
#define AX_SWRESET_PRL            0x08
#define AX_SWRESET_BZ            0x10
#define AX_SWRESET_IPRL            0x20
#define AX_SWRESET_IPPD            0x40

#define AX88772_IPG0_DEFAULT        0x15
#define AX88772_IPG1_DEFAULT        0x0c
#define AX88772_IPG2_DEFAULT        0x12

/* AX88772 & AX88178 Medium Mode Register */
#define AX_MEDIUM_PF        0x0080
#define AX_MEDIUM_JFE        0x0040
#define AX_MEDIUM_TFC        0x0020
#define AX_MEDIUM_RFC        0x0010
#define AX_MEDIUM_ENCK        0x0008
#define AX_MEDIUM_AC        0x0004
#define AX_MEDIUM_FD        0x0002
#define AX_MEDIUM_GM        0x0001
#define AX_MEDIUM_SM        0x1000
#define AX_MEDIUM_SBP        0x0800
#define AX_MEDIUM_PS        0x0200
#define AX_MEDIUM_RE        0x0100

#define AX88178_MEDIUM_DEFAULT    \
    (AX_MEDIUM_PS | AX_MEDIUM_FD | AX_MEDIUM_AC | \
     AX_MEDIUM_JFE | \
     AX_MEDIUM_RE)

#define AX88772_MEDIUM_DEFAULT    \
    (AX_MEDIUM_FD | \
     AX_MEDIUM_PS | \
     AX_MEDIUM_AC | AX_MEDIUM_RE)

/* AX88772 & AX88178 RX_CTL values */
#define AX_RX_CTL_SO            0x0080
#define AX_RX_CTL_AP            0x0020
#define AX_RX_CTL_AM            0x0010
#define AX_RX_CTL_AB            0x0008
#define AX_RX_CTL_SEP            0x0004
#define AX_RX_CTL_AMALL            0x0002
#define AX_RX_CTL_PRO            0x0001
#define AX_RX_CTL_MFB_2048        0x0000
#define AX_RX_CTL_MFB_4096        0x0100
#define AX_RX_CTL_MFB_8192        0x0200
#define AX_RX_CTL_MFB_16384        0x0300

#define AX_DEFAULT_RX_CTL    \
    (AX_RX_CTL_SO | AX_RX_CTL_AB)

/* GPIO 0 .. 2 toggles */
#define AX_GPIO_GPO0EN        0x01    /* GPIO0 Output enable */
#define AX_GPIO_GPO_0        0x02    /* GPIO0 Output value */
#define AX_GPIO_GPO1EN        0x04    /* GPIO1 Output enable */
#define AX_GPIO_GPO_1        0x08    /* GPIO1 Output value */
#define AX_GPIO_GPO2EN        0x10    /* GPIO2 Output enable */
#define AX_GPIO_GPO_2        0x20    /* GPIO2 Output value */
#define AX_GPIO_RESERVED    0x40    /* Reserved */
#define AX_GPIO_RSE        0x80    /* Reload serial EEPROM */

#define AX_EEPROM_MAGIC        0xdeadbeef
#define AX88172_EEPROM_LEN    0x40
#define AX88772_EEPROM_LEN    0xff

#define PHY_MODE_MARVELL    0x0000
#define MII_MARVELL_LED_CTRL    0x0018
#define MII_MARVELL_STATUS    0x001b
#define MII_MARVELL_CTRL    0x0014

#define MARVELL_LED_MANUAL    0x0019

#define MARVELL_STATUS_HWCFG    0x0004

#define MARVELL_CTRL_TXDELAY    0x0002
#define MARVELL_CTRL_RXDELAY    0x0080

/* This structure cannot exceed sizeof(unsigned long [5]) AKA 20 bytes */
struct asix_data
{
    rt_uint8_t multi_filter[AX_MCAST_FILTER_SIZE];
    rt_uint8_t mac_addr[ETH_ALEN];
    rt_uint8_t phymode;
    rt_uint8_t ledmode;
    rt_uint8_t eeprom_len;
};

struct ax88172_int_data
{
    rt_uint16_t res1;
    rt_uint8_t link;
    rt_uint16_t res2;
    rt_uint8_t status;
    rt_uint16_t res3;
} __packed;

/* cpy from linux 49 by zhangy 2018-11-3*/
struct asix_rx_fixup_info
{
    struct pbuf *ax_pbuf;
    rt_uint32_t pbuf_offset;
    rt_uint32_t header;
    rt_uint32_t remaining;
    rt_uint32_t split_head;
};
/* cpy from linux 49 by zhangy 2018-11-3*/
struct asix_common_private
{
    struct asix_rx_fixup_info rx_fixup_info;
};

struct asix_common_private *get_asix_private(struct usbnet *dev)
{
    if (dev->driver_info->driver_priv)
        return (struct asix_common_private *)dev->driver_info->driver_priv;
    return NULL;
}

inline void *kmemdup(const void *src, size_t len, gfp_t gfp)
{
    void *p;

    p = kmalloc_track_caller(len, gfp);
    if (p)
        memcpy(p, src, len);
    return p;
}
static int asix_read_cmd(struct usbnet *dev, rt_uint8_t cmd, rt_uint16_t value, rt_uint16_t index,
                rt_uint16_t size, void *data)
{
    void *buf;
    int err = -ENOMEM;

    netdev_dbg(dev->net, "asix_read_cmd() cmd=0x%02x value=0x%04x index=0x%04x size=%d\n",
           cmd, value, index, size);

    buf = kmalloc(size+4, GFP_KERNEL);
    if (!buf)
        goto out;

    err = usb_control_msg(
        dev->udev,
        usb_rcvctrlpipe(dev->udev, 0),
        cmd,
        USB_DIR_IN | USB_TYPE_VENDOR | USB_RECIP_DEVICE,
        value,
        index,
        buf,
        size,
        USB_CTRL_GET_TIMEOUT);
    if (err == size)
        memcpy(data, buf, size);
    else if (err >= 0)
        err = -EINVAL;
    kfree(buf);

out:
    return err;
}


static int asix_write_cmd(struct usbnet *dev, rt_uint8_t cmd, rt_uint16_t value, rt_uint16_t index,
                 rt_uint16_t size, void *data)
{
    void *buf = NULL;
    int err = -ENOMEM;

    netdev_dbg(dev->net, "asix_write_cmd() cmd=0x%02x value=0x%04x index=0x%04x size=%d\n",
           cmd, value, index, size);

    if (data)
    {
        buf = kmemdup(data, size, GFP_KERNEL);
        if (!buf)
            goto out;
    }

    err = usb_control_msg(
        dev->udev,
        usb_sndctrlpipe(dev->udev, 0),
        cmd,
        USB_DIR_OUT | USB_TYPE_VENDOR | USB_RECIP_DEVICE,
        value,
        index,
        buf,
        size,
        USB_CTRL_SET_TIMEOUT);
    kfree(buf);

out:
    return err;
}

unsigned int asix_rx_fixup(struct usbnet *dev, unsigned char *rx_buf,
                unsigned int rx_buf_size, struct usbnet_rx_node *rx_node)
{

    unsigned char *temp_buf = RT_NULL;
    unsigned int temp_data;
    rt_uint16_t size;
    int offset = 0;
    struct asix_rx_fixup_info *rx;
    struct asix_common_private *priv;
    temp_buf = rx_buf;
    priv = get_asix_private(dev);
    rx = &priv->rx_fixup_info;
    /*| remaining .... Head+[new data] |
     * maybe a bug....may be i need to add remaining + 1 then to check...
     */
    if (rx->remaining && (rx->remaining + sizeof(rt_uint32_t) <= rx_buf_size))
    {
        offset = ((rx->remaining + 1) & 0xfffe);
        rt_memcpy(&rx->header, (temp_buf + offset), sizeof(rt_uint32_t));
        offset = 0;
        size = (rt_uint16_t)(rx->header & 0x7ff);
        if (size != ((~rx->header >> 16) & 0x7ff))
        {
            rt_kprintf("asix_rx_fixup() Data Header synchronisation was lost, remaining %d\n", rx->remaining);
            if (rx->ax_pbuf)
            {
                rt_kprintf("discard pubuff get from last urb buff..\n");
                pbuf_free(rx->ax_pbuf);
                rx->ax_pbuf = NULL;
            }
            rx->pbuf_offset = 0;
            rx->remaining = 0;
        }
    }

    while (offset + sizeof(rt_uint16_t) <= rx_buf_size)
    {
        rt_uint16_t copy_length;

        if (!rx->remaining)
        {
            if (rx_buf_size - offset == sizeof(rt_uint16_t))
            {
                rt_memcpy(&rx->header, (temp_buf + offset), sizeof(rt_uint16_t));
                rx->split_head = RT_TRUE;
                offset += sizeof(rt_uint16_t);
                break;
            }
            if (rx->split_head == RT_TRUE)
            {
                rt_memcpy(&temp_data, (temp_buf + offset), sizeof(rt_uint16_t));
                rx->header |=  temp_data << 16;
                rx->split_head = RT_FALSE;
                offset += sizeof(rt_uint16_t);
            }
            else
            {
                rt_memcpy(&rx->header, (temp_buf + offset), sizeof(rt_uint32_t));
                offset += sizeof(rt_uint32_t);
            }

            /* take frame length from Data header 32-bit word */
            size = (rt_uint16_t)(rx->header & 0x7ff);
            if (size != ((~rx->header >> 16) & 0x7ff))
            {
                rt_kprintf("asix_rx_fixup() Bad Header Length 0x%x, offset %d\n", rx->header, offset);
                return -1;
            }

            rx->ax_pbuf = pbuf_alloc(PBUF_RAW, USBNET_PBUF_MALLOC_LEN, PBUF_RAM);
            if (!rx->ax_pbuf)
            {
                rt_kprintf("malloc pbuf error...\n");
                USB_NET_ASSERT(rx->ax_pbuf != RT_NULL);
            }
            rx->remaining = size;
        }
        if (rx->remaining > rx_buf_size - offset)
        {
            copy_length = rx_buf_size - offset;
            rx->remaining -= copy_length;
        }
        else
        {
            copy_length = rx->remaining;
            rx->remaining = 0;
        }

        if (rx->ax_pbuf)
        {
            rt_memcpy((rx->ax_pbuf->payload + rx->pbuf_offset), (temp_buf + offset), copy_length);
            rx->pbuf_offset += copy_length;
            if (!rx->remaining)
            {
                rx->ax_pbuf->len = rx->ax_pbuf->tot_len = rx->pbuf_offset;
                usbnet_put_one_pbuf(dev, rx->ax_pbuf, &rx_node->list_done_pbuf);
                rx->pbuf_offset = 0;
                rx->ax_pbuf = RT_NULL;
            }
        }
        offset += (copy_length + 1) & 0xfffe;
    }

    if (rx_buf_size != offset)
    {
        rt_kprintf("asix_rx_fixup() Bad SKB Length %d, %d\n", rx_buf_size, offset);
        return -1;
    }
    rx_node->rx_state_mac = GET_ACTIVE_NODE_PBUFF;
    return 0;

}



unsigned char *asix_tx_fixup(struct usbnet *dev,
        struct pbuf *p, unsigned char *tx_buf, unsigned int tx_buf_size, unsigned int *status, unsigned int *size)
{
    unsigned char *temp_buf;
    struct pbuf *temp_pbuf;
    unsigned int packet_len;
    /* check tx buff size.. */
    *status = 0;
    if ((p->tot_len + 4) > tx_buf_size)
    {
        rt_kprintf("tx need 4 byte space to tell the asix how many bytes to send...\n");
        *status = -1;
        goto out;
    }

    packet_len = (((p->tot_len) ^ 0x0000ffff) << 16) + (p->tot_len);
    /* packet_len = cpu_to_le32(packet_len); */
    /* rt_kprintf("self add len is %x\n",packet_len); */
    /* add total size to first .. */
    rt_memcpy(tx_buf, &packet_len, sizeof(packet_len));
    /* cpy data to the allign buf.. */
    for (temp_pbuf = p, temp_buf = tx_buf + sizeof(packet_len); temp_pbuf != NULL; temp_pbuf = temp_pbuf->next)
    {
        rt_memcpy(temp_buf, temp_pbuf->payload, temp_pbuf->len);
        temp_buf += temp_pbuf->len;
    }
    *size = p->tot_len + 4;
    /* rt_kprintf("transfer size is %d\n",*size); */

out :
    return tx_buf;

}

static void asix_status(struct usbnet *dev, struct urb *urb)
{
    struct ax88172_int_data *event;
    int link;
    unsigned int old_link;
    if (urb->actual_length < 8)
        return;
    event = urb->transfer_buffer;
    link = event->link & 0x01;
    old_link = dev->link_state;
    if (link != old_link)
    {
        rt_kprintf("%s Link Status : %s\n",
        dev->rtt_eth_interface.parent.parent.name,
        (link == 1) ? "UP" : "DOWN");
        dev->link_state = link;
        net_device_linkchange(&dev->rtt_eth_interface, dev->link_state);
    }
}

static inline int asix_set_sw_mii(struct usbnet *dev)
{
    int ret;

    ret = asix_write_cmd(dev, AX_CMD_SET_SW_MII, 0x0000, 0, 0, NULL);
    if (ret < 0)
        rt_kprintf("Failed to enable software MII access\n");
    return ret;
}

static inline int asix_set_hw_mii(struct usbnet *dev)
{
    int ret;

    ret = asix_write_cmd(dev, AX_CMD_SET_HW_MII, 0x0000, 0, 0, NULL);
    if (ret < 0)
        rt_kprintf("Failed to enable hardware MII access\n");
    return ret;
}

int asix_get_phy_addr(struct usbnet *dev)
{
    rt_uint8_t buf[2];
    int ret = asix_read_cmd(dev, AX_CMD_READ_PHY_ID, 0, 0, 2, buf);

    netdev_dbg(dev->net, "asix_get_phy_addr()\n");

    if (ret < 0)
    {
        rt_kprintf("Error reading PHYID register: %02x\n", ret);
        goto out;
    }
    netdev_dbg(dev->net, "asix_get_phy_addr() returning 0x%04x\n",
           *((rt_uint16_t *)buf));
    ret = buf[1];

out:
    return ret;
}

static int asix_sw_reset(struct usbnet *dev, rt_uint8_t flags)
{
    int ret;

        ret = asix_write_cmd(dev, AX_CMD_SW_RESET, flags, 0, 0, NULL);
    if (ret < 0)
        rt_kprintf("Failed to send software reset: %02x\n", ret);

    return ret;
}

static rt_uint16_t asix_read_rx_ctl(struct usbnet *dev)
{
    rt_uint16_t v;
    int ret = asix_read_cmd(dev, AX_CMD_READ_RX_CTL, 0, 0, 2, &v);

    if (ret < 0)
    {
        rt_kprintf("Error reading RX_CTL register: %02x\n", ret);
        goto out;
    }
    ret = le16_to_cpu(v);
out:
    return ret;
}

static int asix_write_rx_ctl(struct usbnet *dev, rt_uint16_t mode)
{
    int ret;
    /* mode = 0x81; */
    netdev_dbg(dev->net, "asix_write_rx_ctl() - mode = 0x%04x\n", mode);
    ret = asix_write_cmd(dev, AX_CMD_WRITE_RX_CTL, mode, 0, 0, NULL);
    if (ret < 0)
        rt_kprintf("Failed to write RX_CTL mode to 0x%04x: %02x\n",
               mode, ret);

    return ret;
}

static rt_uint16_t asix_read_medium_status(struct usbnet *dev)
{
    rt_uint16_t v;
    int ret = asix_read_cmd(dev, AX_CMD_READ_MEDIUM_STATUS, 0, 0, 2, &v);

    if (ret < 0)
    {
        rt_kprintf("Error reading Medium Status register: %02x\n",
               ret);
        goto out;
    }
    ret = le16_to_cpu(v);
out:
    return ret;
}

static int asix_write_medium_mode(struct usbnet *dev, rt_uint16_t mode)
{
    int ret;
    netdev_dbg(dev->net, "asix_write_medium_mode() - mode = 0x%04x\n", mode);
    ret = asix_write_cmd(dev, AX_CMD_WRITE_MEDIUM_MODE, mode, 0, 0, NULL);
    if (ret < 0)
        rt_kprintf("Failed to write Medium Mode mode to 0x%04x: %02x\n",
               mode, ret);

    return ret;
}

static int asix_write_gpio(struct usbnet *dev, rt_uint16_t value, int sleep)
{
    int ret;

    netdev_dbg(dev->net, "asix_write_gpio() - value = 0x%04x\n", value);
    ret = asix_write_cmd(dev, AX_CMD_WRITE_GPIOS, value, 0, 0, NULL);
    if (ret < 0)
        rt_kprintf("Failed to write GPIO value 0x%04x: %02x\n",
               value, ret);

    return ret;
}

#if (0)
static void asix_set_multicast(struct usbnet *udev)
{

}
#endif




static int asix_mdio_read(struct usbnet *udev, int phy_id, int loc)
{

    struct usbnet *dev = udev;
    rt_uint16_t res;

    rt_mutex_take(&dev->phy_mutex, RT_WAITING_FOREVER);
    asix_set_sw_mii(dev);
    asix_read_cmd(dev, AX_CMD_READ_MII_REG, phy_id,
                (rt_uint16_t)loc, 2, &res);
    asix_set_hw_mii(dev);
    rt_mutex_release(&dev->phy_mutex);

    netdev_dbg(dev->net, "asix_mdio_read() phy_id=0x%02x, loc=0x%02x, returns=0x%04x\n",
           phy_id, loc, le16_to_cpu(res));
    return le16_to_cpu(res);

}

static void
asix_mdio_write(struct usbnet *udev, int phy_id, int loc, int val)
{

    struct usbnet *dev = udev;
    rt_uint16_t res = cpu_to_le16(val);

    netdev_dbg(dev->net, "asix_mdio_write() phy_id=0x%02x, loc=0x%02x, val=0x%04x\n",
           phy_id, loc, val);
    rt_mutex_take(&dev->phy_mutex, RT_WAITING_FOREVER);
    asix_set_sw_mii(dev);
    asix_write_cmd(dev, AX_CMD_WRITE_MII_REG, phy_id, (rt_uint16_t)loc, 2, &res);
    asix_set_hw_mii(dev);
    rt_mutex_release(&dev->phy_mutex);

}

/* Get the PHY Identifier from the PHYSID1 & PHYSID2 MII registers */
rt_uint32_t asix_get_phyid(struct usbnet *dev)
{

    int phy_reg;
    rt_uint32_t phy_id;

    phy_reg = asix_mdio_read(dev, dev->mii.phy_id, MII_PHYSID1);
    if (phy_reg < 0)
        return 0;

    phy_id = (phy_reg & 0xffff) << 16;

    phy_reg = asix_mdio_read(dev, dev->mii.phy_id, MII_PHYSID2);
    if (phy_reg < 0)
        return 0;

    phy_id |= (phy_reg & 0xffff);
    return phy_id;
}
#if (0)
static int asix_get_eeprom_len(struct usbnet *udev)
{

    struct usbnet *dev = udev;
    struct asix_data *data = (struct asix_data *)&dev->data;

    return data->eeprom_len;

}


static int asix_get_eeprom(struct usbnet *udev,
                  struct ethtool_eeprom *eeprom, rt_uint8_t *data)
{
    struct usbnet *dev = udev;
    rt_uint16_t *ebuf = (rt_uint16_t *)data;
    int i;

    /* Crude hack to ensure that we don't overwrite memory
     * if an odd length is supplied
     */
    if (eeprom->len % 2)
        return -EINVAL;

    eeprom->magic = AX_EEPROM_MAGIC;

    /* ax8817x returns 2 bytes from eeprom on read */
    for (i = 0; i < eeprom->len / 2; i++) {
    {
        if (asix_read_cmd(dev, AX_CMD_READ_EEPROM,
            eeprom->offset + i, 0, 2, &ebuf[i]) < 0)
            return -EINVAL;
    }
    return 0;
}
#endif

static int ax88772_link_reset(struct usbnet *dev)
{
    rt_uint16_t mode;
    struct ethtool_cmd ecmd = { .cmd = ETHTOOL_GSET };

    mii_check_media(&dev->mii, 1, 1);
    mii_ethtool_gset(&dev->mii, &ecmd);
    mode = AX88772_MEDIUM_DEFAULT;

    if (ethtool_cmd_speed(&ecmd) != SPEED_100)
        mode &= ~AX_MEDIUM_PS;

    if (ecmd.duplex != DUPLEX_FULL)
        mode &= ~AX_MEDIUM_FD;

    asix_write_medium_mode(dev, mode);
    asix_write_rx_ctl(dev, AX_DEFAULT_RX_CTL | AX_RX_CTL_SO);
    return 0;
}


static int mii_nway_restart(struct usbnet *dev)
{
    int bmcr;
    int r = -1;

    /* if autoneg is off, it's an error */
    bmcr = asix_mdio_read(dev, dev->mii.phy_id, MII_BMCR);

    if (bmcr & BMCR_ANENABLE)
    {
        bmcr |= BMCR_ANRESTART;
        asix_mdio_write(dev, dev->mii.phy_id, MII_BMCR, bmcr);
        r = 0;
    }

    return r;
}



static int ax88772_bind(struct usbnet *dev, struct usb_interface *intf)
{
    int ret, embd_phy;
    rt_uint16_t rx_ctl;
    /* struct asix_data *data = (struct asix_data *) &dev->data; */
    rt_uint8_t buf[ETH_ALEN];
    /* rt_uint32_t phyid; */
    usbnet_get_endpoints(dev, intf);

    if ((ret = asix_write_gpio(dev,
    AX_GPIO_RSE | AX_GPIO_GPO_2 | AX_GPIO_GPO2EN, 5)) < 0)
        goto out;

    embd_phy = ((asix_get_phy_addr(dev) & 0x1f) == 0x10 ? 1 : 0);
    if ((ret = asix_write_cmd(dev, AX_CMD_SW_PHY_SELECT, embd_phy, 0, 0,
                    NULL)) < 0) {
        rt_kprintf("Select PHY #1 failed: %d\n", ret);
        goto out;
    }
    /* malloc private info by zhangy add below*/
    if (!dev->driver_info->driver_priv)
    {
        dev->driver_info->driver_priv = rt_malloc(sizeof(struct asix_common_private));
        if (!dev->driver_info->driver_priv)
        {
            ret = -ENOMEM;
            rt_kprintf("%s : %d malloc failed..\n", __func__, __LINE__);
            goto out;
        }
        rt_memset(dev->driver_info->driver_priv, 0, sizeof(struct asix_common_private));
    }
    dev->mii.phy_id = asix_get_phy_addr(dev);

    asix_mdio_write(dev, dev->mii.phy_id, MII_BMCR, BMCR_RESET);

    /* wait.. */

    int phy_reg_data;
    do
    {
        phy_reg_data = asix_mdio_read(dev, dev->mii.phy_id, MII_BMCR);
    } while (phy_reg_data & BMCR_RESET);

    mii_nway_restart(dev);
    asix_mdio_write(dev, dev->mii.phy_id, MII_ADVERTISE,
    ADVERTISE_ALL | ADVERTISE_CSMA);

    if ((ret = asix_sw_reset(dev, AX_SWRESET_IPPD | AX_SWRESET_PRL)) < 0)
        goto out;

    if ((ret = asix_sw_reset(dev, AX_SWRESET_CLEAR)) < 0)
        goto out;

    if (embd_phy)
    {
        if ((ret = asix_sw_reset(dev, AX_SWRESET_IPRL)) < 0)
            goto out;
    }
    else
    {
        if ((ret = asix_sw_reset(dev, AX_SWRESET_PRTE)) < 0)
            goto out;
    }

    rx_ctl = asix_read_rx_ctl(dev);
    if ((ret = asix_write_rx_ctl(dev, 0x0000)) < 0)
        goto out;

    rx_ctl = asix_read_rx_ctl(dev);
    /* Get the MAC address */
    if ((ret = asix_read_cmd(dev, AX_CMD_READ_NODE_ID, 0, 0, ETH_ALEN, buf))
                    < 0) {
        rt_kprintf("Failed to read MAC address: %d\n", ret);
        goto out;
    }
    memcpy(dev->usbnet_mac_add, buf, ETH_ALEN);

    dev->mii.dev = dev;
    dev->mii.mdio_read = asix_mdio_read;
    dev->mii.mdio_write = asix_mdio_write;
    dev->mii.phy_id_mask = 0x1f;
    dev->mii.reg_num_mask = 0x1f;
    dev->mii.supports_gmii = 0;
    if ((ret = asix_sw_reset(dev, AX_SWRESET_PRL)) < 0)
        goto out;

    if ((ret = asix_sw_reset(dev, AX_SWRESET_IPRL | AX_SWRESET_PRL)) < 0)
        goto out;

    /* mii_nway_restart(&dev->mii); */

    if ((ret = asix_write_medium_mode(dev, AX88772_MEDIUM_DEFAULT)) < 0)
        goto out;

    if ((ret = asix_write_cmd(dev, AX_CMD_WRITE_IPG0,
    AX88772_IPG0_DEFAULT | AX88772_IPG1_DEFAULT,
    AX88772_IPG2_DEFAULT, 0, NULL)) < 0) {
        rt_kprintf("Write IPG,IPG1,IPG2 failed: %d\n", ret);
        goto out;
    }

    /* Set RX_CTL to default values with 2k buffer, and enable cactus */
    if ((ret = asix_write_rx_ctl(dev, AX_DEFAULT_RX_CTL)) < 0)
        goto out;

    rx_ctl = asix_read_rx_ctl(dev);
    rt_kprintf("RX_CTL is 0x%04x after all initializations\n", rx_ctl);

    rx_ctl = asix_read_medium_status(dev);
    rt_kprintf("Medium Status is 0x%04x after all initializations\n",
                    rx_ctl);

    /* Asix framing packs multiple eth frames into a 2K usb bulk transfer */
    if (dev->driver_info->flags & FLAG_FRAMING_AX)
    {
        /* hard_mtu  is still the default - the device does not support
         jumbo eth frames */
        dev->rx_urb_size = 2048;
    }

    return 0;

out: return ret;

}
static const struct driver_info ax88772_info = {
    .description = "ASIX AX88772 USB 2.0 Ethernet",
    .bind = ax88772_bind,
    .status = asix_status,
    .link_reset = ax88772_link_reset,
    .reset = ax88772_link_reset,
    .flags = FLAG_ETHER | FLAG_FRAMING_AX | FLAG_LINK_INTR,
    .rx_fixup = asix_rx_fixup,
    .tx_fixup = asix_tx_fixup,
};
static const struct usb_device_id    products[] = {

{
    USB_DEVICE(0x0b95, 0x772b),
    .driver_info = (unsigned long) &ax88772_info,
},

    { },
};
static struct usb_driver asix_driver = {
    .name =        "asix",
    .id_table =    products,
    .probe =    usbnet_probe,
    .disconnect = usbnet_disconnect,

};

int asix_usb_mac_init(void)
{
    int retval;

    rt_kprintf("asix mac init go...\n");
    /* register the driver, return usb_register return code if error */
    retval = usb_register_driver(&asix_driver);
    if (retval == 0)
    {
        rt_kprintf("asix mac usb registered done..\n");
    }
    return retval;
}
