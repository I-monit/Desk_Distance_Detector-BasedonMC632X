#ifndef _FH_OSAL_FH_WRAP_H
#define _FH_OSAL_FH_WRAP_H

#if defined(__linux__)
#include <linux/version.h> // no problem no config include in this


#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0)

#define SYS_REG_P2V(addr) (addr)

void fh_get_chipid(unsigned int *plat_id, unsigned int *chip_id);
unsigned int fh_pmu_get_ptsl(void);
unsigned int fh_pmu_get_ptsh(void);
unsigned long long fh_get_pts64(void);

void fh_pmu_arxc_write_A625_INT_RAWSTAT(unsigned int val);
unsigned int fh_pmu_arxc_read_ARM_INT_RAWSTAT(void);
void fh_pmu_arxc_write_ARM_INT_RAWSTAT(unsigned int val);
unsigned int fh_pmu_arxc_read_ARM_INT_STAT(void);
void fh_pmu_arxc_reset(unsigned long phy_addr);
void fh_pmu_arxc_kickoff(void);

struct fh_chip_info
{
    int _plat_id; 
    int _chip_id; 
    int _chip_mask; 
    int chip_id; 
    int ddr_size; 
    char chip_name[32];
};

char *fh_get_chipname(void);
struct fh_chip_info *fh_get_chip_info(void);


void fh_pmu_set_reg(unsigned int offset, unsigned int data);
unsigned int fh_pmu_get_reg(unsigned int offset);
void fh_pmu_set_reg_m(unsigned int offset,unsigned int data, unsigned int mask);


#else
#include <mach/pmu.h>
#endif

#else
#include <fh_pmu.h>
#endif


#endif
