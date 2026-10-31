/***********************************************************************************************************
 * @file    dm9051_stm.h
 * @version V1.02
 * @date    2014/6/10
 * @brief   The header file of DM9051 register.
 *************************************************************************************************************
 *
 * <h2><center>Copyright (C) 2014 DAVICOM Semiconductor Inc. All rights reserved</center></h2>
 *
 ***********************************************************************************************************
 * History:
 *      V1.02 - 2014/8/8
 *      1. Added DM9051_PBCR, PBCR_MAXDRIVE definition for bus driving.
 */
/* Define to prevent recursive inclusion -------------------------------------------------------------------*/
#ifndef __DM9000_REG_H
#define __DM9000_REG_H

#define FifoPointCheck

/* Includes ------------------------------------------------------------------------------------------------*/
#ifndef uIP_NOOS
#include <stdint.h>

#include <rtthread.h>
#include <drivers/spi.h>
#include <netif/ethernetif.h>
#include "gpio.h"
#endif

/* Settings ------------------------------------------------------------------------------------------------*/

#define DM9000_MSG              (1)
#define DM9000_DEBUG            (1)
#define Rx_Int_enable           0

/* Exported typedef ----------------------------------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------------------------------------*/
#define DM9000_ID           (0x90000A46)    /* DM9000A ID                                                   */
#define DM9051_ID           (0x90510A46)
#define DM9000_PKT_MAX      (1536)          /* Received packet max size                                     */
#define DM9000_PKT_RDY      (0x01)          /* Packet ready to receive                                      */

#define DM9000_NCR          (0x00)
#define DM9000_NSR          (0x01)
#define DM9000_TCR          (0x02)
#define DM9000_TSR1         (0x03)
#define DM9000_TSR2         (0x04)
#define DM9000_RCR          (0x05)
#define DM9000_RSR          (0x06)
#define DM9000_ROCR         (0x07)
#define DM9000_BPTR         (0x08)
#define DM9000_FCTR         (0x09)
#define DM9000_FCR          (0x0A)
#define DM9000_EPCR         (0x0B)
#define DM9000_EPAR         (0x0C)
#define DM9000_EPDRL        (0x0D)
#define DM9000_EPDRH        (0x0E)
#define DM9000_WCR          (0x0F)

#define DM9000_PAR          (0x10)
#define DM9000_MAR          (0x16)

#define DM9000_GPCR         (0x1e)
#define DM9000_GPR          (0x1f)
#define DM9000_TRPAL        (0x22)
#define DM9000_TRPAH        (0x23)
#define DM9000_RWPAL        (0x24)
#define DM9000_RWPAH        (0x25)

#define DM9000_VIDL         (0x28)
#define DM9000_VIDH         (0x29)
#define DM9000_PIDL         (0x2A)
#define DM9000_PIDH         (0x2B)

#define DM9000_CHIPR        (0x2C)
#define DM9000_TCR2         (0x2D)
#define DM9000_OTCR         (0x2E)
#define DM9000_SMCR         (0x2F)

#define DM9000_ETCR         (0x30)    /* early transmit control/status register                             */
#define DM9000_CSCR         (0x31)    /* check sum control register                                         */
#define DM9000_RCSSR        (0x32)    /* receive check sum status register                                  */

#define DM9051_PBCR         0x38
#define DM9051_INTR         0x39
#define DM9051_MPCR         0x55
#define DM9051_MBCR         0x5E

#define DM9051_MRCMDX       (0x70)
#define DM9051_MRCMD        (0x72)
#define DM9051_MRRL         (0x74)
#define DM9051_MRRH         (0x75)
#define DM9051_MWCMDX       (0x76)
#define DM9051_MWCMD        (0x78)
#define DM9051_MWRL         (0x7A)
#define DM9051_MWRH         (0x7B)
#define DM9051_TXPLL        (0x7C)
#define DM9051_TXPLH        (0x7D)
#define DM9051_ISR          (0x7E)
#define DM9051_IMR          (0x7F)

#define DM9000_MRCMDX       (0xF0)
#define DM9000_MRCMD        (0xF2)
#define DM9000_MRRL         (0xF4)
#define DM9000_MRRH         (0xF5)
#define DM9000_MWCMDX       (0xF6)
#define DM9000_MWCMD        (0xF8)
#define DM9000_MWRL         (0xFA)
#define DM9000_MWRH         (0xFB)
#define DM9000_TXPLL        (0xFC)
#define DM9000_TXPLH        (0xFD)
#define DM9000_ISR          (0xFE)
#define DM9000_IMR          (0xFF)

#define CHIPR_DM9000A       (0x19)
#define CHIPR_DM9000B       (0x1B)

#define DM9000_REG_RESET     0x01
#define DM9000_IMR_OFF        0x80
#define DM9000_TCR2_SET       0x90  /* one packet */
#define DM9000_RCR_SET        0x31
#define DM9000_BPTR_SET       0x37
#define DM9000_FCTR_SET       0x38
#define DM9000_FCR_SET        0x28
#define DM9000_TCR_SET        0x01


#define NCR_EXT_PHY         (1 << 7)
#define NCR_WAKEEN          (1 << 6)
#define NCR_FCOL            (1 << 4)
#define NCR_FDX             (1 << 3)
#define NCR_LBK             (3 << 1)
#define NCR_RST             (1 << 0)
#define NCR_DEFAULT     0x0             /* Disable Wakeup */

#define NSR_SPEED           (1 << 7)
#define NSR_LINKST          (1 << 6)
#define NSR_WAKEST          (1 << 5)
#define NSR_TX2END          (1 << 3)
#define NSR_TX1END          (1 << 2)
#define NSR_RXOV            (1 << 1)
#define NSR_CLR_STATUS      (NSR_WAKEST | NSR_TX2END | NSR_TX1END)

#define TCR_TJDIS           (1 << 6)
#define TCR_EXCECM          (1 << 5)
#define TCR_PAD_DIS2        (1 << 4)
#define TCR_CRC_DIS2        (1 << 3)
#define TCR_PAD_DIS1        (1 << 2)
#define TCR_CRC_DIS1        (1 << 1)
#define TCR_TXREQ           (1 << 0)        /* Start TX */
#define TCR_DEFAULT     0x0

#define TSR_TJTO            (1 << 7)
#define TSR_LC              (1 << 6)
#define TSR_NC              (1 << 5)
#define TSR_LCOL            (1 << 4)
#define TSR_COL             (1 << 3)
#define TSR_EC              (1 << 2)

#define RCR_WTDIS           (1 << 6)
#define RCR_DIS_LONG        (1 << 5)
#define RCR_DIS_CRC         (1 << 4)
#define RCR_ALL             (1 << 3)
#define RCR_RUNT            (1 << 2)
#define RCR_PRMSC           (1 << 1)
#define RCR_RXEN            (1 << 0)
#define RCR_DEFAULT     (RCR_DIS_LONG | RCR_DIS_CRC)

#define RSR_RF              (1 << 7)
#define RSR_MF              (1 << 6)
#define RSR_LCS             (1 << 5)
#define RSR_RWTO            (1 << 4)
#define RSR_PLE             (1 << 3)
#define RSR_AE              (1 << 2)
#define RSR_CE              (1 << 1)
#define RSR_FOE             (1 << 0)

#define BPTR_DEFAULT    0x3f

#define FCTR_DEAFULT    0x38

#define FCR_DEFAULT     0xFF

#define SMCR_DEFAULT    0x0

#define PBCR_MAXDRIVE   0x64

#define IMR_PAR             (1 << 7)
#define IMR_LNKCHGI         (1 << 5)
#define IMR_UDRUN           (1 << 4)
#define IMR_ROOM            (1 << 3)
#define IMR_ROM             (1 << 2)
#define IMR_PTM             (1 << 1)
#define IMR_PRM             (1 << 0)
#define IMR_FULL        (IMR_PAR | IMR_LNKCHGI | IMR_UDRUN | IMR_ROOM | IMR_ROM | IMR_PTM | IMR_PRM)
#define IMR_OFF         IMR_PAR
#define IMR_DEFAULT     (IMR_PAR | IMR_PRM | IMR_PTM | IMR_LNKCHGI)

#define ISR_ROOS            (1 << 3)
#define ISR_ROS             (1 << 2)
#define ISR_PTS             (1 << 1)
#define ISR_PRS             (1 << 0)

#define ISR_CLR_STATUS      (ISR_ROOS | ISR_ROS | ISR_PTS | ISR_PRS)

#define EPCR_REEP           (1 << 5)
#define EPCR_WEP            (1 << 4)
#define EPCR_EPOS           (1 << 3)
#define EPCR_ERPRR          (1 << 2)
#define EPCR_ERPRW          (1 << 1)
#define EPCR_ERRE           (1 << 0)

#define GPCR_GEP_CNTL       (1 << 0)


#define SPI_WR_BURST        0xF8
#define SPI_RD_BURST        0x72

#define SPI_READ            0x03
#define SPI_WRITE           0x04
#define  SPI_WRITE_BUFFER   0x05        /* Send a series of bytes from the */
                                       /* Master to the Slave */
#define  SPI_READ_BUFFER    0x06        /* Send a series of bytes from the Slave */
                                       /* to the Master */

#define MAX_BUFFER_SIZE     1600           /* Maximum buffer Master will send */

/* DM9051 */
#define GPIO_DM9051_CS                          GPIOC
#define RCC_APB2Periph_GPIO_DM9051_CS           RCC_APB2Periph_GPIOC
#define GPIO_Pin_DM9051_CS                      GPIO_Pin_0
#define DM9051_SPI                              ((SPI_TypeDef*)SPI1_BASE)
#define dm9051_IRQHandler                       (EXTI9_5_IRQHandler)

#ifdef SPI_DMA
/* SPI DMA */
#define SPI_MASTER_DMA               DMA1
#define SPI_MASTER_DMA_CLK           RCC_AHBPeriph_DMA1
#define SPI_MASTER_Rx_DMA_Channel    DMA1_Channel2
#define SPI_MASTER_Rx_DMA_FLAG       DMA1_FLAG_TC2
#define SPI_MASTER_Tx_DMA_Channel    DMA1_Channel3
#define SPI_MASTER_Tx_DMA_FLAG       DMA1_FLAG_TC3
#define SPI_MASTER_DR_Base           (SPI1_BASE + 0xC)      /* 0x4001300C */

#define DMA_CNDTR2_RX_REG            0x40020020
#define DMA_CNDTR3_TX_REG            0x40020034

#endif  /* uIP_NOOS */

/* Select SPI DM9051: Chip Select pin low  */
#define SPI_DM9051_CS_LOW()       GPIO_ResetBits(GPIO_DM9051_CS, GPIO_Pin_DM9051_CS); \
                                                                    GPIO_SetBits(GPIOF, GPIO_Pin_10)
/* Deselect SPI DM9051: Chip Select pin high */
#define SPI_DM9051_CS_HIGH()      GPIO_SetBits(GPIO_DM9051_CS, GPIO_Pin_DM9051_CS); \
                                                                    GPIO_ResetBits(GPIOF, GPIO_Pin_10)

/* Exported macro ------------------------------------------------------------------------------------------*/
/* Exported variables --------------------------------------------------------------------------------------*/
/* Exported functions --------------------------------------------------------------------------------------*/

#define MAX_TX_PACKAGE_SIZE    (1536)

/* start with recbuf at 0/ */
#define RXSTART_INIT    0x0
/* receive buffer end */
#define RXSTOP_INIT     (0x1FFF - MAX_TX_PACKAGE_SIZE*2) - 1
/* start TX buffer at 0x1FFF-0x0600, pace for one full ethernet frame (~1500 bytes) */

#define TXSTART_INIT    (0x1FFF - MAX_TX_PACKAGE_SIZE*2)
/* stp TX buffer at end of mem */
#define TXSTOP_INIT     0x1FFF

/* max frame length which the conroller will accept: */
#define MAX_FRAMELEN    1518

#define MAX_ADDR_LEN    6

struct dm9051_tx_node
{
    struct dm9051_tx_node *next;
    struct pbuf *buff;
};

/* Private typedef -----------------------------------------------------------------------------------------*/
enum DM9000_PHY_mode
{
    DM9000_10MHD = 0,
    DM9000_100MHD = 1,
    DM9000_10MFD = 4,
    DM9000_100MFD = 5,
    DM9000_10M = 6,
    DM9000_AUTO = 8,
    DM9000_1M_HPNA = 0x10
};

enum DM9000_TYPE
{
    TYPE_DM9000E,
    TYPE_DM9000A,
    TYPE_DM9000B,
    TYPE_DM9051
};

struct dm9051_eth
{
    /* inherit from ethernet device */
    struct net_device parent;

    /* interface address info. */
    rt_uint8_t dev_addr[MAX_ADDR_LEN]; /* hw address    */

    enum DM9000_TYPE type;
    enum DM9000_PHY_mode mode;

    uint8_t imr_all;

    uint8_t packet_cnt;        /* packet I or II                                                           */
    uint16_t queue_packet_len; /* queued packet (packet II)                                                */
#if defined (RT_USING_DM9051_ISR)
    struct rt_semaphore sem_dm9051_isr;
#endif
    struct rt_mutex tx_lock;

    /* spi device */
    struct rt_spi_device *spi_device;
    struct rt_mutex lock;
    struct dm9051_tx_node tx_queue; /* tx queue */
    struct dm9051_tx_node rx_queue; /* rx queue */
};


#ifdef uIP_NOOS
void etherdev_init(void);
void etherdev_send(void);
uint16_t etherdev_read(void);
s32 dm9051_init(void);
u32 dm9051_tx(void);
u16 dm9051_rx(void);
#else
void dm9051_isr(void);
rt_err_t dm9051_attach(const char *spi_device_name);
#endif  /* uIP_NOOS */

#endif /* __DM9000_REG_H -----------------------------------------------------------------------------------*/
