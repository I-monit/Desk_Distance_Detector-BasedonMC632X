/*********************************************************************************************************/ /**
 * @file    dm9051_stm.c
 * @version V1.02
 * @date    2014/6/10
 * @brief   The DM9051 driver for uCOSII+LwIP & uIP on STM32F103.
 *************************************************************************************************************
 *
 * <h2><center>Copyright (C) 2014 DAVICOM Semiconductor Inc. All rights reserved</center></h2>
 *
 ***********************************************************************************************************
 * History:
 *  V1.02:  2014/8/8
 *      1. Add bus driving to 8mA(DM9051_PBCR). For SPI clock upgrade to 36Mhz.
 *
 */

/* Includes ------------------------------------------------------------------------------------------------*/
#include "dm9051.h"
#include "gpio.h"


/* Private constants ---------------------------------------------------------------------------------------*/
#define DM9000_PHY (0x40) /* PHY address 0x01                                                               */

/* Private function prototypes -----------------------------------------------------------------------------*/

static rt_uint16_t phy_read(rt_device_t dev, struct rt_spi_device *spi_device, rt_uint32_t uReg);
static void phy_write(rt_device_t dev, struct rt_spi_device *spi_device, uint16_t reg, uint16_t value);
static void phy_mode_set(rt_device_t dev, struct rt_spi_device *spi_device, uint32_t uMediaMode);

static void _dm9000_Delay(uint32_t uDelay);

static void DM9051_Write_Reg(struct rt_spi_device *spi_device, uint8_t Reg_Off, uint8_t spi_data);
static uint8_t DM9051_Read_Reg(struct rt_spi_device *spi_device, uint8_t Reg_Off);
static void DM9051_Write_Mem(struct rt_spi_device *spi_device, uint8_t *pu8data, rt_uint16_t datalen);
static void DM9051_Read_Mem(struct rt_spi_device *spi_device, uint8_t *pu8data, rt_uint16_t datalen);


#if (DM9000_DEBUG == 1)
#define DM9000_TRACE rt_kprintf
#else
#define DM9000_TRACE(...)
#endif

#if (DM9000_MSG == 1)
#define DM9000_TRACE2 rt_kprintf
#else
#define DM9000_TRACE2(...)
#endif

/* Global variables ----------------------------------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------------------------------------*/
static struct dm9051_eth  dm9051_dev;
static struct dm9051_tx_node *dm9051_send_queue_tail = RT_NULL;

/* Global functions ----------------------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------------------------------------*/
/* DM9051_Write_Reg()                                                                                       */
/*                                                                                                          */
/* SPI write command: bit7 = 1,                                                                             */
/*                   bit[6:0] = Reg. offset addr                                                            */
/*----------------------------------------------------------------------------------------------------------*/
static void DM9051_Write_Reg(struct rt_spi_device *spi_device, uint8_t Reg_Off, uint8_t spi_data)
{
    uint8_t buffer[2];

    buffer[0] = (Reg_Off | 0x80);
    buffer[1] = spi_data;
    rt_spi_send(spi_device, buffer, 2);
    return;
}

/* -----------------------------------------------------*/
/* DM9051_Read_Reg()                                    */
/*                                                      */
/* SPI read command: bit7 = 0,                          */
/*                   bit[6:0] = Reg. offset addr        */
/*------------------------------------------------------*/
static uint8_t DM9051_Read_Reg(struct rt_spi_device *spi_device, uint8_t Reg_Off)
{
    uint8_t send_buffer[2];
    uint8_t recv_buffer[1];
    uint32_t send_size = 1;

    if (Reg_Off & 0x80)
    {
        send_size = 2;
    }

    send_buffer[0] = Reg_Off;
    send_buffer[1] = 0x0;
    rt_spi_send_then_recv(spi_device, send_buffer, send_size, recv_buffer, 1);
    return recv_buffer[0];
}

/* -----------------------------------------------------*/
/* DM9051_Write_Mem()                                   */
/*                                                      */
/* DM9051 burst write command: SPI_WR_BURST = 0xF8      */
/*                                                      */
/*------------------------------------------------------*/
static void DM9051_Write_Mem(struct rt_spi_device *spi_device, uint8_t *pu8data, rt_uint16_t datalen)
{
    /*  Send the array to the slave */
    uint8_t burstcmd = SPI_WR_BURST;

    rt_spi_send_then_send(spi_device, &burstcmd, 1, pu8data, datalen);
}

/* -----------------------------------------------------*/
/* DM9051_Read_Mem()                                    */
/*                                                      */
/* DM9051 burst read command: SPI_RD_BURST = 0x72       */
/*                                                      */
/*------------------------------------------------------*/
static void DM9051_Read_Mem(struct rt_spi_device *spi_device, uint8_t *pu8data, rt_uint16_t datalen)
{
    /*  Read SPI_Data_Array back from the slave */
    uint8_t burstcmd = SPI_RD_BURST;

    rt_spi_send_then_recv(spi_device, &burstcmd, 1, pu8data, datalen);
}

/***********************************************************************************************************
    * @brief  Read function of PHY.
    * @param  uReg: PHY register
    * @retval uData: Data of register
    ***********************************************************************************************************/
static rt_uint16_t phy_read(rt_device_t dev, struct rt_spi_device *spi_device, rt_uint32_t uReg)
{
    rt_uint16_t uData;

    /* lock DM9051                                                                                            */
    rt_mutex_take(&((struct dm9051_eth *)dev)->lock, RT_WAITING_FOREVER);

    /* Fill the phyxcer register into REG_0C                                                                  */
    DM9051_Write_Reg(spi_device, DM9000_EPAR, DM9000_PHY | uReg);
    DM9051_Write_Reg(spi_device, DM9000_EPCR, 0xc); /* Issue phyxcer read command                               */

    while (DM9051_Read_Reg(spi_device, DM9000_EPCR) & EPCR_ERRE)
    ;

    DM9051_Write_Reg(spi_device, DM9000_EPCR, 0x0); /* Clear phyxcer read command                               */
    uData = (DM9051_Read_Reg(spi_device, DM9000_EPDRH) << 8) | DM9051_Read_Reg(spi_device, DM9000_EPDRL);
    /* unlock DM9051                                                                                          */
    rt_mutex_release(&((struct dm9051_eth *)dev)->lock);

    return uData;
}

/*******************************************************************************
* Function Name  : phy_write
* Description    : Write a word to phyxcer
* Input          : - reg: reg
*                  - value: data
* Output         : None
* Return         : None
* Attention         : None
*******************************************************************************/
static void phy_write(rt_device_t dev, struct rt_spi_device *spi_device, uint16_t reg, uint16_t value)
{
    /* lock DM9051                                                                                            */
    rt_mutex_take(&((struct dm9051_eth *)dev)->lock, RT_WAITING_FOREVER);
    /* Fill the phyxcer register into REG_0C                                                                */
    DM9051_Write_Reg(spi_device, DM9000_EPAR, DM9000_PHY | reg);

    /* Fill the written data into REG_0D & REG_0E                                                           */
    DM9051_Write_Reg(spi_device, DM9000_EPDRL, (value & 0xff));
    DM9051_Write_Reg(spi_device, DM9000_EPDRH, ((value >> 8) & 0xff));
    DM9051_Write_Reg(spi_device, DM9000_EPCR, 0xa); /* Issue phyxcer write command                                   */

    while (DM9051_Read_Reg(spi_device, DM9000_EPCR) & 0x1)
    {
        _dm9000_Delay(1);
    }; /*Wait complete */

    DM9051_Write_Reg(spi_device, DM9000_EPCR, 0x0); /* Clear phyxcer write command                                   */
    /* unlock DM9051                                                                                          */
    rt_mutex_release(&((struct dm9051_eth *)dev)->lock);
}

/***********************************************************************************************************
  * @brief  Set PHY mode.
  * @param  uMediaMode:
  *         @DM9000_AUTO Auto negotiation
  *         @DM9000_10MHD 10MHz, Half duplex
  *         @DM9000_10MFD 10MHz, Full duplex
  *         @DM9000_100MHD 100MHz, Half duplex
  *         @DM9000_100MFD 100MHz, Full duplex
  * @retval None
  ***********************************************************************************************************/
static void phy_mode_set(rt_device_t dev, struct rt_spi_device *spi_device, rt_uint32_t uMediaMode)
{
    uint16_t phy_reg4 = 0x01e1, phy_reg0 = 0x1000;

    if (!(uMediaMode & DM9000_AUTO))
    {
        switch (uMediaMode)
        {
            case DM9000_10MHD:
            {
                phy_reg4 = 0x21;
                phy_reg0 = 0x0000;
                break;
            }
            case DM9000_10MFD:
            {
                phy_reg4 = 0x41;
                phy_reg0 = 0x1100;
                break;
            }
            case DM9000_100MHD:
            {
                phy_reg4 = 0x81;
                phy_reg0 = 0x2000;
                break;
            }
            case DM9000_100MFD:
            {
                phy_reg4 = 0x101;
                phy_reg0 = 0x3100;
                break;
            }
            case DM9000_10M:
            {
                phy_reg4 = 0x61;
                phy_reg0 = 0x1200;
                break;
            }
        }
        phy_write(dev, spi_device, 4, phy_reg4); /* Set PHY media mode                                               */

        phy_write(dev, spi_device, 0, phy_reg0); /* Tmp                                                              */
    }
}

static void dm9051_set_mac_address(struct rt_spi_device *spi_device, rt_uint8_t *addr)
{
    int i, oft;
    /* set mac address                                                                                        */
    for (i = 0, oft = DM9000_PAR; i < 6; i++, oft++)
    {
        DM9051_Write_Reg(spi_device, oft, addr[i]);
    }

    /* set multicast address      */
    for (i = 0; i < 8; i++)
        DM9051_Write_Reg(spi_device, DM9000_MAR + i, (7 == i) ? 0x80 : 0x00);

    DM9000_TRACE2("DM9051 MAC: ");
    for (i = 0, oft = DM9000_PAR; i < 6; i++, oft++)
    {
        DM9000_TRACE2("%02X:", DM9051_Read_Reg(spi_device, oft));
    }
    DM9000_TRACE2("\n");
}

static rt_err_t dm9051_control(rt_device_t dev, int cmd, void *args)
{
    struct dm9051_eth *dm9051 = (struct dm9051_eth *)dev;
    struct rt_spi_device *spi_device = dm9051->spi_device;

    switch (cmd)
    {
        case NIOCTL_GADDR:
            if (args)
            {
                rt_memcpy(args, dm9051->dev_addr, 6);
            }
            else
                return -RT_ERROR;
            break;
        case NIOCTL_SADDR:
        /* set mac address */
        if (args)
        {
            rt_memcpy(dm9051->dev_addr, args, 6);
            dm9051_set_mac_address(spi_device, dm9051->dev_addr);
            rt_kprintf("set mac address: %02x:%02x:%02x:%02x:%02x:%02x\n",
                       dm9051->dev_addr[0], dm9051->dev_addr[1], dm9051->dev_addr[2],
                       dm9051->dev_addr[3], dm9051->dev_addr[4], dm9051->dev_addr[5]);
        }
        else
            return -RT_ERROR;

        break;
        default:
            break;
    }
    return RT_EOK;
}

/***********************************************************************************************************
  * @brief  Close DM9000.
  * @retval Always 0
  ***********************************************************************************************************/
static rt_err_t dm9051_open(rt_device_t dev, rt_uint16_t oflag)
{
    return RT_EOK;
}

static rt_err_t dm9000_close(rt_device_t dev)
{
    struct dm9051_eth *dm9051 = (struct dm9051_eth *)dev;
    struct rt_spi_device *spi_device = dm9051->spi_device;
    /* RESET devie                                                                                       */
    phy_write(dev, spi_device, 0, 0x8000);               /* PHY RESET                                    */
    DM9051_Write_Reg(spi_device, DM9000_GPR, 0x01); /* Power-Down PHY                                    */
    DM9051_Write_Reg(spi_device, DM9051_IMR, 0x80); /* Disable all interrupt                             */
    DM9051_Write_Reg(spi_device, DM9000_RCR, 0x00); /* Disable RX                                        */

    return RT_EOK;
}

static rt_size_t dm9051_write(rt_device_t dev, rt_off_t pos, const void *buffer, rt_size_t size)
{
    rt_set_errno(-RT_ENOSYS);
    return 0;
}

static rt_size_t dm9051_read(rt_device_t dev, rt_off_t pos, void *buffer, rt_size_t size)
{
    rt_set_errno(-RT_ENOSYS);
    return RT_EOK;
}

static void dm9051_tx_queue_init(struct dm9051_tx_node *queue)
{
    queue->next = queue;
    queue->buff = RT_NULL;
    dm9051_send_queue_tail = queue;
}

static struct dm9051_tx_node *dm9051_dequeue(struct dm9051_tx_node *queue)
{
    struct dm9051_tx_node *result;

    result = queue->next;
    queue->next = queue->next->next;
    if (queue->next == queue)
    {
        dm9051_send_queue_tail = queue;
    }
    return result;
}

static int dm9051_queue_empty(struct dm9051_tx_node *queue)
{
    return queue->next == queue;
}

static void dm9051_enqueue(struct dm9051_tx_node *newque)
{
    newque->next = dm9051_send_queue_tail->next;
    dm9051_send_queue_tail->next = newque;
    dm9051_send_queue_tail = newque;
}

static struct dm9051_tx_node *dm9051_create_node()
{
    struct dm9051_tx_node *temp_node;

    temp_node = (struct dm9051_tx_node *)rt_malloc(sizeof(struct dm9051_tx_node));
    if (temp_node == RT_NULL)
    {
        rt_kprintf("malloc temp_node failed\n");
    }
    return temp_node;
}

static struct pbuf *dm9051_create_pbuf(struct pbuf *p)
{
    struct pbuf *q;


    if ((pbuf_type)p->type_internal == PBUF_REF)
    {
        p->type_internal = PBUF_RAM;
    }

    q = pbuf_alloc(PBUF_RAW, p->tot_len, (pbuf_type)p->type_internal);
    if (q == RT_NULL)
    {
        rt_kprintf("out of pbuf\n");
    }
    else
    {
        pbuf_copy(q, p);
    }
    return q;
}

static void dm9051_free_node(struct dm9051_tx_node *node)
{
    pbuf_free(node->buff);
    rt_free(node);
}

static void dm9051_send_packet(struct rt_spi_device *spi_device, struct dm9051_tx_node *queue)
{
    struct dm9051_tx_node *temp_node;
    uint16_t SendLength;
    struct pbuf *pbuf_temp;

    temp_node = dm9051_dequeue(queue);
    if (temp_node != RT_NULL)
    {
        SendLength = temp_node->buff->tot_len;
        while (DM9051_Read_Reg(spi_device, DM9000_TCR) & DM9000_TCR_SET)
        ;

        {
            DM9051_Write_Reg(spi_device, DM9051_TXPLL, SendLength & 0xff);
            DM9051_Write_Reg(spi_device, DM9051_TXPLH, SendLength >> 8 & 0xff);
            for (pbuf_temp = temp_node->buff; pbuf_temp != NULL; pbuf_temp = pbuf_temp->next)
            {
                DM9051_Write_Mem(spi_device, (u8_t *)pbuf_temp->payload, pbuf_temp->len);
            }
            DM9051_Write_Reg(spi_device, DM9000_TCR, TCR_TXREQ); /* Cleared after TX complete */
            dm9051_free_node(temp_node);
        }
    }
    return;
}

#if defined (RT_USING_DM9051_ISR)
static void dm9051_isr_thread_proc(void *args)
{
    uint16_t int_status;
    struct rt_spi_device *spi_device = dm9051_dev.spi_device;

#if 1
    while (rt_sem_take(&dm9051_dev.sem_dm9051_isr, RT_WAITING_FOREVER) == RT_EOK)
    {
        /* Clear interrupt flag */
        DM9051_Write_Reg(spi_device, DM9051_IMR, DM9000_IMR_OFF);

        /* Got DM9051 interrupt status  */
        int_status = DM9051_Read_Reg(spi_device, DM9051_ISR);

        /* Clear ISR status */
        DM9051_Write_Reg(spi_device, DM9051_ISR, (uint8_t)int_status);

        if (int_status & ISR_PRS)
        {

            /* disable receive interrupt                                                                            */
            dm9051_dev.imr_all &= ~IMR_PRM;

            /* a frame has been received                                                                            */
            net_device_ready(&dm9051_dev.parent);
        }
        /* Transmit Interrupt check                                                                               */
        if (int_status & ISR_PTS)
        {
            {
                rt_mutex_take(&dm9051_dev.tx_lock, RT_WAITING_FOREVER);
                if (!dm9051_queue_empty(&dm9051_dev.tx_queue))
                {
                    dm9051_send_packet(spi_device, &dm9051_dev.tx_queue);
                }
                rt_mutex_release(&dm9051_dev.tx_lock);
            }
        }
        /* enable dm9000a interrupt    */
        DM9051_Write_Reg(spi_device, DM9051_IMR, dm9051_dev.imr_all);
    }
#endif
}
#else

static void dm9051_rx_proc(void *args)
{
    struct rt_spi_device *spi_device = dm9051_dev.spi_device;
    uint16_t int_status;

    while (1)
    {
        /* Clear interrupt flag */
        DM9051_Write_Reg(spi_device, DM9051_IMR, DM9000_IMR_OFF);
        /* Got DM9051 interrupt status  */
        int_status = DM9051_Read_Reg(spi_device, DM9051_ISR);

        /* Clear ISR status */
        DM9051_Write_Reg(spi_device, DM9051_ISR, (uint8_t)int_status);
        if (int_status & ISR_PRS)
        {

            /* disable receive interrupt                                                                            */
            dm9051_dev.imr_all &= ~IMR_PRM;

            /* a frame has been received                                                                            */
            net_device_ready(&dm9051_dev.parent);
        }
        else
        {
            rt_thread_delay(1);
        }
        DM9051_Write_Reg(spi_device, DM9051_IMR, dm9051_dev.imr_all);
    }
}
#endif
/***********************************************************************************************************
  * @brief  DM9051 init function.
  * @retval -1 or 0
  ***********************************************************************************************************/
static rt_err_t dm9051_init(rt_device_t dev)
{
    int i = 0, lnk;

    struct dm9051_eth *dm9051 = (struct dm9051_eth *)dev;
    struct rt_spi_device *spi_device = dm9051->spi_device;

    dm9051_dev.type = TYPE_DM9051;
#ifdef FORCE_10M
    dm9051_dev.mode = DM9000_10M;
#else
    dm9051_dev.mode = DM9000_AUTO;
#endif /* FORCE_10M */

    dm9051_dev.packet_cnt = 0;
    dm9051_dev.queue_packet_len = 0;

    /* SRAM Tx/Rx pointer automatically return to start address                                               */
    /* Packet Transmitted, Packet Received                                                                    */
    dm9051_dev.imr_all = IMR_FULL;

    /* RESET device                                                                                           */
    DM9051_Write_Reg(spi_device, DM9000_NCR, DM9000_REG_RESET);
    _dm9000_Delay(1); /* 1us */
    DM9051_Write_Reg(spi_device, DM9000_NCR, 0);

    /* RESET device                                                                                           */
    DM9051_Write_Reg(spi_device, DM9000_NCR, DM9000_REG_RESET);
    _dm9000_Delay(1);
    DM9051_Write_Reg(spi_device, DM9000_NCR, 0);

    DM9051_Write_Reg(spi_device, DM9000_GPCR, GPCR_GEP_CNTL);
    DM9051_Write_Reg(spi_device, DM9000_GPR, 0x00); /* Power on PHY */

    rt_thread_delay(1);

    /* Set PHY                                                                                                */
    phy_mode_set(dev, spi_device, dm9051_dev.mode);

    /* Activate DM9051                                                                                        */
    /* Setup DM9051 Registers */
    DM9051_Write_Reg(spi_device, DM9000_NCR, NCR_DEFAULT);
    DM9051_Write_Reg(spi_device, DM9000_TCR, TCR_DEFAULT);
    DM9051_Write_Reg(spi_device, DM9000_RCR, RCR_DEFAULT);
    DM9051_Write_Reg(spi_device, DM9000_BPTR, BPTR_DEFAULT);
    DM9051_Write_Reg(spi_device, DM9000_FCTR, 0x3A);
    DM9051_Write_Reg(spi_device, DM9000_FCR, FCR_DEFAULT);
    DM9051_Write_Reg(spi_device, DM9000_SMCR, SMCR_DEFAULT);
    DM9051_Write_Reg(spi_device, DM9000_TCR2, DM9000_TCR2_SET);
    DM9051_Write_Reg(spi_device, DM9051_INTR, 0x1);

    /* Clear status */
    DM9051_Write_Reg(spi_device, DM9000_NSR, NSR_CLR_STATUS);
    DM9051_Write_Reg(spi_device, DM9051_ISR, ISR_CLR_STATUS);

    if ((dm9051_dev.mode == DM9000_AUTO) || (dm9051_dev.mode == DM9000_10M))
    {
        while (!(phy_read(dev, spi_device, 1) & 0x20))
        {
            /* autonegation complete bit                                                                          */
            _dm9000_Delay(2);
            i++;
            if (i == 255)
            {
                DM9000_TRACE2("could not establish link\n");
                break;
            }
        }
    }

    /* see what we've got                                                                                     */
    lnk = phy_read(dev, spi_device, 17) >> 12;
    DM9000_TRACE2("operating at ");
    switch (lnk)
    {
    case 1:
        DM9000_TRACE2("10M half duplex ");
        break;
    case 2:
        DM9000_TRACE2("10M full duplex ");
        break;
    case 4:
        DM9000_TRACE2("100M half duplex ");
        break;
    case 8:
        DM9000_TRACE2("100M full duplex ");
        break;
    default:
        DM9000_TRACE2("unknown: %d ", lnk);
        break;
    }
    DM9000_TRACE2("mode\n");

    DM9051_Write_Reg(spi_device, DM9051_IMR, dm9051_dev.imr_all); /* Enable TX/RX interrupt mask                    */
    DM9051_Write_Reg(spi_device, DM9000_RCR, (RCR_DEFAULT | RCR_RXEN)); /* Enable RX */
#if defined (RT_USING_DM9051_ISR)
    rt_thread_t thread_dm9051_isr;

    thread_dm9051_isr = rt_thread_create("dm9051_isr", (void *)dm9051_isr_thread_proc, RT_NULL, 0x1000, 132, 20);
    if (thread_dm9051_isr != RT_NULL)
    {
        rt_thread_startup(thread_dm9051_isr);
    }
#else
    rt_thread_t thread_dm9051_rx;

    thread_dm9051_rx = rt_thread_create("dm9051_rx", (void *)dm9051_rx_proc, RT_NULL, 0x1000, 132, 20);
    if (thread_dm9051_rx != RT_NULL)
    {
        rt_thread_startup(thread_dm9051_rx);
    }
#endif
    return 0;
}

/***********************************************************************************************************
  * @brief  Interrupt service routine.
  * @retval None
  ***********************************************************************************************************/
#if defined (RT_USING_DM9051_ISR)
void dm9051_isr(void)
{
    rt_sem_release(&dm9051_dev.sem_dm9051_isr);
}
#endif

  /***********************************************************************************************************
  * @brief  Tx function.
  * @param  pbuf: buffer link list
  * @retval Always 0
  ***********************************************************************************************************/
static rt_err_t dm9051_tx(rt_device_t dev, struct pbuf *p)
{

    struct dm9051_eth *dm9051 = (struct dm9051_eth *)dev;
    struct rt_spi_device *spi_device = dm9051->spi_device;
    struct dm9051_tx_node *temp_node;

#ifdef ETH_TX_DUMP
    rt_size_t dump_count = 0;
    rt_uint8_t *dump_ptr;
    rt_size_t dump_i;
#endif


    /* lock DM9051                                                                                            */
    rt_mutex_take(&((struct dm9051_eth *)dev)->lock, RT_WAITING_FOREVER);

    /* disable dm9000a interrupt                                                                              */
    DM9051_Write_Reg(spi_device, DM9051_IMR, IMR_PAR);
#if 1
    {
        rt_mutex_take(&dm9051_dev.tx_lock, RT_WAITING_FOREVER);
        temp_node = dm9051_create_node();
        if (temp_node == RT_NULL)
        {
            rt_mutex_release(&((struct dm9051_eth *)dev)->lock);
            rt_mutex_release(&dm9051_dev.tx_lock);
            /* enable dm9000a interrupt    */
            DM9051_Write_Reg(spi_device, DM9051_IMR, dm9051_dev.imr_all);
            return -RT_ENOMEM;
        }

        temp_node->buff = dm9051_create_pbuf(p);
        if (temp_node->buff != RT_NULL)
        {
            dm9051_enqueue(temp_node);
        }
        else
        {
            rt_free(temp_node);
            rt_mutex_release(&((struct dm9051_eth *)dev)->lock);
            rt_mutex_release(&dm9051_dev.tx_lock);
            /* enable dm9000a interrupt    */
            DM9051_Write_Reg(spi_device, DM9051_IMR, dm9051_dev.imr_all);
            return -RT_ENOMEM;
        }
        if (!dm9051_queue_empty(&dm9051_dev.tx_queue))
        {
            dm9051_send_packet(spi_device, &dm9051_dev.tx_queue);
        }
        rt_mutex_release(&dm9051_dev.tx_lock);
    }
#endif
    /* enable dm9000a interrupt    */
    DM9051_Write_Reg(spi_device, DM9051_IMR, dm9051_dev.imr_all);
    /* unlock DM9051 device                                                                                   */
    rt_mutex_release(&((struct dm9051_eth *)dev)->lock);
    return RT_EOK;
}

  /***********************************************************************************************************
  * @brief  Rx function.
  * @retval pbuf
  ***********************************************************************************************************/
struct pbuf *dm9051_rx(rt_device_t dev)
{
    struct dm9051_eth *dm9051 = (struct dm9051_eth *)dev;
    struct rt_spi_device *spi_device = dm9051->spi_device;

    uint8_t rxbyte;

    uint16_t calc_MRR;
    uint16_t rx_len = 0;

    uint8_t *databyte = 0;
    struct pbuf *p;

    /* init p pointer                                                                                         */
    p = NULL;
    /* lock DM9000                                                                                            */
    rt_mutex_take(&((struct dm9051_eth *)dev)->lock, RT_WAITING_FOREVER);

    /* disable dm9000a interrupt                                                                              */
    DM9051_Write_Reg(spi_device, DM9051_IMR, IMR_PAR);
    /* Check packet ready or not                                                                              */
    rxbyte = DM9051_Read_Reg(spi_device, DM9051_MRCMDX);
    rxbyte = DM9051_Read_Reg(spi_device, DM9051_MRCMDX);

    if ((rxbyte != 1) && (rxbyte != 0))
    {
        DM9000_TRACE2("dm9000 rx: rx error, stop device\n");
        /* Reset RX FIFO pointer */
        DM9051_Write_Reg(spi_device, DM9000_RCR, RCR_DEFAULT); /* RX disable */
        DM9051_Write_Reg(spi_device, DM9051_MPCR, 0x01);       /* Reset RX FIFO pointer */
        _dm9000_Delay(2);
        DM9051_Write_Reg(spi_device, DM9000_RCR, (RCR_DEFAULT | RCR_RXEN)); /* RX Enable */

        /* restore receive interrupt                                                                              */
        dm9051_dev.imr_all |= IMR_PRM;
        DM9051_Write_Reg(spi_device, DM9051_IMR, dm9051_dev.imr_all);
        /* unlock DM9051                                                                                          */
        rt_mutex_release(&((struct dm9051_eth *)dev)->lock);
        return 0;
    }

    if (rxbyte)
    {
        uint16_t rx_status;

        uint8_t ReceiveData[4];

#ifdef FifoPointCheck
        /* Save RX SRAM pointer */
        calc_MRR = (DM9051_Read_Reg(spi_device, DM9051_MRRH) << 8) + DM9051_Read_Reg(spi_device, DM9051_MRRL);
#endif  /* FifoPointCheck */

        DM9051_Read_Reg(spi_device, DM9051_MRCMDX); /* dummy read */

        DM9051_Read_Mem(spi_device, ReceiveData, 4);

        rx_status = ReceiveData[0] + (ReceiveData[1] << 8);
        rx_len = ReceiveData[2] + (ReceiveData[3] << 8);


        /* allocate buffer           */
        p = pbuf_alloc(PBUF_LINK, rx_len, PBUF_RAM);
        if (p != NULL)
        {
            DM9051_Read_Mem(spi_device, (u8_t *)p->payload, rx_len);
        }
        else
        {
            DM9000_TRACE2("dm9000 rx: no pbuf\n");
            /* no pbuf, discard data from DM9000  */
            DM9051_Read_Mem(spi_device, databyte, rx_len);
        }

#ifdef FifoPointCheck
        /* when calc_MRR > 0x3fff, calc_MRR restart at 0x0c00 */
        calc_MRR += (rx_len + 4);
        if (calc_MRR > 0x3fff)
        calc_MRR -= 0x3400;

        if (calc_MRR != ((DM9051_Read_Reg(spi_device, DM9051_MRRH) << 8) + DM9051_Read_Reg(spi_device, DM9051_MRRL)))
        {

            rt_kprintf("DM9K MRR Error!!\n");
            rt_kprintf("Predicut RX Read pointer = 0x%X, Current pointer = 0x%X\n",
                calc_MRR, ((DM9051_Read_Reg(spi_device, DM9051_MRRH) << 8) + DM9051_Read_Reg(spi_device, DM9051_MRRL)));

            /*reset register of DM9051_MRRH and DM9051_MRRL */
            DM9051_Write_Reg(spi_device, DM9051_MRRH, (calc_MRR >> 8) & 0xff);
            DM9051_Write_Reg(spi_device, DM9051_MRRL, calc_MRR & 0xff);
        }
#endif

        if ((rx_status & 0xbf00) || (rx_len < 0x40) || (rx_len > DM9000_PKT_MAX))
        {
            DM9000_TRACE2("rx error: status %04x, rx_len: %d\n", rx_status, rx_len);

            if (rx_status & 0x100)
            {
                DM9000_TRACE2("rx fifo error\n");
            }
            if (rx_status & 0x200)
            {
                DM9000_TRACE2("rx crc error\n");
            }
            if (rx_status & 0x8000)
            {
                DM9000_TRACE2("rx length error\n");
            }
            if (rx_len > DM9000_PKT_MAX)
            {
                DM9000_TRACE2("rx length too big\n");
            }
            pbuf_free(p);
            p = NULL;
        }
    }
    else
    {

        /* clear packet received latch status                                                                   */

        /* restore receive interrupt                                                                            */
        dm9051_dev.imr_all |= IMR_PRM;
        DM9051_Write_Reg(spi_device, DM9051_IMR, dm9051_dev.imr_all);
    }

    /* unlock DM9051                                                                                          */
    rt_mutex_release(&((struct dm9051_eth *)dev)->lock);
    return p;
}


/*********************************************************************************************************/ /**
  * @brief  uS level delay function.
  * @param  uDelay: Delay time
  * @retval None
  ***********************************************************************************************************/
static void _dm9000_Delay(uint32_t uDelay)
{
    while (uDelay--)
        ;
}

#ifdef RT_USING_DEVICE_OPS
const static struct rt_device_ops dm9051_ops = {
    .init    = dm9051_init,
    .open    = dm9051_open,
    .close   = dm9000_close,
    .read    = dm9051_read,
    .write   = dm9051_write,
    .control = dm9051_control
};
#endif


rt_err_t dm9051_attach(const char *spi_device_name)
{
    struct rt_spi_device *spi_device;
    uint32_t value;
    int i, oft;

#if defined (RT_USING_DM9051_ISR)
    rt_kprintf("using ISR mode.\n");
    gpio_request(DM9051_INT);
    gpio_direction_input(DM9051_INT);
    gpio_set_irq_type(DM9051_INT, IRQ_TYPE_EDGE_FALLING);
    rt_hw_interrupt_install(gpio_to_irq(DM9051_INT), (void *)dm9051_isr,
                            RT_NULL, RT_NULL);

    gpio_irq_enable(gpio_to_irq(DM9051_INT));
    gpio_release(DM9051_INT);
#else
    rt_kprintf("using POLL mode.\n");
#endif

    spi_device = (struct rt_spi_device *)rt_device_find(spi_device_name);
    if (spi_device == RT_NULL)
    {
        rt_kprintf("spi device %s not found!\r\n", spi_device_name);
        return -RT_ENOSYS;
    }

    /* config spi */
    {
        struct rt_spi_configuration cfg;

        cfg.data_width = 8;
        cfg.mode = RT_SPI_MODE_3 | RT_SPI_MSB;
        cfg.max_hz = 50000 * 1000;
        cfg.sample_delay = 1;
        rt_spi_configure(spi_device, &cfg);
    }

    memset(&dm9051_dev, 0, sizeof(dm9051_dev));

    dm9051_dev.spi_device = spi_device;
    rt_mutex_init(&dm9051_dev.lock, "dm9051", RT_IPC_FLAG_FIFO);
    rt_mutex_init(&dm9051_dev.tx_lock, "dm9051_tx", RT_IPC_FLAG_FIFO);
#if defined (RT_USING_DM9051_ISR)
    rt_sem_init(&dm9051_dev.sem_dm9051_isr, "sem_dm9051_isr", 0, 0);
#endif


    DM9051_Write_Reg(spi_device, DM9000_NCR, 0x1);
    rt_thread_delay(1);

    DM9051_Write_Reg(spi_device, DM9051_PBCR, PBCR_MAXDRIVE); /* BUS Driving capability */
    /* identfy DM9051                                                                                       */
    value |= DM9051_Read_Reg(spi_device, DM9000_VIDH) << 8;
    value |= DM9051_Read_Reg(spi_device, DM9000_PIDL) << 16;
    value |= DM9051_Read_Reg(spi_device, DM9000_PIDH) << 24;
    value |= DM9051_Read_Reg(spi_device, DM9000_VIDL);

    if (value != DM9051_ID)
    {
    /* one more retry */
        value = DM9051_Read_Reg(spi_device, DM9000_VIDL);
        value |= DM9051_Read_Reg(spi_device, DM9000_VIDH) << 8;
        value |= DM9051_Read_Reg(spi_device, DM9000_PIDL) << 16;
        value |= DM9051_Read_Reg(spi_device, DM9000_PIDH) << 24;

        if (value != DM9051_ID)
        {
            DM9000_TRACE2("DM9051 id: 0x%x\n", value);
            return -1;
        }
    }
    DM9000_TRACE2("DM9051 id: 0x%x\n", value);

    dm9051_dev.dev_addr[0] = 0x00;
    dm9051_dev.dev_addr[1] = 0x12;
    dm9051_dev.dev_addr[2] = 0x34;
    dm9051_dev.dev_addr[3] = 0x56;
    dm9051_dev.dev_addr[4] = 0x89;
    dm9051_dev.dev_addr[5] = 0xaa;

    /* set mac address                                                                                        */
    dm9051_set_mac_address(spi_device, dm9051_dev.dev_addr);

    dm9051_tx_queue_init(&dm9051_dev.tx_queue);

    dm9051_dev.parent.parent.type = RT_Device_Class_NetIf;
#ifdef RT_USING_DEVICE_OPS
    dm9051_dev.parent.parent.ops = &dm9051_ops;
#else
    dm9051_dev.parent.parent.init = dm9051_init;
    dm9051_dev.parent.parent.open = dm9051_open;
    dm9051_dev.parent.parent.close = dm9000_close;
    dm9051_dev.parent.parent.read = dm9051_read;
    dm9051_dev.parent.parent.write = dm9051_write;
    dm9051_dev.parent.parent.control = dm9051_control;
#endif

    dm9051_dev.parent.net.eth.eth_rx = dm9051_rx;
    dm9051_dev.parent.net.eth.eth_tx = dm9051_tx;


    unsigned char flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP;
#ifdef RT_LWIP_IGMP
    flags |= NETIF_FLAG_IGMP;
#endif

    net_device_init(&(dm9051_dev.parent), "d0", flags, NET_DEVICE_DM9051);

    return RT_EOK;

}
