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


#include "fh_clock.h"
#include "board_info.h"
#include "fh_def.h"
#include <rtdef.h>
#include <rtthread.h>
#include <rthw.h>
#include "fh_perf_mon.h"
#include <stdlib.h>

typedef unsigned int u32;


#include "fh_perf_mon.h"

#define AXI_PORT_MAX 8


struct fh_perf_mon_obj_t fh_perf_mon_obj;
struct fh_perf_param_output lastres;
struct clk  *fh_perf_clk;

static void ctl_monitor(int on);

enum fh_perf_state_e
{
    FH_PERF_STOP,
    FH_PERF_RUNNING,
} fh_perf_state;

struct fh_perf_param_input last_input_param;
struct fh_perf_data_buff fh_databuff;

#define WR_CMD_CNT_REG (0x1E)
#define RD_CMD_CNT_REG (0x26)
#define WR_CMD_BYTE_REG (0x2E)
#define RD_CMD_BYTE_REG (0x36)
#define WR_SUM_LAT_REG (0x3e)
#define RD_SUM_LAT_REG (0x46)
#define WR_CMD_CNT_LAT_REG (0x4E)
#define RD_CMD_CNT_LAT_REG (0x56)
#define DDR_WR_BW_REG (0x5E)
#define DDR_RD_BW_REG (0x5F)
#define HWCNT_REG (0x60)

#define OT_REG (0x1A)
#define ADDR_FILTER_REG (0x2)
#define ADDR_MASK_REG (0xA)
#define ID_MASK_REG (0x12)





static void update_lastresult(void)
{
    int i = 0;

    lastres.serial_cnt++;

    lastres.hw_cnt = GET_REG(REG_PERF_MONI_REG(HWCNT_REG))&0xff;

    for (i = 0; i < AXI_PORT_MAX; i++)
    {
        lastres.wr_cmd_cnt[i] =
            GET_REG(REG_PERF_MONI_REG(WR_CMD_CNT_REG+i));
        lastres.rd_cmd_cnt[i] =
            GET_REG(REG_PERF_MONI_REG(RD_CMD_CNT_REG+i));
        lastres.wr_cmd_byte[i] =
            GET_REG(REG_PERF_MONI_REG(WR_CMD_BYTE_REG+i));
        lastres.rd_cmd_byte[i] =
            GET_REG(REG_PERF_MONI_REG(RD_CMD_BYTE_REG+i));
        lastres.wr_sum_lat[i] =
            GET_REG(REG_PERF_MONI_REG(WR_SUM_LAT_REG+i));
        lastres.rd_sum_lat[i] =
            GET_REG(REG_PERF_MONI_REG(RD_SUM_LAT_REG+i));
        lastres.wr_cmd_cnt_lat[i] =
            GET_REG(REG_PERF_MONI_REG(WR_CMD_CNT_LAT_REG+i));
        lastres.rd_cmd_cnt_lat[i] =
            GET_REG(REG_PERF_MONI_REG(RD_CMD_CNT_LAT_REG+i));
        lastres.wr_ot[i] = (GET_REG(REG_PERF_MONI_REG(OT_REG+i/2))>>(i%4*16))&0x3f;
        lastres.rd_ot[i] = (GET_REG(REG_PERF_MONI_REG(OT_REG+i/2))>>(i%4*16+8))&0x3f;
    }

    lastres.ddr_wr_bw = GET_REG(REG_PERF_MONI_REG(DDR_WR_BW_REG));
    lastres.ddr_rd_bw = GET_REG(REG_PERF_MONI_REG(DDR_RD_BW_REG));

    if (fh_databuff.addr != 0 && fh_databuff.cnt > 0)
    {
        u32 idx = (lastres.serial_cnt - 1) % fh_databuff.cnt;

        rt_memcpy((void *)(fh_databuff.addr + idx*sizeof(lastres)), &lastres, sizeof(lastres));
    }
}

static void fh_perf_mon_isr(int irq, void *param)
{
    u32 reg;

    update_lastresult();

    if (last_input_param.mode == FH_PERF_SINGLE)
        ctl_monitor(0);

    reg = GET_REG(REG_PERF_MONI_REG(0));
    reg |= BIT(30);
    SET_REG(REG_PERF_MONI_REG(0), reg);
    reg &= ~BIT(30);
    SET_REG(REG_PERF_MONI_REG(0), reg);

    return;
}

static void set_fh_perf_hw_param(struct fh_perf_param_input *input);

int fh_perf_start(int argc, char *argv[])
{
    int i;
    unsigned int t;
    unsigned int mode;
    unsigned int cnt;

    if (argc != 4)
    {
        rt_kprintf("err argument \r\n");
        rt_kprintf("fh_perf_start intervaltime mode data_count \r\n");
        return -1;
    }
    t = strtoul(argv[1], NULL, 10);
    mode = strtoul(argv[2], NULL, 10);
    cnt = strtoul(argv[3], NULL, 10);

    last_input_param.window_time = t;
    last_input_param.mode = mode;
    rt_kprintf("last_input_param.window_timet %x\r\n",last_input_param.window_time);
    rt_kprintf("last_input_param.mode %x\r\n",last_input_param.mode);

    /* last_input_param.mode = FH_PERF_CONTINUOUS; */
    last_input_param.ddr_bw = 1;
    for (i = 0; i < AXI_PORT_MAX; i++)
        last_input_param.axi_bw[i]  = 1;

    set_fh_perf_hw_param(&last_input_param);

    fh_databuff.cnt =  cnt;
    rt_kprintf("fh_databuff.cnt %x\r\n",fh_databuff.cnt);
    if (fh_databuff.addr != 0)
    {
        rt_free((void *)fh_databuff.addr);
        fh_databuff.addr = 0;
    }
    if (fh_databuff.cnt != 0)
        fh_databuff.addr = (u32)rt_malloc(sizeof(lastres)*fh_databuff.cnt);
    else
        fh_databuff.addr = (u32)NULL;

    rt_kprintf("fh_databuff.addr %x\r\n",fh_databuff.addr);
    ctl_monitor(1);
    return 0;
}
FINSH_FUNCTION_EXPORT_ALIAS(fh_perf_start, __cmd_fh_perf_start, start fh perf);

static void printdata(struct fh_perf_param_output *printres)
{
    int i = 0;

    rt_kprintf("last serial idx 0x%x\n",printres->serial_cnt);
    rt_kprintf("hw idx 0x%x\n",printres->hw_cnt);
    rt_kprintf("DDR WR BW %u (MB/s)\n",(printres->ddr_wr_bw/(last_input_param.window_time*1000)));
    rt_kprintf("DDR RD BW %u (MB/s)\n",(printres->ddr_rd_bw/(last_input_param.window_time*1000)));

    rt_kprintf("                ");
    for (i = 0; i < AXI_PORT_MAX; i++)
        rt_kprintf(" AXI_PORT_%02d ", i);

    rt_kprintf("\n");

    rt_kprintf("wr_ot         ");
    for (i = 0; i < AXI_PORT_MAX; i++)
        rt_kprintf("   %09u  ", printres->wr_ot[i]);

    rt_kprintf("\n");

    rt_kprintf("rd_ot         ");
    for (i = 0; i < AXI_PORT_MAX; i++)
        rt_kprintf("   %09u  ", printres->rd_ot[i]);

    rt_kprintf("\n");
    rt_kprintf("wr_cmd_cnt    ");
    for (i = 0; i < AXI_PORT_MAX; i++)
        rt_kprintf("   %09u  ", printres->wr_cmd_cnt[i]);

    rt_kprintf("\n");

    rt_kprintf("rd_cmd_cnt    ");
    for (i = 0; i < AXI_PORT_MAX; i++)
        rt_kprintf("   %09u  ", printres->rd_cmd_cnt[i]);

    rt_kprintf("\n");

    rt_kprintf("wr_cmd_byte   ");
    for (i = 0; i < AXI_PORT_MAX; i++)
        rt_kprintf("   %09u  ", printres->wr_cmd_byte[i]);

    rt_kprintf("\n");

    rt_kprintf("rd_cmd_byte   ");
    for (i = 0; i < AXI_PORT_MAX; i++)
        rt_kprintf("   %09u  ", printres->rd_cmd_byte[i]);

    rt_kprintf("\n");

    rt_kprintf("wr_cmd_bw(MB/s)");
    for (i = 0; i < AXI_PORT_MAX; i++)
        rt_kprintf("   %09u  ",
            printres->wr_cmd_byte[i]/(last_input_param.window_time*1000));

    rt_kprintf("\n");

    rt_kprintf("rd_cmd_bw(MB/s)");
    for (i = 0; i < AXI_PORT_MAX; i++)
        rt_kprintf("   %09u  ",
            printres->rd_cmd_byte[i]/(last_input_param.window_time*1000));

    rt_kprintf("\n");

    rt_kprintf("wr_sum_lat    ");
    for (i = 0; i < AXI_PORT_MAX; i++)
        rt_kprintf("   %09u  ", printres->wr_sum_lat[i]);

    rt_kprintf("\n");

    rt_kprintf("rd_sum_lat    ");
    for (i = 0; i < AXI_PORT_MAX; i++)
        rt_kprintf("   %09u  ", printres->rd_sum_lat[i]);

    rt_kprintf("\n");

    rt_kprintf("wr_cmd_cnt_lat");
    for (i = 0; i < AXI_PORT_MAX; i++)
        rt_kprintf("   %09u  ", printres->wr_cmd_cnt_lat[i]);

    rt_kprintf("\n");

    rt_kprintf("rd_cmd_cnt_lat");
    for (i = 0; i < AXI_PORT_MAX; i++)
        rt_kprintf("   %09u  ", printres->rd_cmd_cnt_lat[i]);

    rt_kprintf("\n");

    rt_kprintf("wr_av_lat(cycle)");
    for (i = 0; i < AXI_PORT_MAX; i++)
        rt_kprintf("   %09u  ",printres->wr_cmd_cnt_lat[i] == 0 ? 0 :
            printres->wr_sum_lat[i]/printres->wr_cmd_cnt_lat[i]);

    rt_kprintf("\n");

    rt_kprintf("rd_av_lat(cycle)");
    for (i = 0; i < AXI_PORT_MAX; i++)
        rt_kprintf("   %09u  ",printres->rd_cmd_cnt_lat[i] == 0 ? 0 :
            printres->rd_sum_lat[i]/printres->rd_cmd_cnt_lat[i]);

    rt_kprintf("\n");

}

static int fh_perf_show(void)
{
    int i = 0;

    ctl_monitor(0);
    rt_thread_delay(last_input_param.window_time > 100 ? last_input_param.window_time/10 : 10);
    rt_kprintf("\nLast FH Perf Monitor Status:\n");
    rt_kprintf("window time %u ms\n", last_input_param.window_time);

    if (fh_databuff.addr == (u32)NULL)
        printdata(&lastres);
    else
    {
        struct fh_perf_param_output *cur =
            (struct fh_perf_param_output *)fh_databuff.addr;

        for (i = 0; i < fh_databuff.cnt; i++)
        {
            printdata(cur);
            cur++;
        }
    }

    if (fh_databuff.addr != RT_NULL)
    {
        rt_free((void *)fh_databuff.addr);
        fh_databuff.addr = 0;
    }

    return 0;
}
FINSH_FUNCTION_EXPORT_ALIAS(fh_perf_show, __cmd_fh_perf_show,  show fh perf result);

static int  fh_perf_mon_probe(void *priv_data)
{
    fh_perf_mon_obj = *(struct fh_perf_mon_obj_t *)priv_data;
    fh_perf_clk = clk_get(NULL, "ahb_clk");
    if (fh_perf_clk == RT_NULL)
    {
        return -RT_ENOMEM;
    }
    rt_hw_interrupt_install(fh_perf_mon_obj.irq_no, fh_perf_mon_isr, NULL, "fh_perf_irq");
    rt_hw_interrupt_umask(fh_perf_mon_obj.irq_no);

    return 0;
}

static void set_fh_perf_hw_param(struct fh_perf_param_input *input)
{
    u32 reg;
    int i = 0;
    u32 rate;

    if (fh_perf_clk == NULL)
        rate = 200000;
    else
        rate = clk_get_rate(fh_perf_clk)/1000;

    reg = input->window_time*rate;
    SET_REG(REG_PERF_MONI_REG(1), reg);

    reg = GET_REG(REG_PERF_MONI_REG(0));
    if (input->ddr_bw)
        reg |= BIT(24);
    else
        reg &= ~BIT(24);

    if (input->mode == FH_PERF_SINGLE)
        reg &= ~BIT(28);
    else
        reg |= BIT(28);

    SET_REG(REG_PERF_MONI_REG(0), reg);

    for (i = 0; i < AXI_PORT_MAX; i++)
    {
        if (input->addr_param[i].used == 1)
        {
            reg = GET_REG(REG_PERF_MONI_REG(0));
            reg |= BIT((8+i));
            SET_REG(REG_PERF_MONI_REG(0), reg);

            SET_REG(REG_PERF_MONI_REG(ADDR_FILTER_REG+i), input->addr_param[i].filter);
            SET_REG(REG_PERF_MONI_REG(ADDR_MASK_REG+i), input->addr_param[i].mask);
        } else
        {
            reg = GET_REG(REG_PERF_MONI_REG(0));
            reg &= ~BIT((8+i));

            SET_REG(REG_PERF_MONI_REG(0), reg);
            SET_REG(REG_PERF_MONI_REG(ADDR_FILTER_REG+i), 0);
            SET_REG(REG_PERF_MONI_REG(ADDR_MASK_REG+i), 0);
        }
    }

    for (i = 0; i < AXI_PORT_MAX; i++)
    {
        if (input->id_param[i].used == 1)
        {
            reg = GET_REG(REG_PERF_MONI_REG(0));
            reg |= BIT((16+i));
            SET_REG(REG_PERF_MONI_REG(0), reg);

            reg |= input->id_param[i].filter|(input->id_param[i].mask<<16);
            SET_REG(REG_PERF_MONI_REG(ID_MASK_REG+i), reg);

        } else
        {
            reg = GET_REG(REG_PERF_MONI_REG(0));
            reg &= ~BIT((16+i));
            SET_REG(REG_PERF_MONI_REG(0), reg);

            SET_REG(REG_PERF_MONI_REG(ID_MASK_REG+i), 0);

        }
    }

    for (i = 0; i < AXI_PORT_MAX; i++)
    {
        if (input->axi_bw[i] == 1)
        {
            reg = GET_REG(REG_PERF_MONI_REG(0));
            reg |= BIT((i));
            SET_REG(REG_PERF_MONI_REG(0), reg);
        } else
        {
            reg = GET_REG(REG_PERF_MONI_REG(0));
            reg &= ~BIT((i));
            SET_REG(REG_PERF_MONI_REG(0), reg);
        }
    }

}
static void ctl_monitor(int on)
{
    u32 reg;

    if (on)
    {
        lastres.serial_cnt = 0;
        fh_perf_state = FH_PERF_RUNNING;
        if (fh_databuff.addr != 0 && fh_databuff.cnt > 0)
            rt_memset((void *)fh_databuff.addr, 0, sizeof(lastres)*fh_databuff.cnt);

        reg = GET_REG(REG_PERF_MONI_REG(0));
        reg |= BIT(31);
        SET_REG(REG_PERF_MONI_REG(0), reg);
    } else
    {
        if (FH_PERF_RUNNING == fh_perf_state)
            fh_perf_state = FH_PERF_STOP;

        reg = GET_REG(REG_PERF_MONI_REG(0));
        reg &= ~BIT(31);
        SET_REG(REG_PERF_MONI_REG(0), reg);
    }

}

int fh_perf_mon_exit(void *priv_data) { return 0; }
struct fh_board_ops fh_perf_driver_ops = {
    .probe = fh_perf_mon_probe, .exit = fh_perf_mon_exit,
};


void rt_hw_fhperf_init(void)
{
    fh_board_driver_register("fh_perf", &fh_perf_driver_ops);
};
