
#ifndef __ARCH_H__
#define __ARCH_H__

/*****************************/
/* BSP CONTROLLER BASE       */
/*****************************/
#define INTC_REG_BASE    (0xE0200000)
#define DMAC0_REG_BASE   (0xE0300000)
#define GMAC_REG_BASE    (0xE0600000)
#define USB_REG_BASE     (0xE0700000)
#define SDC0_REG_BASE    (0xE2000000)
#define SDC1_REG_BASE    (0xE2200000)
#define VEU_REG_BASE     (0xE7000000)
#define VEU_LOOPBUF_BASE (0xE7400000)
#define NN_REG_BASE      (0xE8000000)
#define AES_REG_BASE     (0xE8200000)
#define JPEG_REG_BASE    (0xE8300000)
/*
 * ISPB/F should be ISP/FD
 */
#define ISPP_REG_BASE   (0xE8400000)
#define ISPF_REG_BASE   (0xE8500000)
#define BGM_REG_BASE    (0xE8600000)
#define JPEG_LOOPBUF_REG_BASE    (0xE8700000)

#define VPU_REG_BASE    (0xEC000000)
#define DDRC_REG_BASE   (0xED000000)
/* #define DMAC_REG_BASE   (0xEE000000) */

#define PERF_REG_BASE    (0xF0E00000)
#define SCU_REG_BASE    (0xF0000000)
#define PMU_REG_BASE    (0xF0000000)
#define I2C2_REG_BASE   (0xF0100000)
#define I2C0_REG_BASE   (0xF0200000)
#define GPIO0_REG_BASE  (0xF0300000)
#define PWM_REG_BASE    (0xF0400000)
#define SPI0_REG_BASE   (0xF0500000)
#define SPI1_REG_BASE   (0xF0600000)
#define SPI2_REG_BASE   (0xF0640000)
#define UART0_REG_BASE  (0xF0700000)
#define UART1_REG_BASE  (0xF0800000)
#define I2S_REG_BASE    (0xF0900000)
#define ACODEC_REG_BASE (0xF0A00000)
#define I2C1_REG_BASE   (0xF0B00000)
#define TMR_REG_BASE    (0xF0C00000)
#define WDT_REG_BASE    (0xF0D00000)
#define SMT0_REG_BASE   (0xF0401000)
#define SMT1_REG_BASE   (0xF0402000)
#define GPIO1_REG_BASE  (0xF4000000)
#define REG_EPHY_BASE   (0xf1800000)
#define REG_MDIO_BASE   (REG_EPHY_BASE + 0x600)
/*
 * should be MIPI_Wrapper
 */
#define MIPI_WRAPPER_BASE   (0xF1000000)
#define MIPIC_REG_BASE      (0xF1100000)
#define SADC_REG_BASE       (0xF1200000)
#define UART2_REG_BASE      (0xF1300000)
#define RTC_REG_BASE        (0xF1500000)
#define EFUSE_REG_BASE      (0xF1600000)

#define GICD_BASE           (0x30001000)
#define GICC_BASE           (0x30002000)

typedef enum IRQn
{
    PMU_IRQn          = (32 + 0),
    DDRC_IRQn         = (32 + 1),
    WDT_IRQn          = (32 + 2),
    TMR0_IRQn         = (32 + 3),
    VEU_IRQ0          = (32 + 4),
    VEU_IRQ1          = (32 + 5),
    VEU_IRQ2          = (32 + 6),
    ISP_IRQn          = (32 + 7),
    PERF_IRQn         = (32 + 8),
    VEU_SW_IRQn       = (32 + 9),
    NN_IRQn           = (32 + 10),
    I2C0_IRQn         = (32 + 11),
    I2C1_IRQn         = (32 + 12),
    JPEG_IRQn         = (32 + 13),
    BGM_IRQn          = (32 + 14),
    VEU_LOOP_IRQn     = (32 + 15),
    AES_IRQn          = (32 + 16),
    MIPIC_IRQn        = (32 + 17),
    MIPI_WRAP_IRQn    = (32 + 18),
    ACW_IRQn          = (32 + 19),
    SADC_IRQn         = (32 + 20),
    SPI1_IRQn         = (32 + 21),
    JPEG_LOOP_IRQn    = (32 + 22),
    DMAC0_IRQn        = (32 + 23),
    DMAC1_IRQn        = (32 + 24),
    GPIO0_IRQn        = (32 + 26),
    SPI0_IRQn         = (32 + 28),
    PMU_ARM_IRQn      = (32 + 29),
    UART0_IRQn        = (32 + 30),
    UART1_IRQn        = (32 + 31),
    PMU_ARC_IRQn      = (32 + 32),
    RTC_IRQn          = (32 + 33),
    SMT0_IRQn         = (32 + 34),
    SMT1_IRQn         = (32 + 35),
    PWM_IRQn          = (32 + 36),
    SPI2_IRQn         = (32 + 38),
    USB_IRQn          = (32 + 39),
    GPIO1_IRQn        = (32 + 40),
    UART2_IRQn        = (32 + 41),
    SDC0_IRQn         = (32 + 42),
    SDC1_IRQn         = (32 + 43),
    EMAC_IRQn         = (32 + 44),
    EPHY_IRQn         = (32 + 45),
    I2C2_IRQn         = (32 + 46),
    RTC_ALM_IRQn      = (32 + 47),
    RTC_CORE_IRQn     = (32 + 48),
} IRQn_Type;

enum DMA_HW_HS_MAP
{
    ACODEC_RX = 0,
    ACODEC_TX,
    SPI1_RX,
    SPI1_TX,
    SPI0_RX,
    SPI0_TX,
    UART0_RX,
    UART0_TX,
    UART1_RX,
    UART1_TX,
    I2S_RX, /* Reserved */
    I2S_TX, /* Reserved */
    UART2_RX,
    UART2_TX,
    SPI2_RX,
    SPI2_TX,
    DMA_HW_HS_END,
};

#define TIMER_CLOCK     1000000 /*ASIC =(50000000)*/
#define DMA_FIXED_CHANNEL_NUM       6

#endif /* ARCH_H_ */
