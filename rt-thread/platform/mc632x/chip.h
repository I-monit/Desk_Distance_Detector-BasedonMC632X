/**
 * Copyright (c) 2015-2019 Shanghai Fullhan Microelectronics Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 *
 * Change Logs:
 * Date           Author       Notes
 *
 */

#ifndef __CHIP_H__
#define __CHIP_H__

#include "mc632x/arch.h"


#define REG_GLB_RESET               ((CEN_GLB_APB_REG_BASE) + 0xac)
#define SW_GLB_RST                  (BIT(0))
#define SW_EXT_RST                  (BIT(1))

#define PAD_REG_START_OFF           0x8

#define REG_AP_PERI_SOFT_RST0       (SYS_REG_P2V(CPU_SYS_AHB_REG_BASE) + 0xB4)
#define INT_CTRL3_SOFT_RST              27
#define INT_CTRL2_SOFT_RST              26
#define MOTORCTL1_DB_SOFT_RST          25
#define MOTORCTL1_SOFT_RST              24
#define MOTORCTL0_DB_SOFT_RST          23
#define MOTORCTL0_SOFT_RST              22
#define SPIC_SOFT_RST                  21
#define SPIC_APB_SOFT_RST              20
#define TIMER0_RTC_SOFT_RST                19
#define TIMER0_XTL_SOFT_RST              18
#define PWM_SOFT_RST                  17
#define SYST_SOFT_RST                 16
#define SPI0_SOFT_RST                 15
#define WDT_SOFT_RST                  14
#define I2S0_SOFT_RST                 13
#define I2C1_SOFT_RST                 12
#define I2C0_SOFT_RST                 11
#define C2C_CA7_TO_ARC_SOFT_RST       10
#define C2C_ARC_TO_CA7_SOFT_RST          9
#define EFUSE_SOFT_RST                  8
#define ARC_TS_24M_SOFT_RST              7
#define UART1_SOFT_RST                  6
#define UART0_SOFT_RST                  5
#define INT_CTRL1_SOFT_RST              3
#define INT_CTRL0_SOFT_RST              2
#define AHB_HSLOCK_SOFT_RST              1
#define ADDRMON_ARC_SOFT_RST          0

#define REG_IIS0_CLK_CTRL       (SYS_REG_P2V(CPU_SYS_AHB_REG_BASE) + 0xB8)
#define CKG_I2S_FRAC_DIV_EN     BIT(4)
#define CKG_I2S_FAST_EN         BIT(3)
#define CKG_I2S_SOURCE_SEL      BIT(2)
#define CKG_I2S_REV             BIT(1)
#define I2S_MCLK_SEL            BIT(0)
#define REG_CKG_I2S_FRAC_DIV_M  (SYS_REG_P2V(CPU_SYS_AHB_REG_BASE) + 0xBC)
#define REG_CKG_I2S_FRAC_DIV_N  (SYS_REG_P2V(CPU_SYS_AHB_REG_BASE) + 0xC0)
#define REG_CKG_I2S0_CTL        (SYS_REG_P2V(CPU_SYS_AHB_REG_BASE) + 0xC4)

#define I2S_CLK_FREQ            450000000 /* i2s_clk=450M */

/* CPU_SYS_AHB_REG_BASE */
#define REG_CPU_SYS_CLK_CTRL          (SYS_REG_P2V(CPU_SYS_AHB_REG_BASE) + 0x1C0)
#define REG_ARC600_0_CTRL              (SYS_REG_P2V(CPU_SYS_AHB_REG_BASE) + 0x1D0)
#define CKG_ARC600_AUTO_GATE_SEL      (BIT(23))
#define ARC0_CTRL_CPU_START           (BIT(3))
#define ARC0_START_A                  (BIT(2))
#define ARC600_0_EN                   (BIT(1))
#define ARC_SOFT_RST                  (BIT(0))

#define REG_EPHY_CTRL0             (SYS_REG_P2V(CPU_SYS_AHB_REG_BASE) + 0x1FC)
#define REG_GMAC_CTRL0             (SYS_REG_P2V(CPU_SYS_AHB_REG_BASE) + 0x1DC)

#define REG_PMU_SYS_RESET           (SYS_REG_P2V(CEN_GLB_APB_REG_BASE) + 0x100)
#define REG_WR_PROTECT              (SYS_REG_P2V(CEN_GLB_APB_REG_BASE) + 0x548)
#define REG_PMU_CHIP_ID             (SYS_REG_P2V(CEN_GLB_APB_REG_BASE) + 0x198)
#define REG_PMU_BOOT_MODE           (SYS_REG_P2V(CEN_GLB_APB_REG_BASE) + 0x0508)
#define REG_PMU_DDR_SIZE            (SYS_REG_P2V(CEN_GLB_APB_REG_BASE) + 0x050C)
#define REG_PMU_RESERVED2           (SYS_REG_P2V(CEN_GLB_APB_REG_BASE) + 0x0510)
#define REG_PMU_CHIP_INFO           (SYS_REG_P2V(CEN_GLB_APB_REG_BASE) + 0x0514)
#define REG_PMU_EPHY_PARAM          (SYS_REG_P2V(CEN_GLB_APB_REG_BASE) + 0x0518)
#define REG_PMU_RTC_PARAM           (SYS_REG_P2V(CEN_GLB_APB_REG_BASE) + 0x051c)
#define REG_ARC600_0_BOOT0          (SYS_REG_P2V(CEN_GLB_APB_REG_BASE) + 0x0)
#define REG_ARC600_0_BOOT1          (SYS_REG_P2V(CEN_GLB_APB_REG_BASE) + 0x4)
#define REG_ARC600_0_BOOT2          (SYS_REG_P2V(CEN_GLB_APB_REG_BASE) + 0x8)
#define REG_ARC600_0_BOOT3          (SYS_REG_P2V(CEN_GLB_APB_REG_BASE) + 0xC)
#define REG_CPU_SYS_MTX_CTRL0       (SYS_REG_P2V(CPU_SYS_AHB_REG_BASE) + 0x1CC)



/* DMC_SYS_APB_REG_BASE */
#define REG_DFIMON_TMR1_CTRL0       (SYS_REG_P2V(DMC_SYS_APB_REG_BASE) + 0x0)
#define REG_DFIMON_TMR1_CTRL1       (SYS_REG_P2V(DMC_SYS_APB_REG_BASE) + 0x4)
#define REG_DFIMON_TMR1_CTRL2       (SYS_REG_P2V(DMC_SYS_APB_REG_BASE) + 0x8)
#define REG_HIGH_LEN_TIMER1         (REG_DFIMON_TMR1_CTRL1)
#define REG_LOW_LEN_TIMER1          (REG_DFIMON_TMR1_CTRL2)
#define REG_DMC_CLK_CTRL            (SYS_REG_P2V(DMC_SYS_APB_REG_BASE) + 0x300)
#define REG_PERF_TRIGGER            (SYS_REG_P2V(DMC_SYS_APB_REG_BASE) + 0xB04)

#define REG_PTS_RD_LOW              (SYS_REG_P2V(ISP_SYS_APB_REG_BASE) + 0x7c)
#define REG_PTS_RD_HIGH             (SYS_REG_P2V(ISP_SYS_APB_REG_BASE) + 0x80)

#define REG_PTS_UPDATE              (SYS_REG_P2V(PTS_REG_BASE) + 0x0)

#define VEU_SYS_CLK_CFG2            (SYS_REG_P2V(VEU_SYS_APB_REG_BASE) + 0x14)
#define USB2_PHY_CFG0               (SYS_REG_P2V(USB2_APB_CFG) + 0x18)
#define USB2_PHY_CFG1               (SYS_REG_P2V(USB2_APB_CFG) + 0x1C)
#define USB2_PHY_CFG2               (SYS_REG_P2V(USB2_APB_CFG) + 0x20)

/*USB*/
#define USB_TUNE_ADJ_SET      (0x78203344)
#define USB_UTMI_RST_BIT      (0x1 << 13)
#define USB_PHY_RST_BIT       (0x3 << 30)
#define USB_SLEEP_MODE_BIT    (0x1 << 6)
#define USB_IDDQ_PWR_BIT      (0x1 << 11)
#define USB_COMMON_CORE_BIT   (0x1 << 20)
#define USB_OTG_DIS_BIT       (0x1 << 31)
#define USB_VBUS_PWR_GPIO     (47)




/* inside fhy done */
#endif
