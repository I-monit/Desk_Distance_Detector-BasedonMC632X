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

#define AXI_PORT_MAX 5


struct fh_perf_mon_obj_t fh_perf_mon_obj;
struct fh_perf_param_output lastres;
struct clk  *fh_perf_clk;

static void ctl_monitor(int on);
static void set_fh_perf_hw_param(struct fh_perf_param_input *input);
static void fh_perf_mon_isr(int irq, void *param);

enum fh_perf_state_e
{
    FH_PERF_STOP,
    FH_PERF_RUNING,
} fh_perf_state;

struct fh_perf_param_input last_input_param;
struct fh_perf_data_buff fh_databuff;
#ifdef FH_PERF_V1_1
#define WR_CMD_CNT_REG 18
#define RD_CMD_CNT_REG 23
#define WR_CMD_BYTE_REG 28
#define RD_CMD_BYTE_REG 33
#define WR_SUM_LAT_REG 38
#define RD_SUM_LAT_REG 43
#define WR_CMD_CNT_LAT_REG 48
#define RD_CMD_CNT_LAT_REG 53
#define DDR_WR_BW_REG 58
#define DDR_RD_BW_REG 59
#define HWCNT_REG 60
#else
#define WR_CMD_CNT_REG 17
#define RD_CMD_CNT_REG 22
#define WR_CMD_BYTE_REG 27
#define RD_CMD_BYTE_REG 32
#define WR_SUM_LAT_REG 37
#define RD_SUM_LAT_REG 42
#define WR_CMD_CNT_LAT_REG 47
#define RD_CMD_CNT_LAT_REG 52
#define DDR_WR_BW_REG 57
#define DDR_RD_BW_REG 58
#define HWCNT_REG 59

#endif
static void update_lastresult(void)
{
    int i = 0;
    unsigned int regval = 0;

    lastres.serial_cnt++;
    /* ddr_bw */
    lastres.hw_cnt = GET_REG(REG_PERF_MONI_REG(59))&0xff;

    for (i = 0; i < AXI_PORT_MAX; i++)
    {
        lastres.wr_cmd_cnt[i] = GET_REG(REG_PERF_MONI_REG(WR_CMD_CNT_REG+i));
        lastres.rd_cmd_cnt[i] = GET_REG(REG_PERF_MONI_REG(RD_CMD_CNT_REG+i));
        lastres.wr_cmd_byte[i] = GET_REG(REG_PERF_MONI_REG(WR_CMD_BYTE_REG+i));
        lastres.rd_cmd_byte[i] = GET_REG(REG_PERF_MONI_REG(RD_CMD_BYTE_REG+i));
        lastres.wr_sum_lat[i] = GET_REG(REG_PERF_MONI_REG(WR_SUM_LAT_REG+i));
        lastres.rd_sum_lat[i] = GET_REG(REG_PERF_MONI_REG(RD_SUM_LAT_REG+i));
        lastres.wr_cmd_cnt_lat[i] = GET_REG(REG_PERF_MONI_REG(WR_CMD_CNT_LAT_REG+i));
        lastres.rd_cmd_cnt_lat[i] = GET_REG(REG_PERF_MONI_REG(RD_CMD_CNT_LAT_REG+i));
        switch (i)
        {
        default:
            break;
        case 0:
            lastres.wr_ot[i] = (GET_REG(REG_PERF_MONI_REG(15))>>0)&0x3f;
            lastres.rd_ot[i] = (GET_REG(REG_PERF_MONI_REG(15))>>8)&0x3f;
            break;
        case 1:
            lastres.wr_ot[i] = (GET_REG(REG_PERF_MONI_REG(15))>>16)&0x3f;
            lastres.rd_ot[i] = (GET_REG(REG_PERF_MONI_REG(15))>>24)&0x3f;
            break;
        case 2:
            lastres.wr_ot[i] = (GET_REG(REG_PERF_MONI_REG(16))>>0)&0x3f;
            lastres.rd_ot[i] = (GET_REG(REG_PERF_MONI_REG(16))>>8)&0x3f;
            break;
        case 3:
            lastres.wr_ot[i] = (GET_REG(REG_PERF_MONI_REG(16))>>16)&0x3f;
            lastres.rd_ot[i] = (GET_REG(REG_PERF_MONI_REG(16))>>24)&0x3f;
            break;
        case 4:
            regval = GET_REG(REG_PERF_MONI_REG(15));
            lastres.wr_ot[i] = ((regval>>6)&0x3)|((regval>>12)&(0x3<<2))|
                    ((regval>>18)&(0x3<<4));
            regval = GET_REG(REG_PERF_MONI_REG(16));
            lastres.rd_ot[i] = ((regval>>6)&0x3)|((regval>>12)&(0x3<<2))|
                    ((regval>>18)&(0x3<<4));
            break;


        }
    }

    lastres.ddr_wr_bw = GET_REG(REG_PERF_MONI_REG(DDR_WR_BW_REG));
    lastres.ddr_rd_bw = GET_REG(REG_PERF_MONI_REG(DDR_RD_BW_REG));




    if (fh_databuff.addr != 0 && fh_databuff.cnt > 0)
    {
        unsigned int idx = (lastres.serial_cnt - 1) % fh_databuff.cnt;

        rt_memcpy((void *)(fh_databuff.addr+idx*sizeof(lastres)), &lastres, sizeof(lastres));
    }
}

static void fh_perf_mon_isr(int irq, void *param)
{

    unsigned int reg;

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

    rt_kprintf("fh_databuff.addr %x\r\n", fh_databuff.addr);
    ctl_monitor(1);
    return 0;
}

FINSH_FUNCTION_EXPORT_ALIAS(fh_perf_start, __cmd_fh_perf_start,  start fh perf);







static void printdata(struct fh_perf_param_output *printres)
{
    int i = 0;

    rt_kprintf("last serial idx 0x%x\n", printres->serial_cnt);
    rt_kprintf("hw idx 0x%x\n", printres->hw_cnt);
    rt_kprintf("DDR WR BW %u (MB/s)\n", printres->ddr_wr_bw/(last_input_param.window_time*1000));
    rt_kprintf("DDR RD BW %u (MB/s) \n", printres->ddr_rd_bw/(last_input_param.window_time*1000));

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
        rt_kprintf("   %09u  ",
            printres->wr_cmd_cnt_lat[i] == 0 ? 0 :
            printres->wr_sum_lat[i]/printres->wr_cmd_cnt_lat[i]);

    rt_kprintf("\n");

    rt_kprintf("rd_av_lat(cycle)");
    for (i = 0; i < AXI_PORT_MAX; i++)
        rt_kprintf("   %09u  ",
            printres->rd_cmd_cnt_lat[i] == 0 ? 0 :
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
    rt_hw_interrupt_install(fh_perf_mon_obj.irq_no, fh_perf_mon_isr, NULL,
                            "fh_perf_irq");
    rt_hw_interrupt_umask(fh_perf_mon_obj.irq_no);

    return 0;
}



static void set_fh_perf_hw_param(struct fh_perf_param_input *input)
{
    u32 reg;
    int i = 0;
    u32 rate;

    /* window */
    if (fh_perf_clk == NULL)
        rate = 200000;
    else
        rate = clk_get_rate(fh_perf_clk)/1000;

    reg = input->window_time*rate;
    SET_REG(REG_PERF_MONI_REG(1), reg);

    /* ddr bw */
    reg = GET_REG(REG_PERF_MONI_REG(0));
    if (input->ddr_bw)
        reg |= BIT(24);
    else
        reg &= ~BIT(24);

    /* mode */
    if (input->mode == FH_PERF_SINGLE)
        reg &= ~BIT(28);
    else
        reg |= BIT(28);

    SET_REG(REG_PERF_MONI_REG(0), reg);


    /* axi addr filter */
    for (i = 0; i < AXI_PORT_MAX; i++)
    {
        if (input->addr_param[i].used == 1)
        {
            reg = GET_REG(REG_PERF_MONI_REG(0));
            reg |= BIT((8+i));
            SET_REG(REG_PERF_MONI_REG(0), reg);
            SET_REG(REG_PERF_MONI_REG(2+i), input->addr_param[i].filter);
            SET_REG(REG_PERF_MONI_REG(7+i), input->addr_param[i].mask);
        }
        else
        {
            reg = GET_REG(REG_PERF_MONI_REG(0));
            reg &= ~BIT((8+i));
            SET_REG(REG_PERF_MONI_REG(0), reg);
            SET_REG(REG_PERF_MONI_REG(2+i), 0);
            SET_REG(REG_PERF_MONI_REG(7+i), 0);
        }
    }
    /* axi id filter */
    for (i = 0; i < AXI_PORT_MAX; i++)
    {
        if (input->id_param[i].used == 1)
        {
            reg = GET_REG(REG_PERF_MONI_REG(0));
            reg |= BIT((16 + i));
            SET_REG(REG_PERF_MONI_REG(0), reg);
            switch (i)
            {
            case 0:
                reg = GET_REG(REG_PERF_MONI_REG(12));
                reg &= 0xffff0000;
                reg |= input->id_param[i].filter|(input->id_param[i].mask<<8);
                SET_REG(REG_PERF_MONI_REG(12), reg);
                break;
            case 1:
                reg = GET_REG(REG_PERF_MONI_REG(12));
                reg &= 0x0000ffff;
                reg |= (input->id_param[i].filter<<16)|(input->id_param[i].mask<<24);
                SET_REG(REG_PERF_MONI_REG(12), reg);
                break;
            case 2:
                reg = GET_REG(REG_PERF_MONI_REG(13));
                reg &= 0xFFFF0000;
                reg |= (input->id_param[i].filter<<0)|(input->id_param[i].mask<<8);
                SET_REG(REG_PERF_MONI_REG(13), reg);
                break;
            case 3:
                reg = GET_REG(REG_PERF_MONI_REG(13));
                reg &= 0x0000ffff;
                reg |= (input->id_param[i].filter<<16)|
                    (input->id_param[i].mask<<24);
                SET_REG(REG_PERF_MONI_REG(13), reg);
                break;
            case 4:
                reg = GET_REG(REG_PERF_MONI_REG(14));
                reg &= 0x0000ffff;
                reg |= (input->id_param[i].filter<<0)|
                    (input->id_param[i].mask<<8);
                SET_REG(REG_PERF_MONI_REG(14), reg);
                break;
            default:
                break;
            }
        }
        else
        {
            reg = GET_REG(REG_PERF_MONI_REG(0));
            reg &= ~BIT((16+i));
            SET_REG(REG_PERF_MONI_REG(0), reg);
            switch (i)
            {
            case 0:
                reg = GET_REG(REG_PERF_MONI_REG(12));
                reg &= 0xffff0000;
                SET_REG(REG_PERF_MONI_REG(12), reg);
                break;
            case 1:
                reg = GET_REG(REG_PERF_MONI_REG(12));
                reg &= 0x0000ffff;
                SET_REG(REG_PERF_MONI_REG(12), reg);
                break;
            case 2:
                reg = GET_REG(REG_PERF_MONI_REG(13));
                reg &= 0xFFFF0000;
                SET_REG(REG_PERF_MONI_REG(13), reg);
                break;
            case 3:
                reg = GET_REG(REG_PERF_MONI_REG(13));
                reg &= 0x0000ffff;
                SET_REG(REG_PERF_MONI_REG(13), reg);
                break;
            case 4:
                reg = GET_REG(REG_PERF_MONI_REG(14));
                reg &= 0xffff0000;
                SET_REG(REG_PERF_MONI_REG(14), reg);
                break;
            default:
                break;
            }
        }
    }
    /* axi bw en */
    for (i = 0; i < AXI_PORT_MAX; i++)
    {
        if (input->axi_bw[i] == 1)
        {
            reg = GET_REG(REG_PERF_MONI_REG(0));
            reg |= BIT((i));
            SET_REG(REG_PERF_MONI_REG(0), reg);
        }
        else
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
        fh_perf_state = FH_PERF_RUNING;
        if (fh_databuff.addr != 0 && fh_databuff.cnt > 0)
            rt_memset((void *)fh_databuff.addr, 0, sizeof(lastres)*fh_databuff.cnt);

        reg = GET_REG(REG_PERF_MONI_REG(0));
        reg |= BIT(31);
        SET_REG(REG_PERF_MONI_REG(0), reg);
    }
    else
    {
        if (FH_PERF_RUNING == fh_perf_state)
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
}


