
#ifndef __ARCH_H__
#define __ARCH_H__

/*****************************/
/* BSP CONTROLLER BASE       */
/*****************************/

#define UART0_REG_BASE        (0x08f00000)
#define UART1_REG_BASE        (0x09000000)
#define UART2_REG_BASE        (0x09100000)
#define CPU_SYS_CLK_RF_REG_BASE    (0x09200200)
#define TMR_REG_BASE          (0x09800000)
#define GPIO0_REG_BASE        (0x0B000000)
#define GPIO1_REG_BASE        (0x0B010000)
#define GPIO2_REG_BASE        (0x0B020000)
#define GPIO3_REG_BASE        (0x0B030000)
#define GPIO4_REG_BASE        (0x0B040000)
#define GPIO5_REG_BASE        (0x0B050000)
#define GPIO6_REG_BASE        (0x0B060000)

#define I2C0_REG_BASE                       (0x08d00000)
#define I2C1_REG_BASE                       (0x08e00000)
#define I2S_REG_BASE                        (0x08900000)
#define RTC_REG_BASE                        (0x2c900000)
#define TIMER0_REG_BASE                     (0x09800000)
#define SYS_TIMER_REG_BASE                  (0x09700000)
#define WDT0_REG_BASE                       (0x09600000)
#define SPI0_REG_BASE                       (0x08c00000)
#define SPI1_REG_BASE                       (0x08b00000)
#define PWM_REG_BASE                        (0x09500000)

#define CPU_MTX_APB_REG_BASE        (0x0b200000)
#define CPU_SYS_AHB_REG_BASE        (0x0c000000)
#define C2C_INTR0_REG_BASE          (0x0a100000)
#define C2C_INTR1_REG_BASE          (0x0a200000)
#define SMT0_REG_BASE               (0x0a400000)
#define SMT1_REG_BASE               (0x0a800000)
#define TOP_PRE_DIV_RF_REG_BASE     (0x10300000)
#define TOP_CLK_RF_REG_BASE         (0x10300200)
#define TOP_CLK_GATE_RF_REG_BASE    (0x10300400)
#define CEN_GLB_APB_REG_BASE        (0x10000000)
#define CEN_PIN_REG_BASE            (0x10200000)
#define SADC_REG_BASE               (0x1c500000)
#define GMAC_REG_BASE               (0x1c600000)
#define EPHY_REG_BASE               (0x1c900000)

#define DMC_SYS_APB_REG_BASE        (0x24000000)
#define DMC_REG_REG_BASE            (0x2C100000)
#define DDR_PHY_REG_BASE            (0x2C200000)
#define DFI_MON_REG_BASE            (0x2C400000)
#define DDR_FW_REG_BASE             (0x2C500000)
#define PERF_MON0_REG_BASE          (0x24800000)
#define PERF_MON1_REG_BASE          (0x24900000)
#define PERF_MON2_REG_BASE          (0x24a00000)

#define VEU_SYS_PIN_REG_BASE    (0x1c100000)
#define ISP_SYS_PIN_REG_BASE    (0x2c000000)
#define CSI_SYS_PIN_REG_BASE    (0x2c600000)
#define CPU_SYS_PIN_REG_BASE    (0x0b100000)

#define VEU_SYS_APB_REG_BASE        (0x1C000000)
#define USB2_APB_CFG                            (0x1CA00000)
#define ISP_SYS_APB_REG_BASE        (0x2c100000)
#define PTS_REG_BASE                (0x2c300000)


#define DMAC0_REG_BASE      (0x0C700000)

#define SDC0_REG_BASE       (0x2D100000)
#define SDC1_REG_BASE       (0x2D200000)
#define GICD_BASE           (0x00201000)
#define GICC_BASE           (0x00202000)

#define USB_PHY_REG_BASE    (0x1CA00000)
#define USB_REG_BASE        (0x1D100000)
#define AES_REG_BASE        (0x0C600000)
#define EFUSE_REG_BASE      (0x0A700000)

#define SYS_REG_V2P(va)     (va)
#define SYS_REG_P2V(pa)     (pa)

#define REG_COH_ADDR        (0x1010032C)
#define ARM_IN_SLP          (0xdeadbeef)


typedef enum IRQn
{
    RTC_IRQn                    = (32 + 0),
    ISP_IRQn                    = (32 + 1),
    VICAP_IRQn                  = (32 + 2),
    SDC0_IRQn                   = (32 + 3),
    ACODEC_IRQn                 = (32 + 4),
    JPG_IRQn                    = (32 + 5),
    VDEC_IRQn                   = (32 + 6),
    USB_IRQn                    = (32 + 7),
    GMAC0_SBD_INTR_IRQn         = (32 + 8),
    GMAC0_SBD_PERCH_TX0_IRQn    = (32 + 9),
    GMAC0_SBD_PERCH_RX0_IRQn    = (32 + 10),
    NNA_IRQn                    = (32 + 11),
    DFI_ALERT_ERR_IRQn          = (32 + 12),
    DFI_MON_IRQn                = (32 + 13),
    DMC_PORT_MPU_IRQn           = (32 + 14),
    DMC_PERF_MON_IRQn           = (32 + 15),
    DMC_PORT_POISON_IRQn        = (32 + 16),
    DDRPHY_IRQn                 = (32 + 17),
    I2C0_IRQn                   = (32 + 18),
    I2C1_IRQn                   = (32 + 19),
    UART0_IRQn                  = (32 + 20),
    UART1_IRQn                  = (32 + 21),
    GPIO0_IRQn                  = (32 + 22),
    GPIO1_IRQn                  = (32 + 23),
    GPIO2_IRQn                  = (32 + 24),
    GPIO3_IRQn                  = (32 + 25),
    GPIO4_IRQn                  = (32 + 26),
    GPIO5_IRQn                  = (32 + 27),
    SPI1_IRQn                   = (32 + 28),
    SPI0_IRQn                   = (32 + 29),
    WDT_IRQn                    = (32 + 30),
    SYSTIMER_IRQn               = (32 + 31),
    TMR0_IRQn                   = (32 + 32),
    PWM_IRQn                    = (32 + 33),
    SMT0_IRQn                   = (32 + 34),
    SMT1_IRQn                   = (32 + 35),
    DMA_IRQn                    = (32 + 36),
    AES_IRQn                    = (32 + 37),
    ADDR_MON_IRQn               = (32 + 38),
    ARC2CA7_COMBIRQn            = (32 + 39),
    I2S0_IRQn                   = (32 + 40),
    ADDR_MON_ARC_IRQn           = (32 + 41),
    DMA_CTRL_IRQn               = (32 + 42),
    ARC2CA7_IRQ0                = (32 + 43),
    ARC2CA7_IRQ1                = (32 + 44),
    ARC2CA7_IRQ2                = (32 + 45),
    ARC2CA7_IRQ3                = (32 + 46),
    ARC2CA7_IRQ4                = (32 + 47),
    ARC2CA7_IRQ5                = (32 + 48),
    ARC2CA7_IRQ6                = (32 + 49),
    ARC2CA7_IRQ7                = (32 + 50),
    ARC2CA7_IRQ8                = (32 + 51),
    ARC2CA7_IRQ9                = (32 + 52),
    UART2_IRQn                  = (32 + 53),
    VOUDHD_IRQn                 = (32 + 54),
    SDC1_IRQn                   = (32 + 55),
    SADC_IRQn                   = (32 + 56),
    GPIO6_IRQn                  = (32 + 57),
} IRQn_Type;

enum DMA_HW_HS_MAP
{
    UART0_RX = 0,
    UART0_TX,
    UART1_RX,
    UART1_TX,
    I2C0_RX,
    I2C0_TX,
    I2C1_RX,
    I2C1_TX,
    AES_RX,
    AES_TX,
    SPI1_RX,
    SPI1_TX,
    SPI0_RX,
    SPI0_TX,
    ACODEC_RX,
    ACODEC_TX,
    I2S_TX,
    I2S_RX,
    UART2_RX,
    UART2_TX,
    VOU_V0_C,
    VOU_V0_Y,
    VOU_OSD,
    DMA_HW_HS_END,
};

#define TIMER_CLOCK     32768

#endif /* ARCH_H_ */
