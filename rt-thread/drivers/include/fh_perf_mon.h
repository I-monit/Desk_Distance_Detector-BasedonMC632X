/*
* Copyright (c) 2015-2019 Shanghai Fullhan Microelectronics Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020-05-06     fullhan         add license Apache-2.0
 */


#ifndef __FH_PERF_MON_H__
#define __FH_PERF_MON_H__



#define REG_PERF_MONI_REG(id) (fh_perf_mon_obj.regs+4*(id))


struct fh_perf_mon_obj_t
{
    unsigned int regs;
    unsigned int irq_no;
};

enum fh_perf_mode_e
{
    FH_PERF_SINGLE,
    FH_PERF_CONTINUOUS,
};

struct fh_perf_adv_port_param
{
    unsigned int used;
    unsigned int filter;
    unsigned int mask;
};

struct fh_perf_data_buff
{
    unsigned int addr;
    unsigned int cnt;
};



struct fh_perf_param_input
{
    enum fh_perf_mode_e mode;
    unsigned int ddr_bw;
    unsigned int window_time;
    unsigned int axi_bw[10];
    struct fh_perf_adv_port_param addr_param[10];
    struct fh_perf_adv_port_param id_param[10];

};

struct fh_perf_param_output
{
    unsigned int serial_cnt;
    unsigned int hw_cnt;
    unsigned int wr_ot[10];
    unsigned int rd_ot[10];
    unsigned int wr_cmd_cnt[10];
    unsigned int rd_cmd_cnt[10];
    unsigned int wr_cmd_byte[10];
    unsigned int rd_cmd_byte[10];
    unsigned int wr_sum_lat[10];
    unsigned int rd_sum_lat[10];
    unsigned int wr_cmd_cnt_lat[10];
    unsigned int rd_cmd_cnt_lat[10];
    unsigned int ddr_wr_bw;
    unsigned int ddr_rd_bw;
};


void rt_hw_fhperf_init(void);











#endif
