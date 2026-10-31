/**
 * Copyright (c) 2015-2019 Shanghai Fullhan Microelectronics Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 *
 * Change Logs:
 * Date           Author       Notes
 * 2019-04-12     wangyl       add license Apache-2.0
 */

#include "rtthread.h"
#include "rtdebug.h"
#include "fh_chip.h"
#include "fh_pmu.h"
#include "fh_def.h"
#include <delay.h>
#include <linux/delay.h>
#include <rthw.h>

#ifndef INTERNAL_PHY
#define INTERNAL_PHY    0x55
#endif
#ifndef EXTERNAL_PHY
#define EXTERNAL_PHY    0xaa
#endif

#if defined(CONFIG_CHIP_FH8862) || defined(CONFIG_CHIP_MC632X) || defined(CONFIG_ARCH_FH885xV500)
#define PMU_REG_BASE        (0)
#endif

#define FH_PMU_WRITEL(base, value) SET_REG(PMU_REG_BASE + base, value)
#define FH_PMU_WRITEL_MASK(base, value, mask) \
    SET_REG_M(PMU_REG_BASE + base, value, mask)
#define FH_PMU_READL(base) GET_REG(PMU_REG_BASE + base)


#if defined CONFIG_ARCH_FH885xV300 || defined CONFIG_ARCH_FH885xV310 || defined CONFIG_ARCH_FH8852V301_FH8662V100
static unsigned long level;

static inline void fh_set_scu_en(void)
{
    level = rt_hw_interrupt_disable();
    FH_PMU_WRITEL(REG_PMU_SCU_PLL_WREN, 0x706c6c63);
}

static inline void fh_set_scu_di(void)
{
    FH_PMU_WRITEL(REG_PMU_SCU_PLL_WREN, 0x0);
    rt_hw_interrupt_enable(level);
}
void fh_pmu_set_reg(unsigned int offset, unsigned int data)
{
    RT_ASSERT(offset < PMU_OFFSET_MAX);
    fh_set_scu_en();
    FH_PMU_WRITEL(offset, data);
    fh_set_scu_di();
    RT_ASSERT(offset < PMU_OFFSET_MAX);
}

unsigned int fh_pmu_get_reg(unsigned int offset)
{
    RT_ASSERT(offset < PMU_OFFSET_MAX);
    return FH_PMU_READL(offset);
}

void fh_pmu_set_reg_m(unsigned int offset, unsigned int data, unsigned int mask)
{
    RT_ASSERT(offset < PMU_OFFSET_MAX);
    fh_pmu_set_reg(offset, (fh_pmu_get_reg(offset) & (~(mask))) |
                      ((data) & (mask)));
}
#elif defined(CONFIG_ARCH_FH8862) || defined(CONFIG_ARCH_FH885xV500)
static unsigned long level;

static inline void fh_set_scu_en(void)
{
    /*level = rt_hw_local_irq_disable();*/
    FH_PMU_WRITEL(REG_PMU_SCU_PLL_WREN, 0x706c6c63);
    FH_PMU_WRITEL(REG_PMU_DDR_SCU_WREN, 0x706c6c63);
}

static inline void fh_set_scu_di(void)
{
    FH_PMU_WRITEL(REG_PMU_SCU_PLL_WREN, 0x0);
    FH_PMU_WRITEL(REG_PMU_DDR_SCU_WREN, 0x0);
    /*rt_hw_local_irq_enable(level);*/
}
void fh_pmu_set_reg(unsigned int base, unsigned int data)
{
    fh_set_scu_en();
    FH_PMU_WRITEL(base, data);
    fh_set_scu_di();
}

unsigned int fh_pmu_get_reg(unsigned int base)
{
    return FH_PMU_READL(base);
}
void fh_pmu_set_reg_m(unsigned int base, unsigned int data, unsigned int mask)
{
    fh_pmu_set_reg(base, (fh_pmu_get_reg(base) & (~(mask))) |
                      ((data) & (mask)));
}
#else
void fh_pmu_set_reg(unsigned int base, unsigned int data)
{
    FH_PMU_WRITEL(base, data);
}

unsigned int fh_pmu_get_reg(unsigned int base)
{
    return FH_PMU_READL(base);
}

void fh_pmu_set_reg_m(unsigned int base, unsigned int data, unsigned int mask)
{
    FH_PMU_WRITEL_MASK(base, data, mask);
}
#endif

void fh_get_chipid(unsigned int *plat_id, unsigned int *chip_id)
{
    unsigned int _plat_id = 0;

    _plat_id = fh_pmu_get_reg(REG_PMU_CHIP_ID);
    if (plat_id != NULL)
        *plat_id = _plat_id;

#ifndef CONFIG_ARCH_MC632X
    if (chip_id != NULL)
        *chip_id = fh_pmu_get_reg(REG_PMU_IP_VER);
#endif
}

unsigned int fh_pmu_get_ptsl(void)
{
#ifndef CONFIG_ARCH_MC632X
    fh_pmu_set_reg(REG_PMU_PTSLO, 0x01);
    return fh_pmu_get_reg(REG_PMU_PTSLO);
#else
    fh_pmu_set_reg(REG_PTS_UPDATE, 0);
    return fh_pmu_get_reg(REG_PTS_RD_LOW);
#endif
}

unsigned int fh_pmu_get_ptsh(void)
{
#ifndef CONFIG_ARCH_MC632X
    fh_pmu_set_reg(REG_PMU_PTSLO, 0x01);
    return fh_pmu_get_reg(REG_PMU_PTSHI);
#else
    fh_pmu_set_reg(REG_PTS_UPDATE, 0);
    return fh_pmu_get_reg(REG_PTS_RD_HIGH);
#endif
}

unsigned long long fh_get_pts64(void)
{
    unsigned int high, low;
    unsigned long long pts64;

#ifdef CONFIG_CHIP_MC632X
    fh_pmu_set_reg(REG_PTS_UPDATE, 0);
    high = fh_pmu_get_reg(REG_PTS_RD_HIGH);
    low = fh_pmu_get_reg(REG_PTS_RD_LOW);
#else
    fh_pmu_set_reg(REG_PMU_PTSLO, 0x01);
    high = fh_pmu_get_reg(REG_PMU_PTSHI);
    low = fh_pmu_get_reg(REG_PMU_PTSLO);
#endif
    pts64 = (((unsigned long long)high)<<32)|((unsigned long long)low);
    return pts64;
}

void fh_pmu_wdt_pause(void)
{
#ifndef CONFIG_CHIP_MC632X
	unsigned int reg;

	reg = fh_pmu_get_reg(REG_PMU_WDT_CTRL);
	reg |= 0x100;
	fh_pmu_set_reg(REG_PMU_WDT_CTRL, reg);
#endif
}

void fh_pmu_wdt_resume(void)
{
#ifndef CONFIG_CHIP_MC632X
	unsigned int reg;

	reg = fh_pmu_get_reg(REG_PMU_WDT_CTRL);
	reg &= ~(0x100);
	fh_pmu_set_reg(REG_PMU_WDT_CTRL, reg);
#endif
}

void fh_pmu_usb_utmi_rst(void)
{
#ifdef CONFIG_ARCH_MC632X
    unsigned int pmu_reg;

    pmu_reg = fh_pmu_get_reg(VEU_SYS_CLK_CFG2);
    pmu_reg |= USB_UTMI_RST_BIT;
    fh_pmu_set_reg(VEU_SYS_CLK_CFG2, pmu_reg);

    udelay(1000);
    pmu_reg &= ~(USB_UTMI_RST_BIT);
    fh_pmu_set_reg(VEU_SYS_CLK_CFG2, pmu_reg);

    pmu_reg = 20;
    while (pmu_reg--)
        udelay(1000);
#else
	unsigned int pmu_reg;

	pmu_reg = fh_pmu_get_reg(REG_PMU_SWRST_MAIN_CTRL);
	pmu_reg &= ~(USB_UTMI_RST_BIT);
	fh_pmu_set_reg(REG_PMU_SWRST_MAIN_CTRL, pmu_reg);
	pmu_reg = fh_pmu_get_reg(REG_PMU_SWRST_MAIN_CTRL);
	udelay(1000);
	pmu_reg |= USB_UTMI_RST_BIT;
	fh_pmu_set_reg(REG_PMU_SWRST_MAIN_CTRL, pmu_reg);
	pmu_reg = fh_pmu_get_reg(REG_PMU_SWRST_MAIN_CTRL);
	udelay(1000);
#endif
}

void fh_pmu_usb_phy_rst(void)
{
#ifdef CONFIG_ARCH_MC632X
    unsigned int pmu_reg;

    pmu_reg = fh_pmu_get_reg(USB2_PHY_CFG2);
    pmu_reg |= USB_PHY_RST_BIT;
    fh_pmu_set_reg(USB2_PHY_CFG2, pmu_reg);

    udelay(1000);

    pmu_reg &= ~USB_PHY_RST_BIT;
    fh_pmu_set_reg(USB2_PHY_CFG2, pmu_reg);
#else
	unsigned int pmu_reg;

	pmu_reg = fh_pmu_get_reg(REG_PMU_USB_SYS);
	pmu_reg |= (USB_PHY_RST_BIT);
	fh_pmu_set_reg(REG_PMU_USB_SYS, pmu_reg);
	udelay(1000);
	pmu_reg = fh_pmu_get_reg(REG_PMU_USB_SYS);
	pmu_reg &= (~USB_PHY_RST_BIT);
	fh_pmu_set_reg(REG_PMU_USB_SYS, pmu_reg);	
#endif
}

void fh_pmu_usb_pwr_on(void)
{
#ifdef CONFIG_ARCH_MC632X
    unsigned int pmu_reg;

    pmu_reg = fh_pmu_get_reg(USB2_PHY_CFG0);
    pmu_reg &= ~USB_IDDQ_PWR_BIT;
    pmu_reg &= ~USB_OTG_DIS_BIT;
    pmu_reg |= USB_COMMON_CORE_BIT;
    fh_pmu_set_reg(USB2_PHY_CFG0, pmu_reg);
    udelay(1000);
#else
    unsigned int pmu_reg;

    pmu_reg = fh_pmu_get_reg(REG_PMU_USB_SYS1);
    pmu_reg &= (~USB_IDDQ_PWR_BIT);
    fh_pmu_set_reg(REG_PMU_USB_SYS1, pmu_reg);
    udelay(1000);
#endif
}

void fh_pmu_usb_resume(void)
{
#ifdef CONFIG_ARCH_MC632X
    unsigned int pmu_reg;

    pmu_reg = fh_pmu_get_reg(USB2_PHY_CFG0);
    pmu_reg |= USB_SLEEP_MODE_BIT;
    fh_pmu_set_reg(USB2_PHY_CFG0, pmu_reg);
    udelay(1000);
#else
	unsigned int pmu_reg;

	pmu_reg = fh_pmu_get_reg(REG_PMU_USB_SYS);
	pmu_reg |= (USB_SLEEP_MODE_BIT);
	fh_pmu_set_reg(REG_PMU_USB_SYS, pmu_reg);
	udelay(1000);
#endif
}

void fh_pmu_usb_tune(void)
{
#ifdef CONFIG_ARCH_MC632X
    unsigned int pmu_reg;

    pmu_reg = fh_pmu_get_reg(USB2_PHY_CFG1);
    pmu_reg &= ~(0x7 << 18);
    pmu_reg |= ((USB_TUNE_ADJ_SET & (0x7 << 0)) << (18 - 0));
    pmu_reg &= ~(0x7 << 21);
    pmu_reg |= ((USB_TUNE_ADJ_SET & (0x7 << 8)) << (21 - 8));
    pmu_reg &= ~(0x7 << 26);
    pmu_reg |= ((USB_TUNE_ADJ_SET & (0x7 << 4)) << (26 - 4));
    fh_pmu_set_reg(USB2_PHY_CFG1, pmu_reg);

    pmu_reg = fh_pmu_get_reg(USB2_PHY_CFG2);
    pmu_reg &= ~(0xF << 0);
    pmu_reg |= ((USB_TUNE_ADJ_SET & (0xF << 12)) >> (12 - 0));
    pmu_reg &= ~(0xF << 4);
    pmu_reg |= ((USB_TUNE_ADJ_SET & (0xF << 24)) >> (24 - 4));
    pmu_reg &= ~(0x3 << 8);
    pmu_reg |= ((USB_TUNE_ADJ_SET & (0x3 << 20)) >> (20 - 8));
    pmu_reg &= ~(0x3 << 10);
    pmu_reg |= ((USB_TUNE_ADJ_SET & (0x3 << 30)) >> (30 - 10));
    pmu_reg &= ~(0x3 << 12);
    pmu_reg |= ((USB_TUNE_ADJ_SET & (0x3 << 16)) >> (16 - 12));
    pmu_reg &= ~(0x1 << 14);
    pmu_reg |= ((USB_TUNE_ADJ_SET & (0x1 << 18)) >> (18 - 14));
    fh_pmu_set_reg(USB2_PHY_CFG2, pmu_reg);

    pmu_reg = fh_pmu_get_reg(USB2_PHY_CFG1);
    pmu_reg &= ~(0x3 << 29);
    pmu_reg |= ((USB_TUNE_ADJ_SET & (0x3 << 28)) << (29 - 28));
    fh_pmu_set_reg(USB2_PHY_CFG1, pmu_reg);
#else
    unsigned int pmu_reg = 0x76203344;

    fh_pmu_set_reg(REG_PMU_USB_TUNE, pmu_reg);
#endif
}

void fh_pmu_usb_vbus_vldext(void)
{
#ifdef CONFIG_ARCH_MC632X
    unsigned int pmu_reg;

    pmu_reg = fh_pmu_get_reg(USB2_PHY_CFG1);
    pmu_reg |= (1 << 2);
    pmu_reg |= (1 << 3);
    fh_pmu_set_reg(USB2_PHY_CFG1, pmu_reg);
#else
    unsigned int pmu_reg;

    pmu_reg = fh_pmu_get_reg(REG_PMU_USB_CFG);
    pmu_reg |= (1 << 4);
    pmu_reg |= (1 << 8);
    fh_pmu_set_reg(REG_PMU_USB_CFG, pmu_reg);
#endif
}

void _pmu_main_reset(unsigned int reg, unsigned int retry, unsigned int udelay)
{
#ifndef CONFIG_ARCH_MC632X
	fh_pmu_set_reg(REG_PMU_SWRST_MAIN_CTRL, reg);

	while (fh_pmu_get_reg(REG_PMU_SWRST_MAIN_CTRL) != 0xffffffff) {
		if (retry-- <= 0)
			return;

		udelay(udelay);
	}
#else

#endif
}

void fh_pmu_timer_reset(void)
{
#ifndef CONFIG_ARCH_MC632X
    unsigned int reg = 0;
	reg = ~(1 << TMR_RSTN_BIT);
    _pmu_main_reset(reg, 1000, 1);
#else

#endif
}

void _pmu_ahb_reset(unsigned int reg, unsigned int retry, unsigned int udelay)
{
#ifndef CONFIG_ARCH_MC632X
	fh_pmu_set_reg(REG_PMU_SWRST_AHB_CTRL, reg);

	while (fh_pmu_get_reg(REG_PMU_SWRST_AHB_CTRL) != 0xffffffff) {
		if (retry-- <= 0)
			return;

		udelay(udelay);
	}
#else

#endif
}

void _pmu_apb_reset(unsigned int reg, unsigned int retry, unsigned int udelay)
{
#ifndef CONFIG_ARCH_MC632X
	fh_pmu_set_reg(REG_PMU_SWRST_APB_CTRL, reg);

	while (fh_pmu_get_reg(REG_PMU_SWRST_APB_CTRL) != 0xffffffff) {
		if (retry-- <= 0)
			return;

		udelay(udelay);
	}
#else

#endif
}

void _pmu_nsr_reset(unsigned int reg, unsigned int reset_time)
{
#ifndef CONFIG_ARCH_MC632X
	fh_pmu_set_reg(REG_PMU_SWRSTN_NSR, reg);
	udelay(reset_time);
	fh_pmu_set_reg(REG_PMU_SWRSTN_NSR, 0xFFFFFFFF);
#else

#endif
}

#ifdef CONFIG_ARCH_MC632X
static void _reset_ip(int offset, int bit)
{
    fh_pmu_set_reg_m(offset, BIT(bit), BIT(bit));
    udelay(1);
    fh_pmu_set_reg_m(offset, 0, BIT(bit));
}
#endif

void fh_pmu_reset_i2s(void)
{
#ifdef CONFIG_ARCH_MC632X
    _reset_ip(REG_AP_PERI_SOFT_RST0, I2S0_SOFT_RST);
#endif
}

void fh_pmu_sdc_reset(int slot_id)
{
#if defined(CONFIG_ARCH_FH8862) || defined (CONFIG_ARCH_MC632X)

#else
	unsigned int reg = 0;

	if (slot_id == 1)
		reg = ~(1 << SDC1_HRSTN_BIT);
	else if (slot_id == 0)
		reg = ~(1 << SDC0_HRSTN_BIT);
	else
		rt_kprintf("%s slot id error:%d\n",__func__, slot_id);

	_pmu_ahb_reset(reg, 1000, 1);
#endif
}

void fh_pmu_enc_reset(void)
{
#if !defined(CONFIG_ARCH_FH8626V100) && !defined(CONFIG_ARCH_FH8636_FH8852V20X) &&\
    !defined(CONFIG_ARCH_FH8852V301_FH8662V100) && !defined(CONFIG_ARCH_MC632X)
	_pmu_ahb_reset(~(1 << VCU_HRSTN_BIT), 100, 10);
#endif
}

#if !defined(CONFIG_ARCH_FH8636_FH8852V20X) && !defined(CONFIG_ARCH_FH8852V301_FH8662V100) &&\
    !defined(CONFIG_ARCH_MC632X)
void fh_pmu_dwi2s_set_clk(unsigned int div_i2s, unsigned int div_mclk)
{
	unsigned int reg;

	reg = fh_pmu_get_reg(PMU_DWI2S_CLK_DIV_REG);
#if defined(CONFIG_ARCH_FH8862) || defined(CONFIG_ARCH_FH885xV500)
    reg &= ~(0xfff << PMU_DWI2S_CLK_DIV_SHIFT);
    reg |= ((div_i2s-1) << 6 | (div_mclk-1)) << PMU_DWI2S_CLK_DIV_SHIFT;
#else
	reg &= ~(0xffff << PMU_DWI2S_CLK_DIV_SHIFT);
	reg |= ((div_i2s-1) << 8 | (div_mclk-1)) << PMU_DWI2S_CLK_DIV_SHIFT;
#endif
	fh_pmu_set_reg(PMU_DWI2S_CLK_DIV_REG, reg);

	/* i2s_clk switch to PLLVCO */
	reg = fh_pmu_get_reg(PMU_DWI2S_CLK_SEL_REG);
	reg &= ~(1 << PMU_DWI2S_CLK_SEL_SHIFT);
	reg |= 1 << PMU_DWI2S_CLK_SEL_SHIFT;
	fh_pmu_set_reg(PMU_DWI2S_CLK_SEL_REG, reg);
}
#endif

void fh_pmu_set_mclk(int freq)
{
#ifdef CONFIG_ARCH_MC632X
    fh_pmu_set_reg_m(REG_CKG_I2S0_CTL, 0x2, 0x3); // i2s_clk=450M
    fh_pmu_set_reg(REG_IIS0_CLK_CTRL, 0);
    fh_pmu_set_reg(REG_CKG_I2S_FRAC_DIV_N, I2S_CLK_FREQ/1600);
    fh_pmu_set_reg(REG_CKG_I2S_FRAC_DIV_M, freq/800);

    fh_pmu_set_reg(REG_IIS0_CLK_CTRL, CKG_I2S_FRAC_DIV_EN|CKG_I2S_FAST_EN);
#endif
}

void fh_pmu_restart(void)
{
#ifndef CONFIG_ARCH_MC632X
    fh_pmu_set_reg(REG_PMU_SWRST_MAIN_CTRL, 0x7fffffff);
#else
    fh_pmu_set_reg(REG_PMU_SYS_RESET, 0x1);
#endif
}

unsigned int fh_pmu_get_tsensor_init_data(void)
{
#ifdef REG_PMU_RTC_PARAM
    return fh_pmu_get_reg(REG_PMU_RTC_PARAM);
#else
    return 0;
#endif
}

void fh_pmu_set_sdc1_funcsel(unsigned int val)
{
#ifdef REG_PMU_SD1_FUNC_SEL
#if defined(CONFIG_ARCH_FH8636_FH8852V20X) || defined(CONFIG_ARCH_FH8852V301_FH8662V100)
    static int sd1_func_map[] = {1, 0, 2}; /* func sel map */
#elif defined(CONFIG_ARCH_FH865x) || defined(CONFIG_ARCH_FH885xV200)
    static int sd1_func_map[] = {0, 1, 2, 3}; /* func sel map */
#elif defined(CONFIG_ARCH_FH8862) || defined(CONFIG_ARCH_FH885xV500)
    static int sd1_func_map[] = {3, 0, 2, 1}; /* func sel map */
#else
    static int sd1_func_map[] = SD1_FUNC_SEL_MAP; /* func sel map */
#endif

    RT_ASSERT(val < ARRAY_SIZE(sd1_func_map));
#if defined(CONFIG_ARCH_FH8862) || defined(CONFIG_ARCH_FH885xV500)
    fh_pmu_set_reg_m(REG_PMU_SDC_MISC, sd1_func_map[val] << 30, 0xc0000000);
#else
    fh_pmu_set_reg(REG_PMU_SD1_FUNC_SEL, sd1_func_map[val]);
#endif
#endif
}

void fh_pmu_arxc_write_A625_INT_RAWSTAT(unsigned int val)
{
#ifndef CONFIG_ARCH_MC632X
	fh_pmu_set_reg(PMU_A625_INT_RAWSTAT, val);
#else

#endif
}

unsigned int fh_pmu_arxc_read_ARM_INT_RAWSTAT(void)
{
#ifndef CONFIG_ARCH_MC632X
	return fh_pmu_get_reg(PMU_ARM_INT_RAWSTAT);
#else
	return 0;
#endif
}

void fh_pmu_arxc_write_ARM_INT_RAWSTAT(unsigned int val)
{
#ifndef CONFIG_ARCH_MC632X
	fh_pmu_set_reg(PMU_ARM_INT_RAWSTAT, val);
#else

#endif
}

unsigned int fh_pmu_arxc_read_ARM_INT_STAT(void)
{
#ifndef CONFIG_ARCH_MC632X
	return fh_pmu_get_reg(PMU_ARM_INT_STAT);
#else
	return 0;
#endif
}

void fh_pmu_arxc_reset(unsigned long phy_addr)
{
#ifndef CONFIG_ARCH_MC632X
    unsigned int arc_addr;

    /*ARC Reset*/
    fh_pmu_set_reg(REG_PMU_SWRSTN_NSR, ~(1<<ARC_RSTN_BIT));

    arc_addr = ((phy_addr & 0xffff) << 16) | (phy_addr >> 16);

    fh_pmu_set_reg(REG_PMU_A625BOOT0, 0x7940266B);
    /* Configure ARC Bootcode start address */
    fh_pmu_set_reg(REG_PMU_A625BOOT1, arc_addr);
    fh_pmu_set_reg(REG_PMU_A625BOOT2, 0x0F802020);
    fh_pmu_set_reg(REG_PMU_A625BOOT3, arc_addr);

    /*clear ARC ready flag*/
    fh_pmu_arxc_write_ARM_INT_RAWSTAT(0);

    /* don't let ARC run when release ARC */
    fh_pmu_set_reg(REG_PMU_A625_START_CTRL, 0);
    udelay(2);

    /* ARC reset released */
    fh_pmu_set_reg(REG_PMU_SWRSTN_NSR, 0xFFFFFFFF);
#else
    unsigned int arc_addr;
    unsigned long rst_val = 0;
    unsigned long rst_msk = 0;
    
    /*ARC Reset*/
    fh_pmu_set_reg_m(REG_ARC600_0_CTRL, ARC_SOFT_RST, ARC_SOFT_RST);
    
    /* Configure ARC Bootcode start address */
    arc_addr = ((phy_addr & 0xffff) << 16) | (phy_addr >> 16);
    fh_pmu_set_reg(REG_ARC600_0_BOOT0, 0x7940266B);
    fh_pmu_set_reg(REG_ARC600_0_BOOT1, arc_addr);
    fh_pmu_set_reg(REG_ARC600_0_BOOT2, 0x0F802020);
    fh_pmu_set_reg(REG_ARC600_0_BOOT3, arc_addr);
    
    /*config ARC CLK*/
    fh_pmu_set_reg_m(REG_CPU_SYS_CLK_CTRL,
    		CKG_ARC600_AUTO_GATE_SEL, CKG_ARC600_AUTO_GATE_SEL);
    
    /* enable clk, config ARC RESET*/
    rst_val |= ARC0_CTRL_CPU_START;
    rst_val |= ARC0_START_A;
    rst_val |= ARC600_0_EN;
    rst_msk = ARC0_CTRL_CPU_START | ARC0_START_A | ARC600_0_EN;
    fh_pmu_set_reg_m(REG_ARC600_0_CTRL, rst_val, rst_msk);
#endif
}

void fh_pmu_arxc_kickoff(void)
{
#ifndef CONFIG_ARCH_MC632X
    //start ARC625
    fh_pmu_set_reg(REG_PMU_A625_START_CTRL, 0x10);
#else
    fh_pmu_set_reg_m(REG_ARC600_0_CTRL, 0, ARC_SOFT_RST);
#endif
}

void fh_pmu_set_user(void)
{
#ifdef CONFIG_ARCH_FH885xV200
    unsigned int pmuuser0 = fh_pmu_get_reg(REG_PMU_USER0);
    pmuuser0 &= ~(0x01);
    fh_pmu_set_reg(REG_PMU_USER0,pmuuser0);
#endif
}

unsigned int fh_pmu_get_ddrsize(void)
{
#ifdef REG_PMU_DDR_SIZE
    return fh_pmu_get_reg(REG_PMU_DDR_SIZE);
#else
    return 0;
#endif
}

void fh_pmu_set_stmautostopgpio(int stmid, int gpio)
{
#if defined(CONFIG_ARCH_FH885xV300) || defined(CONFIG_ARCH_FH8862) ||\
    defined(CONFIG_ARCH_FH885xV310) || defined(CONFIG_ARCH_FH885xV500) || defined(CONFIG_ARCH_FH8852V301_FH8662V100)
    unsigned int val;

    val = fh_pmu_get_reg(REG_PMU_EFHY_REG0);

    if (stmid)
    {
        val &= (~0xfc00);
        val |= gpio<<10;
    }
    else
    {
        val &= (~0x3f0);
        val |= gpio<<4;
    }
    fh_pmu_set_reg(REG_PMU_EFHY_REG0, val);
#elif defined(CONFIG_ARCH_MC632X)
    unsigned int val;

    val = fh_pmu_get_reg(REG_CPU_SYS_MTX_CTRL0);

    if (stmid)
    {
        val &= (~0xfc00);
        val |= gpio<<10;
    }
    else
    {
      val &= (~0x3f0);
      val |= gpio<<4;
    }
    fh_pmu_set_reg(REG_CPU_SYS_MTX_CTRL0, val);

#endif
}
void fh_pmu_set_hashen(unsigned long en)
{
#if defined(CONFIG_ARCH_FH885xV300) || defined(CONFIG_ARCH_FH8862) || defined(CONFIG_ARCH_FH885xV310) || defined(CONFIG_ARCH_FH885xV500)
    unsigned int val;

    val = fh_pmu_get_reg(REG_PMU_DMA_SEL);
    if (en)
        val |= 1<<16;
    else
        val &= ~(1<<16);

    fh_pmu_set_reg(REG_PMU_DMA_SEL, val);
#endif
}

#if defined(CONFIG_CHIP_MC632X)
static int thm_adjutsB = 0;
#define MOL_THM_REF_TEMP (70)
rt_int32_t efuse_read_entry(rt_uint32_t key,
    rt_uint32_t start_entry, rt_uint8_t *buff,
    rt_uint32_t size);

int fh_get_mol_thm_temperature(void)
{
    rt_uint8_t low;
    if (thm_adjutsB == 0)
    {
        /* init */
        fh_pmu_set_reg(0x1c000370, 1);
        rt_udelay(10);
        fh_pmu_set_reg(0x1c000378, 0x20);
        rt_udelay(10);
        fh_pmu_set_reg(0x1c000378, 0);
        rt_udelay(10);
        fh_pmu_set_reg(0x1c000370, 3);
        rt_udelay(10);
        efuse_read_entry(0x92fc0025, 53, &low, 1);
        if (low > 0)
            thm_adjutsB =  MOL_THM_REF_TEMP*10 - ((low<<1))*371*10/1000;
        else
            thm_adjutsB = -450;
        rt_udelay(1000);
    }
    return (((fh_pmu_get_reg(0x1c000374)>>3)&0x1ff)*371*10/1000 + thm_adjutsB)/10;
}

#endif


