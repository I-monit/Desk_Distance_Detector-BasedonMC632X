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

#ifndef __FH_PMU_H__
#define __FH_PMU_H__

#include "fh_chipid.h"

void fh_pmu_set_reg(unsigned int offset, unsigned int data);
unsigned int fh_pmu_get_reg(unsigned int offset);
void fh_pmu_set_reg_m(unsigned int offset, unsigned int data, unsigned int mask);
void fh_get_chipid(unsigned int *plat_id, unsigned int *chip_id);
unsigned int fh_pmu_get_ptsl(void);
unsigned int fh_pmu_get_ptsh(void);
unsigned long long fh_get_pts64(void);
void fh_pmu_wdt_pause(void);
void fh_pmu_wdt_resume(void);
void fh_pmu_usb_utmi_rst(void);
void fh_pmu_usb_phy_rst(void);
void fh_pmu_usb_pwr_on(void);
void fh_pmu_usb_resume(void);
void fh_pmu_usb_tune(void);
void fh_pmu_usb_vbus_vldext(void);

unsigned int fh_pmu_get_tsensor_init_data(void);
void fh_pmu_set_sdc1_funcsel(unsigned int val);

void fh_pmu_sdc_reset(int slot_id);
void fh_pmu_enc_reset(void);
void fh_pmu_dwi2s_set_clk(unsigned int div_i2s, unsigned int div_mclk);
void fh_pmu_set_mclk(int freq);
void fh_pmu_restart(void);
void fh_pmu_reset_i2s(void);

void fh_pmu_arxc_write_A625_INT_RAWSTAT(unsigned int val);
unsigned int fh_pmu_arxc_read_ARM_INT_RAWSTAT(void);
void fh_pmu_arxc_write_ARM_INT_RAWSTAT(unsigned int val);
unsigned int fh_pmu_arxc_read_ARM_INT_STAT(void);
void fh_pmu_arxc_reset(unsigned long phy_addr);
void fh_pmu_arxc_kickoff(void);
void fh_pmu_timer_reset(void);
void fh_pmu_sdc_setphase(int slot_id, unsigned int drv_degree, unsigned int sam_degree);
void fh_pmu_set_user(void);
unsigned int fh_pmu_get_ddrsize(void);
void fh_pmu_set_stmautostopgpio(int stmid, int gpio);
void fh_pmu_set_hashen(unsigned long en);

struct fh_chip_info *fh_get_chip_info(void);
char *fh_get_chipname(void);
#endif /* FH_PMU_H_ */
